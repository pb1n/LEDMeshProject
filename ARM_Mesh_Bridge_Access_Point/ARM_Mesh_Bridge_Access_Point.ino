//************************************************************
// THIS NODE IS THE ONLY ROOT NODE!
// DO NOT HAVE ANOTHER NODE AS A ROOT NODE OTHERWISE THERE WILL BE A POWER STRUGGLE AND THEN... KAPUT
//
// This code allows an ESP32 to act as an Access Point for all other nodes in the mesh network 
// this is achieved by allocating the device as the ROOT node. This also speeds up mesh construction.
// No sensor readings are acquired 
//************************************************************

//#include "DHT.h"
#include "painlessMesh.h"
#include <ArduinoJson.h>

// MESH Details
#define   MESH_PREFIX     "RNTMESH" //name for your MESH
#define   MESH_PASSWORD   "MESHpassword" //password for your MESH
#define   MESH_PORT       5555 //default port

#define RXD2 10
#define TXD2 11


String readings;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
//void sendMessage(); // Prototype so PlatformIO doesn't complain
//String getReadings(); // Prototype for sending sensor readings

//Create tasks: to send messages and get readings;
//Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage); //change message transmission rate

//String getReadings() {
//  StaticJsonDocument<200> jsonReadings;
//  jsonReadings["nodeId"] = String(mesh.getNodeId());  // Convert node ID to string
//  jsonReadings["temp"] = dht.readTemperature();
//  jsonReadings["hum"] = dht.readHumidity();

//  String result;
//  serializeJson(jsonReadings, result);
//  return result;
//}

//void sendMessage() {
//  String msg = getReadings();
//  mesh.sendBroadcast(msg);
//}

// Needed for painless library
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());

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
  
  //Serial.print("Node ID: ");
  //Serial.println(nodeId);
  //Serial.print("Temperature: ");
  //Serial.print(temp);
  //Serial.println(" C");
  //Serial.print("Humidity: ");
  //Serial.print(hum);
  //Serial.println(" %");

  Serial1.print("Node ID: ");
  Serial1.println(nodeId);
  Serial1.print("Temperature: ");
  Serial1.print(temp);
  Serial1.println(" C");
  Serial1.print("Humidity: ");
  Serial1.print(hum);
  Serial1.println(" %");
  Serial1.println(); //needed for effective phrasing if uart messages

}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
  Serial1.print("New Connection: "); Serial1.println(nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
  Serial1.println("Changed Connections");
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

  //userScheduler.addTask(taskSendMessage);
  //taskSendMessage.enable();
}

void loop() {

   if (Serial1.available()) {
    String receivedMessage = Serial1.readStringUntil('\n');
    Serial.println("Received from Serial1: " + receivedMessage); // Print the received message for debugging
    // You can also process the receivedMessage further if needed
  }
  // it will run the user scheduler as well
  mesh.update();
}
