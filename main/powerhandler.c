#include "powerhandler.h"
#include "INA220.h"


#define TAG "POWER"

#define INA220_SDA_GPIO     21
#define INA220_SCL_GPIO     22

#define MEASURINGPERIOD     10 //ms

SemaphoreHandle_t powerdataLock;

double voltage = 0;
double current = 0;
double power = 0;
double wattHours = 0;

void vPowerHandler(void* pvParameter) {
    powerdataLock = xSemaphoreCreateMutex();
    xSemaphoreTake(powerdataLock, portMAX_DELAY);
#ifndef AQCOOL_DEBUG
    ina220_params_t params;
    params.shunt_resolution = RESOLUTION_32Samples;
    params.bus_resolution = RESOLUTION_32Samples;
    ina220_init_default_params(&params);
    ina220_t dev;
    memset(&dev, 0, sizeof(ina220_t));

    ESP_ERROR_CHECK(ina220_init_desc(&dev, INA220_I2C_ADDR, 0, INA220_SDA_GPIO, INA220_SCL_GPIO));
    ESP_ERROR_CHECK(ina220_init(&dev, &params));

    ESP_ERROR_CHECK(ina220_setCalibrationData(&dev, &params, 11.0, 0.002));

    ESP_LOGI(TAG, "Init Done...");
#endif
    xSemaphoreGive(powerdataLock);
    while(1) {
#ifndef AQCOOL_DEBUG
        while(ina220_newDataAvailable(&dev, &params) != true) {
            vTaskDelay(1/portTICK_PERIOD_MS);
        }
#endif
        xSemaphoreTake(powerdataLock, portMAX_DELAY);
#ifndef AQCOOL_DEBUG
        voltage = ina220_getVBus_mv(&dev, &params);
        current = ina220_getCurrent_mA(&dev, &params);
        power = ina220_getPower_mW(&dev, &params);
        wattHours += ((power/1000.0) / (double)(3600*1000/MEASURINGPERIOD));
#endif
        aqCoolLiveData_t* liveData = aquireLiveData();
        liveData->powerdata.voltage_V = voltage/1000;
        liveData->powerdata.current_mA = current;
        liveData->powerdata.power_mW = power;
        liveData->powerdata.energy_Wh = wattHours;
        freeLiveData();
        xSemaphoreGive(powerdataLock);
        //ESP_LOGI(TAG, "U=%.2fV - I=%.1fmA - P=%.1fmW", voltage/1000.0, current, power);
        vTaskDelay(15/portTICK_PERIOD_MS);
    }
}

void initPowerHandler(aqCoolLiveData_t* data) {
    ESP_ERROR_CHECK(i2cdev_init());
    xTaskCreate(vPowerHandler, "powerhandler", 2048+1000, NULL, 5, NULL);
}

double getPowerHandlerVoltage_mV() {
    double returnValue = 0;
    xSemaphoreTake(powerdataLock, portMAX_DELAY);
    returnValue = voltage;
    xSemaphoreGive(powerdataLock);
    return returnValue;
}
double getPowerHandlerCurrent_mA() {
    double returnValue = 0;
    xSemaphoreTake(powerdataLock, portMAX_DELAY);
    returnValue = current;
    xSemaphoreGive(powerdataLock);
    return returnValue;
}
double getPowerHandlerPower_mW() {
    double returnValue = 0;
    xSemaphoreTake(powerdataLock, portMAX_DELAY);
    returnValue = power;
    xSemaphoreGive(powerdataLock);
    return returnValue;
}
double getPowerHandlerWattHours() {
    double returnValue = 0;
    xSemaphoreTake(powerdataLock, portMAX_DELAY);
    returnValue = wattHours;
    xSemaphoreGive(powerdataLock);
    return returnValue;
}
