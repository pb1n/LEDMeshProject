#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#define SERIAL_BAUD 115200
#define CHUNK_SIZE 1000

// Serial2 pins of ESP32
#define RXD2 18
#define TXD2 19

// LED pins
#define LED_RECEIVE 4    // External LED for receiving messages
#define LED_SEND 5       // External LED for sending messages
#define LED_CONNECTED 2  // External LED for WebSocket connection and debugging mode
#define LED_WIFI 3       // External LED for Wifi connection

//#define debuggingMode // directive used for debugging purposes
#define TEST_IMAGE_SEQUENCE // directive used for testing image sequence functionality

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
      StaticJsonDocument<2048> jsonDoc;  // Increase the size to handle larger JSON objects
      DeserializationError error = deserializeJson(jsonDoc, payload);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }

      // Check if the JSON contains the fields required for an image sequence
      if (jsonDoc.containsKey("name") && jsonDoc.containsKey("fps") && jsonDoc.containsKey("images")) {
        Serial.println("Image sequence received");
        // Segment and send the JSON content
        String jsonString = String((char*)payload); // Convert payload to String
        segmentAndSend(jsonString);
      } else {
        // Handle regular command
        if (jsonDoc.containsKey("command")) {
          const char* command = jsonDoc["command"];
          Serial.printf("Command received: %s\n", command);
          Serial1.println(command);
          Serial.printf("Sent to Serial2: %s\n", command);
        }
      }
      
      sendLedState = HIGH;
      previousMillisSend = millis();  // Record the time the message was sent
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

#ifdef TEST_IMAGE_SEQUENCE
  // Send mock JSON file on startup
  sendMockJsonFile();
#endif
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

void segmentAndSend(const String &jsonContent) {
  int totalChunks = (jsonContent.length() + CHUNK_SIZE - 1) / CHUNK_SIZE;
  String messageId = String(random(0xffff), HEX);

  for (int i = 0; i < totalChunks; i++) {
    int start = i * CHUNK_SIZE;
    int end = start + CHUNK_SIZE;
    if (end > jsonContent.length()) {
      end = jsonContent.length();
    }
    String chunk = jsonContent.substring(start, end);

    StaticJsonDocument<2048> doc;
    doc["id"] = messageId;
    doc["index"] = i;
    doc["total"] = totalChunks;
    doc["chunk"] = chunk;

    String output;
    serializeJson(doc, output);

    Serial1.println(output); // Send the chunk over Serial1

    // Print the chunk to the serial monitor
    Serial.println("Chunk " + String(i) + "/" + String(totalChunks) + ":");
    Serial.println(output);
    delay(200); // Small delay to ensure serial buffer is not overwhelmed
  }
}

void sendMockJsonFile() {
  // JSON content directly in the code
  const char* jsonContent = R"({
    "name": "Coin",
    "fps": 10,
    "images" : [
      "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"
    ]
  })";

  // Segment and send the JSON content
  segmentAndSend(jsonContent);
}
