#ifndef APA102_H
#define APA102_H

#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#define LED_MOSI 14
#define LED_SCLK 5


typedef struct {
    uint8_t brightness;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} PixelInfo_t;

void APA102_Init(size_t ledsCount, spi_host_device_t spiDevice_LED);
void flush();
void setPixel(uint8_t index, int brightness, int red, int green, int blue);

#endif