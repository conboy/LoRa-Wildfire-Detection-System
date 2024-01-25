#include <WiFi.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "The Chicken Coop";
const char* password = "Nellysworld";
const char* mapboxApiKey = "pk.eyJ1IjoiMTlsZzI2IiwiYSI6ImNscnM3dGoyejAzeHIyc3RneGhpN2Z0bmUifQ.KY8X1N559UeOWP-yQeQ_YA";

float latitude = 44.5; // Initialize latitude
float longitude = -77.2; // Initialize longitude

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html>"
                    "<head>"
                      "<title>Mapbox Example</title>"
                      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                      "<script src='https://api.mapbox.com/mapbox-gl-js/v2.8.1/mapbox-gl.js'></script>"
                      "<link href='https://api.mapbox.com/mapbox-gl-js/v2.8.1/mapbox-gl.css' rel='stylesheet' />"
                    "</head>"
                    "<body>"
                      "<div id='map' style='width: 100%; height: 400px;'></div>"
                      "<script>"
                        "mapboxgl.accessToken = '" + String(mapboxApiKey) + "';"
                        "var latitude = " + String(latitude) + ";"
                        "var longitude = " + String(longitude) + ";"
                        "var map = new mapboxgl.Map({"
                          "container: 'map',"
                          "style: 'mapbox://styles/mapbox/streets-v11',"
                          "center: [longitude, latitude],"
                          "zoom: 14"
                        "});"
                        "var marker = new mapboxgl.Marker().setLngLat([longitude, latitude]).addTo(map);"
                        "function updateCoordinates() {"
                          "latitude += 1;"
                          "longitude += 1;"
                          "map.setCenter([longitude, latitude]);"
                          "marker.setLngLat([longitude, latitude]);"
                        "}"
                        "setInterval(updateCoordinates, 5000);"
                      "</script>"
                    "</body>"
                  "</html>";

    request->send(200, "text/html", html);
  });

  server.begin();
}

void loop() {
  // Your loop code here
}
