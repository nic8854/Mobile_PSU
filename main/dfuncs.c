#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "ili9340.h"
#include "pngle.h"
#include "decode_image.h"
#include "dfuncs.h"


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

int print_string(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, uint8_t * ascii, uint16_t color) {
	int length = strlen((char *)ascii);
	ESP_LOGW(__FUNCTION__,"lcdDrawString length=%d",length);
	for(int i=0;i<length;i++) {
		//printf("ascii[%d]=%x x=%d y=%d\n",i,ascii[i],x,y);
		if (dev->_font_direction == 0)
			x = lcdDrawChar(dev, fx, x, y, ascii[i], color);
		if (dev->_font_direction == 1)
			y = lcdDrawChar(dev, fx, x, y, ascii[i], color);
		if (dev->_font_direction == 2)
			x = lcdDrawChar(dev, fx, x, y, ascii[i], color);
		if (dev->_font_direction == 3)
			y = lcdDrawChar(dev, fx, x, y, ascii[i], color);
	}
	if (dev->_font_direction == 0) return x;
	if (dev->_font_direction == 2) return x;
	if (dev->_font_direction == 1) return y;
	if (dev->_font_direction == 3) return y;
	return 0;
}

TickType_t print_png(TFT_t * dev, char * file, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdSetFontDirection(dev, 0);
	lcdFillScreen(dev, BLACK);

	int _width = width;
	if (width > 240) _width = 240;
	int _height = height;
	if (height > 320) _height = 320;

	// open PNG file
	FILE* fp = fopen(file, "rb");
	if (fp == NULL) {
		ESP_LOGW(__FUNCTION__, "File not found [%s]", file);
		return 0;
	}

	char buf[1024];
	size_t remain = 0;
	int len;

	pngle_t *pngle = pngle_new(_width, _height);

	pngle_set_init_callback(pngle, print_png_init);
	pngle_set_draw_callback(pngle, print_png_draw);
	pngle_set_done_callback(pngle, print_png_finish);

	double display_gamma = 3;
	pngle_set_display_gamma(pngle, display_gamma);


	while (!feof(fp)) {
		if (remain >= sizeof(buf)) {
			ESP_LOGE(__FUNCTION__, "Buffer exceeded");
			while(1) vTaskDelay(1);
		}

		len = fread(buf + remain, 1, sizeof(buf) - remain, fp);
		if (len <= 0) {
			//printf("EOF\n");
			break;
		}

		int fed = pngle_feed(pngle, buf, remain + len);
		if (fed < 0) {
			ESP_LOGE(__FUNCTION__, "ERROR; %s", pngle_error(pngle));
			while(1) vTaskDelay(1);
		}

		remain = remain + len - fed;
		if (remain > 0) memmove(buf, buf + fed, remain);
	}

	fclose(fp);

	uint16_t pngWidth = width;
	uint16_t offsetX = 0;
	if (width > pngle->imageWidth) {
		pngWidth = pngle->imageWidth;
		offsetX = (width - pngle->imageWidth) / 2;
	}
	ESP_LOGD(__FUNCTION__, "pngWidth=%d offsetX=%d", pngWidth, offsetX);

	uint16_t pngHeight = height;
	uint16_t offsetY = 0;
	if (height > pngle->imageHeight) {
		pngHeight = pngle->imageHeight;
		offsetY = (height - pngle->imageHeight) / 2;
	}
	ESP_LOGD(__FUNCTION__, "pngHeight=%d offsetY=%d", pngHeight, offsetY);
	uint16_t *colors = (uint16_t*)malloc(sizeof(uint16_t) * pngWidth);

#if 0
	for(int y = 0; y < pngHeight; y++){
		for(int x = 0;x < pngWidth; x++){
			pixel_png pixel = pngle->pixels[y][x];
			uint16_t color = rgb565_conv(pixel.red, pixel.green, pixel.blue);
			lcdDrawPixel(dev, x+offsetX, y+offsetY, color);
		}
	}
#endif
	//pixel_png vscreen[width][height];

	for(int y = 0; y < pngHeight; y++){
		for(int x = 0;x < pngWidth; x++){
			pixel_png pixel = pngle->pixels[y][x];
			colors[x] = rgb565_conv(pixel.blue, pixel.green, pixel.red);
			//uint16_t color = rgb565_conv(pixel.red, pixel.green, pixel.blue);
			//colors[x] = ~color;

			//vscreen[x][y] = pngle->pixels[y][x];

		}
		lcdDrawMultiPixels(dev, offsetX, y+offsetY, pngWidth, colors);
		vTaskDelay(1);
	}
	free(colors);
	pngle_destroy(pngle, _width, _height);
	//return *vscreen;
	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "printing value");
	return diffTick;
}

void print_png_init(pngle_t *pngle, uint32_t w, uint32_t h)
{
	ESP_LOGD(__FUNCTION__, "print_png_init w=%d h=%d", w, h);
	ESP_LOGD(__FUNCTION__, "screenWidth=%d screenHeight=%d", pngle->screenWidth, pngle->screenHeight);
	pngle->imageWidth = w;
	pngle->imageHeight = h;
	pngle->reduction = false;
	pngle->scale_factor = 1.0;

	// Calculate Reduction
	if (pngle->screenWidth < pngle->imageWidth || pngle->screenHeight < pngle->imageHeight) {
		pngle->reduction = true;
		double factorWidth = (double)pngle->screenWidth / (double)pngle->imageWidth;
		double factorHeight = (double)pngle->screenHeight / (double)pngle->imageHeight;
		pngle->scale_factor = factorWidth;
		if (factorHeight < factorWidth) pngle->scale_factor = factorHeight;
		pngle->imageWidth = pngle->imageWidth * pngle->scale_factor;
		pngle->imageHeight = pngle->imageHeight * pngle->scale_factor;
	}
	ESP_LOGD(__FUNCTION__, "reduction=%d scale_factor=%f", pngle->reduction, pngle->scale_factor);
	ESP_LOGD(__FUNCTION__, "imageWidth=%d imageHeight=%d", pngle->imageWidth, pngle->imageHeight);
		
}

void print_png_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
	ESP_LOGD(__FUNCTION__, "print_png_draw x=%d y=%d w=%d h=%d", x,y,w,h);
#if 0
	uint8_t r = rgba[0];
	uint8_t g = rgba[1];
	uint8_t b = rgba[2];
#endif

	// image reduction
	uint32_t _x = x;
	uint32_t _y = y;
	if (pngle->reduction) {
		_x = x * pngle->scale_factor;
		_y = y * pngle->scale_factor;
	}
	if (_y < pngle->screenHeight && _x < pngle->screenWidth) {
		pngle->pixels[_y][_x].red = rgba[0];
		pngle->pixels[_y][_x].green = rgba[1];
		pngle->pixels[_y][_x].blue = rgba[2];
	}

}

void print_png_finish(pngle_t *pngle) {
	ESP_LOGD(__FUNCTION__, "print_png_finish");
}

