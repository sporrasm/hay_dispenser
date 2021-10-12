#ifndef GPIO_FUNCS_H
#define GPIO_FUNCS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

// GPIO pins controlling the locks. Lock 0 is the bottom-most lock
#define LOCK_GPIO_0 4
#define LOCK_GPIO_1 21
#define LOCK_GPIO_2 23
#define LOCK_GPIO_3 22
#define LOCK_GPIO_4 32
#define LOCK_GPIO_5 33
// Bit mask for output pins (is there a more reasonable way of doing this definition?)
#define GPIO_OUTPUT_PIN_SEL ((1ULL << LOCK_GPIO_0) | (1ULL << LOCK_GPIO_1) \
    | (1ULL << LOCK_GPIO_2) | (1ULL << LOCK_GPIO_3) | (1ULL << LOCK_GPIO_4) \
    | (1ULL << LOCK_GPIO_5))

#define DPD_GPIO 13 // Power down detection pin
#define DPD_GPIO_SEL (1ULL << DPD_GPIO) // Bit mask for DPD pin

#define LEFT_PIN 34
#define RIGHT_PIN 35
#define CENTER_PIN 25

// Time used for debouncing (in milliseconds)
#define DEBOUNCE_MS 200

#define BUTTON_PIN_SEL ((1ULL << LEFT_PIN) | (1ULL << RIGHT_PIN) | (1ULL << CENTER_PIN))

void IRAM_ATTR write_lock_idx_isr(void* arg);
void IRAM_ATTR button_left_interrupt(void* arg);
void IRAM_ATTR button_right_interrupt(void* arg);
void IRAM_ATTR button_center_interrupt(void* arg);

uint8_t lock_pin_init();
uint8_t button_pin_init();

uint8_t dpd_pin_init(int* lock_idx);

extern QueueHandle_t DPD_event_queue;
extern portMUX_TYPE lmux;
extern portMUX_TYPE rmux;
extern portMUX_TYPE cmux;
extern volatile uint32_t lnum_edges;
extern volatile uint32_t rnum_edges;
extern volatile uint32_t cnum_edges;
extern volatile uint32_t lstate;
extern volatile uint32_t rstate;
extern volatile uint32_t cstate;
extern volatile uint32_t ldebounce_tick;
extern volatile uint32_t rdebounce_tick;
extern volatile uint32_t cdebounce_tick;

#endif
