#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "ili9340.h"


void print_value(TFT_t dev, uint16_t color, FontxFile font[2], uint16_t xpos, uint16_t ypos, int int_value, float float_value)
{
	lcdSetFontDirection(&dev, 0);
	char text[40];
	uint8_t ascii[40];
	if(int_value != -1)
	{
		ESP_LOGW(__FUNCTION__, "Int Value: %d", int_value);
		sprintf(text, "%d", int_value);
	}
	if(float_value != -1)
	{
		ESP_LOGW(__FUNCTION__, "Float Value: %.2f", float_value);
		sprintf(text, "%.2f", float_value);
	}
	strcpy((char *)ascii, text);
	lcdDrawString(&dev, font, xpos, ypos, ascii, color);
	
}