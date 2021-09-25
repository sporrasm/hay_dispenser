#ifndef _TIMER_HANDLER_H_
#define _TIMER_HANDLER_H_

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "esp_log.h"
#include "driver/timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TIMER_DIVIDER 16
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER)

void ini_timer(int group, int timer, int time_interval);
time_t* timeFromString(char** times, unsigned int len);
void IRAM_ATTR timer_group0_isr(void* arg);
void IRAM_ATTR timer_group1_isr(void* arg);

typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
} timer_info;


typedef struct {
    int timer_group;
    int timer_idx;
} timer_event;

SemaphoreHandle_t s_timer_semaphore;

#endif 
