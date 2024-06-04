#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// Serial2 pins of ESP32
#define RXD2 16
#define TXD2 17

// Replace with your network credentials
const char* ssid = "iPhone888";
const char* password = "pi12345678";

String message = "";
String receivedMessage = "";
bool messageReady = false;
bool TXmessageReady = false;
bool RXmessageReady = false;

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket Connected!");
      webSocket.sendTXT("Hello from ESP32");
      break;
    case WStype_TEXT:
      Serial.printf("Text message received: %s\n", payload);
      if (payload[0] == 't') {
        Serial.println("Received 't' from server!");
        // Perform any specific action you want on receiving 't'
        Serial2.println("t");                       // Send the message to the other ESP32 via Serial2
        Serial.println("Sent to Serial2: " + message);  // Print the sent message for debugging
      }
      break;
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

void setup() {
  // Debug console
  Serial.begin(115200);                           // For Debugging purpose
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);  // For sending and receiving data to another ESP32
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  const char* serverAddress = "172.20.10.12";
  uint16_t serverPort = 8080;

  Serial.printf("Connecting to WebSocket server at ws://%s:%d\n", serverAddress, serverPort);
  webSocket.begin(serverAddress, serverPort, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  Serial.println("Mesh Gateway Ready!");
}

void loop() {
  while (Serial2.available()) {
    receivedMessage = Serial2.readString();
    Serial.println(receivedMessage);
  }

  webSocket.loop();
}
