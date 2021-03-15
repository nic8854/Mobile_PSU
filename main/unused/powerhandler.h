#ifndef POWERHANDLER_H
#define POWERHANDLER_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"

#define INA220_I2C_ADDR     0x40

void initPowerHandler(aqCoolLiveData_t* data);
double getPowerHandlerVoltage_mV();
double getPowerHandlerCurrent_mA();
double getPowerHandlerPower_mW();
double getPowerHandlerWattHours();

#endif