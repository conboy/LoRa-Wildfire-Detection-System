#include <LoRa.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

const char *ssid = "aquarium 22/23";
const char *password = "ComeOnNow69420";
const char *mapboxApiKey = "pk.eyJ1IjoiMTlsZzI2IiwiYSI6ImNscnM3dGoyejAzeHIyc3RneGhpN2Z0bmUifQ.KY8X1N559UeOWP-yQeQ_YA";

AsyncWebServer server(80);

struct Coordinates {
  float latitude;
  float longitude;
};

const int MAX_NODES = 50;  // Maximum number of nodes
Coordinates nodeCoordinates[MAX_NODES];
bool nodesDiscovered[255] = { false };
bool newCoordinatesReceived = true;


void setup() {
  Serial.begin(115200);
  while (!Serial);

  // WiFi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // LoRa initialization
  LoRa.setPins(5, 14, 2);
  if (!LoRa.begin(915E6)) {
    Serial.println("LoRa initialization failed. Check your wiring.");
    while (1);
  }

  Serial.println("LoRa initialized");
  LoRa_rxMode();

  nodeCoordinates[0] = { 44.5, -77.2 };  // Node 1
  nodeCoordinates[1] = { 45.0, -78.0 };  // Node 2

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Check if new coordinates have been received
    if (newCoordinatesReceived) {
      newCoordinatesReceived = false;
      String html = "<html>"
                    "<head>"
                    "<title>Mapbox Example</title>"
                    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                    "<script src='https://api.mapbox.com/mapbox-gl-js/v2.8.1/mapbox-gl.js'></script>"
                    "<link href='https://api.mapbox.com/mapbox-gl-js/v2.8.1/mapbox-gl.css' rel='stylesheet' />"
                    "</head>"
                    "<body>"
                    "<div id='map' style='width: 100%; height: 100%;'></div>"
                    "<script>"
                    "mapboxgl.accessToken = '"
                    + String(mapboxApiKey) + "';"
                                             "var map = new mapboxgl.Map({"
                                             "container: 'map',"
                                             "style: 'mapbox://styles/mapbox/streets-v11',"
                                             "center: [0, 0],"
                                             "zoom: 10"
                                             "});"
                                             "var markers = {};"
                                             "function updateCoordinates() {"
                                             "fetch('/getCoordinates')"
                                             ".then(response => response.json())"
                                             ".then(data => {"
                                             "data.forEach(node => {"
                                             "addOrUpdateMarker(node);"
                                             "});"
                                             "});"
                                             "}"
                                             "function addOrUpdateMarker(node) {"
                                             "var markerId = 'marker-' + node.id;"
                                             "var existingMarker = markers[markerId];"
                                             "if (!existingMarker) {"
                                             "markers[markerId] = new mapboxgl.Marker()"
                                             ".setLngLat([node.longitude, node.latitude])"
                                             ".addTo(map);"
                                             "}"
                                             "}"
                                             "setInterval(updateCoordinates, 2000);"
                                             "</script>"
                                             "</body>"
                                             "</html>";

      request->send(200, "text/html", html);
    }else {
      request->send(304);  // Not Modified
    }
  });

  // Provide coordinates via /getCoordinates endpoint
  server.on("/getCoordinates", HTTP_GET, [](AsyncWebServerRequest *request) {
    String jsonResponse = "[";
    for (int i = 0; i < MAX_NODES; i++) {
      if (nodeCoordinates[i].latitude != NULL) {
        jsonResponse += "{\"id\":" + String(i) + ",\"latitude\":" + String(nodeCoordinates[i].latitude, 6) + ",\"longitude\":" + String(nodeCoordinates[i].longitude, 6) + "},";
      }
    }
    jsonResponse.remove(jsonResponse.length() - 1);  // Remove the trailing comma
    jsonResponse += "]";
    //Serial.println(jsonResponse);
    request->send(200, "application/json", jsonResponse);
  });

  server.begin();
}

void loop() {

  int packetSize = LoRa.parsePacket();
  String LoRaData = "";
  if (packetSize) {
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
    }
    if (LoRaData.endsWith("ID498")) {
      processIncomingMessage(LoRaData);
    }
  }
}

void processIncomingMessage(String message) {
  String parts[8];  // tweak
  int partIndex = 0;
  for (int i = 0; i < message.length(); i++) {
    if (message.charAt(i) == ':') {
      partIndex++;
    } else {
      parts[partIndex] += message.charAt(i);
    }
  }

  int senderID = parts[1].toInt();
  int senderHopCount = parts[2].toInt();

  if (parts[0] == "HELLO" && senderID != 0) {
    Serial.println("Message from Node " + String(senderID) + " with hop count " + String(senderHopCount));

    float latitude = parts[4].toFloat();
    float longitude = parts[6].toFloat();

    nodeCoordinates[senderID].latitude = latitude;
    nodeCoordinates[senderID].longitude = longitude;
    newCoordinatesReceived = true;

    Serial.println("Latitude: " + String(latitude, 6) + " | Longitude: " + String(longitude, 6));

    acknowledgeNode(senderID);
  } else if (parts[0] == "ACK" && senderID != 0) {
    Serial.println("Acknowledgment from Node " + String(senderID));
    nodesDiscovered[senderID] = true;
  } else {
    processSensorData(message);
  }
}

void processSensorData(String message) {
  String parts[8];  // tweak
  int partIndex = 0;
  for (int i = 0; i < message.length(); i++) {
    if (message.charAt(i) == ':') {
      partIndex++;
    } else {
      parts[partIndex] += message.charAt(i);
    }
  }

  float temp = parts[2].toFloat();
  float humidity = parts[3].toFloat();
  float rainValue = parts[4].toFloat();

  Serial.println("Temperature: " + String(temp, 2) + " | Humidity: " + String(humidity, 2) + " | Rain Value: " + String(rainValue, 2));
}

void acknowledgeNode(int nodeID) {
  Serial.println("Acknowledging Node " + String(nodeID));
  String ackMessage = "ACK:" + String(nodeID) + " ID498";
  Serial.println(ackMessage);
  delay(1000);
  LoRa_sendMessage(ackMessage);
}


void LoRa_rxMode() {
  LoRa.disableInvertIQ();
  LoRa.receive();
  LoRa.setSignalBandwidth(125E3);
}

void LoRa_txMode() {
  LoRa.idle();
  LoRa.enableInvertIQ();
  LoRa.setSignalBandwidth(62.5E3);
}

void LoRa_sendMessage(String message) {
  LoRa_txMode();
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
  LoRa_rxMode();
}
