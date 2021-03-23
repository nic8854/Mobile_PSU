#include "stdio.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "math.h"
#include "esp_log.h"
#include "IO_driver.h"

static const char *TAG = "Button_driver";

SemaphoreHandle_t xBTSemaphore;

uint8_t reg_read = 0;
uint8_t return_value = 0;
uint8_t reg_write = 0;

void Button_handler(void *pvParameters)
{
	while(1)
	{
		if( xBTSemaphore != NULL )
		{
			if( xSemaphoreTake( xBTSemaphore, ( TickType_t ) 10 ) == pdTRUE )
		    {
				IO_exp_write_reg_1(reg_write);
				reg_read = IO_exp_read_reg_0();
				xSemaphoreGive( xBTSemaphore );
			}
			else
			{
				ESP_LOGE(TAG, "Could not take Semaphore");
			}
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);	
	}
	
}

void Button_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO)
{	
	xBTSemaphore = xSemaphoreCreateMutex();
	IO_init(I2C_PORT, SDA_GPIO, SCL_GPIO);
	xTaskCreate(Button_handler, "Button_handler", 1024*4, NULL, 2, NULL);
	
}

void Button_write_reg_1(uint8_t write_value)
{
	if( xBTSemaphore != NULL )
	{
		if( xSemaphoreTake( xBTSemaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			reg_write = write_value;
			xSemaphoreGive( xBTSemaphore );
			//ESP_LOGI(TAG, "reg_1 set to 0x%x", reg_write);
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
}

uint8_t Button_read_reg_0()
{
	if( xBTSemaphore != NULL )
	{
		if( xSemaphoreTake( xBTSemaphore, ( TickType_t ) 10 ) == pdTRUE )
        {
			return_value = reg_read;
			xSemaphoreGive( xBTSemaphore );
			//ESP_LOGI(TAG, "reg_0 read as 0x%x", return_value);
			return return_value;
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
	return 0;
}