#include "timer_handler.h"

void ini_timer(int group, int timer, int time_interval) {
  /*
      This function is used to initialize a hardware timer. The timer will
      trigger an interrupt, which in turn will trigger the next lock to
      be pulsed and dispense the hay.

      Parameters:
      int group:
        Timer group, either 0 or 1
      int timer
        Timer index, either 0 or 1
      int time_interval 
        Time until alarm sets off (in seconds)

  */

  // Initialize timer struct, contants defined in hal/include/hal/timer_types.h
  timer_config_t config = {
    .divider = TIMER_DIVIDER,
    .alarm_en = TIMER_ALARM_EN,
    .counter_en = TIMER_PAUSE,
    .counter_dir = TIMER_COUNT_UP,
    .auto_reload = TIMER_AUTORELOAD_DIS,
    .intr_type = TIMER_INTR_LEVEL, // Note, mandatory in alarm mode
  };
  ESP_ERROR_CHECK(timer_init(group, timer, &config));
  ESP_ERROR_CHECK(timer_set_counter_value(group, timer, 0x00000000ULL));
  int timer_value = time_interval*TIMER_SCALE;
  ESP_ERROR_CHECK(timer_set_alarm_value(group, timer, timer_value));
  ESP_ERROR_CHECK(timer_enable_intr(group, timer));

  if (!group) {
    ESP_ERROR_CHECK(timer_isr_register(TIMER_GROUP_0, timer, timer_group0_isr,
        (void *) timer, ESP_INTR_FLAG_IRAM, NULL));
  }
  else {
    ESP_ERROR_CHECK(timer_isr_register(TIMER_GROUP_1, timer, timer_group1_isr,
        (void *) timer, ESP_INTR_FLAG_IRAM, NULL));
  }
  //ESP_ERROR_CHECK(timer_pause(group, timer));
  //ESP_ERROR_CHECK(timer_set_counter_value(group, timer, 0x00000000ULL));
  ESP_ERROR_CHECK(timer_start(group, timer));
}

void IRAM_ATTR timer_group0_isr(void *arg)
{
    int need_yield;

    int timer_idx = (int) arg;

    /* Retrieve the interrupt status and the counter value from the timer that reported the interrupt */
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    TIMERG0.hw_timer[timer_idx].update = 1;

    /* Clear the interrupt (either Timer 0 or 1, whichever caused this interrupt */
    if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0) {
        TIMERG0.int_clr_timers.t0 = 1;
    } else if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_1) {
        TIMERG0.int_clr_timers.t1 = 1;
    }

    /* After the alarm has been triggered we need enable it again, so it is triggered the next time */
    //TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_DIS;

    // Give a semaphore to the Simulink task, so that it wakes up and executes 1 loop
    if (xSemaphoreGiveFromISR(s_timer_semaphore, &need_yield) != pdPASS)
    {
        ESP_EARLY_LOGD("timer_group0_isr", "timer queue overflow!");

    	return;
    }

    if (need_yield == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }

}

void IRAM_ATTR timer_group1_isr(void *arg)
{
    int need_yield;

    int timer_idx = (int) arg;

    /* Retrieve the interrupt status and the counter value from the timer that reported the interrupt */
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    TIMERG0.hw_timer[timer_idx].update = 1;

    /* Clear the interrupt (either Timer 0 or 1, whichever caused this interrupt */
    if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0) {
        TIMERG0.int_clr_timers.t0 = 1;
    } else if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_1) {
        TIMERG0.int_clr_timers.t1 = 1;
    }

    // Try to give a semaphore to the pulseLock task 
    if (xSemaphoreGiveFromISR(s_timer_semaphore, &need_yield) != pdPASS)
    {
        ESP_EARLY_LOGD("timer_group1_isr", "timer queue overflow!");

    	return;
    }

    if (need_yield == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }

}

time_t* timeFromString(char** times, unsigned int len) {
  time_t* time_arr = malloc((len+1)*sizeof(time_t));; // Array for storing output times (+1 for null)
  const char* TAG_TFS = "TimeFromString";
  char datebuf[30] = { '\0' };
  if (time_arr==NULL) {
    ESP_LOGW(TAG_TFS,"Malloc failed");
    return NULL;
  }
  struct tm currtime,desttime;
  memset(time_arr, 0, (len+1)*sizeof(time_t));
  memset(&currtime, 0, sizeof(struct tm));
  time_t now = time(NULL);
  if (localtime_r(&now, &currtime) == NULL) {
    ESP_LOGW(TAG_TFS,"localtime_r failed");
    return NULL;
  }
  // Process strings 
  ESP_LOGI(TAG_TFS, "%d", len);
  for (int i=0; i<len+1; i++) {
      if (i == len) {
        time_arr[i]=0;
      }
      else {
        memset(&desttime, 0, sizeof(struct tm));
        int ret = sprintf(datebuf, "%04d %02d %02d %s", currtime.tm_year+1900,
                currtime.tm_mon+1, currtime.tm_mday,
                times[i]);
        ESP_LOGI(TAG_TFS, "%s", datebuf);
        ESP_LOGI(TAG_TFS, "%s", times[i]);
        if (ret != strlen(datebuf)) {
          ESP_LOGW(TAG_TFS,"sprintf failed");
        }
        if (strptime(datebuf, "%Y %m %d %H:%M ", &desttime) == NULL) {
          ESP_LOGW(TAG_TFS,"strptime failed");
        }
        if (strptime(times[i], "%H:%M", &desttime) == NULL) {
          ESP_LOGW(TAG_TFS,"strptime failed");
        }
        else {
          desttime.tm_isdst=currtime.tm_isdst;
          if (!(strncmp(times[i],"00:00", 5))) {
            desttime.tm_mday++;
          }
          time_t tstamp = mktime(&desttime);
          double diff = difftime(tstamp, now);
          printf("Tstamp: %s", ctime(&tstamp));
          printf("Tnow: %s", ctime(&now));
          printf("%.2f\n", diff);
          if (diff < 0) {
            desttime.tm_mday++;
            tstamp = mktime(&desttime);
          }
          time_arr[i]=tstamp;
        }
      }
  }
  return time_arr;
}


