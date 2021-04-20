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
#include "ADC_data_driver.h"

//Tag for ESP_LOG functions
static const char *TAG = "Master_Task";

//define max values
#define Max_P_mW 25
#define Max_U_mV 7
#define Max_I_mA 5

//ADC calibration value
#define out24_cal 240
#define out5_cal 50
#define out33_cal 33
#define outvar_cal 260

//ADC value at 1V
#define ADC_cal 34100

//define I2C Pins
#define I2C_PORT 0
#define SDA_GPIO 21
#define SCL_GPIO 22

//define Button directions
#define up      0
#define down    1
#define left    2
#define right   3
#define sel     4

//define screens for switch case
#define calibrate 		0
#define main 			1
#define voltage 		2
#define variable 		3
#define statistics_p 	4
#define statistics_u 	5
#define statistics_i 	6
#define tcbus			7

//Subtask Functions
void calibrate_func(void);
void main_func(void);
void voltages_func(void);
void variable_func(void);
void statistics_p_func(void);
void statistics_u_func(void);
void statistics_i_func(void);
void tcbus_func(void);
void house_keeping(void);

//Function for Spiffs
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

//Init internal variables
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
bool siren_toggle = 0;

//init Value variables
nvs_handle INA_config_NVS;
esp_err_t err;
int32_t INA1_S_val = 0;
int32_t INA1_A_val = 0;
int32_t INA2_S_val = 0;
int32_t INA2_A_val = 0;
double power_val = 0;
double voltage_val = 0;
double current_val = 0;
uint16_t adc1_read = 0;
uint16_t adc2_read = 0;
uint16_t adc3_read = 0;
uint16_t adc4_read = 0;
uint16_t adc5_read = 0;
double out24_val = 0;
double out5_val = 0;
double out33_val = 0;
double outvar_val = 0;
bool output_val = 0;
double uset_val = 0;
double ueff_val = 0;
int division_select = 0;
bool TC_EN_val = 0;
bool TC_NFON_val = 0;
APA102_t RGB_0;
APA102_t RGB_1;
uint16_t p_val[100];
uint16_t u_val[100];
uint16_t i_val[100];

//main Task
void Master_Task(void *pvParameters)
{
	// Initialize NVS
    err = nvs_flash_init();
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

	//Init ADC
	ADCD_init(I2C_PORT, SDA_GPIO, SCL_GPIO);

	
	// Open NVS Handle
    printf("\n");
    printf("Opening NVS handle... ");
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

	//check sel press for calibrate screen
	if(UI_get_press(sel)) 
	{
		page_select = 0;
		page_select_last = 0;
		ESP_LOGI(TAG, "Calibrate Screen entered");
	}

	//set RGB_led values to zero
	RGB_0.bright = 0;
	RGB_0.red = 0;
	RGB_0.green = 0;
	RGB_0.blue = 0;
	RGB_1.bright = 0;
	RGB_1.red = 0;
	RGB_1.green = 0;
	RGB_1.blue = 0;

	UI_reset_all_states();

	while(1) 
	{
		//measure values for overcurrent and overvoltage detection
		power_val = INAD_getPower_mW(INA1);
		voltage_val = INAD_getVShunt_mv(INA1);
		current_val = INAD_getCurrent_mA(INA1);

		switch(page_select)
		{
			case calibrate:
				calibrate_func();
			break;
			case main:
				main_func();
			break;	
			case voltage:
				voltages_func();
			break;
			case variable:
				variable_func();
			break;
			case statistics_p:
				statistics_p_func();
			break;
			case statistics_u:
				statistics_u_func();
			break;
			case statistics_i:
				statistics_i_func();
			break;
			case tcbus:
				tcbus_func();
			break;
		}
		//do everything that needs to be done every loop (including the delay)
		house_keeping();
	}

	// never reach
	while (1) {
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}

void calibrate_func(void)
{
	//value selection up
	if(up_press)
	{
		UI_Buzzer_beep();
		up_press = 0;
		if(value_select > 0) value_select--;
		else if(value_select == 0) value_select = 3;
	}
	//value selection down
	if(down_press)
	{
		UI_Buzzer_beep();
		down_press = 0;
		if(value_select < 3) value_select++;
		else if(value_select == 3) value_select = 0;
	}
	//change value
	if(ENC_count != ENC_count_last)
	{
		UI_Buzzer_beep();
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
		UI_Buzzer_beep();
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
		page_select = main;
	}
	//draw Screen
	UI_draw_calibrate_screen(INA1_S_val, INA1_A_val, INA2_S_val, INA2_A_val, value_select);
}
void main_func(void)
{
	//change value
	if(ENC_count != ENC_count_last)
	{
		UI_Buzzer_beep();
		output_val = !output_val;
	}
	//change page -
	if(left_press > 0)
	{
		UI_Buzzer_beep();
		page_select = tcbus;
	}
	//change page +
	if(right_press > 0)
	{
		UI_Buzzer_beep();
		page_select = voltage;
	}
	//draw Screen
	UI_draw_main_screen(power_val, voltage_val, current_val, output_val);
}
void voltages_func(void)
{
	adc1_read = ADCD_get(1);
	adc2_read = ADCD_get(2);
	adc3_read = ADCD_get(3);
	adc4_read = ADCD_get(4);

	out24_val = (double)adc1_read * out24_cal / ADC_cal;
	out5_val = (double)(adc2_read - 0x1000) * out5_cal / ADC_cal;
	out33_val = (double)(adc3_read - 0x2000) * out33_cal / ADC_cal;
	outvar_val = (double)(adc4_read - 0x3000) * outvar_cal / ADC_cal;

	//change value
	if(ENC_count != ENC_count_last)
	{
		UI_Buzzer_beep();
		output_val = !output_val;
	}
	//change page -
	if(left_press > 0)
	{
		UI_Buzzer_beep();
		page_select = main;
	}
	//change page +
	if(right_press > 0)
	{
		UI_Buzzer_beep();
		page_select = variable;
	}
	//draw Screen
	UI_draw_voltages_screen(out24_val, out5_val, outvar_val, out33_val, output_val);
}
void variable_func(void)
{
	//value selection up
	if(up_press)
	{
		UI_Buzzer_beep();
		up_press = 0;
		value_select = !value_select;
	}
	//value selection down
	if(down_press)
	{
		UI_Buzzer_beep();
		down_press = 0;
		value_select = !value_select;
	}
	//change value
	if(ENC_count != ENC_count_last)
	{
		UI_Buzzer_beep();
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
	if(left_press > 0)
	{
		UI_Buzzer_beep();
		page_select = voltage;
	}
	//change page +
	if(right_press > 0)
	{
		UI_Buzzer_beep();
		page_select = statistics_p;
	}
	//draw Screen
	UI_draw_variable_screen(uset_val, ueff_val, value_select, output_val);
}
void statistics_p_func(void)
{
	//get values for p_val
	for(int i = 0; i < 50; i++)
	{
		p_val[i] = p_val[i+1]; 
	}
	p_val[49] = (power_val/Max_P_mW*60);
	//value selection up
	if(up_press)
	{
		UI_Buzzer_beep();
		up_press = 0;
		value_select = !value_select;
	}
	//value selection down
	if(down_press)
	{
		UI_Buzzer_beep();
		down_press = 0;
		value_select = !value_select;
	}
	//change value
	if(ENC_count != ENC_count_last)
	{
		UI_Buzzer_beep();
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
	if(left_press > 0)
	{
		UI_Buzzer_beep();
		page_select = variable;
	}
	//change page +
	if(right_press > 0)
	{
		UI_Buzzer_beep();
		page_select = statistics_u;
	}
	//draw Screen
	UI_draw_statistics_screen(p_val, 0, division_select, value_select, output_val);
}
void statistics_u_func(void)
{
	//get values for p_val
	for(int i = 0; i < 50; i++)
	{
		u_val[i] = u_val[i+1]; 
	}
	u_val[49] = (voltage_val/Max_U_mV*60);
	//value selection up
	if(up_press)
	{
		UI_Buzzer_beep();
		up_press = 0;
		value_select = !value_select;
	}
	//value selection down
	if(down_press)
	{
		UI_Buzzer_beep();
		down_press = 0;
		value_select = !value_select;
	}
	//change value
	if(ENC_count != ENC_count_last)
	{
		UI_Buzzer_beep();
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
	if(left_press > 0)
	{
		UI_Buzzer_beep();
		page_select = statistics_p;
	}
	//change page +
	if(right_press > 0)
	{
		UI_Buzzer_beep();
		page_select = statistics_i;
	}
	//draw Screen
	UI_draw_statistics_screen(u_val, 1, division_select, value_select, output_val);
}
void statistics_i_func(void)
{
	//get values for p_val
	for(int i = 0; i < 50; i++)
	{
		i_val[i] = i_val[i+1]; 
	}
	i_val[49] = (current_val/Max_I_mA*60);
	//value selection up+
	if(up_press)
	{
		UI_Buzzer_beep();
		up_press = 0;
		value_select = !value_select;
	}
	//value selection down
	if(down_press)
	{
		UI_Buzzer_beep();
		down_press = 0;
		value_select = !value_select;
	}
	//change value
	if(ENC_count != ENC_count_last)
	{
		UI_Buzzer_beep();
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
	if(left_press > 0)
	{
		UI_Buzzer_beep();
		page_select = statistics_u;
	}
	//change page +
	if(right_press > 0)
	{
		UI_Buzzer_beep();
		page_select = tcbus;
	}
	//draw Screen
	UI_draw_statistics_screen(i_val, 2, division_select, value_select, output_val);
}
void tcbus_func(void)
{
	//value selection up
	if(up_press)
	{
		UI_Buzzer_beep();
		up_press = 0;
		if(value_select > 0) value_select--;
		else if(value_select == 0) value_select = 3;
	}
	//value selection down
	if(down_press)
	{
		UI_Buzzer_beep();
		down_press = 0;
		if(value_select < 2) value_select++;
		else if(value_select == 2) value_select = 0;
	}
	//change value
	if(ENC_count != ENC_count_last)
	{
		UI_Buzzer_beep();
		//add difference to selected value
		switch(value_select)
		{
			case 0: 
				TC_EN_val = !TC_EN_val;
			break;
			case 1: 
				TC_NFON_val = !TC_NFON_val;
			break;
			case 2: 
				output_val = !output_val;
			break;
		}
	}
	//change page -
	if(left_press > 0)
	{
		UI_Buzzer_beep();
		page_select = statistics_i;
	}
	//change page +
	if(right_press > 0)
	{
		UI_Buzzer_beep();
		page_select = main;
	}
	//draw Screen
	UI_draw_tcbus_screen(TC_EN_val, TC_NFON_val, output_val, value_select);
}
void house_keeping(void)
{
	//set Expander value for TC_NFON anf TC_EN
	if(TC_EN_val) UI_set_TC_EN(1);
	else UI_set_TC_EN(0);
	if(TC_NFON_val) UI_set_TC_NFON(1);
	else UI_set_TC_NFON(0);
	//overcurrent or overvoltage siren
	if(voltage_val > Max_U_mV || current_val > Max_I_mA)
	{
		//Buzzer on
		UI_Buzzer_power(1);
		//toggle Siren mode
		siren_toggle = !siren_toggle;

		//RGB0 red and low tone
		if(siren_toggle)
		{
			//LED1 red and Buzzer frequency low
			UI_Buzzer_PWM(400);
			RGB_0.bright = 10;
			RGB_0.red = 250;
			RGB_0.green = 0;
			RGB_0.blue = 0;
			RGB_1.bright = 0;
			RGB_1.red = 0;
			RGB_1.green = 0;
			RGB_1.blue = 0;
		}
		//RGB1 red and high tone
		else
		{
			//LED0 red and Buzzer frequency high
			UI_Buzzer_PWM(800);
			RGB_0.bright = 00;
			RGB_0.red = 0;
			RGB_0.green = 0;
			RGB_0.blue = 0;
			RGB_1.bright = 10;
			RGB_1.red = 250;
			RGB_1.green = 0;
			RGB_1.blue = 0;
		}
	}
	else
	{
		//Buzzer off
		UI_Buzzer_power(0);
		//output off
		UI_GPIO_set(OUT_EN, 0);
		RGB_0.bright = 00;
		RGB_0.red = 0;
		RGB_0.green = 0;
		RGB_0.blue = 0;
		RGB_1.bright = 0;
		RGB_1.red = 0;
		RGB_1.green = 0;
		RGB_1.blue = 0;
		if(output_val)
		{
			//output on
			UI_GPIO_set(OUT_EN, 1);
			//LEDs to purple
			RGB_0.bright = 2;
			RGB_0.red = 50;
			RGB_0.green = 0;
			RGB_0.blue = 200; 
			RGB_1.bright = 2;
			RGB_1.red = 50;
			RGB_1.green = 0;
			RGB_1.blue = 200; 
		}
	}
	//update RGB LEDs
	UI_set_RGB(0, RGB_0.bright, RGB_0.red, RGB_0.green, RGB_0.blue);
	UI_set_RGB(1, RGB_1.bright, RGB_1.red, RGB_1.green, RGB_1.blue);
	//update Display
	UI_Update();
	//write last state for detecting change
	ENC_count_last = ENC_count;
	//write last state for detecting change
	page_select_last = page_select;
	//if page is the same, update button values
	if(page_select == page_select_last)
	{
		up_press = UI_get_press(up); 
		down_press = UI_get_press(down);
		left_press = UI_get_press(left);
		right_press = UI_get_press(right);
		select_press = UI_get_press(sel);
		ENC_count = UI_get_ENC();
	}
	//if state changed, reset all Buttons
	if(page_select != page_select_last)
	{
		UI_reset_all_states();
		up_press = 0;
		down_press = 0;
		left_press = 0;
		right_press = 0;
		select_press = 0;
		value_select = 0;
		ENC_count = 0;
		ENC_count_last = 0;
	}
	//Display free Heap size
	ESP_LOGI(__FUNCTION__, "Free Heap size: %d\n", xPortGetFreeHeapSize());
	
	vTaskDelay(3 / portTICK_PERIOD_MS);
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
