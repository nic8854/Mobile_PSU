#ifndef STACK_USAGE_QUEUE_HANDLER_DRIVER_H_
#define STACK_USAGE_QUEUE_HANDLER_DRIVER_H_

#include "freertos/task.h"

#define MASTER_TASK  0
#define ADC_TASK     1
#define INA_TASK     2
#define BUTTON_TASK  3
#define IO_TASK      5

typedef struct{
	uint8_t task_num;
	uint32_t size;
}stack_usage_dataframe_t;

QueueHandle_t stack_usage_queue;

#endif