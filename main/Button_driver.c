#include "stdio.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"
#include "expander_driver.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "Button_driver";

#define GPIO_OUTPUT_IO_0    2
#define GPIO_OUTPUT_IO_1    26
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))

//temporary
expander_t dev_port_expander;
conf_t config = Default_Config;
gpio_config_t io_conf;

SemaphoreHandle_t xSemaphore;

uint8_t reg_0_val = 0;
uint8_t return_value = 0;
uint8_t reg_1_val = 0;

void Button_handler(void *pvParameters)
{
	while(1)
	{
		if( xSemaphore != NULL )
		{
			if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
		    {
				//temporary
    			read_reg_8(&dev_port_expander, reg_in_port_0, &reg_0_val);
				write_reg_8(&dev_port_expander, reg_out_port_1, reg_1_val);
				xSemaphoreGive( xSemaphore );
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
	//temporary
    memset(&dev_port_expander, 0, sizeof(expander_t));
	config.conf_port_0 = 0xFF;
	config.conf_port_1 = 0x00;
	config.pol_inv_0 = 0xFF;
	config.pol_inv_1 = 0x00;
    expander_init_desc(&dev_port_expander, expander_addr_low, I2C_PORT, SDA_GPIO, SCL_GPIO);
	expander_configure(&dev_port_expander, &config);
	xSemaphore = xSemaphoreCreateMutex();
	xTaskCreate(Button_handler, "Button_handler", 1024*4, NULL, 2, NULL);
}



void Button_write_reg_1(uint8_t write_value)
{
	if( xSemaphore != NULL )
	{
		if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			reg_1_val = write_value;
			xSemaphoreGive( xSemaphore );
			ESP_LOGI(TAG, "reg_1 set to 0x%x", reg_1_val);
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
}

uint8_t Button_read_reg_0()
{
	if( xSemaphore != NULL )
	{
		if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
        {
			return_value = reg_0_val;
			xSemaphoreGive( xSemaphore );
			ESP_LOGI(TAG, "reg_0 read as 0x%x", return_value);
			return return_value;
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
	return 0;
}