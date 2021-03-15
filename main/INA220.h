#ifndef INA220_H
#define INA220_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <i2cdev.h>

#define INA220ADDRESSMIN   0x40
#define INA220ADDRESSMAX   0x4F

#define INA220_CONFIGURATION_ADDR       0x00
#define INA220_SHUNTVOLTAGE_ADDR        0x01
#define INA220_BUSVOLTAGE_ADDR          0x02
#define INA220_POWER_ADDR               0x03
#define INA220_CURRENT_ADDR             0x04
#define INA220_CALIBRATION              0x05

#define RST_POS                         15
#define BRNG_POS                        13
#define PGA_POS                         11
#define BADC_POS                        7
#define SADC_POS                        3
#define MODE_POS                        0


typedef enum {
    VOLTAGERANGE_16V = 0,
    VOLTAGERANGE_32V = 1
} brng_t;

typedef enum {
    SHUTVOLTAGEGAIN_40mv = 0,
    SHUTVOLTAGEGAIN_80mv = 1,
    SHUTVOLTAGEGAIN_160mv = 2,
    SHUTVOLTAGEGAIN_320mv = 3
} pga_t;

typedef enum {
    RESOLUTION_9bit = 0x0,
    RESOLUTION_10bit = 0x1,
    RESOLUTION_11bit = 0x2,
    RESOLUTION_12bit = 0x3,
    RESOLUTION_2Samples = 0x9,
    RESOLUTION_4Samples = 0xA,
    RESOLUTION_8Samples = 0xB,
    RESOLUTION_16Samples = 0xC,
    RESOLUTION_32Samples = 0xD,
    RESOLUTION_64Samples = 0xE,
    RESOLUTION_128Samples = 0xF
} adcResolution_t;

typedef enum {
    MODE_POWERDOWN = 0x0,
    MODE_SHUNTVOLTAGE_TRIG = 0x1,
    MODE_BUSVOLTAGE_TRIG = 0x2,
    MODE_SHUNT_BUS_TRIG = 0x3,
    MODE_ADCOFF = 0x4,
    MODE_SHUNTVOLTAGE_CONTINUOUS = 0x5,
    MODE_BUSVOLTAGE_CONTINUOUS = 0x6,
    MODE_SHUNT_BUS_CONTINUOUS = 0x7
} ina220Mode_t;


typedef struct {
    ina220Mode_t mode;
    adcResolution_t bus_resolution;
    adcResolution_t shunt_resolution;
    pga_t shuntRange;
    brng_t busRange;
} ina220_params_t;

typedef struct {
    i2c_dev_t   i2c_dev;  //!< I2C device descriptor
    uint16_t    id;       //!< Chip ID
    double      currentLSB;
    double      powerLSB;
} ina220_t;

esp_err_t ina220_init_desc(ina220_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);
esp_err_t ina220_free_desc(ina220_t *dev);
esp_err_t ina220_init_default_params(ina220_params_t *params);
esp_err_t ina220_init(ina220_t *dev, ina220_params_t *params);

double ina220_getVShunt_mv(ina220_t *dev, ina220_params_t *params);
double ina220_getVBus_mv(ina220_t *dev, ina220_params_t *params);
bool ina220_newDataAvailable(ina220_t *dev, ina220_params_t *params);
double ina220_getPower_mW(ina220_t *dev, ina220_params_t *params);
double ina220_getCurrent_mA(ina220_t *dev, ina220_params_t *params);
esp_err_t ina220_setCalibrationData(ina220_t *dev, ina220_params_t *params, double maxCurrent_A, double shuntRes_Ohm);

#endif