#include "stdio.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "math.h"
#include "NVS_driver.h"
#include "esp_log.h"

static const char *TAG = "NVS_Driver";

nvs_handle NVS_config;

/**
 * Function to read values from NVS
 *
 * NVS must be initialized to use
 * @param NVS_name Where the value should be read from. Enter like this: "name"
 * @param NVS_value Where the value should be read to.
 *  
 * @endcode
 * \ingroup NVS
 */
esp_err_t NVS_read_values(char *NVS_name, int32_t *NVS_value)
{
    esp_err_t err = 0;

    // Open NVS Handle
    printf("\n");
    printf("Opening NVS handle... ");
    err = nvs_open("storage", NVS_READWRITE, &NVS_config);
    if (err != ESP_OK) 
	{
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else 
	{
		// Reading from NVS
        printf("Reading Value from NVS ... ");
        err = nvs_get_i32(NVS_config, NVS_name, NVS_value);
        switch (err) {
            case ESP_OK:
                printf("Success! Value = %d\n", *NVS_value);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
        nvs_close(NVS_config);
	}
    return err;
}

/**
 * Function to write values to NVS
 *
 * NVS must be initialized to use
 * @param NVS_name Where the value should be read from. Enter like this: "name"
 * @param NVS_value Value which should be written.
 *  
 * @endcode
 * \ingroup NVS
 */
esp_err_t NVS_write_values(char *NVS_name, int32_t NVS_value)
{
    esp_err_t err = 0;
    // Open NVS Handle
    printf("\n");
    printf("Opening NVS handle... ");
    err = nvs_open("storage", NVS_READWRITE, &NVS_config);
    if (err != ESP_OK) 
	{
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else 
	{
        err = nvs_set_i32(NVS_config, NVS_name, NVS_value);

	    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
	    // Commit written value.
	    printf("Committing updates in NVS ... ");
	    err = nvs_commit(NVS_config);
	    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        err = 0;
	    // Close
	    nvs_close(NVS_config);
    }
    return err;
}

/**
 * Function to initialize NVS
 *  
 * @endcode
 * \ingroup NVS
 */
esp_err_t NVS_init()
{
    esp_err_t err = 0;
    // Initialize NVS
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    return err;
}