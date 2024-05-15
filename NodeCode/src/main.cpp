#include <FastLED.h>
#include <vector>
#include <lodepng.h>
#include "FS.h"
#include "SPIFFS.h"

#define PIN 16
#define NUMPIXELS 56
#define DIMMING_FACTOR 16

#define GRID_MODE 1
#ifdef GRID_MODE
#define GRID_HEIGHT 10
#define GRID_WIDTH 10
#endif

#define COLOUR_RED CRGB::Red
#define COLOUR_GREEN CRGB::Green
#define COLOUR_BLUE CRGB::Blue
#define COLOUR_WHITE CRGB::White

CRGB leds[NUMPIXELS];
#define DELAYVAL 500

void setPixel(int pixel, CRGB colour)
// Sets a pixel to a given colour.
{
  leds[pixel] = colour;
}

void updatePixels()
// Updates pixels. Use this to show the next 'frame' in an animation.
{
  FastLED.show();
}

int gridToSerpentine(int x, int y)
{
  int serpentineAddress;
  if (y % 2 == 0)
  {
    serpentineAddress = x + y * GRID_WIDTH;
  }
  else
  {
    serpentineAddress = (y + 1) * GRID_WIDTH - (x + 1);
  }
  return serpentineAddress;
}

void drawLineStraight(int xy, bool vertical = true, CRGB colour = CRGB::White)
{
  if (vertical)
  {
    for (int i = 0; i < GRID_HEIGHT; i++)
    {
      setPixel(gridToSerpentine(xy, i), colour);
    }
  }
  else
  {
    for (int i = 0; i < GRID_WIDTH; i++)
    {
      setPixel(gridToSerpentine(i, xy), colour);
    }
  }
}

void drawImage(const char *filepath)
// Displays an image on the grid.
{
  std::vector<unsigned char> image; // Raw pixel data
  unsigned width, height;

  // Open file from SPIFFS
  File file = SPIFFS.open(filepath, "r");
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Read file contents into buffer
  std::vector<unsigned char> png_data;
  while (file.available())
  {
    png_data.push_back(file.read());
  }
  file.close();

  // Decode PNG
  unsigned error = lodepng::decode(image, width, height, png_data);

  if (error)
  {
    Serial.print("Decoder error: ");
    Serial.println(error);
    return;
  }

  // Ensuring the image dimensions match the LED grid
  if (width != GRID_WIDTH || height != GRID_HEIGHT)
  {
    Serial.println("Image dimensions do not match the LED grid size.");
    return;
  }

  // Handling different color types
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      int ledIndex = gridToSerpentine(x, y); // Map 2D (x, y) to 1D index
      int pixelIndex = 4 * (y * width + x);  // Each pixel has 4 components (RGBA)
      uint8_t r = image[pixelIndex];
      uint8_t g = image[pixelIndex + 1];
      uint8_t b = image[pixelIndex + 2];
      uint8_t a = image[pixelIndex + 3];

      // Apply alpha transparency
      r = (r * a) / 255;
      g = (g * a) / 255;
      b = (b * a) / 255;

      setPixel(ledIndex, CRGB(r, g, b));
    }
  }
  updatePixels();
}

void setup()
{
  Serial.begin(115200);
  FastLED.addLeds<NEOPIXEL, PIN>(leds, NUMPIXELS);
  FastLED.setBrightness(255 / DIMMING_FACTOR);

  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }
}

void loop()
{
  // drawImage("/colourBars.png");
  // updatePixels();
  // delay(500);
  // drawImage("/kirby.png");
  // updatePixels();
  // delay(2500);
  // drawImage("/colourBars.png");
  // updatePixels();
  // delay(500);
  // drawImage("/test2.png");
  // updatePixels();
  // delay(2500);
  drawImage("/colourBars10x10.png");
  updatePixels();
  delay(500);
  drawImage("/creeper10x10.png");
  updatePixels();
  delay(2500);
}
