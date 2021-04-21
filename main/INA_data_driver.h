#ifndef MAIN_INA_DATA_DRIVER_H_
#define MAIN_INA_DATA_DRIVER_H_

#include "INA220.h"

#define INA1 1
//#define INA2 2

typedef struct
{
    int32_t INA1_S_val;
    int32_t INA1_A_val;
    int32_t INA2_S_val;
    int32_t INA2_A_val;
} INA_cal_t;


void INAD_handler(void *pvParameters);
void INAD_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO, INA_cal_t INA_cal);
double INAD_getVShunt_mv(int INA);
double INAD_getVBus_mv(int INA);
double INAD_getPower_mW(int INA);
double INAD_getCurrent_mA(int INA);
#endif