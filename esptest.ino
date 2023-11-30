#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Replace with your network credentials
const char* ssid = "enteryourwifiname";
const char* password = "enteryourwifipassword";

// Create instances of the DHT and rain sensors
#define DHTPIN 4      // DHT22 sensor data pin is connected to GPIO 4
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

#define POWER_PIN 23  // ESP32's pin GPIO32 that provides power to the rain sensor
#define AO_PIN 2      // ESP32's pin GPIO36 connected to AO pin of the rain sensor

// Create an instance of the server
AsyncWebServer server(80);

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Print ESP32 IP address
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize DHT sensor
  dht.begin();

  // Configure rain sensor power pin
  pinMode(POWER_PIN, OUTPUT);

  // Serve the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><meta http-equiv='refresh' content='2'></head><body>";  // Refresh every 2 seconds
    html += "<h1>Environment Monitoring</h1>";
    html += "<p id='temperature'>Temperature: " + String(readTemperature()) + " &#8451;</p>";
    html += "<p id='rain'>Rain Value: " + String(readRainValue()) + "</p>";
    html += "<script>function updateData() {"
            "var xhrTemp = new XMLHttpRequest();"
            "xhrTemp.onreadystatechange = function() {"
            "if (xhrTemp.readyState == 4 && xhrTemp.status == 200) {"
            "document.getElementById('temperature').innerHTML = 'Temperature: ' + xhrTemp.responseText + ' &#8451;';"
            "}"
            "};"
            "xhrTemp.open('GET', '/temperature', true);"
            "xhrTemp.send();"

            "var xhrRain = new XMLHttpRequest();"
            "xhrRain.onreadystatechange = function() {"
            "if (xhrRain.readyState == 4 && xhrRain.status == 200) {"
            "document.getElementById('rain').innerHTML = 'Rain Value: ' + xhrRain.responseText;"
            "}"
            "};"
            "xhrRain.open('GET', '/rain', true);"
            "xhrRain.send();"
            "}"
            "setInterval(updateData, 2000);"
            "</script>";  // Update every 2 seconds
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Serve the temperature data separately
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    String temp = String(readTemperature());
    request->send(200, "text/plain", temp);
  });

  // Serve the rain sensor data separately
  server.on("/rain", HTTP_GET, [](AsyncWebServerRequest *request){
    String rainValue = String(readRainValue());
    request->send(200, "text/plain", rainValue);
  });

  // Start server
  server.begin();
}

void loop() {
  // Nothing to do here
}

float readTemperature() {
  // Read temperature in Celsius
  float temperatureC = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(temperatureC)) {
    Serial.println("Failed to read from DHT sensor!");
    return 0.0;
  }

  return temperatureC;
}

int readRainValue() {
  digitalWrite(POWER_PIN, HIGH);  // turn the rain sensor's power ON
  delay(10);                      // wait 10 milliseconds

  int rain_value = analogRead(AO_PIN);

  digitalWrite(POWER_PIN, LOW);  // turn the rain sensor's power OFF

  return rain_value;
}