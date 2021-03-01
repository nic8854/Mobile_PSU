#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_heap_caps.h"
#include "dfuncs.h"
#include "expander_driver.h"

#include "ili9340.h"
#include "fontx.h"
#include "bmpfile.h"
#include "decode_image.h"
#include "pngle.h"

#define	INTERVAL		400
#define WAIT			vTaskDelay(INTERVAL)

static const char *TAG = "ILI9340";

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


static void SPIFFS_Directory(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(__FUNCTION__,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

TickType_t PNGTest(TFT_t * dev, char * file, int width, int height) {
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
	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "printing value");
	return diffTick;
}

void ILI9341(void *pvParameters)
{
	// set font file
	FontxFile fx16G[2];
	FontxFile fx24G[2];
	FontxFile fx32G[2];
	InitFontx(fx16G,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
	InitFontx(fx24G,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
	InitFontx(fx32G,"/spiffs/ILGH32XB.FNT",""); // 16x32Dot Gothic

	FontxFile fx16M[2];
	FontxFile fx24M[2];
	FontxFile fx32M[2];
	InitFontx(fx16M,"/spiffs/ILMH16XB.FNT",""); // 8x16Dot Mincyo
	InitFontx(fx24M,"/spiffs/ILMH24XB.FNT",""); // 12x24Dot Mincyo
	InitFontx(fx32M,"/spiffs/ILMH32XB.FNT",""); // 16x32Dot Mincyo
	
	TFT_t dev;
	spi_master_init(&dev, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);



#if CONFIG_ILI9225
	uint16_t model = 0x9225;
#endif
#if CONFIG_ILI9225G
	uint16_t model = 0x9226;
#endif
#if CONFIG_ILI9340
	uint16_t model = 0x9340;
#endif
#if CONFIG_ILI9341
	uint16_t model = 0x9341;
#endif
#if CONFIG_ST7735
	uint16_t model = 0x7735;
#endif
#if CONFIG_ST7796
	uint16_t model = 0x7796;
#endif
	lcdInit(&dev, model, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);

	char file[32];

	uint16_t color;
	char text[40];
	uint8_t ascii[40];
	color = WHITE;
	lcdSetFontDirection(&dev, 0);
	uint16_t xpos = 35;
	uint16_t ypos = 25;
	int Inhalt[3] = {987, 654, 321};

	#define I2C_PORT 0
	#define I2C_ADDR 0x20
	#define SDA_GPIO 21
	#define SCL_GPIO 19

	uint8_t in_value = 0xFF;
	uint8_t out_value = 0x00;
	int error_code = 0;
	int return_value = 0;

	expander_t dev_port_expander;
	conf_t config = Default_Config;
	memset(&dev_port_expander, 0, sizeof(expander_t));
	config.conf_port_0 = 0xFF;
	config.conf_port_1 = 0x00;
	config.pol_inv_0 = 0xFF;
	config.pol_inv_1 = 0x00;
	expander_init_desc(&dev_port_expander, I2C_ADDR, I2C_PORT, SDA_GPIO, SCL_GPIO);
	expander_configure(&dev_port_expander, &config);
	

	while(1) {
		
		for(int i = 0; i < 5; i++)
		{
			write_reg_8(&dev_port_expander, reg_out_port_1, 0x01);	
			vTaskDelay(15);
			write_reg_8(&dev_port_expander, reg_out_port_1, 0x02);
			vTaskDelay(15);
		}
		write_reg_8(&dev_port_expander, reg_out_port_1, 0x00);
		strcpy(file, "/spiffs/background.png");
		print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

		color = WHITE;
		xpos = 35;
		ypos = 25;
		strcpy((char *)ascii, "Titel");
		return_value = print_string(&dev, fx24G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		xpos = 5;
		ypos = 50;
		strcpy((char *)ascii, "Tasten:");
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		xpos = 5;
		ypos = 70;
		strcpy((char *)ascii, "Inhalt:");
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		xpos = 5;
		ypos = 90;
		strcpy((char *)ascii, "Inhalt:");
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		xpos = 65;
		ypos = 50;
		sprintf(text, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(out_value));
		strcpy((char *)ascii, text);
		for(int i = 0; i < 10; i++)
		{
			ascii[i] = ascii[i+2];
		}
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		xpos = 65;
		ypos = 70;
		return_value = print_value(&dev, color, fx16G, xpos, ypos, Inhalt[1], -1);
		if(return_value < error_code) error_code = return_value;
		xpos = 65;
		ypos = 90;
		return_value = print_value(&dev, color, fx16G, xpos, ypos, Inhalt[2], -1);
		if(return_value < error_code) error_code = return_value;
		VlcdUpdate(&dev);
		if(error_code < 0) 
		{
			ESP_LOGE(__FUNCTION__, "Error accured in print_string function");
		}
		error_code = 0;
		return_value = 0;
		read_reg_8(&dev_port_expander, reg_in_port_0, &out_value);
		ESP_LOGI(__FUNCTION__, "%d\n", xPortGetFreeHeapSize());
		vTaskDelay(10);
	
	} // end while

	// never reach
	while (1) {
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 8,
		.format_if_mount_failed =true
	};

	// Use settings defined above toinitialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is anall-in-one convenience function.
	esp_err_t ret =esp_vfs_spiffs_register(&conf);

	i2cdev_init();

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
		}
		return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total,&used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
	}

	SPIFFS_Directory("/spiffs/");
	xTaskCreate(ILI9341, "ILI9341", 1024*8, NULL, 2, NULL);
}
