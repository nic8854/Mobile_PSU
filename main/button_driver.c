#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "i2cdev.h"
#include "button_driver.h"

#define I2C_FREQ_HZ 1000000

static const char *TAG = "INA219";

#define REG_IN_LOWER 0
#define REG_IN_UPPER 1
#define REG_OUT_LOWER 2
#define REG_OUT_UPPER 3
#define REG_POL_INV_LOWER 4
#define REG_POL_INV_UPPER 5
#define REG_CONF_LOWER 6
#define REG_CONF_UPPER 7

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

static esp_err_t read_reg_8(expander_t *dev, uint8_t reg, uint16_t *val)
{
    CHECK_ARG(val);

    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_read_reg(&dev->i2c_dev, reg, val, 1));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);


    return ESP_OK;
}

static esp_err_t write_reg_8(expander_t *dev, uint8_t reg, uint16_t val)
{
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_write_reg(&dev->i2c_dev, reg, &v, 1));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;
}

esp_err_t expander_init(expander_t *dev)
{
    CHECK_ARG(dev);

    CHECK(read_reg_8(dev, REG_CONFIG, &dev->config));

    ESP_LOGD(TAG, "Initialize, config: 0x%04x", dev->config);

    return ESP_OK;
}