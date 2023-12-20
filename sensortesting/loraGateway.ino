#include <LoRa.h>

#define GATEWAY_ID 0 // Unique identifier for the gateway
#define ss 5
#define rst 14
#define dio0 2


void setup() {
  Serial.begin(115200);
  while (!Serial);
  LoRa.setPins(ss, rst, dio0);


  if (!LoRa.begin(915E6)) {
    Serial.println("LoRa initialization failed. Check your wiring.");
    while (1);
  }
  Serial.println("all good");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  String LoRaData = "";
  if (packetSize) {
    // received a packet

    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      // Serial.print(LoRaData); 
    }
    // Process the incoming message
    processIncomingMessage(LoRaData);
  }
}

void processIncomingMessage(String message) {
  Serial.println("Received message: " + message);

  // Parse the incoming message
  String parts[3];
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
  if (parts[0] == "HELLO" && senderID != GATEWAY_ID) {
    Serial.println("Message from Node " + String(senderID) + " with hop count " + String(senderHopCount));

    processSensorData(message);

    // Acknowledge the node
    acknowledgeNode(senderID);
  } else if (parts[0] == "ACK" && senderID != GATEWAY_ID) {
    Serial.println("Acknowledgment from Node " + String(senderID));
    // Handle the acknowledgment as needed
  }
}

void processSensorData(String message) {
  // Extract sensor data from the message and perform actions as needed
  // Serial.println("Processing sensor data...");
  // Serial.println("Sensor Data: " + message);
}

void acknowledgeNode(int nodeID) {
  Serial.println("Acknowledging Node " + String(nodeID));
  // Send an acknowledgment message to the node
  String ackMessage = "ACK:" + String(nodeID);
  delay(1000);  // Increase the delay after sending ACK
  LoRa.beginPacket();
  LoRa.print(ackMessage);
  LoRa.endPacket();
}