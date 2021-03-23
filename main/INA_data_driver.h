#ifndef MAIN_INA_DATA_DRIVER_H_
#define MAIN_INA_DATA_DRIVER_H_

#include "INA220.h"

void INAD_handler(void *pvParameters);
void INAD_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO);
double INAD_getVShunt_mv(int INA);
double INAD_getVBus_mv(int INA);
double INAD_getPower_mW(int INA);
double INAD_getCurrent_mA(int INA);
#endif