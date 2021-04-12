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
#include "UI_driver.h"
#include "Button_driver.h"
#include "INA_data_driver.h"

//Tag for ESP_LOG functions
static const char *TAG = "PSU_main";

#define Max_P_mW 25
#define Max_U_mV 7
#define Max_I_mA 5

//define I2C Pins
#define I2C_PORT 0
#define SDA_GPIO 21
#define SCL_GPIO 22
#define I2C_port 0

#define up      0
#define down    1
#define left    2
#define right   3
#define sel     4

//Define Spiffs
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

//main Task
void PSU_main(void *pvParameters)
{
	//Init: Display, Buttons, IO and Buzzer
	UI_init(I2C_PORT, SDA_GPIO, SCL_GPIO);	

	//Init INAs
	INAD_init(I2C_PORT, SDA_GPIO, SCL_GPIO);

	//init variables
	uint8_t in_value = 0xFF;
	uint8_t out_value = 0x00;
	uint8_t button_last_1 = 0;
	uint8_t button_last_2 = 0;
	uint8_t button_last_3 = 0;
	uint8_t button_last_4 = 0;
	uint8_t button_last_5 = 0;
	double power_val = 0;
	double voltage_val = 0;
	double current_val = 0;
	bool output_val = 0;
	int ENC_value = 0;
	uint16_t p_val[100];
	uint16_t u_val[100];
	uint16_t i_val[100];
	int select = 0;

	for(int i = 0; i < 100; i++)
	{
		p_val[i] = 0; 
	}
	for(int i = 0; i < 100; i++)
	{
		u_val[i] = 0; 
	}
	for(int i = 0; i < 100; i++)
	{
		i_val[i] = 0; 
	}
	
	while(1) 
	{
		//Button state test
		ESP_LOGW(TAG, "Button Up = %d", UI_get_press(up));
		//get values to Display
		in_value = Button_read_reg_0();
		power_val = INAD_getPower_mW(INA1);
		voltage_val = INAD_getVShunt_mv(INA1);
		current_val = INAD_getCurrent_mA(INA1);

		if(in_value & 0x04) select++;
		if(select == 4) select = 0;

		//---------------------------------------------------p_val for statistics
		for(int i = 0; i < 50; i++)
		{
			p_val[i] = p_val[i+1]; 
		}
		//---------------------------------------------------u_val for statistics
		p_val[49] = (power_val/Max_P_mW*60);
		for(int i = 0; i < 50; i++)
		{
			u_val[i] = u_val[i+1]; 
		}
		//---------------------------------------------------i_val for statistics
		u_val[49] = (voltage_val/Max_U_mV*60);
		for(int i = 0; i < 50; i++)
		{
			i_val[i] = i_val[i+1]; 
		}
		i_val[49] = (current_val/Max_I_mA*60);

		//---------------------------------------------------TC_EN
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
		//---------------------------------------------------TC_NFON
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
		//---------------------------------------------------EN_IN
		if(in_value & 0x20 && !button_last_3) 
		{
			UI_GPIO_set(LED_0, 1);
			out_value = out_value ^ 0x10; //does not set EN_IN directly
			output_val =!output_val;
			vTaskDelay(50 / portTICK_PERIOD_MS);
			UI_GPIO_set(LED_0, 0);
			button_last_3 = 1;
		}
		else if(!(in_value & 0x20) && button_last_3)
		{
			button_last_3 = 0;
		}
		//---------------------------------------------------BUZZER
		if(in_value & 0x01 && !button_last_4) 
		{
			UI_Buzzer_power(1);
			UI_Buzzer_PWM(100);
			UI_GPIO_set(LED_0, 1);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			UI_Buzzer_PWM(300);
			UI_GPIO_set(LED_0, 0);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			UI_Buzzer_PWM(500);
			UI_GPIO_set(LED_0, 1);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			UI_Buzzer_PWM(700);
			UI_GPIO_set(LED_0, 0);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			UI_Buzzer_PWM(900);
			UI_GPIO_set(LED_0, 1);
			vTaskDelay(200 / portTICK_PERIOD_MS);
			UI_Buzzer_PWM(700);
			UI_GPIO_set(LED_0, 0);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			UI_Buzzer_PWM(500);
			UI_GPIO_set(LED_0, 1);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			UI_Buzzer_PWM(300);
			UI_GPIO_set(LED_0, 0);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			UI_Buzzer_PWM(100);
			UI_GPIO_set(LED_0, 0);
			vTaskDelay(200 / portTICK_PERIOD_MS);
			UI_Buzzer_power(0);
			button_last_4 = 1;
		}
		else if(!(in_value & 0x01) && button_last_4)
		{
			button_last_4 = 0;
		}
		//---------------------------------------------------RGB_LEDs
		if(in_value & 0x02 && !button_last_5) 
		{
			UI_GPIO_set(LED_0, 1);
			out_value = out_value ^ 0x20;
			vTaskDelay(50 / portTICK_PERIOD_MS);
			UI_GPIO_set(LED_0, 0);
			button_last_5 = 1;
		}
		else if(!(in_value & 0x02) && button_last_5)
		{
			button_last_5 = 0;
		}
		//Write Expander
		Button_write_reg_1(out_value);
		//Set EN_IN
		if(out_value & 0x10) UI_GPIO_set(LED_1, 1);
		else UI_GPIO_set(LED_1, 0);
		//Set RGB LEDs
		if(out_value & 0x20) led_test(1);
		if(!(out_value & 0x20)) led_test(0);
		ESP_LOGW(__FUNCTION__, "Expander Read Reg 0 = 0b"BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(in_value));
		ESP_LOGW(__FUNCTION__, "Expander Write Reg 1 = 0b"BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(out_value));
		//Draw UI

		switch(ENC_value)
		{
			case -1:
				Button_ENC_set(6);
			break;
			case 0:
				UI_draw_main_screen(power_val, voltage_val, current_val, output_val);
			break;
			case 1:
				UI_draw_voltages_screen(42.0, 6.9, 4.2, 69.0, output_val);
			break;
			case 2:
				UI_draw_variable_screen(42.0, 6.9, (select%2), output_val);
			break;
			case 3:
				UI_draw_statistics_screen(p_val, 0, select, output_val);
			break;
			case 4:
				UI_draw_statistics_screen(u_val, 1, select, output_val);
			break;
			case 5:
				UI_draw_statistics_screen(i_val, 2, select, output_val);
			break;
			case 6:
				UI_draw_calibrate_screen(1.064, 0.654, 0.154, 1.674, select);
			break;
			default:
				Button_ENC_set(0);
			break;


		}
		
		//Update Display
		UI_Update();

		ENC_value = Button_ENC_get();
		ESP_LOGW(TAG, "Encoder state = %d", ENC_value);

		//Display free Heap size
		ESP_LOGI(__FUNCTION__, "Free Heap size: %d\n", xPortGetFreeHeapSize());
		vTaskDelay(5 / portTICK_PERIOD_MS);
	
	} // end while

	// never reach
	while (1) {
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	ESP_LOGI(TAG, "Initializing SPIFFS");
	//Initialize Spiffs
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 8,
		.format_if_mount_failed =true
	};

	// Use settings defined above toinitialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is anall-in-one convenience function.
	esp_err_t ret =esp_vfs_spiffs_register(&conf);

	//Init I2C Bus
	i2cdev_init();

	//Spiff Error Messages
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
	//Display Spiff Information
	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total,&used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
	}
	//Define SPiff Directories as /spiffs/...
	SPIFFS_Directory("/spiffs/");

	//Create Main Task
	xTaskCreate(PSU_main, "PSU_MAIN", 1024*8, NULL, 2, NULL);
}
