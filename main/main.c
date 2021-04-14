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
#include "nvs_flash.h"
#include "nvs.h"
#include "UI_driver.h"
#include "Button_driver.h"
#include "INA_data_driver.h"

//Tag for ESP_LOG functions
static const char *TAG = "Master_Task";

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

#define calibrate 		0
#define main 			1
#define voltage 		2
#define variable 		3
#define statistics_p 	4
#define statistics_u 	5
#define statistics_i 	6

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
void Master_Task(void *pvParameters)
{
	// Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    
	//Init: Display, Buttons, IO and Buzzer
	UI_init(I2C_PORT, SDA_GPIO, SCL_GPIO);	

	//Init INAs
	INAD_init(I2C_PORT, SDA_GPIO, SCL_GPIO);

	//Init page selection variables
	int page_select = 1;
	int page_select_last = 1;
	int value_select = 0;
	int ENC_count = 0;
	int ENC_count_last = 0;
	int up_press = 0;
	int down_press = 0;
	int left_press = 0;
	int right_press = 0;
	int select_press = 0;


	//init Value variables
	uint8_t in_value = 0xFF;
	uint8_t out_value = 0x00;
	uint8_t button_last_1 = 0;
	uint8_t button_last_2 = 0;
	uint8_t button_last_3 = 0;
	uint8_t button_last_4 = 0;
	uint8_t button_last_5 = 0;
	int32_t INA1_S_val = 0;
	int32_t INA1_A_val = 0;
	int32_t INA2_S_val = 0;
	int32_t INA2_A_val = 0;
	double power_val = 0;
	double voltage_val = 0;
	double current_val = 0;
	bool output_val = 0;
	double uset_val = 0;
	double ueff_val = 0;
	int division_select = 0;
	int ENC_value = 0;
	uint16_t p_val[100];
	uint16_t u_val[100];
	uint16_t i_val[100];
	int select = 0;

	// Open NVS Handle
    printf("\n");
    printf("Opening NVS handle... ");
    nvs_handle INA_config_NVS;
    err = nvs_open("storage", NVS_READWRITE, &INA_config_NVS);
    if (err != ESP_OK) 
	{
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else 
	{
        printf("Done\n");

		// Reading from NVS
        printf("Reading INA1_S_val from NVS ... ");
        err = nvs_get_i32(INA_config_NVS, "INA1_S_val", &INA1_S_val);
		err = nvs_get_i32(INA_config_NVS, "INA1_A_val", &INA1_A_val);
		err = nvs_get_i32(INA_config_NVS, "INA2_S_val", &INA2_S_val);
		err = nvs_get_i32(INA_config_NVS, "INA2_A_val", &INA2_A_val);
        switch (err) {
            case ESP_OK:
                printf("NVS read successfully\n");
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
	}

	//set all elements of arrays to 0
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

	if(UI_get_press(sel)) 
	{
		page_select = 0;
		page_select_last = 0;
		ESP_LOGI(TAG, "Calibrate Screen entered");
	}
	
	UI_reset_all_states();

	while(1) 
	{
		switch(page_select)
		{
			case calibrate:
				//value selection up
				if(up_press)
				{
					up_press = 0;
					if(value_select > 0) value_select--;
					else if(value_select == 0) value_select = 3;
				}
				//value selection down
				if(down_press)
				{
					down_press = 0;
					if(value_select < 3) value_select++;
					else if(value_select == 3) value_select = 0;
				}
				//change value
				if(ENC_count != ENC_count_last)
				{
					//save difference in temporary variable
					double diff_count_temp = ENC_count - ENC_count_last;
					//add difference to selected value
					switch(value_select)
					{
						case 0: 
							if((INA1_S_val + diff_count_temp) > 0) INA1_S_val += diff_count_temp; 
						break;
						case 1: 
							if((INA1_A_val + diff_count_temp) > 0) INA1_A_val += diff_count_temp; 
						break;
						case 2: 
							if((INA2_S_val + diff_count_temp) > 0) INA2_S_val += diff_count_temp; 
						break;
						case 3:
							if((INA2_A_val + diff_count_temp) > 0) INA2_A_val += diff_count_temp; 
						break;
					}
				}
				//exit page
				if(left_press || right_press > 1)
				{
					err = nvs_set_i32(INA_config_NVS, "INA1_S_val", INA1_S_val);
					err = nvs_set_i32(INA_config_NVS, "INA1_A_val", INA1_A_val);
					err = nvs_set_i32(INA_config_NVS, "INA2_S_val", INA2_S_val);
					err = nvs_set_i32(INA_config_NVS, "INA2_A_val", INA2_A_val);
					printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

					// Commit written value.
					printf("Committing updates in NVS ... ");
					err = nvs_commit(INA_config_NVS);
					printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

					// Close
					nvs_close(INA_config_NVS);
					page_select = 1;
				}
				//draw Screen
				UI_draw_calibrate_screen(INA1_S_val, INA1_A_val, INA2_S_val, INA2_A_val, value_select);
			break;
			case main:
				power_val = INAD_getPower_mW(INA1);
				voltage_val = INAD_getVShunt_mv(INA1);
				current_val = INAD_getCurrent_mA(INA1);

				//change value
				if(ENC_count != ENC_count_last)
				{
					output_val = !output_val;
				}
				//change page -
				if(left_press > 1)
				{
					page_select = 6;
				}
				//change page +
				if(right_press > 1)
				{
					page_select = 2;
				}
				//draw Screen
				UI_draw_main_screen(power_val, voltage_val, current_val, output_val);
			break;	
			case voltage:
				//change value
				if(ENC_count != ENC_count_last)
				{
					output_val = !output_val;
				}
				//change page -
				if(left_press > 1)
				{
					page_select = 1;
				}
				//change page +
				if(right_press > 1)
				{
					page_select = 3;
				}
				//draw Screen
				UI_draw_voltages_screen(24, 5, 42.69, 3.3, output_val);
			break;
			case variable:
				//value selection up
				if(up_press)
				{
					up_press = 0;
					value_select = !value_select;
				}
				//value selection down
				if(down_press)
				{
					down_press = 0;
					value_select = !value_select;
				}
				//change value
				if(ENC_count != ENC_count_last)
				{
					//save difference in temporary variable
					double diff_count_temp = ENC_count - ENC_count_last;
					//add difference to selected value
					switch(value_select)
					{
						case 0: 
							diff_count_temp = diff_count_temp / 10;
							if((uset_val + diff_count_temp) > 0) uset_val += diff_count_temp; 
						break;
						case 1: 
							output_val = !output_val;
						break;
					}
				}
				//change page -
				if(left_press > 1)
				{
					page_select = 2;
				}
				//change page +
				if(right_press > 1)
				{
					page_select = 4;
				}
				//draw Screen
				UI_draw_variable_screen(uset_val, ueff_val, value_select, output_val);
			break;
			case statistics_p:
				//get power_val from INA1
				power_val = INAD_getPower_mW(INA1);
				//get values for p_val
				for(int i = 0; i < 50; i++)
				{
					p_val[i] = p_val[i+1]; 
				}
				p_val[49] = (power_val/Max_P_mW*60);
				//value selection up
				if(up_press)
				{
					up_press = 0;
					value_select = !value_select;
				}
				//value selection down
				if(down_press)
				{
					down_press = 0;
					value_select = !value_select;
				}
				//change value
				if(ENC_count != ENC_count_last)
				{
					//save difference in temporary variable
					double diff_count_temp = ENC_count - ENC_count_last;
					//add difference to selected value
					switch(value_select)
					{
						case 0: 
							if((division_select + diff_count_temp) >= 0 && (division_select + diff_count_temp) < 4) division_select += diff_count_temp; 
						break;
						case 1: 
							output_val = !output_val;
						break;
					}
				}
				//change page -
				if(left_press > 1)
				{
					page_select = 3;
				}
				//change page +
				if(right_press > 1)
				{
					page_select = 5;
				}
				//draw Screen
				UI_draw_statistics_screen(p_val, 0, division_select, value_select, output_val);
			break;
			case statistics_u:
				//get power_val from INA1
				voltage_val = INAD_getVShunt_mv(INA1);
				//get values for p_val
				for(int i = 0; i < 50; i++)
				{
					u_val[i] = u_val[i+1]; 
				}
				u_val[49] = (voltage_val/Max_U_mV*60);
				//value selection up
				if(up_press)
				{
					up_press = 0;
					value_select = !value_select;
				}
				//value selection down
				if(down_press)
				{
					down_press = 0;
					value_select = !value_select;
				}
				//change value
				if(ENC_count != ENC_count_last)
				{
					//save difference in temporary variable
					double diff_count_temp = ENC_count - ENC_count_last;
					ESP_LOGI(TAG, "%f", diff_count_temp);
					//add difference to selected value
					switch(value_select)
					{
						case 0: 
							if((division_select + diff_count_temp) >= 0 && (division_select + diff_count_temp) < 4) division_select += diff_count_temp; 
						break;
						case 1: 
							output_val = !output_val;
						break;
					}
				}
				//change page -
				if(left_press > 1)
				{
					page_select = 4;
				}
				//change page +
				if(right_press > 1)
				{
					page_select = 6;
				}
				//draw Screen
				UI_draw_statistics_screen(u_val, 1, division_select, value_select, output_val);
			break;
			case statistics_i:
				//get power_val from INA1
				current_val = INAD_getCurrent_mA(INA1);
				//get values for p_val
				for(int i = 0; i < 50; i++)
				{
					i_val[i] = i_val[i+1]; 
				}
				i_val[49] = (current_val/Max_I_mA*60);
				//value selection up
				if(up_press)
				{
					up_press = 0;
					value_select = !value_select;
				}
				//value selection down
				if(down_press)
				{
					down_press = 0;
					value_select = !value_select;
				}
				//change value
				if(ENC_count != ENC_count_last)
				{
					//save difference in temporary variable
					double diff_count_temp = ENC_count - ENC_count_last;
					ESP_LOGI(TAG, "%f", diff_count_temp);
					//add difference to selected value
					switch(value_select)
					{
						case 0: 
							if((division_select + diff_count_temp) >= 0 && (division_select + diff_count_temp) < 4) division_select += diff_count_temp; 
						break;
						case 1: 
							output_val = !output_val;
						break;
					}
				}
				//change page -
				if(left_press > 1)
				{
					page_select = 5;
				}
				//change page +
				if(right_press > 1)
				{
					page_select = 1;
				}
				//draw Screen
				UI_draw_statistics_screen(u_val, 2, division_select, value_select, output_val);
			break;

		}

		UI_Update();
		ENC_count_last = ENC_count;
		if(page_select == page_select_last)
		{
			up_press = UI_get_press(up); 
			down_press = UI_get_press(down);
			left_press = UI_get_press(left);
			right_press = UI_get_press(right);
			select_press = UI_get_press(sel);
			ENC_count = UI_get_ENC();
		}
		if(page_select != page_select_last)
		{
			UI_reset_all_states();
			up_press = 0;
			down_press = 0;
			left_press = 0;
			right_press = 0;
			select_press = 0;
			value_select = 0;
		}
		page_select_last = page_select;
		vTaskDelay(5 / portTICK_PERIOD_MS);


		/*
		//Button state test
		//ESP_LOGW(TAG, "Button Up = %d", UI_get_press(up));
		//get values to Display
		//in_value = Button_read_reg_0();
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
		
		p_val[49] = (power_val/Max_P_mW*60);
		//---------------------------------------------------u_val for statistics
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
				UI_set_ENC(6);
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
				UI_draw_statistics_screen(p_val, 0, select, 0, output_val);
			break;
			case 4:
				UI_draw_statistics_screen(u_val, 1, select, 1, output_val);
			break;
			case 5:
				UI_draw_statistics_screen(i_val, 2, select, 0, output_val);
			break;
			case 6:
				UI_draw_calibrate_screen(1.064, 0.654, 0.154, 1.674, select);
			break;
			default:
				UI_set_ENC(0);
			break;


		}
		
		//Update Display
		UI_Update();

		ENC_value = UI_get_ENC();
		ESP_LOGW(TAG, "Encoder state = %d", ENC_value);

		//Display free Heap size
		ESP_LOGI(__FUNCTION__, "Free Heap size: %d\n", xPortGetFreeHeapSize());
		vTaskDelay(5 / portTICK_PERIOD_MS);
	*/
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
	xTaskCreate(Master_Task, "Master_Task", 1024*8, NULL, 2, NULL);
}
