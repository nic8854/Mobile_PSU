#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_heap_caps.h"
#include "UI_driver.h"
#include "NVS_driver.h"
#include "Button_driver.h"
#include "INA_data_driver.h"
#include "ADC_data_driver.h"
#include "stack_usage_queue_handler.h"

//Tag for ESP_LOG functions
static const char *TAG = "Master_Task";

//define max values
#define Max_P_mW 25
#define Max_U_mV 7
#define Max_I_mA 5

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
#define calibrate_1 	0
#define calibrate_2 	1
#define main 			2
#define voltage 		3
#define variable 		4
#define statistics_p 	5
#define statistics_u 	6
#define statistics_i 	7
#define tcbus			8
#define test_1			9
#define test_2			10

//Subtask Functions
void calibrate_1_func(void);
void calibrate_2_func(void);
void main_func(void);
void voltages_func(void);
void variable_func(void);
void statistics_p_func(void);
void statistics_u_func(void);
void statistics_i_func(void);
void tcbus_func(void);
void test_func_1(void);
void test_func_2(void);
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
TaskHandle_t master_task;
int page_select = main;
int page_select_last = main;
int value_select = 0;
int ENC_count = 0;
int ENC_count_last = 0;
int up_press = 0;
int down_press = 0;
int left_press = 0;
int right_press = 0;
int select_press = 0;
bool siren_toggle = 0;


nvs_handle INA_config_NVS;
esp_err_t err;
INA_cal_t INA_cal;
ADC_cal_t ADC_cal;

stack_usage_dataframe_t stack_temp;
uint32_t stack_master_size = 0;
uint32_t stack_ADC_size = 0;
uint32_t stack_INA_size = 0;
uint32_t stack_button_size = 0;
uint32_t stack_IO_size = 0;
//INA calibration variables
double INA1_S_val = 0;
double INA1_A_val = 0;
double INA2_S_val = 0;
double INA2_A_val = 0;
//ADC calibration variables
double out24_cal = 24;
double out5_cal = 5;
double out33_cal = 3.3;
double outvar_cal = 26;
//INA value variables
double power_val = 0;
double voltage_val = 0;
double current_val = 0;
//ADC value variables
double out24_val = 0;
double out5_val = 0;
double out33_val = 0;
double outvar_val = 0;
//ADC raw values
int adc1_read = 0;
int adc2_read = 0;
int adc3_read = 0;
int adc4_read = 0;
int adc5_read = 0;
//output value variable
bool output_val = 0;
//variable out value variable
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

int receive = 0;

//main Task
void Master_Task(void *pvParameters)
{
	// Initialize NVS
    NVS_init();
	//Read calibration values from NVS
	NVS_read_values("INA1_A_val", &INA_cal.INA1_A_val);
	NVS_read_values("INA1_S_val", &INA_cal.INA1_S_val);
	NVS_read_values("INA2_A_val", &INA_cal.INA2_A_val);
	NVS_read_values("INA2_S_val", &INA_cal.INA2_S_val);

	NVS_read_values("OUT24_cal", &ADC_cal.OUT24_cal);
	NVS_read_values("OUT5_cal", &ADC_cal.OUT5_cal);
	NVS_read_values("OUT33_cal", &ADC_cal.OUT33_cal);
	NVS_read_values("OUTvar_cal", &ADC_cal.OUTvar_cal);

	//convert to doubles
	INA1_S_val = (double)INA_cal.INA1_S_val;
	INA1_A_val = ((double)INA_cal.INA1_A_val / 1000);
	INA2_S_val = (double)INA_cal.INA2_S_val;
	INA2_A_val = ((double)INA_cal.INA2_A_val / 1000);

	out24_cal = ((double)ADC_cal.OUT24_cal / 1000);
	out5_cal = ((double)ADC_cal.OUT5_cal / 1000);
	out33_cal = ((double)ADC_cal.OUT33_cal / 1000);
	outvar_cal = ((double)ADC_cal.OUTvar_cal / 1000);
	
	//initialize queue
	stack_usage_queue = xQueueCreate( 10, sizeof( stack_usage_dataframe_t ) );

	//check if queue is initialized
	if(stack_usage_queue == NULL)
	{
		ESP_LOGW(TAG, "Stack queue was not created!");
	}
	if(stack_usage_queue)
	{
		ESP_LOGI(TAG, "Stack queue was created successfully!");
	}
    
	//Init: Display, Buttons, IO and Buzzer
	UI_init(I2C_PORT, SDA_GPIO, SCL_GPIO);	

	//Init INAs
	INAD_init(I2C_PORT, SDA_GPIO, SCL_GPIO, INA_cal);

	//Init ADC
	ADCD_init(I2C_PORT, SDA_GPIO, SCL_GPIO, ADC_cal);

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
		page_select = calibrate_1;
		page_select_last = calibrate_1;
		ESP_LOGI(TAG, "Calibrate Screen entered");
	}

	//check sel press for calibrate screen
	if(UI_get_press(left) && UI_get_press(right)) 
	{
		page_select = test_1;
		page_select_last = test_1;
		ESP_LOGI(TAG, "Test Screen entered");
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
			case calibrate_1:
				calibrate_1_func();
			break;
			case calibrate_2:
				calibrate_2_func();
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
			case test_1:
				test_func_1();
			break;
			case test_2:
				test_func_2();
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

void calibrate_1_func(void)
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
				if((INA1_A_val + diff_count_temp) > -1) INA1_A_val += diff_count_temp / 10; 
			break;
			case 2: 
				if((INA2_S_val + diff_count_temp) > 0) INA2_S_val += diff_count_temp; 
			break;
			case 3:
				if((INA2_A_val + diff_count_temp) > -1) INA2_A_val += diff_count_temp / 10; 
			break;
		}
	}
	//exit page
	if(select_press > 1)
	{
		INA_cal.INA1_S_val = (int32_t)INA1_S_val;
		INA_cal.INA1_A_val = (int32_t)(INA1_A_val*1000);
		INA_cal.INA2_S_val = (int32_t)INA2_S_val;
		INA_cal.INA2_A_val = (int32_t)(INA2_A_val*1000);

		ADC_cal.OUT24_cal = (int32_t)(out24_cal * 1000);
		ADC_cal.OUT5_cal = (int32_t)(out5_cal * 1000);
		ADC_cal.OUT33_cal = (int32_t)(out33_cal * 1000);
		ADC_cal.OUTvar_cal = (int32_t)(outvar_cal * 1000);

		UI_Buzzer_beep();

		NVS_write_values("INA1_S_val", INA_cal.INA1_S_val);
		NVS_write_values("INA1_A_val", INA_cal.INA1_A_val);
		NVS_write_values("INA2_S_val", INA_cal.INA2_S_val);
		NVS_write_values("INA2_A_val", INA_cal.INA2_A_val);

		NVS_write_values("OUT24_cal", ADC_cal.OUT24_cal);
		NVS_write_values("OUT5_cal", ADC_cal.OUT5_cal);
		NVS_write_values("OUT33_cal", ADC_cal.OUT33_cal);
		NVS_write_values("OUTvar_cal", ADC_cal.OUTvar_cal);

		page_select = main;
	}

	if(left_press || right_press)
	{
		page_select = calibrate_2;
	}
	//draw Screen
	UI_draw_calibrate_screen_1(INA1_S_val, INA1_A_val, INA2_S_val, INA2_A_val, value_select);
}

void calibrate_2_func(void)
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
		double diff_count_temp = ((double)(ENC_count - ENC_count_last)) / 100;
		ESP_LOGE(TAG, "-------> %f", diff_count_temp);
		//add difference to selected value
		switch(value_select)
		{
			case 0: 
				if((out24_cal + diff_count_temp) > -1) out24_cal += diff_count_temp; 
			break;
			case 1: 
				if((out5_cal + diff_count_temp) > -1) out5_cal += diff_count_temp; 
			break;
			case 2: 
				if((out33_cal + diff_count_temp) > -1) out33_cal += diff_count_temp; 
			break;
			case 3:
				if((outvar_cal + diff_count_temp) > -1) outvar_cal += diff_count_temp; 
			break;
		}
	}
	//exit page
	if(select_press > 1)
	{
		INA_cal.INA1_S_val = (uint32_t)INA1_S_val;
		INA_cal.INA1_A_val = (uint32_t)(INA1_A_val*1000);
		INA_cal.INA2_S_val = (uint32_t)INA2_S_val;
		INA_cal.INA2_A_val = (uint32_t)(INA2_A_val*1000);

		ADC_cal.OUT24_cal = (int32_t)(out24_cal * 1000);
		ADC_cal.OUT5_cal = (int32_t)(out5_cal * 1000);
		ADC_cal.OUT33_cal = (int32_t)(out33_cal * 1000);
		ADC_cal.OUTvar_cal = (int32_t)(outvar_cal * 1000);

		UI_Buzzer_beep();
		NVS_write_values("INA1_S_val", INA_cal.INA1_S_val);
		NVS_write_values("INA1_A_val", INA_cal.INA1_A_val);
		NVS_write_values("INA2_S_val", INA_cal.INA2_S_val);
		NVS_write_values("INA2_A_val", INA_cal.INA2_A_val);

		NVS_write_values("OUT24_cal", ADC_cal.OUT24_cal);
		NVS_write_values("OUT5_cal", ADC_cal.OUT5_cal);
		NVS_write_values("OUT33_cal", ADC_cal.OUT33_cal);
		NVS_write_values("OUTvar_cal", ADC_cal.OUTvar_cal);
		page_select = main;
	}

	if(left_press || right_press)
	{
		page_select = calibrate_1;
	}
	//draw Screen
	UI_draw_calibrate_screen_2(out24_cal, out5_cal, out33_cal, outvar_cal, value_select);
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
	out24_val = ADCD_get_volt(1);
	out5_val = ADCD_get_volt(2);
	out33_val = ADCD_get_volt(3);
	outvar_val = ADCD_get_volt(4);

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
				if((uset_val + diff_count_temp) >= 0) uset_val += diff_count_temp; 
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
void test_func_1(void)
{
	adc1_read = ADCD_get(1);
	adc2_read = ADCD_get(2);
	adc3_read = ADCD_get(3);
	adc4_read = ADCD_get(4);
	adc5_read = ADCD_get(5);
	//change page +
	if(select_press > 1)
	{
		UI_Buzzer_beep();
		page_select = main;
	}
	if(right_press || left_press)
	{
		UI_Buzzer_beep();
		page_select = test_2;
	}
	//draw Screen
	UI_draw_test_screen_1(adc1_read, adc2_read, adc3_read, adc4_read, adc5_read);
}
void test_func_2(void)
{
	if(xQueueReceive(stack_usage_queue, &stack_temp, 0))
	{
		receive++;
	}

	switch(stack_temp.task_num)
	{
		break;
		case ADC_TASK:
			stack_ADC_size = stack_temp.size;
		break;
		case INA_TASK:
			stack_INA_size = stack_temp.size;
		break;
		case BUTTON_TASK:
			stack_button_size = stack_temp.size;
		break;
		case IO_TASK:
			stack_IO_size = stack_temp.size;
		break;
	}

	//change page +
	if(select_press > 1)
	{
		UI_Buzzer_beep();
		page_select = main;
	}
	if(right_press || left_press)
	{
		UI_Buzzer_beep();
		page_select = test_1;
	}
	//draw Screen
	UI_draw_test_screen_2(stack_master_size, stack_ADC_size, stack_INA_size, stack_button_size, stack_IO_size);
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
	//write last state for detecting change
	page_select_last = page_select;
	//Display free Heap size
	ESP_LOGI(__FUNCTION__, "Free Heap size: %d\n", xPortGetFreeHeapSize());
	//send free stack of task to queue
	stack_master_size = uxTaskGetStackHighWaterMark(master_task);
	
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
	xTaskCreate(Master_Task, "Master_Task", 1024*8, NULL, 2, master_task);
}
