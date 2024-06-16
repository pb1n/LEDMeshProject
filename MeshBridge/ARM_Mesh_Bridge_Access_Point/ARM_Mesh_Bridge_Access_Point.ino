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
#define MESH_PREFIX "RNTMESH" //name for your MESH
#define MESH_PASSWORD "MESHpassword" //password for your MESH
#define MESH_PORT 5555 //default port

#define RXD2 10
#define TXD2 11

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

  String nodeIdStr = doc["nodeId"];
  uint32_t nodeId = strtoul(nodeIdStr.c_str(), NULL, 10);
  double temp = doc["temp"];
  double hum = doc["hum"];
  
  Serial.print("Node ID: ");
  Serial.println(nodeId);
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" C");
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.println(" %");
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
  Serial1.print("New Connection: "); Serial1.println(nodeId);
  sendJsonMessage("newConnection", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
  //Serial1.println("Changed Connections");

  // Track nodes to detect disconnections
  static std::set<uint32_t> knownNodes;
  std::list<uint32_t> currentNodesList = mesh.getNodeList(true);
  std::set<uint32_t> currentNodes(currentNodesList.begin(), currentNodesList.end());

  for (const auto& nodeId : knownNodes) {
    if (currentNodes.find(nodeId) == currentNodes.end()) {
      // Node is no longer in the current list, it was disconnected
      sendJsonMessage("nodeDisconnected", nodeId);
    }
  }

  // Update known nodes
  knownNodes = currentNodes;
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RXD2, TXD2); // For sending and receiving data to another ESP32
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.setRoot(true); 

  Serial1.print("Mesh AP Ready!");
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
