#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include "esp_log.h"
#include "ADC_driver.h"

//define I2C frequency
#define I2C_FREQ_HZ 400000

//Tag for ESP_LOG functions
static const char *TAG = "AD_driver";

//define Error checking function
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

esp_err_t AD_read_reg_8(AD_t *dev, uint8_t reg, uint8_t *val)
{
    CHECK_ARG(val);

    //Take I2C Mutex and Read 8Bit Register
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_read_reg(&dev->i2c_dev, reg, val, 1));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);


    return ESP_OK;
}

esp_err_t AD_write_reg_8(AD_t *dev, uint8_t reg, uint8_t val)
{
    //Take I2C Mutex and Write 8Bit Register
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_write_reg(&dev->i2c_dev, reg, &val, 1));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;
}

esp_err_t AD_read_reg_16(AD_t *dev, uint8_t reg, uint16_t *val)
{
    CHECK_ARG(val);
    //Take I2C Mutex and Read 16Bit Register
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_read_reg(&dev->i2c_dev, reg, val, 2));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    *val = (*val >> 8) | (*val << 8);

    return ESP_OK;
}

esp_err_t AD_write_reg_16(AD_t *dev, uint8_t reg, uint16_t val)
{
    //Switch bytes around
    uint16_t v = (val >> 8) | (val << 8);
    //Take I2C Mutex and Write 16Bit Register
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_write_reg(&dev->i2c_dev, reg, &v, 2));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;
}


esp_err_t AD_init_desc(AD_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    //check if argument !=0
    CHECK_ARG(dev);
    //check if address is correct
    if (addr < AD_addr_low || addr > AD_addr_high)
    {
        ESP_LOGE(TAG, "Invalid I2C address");
        return ESP_ERR_INVALID_ARG;
    }

    //write values into AD object
    dev->i2c_dev.port = port;
    dev->i2c_dev.addr = addr;
    dev->i2c_dev.cfg.sda_io_num = sda_gpio;
    dev->i2c_dev.cfg.scl_io_num = scl_gpio;
#if HELPER_TARGET_IS_ESP32
    dev->i2c_dev.cfg.master.clk_speed = I2C_FREQ_HZ;
#endif

    return i2c_dev_create_mutex(&dev->i2c_dev);
    ESP_LOGI(TAG, "--> AD initialized successfully");
}

