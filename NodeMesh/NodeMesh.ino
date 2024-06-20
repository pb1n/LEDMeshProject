#include <Arduino.h>
#include <LittleFS.h>
#include <NeoPixelBus.h>
#include <vector>
#include <lodepng.h>
#include "Adafruit_SHT4x.h"
#include <Wire.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>

// NeoPixel setup
#define PIN 2
#define NUMPIXELS 256
#define DIMMING_FACTOR 16
#define GRID_HEIGHT 16
#define GRID_WIDTH 16
#define BOOT_BUTTON_PIN 9

// Define I2C pins for the SHT4x sensor
#define SDA_PIN 21
#define SCL_PIN 22
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> pixels(NUMPIXELS, PIN);

// Create a TwoWire instance for I2C with specific pins
TwoWire myWire = TwoWire(0);  // Create a TwoWire instance, use 0 for ESP32 or other similar hardware

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

bool lastButtonState = HIGH;  // Assume the button is not pressed initially
bool sensorMode = true;  // Global variable for sensor mode
bool temperatureMode = true;  // Global variable for temperature mode

// Temperature and humidity thresholds
float coldThreshold = 25.0;
float hotThreshold = 30.0;
float dryThreshold = 60.0;
float wetThreshold = 80.0;

// Image handling structures
struct imageFrame {
  std::vector<RgbColor> pixels;  // To store pixel colors directly
  unsigned width, height;
};

struct imageSequence {
  std::vector<imageFrame> frames;
  int fps;  // Frames per second for this sequence
};

std::vector<imageSequence> sequences;  // Global variable to store sequences
std::vector<imageSequence> tempSequences;  // Sequences for temperature mode
std::vector<imageSequence> humiditySequences;  // Sequences for humidity mode
int sequenceIndex = 0;                 // Global sequence index
int tempSequenceIndex = 0;           // Temperature mode sequence index
int humiditySequenceIndex = 0;       // Humidity mode sequence index
int frameIndex = 0;                    // Frame index for the current sequence
bool isPaused = false;                 // State variable for pause/resume

// Mesh network setup
#define MESH_PREFIX "RNTMESH"
#define MESH_PASSWORD "MESHpassword"
#define MESH_PORT 5555

Scheduler userScheduler;  // to control your personal task
painlessMesh mesh;

// Function declarations
void displayTask();
String getReadings();
void switchToNextImageSequence();
void switchToSpecificImageSequence(int index);
void buttonTaskCallback();
void updateSensorImage();
void updateTempImage();
void updateHumidityImage();

Task taskSendMessage(TASK_SECOND * 2, TASK_FOREVER, []() {
  String msg = getReadings();
  mesh.sendBroadcast(msg);  // broadcasts to all nodes
});

Task buttonTask(TASK_MILLISECOND * 50, TASK_FOREVER, &buttonTaskCallback);  // Debounce task

String getReadings() {
  StaticJsonDocument<200> jsonReadings;
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp); // Populate temp and humidity objects with fresh data
  jsonReadings["nodeId"] = String(mesh.getNodeId());
  jsonReadings["temperature"] = temp.temperature;
  jsonReadings["humidity"] = humidity.relative_humidity;
  String result;
  serializeJson(jsonReadings, result);
  return result;
}

void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  if (msg == "t") {
    switchToNextImageSequence();
  } else if (msg == "p") {
    isPaused = true;
    Serial.println("Animation paused");
  } else if (msg == "r") {
    isPaused = false;
    Serial.println("Animation resumed");
  } else if (msg == "a") {
    sensorMode = false;
    Serial.println("Switched to animation mode");
    frameIndex = 0;  // Reset frame index
  } else if (msg == "s") {
    sensorMode = true;
    Serial.println("Switched to sensor mode");
    frameIndex = 0;  // Reset frame index
  } else if (msg == "st") {
    temperatureMode = true;
    Serial.println("Switched to temperature mode");
    frameIndex = 0;  // Reset frame index
  } else if (msg == "sh") {
    temperatureMode = false;
    Serial.println("Switched to humidity mode");
    frameIndex = 0;  // Reset frame index
  } else if (msg.startsWith("a")) {
    int index = msg.substring(1).toInt();
    switchToSpecificImageSequence(index - 1);  // Convert 1-based index to 0-based
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // Initialize I2C communication with custom pins
  myWire.begin(SDA_PIN, SCL_PIN);  // SDA, SCL
  if (!sht4.begin(&myWire)) {  // Pass the custom Wire instance
    Serial.println("Couldn't find SHT4x");
    while (1) delay(1);
  }
  Serial.println("Found SHT4x sensor");
  Serial.print("Serial number 0x");
  Serial.println(sht4.readSerial(), HEX);

  // You can have 3 different precisions, higher precision takes longer
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  switch (sht4.getPrecision()) {
    case SHT4X_HIGH_PRECISION: 
      Serial.println("High precision");
      break;
    case SHT4X_MED_PRECISION: 
      Serial.println("Med precision");
      break;
    case SHT4X_LOW_PRECISION: 
      Serial.println("Low precision");
      break;
  }

  pixels.Begin();
  pixels.Show();  // Initialize all pixels to 'off'

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.setContainsRoot(true);
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
  userScheduler.addTask(buttonTask);
  buttonTask.enable();

  // Load all image sequences
  const char* imageDirectories[] = {
    "/images/bird",
    "/images/coin",
    "/images/sword",
    "/images/torch",
    "/images/tank",
    "/images/zombies",
    "/images/armLogo",
    "/images/dry",
    "/images/moderateHum",
    "/images/wet",
  };
  const int fpsValues[] = { 30, 8, 20, 10, 8, 8, 1, 8, 8, 8 };

  for (int i = 0; i < sizeof(imageDirectories) / sizeof(imageDirectories[0]); i++) {
    imageSequence sequence;
    sequence.fps = fpsValues[i];  // Assign FPS value from predefined array
    loadImageFrames(imageDirectories[i], sequence);
    sequences.push_back(sequence);
  }

  // Load temperature image sequences
  const char* tempImageDirectories[] = {
    "/images/cold",
    "/images/moderate",
    "/images/hot"
  };
  const int tempFpsValues[] = { 8, 8, 8 };

  for (int i = 0; i < sizeof(tempImageDirectories) / sizeof(tempImageDirectories[0]); i++) {
    imageSequence sequence;
    sequence.fps = tempFpsValues[i];  // Assign FPS value from predefined array
    loadImageFrames(tempImageDirectories[i], sequence);
    tempSequences.push_back(sequence);
  }

  // Load humidity image sequences
  const char* humidityImageDirectories[] = {
    "/images/dry",
    "/images/moderateHum",
    "/images/wet"
  };
  const int humidityFpsValues[] = { 8, 8, 8 };

  for (int i = 0; i < sizeof(humidityImageDirectories) / sizeof(humidityImageDirectories[0]); i++) {
    imageSequence sequence;
    sequence.fps = humidityFpsValues[i];  // Assign FPS value from predefined array
    loadImageFrames(humidityImageDirectories[i], sequence);
    humiditySequences.push_back(sequence);
  }
}

void loop() {
  mesh.update();
  userScheduler.execute();
  if (!isPaused) {
    displayTask();
  }
}

void buttonTaskCallback() {
  int buttonState = digitalRead(BOOT_BUTTON_PIN);

  // Check if the button is pressed and the state has changed
  if (buttonState == LOW && lastButtonState == HIGH) {
    switchToNextImageSequence();
    Serial.println("Boot button pressed! Switched to next animation.");
  }

  lastButtonState = buttonState;  // Update the last button state
}

void displayTask() {
  static unsigned long lastUpdateTime = 0;

  if (sensorMode) {
    updateSensorImage();
  } else {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime > (1000 / sequences[sequenceIndex].fps)) {
      lastUpdateTime = currentTime;
      if (frameIndex < sequences[sequenceIndex].frames.size()) {
        drawImageFromMemory(sequences[sequenceIndex].frames[frameIndex]);

        frameIndex++;
        if (frameIndex >= sequences[sequenceIndex].frames.size()) {
          frameIndex = 0;
        }
      } else {
        Serial.println("Error: frameIndex out of bounds");
      }
    }
  }
}

void updateSensorImage() {
  if (temperatureMode) {
    updateTempImage();
  } else {
    updateHumidityImage();
  }
}

void updateTempImage() {
  static unsigned long lastSensorUpdateTime = 0;
  static unsigned long lastFrameUpdateTime = 0;
  unsigned long currentTime = millis();

  // Check if it's time to read sensor data
  if (currentTime - lastSensorUpdateTime >= 2000) { // 3000 ms = 3 seconds
    lastSensorUpdateTime = currentTime;
    sensors_event_t humidity, temp;
    sht4.getEvent(&humidity, &temp);

    if (temp.temperature < coldThreshold) {
      if (tempSequenceIndex != 0) {
        tempSequenceIndex = 0;  // Cold image sequence index
        frameIndex = 0;
        clearPixels();
        Serial.println("Switched to cold image sequence.");
      }
    } else if (temp.temperature > hotThreshold) {
      if (tempSequenceIndex != 2) {
        tempSequenceIndex = 2;  // Hot image sequence index
        frameIndex = 0;
        clearPixels();
        Serial.println("Switched to hot image sequence.");
      }
    } else {
      if (tempSequenceIndex != 1) {
        tempSequenceIndex = 1;  // Moderate image sequence index
        frameIndex = 0;
        clearPixels();
        Serial.println("Switched to moderate image sequence.");
      }
    }
  }

  // Update frames based on fps of the current sequence
  if (currentTime - lastFrameUpdateTime > (1000 / tempSequences[tempSequenceIndex].fps)) {
    lastFrameUpdateTime = currentTime;
    if (frameIndex < tempSequences[tempSequenceIndex].frames.size()) {
      drawImageFromMemory(tempSequences[tempSequenceIndex].frames[frameIndex]);
      frameIndex++;
      if (frameIndex >= tempSequences[tempSequenceIndex].frames.size()) {
        frameIndex = 0; // Reset frame index to loop the animation
      }
    } else {
      Serial.println("Error: frameIndex out of bounds in temperature mode");
    }
  }
}

void updateHumidityImage() {
  static unsigned long lastSensorUpdateTime = 0;
  static unsigned long lastFrameUpdateTime = 0;
  unsigned long currentTime = millis();

  // Check if it's time to read sensor data
  if (currentTime - lastSensorUpdateTime >= 2000) { // 3000 ms = 3 seconds
    lastSensorUpdateTime = currentTime;
    sensors_event_t humidity, temp;
    sht4.getEvent(&humidity, &temp);

    if (humidity.relative_humidity < dryThreshold) {
      if (humiditySequenceIndex != 0) {
        humiditySequenceIndex = 0;  // Dry image sequence index
        frameIndex = 0;
        clearPixels();
        Serial.println("Switched to dry image sequence.");
      }
    } else if (humidity.relative_humidity > wetThreshold) {
      if (humiditySequenceIndex != 2) {
        humiditySequenceIndex = 2;  // Wet image sequence index
        frameIndex = 0;
        clearPixels();
        Serial.println("Switched to wet image sequence.");
      }
    } else {
      if (humiditySequenceIndex != 1) {
        humiditySequenceIndex = 1;  // Moderate image sequence index
        frameIndex = 0;
        clearPixels();
        Serial.println("Switched to moderate humidity image sequence.");
      }
    }
  }

  // Update frames based on fps of the current sequence
  if (currentTime - lastFrameUpdateTime > (1000 / humiditySequences[humiditySequenceIndex].fps)) {
    lastFrameUpdateTime = currentTime;
    if (frameIndex < humiditySequences[humiditySequenceIndex].frames.size()) {
      drawImageFromMemory(humiditySequences[humiditySequenceIndex].frames[frameIndex]);
      frameIndex++;
      if (frameIndex >= humiditySequences[humiditySequenceIndex].frames.size()) {
        frameIndex = 0; // Reset frame index to loop the animation
      }
    } else {
      Serial.println("Error: frameIndex out of bounds in humidity mode");
    }
  }
}

void drawImageFromMemory(const imageFrame& frame) {
  for (unsigned y = 0; y < frame.height; y++) {
    for (unsigned x = 0; x < frame.width; x++) {
      int ledIndex = gridToSerpentine(x, y);
      if (ledIndex < NUMPIXELS) {
        pixels.SetPixelColor(ledIndex, frame.pixels[y * frame.width + x]);
      } else {
        Serial.println("Error: ledIndex out of bounds");
      }
    }
  }
  pixels.Show();
}

void clearPixels() {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.SetPixelColor(i, RgbColor(0, 0, 0));
  }
  pixels.Show();
}

int gridToSerpentine(int x, int y) {
  return (y % 2 == 0) ? ((y + 1) * GRID_WIDTH - (x + 1)) : (x + y * GRID_WIDTH);
}

void loadImageFrames(const char* directory, imageSequence& sequence) {
  File root = LittleFS.open(directory);
  if (!root) {
    Serial.print("Failed to open directory: ");
    Serial.println(directory);
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();  // Convert the C-string to a String object
    if (fileName.endsWith(".png")) {
      std::vector<uint8_t> png_data;
      while (file.available()) {
        png_data.push_back(file.read());
      }
      processImageData(png_data.data(), png_data.size(), sequence);
    }
    file = root.openNextFile();
  }
}

void processImageData(const uint8_t* imageData, size_t length, imageSequence& sequence) {
  imageFrame frame;
  std::vector<uint8_t> pixelData;
  unsigned error = lodepng::decode(pixelData, frame.width, frame.height, imageData, length);
  if (error) {
    Serial.print("Decoder error ");
    Serial.println(error);
    return;
  }
  frame.pixels.reserve(frame.width * frame.height);
  for (size_t i = 0; i < pixelData.size(); i += 4) {
    uint8_t r = pixelData[i];
    uint8_t g = pixelData[i + 1];
    uint8_t b = pixelData[i + 2];
    frame.pixels.push_back(RgbColor(r / DIMMING_FACTOR, g / DIMMING_FACTOR, b / DIMMING_FACTOR));
  }
  sequence.frames.push_back(frame);
}

void switchToNextImageSequence() {
  if (!sequences.empty()) {
    sequenceIndex = (sequenceIndex + 1) % sequences.size();
    frameIndex = 0;
    Serial.println("Switched to next image sequence: " + String(sequenceIndex));
    clearPixels();  // Clear pixels when switching sequences
  } else {
    Serial.println("Error: No sequences available");
  }
}

void switchToSpecificImageSequence(int index) {
  if (index >= 0 && index < sequences.size()) {
    sequenceIndex = index;
    frameIndex = 0;
    Serial.println("Switched to image sequence: " + String(sequenceIndex));
    clearPixels();  // Clear pixels when switching sequences
  } else {
    Serial.println("Invalid sequence index: " + String(index));
  }
}
