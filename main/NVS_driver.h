#ifndef MAIN_NVS_DRIVER_H_
#define MAIN_NVS_DRIVER_H_

#include "nvs_flash.h"
#include "nvs.h"

esp_err_t NVS_read_values(char *NVS_name, int32_t *NVS_value);
esp_err_t NVS_write_values(char *NVS_name, int32_t NVS_value);
esp_err_t NVS_init();

#endif