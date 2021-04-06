#ifndef MAIN_Button_Driver_H_
#define MAIN_Button_Driver_H_

#include "IO_driver.h"

void Button_handler(void *pvParameters);
void Button_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO);
void Button_write_reg_1(uint8_t write_value);
uint8_t Button_read_reg_0();
int Button_ENC_get();
void Button_ENC_reset();
void Button_Buzzer_PWM(int freq);
void Button_Buzzer_power(bool power);

#endif