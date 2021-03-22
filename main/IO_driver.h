#ifndef MAIN_IO_Driver_H_
#define MAIN_IO_Driver_H_

void IO_handler(void *pvParameters);
void IO_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO);
void IO_exp_write_reg_1(uint8_t write_value);
uint8_t IO_exp_read_reg_0();

#endif