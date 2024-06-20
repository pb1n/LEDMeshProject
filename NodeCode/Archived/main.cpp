#include <esp_log.h>
#include <rmt_led_strip.hpp>
#include <vector>
#include <lodepng.h>
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include <esp_partition.h>

#define PIN 2
#define NUMPIXELS 256
#define DIMMING_FACTOR 16
#define GIF_FPS 20

#define GRID_MODE
#ifdef GRID_MODE
#define GRID_HEIGHT 16
#define GRID_WIDTH 16
#endif

using namespace esp_idf;

ws2812 pixels(PIN, NUMPIXELS);

int correctedColour(int colour)
// Dims a Hex colour code by a given factor
{
    return ((colour >> 16) / DIMMING_FACTOR << 16) |
           ((colour >> 8 & 0xFF) / DIMMING_FACTOR << 8) |
           ((colour & 0xFF) / DIMMING_FACTOR);
}

void setPixel(int pixel, uint32_t colour)
// Sets a pixel to a given colour.
{
    pixels.color(pixel, correctedColour(colour));
}

void updatePixels()
// Updates pixels. Use this to show the next 'frame' in an animation.
{
    pixels.update();
}

#ifdef GRID_MODE
int gridToSerpentine(int x, int y)
{
    int serpentineAddress;
    if (y % 2 == 0)
    {
        serpentineAddress = (y + 1) * GRID_WIDTH - (x + 1);
    }
    else
    {
        serpentineAddress = x + y * GRID_WIDTH;
    }
    return serpentineAddress;
}

void drawLineStraight(int xy, bool vertical = true, uint32_t colour = 0xFFFFFF)
// Basic function to draw a straight line across the matrix of LEDs
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
    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        ESP_LOGE("drawImage", "Failed to open file for reading");
        return;
    }

    // Read file contents into buffer
    std::vector<unsigned char> png_data;
    while (true)
    {
        int c = fgetc(file);
        if (c == EOF)
            break;
        png_data.push_back(c);
    }
    fclose(file);

    // Decode PNG
    unsigned error = lodepng::decode(image, width, height, png_data);

    if (error)
    {
        ESP_LOGE("drawImage", "Decoder error: %d", error);
        return;
    }

    // Ensuring the image dimensions match the LED grid
    if (width != GRID_WIDTH || height != GRID_HEIGHT)
    {
        ESP_LOGE("drawImage", "Image dimensions do not match the LED grid size.");
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

            setPixel(ledIndex, (r << 16) | (g << 8) | b);
        }
    }
    updatePixels();
}
#endif

extern "C"
{
    void app_main(void);
}

void setup()
{
    ESP_LOGI("setup", "Starting setup...");
    if (!pixels.initialize())
    {
        ESP_LOGE("setup", "Failed to initialize LED pixels");
        return;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    // Initialize SPIFFS
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE("SPIFFS", "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE("SPIFFS", "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE("SPIFFS", "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    // Check SPIFFS
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE("SPIFFS", "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI("SPIFFS", "Partition size: total: %d, used: %d", total, used);
    }
}

void loop()
{
    for (int i = 0; i < 15; i++)
    {
        std::string filepath = "/spiffs/images/freddy/freddy-" + std::to_string(i) + ".png";
        ESP_LOGI("loop", "Attempting to open: %s", filepath.c_str());
        drawImage(filepath.c_str()); // Convert std::string to const char*
        updatePixels();
        vTaskDelay(1000 / GIF_FPS / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    setup();
    // drawImage("/spiffs/kirby16x16.png");
    for (;;)
    {
        loop();
    }
}
