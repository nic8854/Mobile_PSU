#ifndef MAIN_BUTTON_DRIVER_H_
#define MAIN_BUTTON_DRIVER_H_

static esp_err_t read_reg_8(expander_t *dev, uint8_t reg, uint16_t *val);
static esp_err_t write_reg_8(expander_t *dev, uint8_t reg, uint16_t val);
esp_err_t expander_init(expander_t *dev);

typedef struct
{
    i2c_dev_t i2c_dev;

    uint16_t config;
    float i_lsb, p_lsb;
} expander_t;

#endif