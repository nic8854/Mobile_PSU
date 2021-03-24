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
#include "ui_driver.h"
#include "Button_driver.h"
#include "INA_data_driver.h"

#define	INTERVAL		400
#define WAIT			vTaskDelay(INTERVAL)

static const char *TAG = "PSU_main";

#define GPIO_OUTPUT_IO_0    2
#define GPIO_OUTPUT_IO_1    26
#define GPIO_OUTPUT_IO_2    4

#define INA1 1
#define INA2 2

#define I2C_PORT 0
#define I2C_EXP_ADDR 0x20
#define I2C_INA1_ADDR 0x40
#define SDA_GPIO 21
#define SCL_GPIO 22
#define I2C_port 0

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

void PSU_main(void *pvParameters)
{
	UI_init(I2C_PORT, SDA_GPIO, SCL_GPIO);	

	INAD_init(I2C_PORT, SDA_GPIO, SCL_GPIO);

	//init variables
	uint8_t in_value = 0xFF;
	uint8_t out_value = 0x00;
	uint8_t button_last_1 = 0;
	uint8_t button_last_2 = 0;
	uint8_t button_last_3 = 0;
	uint8_t button_last_4 = 0;
	uint8_t button_last_5 = 0;
	double current_val = 0;
	double shunt_val = 0;

	
	while(1) {
			in_value = Button_read_reg_0();
			current_val = INAD_getCurrent_mA(INA1);
			shunt_val = INAD_getVShunt_mv(INA1);

			if(in_value & 0x08 && !button_last_1)
			{
				UI_GPIO_set(LED_0, 1);
				out_value = out_value ^ 0x01; //0x02 on PSU Board (TC_EN)
				vTaskDelay(50 / portTICK_PERIOD_MS);
				UI_GPIO_set(LED_0, 0);
				button_last_1 = 1;
			} 
			else if(!(in_value & 0x08) && button_last_1)
			{
				button_last_1 = 0;
			}
			if(in_value & 0x10 && !button_last_2)
			{
				UI_GPIO_set(LED_0, 1);
				out_value = out_value ^ 0x02; //0x04 on PSU Board (TC_NFON)
				vTaskDelay(50 / portTICK_PERIOD_MS);
				UI_GPIO_set(LED_0, 0);
				button_last_2 = 1;
			} 
			else if(!(in_value & 0x10) && button_last_2)
			{
				button_last_2 = 0;
			}
			if(in_value & 0x20 && !button_last_3) 
			{
				UI_GPIO_set(LED_0, 1);
				out_value = out_value ^ 0x10; //does not set EN_IN directly
				vTaskDelay(50 / portTICK_PERIOD_MS);
				UI_GPIO_set(LED_0, 0);
				button_last_3 = 1;
			}
			else if(!(in_value & 0x20) && button_last_3)
			{
				button_last_3 = 0;
			}
			if(in_value & 0x01 && !button_last_4) 
			{
				UI_GPIO_set(BUZZER, 1);
				UI_GPIO_set(LED_0, 1);
				vTaskDelay(50 / portTICK_PERIOD_MS);
				UI_GPIO_set(BUZZER, 0);
				UI_GPIO_set(LED_0, 0);
				vTaskDelay(50 / portTICK_PERIOD_MS);
				UI_GPIO_set(BUZZER, 1);
				UI_GPIO_set(LED_0, 1);
				vTaskDelay(50 / portTICK_PERIOD_MS);
				UI_GPIO_set(BUZZER, 0);
				UI_GPIO_set(LED_0, 0);
				button_last_4 = 1;
			}
			else if(!(in_value & 0x01) && button_last_4)
			{
				button_last_4 = 0;
			}
			if(in_value & 0x02 && !button_last_5) 
			{
				UI_GPIO_set(LED_0, 1);
				out_value = out_value ^ 0x20; //does not set EN_IN directly
				vTaskDelay(50 / portTICK_PERIOD_MS);
				UI_GPIO_set(LED_0, 0);
				button_last_5 = 1;
			}
			else if(!(in_value & 0x02) && button_last_5)
			{
				button_last_5 = 0;
			}
			Button_write_reg_1(out_value);
			if(out_value & 0x10) UI_GPIO_set(LED_1, 1);
			else UI_GPIO_set(LED_1, 0);
			if(out_value & 0x20) led_test(1);
			if(!(out_value & 0x20)) led_test(0);
			ESP_LOGW(__FUNCTION__, "Expander Read Reg 0 = 0b"BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(in_value));
			ESP_LOGW(__FUNCTION__, "Expander Write Reg 1 = 0b"BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(out_value));

		UI_draw_test_screen(in_value, out_value, current_val, shunt_val);

		UI_Update();
	
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
	xTaskCreate(PSU_main, "PSU_MAIN", 1024*8, NULL, 2, NULL);
}
