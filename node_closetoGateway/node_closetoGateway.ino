#include <LoRa.h>
#include <SPI.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>


#define NODE_ID 3        // Unique identifier for each node
#define GATEWAY_ID 0     // ID of the gateway
#define MAX_HOP_COUNT 3  // Maximum number of hops
#define ss 5
#define rst 14
#define dio0 2

#define uS_TO_S_FACTOR 1000000  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 10        // Time ESP32 will go to sleep (in seconds)

// Create instances of the DHT and rain sensors
#define DHTPIN 4      // DHT22 sensor data pin is connected to GPIO 4
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

#define POWER_PIN 23  // ESP32's pin GPIO32 that provides power to the rain sensor
#define AO_PIN 34 

RTC_DATA_ATTR int bootCount = 0;
#define DONEPIN 12


unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;  // will store last time LED was updated

// constants won't change:
const long interval = 2500; 
const long interval1 = 12000; 


RTC_DATA_ATTR bool nodesDiscovered[255] = { false };  // Array to track discovered nodes
RTC_DATA_ATTR int hopCount[255] = { 0 };              // Array to track hop count for each node

void setup() {
  Serial.begin(115200);
   ++bootCount;
  Serial.println(bootCount);
  while (!Serial);
  Serial.println("LoRa Receiver");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);

  pinMode(POWER_PIN, OUTPUT);

  pinMode(DONEPIN, OUTPUT);
  digitalWrite(DONEPIN, LOW);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,1); //1 = High, 0 = Low


  dht.begin();
  
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  // Check if ESP32 woke up from deep sleep
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER) {
    while (!LoRa.begin(915E6)) {
      Serial.println(".");
      delay(500);
    }

    Serial.println("LoRa Initializing OK!");
    LoRa_rxMode();
    // Node Discovery
    broadcastPresence();
  } else {
    Serial.println("Woke up from deep sleep!");
    while (!LoRa.begin(915E6)) {
      Serial.println(".");
      delay(500);
    }

    Serial.println("LoRa Initializing OK!");
    LoRa_rxMode();
    // Node Discovery
    broadcastPresence();
  }
  LoRa_rxMode();
}

void loop() {
  // Node Discover

  // Node Discover
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    if(bootCount == 0){
      broadcastPresence();
      delay(500);
    }
    sendSensorData();

  }
  String LoRaData = ""; 
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // read packet
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      //Serial.print(LoRaData);
    }
    if (LoRaData.endsWith("ID498")) {
      processIncomingMessage(LoRaData);
    }
  }
  unsigned long currentMillis1 = millis();
  if (currentMillis1 - previousMillis1 >= interval1) {
    // save the last time you blinked the LED
    previousMillis1 = currentMillis1;

    Serial.println("Going to sleep...");
    delay(1000); // Allow serial messages to be sent before sleeping
    digitalWrite(DONEPIN, HIGH);
    delay(1);
    digitalWrite(DONEPIN, LOW);
    delay(1);
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();

  }
}

void broadcastPresence() {
  if (!nodesDiscovered[NODE_ID]) {
    Serial.println("Broadcasting presence");
    String message = "HELLO:" + String(NODE_ID) + ":" + String(hopCount[NODE_ID]) + ":SensorDataHere" + "ID498";
    Serial.println(message);
    LoRa_sendMessage(message); // Adjust the interval based on your requirements
  }
}

void sendSensorData() {
  // Simulated sensor data
  float sensorValue = readTemperature();

  digitalWrite(POWER_PIN, HIGH);  // turn the rain sensor's power ON
  delay(10);                      // wait 10 milliseconds

  int rain_value = analogRead(AO_PIN);

  digitalWrite(POWER_PIN, LOW); 

  // Send sensor data
  String sensorMessage = "SENSOR:" + String(NODE_ID) + ":" + String(sensorValue) + ":" + String(rain_value) + ":ID498" ;
  LoRa_sendMessage(sensorMessage);
}

void processIncomingMessage(String message) {
  // Serial.println("Received message: " + message);

  // Parse the incoming message
  String parts[4];
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

  // Check if the message is from a sensor node
  if (parts[0] == "HELLO" && senderID != NODE_ID) {
    Serial.println("Message from Node " + String(senderID) + " with hop count " + String(senderHopCount));

    // Mark the node as discovered
    nodesDiscovered[senderID] = true;

    // Relay the message to the gateway if hop count is within the limit
    if (senderHopCount < MAX_HOP_COUNT) {
      relayMessage(message);

      // Acknowledge the node
      acknowledgeNode(senderID);
    }
  } else if (parts[0] == "SENSOR" && senderID != NODE_ID) {
    // Process sensor data received from other nodes
    float receivedSensorValue1 = parts[2].toFloat();
    float receivedSensorValue2 = parts[3].toFloat();
    Serial.println("Received sensor data from Node " + String(senderID) + ": " + String(receivedSensorValue1) + "C, Rain Value:"+ String(receivedSensorValue2));
    relayMessage(message);
  } else if (parts[0] == "ACK" && senderID == NODE_ID) {
    Serial.println("Acknowledgment received from the gateway");
    nodesDiscovered[NODE_ID] = true;
    Serial.println(nodesDiscovered[NODE_ID]);
  }
}

void relayMessage(String message) {
  // Increase hop count before relaying
  int colonPos = message.indexOf(':');
  int newHopCount = message.substring(colonPos + 1).toInt() + 1;

  // Extract destination ID
  colonPos = message.indexOf(':', colonPos + 1);
  int destinationID = message.substring(colonPos + 1).toInt();

  // Update hop count for the destination
  hopCount[destinationID] = newHopCount;

  Serial.println("Relaying message to Node " + String(destinationID) + " with hop count " + String(newHopCount) + "ID498");
  LoRa_sendMessage(message); 

}

void acknowledgeNode(int nodeID) {
  Serial.println("Acknowledging Node " + String(nodeID));
  // Send an acknowledgment message to the node
  String ackMessage = "ACK:" + String(nodeID) + "ID498";
  LoRa_sendMessage(ackMessage); 
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

void LoRa_rxMode(){
  LoRa.enableInvertIQ();                // active invert I and Q signals
  LoRa.receive();                       // set receive mode
  LoRa.setSignalBandwidth(62.5E3);
}

void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
  LoRa.disableInvertIQ();               // normal mode
  LoRa.setSignalBandwidth(125E3);
}

void LoRa_sendMessage(String message) {
  LoRa_txMode();                        // set tx mode
  LoRa.beginPacket();                   // start packet
  LoRa.print(message);                  // add payload
  LoRa.endPacket();                 // finish packet and send it
  LoRa_rxMode();
}
