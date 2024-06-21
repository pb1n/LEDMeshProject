//************************************************************
// THIS NODE IS THE ONLY ROOT NODE!
// DO NOT HAVE ANOTHER NODE AS A ROOT NODE OTHERWISE KAPUT
//
// This code allows an ESP32 to act as an Access Point for all other nodes in the mesh network 
// this is achieved by allocating the device as the ROOT node. This also speeds up mesh construction.
// No sensor readings are acquired 
//************************************************************

#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <set>  // Include the set header

// MESH Details
#define MESH_PREFIX "ARMMESH" //name for your MESH
#define MESH_PASSWORD "ARM12345678" //password for your MESH
#define MESH_PORT 5555 //default port

#define RXD2 10
#define TXD2 11

#define LED_MESH_CONNECTED 5 // Define the built-in LED pin for mesh connection

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

// Function declarations
void sendMessage(); // Prototype so PlatformIO doesn't complain
void sendJsonMessage(const String& event, uint32_t nodeId);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void receivedCallback(uint32_t from, String &msg);

// Needed for painless library
void sendMessage() {
  mesh.sendBroadcast("t");
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  Serial1.println(msg);
  Serial.printf("Forwarded to Serial1: %s\n", msg.c_str());
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, msg.c_str());
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
  Serial1.print("New Connection: "); Serial1.println(nodeId);
  sendJsonMessage("newConnection", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
  //Serial1.println("Changed Connections");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RXD2, TXD2); // For sending and receiving data to another ESP32

  // Initialize LED
  pinMode(LED_MESH_CONNECTED, OUTPUT);
  digitalWrite(LED_MESH_CONNECTED, LOW); // Ensure LED is off initially

  mesh.setDebugMsgTypes(ERROR | STARTUP);  // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.setRoot(true); 

  Serial1.print("Mesh AP Ready!");

  // Turn on the LED when the mesh network is initialized and the root node is set
  digitalWrite(LED_MESH_CONNECTED, HIGH);
}

void loop() {
  if (Serial1.available()) {
    String receivedMessage = Serial1.readStringUntil('\n');
    Serial.println("Received from Serial1: " + receivedMessage); // Print the received message for debugging
    receivedMessage.trim();
    mesh.sendBroadcast(receivedMessage);
    Serial.println("command broadcasted"); // Print the received message for debugging
  }
  // it will run the user scheduler as well
  mesh.update();
}

void sendJsonMessage(const String& event, uint32_t nodeId) {
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["event"] = event;
  jsonDoc["nodeId"] = String(nodeId);
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  Serial1.println(jsonString);
}
