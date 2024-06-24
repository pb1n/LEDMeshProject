#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>  // Include the HTTPClient library

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

#define DEBUGGING_MODE// directive used for debugging purposes
#define TEST_IMAGE_SEQUENCE // directive used for testing image sequence functionality

// Replace with your network credentials
const char* ssid = "mesh123";
const char* password = "12345678";

#ifndef DEBUGGING_MODE
WebSocketsClient webSocket;
#endif

String inboundMessage = "";
String receivedMessage = "";
String message = "";
String jsonUrl = ""; // Global variable for the URL

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

#ifndef DEBUGGING_MODE
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
          if (String(command) == "c") {
            Serial1.println("CUSTOM ANIMATION FLAG RECEIVED");
            // Initiate HTTP GET request
            if (!jsonUrl.isEmpty()) {
              HTTPClient http;
              http.begin(jsonUrl);  // Specify the URL
              int httpCode = http.GET();  // Make the request

              // Check for the returning code
              if (httpCode > 0) {
                // HTTP header has been sent and server response header has been handled
                Serial.printf("[HTTP] GET... code: %d\n", httpCode);

                // File found at server
                if (httpCode == HTTP_CODE_OK) {
                  String payload = http.getString();
                  Serial.println("Received payload: " + payload);
                  segmentAndSend(payload);
                }
              } else {
                Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
              }

              http.end();  // End the connection
            } else {
              Serial.println("No URL specified for HTTP GET request");
            }
          } else {
            Serial1.println(command);
            Serial.printf("Sent to Serial2: %s\n", command);
          }
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

#ifndef DEBUGGING_MODE
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

  const char* serverAddress = "192.168.30.88";
  uint16_t serverPort = 8081;

  Serial.printf("Connecting to WebSocket server at ws://%s:%d\n", serverAddress, serverPort);
  webSocket.begin(serverAddress, serverPort, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
#endif
  Serial.println("Mesh Gateway Ready!");

  // Set the URL for the HTTP GET request
  jsonUrl = "http://your-json-url.com/path/to/your/json";  // Replace with your actual URL

#ifdef TEST_IMAGE_SEQUENCE
  // Send mock JSON file on startup
  sendMockJsonFile();
#endif
}

void loop() {
#ifndef DEBUGGING_MODE
  webSocket.loop();
#endif

#ifdef DEBUGGING_MODE
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
#ifndef DEBUGGING_MODE
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
  "name": "Sunset",
  "fps": 10,
  "images": [
    "iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAAXNSR0IArs4c6QAAAERlWElmTU0AKgAAAAgAAYdpAAQAAAABAAAAGgAAAAAAA6ABAAMAAAABAAEAAKACAAQAAAABAAAAEKADAAQAAAABAAAAEAAAAAA0VXHyAAAA/ElEQVQ4EWM0l5ryn4ECwCLGIUeBdgYGFkE+S6IN0JAXhqu98fAtmM2CLAiXBTIqhYOQuWB2+9t1cDGYPsaNPf9QwsDvGqZGuC4oY5MWwiDGmyf//d8l9Z8hpz4YXR1e/q10iCGMjHn9YBf8+3IQrwZ0SSYee7AQy39jT4jcQdIMgOljurxNjQGESQWTnVTBWhgvhyMCUYebcACCdF35ighEJmSbkSWQxZHZ6GoYJz/+ixKNMMXosTKlcS1YKkckj2HKm0kMMBpsAIgDAiAJYgCyesb/37P/Y9MIswFkILIGdAuYYM5BloBpRtaIrA4mDtID9wI2VyAbiosNAArnZb0sbI+yAAAAAElFTkSuQmCC"
  ]
})";

  // Segment and send the JSON content
  segmentAndSend(jsonContent);
}
