#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "ili9340.h"
#include "dfuncs.h"
#include "ui_driver.h"

TFT_t dev;
uint16_t model = 0x7735;
char file[32];
uint16_t color;
char text[40];
uint8_t ascii[40];
uint16_t xpos;
uint16_t ypos;

FontxFile fx16G[2];
FontxFile fx24G[2];
FontxFile fx32G[2];

FontxFile fx16M[2];
FontxFile fx24M[2];
FontxFile fx32M[2];

void UI_ST7735_init()
{
	InitFontx(fx16G,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
	InitFontx(fx24G,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
	InitFontx(fx32G,"/spiffs/ILGH32XB.FNT",""); // 16x32Dot Gothic

	InitFontx(fx16M,"/spiffs/ILMH16XB.FNT",""); // 8x16Dot Mincyo
	InitFontx(fx24M,"/spiffs/ILMH24XB.FNT",""); // 12x24Dot Mincyo
	InitFontx(fx32M,"/spiffs/ILMH32XB.FNT",""); // 16x32Dot Mincyo

    spi_master_init(&dev, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
    lcdInit(&dev, model, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);
    lcdSetFontDirection(&dev, 0);

    DF_print_fill_screen(BLACK);
	DF_VlcdUpdate(&dev);
	vTaskDelay(500 / portTICK_PERIOD_MS);
	
    color = WHITE;
	xpos = 10;
	ypos = 25;
	strcpy((char *)ascii, "Project:");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 45;
	strcpy((char *)ascii, "PSU_main");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 65;
	strcpy((char *)ascii, "Author:");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 85;
	strcpy((char *)ascii, "M.Maechler");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 105;
	strcpy((char *)ascii, "Version:");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 125;
	strcpy((char *)ascii, "1.0");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 145;
	strcpy((char *)ascii, "#");
	for(int i = 1; i < 15; i++)
	{
		xpos = i*8;
		DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
		DF_VlcdUpdate(&dev);
		vTaskDelay(150 / portTICK_PERIOD_MS);
	}
	
	

    DF_VlcdUpdate(&dev);
}

void UI_draw_test_screen(uint8_t in_value, uint8_t out_value, double current_val, double shunt_val)
{
	strcpy(file, "/spiffs/background.png");
	DF_print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

	color = WHITE;
	xpos = 40;
	ypos = 25;
	strcpy((char *)ascii, "TEST");
	DF_print_string(&dev, fx24G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 50;
	strcpy((char *)ascii, "Reg 0:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 70;
	strcpy((char *)ascii, "Reg 1:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 90;
	strcpy((char *)ascii, "INA I:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 110;
	strcpy((char *)ascii, "SHUNT:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 55;
	ypos = 50;
	sprintf(text, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(in_value));
	strcpy((char *)ascii, text);
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 55;
	ypos = 70;
	sprintf(text, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(out_value));
	strcpy((char *)ascii, text);
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 55;
	ypos = 90;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, current_val);
	xpos = 100;
	ypos = 90;
	strcpy((char *)ascii, "mA");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 55;
	ypos = 110;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, shunt_val);
	xpos = 100;
	ypos = 110;
	strcpy((char *)ascii, "mV");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
}

void UI_Update()
{
	DF_VlcdUpdate(&dev);
}