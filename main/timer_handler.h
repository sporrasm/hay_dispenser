#ifndef _TIMER_HANDLER_H_
#define _TIMER_HANDLER_H_

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gptimer.h"
#include <inttypes.h>

time_t* timeFromString(char** times, unsigned int len);
gptimer_handle_t ini_timer(uint64_t time_interval);
void sort_time(time_t* arr, int len);
int comp_time(const void* elem1, const void* elem2);
void IRAM_ATTR timer_group0_isr(void *arg);
void IRAM_ATTR timer_group1_isr(void* arg);

// TIMER_BASE_CLK should be 80 MHz, configure timer to tick at 1 MHz
//#define TIMER_DIVIDER 80
//#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER)
#define TIMER_SCALE 1000000

typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
} timer_info;


typedef struct {
    int timer_group;
    int timer_idx;
} timer_event;

typedef struct {
    uint64_t event_count;
} example_queue_element_t;

extern SemaphoreHandle_t s_timer_semaphore;

#endif 
