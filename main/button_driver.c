#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "button_driver.h"

#define I2C_FREQ_HZ 1000000

static const char *TAG = "EXPANDER";

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

esp_err_t read_reg_8(expander_t *dev, uint8_t reg, uint8_t *val)
{
    CHECK_ARG(val);

    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_read_reg(&dev->i2c_dev, reg, val, 1));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);


    return ESP_OK;
}

esp_err_t write_reg_8(expander_t *dev, uint8_t reg, uint8_t val)
{
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_write_reg(&dev->i2c_dev, reg, &val, 1));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;
}

esp_err_t read_reg_16(expander_t *dev, uint8_t reg, uint16_t *val)
{
    CHECK_ARG(val);

    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_read_reg(&dev->i2c_dev, reg, val, 2));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    *val = (*val >> 8) | (*val << 8);

    return ESP_OK;
}

esp_err_t write_reg_16(expander_t *dev, uint8_t reg, uint16_t val)
{
    uint16_t v = (val >> 8) | (val << 8);

    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_write_reg(&dev->i2c_dev, reg, &v, 2));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;
}

esp_err_t expander_init_desc(expander_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);

    if (addr < expander_addr_low || addr > expander_addr_high)
    {
        ESP_LOGE(TAG, "Invalid I2C address");
        return ESP_ERR_INVALID_ARG;
    }

    dev->i2c_dev.port = port;
    dev->i2c_dev.addr = addr;
    dev->i2c_dev.cfg.sda_io_num = sda_gpio;
    dev->i2c_dev.cfg.scl_io_num = scl_gpio;
#if HELPER_TARGET_IS_ESP32
    dev->i2c_dev.cfg.master.clk_speed = I2C_FREQ_HZ;
#endif

    return i2c_dev_create_mutex(&dev->i2c_dev);
}

esp_err_t ina219_configure(expander_t *dev, 
uint8_t conf_port_0, uint8_t conf_port_1, 
uint8_t pol_inv_0, uint8_t pol_inv_1, 
uint16_t drive_port_0, uint16_t drive_port_1,
uint8_t latch_port_0, uint8_t latch_port_1,
uint8_t pull_en_port_0, uint8_t pull_en_port_1,
uint8_t pull_sel_port_0, uint8_t pull_sel_port_1,
uint8_t interr_mask_port_0, uint8_t interr_mask_port_1,
uint8_t out_port_conf)
{
    if(conf_port_0 != -1) write_reg_8(&dev, reg_conf_port_0, conf_port_0);
    if(conf_port_1 != -1) write_reg_8(&dev, reg_conf_port_1, conf_port_1);
    if(pol_inv_0 != -1) write_reg_8(&dev, reg_polinv_port_0, pol_inv_0);
    if(pol_inv_1 != -1) write_reg_8(&dev, reg_polinv_port_1, pol_inv_1);
    if(drive_port_0 != -1) 
    {
        uint8_t drive_0_low = (uint8_t)(drive_port_0 & 0x00FF);
        write_reg_8(&dev, reg_outdr_port_0_low, drive_0_low);
        uint8_t drive_0_high = (uint8_t)(drive_port_0 >> 8);
        write_reg_8(&dev, reg_outdr_port_0_high, drive_0_high);
    }
    if(drive_port_1 != -1) 
    {
        uint8_t drive_1_low = (uint8_t)(drive_port_1 & 0x00FF);
        write_reg_8(&dev, reg_outdr_port_1_low, drive_1_low);
        uint8_t drive_1_high = (uint8_t)(drive_port_1 >> 8);
        write_reg_8(&dev, reg_outdr_port_1_high, drive_1_high);
    }
    if(latch_port_0 != -1) write_reg_8(&dev, reg_latch_port_0, latch_port_0);
    if(latch_port_1 != -1) write_reg_8(&dev, reg_latch_port_1 , latch_port_1);
    if(pull_en_port_0 != -1) write_reg_8(&dev, reg_pull_en_port_0, pull_en_port_0);
    if(pull_en_port_1 != -1) write_reg_8(&dev, reg_pull_en_port_1, pull_en_port_1);
    if(pull_sel_port_0 != -1) write_reg_8(&dev, reg_pull_select_port_0, pull_sel_port_0);
    if(pull_sel_port_1 != -1) write_reg_8(&dev, reg_pull_select_port_1, pull_sel_port_1);
    if(interr_mask_port_0 != -1) write_reg_8(&dev, reg_interr_mask_port_0, interr_mask_port_0);
    if(interr_mask_port_1 != -1) write_reg_8(&dev, reg_interr_mask_port_1, interr_mask_port_1);
    if(out_port_conf != -1) write_reg_8(&dev, reg_out_port_conf, out_port_conf);

    return ESP_OK;
}
