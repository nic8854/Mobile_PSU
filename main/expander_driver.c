#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "expander_driver.h"

//define I2c Frequency
#define I2C_FREQ_HZ 400000

//Tag for ESP_LOG functions
static const char *TAG = "EXPANDER";

//define Error checking function
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

esp_err_t read_reg_8(expander_t *dev, uint8_t reg, uint8_t *val)
{
    CHECK_ARG(val);

    //Take I2C Mutex and Read 8Bit Register
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_read_reg(&dev->i2c_dev, reg, val, 1));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);


    return ESP_OK;
}

esp_err_t write_reg_8(expander_t *dev, uint8_t reg, uint8_t val)
{
    //Take I2C Mutex and Write 8Bit Register
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_write_reg(&dev->i2c_dev, reg, &val, 1));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;
}

esp_err_t read_reg_16(expander_t *dev, uint8_t reg, uint16_t *val)
{
    CHECK_ARG(val);
    //Take I2C Mutex and Read 16Bit Register
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_read_reg(&dev->i2c_dev, reg, val, 2));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    *val = (*val >> 8) | (*val << 8);

    return ESP_OK;
}

esp_err_t write_reg_16(expander_t *dev, uint8_t reg, uint16_t val)
{
    //Switch bytes around
    uint16_t v = (val >> 8) | (val << 8);
    //Take I2C Mutex and Write 16Bit Register
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, i2c_dev_write_reg(&dev->i2c_dev, reg, &v, 2));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;
}

esp_err_t expander_init_desc(expander_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);
    //check if address is correct
    if (addr < expander_addr_low || addr > expander_addr_high)
    {
        ESP_LOGE(TAG, "Invalid I2C address");
        return ESP_ERR_INVALID_ARG;
    }

    //write values into expander object
    dev->i2c_dev.port = port;
    dev->i2c_dev.addr = addr;
    dev->i2c_dev.cfg.sda_io_num = sda_gpio;
    dev->i2c_dev.cfg.scl_io_num = scl_gpio;
#if HELPER_TARGET_IS_ESP32
    dev->i2c_dev.cfg.master.clk_speed = I2C_FREQ_HZ;
#endif

    return i2c_dev_create_mutex(&dev->i2c_dev);
    ESP_LOGI(TAG, "--> Expander initialized successfully");
}



esp_err_t expander_configure(expander_t *dev, conf_t *config)
{
    //set config values with error checking
    esp_err_t error_check = 0;
    if(config->conf_port_0 != 0xFF) error_check = config_value(dev, reg_conf_port_0, config->conf_port_0);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    if(config->conf_port_1 != 0xFF) error_check = config_value(dev, reg_conf_port_1, config->conf_port_1);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    if(config->pol_inv_0   != 0x00) error_check = config_value(dev, reg_polinv_port_0, config->pol_inv_0);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    if(config->pol_inv_1   != 0x00) error_check = config_value(dev, reg_polinv_port_1, config->pol_inv_1);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);

    if(config->drive_port_0 != 0xFF) 
    {
        uint8_t drive_0_low = (uint8_t)(config->drive_port_0 & 0x00FF);
        error_check = config_value(dev, reg_outdr_port_0_low, drive_0_low);
        if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
        uint8_t drive_0_high = (uint8_t)(config->drive_port_0 >> 8);
        error_check = config_value(dev, reg_outdr_port_0_high, drive_0_high);
        if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    }
    if(config->drive_port_1 != 0xFF) 
    {
        uint8_t drive_1_low = (uint8_t)(config->drive_port_1 & 0x00FF);
        error_check = config_value(dev, reg_outdr_port_1_low, drive_1_low);
        if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
        uint8_t drive_1_high = (uint8_t)(config->drive_port_1 >> 8);
        error_check = config_value(dev, reg_outdr_port_1_high, drive_1_high);
        if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    }
    if(config->latch_port_0 != 0x00) error_check = config_value(dev, reg_latch_port_0, config->latch_port_0);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    if(config->latch_port_1 != 0x00) error_check = config_value(dev, reg_latch_port_1 , config->latch_port_1);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    if(config->pull_en_port_0 != 0x00) error_check = config_value(dev, reg_pull_en_port_0, config->pull_en_port_0);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    if(config->pull_en_port_1 != 0x00) error_check = config_value(dev, reg_pull_en_port_1, config->pull_en_port_1);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    if(config->pull_sel_port_0 != 0xFF) error_check = config_value(dev, reg_pull_select_port_0, config->pull_sel_port_0);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    if(config->pull_sel_port_1 != 0xFF) error_check = config_value(dev, reg_pull_select_port_1, config->pull_sel_port_1);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    if(config->interr_mask_port_0 != 0xFF) error_check = config_value(dev, reg_interr_mask_port_0, config->interr_mask_port_0);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    if(config->interr_mask_port_1 != 0xFF) error_check = config_value(dev, reg_interr_mask_port_1, config->interr_mask_port_1);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    if(config->out_port_conf != 0x00) error_check = config_value(dev, reg_out_port_conf, config->out_port_conf);
    if(error_check != 0) ESP_LOGE(__FUNCTION__, "AN ERROR OCCURED: 0x%x", error_check);
    return ESP_OK;
}

//configuration value set (tries 5 times)
esp_err_t config_value(expander_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t ref_value = 0;
    int counter = 5;
    while(counter > 0)
    {
        write_reg_8(dev, reg, value);
        vTaskDelay(2 / portTICK_PERIOD_MS);
        read_reg_8(dev, reg, &ref_value); 
        if(ref_value == value) counter = -1;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        counter--;
    }
    if(ref_value == value) return ESP_OK;
    else return ESP_ERR_TIMEOUT;
}