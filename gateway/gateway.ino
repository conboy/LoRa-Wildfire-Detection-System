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
  LoRa_rxMode();
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
    if(LoRaData.endsWith("ID498")){
      processIncomingMessage(LoRaData);
    }
  }
}

void processIncomingMessage(String message) {

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
  if (parts[0] == "HELLO" && senderID != GATEWAY_ID) {
    Serial.println("Message from Node " + String(senderID) + " with hop count " + String(senderHopCount));

    processSensorData(message);

    // Acknowledge the node
    acknowledgeNode(senderID);
  } else if (parts[0] == "ACK" && senderID != GATEWAY_ID) {
    Serial.println("Acknowledgment from Node " + String(senderID));
    // Handle the acknowledgment as needed
  }
  else{
    Serial.println("Received message: " + message);
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
  String ackMessage = "ACK:" + String(nodeID) + " ID498";
  Serial.println(ackMessage);
  delay(1000);  // Increase the delay after sending ACK
  LoRa_sendMessage(ackMessage);
}

void LoRa_rxMode(){
  LoRa.disableInvertIQ();               // normal mode
  LoRa.receive();                       // set receive mode
    LoRa.setSignalBandwidth(125E3);

}

void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
  LoRa.enableInvertIQ();                // active invert I and Q signals
  LoRa.setSignalBandwidth(62.5E3);
}

void LoRa_sendMessage(String message) {
  LoRa_txMode();                        // set tx mode
  LoRa.beginPacket();                   // start packet
  LoRa.print(message);                  // add payload
  LoRa.endPacket();                 // finish packet and send it
  LoRa_rxMode();
}

