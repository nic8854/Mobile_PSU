#include "stdio.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "IO_driver.h"

static const char *TAG = "Button_driver";

SemaphoreHandle_t xBTSemaphore;

uint8_t reg_read = 0;
uint8_t return_value = 0;
uint8_t reg_write = 0;
int DT_state = 0;
int CLK_state = 0;
int CLK_state_last = 0;
int ENC_counter = 0;

//Task
void Button_handler(void *pvParameters)
{
	while(1)
	{
		//If semaphore is initialized
		if( xBTSemaphore != NULL )
		{
			//If able, take semaphore, otherwise try again for 10 Ticks
			if( xSemaphoreTake( xBTSemaphore, ( TickType_t ) 10 ) == pdTRUE )
		    {
				//Read Reg 0 and write Reg 1
				IO_exp_write_reg_1(reg_write);
				reg_read = IO_exp_read_reg_0();

				//get states of Encoder Pins
				DT_state = IO_GPIO_get(ENC_DT);
				CLK_state = IO_GPIO_get(ENC_CLK);

				xSemaphoreGive( xBTSemaphore );

				if(CLK_state != CLK_state_last)
				{
					if(!CLK_state && !DT_state) ENC_counter--;
					if(!CLK_state && DT_state) ENC_counter++;
					if(CLK_state && !DT_state) ENC_counter++;
					if(CLK_state && DT_state) ENC_counter--;
				}
				CLK_state_last = CLK_state;
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
	//Create Mutex
	xBTSemaphore = xSemaphoreCreateMutex();
	//Initialize IO_Driver(Expander, GPIO, Buzzer etc.)
	IO_init(I2C_PORT, SDA_GPIO, SCL_GPIO);
	//Create Main Task
	xTaskCreate(Button_handler, "Button_handler", 1024*4, NULL, 2, NULL);
	ESP_LOGI(TAG, "--> Button_driver initialized successfully");
	
}

void Button_write_reg_1(uint8_t write_value)
{
	//If semaphore is initialized
	if( xBTSemaphore != NULL )
	{
		//If able, take semaphore, otherwise try again for 10 Ticks
		if( xSemaphoreTake( xBTSemaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			//set Reg1 to Parameter value
			reg_write = write_value;
			xSemaphoreGive( xBTSemaphore );
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
}

uint8_t Button_read_reg_0()
{
	//If semaphore is initialized
	if( xBTSemaphore != NULL )
	{
		//If able, take semaphore, otherwise try again for 10 Ticks
		if( xSemaphoreTake( xBTSemaphore, ( TickType_t ) 10 ) == pdTRUE )
        {
			//return value from Reg0
			return_value = reg_read;
			xSemaphoreGive( xBTSemaphore );
			return return_value;
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
	return 0;
}

int Button_ENC_get()
{
	return ENC_counter;
}

void Button_ENC_reset()
{
	ENC_counter = 0;
}

void Button_Buzzer_PWM(int freq)
{
	IO_Buzzer_PWM(freq);
}

void Button_Buzzer_power(bool power)
{
	IO_Buzzer_power(power);
}