#include "stdio.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "IO_driver.h"
#include "Button_driver.h"

static const char *TAG = "Button_driver";

#define LONG_PRESS_TIME 50; //  1 = 10ms

SemaphoreHandle_t xBTSemaphore;

//expander vars
uint8_t reg_read = 0;
uint8_t return_value = 0;
uint8_t reg_write = 0;
//button vars

button_states buttons;

//encoder vars
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

				//set button states vars to button states from reg 0 var
				Button_set_states();
				//set button press vars from count and states
				Button_set_press();

				xSemaphoreGive( xBTSemaphore );
				
				
				
				
				
				//Encoder Logic (counter)
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
	//set all values in buttons to 0
	memset( &buttons, 0, sizeof( button_states ) );
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
	int ENC_temp = 0;
	//If semaphore is initialized
	if( xBTSemaphore != NULL )
	{
		//If able, take semaphore, otherwise try again for 10 Ticks
		if( xSemaphoreTake( xBTSemaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			ENC_temp = ENC_counter;
			xSemaphoreGive( xBTSemaphore );
			return ENC_temp;
		}
		ESP_LOGE(TAG, "Could not take Semaphore");
	}
	return 0;
}

void Button_ENC_set(int value)
{
	//If semaphore is initialized
	if( xBTSemaphore != NULL )
	{
		//If able, take semaphore, otherwise try again for 10 Ticks
		if( xSemaphoreTake( xBTSemaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			ENC_counter = value;
			xSemaphoreGive( xBTSemaphore );
		}
		ESP_LOGE(TAG, "Could not take Semaphore");
	}
}

void Button_set_states()
{
	if(reg_read & 0x08) buttons.state[btn_up] = 1;
	else buttons.state[btn_up] = 0;
	if(reg_read & 0x10) buttons.state[btn_down] = 1;
	else buttons.state[btn_down] = 0;
	if(reg_read & 0x20) buttons.state[btn_left] = 1;
	else buttons.state[btn_left] = 0;
	if(reg_read & 0x40) buttons.state[btn_right] = 1;
	else buttons.state[btn_right] = 0;
	if(reg_read & 0x80) buttons.state[btn_sel] = 1;
	else buttons.state[btn_sel] = 0;
}

void Button_set_press()
{
	for(int i = 0; i < 5; i++)
	{
		if(!buttons.state[i] && buttons.state_last[i])
		{
			if(buttons.count[i] > LONG_PRESS_TIME)
			{
				buttons.press[i] = 2;
			}
			else if(buttons.press[i] == 0)
			{
				buttons.press[i] = 1;
			}
			buttons.count[i] = 0;
		}
		if(buttons.state[i])
		{
			buttons.count[i]++;
		}
		buttons.state_last[i] = buttons.state[i];
	}
}

int Button_get_press(int button_select)
{
	//If semaphore is initialized
	if( xBTSemaphore != NULL )
	{
		//If able, take semaphore, otherwise try again for 10 Ticks
		if( xSemaphoreTake( xBTSemaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			int press_temp = buttons.press[button_select];
			buttons.press[button_select] = 0;
			xSemaphoreGive( xBTSemaphore );
			return press_temp;
		}
	}
	return 0;
}

void Button_reset_all_states()
{
	//If semaphore is initialized
	if( xBTSemaphore != NULL )
	{
		//If able, take semaphore, otherwise try again for 10 Ticks
		if( xSemaphoreTake( xBTSemaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			for(int i = 0; i < 5; i++)
			{
				buttons.state[i] = 0;
    			buttons.state_last[i] = 0;
    			buttons.count[i] = 0;
    			buttons.press[i] = 0;
			}
			xSemaphoreGive( xBTSemaphore );
		}
	}
}