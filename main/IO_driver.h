#ifndef MAIN_IO_Driver_H_
#define MAIN_IO_Driver_H_

#include "driver/gpio.h"

#define LED_0   0
#define LED_1   1
#define BUZZER  2

void IO_handler(void *pvParameters);
void IO_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO);
void IO_exp_write_reg_1(uint8_t write_value);
uint8_t IO_exp_read_reg_0();
void IO_GPIO_set(uint8_t GPIO_Num, bool GPIO_state);

#endif