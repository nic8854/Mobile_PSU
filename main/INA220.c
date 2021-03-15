#include "INA220.h"
#include "esp_log.h"

#define TAG "INA220"

#define I2C_FREQ_HZ 1000000 // Max 1MHz for esp-idf

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)
#define CHECK_LOGE(dev, x, msg, ...) do { \
        esp_err_t __; \
        if ((__ = x) != ESP_OK) { \
            I2C_DEV_GIVE_MUTEX(&dev->i2c_dev); \
            ESP_LOGE(TAG, msg, ## __VA_ARGS__); \
            return __; \
        } \
    } while (0)

static esp_err_t read_register16(i2c_dev_t *dev, uint8_t reg, uint16_t* r)
{
    uint8_t d[] = { 0, 0 };

    CHECK(i2c_dev_read_reg(dev, reg, d, 2));
    *r = d[1] | (d[0] << 8);

    return ESP_OK;
}

static esp_err_t write_register16(i2c_dev_t *dev, uint8_t reg, uint16_t value) {
    uint8_t value8[2] = {((value>>8)&0x00FF), (value&0x00FF)};
    CHECK(i2c_dev_write(dev, &reg, 1, value8, 2));
    return ESP_OK;
}

static uint16_t ina220Config16(ina220_params_t *params) {
    uint16_t returnValue = 0;
    returnValue |= (params->busRange << BRNG_POS);
    returnValue |= (params->shuntRange << PGA_POS);
    returnValue |= (params->bus_resolution << BADC_POS);
    returnValue |= (params->shunt_resolution << SADC_POS);
    returnValue |= (params->mode << MODE_POS);
    return returnValue;
}

esp_err_t ina220_init_desc(ina220_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio) {
    CHECK_ARG(dev);
    if (addr < INA220ADDRESSMIN || addr > INA220ADDRESSMAX)
    {
        ESP_LOGE(TAG, "Invalid I2C address");
        return ESP_ERR_INVALID_ARG;
    }
    dev->i2c_dev.port = port;
    dev->i2c_dev.addr = addr;
    dev->i2c_dev.cfg.sda_io_num = sda_gpio;
    dev->i2c_dev.cfg.scl_io_num = scl_gpio;
    dev->i2c_dev.cfg.master.clk_speed = I2C_FREQ_HZ;
    dev->currentLSB = 0;
    dev->powerLSB = 0;
    CHECK(i2c_dev_create_mutex(&dev->i2c_dev));
    return ESP_OK;
}

esp_err_t ina220_free_desc(ina220_t *dev)
{
    CHECK_ARG(dev);
    return i2c_dev_delete_mutex(&dev->i2c_dev);
}

esp_err_t ina220_init_default_params(ina220_params_t *params) {
    CHECK_ARG(params);
    params->mode = MODE_SHUNT_BUS_CONTINUOUS;
    params->busRange = VOLTAGERANGE_32V;
    params->shuntRange = SHUTVOLTAGEGAIN_320mv;
    params->bus_resolution = RESOLUTION_12bit;
    params->shunt_resolution = RESOLUTION_12bit;
    return ESP_OK;
}

esp_err_t ina220_init(ina220_t *dev, ina220_params_t *params)
{
    CHECK_ARG(dev);
    CHECK_ARG(params);

    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

    // Soft reset.
    CHECK_LOGE(dev, write_register16(&dev->i2c_dev, INA220_CONFIGURATION_ADDR, 0x8000), "Failed resetting sensor");
    vTaskDelay(10/portTICK_PERIOD_MS);

    CHECK_LOGE(dev, read_register16(&dev->i2c_dev, INA220_CONFIGURATION_ADDR, &dev->id), "Sensor not found");

    if (dev->id != 0x399F)
    {
        CHECK_LOGE(dev, ESP_ERR_INVALID_VERSION, "Config Data wrong! Data: 0x%X", dev->id);
    }
    uint16_t data = 0;
    data = ina220Config16(params);
    ESP_LOGW(TAG, "Write ConfigData to INA220: 0x%X", data);
    CHECK_LOGE(dev, write_register16(&dev->i2c_dev, INA220_CONFIGURATION_ADDR, data), "Could not set Params");
    
    
    read_register16(&dev->i2c_dev, INA220_CONFIGURATION_ADDR, &data);
    ESP_LOGW(TAG, "ConfigReg readback= 0x%X", data);

    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;
}

double ina220_getVShunt_mv(ina220_t *dev, ina220_params_t *params) {
    double returnValue = 0;
    uint16_t data = 0;
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    read_register16(&dev->i2c_dev, INA220_SHUNTVOLTAGE_ADDR, &data);
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    switch(params->shuntRange) {
        case SHUTVOLTAGEGAIN_320mv:
            returnValue = (double)((int16_t)data)/100.0;
        break;
        case SHUTVOLTAGEGAIN_160mv:
            data &= 0xBFFF; //Remove additional sign bit at pos 14
            returnValue = (double)((int16_t)data)/100.0;
        break;
        case SHUTVOLTAGEGAIN_80mv:
            data &= 0xAFFF; //Remove additional sign bit at pos 14 and 13
            returnValue = (double)((int16_t)data)/100.0;
        break;
        case SHUTVOLTAGEGAIN_40mv:
            data &= 0x8FFF; //Remove additional sign bit at pos 14 to 12
            returnValue = (double)((int16_t)data)/100.0;
        break;
    }
    return returnValue;
}
double ina220_getVBus_mv(ina220_t *dev, ina220_params_t *params) {
    double returnValue = 0;
    uint16_t data = 0;
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    read_register16(&dev->i2c_dev, INA220_BUSVOLTAGE_ADDR, &data);
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    switch(params->busRange) {
        case VOLTAGERANGE_32V:
            returnValue = (double)(data>>3)*4.0;
        break;
        case VOLTAGERANGE_16V:
            returnValue = (double)(data>>3)*4.0;
        break;
    }
    return returnValue;
}

bool ina220_newDataAvailable(ina220_t *dev, ina220_params_t *params) {    
    uint16_t data = 0;
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    read_register16(&dev->i2c_dev, INA220_BUSVOLTAGE_ADDR, &data);
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    bool cnvr = (data>>1)&0x0001;
    bool ovf = (data)&0x0001;
    if(cnvr == true && ovf == false) {
        return true;
    }
    return false;
}

double ina220_getPower_mW(ina220_t *dev, ina220_params_t *params) {
    double returnValue = 0;
    uint16_t data = 0;
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    read_register16(&dev->i2c_dev, INA220_POWER_ADDR, &data);
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    returnValue = (double)(data)*dev->powerLSB;
    returnValue *= 1000;
    return returnValue;
}

double ina220_getCurrent_mA(ina220_t *dev, ina220_params_t *params) {
    double returnValue = 0;
    uint16_t data = 0;
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    read_register16(&dev->i2c_dev, INA220_CURRENT_ADDR, &data);
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    returnValue = (double)((int16_t)(data))*dev->currentLSB;
    returnValue *= 1000;
    return returnValue;
}

esp_err_t ina220_setCalibrationData(ina220_t *dev, ina220_params_t *params, double maxCurrent_A, double shuntRes_Ohm) {
    uint16_t calibData = 0;
    double currentLSB = maxCurrent_A/32768.0;
    double calData = 0.04096 / (currentLSB * shuntRes_Ohm);
    if(calData < 0 && calData > 65535.0) {
        ESP_LOGE(TAG, "Cant calibrate that!");
        return ESP_ERR_INVALID_SIZE;
    }
    calibData = (uint16_t)(calData);
    dev->currentLSB = currentLSB;
    dev->powerLSB = 20 * currentLSB;
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    write_register16(&dev->i2c_dev, INA220_CALIBRATION, calibData);
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    return ESP_OK;
}