#ifndef MAIN_UI_DRIVER_H_
#define MAIN_UI_DRIVER_H_

#include "driver/spi_master.h"
#include "fontx.h"
#include "ili9340.h"
#include "pngle.h"

//define to convert int into binary
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
  
  typedef struct {
    int bright;
    int red;
    int green;
    int blue;
} APA102_t;

void UI_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO);
//draw screen functions
void UI_draw_main_screen(double power_val, double voltage_val, double current_val, bool output_val);
void UI_draw_voltages_screen(double out24_val, double out5_val, double outvar_val, double out33_val, bool output_val);
void UI_draw_variable_screen(double uset_val, double ueff_val, int select_val, bool output_val);
void UI_draw_statistics_screen(uint16_t p_val[100], int screen_select, int division_select, int select_val, bool output_val);
void UI_draw_calibrate_screen_1(double INA1_S, double INA1_A, double INA2_S, double INA2_A, int select_val);
void UI_draw_calibrate_screen_2(double out24, double out5, double out33, double outvar, int select_val);
void UI_draw_tcbus_screen(bool TC_EN_val, bool TC_NFON_val, bool output_val, int select_val);

//Linking Functions
void UI_Update();
void UI_GPIO_set(uint8_t GPIO_Num, bool GPIO_state);
int UI_GPIO_get(uint8_t GPIO_Num);
void UI_exp_write_reg_1(uint8_t write_value);
uint8_t UI_exp_read_reg_0();
int UI_get_press(int button_select);
void UI_reset_all_states();
int UI_get_ENC();
void UI_Buzzer_PWM(int freq);
void UI_Buzzer_power(bool power);
void UI_Buzzer_beep();
void UI_set_LED0(bool value);
void UI_set_RGB(uint8_t index, int bright, int red, int green, int blue);
void UI_set_TC_EN(bool value);
void UI_set_TC_NFON(bool value);

#endif