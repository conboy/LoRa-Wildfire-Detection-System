#include <LoRa.h>

#define GATEWAY_ID 0 // Unique identifier for the gateway
#define ss 5
#define rst 14
#define dio0 2

struct Coordinates {
  float latitude;
  float longitude;
};

const int MAX_NODES = 255;  // Adjust the size based on your requirements

RTC_DATA_ATTR bool nodesDiscovered[255] = { false };  // Array to track discovered nodes
Coordinates nodeCoordinates[MAX_NODES];

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
    }
    // Process the incoming message
    if(LoRaData.endsWith("ID498")){
      processIncomingMessage(LoRaData);
    }
  }
}

void processIncomingMessage(String message) {
  // Parse the incoming message
  String parts[8]; // tweak
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

    float latitiude = parts[4].toFloat();
    float longitude = parts[6].toFloat();

    nodeCoordinates[senderID].latitude = parts[4].toFloat();
    nodeCoordinates[senderID].longitude = parts[6].toFloat();

    //Next step is send to webserver to plot coordinates
    Serial.println("Latitude: " + String(latitiude, 6) + " | Longitude: " + String(longitude, 6));

    // Acknowledge the node
    acknowledgeNode(senderID);

    Serial.println(message);
  } else if (parts[0] == "ACK" && senderID != GATEWAY_ID) {
    Serial.println("Acknowledgment from Node " + String(senderID));
    // Handle the acknowledgment as needed
    nodesDiscovered[senderID] = true; //ack the node and store it in the array (keep track of it)
  }
  else{
    // Serial.println("Received message: " + message);
    processSensorData(message);
  }
}

void processSensorData(String message) {
  // Extract sensor data from the message and perform actions as needed
  String parts[8]; // tweak
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

  //Send to webserver to be displayed on GUI 
  Serial.println("Temperature: " + String(temp, 2) + " | Humidity: " + String(humidity, 2) + " | Rain Value: " + String(rainValue, 2));

}

void acknowledgeNode(int nodeID) {
  Serial.println("Acknowledging Node " + String(nodeID));
  // Send an acknowledgment message to the node
  String ackMessage = "ACK:" + String(nodeID) + " ID498";
  Serial.println(ackMessage);
  delay(1000); 
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

