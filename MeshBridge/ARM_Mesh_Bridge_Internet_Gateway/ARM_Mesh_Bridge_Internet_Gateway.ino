//************************************************************
// This code is used in conjunction with the mesh AP to form a mesh-internet bridge:
//1) Receive sensor readings and statuses of all nodes in the network from the mesh AP via UART and 
//send to webpage and database
//2) Forward requests via UART to mesh AP to communicate to mesh nodes
//************************************************************

#include <ArduinoJson.h>

// Serial2 pins of ESP32
#define RXD2 16
#define TXD2 17

String message = "";
String receivedMessage = "";
bool messageReady = false;
bool TXmessageReady = false;
bool RXmessageReady = false;

void setup()
{
  // Debug console
  Serial.begin(115200); // For Debugging purpose
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); // For sending and receiving data to another ESP32
  Serial.println("Mesh Gateway Ready!");
}

void loop()
{

  while (Serial.available()) {
    message = Serial.readStringUntil('\n'); // Read the input from the Serial monitor
    Serial2.println(message); // Send the message to the other ESP32 via Serial2
    Serial.println("Sent to Serial2: " + message); // Print the sent message for debugging
  }

  while (Serial2.available()) 
  {
    receivedMessage = Serial2.readString();
    //RXmessageReady = true;
    Serial.println(receivedMessage);
  }

  // Only process message if there's one
  //if (RXmessageReady)
  //{
    //Serial.println("Received from Serial2: " + message); 
   // Serial.println(message); 
   // RXmessageReady = false;
  //}

}
 