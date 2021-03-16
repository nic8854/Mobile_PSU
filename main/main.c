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
#include "driver/gpio.h"
#include "ili9340.h"
#include "fontx.h"
#include "bmpfile.h"
#include "decode_image.h"
#include "pngle.h"
#include "INA220.h"

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

#define GPIO_OUTPUT_IO_0    2
#define GPIO_OUTPUT_IO_1    26
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))


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

	#define I2C_PORT 0
	#define I2C_EXP_ADDR 0x20
	#define I2C_INA_ADDR 0x40
	#define SDA_GPIO 21
	#define SCL_GPIO 22

	
	//init ina object
	ina220_t dev_ina_1;
	ina220_params_t ina_params;
	ina220_init_default_params(&ina_params);
	memset(&dev_ina_1, 0, sizeof(ina220_t));

	//init expander object
	expander_t dev_port_expander;
	memset(&dev_port_expander, 0, sizeof(expander_t));

	//init expander config object
	conf_t config = Default_Config;
	config.conf_port_0 = 0xFF;
	config.conf_port_1 = 0x00;
	config.pol_inv_0 = 0xFF;
	config.pol_inv_1 = 0x00;

	//init GPIO config object
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	//init variables
	uint8_t in_value = 0xFF;
	uint8_t out_value = 0x00;
	int error_code = 0;
	int return_value = 0;
	uint8_t button_last_1 = 0;
	uint8_t button_last_2 = 0;
	uint8_t button_last_3 = 0;
	double current_val = 0;
	double shunt_val = 0;

	//init and configure expander
	expander_init_desc(&dev_port_expander, I2C_EXP_ADDR, I2C_PORT, SDA_GPIO, SCL_GPIO);
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	expander_configure(&dev_port_expander, &config);

	//init and configure INA220
	ina220_init_desc(&dev_ina_1, I2C_INA_ADDR, I2C_PORT, SDA_GPIO, SCL_GPIO);
	ina220_init(&dev_ina_1, &ina_params);
	ina220_setCalibrationData(&dev_ina_1, &ina_params, 0.1, 1.4);
	

	while(1) {
			read_reg_8(&dev_port_expander, reg_in_port_0, &in_value);
			current_val = ina220_getCurrent_mA(&dev_ina_1, &ina_params);
			shunt_val = ina220_getVShunt_mv(&dev_ina_1, &ina_params);

			if(in_value & 0x08 && !button_last_1)
			{
				gpio_set_level(GPIO_OUTPUT_IO_0, 1);
				out_value = out_value ^ 0x01; //0x02 on PSU Board (TC_EN)
				vTaskDelay(50 / portTICK_PERIOD_MS);
				gpio_set_level(GPIO_OUTPUT_IO_0, 0);
				button_last_1 = 1;
			} 
			else if(!(in_value & 0x08) && button_last_1)
			{
				button_last_1 = 0;
			}
			if(in_value & 0x10 && !button_last_2)
			{
				gpio_set_level(GPIO_OUTPUT_IO_0, 1);
				out_value = out_value ^ 0x02; //0x04 on PSU Board (TC_NFON)
				vTaskDelay(50 / portTICK_PERIOD_MS);
				gpio_set_level(GPIO_OUTPUT_IO_0, 0);
				button_last_2 = 1;
			} 
			else if(!(in_value & 0x10) && button_last_2)
			{
				button_last_2 = 0;
			}
			if(in_value & 0x20 && !button_last_3) 
			{
				gpio_set_level(GPIO_OUTPUT_IO_0, 1);
				out_value = out_value ^ 0x10; //does not set EN_IN directly
				vTaskDelay(50 / portTICK_PERIOD_MS);
				gpio_set_level(GPIO_OUTPUT_IO_0, 0);
				button_last_3 = 1;
			}
			else if(!(in_value & 0x20) && button_last_3)
			{
				button_last_3 = 0;
			}
			write_reg_8(&dev_port_expander, reg_out_port_1, out_value);
			if(out_value & 0x10) gpio_set_level(GPIO_OUTPUT_IO_1, 1);
			else gpio_set_level(GPIO_OUTPUT_IO_1, 0);
			ESP_LOGW(__FUNCTION__, "Expander Read Reg 0 = 0b"BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(in_value));
			ESP_LOGW(__FUNCTION__, "Expander Write Reg 1 = 0b"BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(out_value));

		strcpy(file, "/spiffs/background.png");
		print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

		color = WHITE;
		xpos = 40;
		ypos = 25;
		strcpy((char *)ascii, "TEST");
		return_value = print_string(&dev, fx24G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		xpos = 5;
		ypos = 50;
		strcpy((char *)ascii, "Reg 0:");
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		xpos = 5;
		ypos = 70;
		strcpy((char *)ascii, "Reg 1:");
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		xpos = 5;
		ypos = 90;
		strcpy((char *)ascii, "INA I:");
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		xpos = 5;
		ypos = 110;
		strcpy((char *)ascii, "SHUNT:");
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;

		xpos = 55;
		ypos = 50;
		sprintf(text, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(in_value));
		strcpy((char *)ascii, text);
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		xpos = 55;
		ypos = 70;
		sprintf(text, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(out_value));
		strcpy((char *)ascii, text);
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		xpos = 55;
		ypos = 90;
		return_value = print_value(&dev, color, fx16G, xpos, ypos, -1, current_val);
		if(return_value < error_code) error_code = return_value;
		if(return_value < error_code) error_code = return_value;
		xpos = 100;
		ypos = 90;
		strcpy((char *)ascii, "mA");
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		xpos = 55;
		ypos = 110;
		return_value = print_value(&dev, color, fx16G, xpos, ypos, -1, shunt_val);
		if(return_value < error_code) error_code = return_value;
		xpos = 100;
		ypos = 110;
		strcpy((char *)ascii, "mV");
		return_value = print_string(&dev, fx16G, xpos, ypos, ascii, color);
		if(return_value < error_code) error_code = return_value;
		VlcdUpdate(&dev);
		if(error_code < 0) 
		{
			ESP_LOGE(__FUNCTION__, "Error accured in print_string function");
		}
		error_code = 0;
		return_value = 0;
		ESP_LOGI(__FUNCTION__, "Free Heap size: %d\n", xPortGetFreeHeapSize());
		vTaskDelay(10 / portTICK_PERIOD_MS);
	
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
