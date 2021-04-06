#include "stdio.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "expander_driver.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "IO_Driver";

expander_t dev_port_expander;
conf_t config = Default_Config;

#define GPIO_OUTPUT_IO_0    2
#define GPIO_OUTPUT_IO_1    26
#define GPIO_OUTPUT_IO_Buzzer   4
#define GPIO_INPUT_IO_DT    16
#define GPIO_INPUT_IO_CLK   15
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1) | (1ULL<<GPIO_OUTPUT_IO_Buzzer))

//init GPIO config object
gpio_config_t io_conf;

//create Mutex
SemaphoreHandle_t xIO_Semaphore;

//Init Variables
uint8_t reg_0_val = 0;
uint8_t return_val = 0;
uint8_t reg_1_val = 0;
bool GPIO_0_state = 0;
bool GPIO_1_state = 0;
bool GPIO_Buzzer_state = 0;
int GPIO_DT_state = 0;
int GPIO_CLK_state = 0;

//Task
void IO_handler(void *pvParameters)
{
	while(1)
	{
		if( xIO_Semaphore != NULL )
		{
			if( xSemaphoreTake( xIO_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
		    {
    			read_reg_8(&dev_port_expander, reg_in_port_0, &reg_0_val);
				write_reg_8(&dev_port_expander, reg_out_port_1, reg_1_val);
				gpio_set_level(GPIO_OUTPUT_IO_0, GPIO_0_state);
				gpio_set_level(GPIO_OUTPUT_IO_1, GPIO_1_state);
				gpio_set_level(GPIO_OUTPUT_IO_Buzzer, GPIO_Buzzer_state);
				GPIO_DT_state = gpio_get_level(GPIO_INPUT_IO_DT);
				GPIO_CLK_state = gpio_get_level(GPIO_INPUT_IO_CLK);
				xSemaphoreGive( xIO_Semaphore );
			}
			else
			{
				ESP_LOGE(TAG, "Could not take Semaphore");
			}
		}
		vTaskDelay(3 / portTICK_PERIOD_MS);	
	}
}

void IO_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO)
{	
	//Set memory containing dev_port_expander to 0
    memset(&dev_port_expander, 0, sizeof(expander_t));
	//change Expander config
	config.conf_port_0 = 0xFF;
	config.conf_port_1 = 0x00;
	config.pol_inv_0 = 0xFF;
	config.pol_inv_1 = 0x00;
	//Init and configure Expander
    expander_init_desc(&dev_port_expander, expander_addr_low, I2C_PORT, SDA_GPIO, SCL_GPIO);
	expander_configure(&dev_port_expander, &config);
	//change GPIO Config
	xIO_Semaphore = xSemaphoreCreateMutex();
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	//Init and configure GPIO
	gpio_config(&io_conf);
	//Create main Task
	xTaskCreate(IO_handler, "IO_handler", 1024*4, NULL, 2, NULL);
	ESP_LOGI(TAG, "--> IO_driver initialized successfully");
}

void IO_exp_write_reg_1(uint8_t write_value)
{
	if( xIO_Semaphore != NULL )
	{
		if( xSemaphoreTake( xIO_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			reg_1_val = write_value;
			xSemaphoreGive( xIO_Semaphore );
			//ESP_LOGI(TAG, "reg_1 set to 0x%x", reg_1_val);
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
}

uint8_t IO_exp_read_reg_0()
{
	if( xIO_Semaphore != NULL )
	{
		if( xSemaphoreTake( xIO_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
        {
			return_val = reg_0_val;
			xSemaphoreGive( xIO_Semaphore );
			//ESP_LOGI(TAG, "reg_0 read as 0x%x", return_val);
			return return_val;
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
	return 0;
}

void IO_GPIO_set(uint8_t GPIO_Num, bool GPIO_state)
{
	if( xIO_Semaphore != NULL )
	{
		if( xSemaphoreTake( xIO_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			//set GPIO with the specified Number
			if(GPIO_Num == 0) GPIO_0_state = GPIO_state;
			if(GPIO_Num == 1) GPIO_1_state = GPIO_state;
			if(GPIO_Num == 2) GPIO_Buzzer_state = GPIO_state;
			else if(GPIO_Num > 2) ESP_LOGE(TAG, "GPIO_set: GPIO_Num ERROR");	
			xSemaphoreGive( xIO_Semaphore );
			//ESP_LOGI(TAG, "reg_1 set to 0x%x", reg_1_val);
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
}

int IO_GPIO_get(uint8_t GPIO_Num)
{
	int GPIO_state = 0;
	if( xIO_Semaphore != NULL )
	{
		if( xSemaphoreTake( xIO_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
			//set GPIO with the specified Number
			if(GPIO_Num == 3) GPIO_state = GPIO_DT_state;
			if(GPIO_Num == 4) GPIO_state = GPIO_CLK_state;
			else if(GPIO_Num > 4 || GPIO_Num < 3) ESP_LOGE(TAG, "GPIO_get: GPIO_Num ERROR");	
			xSemaphoreGive( xIO_Semaphore );
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
	return GPIO_state;
}