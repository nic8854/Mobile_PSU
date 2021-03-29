#include "stdio.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "math.h"
#include "INA220.h"
#include "esp_log.h"

static const char *TAG = "INAD_Data_Driver";

//select INA1, INA2 or both

#define INA1 1
//#define INA2 2

//initialize Mutex Handle
SemaphoreHandle_t xINAD_Semaphore;

//Vars INA1
#ifdef INA1
    #define I2C_INA1_ADDR 0x40
    ina220_t INA1_dev;
    ina220_params_t INA1_params;
    double INA1_s_val = 0;
    double INA1_b_val = 0;
    double INA1_p_val = 0;
    double INA1_i_val = 0;
    double INA1_i_max = 0.1;
    double INA1_s_cal = 1.4;
#endif

//Vars INA2
#ifdef INA2
    #define I2C_INA2_ADDR 0x41
    ina220_t INA2_dev;
    ina220_params_t INA2_params;
    double INA2_s_val = 0;
    double INA2_b_val = 0;
    double INA2_p_val = 0;
    double INA2_i_val = 0;
    double INA2_i_max = 0.1;
    double INA2_s_cal = 1.4;
#endif

double INAD_return = 0;

void INAD_handler(void *pvParameters)
{
	while(1)
	{
		//If semaphore is initialized
		if( xINAD_Semaphore != NULL )
		{	
			//If able, take semaphore, otherwise try again for 10 Ticks
			if( xSemaphoreTake( xINAD_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
		    {
#ifdef INA1
				//get INA1 values
                INA1_s_val = ina220_getVShunt_mv(&INA1_dev, &INA1_params);
                INA1_b_val = ina220_getVBus_mv(&INA1_dev, &INA1_params);
                INA1_p_val = ina220_getPower_mW(&INA1_dev, &INA1_params);
                INA1_i_val = ina220_getCurrent_mA(&INA1_dev, &INA1_params);
#endif

#ifdef INA2
				//get INA1 values
                INA2_s_val = ina220_getVShunt_mv(&INA2_dev, &INA2_params);
                INA2_b_val = ina220_getVBus_mv(&INA2_dev, &INA2_params);
                INA2_p_val = ina220_getPower_mW(&INA2_dev, &INA2_params);
                INA2_i_val = ina220_getCurrent_mA(&INA2_dev, &INA2_params);
#endif			
				//Give Semaphore
				xSemaphoreGive( xINAD_Semaphore );
			}
			else
			{
				ESP_LOGE(TAG, "Could not take Semaphore");
			}
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);	
	}
}

void INAD_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO)
{
	//INA1 Init
#ifdef INA1
    ina220_init_default_params(&INA1_params);
    memset(&INA1_dev, 0, sizeof(ina220_t));
    ina220_init_desc(&INA1_dev, I2C_INA1_ADDR, I2C_PORT, SDA_GPIO, SCL_GPIO);
	ina220_init(&INA1_dev, &INA1_params);
	ina220_setCalibrationData(&INA1_dev, &INA1_params, INA1_i_max, INA1_s_cal);
#endif
	//INA2 Init
#ifdef INA2
    ina220_init_default_params(&INA2_params);
    memset(&INA2_dev, 0, sizeof(ina220_t));
    ina220_init_desc(&INA2_dev, I2C_INA2_ADDR, I2C_PORT, SDA_GPIO, SCL_GPIO);
	ina220_init(&INA2_dev, &INA2_params);
	ina220_setCalibrationData(&INA2_dev, &INA2_params, INA2_i_max, INA2_s_cal);
#endif
	//Create Mutex
	xINAD_Semaphore = xSemaphoreCreateMutex();
	//Create Handler Task
	xTaskCreate(INAD_handler, "INAD_handler", 1024*4, NULL, 2, NULL);
	ESP_LOGI(TAG, "--> INA220_data_driver initialized successfully");
}

double INAD_getVShunt_mv(int INA)
{
	if( xINAD_Semaphore != NULL )
	{
		if( xSemaphoreTake( xINAD_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
#ifdef INA1
            if(INA == INA1) INAD_return = INA1_s_val;
#endif
#ifdef INA2
            if(INA == INA2) INAD_return = INA2_s_val;
#endif
			xSemaphoreGive( xINAD_Semaphore );
            return INAD_return;
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
    return 0;
}

double INAD_getVBus_mv(int INA)
{
	//If semaphore is initialized
	if( xINAD_Semaphore != NULL )
	{
		//If able, take semaphore, otherwise try again for 10 Ticks
		if( xSemaphoreTake( xINAD_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
#ifdef INA1
            if(INA == INA1) INAD_return = INA1_b_val;
#endif
#ifdef INA2
            if(INA == INA2) INAD_return = INA2_b_val;
#endif
			xSemaphoreGive( xINAD_Semaphore );
            return INAD_return;
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
    return 0;
}

double INAD_getPower_mW(int INA)
{
	//If semaphore is initialized
	if( xINAD_Semaphore != NULL )
	{
		//If able, take semaphore, otherwise try again for 10 Ticks
		if( xSemaphoreTake( xINAD_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
#ifdef INA1
			//if INA1 selected, return INA1 Value
            if(INA == INA1) INAD_return = INA1_p_val;
#endif
#ifdef INA2
			//if INA2 selected, return INA1 Value
            if(INA == INA2) INAD_return = INA2_p_val;
#endif
			xSemaphoreGive( xINAD_Semaphore );
            return INAD_return;
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
		}
	}
    return 0;
}

double INAD_getCurrent_mA(int INA)
{
	if( xINAD_Semaphore != NULL )
	{
		if( xSemaphoreTake( xINAD_Semaphore, ( TickType_t ) 10 ) == pdTRUE )
	    {
#ifdef INA1
            if(INA == INA1) INAD_return = INA1_i_val;
#endif
#ifdef INA2
            if(INA == INA2) INAD_return = INA2_i_val;
#endif
			xSemaphoreGive( xINAD_Semaphore );
            return INAD_return;
		}
		else
		{
			ESP_LOGE(TAG, "Could not take Semaphore");
            
		}
	}
    return 0;
}