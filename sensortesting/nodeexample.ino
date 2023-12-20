#include <LoRa.h>
#include <SPI.h>
#include <DHT.h>

#define NODE_ID 2        // Unique identifier for each node
#define GATEWAY_ID 0     // ID of the gateway
#define MAX_HOP_COUNT 3  // Maximum number of hops
#define ss 5
#define rst 14
#define dio0 2


// Create instances of the DHT and rain sensors
#define DHTPIN 4      // DHT22 sensor data pin is connected to GPIO 4
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0;  // will store last time LED was updated

// constants won't change:
const long interval = 5000; 

bool nodesDiscovered[255] = { false };  // Array to track discovered nodes
int hopCount[255] = { 0 };              // Array to track hop count for each node

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Receiver");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);

  dht.begin();
  
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
   // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  Serial.println("LoRa Initializing OK!");

  broadcastPresence();
}

void loop() {
  // Node Discover
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    broadcastPresence();
    delay(500);
    sendSensorData();

  }
  String LoRaData = ""; 
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");

    // read packet
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      Serial.print(LoRaData);
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
    processIncomingMessage(LoRaData);
  }
  // Send Sensor Data
  //
}

void broadcastPresence() {
  if (!nodesDiscovered[NODE_ID]) {
    Serial.println("Broadcasting presence");
    String message = "HELLO:" + String(NODE_ID) + ":" + String(hopCount[NODE_ID]) + ":SensorDataHere";
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();  // Adjust the interval based on your requirements
  }
}

void sendSensorData() {
  // Simulated sensor data
  float sensorValue = readTemperature();

  // Send sensor data
  String sensorMessage = "SENSOR:" + String(NODE_ID) + ":" + String(sensorValue);
  LoRa.beginPacket();
  LoRa.print(sensorMessage);
  LoRa.endPacket();
}

void processIncomingMessage(String message) {
  Serial.println("Received message: " + message);

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
    float receivedSensorValue = parts[3].toFloat();
    Serial.println("Received sensor data from Node " + String(senderID) + ": " + String(receivedSensorValue));
    // Add your logic here to handle the received sensor data
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

  Serial.println("Relaying message to Node " + String(destinationID) + " with hop count " + String(newHopCount));
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
}

void acknowledgeNode(int nodeID) {
  Serial.println("Acknowledging Node " + String(nodeID));
  // Send an acknowledgment message to the node
  String ackMessage = "ACK:" + String(nodeID);
  LoRa.beginPacket();
  LoRa.print(ackMessage);
  LoRa.endPacket();
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