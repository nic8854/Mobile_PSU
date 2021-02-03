#ifndef MAIN_BUTTON_DRIVER_H_
#define MAIN_BUTTON_DRIVER_H_

static esp_err_t read_reg_8(expander_t *dev, uint8_t reg, uint8_t *val);
static esp_err_t write_reg_8(expander_t *dev, uint8_t reg, uint8_t val);
static esp_err_t read_reg_16(expander_t *dev, uint8_t reg, uint16_t *val);
static esp_err_t write_reg_16(expander_t *dev, uint8_t reg, uint16_t val);
esp_err_t expander_init(expander_t *dev);
esp_err_t ina219_configure(ina219_t *dev, ina219_bus_voltage_range_t u_range,
        ina219_gain_t gain, ina219_resolution_t u_res,
        ina219_resolution_t i_res, ina219_mode_t mode);
esp_err_t ina219_init_desc(ina219_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);

typedef struct
{
    i2c_dev_t i2c_dev;

    uint16_t config;
    float i_lsb, p_lsb;
} expander_t;

#define expander_addr_low 0x20
#define expander_addr_high 0x21

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


#endif