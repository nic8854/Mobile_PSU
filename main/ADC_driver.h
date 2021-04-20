#ifndef MAIN_AD_DRIVER_H_
#define MAIN_AD_DRIVER_H_

#include "i2cdev.h"
#include <esp_err.h>

//I2C Addresses
#define AD_addr_low 0x23
#define AD_addr_high 0x24
#define default_config 0x0010
#define default_interval 0x01

//Register addresses
#define reg_convert            0x00
#define reg_alert_stat         0x01
#define reg_config             0x02
#define reg_cycle_timer        0x03
#define reg_data_low_CH1       0x04
#define reg_data_high_CH1      0x05
#define reg_hysteresis_CH1     0x06
#define reg_data_low_CH2       0x07
#define reg_data_high_CH2      0x08
#define reg_hysteresis_CH2     0x09
#define reg_data_low_CH3       0x0A
#define reg_data_high_CH3      0x0B
#define reg_hysteresis_CH3     0x0C
#define reg_data_low_CH4       0x0D
#define reg_data_high_CH4      0x0E
#define reg_hysteresis_CH4     0x0F

//I2C AD Object
typedef struct
{
    i2c_dev_t i2c_dev;

    uint16_t config;
    float i_lsb, p_lsb;
} AD_t;

esp_err_t AD_read_reg_8(AD_t *dev, uint8_t reg, uint8_t *val);
esp_err_t AD_write_reg_8(AD_t *dev, uint8_t reg, uint8_t val);
esp_err_t AD_read_reg_16(AD_t *dev, uint8_t reg, uint16_t *val);
esp_err_t AD_write_reg_16(AD_t *dev, uint8_t reg, uint16_t val);
esp_err_t AD_init_desc(AD_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);

#endif