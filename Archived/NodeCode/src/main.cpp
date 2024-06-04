#include <esp_log.h>
#include <rmt_led_strip.hpp>
#include <vector>
#include <lodepng.h>
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include <esp_partition.h>
#include <dirent.h>

#define PIN 2
#define NUMPIXELS 256
#define DIMMING_FACTOR 16

#define GRID_MODE
#ifdef GRID_MODE
#define GRID_HEIGHT 16
#define GRID_WIDTH 16
#endif

using namespace esp_idf;

ws2812 pixels(PIN, NUMPIXELS);

const std::vector<std::string> imageDirectories = {
    // "/spiffs/images/freddy",
    "/spiffs/images/bird",
    "/spiffs/images/coin",
    "/spiffs/images/sword",
    // "/spiffs/images/zombie",
    // "/spiffs/images/tank",
    "/spiffs/images/torch"};

const std::vector<size_t> imageCounts = {/*15,*/ 8, 7, 15, /*45, 18,*/ 5};
const std::vector<int> fpsValues = {/*20,*/ 10, 8, 20, /*21, 20,*/ 10}; // Adjust FPS values as needed

size_t currentDirectoryIndex = 0;
size_t currentImageIndex = 0;
std::vector<std::string> images;
const size_t maxIterations = 10;
size_t iterationCounter = 0;

int correctedColour(int colour)
{
    return ((colour >> 16) / DIMMING_FACTOR << 16) |
           ((colour >> 8 & 0xFF) / DIMMING_FACTOR << 8) |
           ((colour & 0xFF) / DIMMING_FACTOR);
}

void setPixel(int pixel, uint32_t colour)
{
    pixels.color(pixel, correctedColour(colour));
}

void updatePixels()
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

void drawImage(const char *filepath)
{
    std::vector<unsigned char> image; // Raw pixel data
    unsigned width, height;

    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        ESP_LOGE("drawImage", "Failed to open file for reading");
        return;
    }

    std::vector<unsigned char> png_data;
    while (true)
    {
        int c = fgetc(file);
        if (c == EOF)
            break;
        png_data.push_back(c);
    }
    fclose(file);

    unsigned error = lodepng::decode(image, width, height, png_data);

    if (error)
    {
        ESP_LOGE("drawImage", "Decoder error: %d", error);
        return;
    }

    if (width != GRID_WIDTH || height != GRID_HEIGHT)
    {
        ESP_LOGE("drawImage", "Image dimensions do not match the LED grid size.");
        return;
    }

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int ledIndex = gridToSerpentine(x, y);
            int pixelIndex = 4 * (y * width + x);
            uint8_t r = image[pixelIndex];
            uint8_t g = image[pixelIndex + 1];
            uint8_t b = image[pixelIndex + 2];
            uint8_t a = image[pixelIndex + 3];

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

void scanImages(const std::string &directory, std::vector<std::string> &images)
{
    images.clear();
    DIR *dir = opendir(directory.c_str());
    if (dir == nullptr)
    {
        ESP_LOGE("scanImages", "Failed to open directory: %s", directory.c_str());
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_type == DT_REG)
        {
            std::string fileName = entry->d_name;
            if (fileName.substr(fileName.find_last_of(".") + 1) == "png")
            {
                images.push_back(directory + "/" + fileName);
            }
        }
    }
    closedir(dir);
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

    scanImages(imageDirectories[currentDirectoryIndex], images);
}

void loop()
{
    if (images.empty())
    {
        ESP_LOGE("loop", "No images found in directory: %s", imageDirectories[currentDirectoryIndex].c_str());
        return;
    }

    for (size_t i = 0; i < maxIterations; ++i)
    {
        for (size_t j = 0; j < images.size(); ++j)
        {
            const std::string &filepath = images[j];
            ESP_LOGI("loop", "Attempting to open: %s", filepath.c_str());
            drawImage(filepath.c_str());
            updatePixels();
            vTaskDelay(1000 / fpsValues[currentDirectoryIndex] / portTICK_PERIOD_MS);
        }
    }

    // Move to the next directory
    currentDirectoryIndex = (currentDirectoryIndex + 1) % imageDirectories.size();
    scanImages(imageDirectories[currentDirectoryIndex], images);
    currentImageIndex = 0; // Reset image index
    iterationCounter = 0;  // Reset iteration counter
}

void app_main()
{
    setup();
    for (;;)
    {
        loop();
    }
}
