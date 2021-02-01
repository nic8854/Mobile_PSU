#ifndef MAIN_BUTTON_DRIVER_H_
#define MAIN_BUTTON_DRIVER_H_

static esp_err_t read_reg_8(expander_t *dev, uint8_t reg, uint16_t *val);
static esp_err_t write_reg_8(expander_t *dev, uint8_t reg, uint16_t val);
static esp_err_t read_reg_16(expander_t *dev, uint8_t reg, uint16_t *val);
static esp_err_t write_reg_16(expander_t *dev, uint8_t reg, uint16_t val);
esp_err_t expander_init(expander_t *dev);
esp_err_t ina219_configure(ina219_t *dev, ina219_bus_voltage_range_t u_range,
        ina219_gain_t gain, ina219_resolution_t u_res,
        ina219_resolution_t i_res, ina219_mode_t mode);

typedef struct
{
    i2c_dev_t i2c_dev;

    uint16_t config;
    float i_lsb, p_lsb;
} expander_t;

#define expander_addr_low 0x20
#define expander_addr_high 0x21

#endif