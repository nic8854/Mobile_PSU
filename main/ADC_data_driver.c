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

static const char *TAG = "ADCD_Data_Driver";

#define ADC1_config 0x0010
#define ADC2_config 0x0020
#define ADC3_config 0x0040
#define ADC4_config 0x0080
#define ADC5_config 0x0100


//initialize Mutex Handle
SemaphoreHandle_t xADCD_Semaphore;
//Initialize ADC I2C Object
AD_t ADC_dev;

//Variables to save values to
uint16_t ADC1_read = 0x0000;
uint16_t ADC2_read = 0x0000;
uint16_t ADC3_read = 0x0000;
uint16_t ADC4_read = 0x0000;
uint16_t ADC5_read = 0x0000;

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
			vTaskDelay(50 / portTICK_PERIOD_MS);
		}
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
void ADCD_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO)
{
	
	//ADC Init
	memset(&ADC_dev, 0, sizeof(AD_t));
	AD_init_desc(&ADC_dev, AD_addr_low, I2C_PORT, SDA_GPIO, SCL_GPIO);
	//Create Mutex
	xADCD_Semaphore = xSemaphoreCreateMutex();



	//try 5 times to set the config
	ADCD_write_value_16(reg_config, default_config);
	//try 5 times to set the interval
	ADCD_write_value_8(reg_cycle_timer, default_interval);

	//Create Handler Task
	xTaskCreate(ADCD_handler, "ADCD_handler", 1024*4, NULL, 2, NULL);
	ESP_LOGI(TAG, "--> INA220_data_driver initialized successfully");
}

/**
 * Function used to get values from the 5 ADCs. 
 * 
 * This is a 10 Bit ADC, so values range from 0 to 4092, 0V being equal to 0 and 1.2V being equal to 4092. 
 * The resistive voltage divider used in the schematic of this project, divides the voltages to 1V, which is equal to 3410.
 *
 * ADC must be initialized to use this function. 
 * 
 * @param ADC_num value used to define which ADC should be used (1-5)
 * @return returns ADC value as a uint16_t (0-4092)
 * \ingroup ADCD
 * @endcode
 */
uint16_t ADCD_get(int ADC_num)
{
	
	//If semaphore is initialized
	if( xADCD_Semaphore != NULL )
	{
		//If able, take semaphore, otherwise try again for 10 Ticks
		if( xSemaphoreTake( xADCD_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			uint16_t ADCD_return = 0;
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
 * Writes Could not write to 0x... if it is uable to write
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
 * Writes Could not write to 0x... if it is uable to write
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