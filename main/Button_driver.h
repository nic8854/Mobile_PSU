#ifndef MAIN_Button_Driver_H_
#define MAIN_Button_Driver_H_

#include "IO_driver.h"

#define btn_up      0
#define btn_down    1
#define btn_left    2
#define btn_right   3
#define btn_sel     4

typedef struct {
    bool state[5];
    bool state_last[5];
    int count[5];
    int press[5];
} button_states;

void Button_handler(void *pvParameters);
void Button_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO);
void Button_write_reg_1(uint8_t write_value);
uint8_t Button_read_reg_0();
int Button_ENC_get();
void Button_ENC_set(int value);
void Button_set_states();
void Button_set_press();
int Button_get_press(int button_select);

#endif