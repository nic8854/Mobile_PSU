#include "stdio.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "expander_driver.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/ledc.h"

static const char *TAG = "IO_Driver";

expander_t dev_port_expander;
conf_t config = Default_Config;

//define for GPIO Pins
#define GPIO_OUTPUT_IO_0    2
#define GPIO_OUTPUT_IO_1    26
#define GPIO_OUTPUT_IO_Buzzer   4
#define GPIO_INPUT_IO_DT    16
#define GPIO_INPUT_IO_CLK   15
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))

//Timer for PWM defines
#define LEDC_LS_TIMER          LEDC_TIMER_0
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_LS_CH0_GPIO       GPIO_OUTPUT_IO_Buzzer
#define LEDC_LS_CH0_CHANNEL    LEDC_CHANNEL_0

//init GPIO config object
gpio_config_t io_conf;

//PWM timer and channel Objects
ledc_timer_config_t ledc_timer;
ledc_channel_config_t ledc_channel;

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

/**
 * Main Task in IO_driver Library. Handles GPIO and Expander Input, Outputs and PWM for Buzzer.
 * @param pvParameters usused
 * @endcode
 * \ingroup UI_draw
 */
void IO_handler(void *pvParameters)
{
	while(1)
	{
		if( xIO_Semaphore != NULL )
		{
			if( xSemaphoreTake( xIO_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
		    {
				//Read Register 0 and Write Register 1 (Expander)
    			read_reg_8(&dev_port_expander, reg_in_port_0, &reg_0_val);
				write_reg_8(&dev_port_expander, reg_out_port_1, reg_1_val);
				//Set Level of NFON and TC_EN
				gpio_set_level(GPIO_OUTPUT_IO_0, GPIO_0_state);
				gpio_set_level(GPIO_OUTPUT_IO_1, GPIO_1_state);
				//get State of Encoder Pins
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

/**
 * Initialization function for GPIOs, Buzzer and Expander(Expander handled in seperate Function).
 * 
 * @param I2C_PORT sets I2c_port
 * @param SDA_GPIO sets SDA GPIO
 * @param SCL_GPIO sets SCL GPIO
 *  
 * @endcode
 * \ingroup UI_draw
 */
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
	//change GPIO Config Object
	xIO_Semaphore = xSemaphoreCreateMutex();
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	//Init and configure GPIO
	gpio_config(&io_conf);

	//Init Timer for PWM
    ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; // resolution of PWM duty
    ledc_timer.freq_hz = 500;                      // frequency of PWM signal
    ledc_timer.speed_mode = LEDC_LS_MODE;           // timer mode
    ledc_timer.timer_num = LEDC_LS_TIMER;            // timer index
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;              // Auto select the source clock

    ledc_timer_config(&ledc_timer);

	//Init PWM Channel for Buzzer
    ledc_channel.channel    = LEDC_LS_CH0_CHANNEL;
    ledc_channel.duty       = 0;
    ledc_channel.gpio_num   = LEDC_LS_CH0_GPIO;
    ledc_channel.speed_mode = LEDC_LS_MODE;
    ledc_channel.hpoint     = 0;
    ledc_channel.timer_sel  = LEDC_LS_TIMER;
	//write COnfig
	ledc_channel_config(&ledc_channel);

	//Create main Task
	xTaskCreate(IO_handler, "IO_handler", 1024*4, NULL, 2, NULL);
	ESP_LOGI(TAG, "--> IO_driver initialized successfully");
}

/**
 * Sets levels of Reg 1.
 * @param write_value Bit Pattern to set Reg 1 to.
 * @return returns state of GPIO as an int
 * @endcode
 */
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

/**
 * Gets levels of reg 0
 * @return returns states of reg 0 as a bit pattern
 * @endcode
 */
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

/**
 * Sets level of specified GPIO Pin.
 * @param GPIO_Num selects GPIO from a List of defines(see header)
 * @param GPIO_state selects what state the GPIO should change to (HIGH or LOW)
 * @endcode
 */
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

/**
 * Linking Function to IO driver
 * Gets level of specified GPIO Pin.
 * @param GPIO_Num selects GPIO from a List of defines(see header)
 * @return returns state of GPIO as an int
 * @endcode
 */
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

/**
 * Sets Buzzer PWM to specified frequency
 * @param freq set frequency. Needs to be within 100 - 10000Hz.
 * @endcode
 */
void IO_Buzzer_PWM(int freq)
{
	//check if Frequency is within Range
	if(freq > 90 && freq < 10000) {
		//set Frequency
		ledc_set_freq(ledc_channel.speed_mode, ledc_timer.timer_num, freq);
	}
}

/**
 * Sets Buzzer PWM to on or off
 * @param power sets power state. 1 = on, 0 = off.
 * @endcode
 */
void IO_Buzzer_power(bool power)
{
	if(power)
	{
		//set the Duty cycle of the PWM to 100 (ON state)
		ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 100);
		ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
	}
	else
	{
		//set the Duty cycle of the PWM to 0 (OFF state)
		ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
		ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
	}
	
		
}