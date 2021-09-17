#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "wifi_connect.h"
#include "ntp_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include <driver/i2c.h>
#include "AIP31068L.h"

#define TIMER_DIVIDER 16
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER)
#define LCD_ADDR 0x3e
#define SDA_PIN  19
#define SCL_PIN  18
#define LCD_COLS 16 
#define LCD_ROWS 2

void LCD_updater(void* param);
void pulseLock(void* param);
time_t* timeFromString(char** times, unsigned int len);
static void ini_timer(int group, int timer, int time_interval);
static xQueueHandle timer_queue;
SemaphoreHandle_t s_timer_semaphore;

static const char* TAG="MAIN";

typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
} timer_info;


typedef struct {
    int timer_group;
    int timer_idx;
} timer_event;

int set_tz(const char* targ_tz){
  int ret=setenv("TZ", targ_tz, 1);
  if (ret != 0) {
    ESP_LOGW(TAG, "Error setting TZ variable to %s! Time output will be wrong!!", targ_tz);
    return -1;
  }
  else {
    ESP_LOGI(TAG, "Time zone set to %s.", targ_tz);
    tzset(); // Set timezone from TZ env. variable
    return 0;
  }
}

int init_horse_feeder(void) {
  const char* targ_tz="EET-2EEST,M3.5.0/3,M10.5.0/4";
  // Check first if timezone is set
  char* env_tz=getenv("TZ");
  // If not, try to set
  if (env_tz == NULL) {    
    return set_tz(targ_tz);
  }
  // If it was set, check whether it was correct
  else {
    ESP_LOGI(TAG, "Time zone was %s", env_tz);
    if (strcmp(env_tz, targ_tz)==0) {
      return 0; 
    }
    else { // If not, try to set to correct one
      return set_tz(targ_tz);
    }
  }
}

void init_LCD() {
  ESP_LOGI(TAG, "Starting up LCD");
  LCD_quick_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);
  LCD_clearScreen();
  LCD_home();
  LCD_writeStr("HorseFeeder 3000");
  LCD_setCursor(0, 1);
  LCD_writeStr("Initializing ...");
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
        ESP_EARLY_LOGD("timer_group0_isr", "timer queue overflow!");

    	return;
    }

    if (need_yield == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }

}
  

static void ini_timer(int group, int timer, int time_interval) {
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
  
void app_main(void)
{
  // The parameter to strptime must given as hour (0-23) and
  char* times[6] = {"09:00", "12:00", "15:00", "18:00", "21:00", "00:00"};
  char buf[6] = { 0 };
  int init_stat=init_horse_feeder();
  if (init_stat != 0) {
    ESP_LOGW(TAG, "Initialization failed!");
  }
  init_LCD();
  int wifi_conn_stat=0;
  printf("Initializing WiFi\n");
  wifi_conn_stat=conn_wifi();
  if (wifi_conn_stat != 0) {
    ESP_LOGW(TAG, "Connection failed!");
  }   
  else {
    printf("Connected to WiFi\n");
    printf("Getting current time\n");
    time_t currtime = get_time();
    ESP_LOGI(TAG, "Setting system time to %s", ctime(&currtime));
    struct timeval tv;
    memset(&tv, 0, sizeof(tv)); // Initialize
    tv.tv_sec=currtime;
    settimeofday(&tv, NULL);
    time_t* arr=timeFromString(times, sizeof(times) / sizeof(times[0]));
    
    // Init LCD, start task to update time
    LCD_home();
    LCD_clearScreen();
    LCD_writeStr("PonyFeeder 3000");
    LCD_setCursor(0, 1);
    if (strftime(buf, 6, "%H:%M", localtime(&currtime))!=0) {
      LCD_writeStr("TIME: ");
      LCD_setCursor(6, 1);
      LCD_writeStr(buf);
    }
    int* sec_tupdate = pvPortMalloc(sizeof(int));
    *sec_tupdate = 60 - (currtime - (currtime / 60)*60);
    int num_array=sizeof(times)/sizeof(times[0]);
    int* lock_idx = pvPortMalloc(sizeof(int));
    *lock_idx=0;
    xTaskCreate(&LCD_updater, "LCD_updater", 2048, sec_tupdate, 5, NULL);
    xTaskCreate(&pulseLock , "pulseLock", 2048, lock_idx, 5, NULL);
    s_timer_semaphore = xSemaphoreCreateBinary();
    ini_timer(0,0, 1);
    int curr_idx = *lock_idx;
    while(1) {
      if ((curr_idx != *lock_idx)) {
        printf("Var changed (%d) \n", *lock_idx);
        curr_idx = *lock_idx;
        timer_set_counter_value(0,0,0);
        timer_set_alarm(0,0,1);
      }
      else {
        printf("Var not changed (%d) \n", *lock_idx);
        vTaskDelay(50);
      }
    }
  }
}

void pulseLock(void* param) {
  int* lock_idx = (int *) param;
  for (;;) {
    // Wait until ISR gives semaphore, increment lock_idx counter
    xSemaphoreTake(s_timer_semaphore, portMAX_DELAY);
    ESP_LOGI(TAG, "INTERRUPT FROM TIMER");
    ESP_LOGI(TAG, "RELEASING LOCK WITH IDX: %d", *lock_idx);
    if (*lock_idx == 5) {
      *lock_idx=0; 
    }
    else {
      (*lock_idx)++;
    }
  }
}

void LCD_updater(void* param)
/*
   This task should run every 60 seconds or so
   to update the time reading on the LCD screen
*/
{
    char txtBuf[16];
    int* sec_tupdate=(int *) param;
    TickType_t xLastWakeTime;
    const TickType_t xFreq = pdMS_TO_TICKS(60000);
    xLastWakeTime = xTaskGetTickCount();
    //ESP_LOGI(TAG, "Waiting %d seconds to sync with clock.", *sec_tupdate);
    vTaskDelay(pdMS_TO_TICKS(*(sec_tupdate)*1000));
    while (1) {
      vTaskDelayUntil(&xLastWakeTime, xFreq);
      LCD_setCursor(0, 1);
      time_t now = time(NULL);
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);
      sprintf(txtBuf, "TIME: %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
      LCD_writeStr(txtBuf);
    }
}
