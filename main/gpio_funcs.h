#ifndef GPIO_FUNCS_H
#define GPIO_FUNCS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

// GPIO pins controlling the locks
#define LOCK_GPIO_0 33 
#define LOCK_GPIO_1 32
#define LOCK_GPIO_2 23 
#define LOCK_GPIO_3 22 
#define LOCK_GPIO_4 21 
#define LOCK_GPIO_5 4 
// Bit mask for output pins (is there a more reasonable way of doing this definition?)
#define GPIO_OUTPUT_PIN_SEL ((1ULL << LOCK_GPIO_0) | (1ULL << LOCK_GPIO_1) \
    | (1ULL << LOCK_GPIO_2) | (1ULL << LOCK_GPIO_3) | (1ULL << LOCK_GPIO_4) \
    | (1ULL << LOCK_GPIO_5))

#define DPD_GPIO 13 // Power down detection pin
#define DPD_GPIO_SEL (1ULL << DPD_GPIO) // Bit mask for DPD pin

void IRAM_ATTR write_lock_idx_isr(void* arg);

uint8_t lock_pin_init();

uint8_t dpd_pin_init(int* lock_idx);

extern QueueHandle_t DPD_event_queue;

#endif
