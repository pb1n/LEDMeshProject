#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// Serial2 pins of ESP32
#define RXD2 18
#define TXD2 19

// LED pins
#define LED_RECEIVE 4    // External LED for receiving messages
#define LED_SEND 5       // External LED for sending messages
#define LED_CONNECTED 2  // External LED for WebSocket connection and debugging mode
#define LED_WIFI 3       // External LED for Wifi connection

#define debuggingMode //directive used for debugging purposes

// Replace with your network credentials
const char* ssid = "mesh123";
const char* password = "12345678";

#ifndef debuggingMode
WebSocketsClient webSocket;
#endif

String inboundMessage = "";
String receivedMessage = "";
String message = "";

// Variables for non-blocking delays
unsigned long previousMillisReceive = 0;
unsigned long previousMillisSend = 0;
unsigned long previousMillisConnected = 0;
const long interval = 100;  // Duration to flash the LED
const long intervalConnected = 500;  // Duration to flash the connected LED

bool receiveLedState = LOW;
bool sendLedState = LOW;
bool connectedLedState = LOW;

bool previousWiFiStatus = false;

#ifndef debuggingMode
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket Disconnected!");
      digitalWrite(LED_CONNECTED, LOW);  // Turn off the connected LED
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket Connected!");
      digitalWrite(LED_CONNECTED, HIGH);  // Turn on the connected LED
      digitalWrite(LED_WIFI, HIGH);
      break;
    case WStype_TEXT: {
      Serial.printf("Text message received: %s\n", payload);
      receiveLedState = HIGH;
      previousMillisReceive = millis();  // Record the time the message was received

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
        Serial1.println(command);
        Serial.printf("Sent to Serial2: %s\n", command);
        sendLedState = HIGH;
        previousMillisSend = millis();  // Record the time the message was sent
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
  Serial1.begin(115200, SERIAL_8N1, RXD2, TXD2);  // For sending and receiving data to another ESP32

  // Initialize LEDs
  pinMode(LED_RECEIVE, OUTPUT);
  pinMode(LED_SEND, OUTPUT);
  pinMode(LED_CONNECTED, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);

  digitalWrite(LED_RECEIVE, LOW);
  digitalWrite(LED_SEND, LOW);
  digitalWrite(LED_CONNECTED, LOW);
  digitalWrite(LED_WIFI, LOW);

#ifndef debuggingMode
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_WIFI, HIGH);
    delay(500);
    digitalWrite(LED_WIFI, LOW);
    delay(500);
    Serial.print(".");
  }
  digitalWrite(LED_WIFI, HIGH);
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
  // Handle the connected LED flashing every 500ms
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisConnected >= intervalConnected) {
    previousMillisConnected = currentMillis;
    connectedLedState = !connectedLedState;  // Toggle the state of the connected LED
    digitalWrite(LED_CONNECTED, connectedLedState);
    digitalWrite(LED_WIFI, connectedLedState);
  }

  while (Serial.available()) {
    message = Serial.readStringUntil('\n');         // Read the input from the Serial monitor
    Serial1.println(message);                       // Send the message to the other ESP32 via Serial2
    Serial.println("Sent to Serial2: " + message);  // Print the sent message for debugging
    sendLedState = HIGH;
    previousMillisSend = millis();  // Record the time the message was sent
  }
#endif

  // Check WiFi connection status and update LED
  bool currentWiFiStatus = (WiFi.status() == WL_CONNECTED);
  if (currentWiFiStatus != previousWiFiStatus) {
    digitalWrite(LED_WIFI, currentWiFiStatus ? HIGH : LOW);
    previousWiFiStatus = currentWiFiStatus;
  }

  // Check if it's time to turn off the receive LED
  if (receiveLedState == HIGH && millis() - previousMillisReceive >= interval) {
    receiveLedState = LOW;
    digitalWrite(LED_RECEIVE, LOW);  // Turn off the receive LED
  } else if (receiveLedState == HIGH) {
    digitalWrite(LED_RECEIVE, HIGH);  // Keep the receive LED on
  }

  // Check if it's time to turn off the send LED
  if (sendLedState == HIGH && millis() - previousMillisSend >= interval) {
    sendLedState = LOW;
    digitalWrite(LED_SEND, LOW);  // Turn off the send LED
  } else if (sendLedState == HIGH) {
    digitalWrite(LED_SEND, HIGH);  // Keep the send LED on
  }

  // This is for reading from the serial monitor
  while (Serial1.available()) {
    receivedMessage = Serial1.readString();
    // Send JSON string to the WebSocket server when message received
#ifndef debuggingMode
    webSocket.sendTXT(receivedMessage);
#endif
    Serial.println(receivedMessage);
    receiveLedState = HIGH;
    previousMillisReceive = millis();  // Record the time the message was received
  }
}
