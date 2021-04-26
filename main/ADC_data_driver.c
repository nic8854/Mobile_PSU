#include "stdio.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "math.h"
#include "ADC_driver.h"
#include "esp_log.h"
#include "ADC_data_driver.h"
#include "stack_usage_queue_handler.h"

static const char *TAG = "ADCD_Data_Driver";

//ADC value at 1V
#define ADC_cal_factor 3410

#define ADC1_config 0x0010
#define ADC2_config 0x0020
#define ADC3_config 0x0040
#define ADC4_config 0x0080
#define ADC5_config 0x0100


//initialize Mutex Handle
SemaphoreHandle_t xADCD_Semaphore;
//Initialize ADC I2C Object
AD_t ADC_dev;

//Initialize Object for stack usage queue
stack_usage_dataframe_t stack_ADC;

//Initialize Task handle
TaskHandle_t ADC_task;

//Variables to save values to
uint16_t ADC1_read = 0x0000;
uint16_t ADC2_read = 0x0000;
uint16_t ADC3_read = 0x0000;
uint16_t ADC4_read = 0x0000;
uint16_t ADC5_read = 0x0000;

double out24_calibrate = 0;
double out5_calibrate = 0;
double out33_calibrate = 0;
double outvar_calibrate = 0;

double out24_value = 0;
double out5_value = 0;
double out33_value = 0;
double outvar_value = 0;

/**
 * Main task of ADC data driver.
 * Handles the gathering of information over the 5 ADCs
 *
 * ADC must be initialized to use this function. 
 * 
 * @param pvParameters unused

 *  
 * @endcode
 * \ingroup UI_draw
 */
void ADCD_handler(void *pvParameters)
{
	while(1)
	{
		//If semaphore is initialized
		if( xADCD_Semaphore != NULL )
		{	
			//If able, take semaphore, otherwise try again for 10 Ticks
			if( xSemaphoreTake( xADCD_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
		    {
				ADCD_write_value_16(reg_config, ADC1_config);
				AD_read_reg_16(&ADC_dev, reg_convert, &ADC1_read);
				ADCD_write_value_16(reg_config, ADC2_config);
				AD_read_reg_16(&ADC_dev, reg_convert, &ADC2_read);
				ADCD_write_value_16(reg_config, ADC3_config);
				AD_read_reg_16(&ADC_dev, reg_convert, &ADC3_read);
				ADCD_write_value_16(reg_config, ADC4_config);
				AD_read_reg_16(&ADC_dev, reg_convert, &ADC4_read);
				ADCD_write_value_16(reg_config, ADC5_config);
				AD_read_reg_16(&ADC_dev, reg_convert, &ADC5_read);

				//Give Semaphore
				xSemaphoreGive( xADCD_Semaphore );
			}
			else
			{
				ESP_LOGE(TAG, "Could not take Semaphore");
			}
			out24_value = (double)ADC1_read * out24_calibrate / ADC_cal_factor;
			out5_value = (double)(ADC2_read - 0x1000) * out5_calibrate / ADC_cal_factor;
			out33_value = (double)(ADC3_read - 0x2000) * out33_calibrate / ADC_cal_factor;
			outvar_value = (double)(ADC4_read - 0x3000) * outvar_calibrate / ADC_cal_factor;
			
		}
		//send free stack of task to queue
		stack_ADC.size = uxTaskGetStackHighWaterMark(ADC_task);
		if(stack_usage_queue)
		{
			xQueueSendToBack(stack_usage_queue, &stack_ADC, 0);
		}
		vTaskDelay(50 / portTICK_PERIOD_MS);
	}
}

/**
 * Function to initalize ADC I2C Ojbect, set default config values and make a Task. 
 *
 * I2C dev must be initialized to use this function. 
 * 
 * @param I2C_PORT sets I2c_port for ADC init
 * @param SDA_GPIO sets SDA GPIO for ADC init
 * @param SCL_GPIO sets SCL GPIO for ADC init
 *  
 * @endcode
 * \ingroup ADCD
 */
void ADCD_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO, ADC_cal_t ADC_cal)
{
	out24_calibrate = ((double)ADC_cal.OUT24_cal) / 1000;
	out5_calibrate = ((double)ADC_cal.OUT5_cal) / 1000;
	out33_calibrate = ((double)ADC_cal.OUT33_cal) / 1000;
	outvar_calibrate = ((double)ADC_cal.OUTvar_cal) / 1000;

	//ADC Init
	memset(&ADC_dev, 0, sizeof(AD_t));
	AD_init_desc(&ADC_dev, AD_addr_low, I2C_PORT, SDA_GPIO, SCL_GPIO);
	//Create Mutex
	xADCD_Semaphore = xSemaphoreCreateMutex();

	if(stack_usage_queue)
	{
		stack_ADC.task_num = ADC_TASK;
	}

	//try 5 times to set the config
	ADCD_write_value_16(reg_config, default_config);
	//try 5 times to set the interval
	ADCD_write_value_8(reg_cycle_timer, default_interval);

	//Create Handler Task
	xTaskCreate(ADCD_handler, "ADCD_handler", 1024*4, NULL, 2, ADC_task);
	ESP_LOGI(TAG, "--> INA220_data_driver initialized successfully");
}

/**
 * Function used to get voltages from the 5 ADCs. 
 * 
 *
 * ADC must be initialized to use this function. 
 * 
 * @param ADC_num value used to define which ADC should be used (1-5)
 * @return returns ADC voltage as a double
 * \ingroup ADCD
 * @endcode
 */
double ADCD_get_volt(int ADC_num)
{
	
	//If semaphore is initialized
	if( xADCD_Semaphore != NULL )
	{
		//If able, take semaphore, otherwise try again for 10 Ticks
		if( xSemaphoreTake( xADCD_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			double ADCD_return = 0;
			switch(ADC_num)
			{
				case 1:
					ADCD_return = out24_value;
				break;
				case 2:
					ADCD_return = out5_value;
				break;
				case 3:
					ADCD_return = out33_value;
				break;
				case 4:
					ADCD_return = outvar_value;
				break;
				case 5:
					ADCD_return = ADC5_read;
				break;
			}
			xSemaphoreGive( xADCD_Semaphore );
            return ADCD_return;
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
    return 0;
}

/**
 * Function used to get values from the 5 ADCs. 
 * 
 * Values range from 0 to 4092, 0V being equal to 0 and 1.2V being equal to 4092. 
 * The resistive voltage divider used in the schematic of this project, divides the voltages to 1V, which is equal to 3410.
 *
 * ADC must be initialized to use this function. 
 * 
 * @param ADC_num value used to define which ADC should be used (1-5)
 * @return returns ADC value as a uint16_t (0-4092)
 * \ingroup ADCD
 * @endcode
 */
int ADCD_get(int ADC_num)
{
	
	//If semaphore is initialized
	if( xADCD_Semaphore != NULL )
	{
		//If able, take semaphore, otherwise try again for 10 Ticks
		if( xSemaphoreTake( xADCD_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			double ADCD_return = 0;
			switch(ADC_num)
			{
				case 1:
					ADCD_return = ADC1_read;
				break;
				case 2:
					ADCD_return = ADC2_read;
				break;
				case 3:
					ADCD_return = ADC3_read;
				break;
				case 4:
					ADCD_return = ADC4_read;
				break;
				case 5:
					ADCD_return = ADC5_read;
				break;
			}
			xSemaphoreGive( xADCD_Semaphore );
            return ADCD_return;
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
    return 0;
}

/**
 * Internal function!!
 * Function used to try writing a 8bit value 5 times to a register.
 * Writes Could not write to 0x... if it is unable to write
 * 
 * @param reg register to write to
 * @param value value to write to register
 * 
 * \ingroup ADCD
 * @endcode
 */
void ADCD_write_value_8(uint8_t reg, uint8_t value)
{
	uint8_t ADC_read = 0x0000;
	int counter = 0;
	while(value != ADC_read && counter < 5)
	{
		AD_write_reg_8(&ADC_dev, reg, value);
		vTaskDelay(1 / portTICK_PERIOD_MS);
		AD_read_reg_8(&ADC_dev, reg, &ADC_read);
		vTaskDelay(1 / portTICK_PERIOD_MS);
		counter++;
	}
	if(value != ADC_read) ESP_LOGE(TAG, "COULD NOT WRITE to 0x%x", reg);
}

/**
 * Internal function!!
 * Function used to try writing a 16bit value 5 times to a register.
 * Writes Could not write to 0x... if it is unable to write
 * 
 * @param reg register to write to
 * @param value value to write to register
 * 
 * \ingroup ADCD
 * @endcode
 */
void ADCD_write_value_16(uint8_t reg, uint16_t value)
{
	uint16_t ADC_read = 0x0000;
	int counter = 0;
	while(value != ADC_read && counter < 3)
	{
		AD_write_reg_16(&ADC_dev, reg, value);
		vTaskDelay(1 / portTICK_PERIOD_MS);
		AD_read_reg_16(&ADC_dev, reg, &ADC_read);
		vTaskDelay(1 / portTICK_PERIOD_MS);
		counter++;
	}
	if(value != ADC_read) ESP_LOGE(TAG, "COULD NOT WRITE to 0x%x", reg);
}