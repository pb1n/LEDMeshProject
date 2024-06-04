#include <Arduino.h>
#include <LittleFS.h>
#include <NeoPixelBus.h>
#include <vector>
#include <lodepng.h>
#include "DHT.h"
#include "painlessMesh.h"
#include <ArduinoJson.h>

// NeoPixel setup
#define PIN 2
#define NUMPIXELS 256
#define DIMMING_FACTOR 16
#define GRID_HEIGHT 16
#define GRID_WIDTH 16
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> pixels(NUMPIXELS, PIN);

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

// Mesh network setup
#define MESH_PREFIX "RNTMESH"
#define MESH_PASSWORD "MESHpassword"
#define MESH_PORT 5555
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

Scheduler userScheduler;  // to control your personal task
painlessMesh mesh;

// Functions declarations
void displayTask();
String getReadings();

Task taskSendMessage(TASK_SECOND * 3, TASK_FOREVER, []() {
    String msg = getReadings();
    mesh.sendBroadcast(msg);  // broadcasts to all nodes
});

String getReadings() {
    StaticJsonDocument<200> jsonReadings;
    jsonReadings["nodeId"] = String(mesh.getNodeId());
    jsonReadings["temp"] = dht.readTemperature();
    jsonReadings["hum"] = dht.readHumidity();
    String result;
    serializeJson(jsonReadings, result);
    return result;
}

void receivedCallback(uint32_t from, String &msg) {
    Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
}

void setup() {
    Serial.begin(115200);
    if (!LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    dht.begin();
    pixels.Begin();
    pixels.Show();  // Initialize all pixels to 'off'

    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.setContainsRoot(true);
    userScheduler.addTask(taskSendMessage);
    taskSendMessage.enable();

    // Load all image sequences
    const char* imageDirectories[] = {
        "/images/bird",
        "/images/coin",
        "/images/sword",
        "/images/torch"
    };
    const int fpsValues[] = {30, 8, 20, 10};

    for (int i = 0; i < sizeof(imageDirectories) / sizeof(imageDirectories[0]); i++) {
        imageSequence sequence;
        sequence.fps = fpsValues[i];  // Assign FPS value from predefined array
        loadImageFrames(imageDirectories[i], sequence);
        sequences.push_back(sequence);
    }
}

void loop() {
    mesh.update();
    displayTask();
}

void displayTask() {
    static unsigned long lastUpdateTime = 0;
    static int sequenceIndex = 0;
    static int frameIndex = 0;
    
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime > (1000 / sequences[sequenceIndex].fps)) {
        lastUpdateTime = currentTime;
        drawImageFromMemory(sequences[sequenceIndex].frames[frameIndex]);

        frameIndex++;
        if (frameIndex >= sequences[sequenceIndex].frames.size()) {
            frameIndex = 0;
            sequenceIndex = (sequenceIndex + 1) % sequences.size();
        }
    }
}

void drawImageFromMemory(const imageFrame& frame) {
    for (unsigned y = 0; y < frame.height; y++) {
        for (unsigned x = 0; x < frame.width; x++) {
            int ledIndex = gridToSerpentine(x, y);
            pixels.SetPixelColor(ledIndex, frame.pixels[y * frame.width + x]);
        }
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
