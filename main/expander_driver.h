#ifndef MAIN_BUTTON_DRIVER_H_
#define MAIN_BUTTON_DRIVER_H_

#include "i2cdev.h"
#include <esp_err.h>

//I2C Addresses
#define expander_addr_low 0x20
#define expander_addr_high 0x21

//Register addresses
#define reg_in_port_0          0x00
#define reg_in_port_1          0x01
#define reg_out_port_0         0x02
#define reg_out_port_1         0x03
#define reg_polinv_port_0      0x04
#define reg_polinv_port_1      0x05
#define reg_conf_port_0        0x06
#define reg_conf_port_1        0x07
#define reg_outdr_port_0_low   0x40
#define reg_outdr_port_0_high  0x41
#define reg_outdr_port_1_low   0x42
#define reg_outdr_port_1_high  0x43
#define reg_latch_port_0       0x44
#define reg_latch_port_1       0x45
#define reg_pull_en_port_0     0x46
#define reg_pull_en_port_1     0x47
#define reg_pull_select_port_0 0x48
#define reg_pull_select_port_1 0x49
#define reg_interr_mask_port_0 0x4A
#define reg_interr_mask_port_1 0x4B
#define reg_interr_stat_port_0 0x4C
#define reg_interr_stat_port_1 0x4D
#define reg_out_port_conf      0x4F

//Default configuration (should be set when initializing)
#define Default_Config { \
    .conf_port_0 = 0xFF, \
    .conf_port_1 = 0xFF, \
    .pol_inv_0 = 0x00, \
    .pol_inv_1 = 0x00, \
    .drive_port_0 = 0xFFFF, \
    .drive_port_1 = 0xFFFF, \
    .latch_port_0 = 0x00, \
    .latch_port_1 = 0x00, \
    .pull_en_port_0 = 0x00, \
    .pull_en_port_1 = 0x00, \
    .pull_sel_port_0 = 0xFF, \
    .pull_sel_port_1 = 0xFF, \
    .interr_mask_port_0 = 0xFF, \
    .interr_mask_port_1 = 0xFF, \
    .out_port_conf = 0x00 \
}

//Expander Object
typedef struct
{
    uint8_t conf_port_0;
    uint8_t conf_port_1;
    uint8_t pol_inv_0;
    uint8_t pol_inv_1; 
    uint16_t drive_port_0;
    uint16_t drive_port_1;
    uint8_t latch_port_0;
    uint8_t latch_port_1;
    uint8_t pull_en_port_0;
    uint8_t pull_en_port_1;
    uint8_t pull_sel_port_0;
    uint8_t pull_sel_port_1;
    uint8_t interr_mask_port_0;
    uint8_t interr_mask_port_1;
    uint8_t out_port_conf;
} conf_t;

//I2C Expander Object
typedef struct
{
    i2c_dev_t i2c_dev;

    uint16_t config;
    float i_lsb, p_lsb;
} expander_t;

esp_err_t read_reg_8(expander_t *dev, uint8_t reg, uint8_t *val);
esp_err_t write_reg_8(expander_t *dev, uint8_t reg, uint8_t val);
esp_err_t read_reg_16(expander_t *dev, uint8_t reg, uint16_t *val);
esp_err_t write_reg_16(expander_t *dev, uint8_t reg, uint16_t val);
esp_err_t expander_init_desc(expander_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);
esp_err_t expander_configure(expander_t *dev, conf_t *config);
esp_err_t config_value(expander_t *dev, uint8_t port, uint8_t value);

#endif