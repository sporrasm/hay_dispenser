#include "timer_handler.h"
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
static const char* TAG_TIMER_INIT="TIMER_INIT";


time_t* timeFromString(char** times, unsigned int len) {
  /*
  Ideas: if power goes out in the middle of feeding routine,
  we need to pulse all locks before the current time to ensure
  feeding. Strategy:
  if current time is smaller than last time in the list,
  we need parse all timestamps before 00:00 as the previous
  day, and time stamps after 00:0 as current day
  if current time is larger than last time stamp in the list,
  we set all times to current day or the next, if time stamp
  is larger than or equal to midnight. 
  
  Other option: write lock idx to flash:
  https://www.esp32.com/viewtopic.php?t=6855
     */
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
        ESP_LOGI(TAG_TFS, "Constructed timestamp: %s from time: %s", datebuf, times[i]);
        if (ret != strlen(datebuf)) {
          ESP_LOGW(TAG_TFS,"sprintf failed");
        }
        if (strptime(datebuf, "%Y %m %d %H:%M:%S ", &desttime) == NULL) {
          ESP_LOGW(TAG_TFS,"strptime failed");
        }
        if (strptime(times[i], "%H:%M:%S", &desttime) == NULL) {
          ESP_LOGW(TAG_TFS,"strptime failed");
        }
        else {
          desttime.tm_isdst=currtime.tm_isdst;
          if (!(strncmp(times[i],"00:00", 5))) {
            desttime.tm_mday++;
          }
          time_t tstamp = mktime(&desttime);
          double diff = difftime(tstamp, now);
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

int comp_time(const void* elem1, const void* elem2) {
  int f = *((int *) elem1);
  int s = *((int *) elem2);
  if (f > s) return 1;
  if (f < s) return -1;
  return 0;
}

void sort_time(time_t* arr, int len) {
  char timestr[9];
  struct tm* time_info;
  qsort(arr, len, sizeof(*arr), comp_time);
  for (int i=0; i<len; i++) {
    ESP_LOGD("TAG_TIMER_INIT", "Sorted array[%d]: %lld", i, arr[i]);
    time_info=localtime(&arr[i]);
    strftime(timestr, sizeof(timestr), "%H:%M:%S", time_info);
    ESP_LOGD("TAG_TIMER_INIT", "Corresponding time stamp: %s", timestr);
  }
}

static bool IRAM_ATTR timer_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    // Give a semaphore to the pulseLock task, so that it wakes up and executes 1 loop
    gptimer_stop(timer);
    xSemaphoreGiveFromISR(s_timer_semaphore, &high_task_awoken);
    return (high_task_awoken == pdTRUE);
}

gptimer_handle_t ini_timer(uint64_t time_interval) {
  /*
      This function is used to initialize a hardware timer. The timer will
      trigger an interrupt, which in turn will trigger the next lock to
      be pulsed and dispense the hay.
      Parameters:
      uint64_t time_interval 
        Time until alarm sets off (in seconds)
      Returns:
  */
  // Configure timer

  gptimer_handle_t gptimer=NULL;
  gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1 * 1000 * 1000, 
  };
  // Create timer
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
  // Alarm config
  gptimer_alarm_config_t alarm_config = {
    .reload_count=0,
    .alarm_count=time_interval * TIMER_SCALE,
    .flags.auto_reload_on_alarm=true,
  };
  // Register alarm
  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
  // Configure ISR action
  gptimer_event_callbacks_t cbs = {
    .on_alarm = timer_isr,
  };
  // Register ISR
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
  return gptimer;
}

