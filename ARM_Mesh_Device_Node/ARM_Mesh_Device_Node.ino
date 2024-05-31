//************************************************************
// THIS IS A MESH CHILD NODE
// NODE MUST BE INFORMED THAT THERE IS A ROOT NODE IN THE SYSTEM!
// ID THE NODE IS NOT INFORMED THEN THIS NODE COULD BEEF WITH THE ROOT NODE AND THEN ... KAPUT
// 
// This code allows an ESP32 to be configured as a Child Node. Sensor data will be accquired and sent back to the Mesh Bridge.
// Requests will also be received and the LED matrix will be controlled
// The root node has an ID of 0
//************************************************************
#include "DHT.h"
#include "painlessMesh.h"
#include <ArduinoJson.h>

// MESH Details
#define   MESH_PREFIX     "RNTMESH" //name for your MESH
#define   MESH_PASSWORD   "MESHpassword" //password for your MESH
#define   MESH_PORT       5555 //default port

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

String readings;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage(); // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings

//Create tasks: to send messages and get readings;
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage); //change message transmission rate

String getReadings() {
  StaticJsonDocument<200> jsonReadings;
  jsonReadings["nodeId"] = String(mesh.getNodeId());  // Convert node ID to string
  jsonReadings["temp"] = dht.readTemperature();
  jsonReadings["hum"] = dht.readHumidity();

  String result;
  serializeJson(jsonReadings, result);
  return result;
}

void sendMessage() {
  String msg = getReadings();
  mesh.sendBroadcast(msg); // broadcasts to all nodes
  //mesh.sendSingle(0, msg); //broadcasts back to ROOT!
}

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
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);
  
  dht.begin();

  mesh.setDebugMsgTypes(ERROR | STARTUP);  // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.setContainsRoot(true); //tell node that there is root node at all time to speed up meshing
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}
