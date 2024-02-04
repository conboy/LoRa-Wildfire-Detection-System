#include <LoRa.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// const char *ssid = "aquarium 22/23";
const char *ssid = "QHDTESPhotspot";
const char *password = "qhdt1234";
// const char *password = "ComeOnNow69420";
const char *mapboxApiKey = "pk.eyJ1IjoiMTlsZzI2IiwiYSI6ImNscnM3dGoyejAzeHIyc3RneGhpN2Z0bmUifQ.KY8X1N559UeOWP-yQeQ_YA";

AsyncWebServer server(80);

//Make a new struct to accomodate all of the data that is sent to the gateway
struct Coordinates {
  float latitude;
  float longitude;
};

const int MAX_NODES = 255;  // Maximum number of nodes
Coordinates nodeCoordinates[MAX_NODES];
bool nodesDiscovered[MAX_NODES] = { false };
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
  } else if (nodesDiscovered[senderID] != false) {
    //We only want to process acknowledged nodes
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
  nodesDiscovered[nodeID] = true;
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

/*
  Rui Santos
  Complete project details at Complete project details at https://RandomNerdTutorials.com/esp32-http-get-post-arduino/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <WiFi.h>
#include <HTTPClient.h>

const char *ssid = "QHDTESPhotspot";
const char *password = "qhdt1234";

//Your Domain name with URL path or IP address with path
const char* serverName = "https://lo-ra-wildfire-detection-system.vercel.app/api";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;
    
      // Your Domain name with URL path or IP address with path
      http.begin(serverName);
      
      // If you need Node-RED/server authentication, insert user and password below
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
      
      // Specify content-type header
      //http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      // Data to send with HTTP POST
      //String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME280&value1=24.25&value2=49.54&value3=1005.14";           
      // Send HTTP POST request
      //int httpResponseCode = http.POST(httpRequestData);
      
      // If you need an HTTP request with a content type: application/json, use the following:
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST("{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}");

      // If you need an HTTP request with a content type: text/plain
      // http.addHeader("Content-Type", "text/plain");
      // int httpResponseCode = http.POST("Hello, World!");
     
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);

      Serial.println(http.getString());
        
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}