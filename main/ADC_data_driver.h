#ifndef MAIN_ADC_DATA_DRIVER_H_
#define MAIN_ADC_DATA_DRIVER_H_

#include "ADC_driver.h"


void ADCD_handler(void *pvParameters);
void ADCD_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO);
uint16_t ADCD_get(int ADC_num);
void ADCD_write_value_8(uint8_t reg, uint8_t value);
void ADCD_write_value_16(uint8_t reg, uint16_t value);
#endif