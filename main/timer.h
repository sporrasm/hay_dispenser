#ifndef _TIMER_FUNC_H
#define _TIMER_FUNC_H

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/timer.h"

time_t* timeFromString(char** times, unsigned int len);
void ini_timer(int group, int timer, int time_interval);
void sort_time(time_t* arr, int len);
int comp_time(const void* elem1, const void* elem2);

#define TIMER_DIVIDER 16
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER)


static const char* TAG_TIMER="TIMER_FUNC";

typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
} timer_info;


typedef struct {
    int timer_group;
    int timer_idx;
} timer_event;

void IRAM_ATTR timer_group0_isr(void *arg);
void IRAM_ATTR timer_group1_isr(void* arg);
void ini_timer(int group, int timer, int time_interval);

extern SemaphoreHandle_t s_timer_semaphore;

#endif
