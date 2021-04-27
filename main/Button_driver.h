#ifndef MAIN_Button_Driver_H_
#define MAIN_Button_Driver_H_

#include "IO_driver.h"

//defines for which array element is which button
#define btn_up      0
#define btn_down    1
#define btn_left    2
#define btn_right   3
#define btn_sel     4

//struct for all button variables
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
void Button_set_ENC_from_GPIO();
int Button_get_ENC();
void Button_set_ENC(int value);
void Button_set_states();
void Button_set_press();
int Button_get_press(int button_select);
void Button_reset_all_states();

#endif