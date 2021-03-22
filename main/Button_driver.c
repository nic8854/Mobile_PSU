#include "stdio.h"
#include "math.h"
#include "expander_driver.h"
#include "driver/gpio.h"

#define GPIO_OUTPUT_IO_0    2
#define GPIO_OUTPUT_IO_1    26
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))

expander_t dev_port_expander;
conf_t config;
gpio_config_t io_conf;

void Button_init()
{
    memset(&dev_port_expander, 0, sizeof(expander_t));
    config = Default_Config;
	config.conf_port_0 = 0xFF;
	config.conf_port_1 = 0x00;
	config.pol_inv_0 = 0xFF;
	config.pol_inv_1 = 0x00;
    expander_init_desc(&dev_port_expander, I2C_EXP_ADDR, I2C_PORT, SDA_GPIO, SCL_GPIO);
	expander_configure(&dev_port_expander, &config);
    io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

void button_handler()
{
    read_reg_8(&dev_port_expander, reg_in_port_0, &in_value);
}