#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "APA102.h"

#define TAG "APA102"

#define APA102_FIRST_LED_OFFSET 4
#define APA102_FRAME_PREAMBLE 0xE0 // 11100000

uint8_t count;
spi_device_handle_t handle_LED;
spi_transaction_t transaction_LED;

bool hasInit = 0;
uint8_t bufferSize;
uint8_t * outputBuffer;
uint8_t sendBuffer[4+4*4+3];

void APA102_Init(size_t ledsCount, spi_host_device_t spiDevice_LED) {
    count = ledsCount;

    bufferSize = 23; /*
        4 + // Start Frame: 4 bytes of 0's
        count * 4 + // Each LED has 4 bytes of data
        (count / 4) + 1; // End frame of just 1's
*/
    
    

    //Configuration for the SPI bus
    spi_bus_config_t buscfg={
        .mosi_io_num=LED_MOSI,
        .miso_io_num=-1,
        .sclk_io_num=LED_SCLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };

    //Configuration for the SPI device on the other side of the bus
    spi_device_interface_config_t devcfg={
        .command_bits=0,
        .address_bits=0,
        .dummy_bits=0,
        .clock_speed_hz=1000000,
        .duty_cycle_pos=0,        //50% duty cycle
        .mode=0,
        .spics_io_num=-1,
        .cs_ena_posttrans=0,        //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .queue_size=50
    };


    esp_err_t ret;

    ret = spi_bus_initialize(spiDevice_LED, &buscfg, 2);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_add_device(spiDevice_LED, &devcfg, &handle_LED);
    ESP_ERROR_CHECK(ret);

    //outputBuffer = (uint8_t *) malloc(bufferSize);



    // Preamble: 4 bytes of 0
    //outputBuffer[0] = 0xFF;
    //outputBuffer[1] = 0;
    //outputBuffer[2] = 0;
    //outputBuffer[3] = 0;

    sendBuffer[0] = 0x00;
    sendBuffer[1] = 0x00;
    sendBuffer[2] = 0x00;
    sendBuffer[3] = 0x00;

    // Postamble: at least n/2 bits of 1
    for (int k = APA102_FIRST_LED_OFFSET + 4 * count; k < bufferSize; k++) {
        //outputBuffer[k] = 255; // 11111111
        sendBuffer[k] = 0xFF;
    }

    memset(&transaction_LED, 0, sizeof(transaction_LED));
    transaction_LED.length = sizeof(sendBuffer)*8;
    transaction_LED.tx_buffer = sendBuffer;
    transaction_LED.rx_buffer = NULL;
    ESP_LOGI(TAG, "LED Init Done...");
}

void setPixel(uint8_t index, int brightness, int red, int green, int blue) {
    int bytesOffset = APA102_FIRST_LED_OFFSET + index * 4;    
    sendBuffer[bytesOffset] = APA102_FRAME_PREAMBLE | brightness;
    sendBuffer[bytesOffset + 1] = blue;
    sendBuffer[bytesOffset + 2] = green;
    sendBuffer[bytesOffset + 3] = red;
    //ESP_LOGI(TAG, "LED SetPixl...");
}

void flush() {
    //ESP_LOGI(TAG, "LED Flush...");
    //ESP_LOGI(TAG, "Bytecount: %i...", bufferSize);
    //for(int i = 0; i < bufferSize/4;i++) {
        //ESP_LOGW(TAG, "transaction: %X-%X-%X-%X", sendBuffer[i*4+0], sendBuffer[i*4+1], sendBuffer[i*4+2], sendBuffer[i*4+3]);
    //}
    //transaction.length = sizeof(sendBuffer);
    //transaction.tx_buffer = sendBuffer;

    int ret = spi_device_queue_trans(handle_LED, &transaction_LED, portMAX_DELAY);
    ESP_ERROR_CHECK(ret);
}