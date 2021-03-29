#ifndef MAIN_UI_DRIVER_H_
#define MAIN_UI_DRIVER_H_

#include "driver/spi_master.h"
#include "fontx.h"
#include "ili9340.h"
#include "pngle.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 
  
void UI_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO);
void UI_draw_test_screen(uint8_t in_value, uint8_t out_value, double current_val, double shunt_val);
void UI_Update();
void UI_GPIO_set(uint8_t GPIO_Num, bool GPIO_state);
void UI_exp_write_reg_1(uint8_t write_value);
uint8_t UI_exp_read_reg_0();
void led_test(bool mode);

#endif