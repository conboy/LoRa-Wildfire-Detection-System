#include <LoRa.h>
#include <SPI.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <TinyGPSPlus.h>

#define NODE_ID 3        // Unique identifier for each node
#define GATEWAY_ID 0     // ID of the gateway
#define MAX_HOP_COUNT 5  // Maximum number of hops
#define ss 5
#define rst 14
#define dio0 2

#define uS_TO_S_FACTOR 60000000  // Conversion factor for micro seconds to minutes
#define TIME_TO_SLEEP 1     // Time ESP32 will go to sleep (in minuntes)

// Create instances of the DHT and rain sensors
#define DHTPIN 4      // DHT22 sensor data pin is connected to GPIO 4
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

#define POWER_PIN 22  // ESP32's pin GPIO32 that provides power to the rain sensor
#define AO_PIN 34 // Pin to read rain sensor value

RTC_DATA_ATTR int bootCount = 0; // boot count, stored in RTC data that is not cleared on deep sleep wakeup/reset
#define DONEPIN 12 // May need to use this for the TPL5110 timer ()

unsigned long previousMillis = 0;

const long interval = 2500; // Sending sensor data interval

//GPS Stuff
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;

//RTC data, won't be wiped on sleep wakeup
RTC_DATA_ATTR bool nodesDiscovered[255] = { false };  // Array to track discovered nodes
RTC_DATA_ATTR int hopCount[255] = { 0 };              // Array to track hop count for each node

void setup() {
  Serial.begin(115200);

  //Serial 2 for GPS Serial Port
  Serial2.begin(9600, SERIAL_8N1, RXPin, TXPin);

  ++bootCount;
  Serial.println(bootCount);
  
  while (!Serial);
    Serial.println("LoRa Node(close)");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);

  //Power pin for rain sensor
  pinMode(POWER_PIN, OUTPUT);

  //This section is for the TPL timer (not used as of now)
  pinMode(DONEPIN, OUTPUT);
  digitalWrite(DONEPIN, LOW);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,1); //1 = High, 0 = Low

  //Start the temp sensor
  dht.begin();
  
  // Check if ESP32 woke up from deep sleep or not
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER) { // did not wake up from sleep (either external factor or simply first boot)
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
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    //Only broadcast the presence if the boot count is 0, or if not acked ()
    if(bootCount == 0 || nodesDiscovered[NODE_ID] == false){
      broadcastPresence();
    }
    sendSensorData();

  }
  String LoRaData = ""; 
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // read packet
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
    }
    //Check packet ID to ensure we are not reading noise, each packet has this ID to ensure system works as expected
    if (LoRaData.endsWith("ID498")) {
      processIncomingMessage(LoRaData);
    }
  }
  /*
  * GPS SECTION
  */
  while (Serial2.available() > 0)
    gps.encode(Serial2.read());

  // Check if GPS has a fix and the minutes are at intervals of 2 
  // only go to sleep if the gps has a fix
  if (gps.location.isUpdated() && gps.time.isUpdated() && (gps.time.minute() % 2 == 0) && (gps.time.second() == 30)) {
    Serial.println("Going to sleep for x amount of time");
    delay(1000);
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  }
  /*
  *END GPS SECTION
  */

}

///This code will poll here until the GPS aquires a fix, once it does, it will send the broadcast message
// along with its coordinates so that the gateway knows where the node is
void broadcastPresence() {
  if (!nodesDiscovered[NODE_ID]) { 
    Serial.println("Broadcasting presence");

    // Wait for a valid GPS fix
    while (!gps.location.isValid()) {
      while (Serial2.available() > 0) {
        gps.encode(Serial2.read());
      }
    }

    // GPS fix acquired, proceed with your code
    Serial.println("GPS Fix Acquired!");
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);

    // Create the message with GPS coordinates
    String message = "HELLO:" + String(NODE_ID) + ":" + String(hopCount[NODE_ID]) +
                     ":Latitude=" + String(gps.location.lat(), 6) +
                     ":Longitude=" + String(gps.location.lng(), 6) +
                     ":ID498";

    // Send the message using LoRa
    LoRa_sendMessage(message);

    // Add a delay or other logic as needed
    delay(500);
  }
}


void sendSensorData() {
  float temp = readTemperature(); 
  float humidity = readHumidity();  

  digitalWrite(POWER_PIN, HIGH);  // turn the rain sensor's power ON
  delay(10);                      // wait 10 milliseconds

  int rain_value = analogRead(AO_PIN);

  digitalWrite(POWER_PIN, LOW); // Turn off the sensor to save power 

  // Send sensor data
  String sensorMessage = "SENSOR:" + String(NODE_ID) + ":" + String(temp) + ":" + String(humidity) +  ":" + String(rain_value) + ":ID498" ;
  LoRa_sendMessage(sensorMessage);
}

void processIncomingMessage(String message) {
  // Parse the incoming message
  String parts[5]; // figure out the max length of this 
  int partIndex = 0;
  for (int i = 0; i < message.length(); i++) {
    if (message.charAt(i) == ':') {
      partIndex++;
    } else {
      parts[partIndex] += message.charAt(i);
    }
  }

  //Figure out a way to only do this if the packet is not data or something???
  //Maybe add the IDS and hop counts to the front??
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

    //Work on this whole section
  } else if (parts[0] == "SENSOR" && senderID != NODE_ID) {
    // Process sensor data received from other nodes
    float receivedSensorValue1 = parts[2].toFloat();
    float receivedSensorValue2 = parts[3].toFloat();
    //Add a section for the humidity
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

float readHumidity() {
  float humidity = dht.readHumidity();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return 0.0;
  }

  return humidity;
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
