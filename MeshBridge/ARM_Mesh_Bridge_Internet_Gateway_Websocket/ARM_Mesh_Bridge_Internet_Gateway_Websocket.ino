//************************************************************
// This code is used in conjunction with the mesh AP to form a mesh-internet bridge:
//1) Receive sensor readings and statuses of all nodes in the network from the mesh AP via UART and
//send to webpage and database
//2) Forward requests via UART to mesh AP to communicate to mesh nodes
//************************************************************
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// Serial2 pins of ESP32
#define RXD2 16
#define TXD2 17

#define debuggingMode


// Replace with your network credentials
const char* ssid = "mesh123";
const char* password = "12345678";

#ifndef debuggingMode
WebSocketsClient webSocket;
#endif

String inboundMessage = "";
String receivedMessage = "";
String message = "";

#ifndef debuggingMode
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket Connected!");
      break;
    case WStype_TEXT:
      {
        Serial.printf("Text message received: %s\n", payload);
        // Parse the JSON message
        StaticJsonDocument<200> jsonDoc;
        DeserializationError error = deserializeJson(jsonDoc, payload);
        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          return;
        }

        // Handle the received command
        if (jsonDoc.containsKey("command")) {
          const char* command = jsonDoc["command"];
          Serial.printf("Command received: %s\n", command);
          Serial2.println(command);
          Serial.printf("Sent to Serial2: %s\n", command);
        }
        break;
      }
    case WStype_ERROR:
      Serial.println("WebSocket Error!");
      break;
    case WStype_BIN:
      Serial.println("Binary message received");
      break;
    case WStype_PING:
      Serial.println("Ping received");
      break;
    case WStype_PONG:
      Serial.println("Pong received");
      break;
  }
}
#endif

void setup() {
  // Debug console
  Serial.begin(115200);                           // For Debugging purpose
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);  // For sending and receiving data to another ESP32
#ifndef debuggingMode
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  const char* serverAddress = "172.20.10.2";
  uint16_t serverPort = 8080;

  Serial.printf("Connecting to WebSocket server at ws://%s:%d\n", serverAddress, serverPort);
  webSocket.begin(serverAddress, serverPort, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
#endif
  Serial.println("Mesh Gateway Ready!");
}

void loop() {
#ifndef debuggingMode
  webSocket.loop();
#endif

#ifdef debuggingMode
  while (Serial.available()) {
    message = Serial.readStringUntil('\n');         // Read the input from the Serial monitor
    Serial2.println(message);                       // Send the message to the other ESP32 via Serial2
    Serial.println("Sent to Serial2: " + message);  // Print the sent message for debugging
  }
#endif
  //this is for reading from the sertial monitor

  while (Serial2.available()) {
    receivedMessage = Serial2.readString();
    // Send JSON string to the WebSocket server when message recieved
#ifndef debuggingMode
    webSocket.sendTXT(receivedMessage);
#endif
    Serial.println(receivedMessage);
  }
}
