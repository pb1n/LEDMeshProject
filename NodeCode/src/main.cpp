#include <Adafruit_NeoPixel.h>
#include <random>
#include <lodepng.h>
#include "FS.h"
#include "SPIFFS.h"

#define PIN 16
#define NUMPIXELS 56
#define DIMMING_FACTOR 16

#define GRID_MODE 1
#ifdef GRID_MODE
#define GRID_HEIGHT 7
#define GRID_WIDTH 8
#endif

#define COLOUR_RED 0xFF0000
#define COLOUR_GREEN 0x00FF00
#define COLOUR_BLUE 0x0000FF
#define COLOUR_WHITE 0xFFFFFF

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500

void setPixel(int pixel, int colour)
// Sets a pixel to a given colour.
{
  pixels.setPixelColor(pixel, colour);
}

void updatePixels()
// Updates pixels. Use this to show the next 'frame' in an animation.
{
  pixels.show();
}

int gridToSeries(int x, int y)
{
  int seriesAddress;
  if (y % 2 == 0)
  {
    seriesAddress = x + y * GRID_WIDTH;
  }
  else
  {
    seriesAddress = (y + 1) * GRID_WIDTH - (x + 1);
  }
  return seriesAddress;
}

void drawLineStraight(int xy, bool vertical = true, int colour = 0xFFFFFF)
{
  if (vertical)
  {
    for (int i = 0; i < GRID_HEIGHT; i++)
    {
      setPixel(gridToSeries(xy, i), colour);
    }
  }
  else
  {
    for (int i = 0; i < GRID_WIDTH; i++)
    {
      setPixel(gridToSeries(i, xy), colour);
    }
  }
}

void drawImage(char *filepath)
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
      int ledIndex = gridToSeries(x, y);    // Map 2D (x, y) to 1D index
      int pixelIndex = 4 * (y * width + x); // Each pixel has 4 components (RGBA)
      uint8_t r = image[pixelIndex];
      uint8_t g = image[pixelIndex + 1];
      uint8_t b = image[pixelIndex + 2];
      uint8_t a = image[pixelIndex + 3];

      // Apply alpha transparency
      r = (r * a) / 255;
      g = (g * a) / 255;
      b = (b * a) / 255;

      setPixel(ledIndex, pixels.Color(r, g, b));
    }
  }
  updatePixels();
}

// void drawLine(int x1, int y1, int x2, int y2){

// }

// void drawCircle(int height, int width, int offset = 0, int colour = 0xFFFFFF)
// {
//   for (int i = 0; i < width; i++)
//   {
//     Serial.println(i);
//     setPixel(i, colour);
//     delay(DELAYVAL);
//   }
//   for (int j = 0; j < height; j++)
//   {
//     Serial.println(j);
//     setPixel(width * j, colour);
//     delay(DELAYVAL);
//   }
// }

void setup()
{
  Serial.begin(115200);
  pixels.setBrightness(4);
  pixels.begin();

  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }
}

void loop()
{
  // bool redFirst;
  // for (int i = 0; i < 4; i++)
  // {
  //   drawLineStraight(2 * i, true, COLOUR_WHITE);
  //   drawLineStraight(2 * i + 1, true, COLOUR_BLUE);
  // }
  // updatePixels();
  // delay(DELAYVAL);

  // for (int i = 0; i < 4; i++)
  // {
  //   drawLineStraight(2 * i, true, COLOUR_BLUE);
  //   drawLineStraight(2 * i + 1, true, COLOUR_WHITE);
  // }
  // updatePixels();
  // delay(DELAYVAL);
  // drawImage("images/test2.png");
  drawImage("/colourBars.png");
  updatePixels();
  delay(500);
  drawImage("/kirby.png");
  updatePixels();
  delay(25000);
  drawImage("/colourBars.png");
  updatePixels();
  delay(500);
  drawImage("/test2.png");
  updatePixels();
  delay(2500);
}
