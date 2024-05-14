#include <Adafruit_NeoPixel.h>
#include <random>
#define PIN 16
#define NUMPIXELS 56
#define DIMMING_FACTOR 16

#define GRID_MODE 1
#ifdef GRID_MODE
#define GRID_HEIGHT 7
#define GRID_WIDTH 8
#endif

#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define WHITE 0xFFFFFF

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 40

int dimmedColour(int colour)
// Dims a Hex colour code by a given factor
{
  return ((colour >> 16) / DIMMING_FACTOR << 16) |
         ((colour >> 8 & 0xFF) / DIMMING_FACTOR << 8) |
         ((colour & 0xFF) / DIMMING_FACTOR);
}

void setPixel(int pixel, int colour)
// Sets a pixel to a given colour
{
  pixels.setPixelColor(pixel, dimmedColour(colour));
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

// void drawLine(int x1, int y1, int x2, int y2){

// }

// void loadBMP(){

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
  pixels.begin();
}

void loop()
{
  pixels.clear();
  drawLineStraight(2, true, BLUE);
  drawLineStraight(5, true, BLUE);
  delay(2000);
  pixels.clear();
  drawLineStraight(1, false, RED);
  drawLineStraight(5, false, RED);
  delay(2000);

  // drawCircle(7, 8);
  //  for (int i = 0; i < NUMPIXELS; i++)
  //  {
  //    setPixel(i, 0xFFFFFF);
  //    delay(DELAYVAL);
  //  }
  // setPixel(gridToSeries(4, 3), 0xFFFFFF);
  // delay(DELAYVAL);
}
