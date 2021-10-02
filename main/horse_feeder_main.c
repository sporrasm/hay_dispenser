#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include "esp_log.h"
#include "wifi_connect.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/i2c.h>
#include "timer_handler.h"
#include "ntp_client.h"
#include "gpio_funcs.h"
#include "AIP31068L.h"

#define T_OFFS 7*15//24*3600 // Time in seconds to offset the array values once it has been looped over
#define LCD_ADDR 0x3e
#define SDA_PIN  19
#define SCL_PIN  18
#define LCD_COLS 16 
#define LCD_ROWS 2
#define NUM_LOCKS 6 // Number of locks in dispenser array
#define LOCK_MS 500 // Time to pulse the lock in milliseconds

void LCD_updater(void* param);
void pulseLock(void* param);
void updateAlarm(void* param);
void updateScreenIdx(void* param);
//void writeIdxToFlash(void* param);
// Global definition for timer semaphore
SemaphoreHandle_t s_timer_semaphore;
// Global definition for timer update queue (used for sending next alarm value between tasks)
QueueHandle_t update_queue;
// Global definition for screen update queue (used for sending screen idx and mode between tasks)
QueueHandle_t screen_queue;

//QueueHandle_t DPD_event_queue;
//Global definition for button pin interrupt variables:
volatile uint32_t lnum_edges=0;
volatile uint32_t rnum_edges=0;
volatile uint32_t cnum_edges=0;
volatile uint32_t lstate=0;
volatile uint32_t rstate=0;
volatile uint32_t cstate=0;
volatile uint32_t ldebounce_tick=0;
volatile uint32_t rdebounce_tick=0;
volatile uint32_t cdebounce_tick=0;

portMUX_TYPE lmux=portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE rmux=portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE cmux=portMUX_INITIALIZER_UNLOCKED;

static const char* TAG="MAIN";
static const char* TAG_LOCK="PulseLock";
static const char* TAG_ALARM="UpdateAlarm";

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
  // TZ variable for Finland
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

void app_main(void)
{
  // The parameter to strptime must given as hour (0-23) and
  //char* times[NUM_LOCKS] = {"01:36:00", "02:30:00", "04:30:00", "06:30:00", "08:30:00", "10:30:00"};
  char* times[NUM_LOCKS] = {"18:25:00", "18:25:15", "18:25:30", "18:25:45", "18:26:00", "18:26:15"};
  // Table mapping lock indices to GPIO pin numbers
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
    ESP_LOGI(TAG, "Restarting in 5 seconds ...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    esp_restart();
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
    sort_time(arr, sizeof(times) / sizeof(times[0])); 
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
    int* lock_idx = pvPortMalloc(sizeof(int));
    int* time_idx = pvPortMalloc(sizeof(int));
    *lock_idx=0;
    *time_idx=0;
    
    s_timer_semaphore = xSemaphoreCreateBinary();
    update_queue = xQueueCreate(4, sizeof(time_t));
    screen_queue = xQueueCreate(4, sizeof(int));
//    DPD_event_queue = xQueueCreate(4, sizeof(int));
    if (s_timer_semaphore == NULL) {
      ESP_LOGW(TAG, "Semaphore creation failed, restarting in 5 s...");
      vTaskDelay(pdMS_TO_TICKS(5000));
      esp_restart();
    }
    if (update_queue == NULL) {
      ESP_LOGW(TAG, "Queue creation failed, restarting in 5 s...");
      vTaskDelay(pdMS_TO_TICKS(5000));
      esp_restart();
    }
    if (screen_queue == NULL) {
      ESP_LOGW(TAG, "Queue creation failed, restarting in 5 s...");
      vTaskDelay(pdMS_TO_TICKS(5000));
      esp_restart();
    }
    //if (DPD_event_queue == NULL) {
    //  ESP_LOGW(TAG, "Queue creation failed, restarting in 5 s...");
    //  vTaskDelay(pdMS_TO_TICKS(5000));
    //  esp_restart();
    //}
    for (unsigned int i = 0; i<6; i++) {
      printf("Time value at arr[%d] = %ld\n", i, arr[i]);
    }

    time_t now = time(NULL);
    time_t diff = *arr - now;
    if (diff > 0) {
      ini_timer(0,0, diff);
    }
    else {
      ESP_LOGW(TAG, "Time diff was negative! (%ld seconds)", diff);
    }

    if (lock_pin_init() != 0) {
      ESP_LOGW(TAG, "Lock GPIO init failed, invalid args!");
    }
    if (button_pin_init() != 0) {
      ESP_LOGW(TAG, "Button GPIO init failed, invalid args!");
    }
    xTaskCreate(&LCD_updater, "LCD_updater", 2048, NULL, 5, NULL);
    xTaskCreate(&pulseLock , "pulseLock", 2048, arr, 5, NULL);
    xTaskCreate(&updateAlarm, "updateAlarm", 2048, NULL, 5, NULL);
    xTaskCreate(&updateScreenIdx, "updateScreenIdx", 2048, NULL, 5, NULL);
    //xTaskCreate(&writeIdxToFlash, "writeFlash", 2048, NULL, 6, NULL);
  }
}

//void writeIdxToFlash(void* param) {
//  int idx = 0;
//  for (;;) {
//    xQueueReceive(DPD_event_queue, &idx, portMAX_DELAY);
//    ESP_LOGI(TAG, "Power down detected!");
//    ESP_LOGI(TAG, "Writing idx %d to flash!", idx);
//  }
//}

void updateAlarm(void* param) {
  time_t val = 0;
  for (;;) {
    xQueueReceive(update_queue, &val, portMAX_DELAY);
    if (val > 0) {
      ESP_LOGI(TAG_ALARM, "Setting timer to %ld seconds", val);
      ESP_ERROR_CHECK(timer_set_counter_value(0,0,0));
      ESP_ERROR_CHECK(timer_set_alarm_value(0,0, ((uint64_t) val)*((uint64_t)TIMER_SCALE)));
      ESP_ERROR_CHECK(timer_set_alarm(0,0,1));
    }
    else {
      ESP_LOGW(TAG_ALARM,"Invalid time value for alarm: %ld seconds!!", val);
    }
  }
}

void updateScreenIdx(void* param) {
  int screen_idx=0;
  int curr_state=0;
  int num_edges=0;
  int last_state=0;
  int debounce_time=0;
  for (;;) {
    // Ensure that ISR doesn't write to variables
    portENTER_CRITICAL_ISR(&lmux);
    // Read the last status from button
    num_edges=lnum_edges;
    debounce_time=ldebounce_tick;
    last_state=lstate;
    portEXIT_CRITICAL_ISR(&lmux);
    // Read the current state of the button
    curr_state=gpio_get_level(LEFT_PIN);
    // Debounce switch in software
    if ((num_edges != 0) && (curr_state==last_state) && ((xTaskGetTickCount() - debounce_time) > pdMS_TO_TICKS(200))) {
      screen_idx--;
      ESP_LOGI(TAG, "Screen idx is now: %d", screen_idx);
      if (xQueueSend(screen_queue, (void *) &screen_idx, pdMS_TO_TICKS(100) ) != pdPASS) {
        ESP_LOGW(TAG, "Failed to update screen_idx");
      }
      // Reset edge counter
      portENTER_CRITICAL_ISR(&lmux);
      lnum_edges=0;
      portEXIT_CRITICAL_ISR(&lmux);
      vTaskDelay(pdMS_TO_TICKS(10));
  } 
  else {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
    // Do same thing for the right button
    portENTER_CRITICAL_ISR(&rmux);
    // Read the last status from button
    num_edges=rnum_edges;
    debounce_time=rdebounce_tick;
    last_state=rstate;
    portEXIT_CRITICAL_ISR(&rmux);
    // Read the current state of the button
    curr_state=gpio_get_level(LEFT_PIN);
    // Debounce switch in software
    if ((num_edges != 0) && (curr_state==last_state) && ((xTaskGetTickCount() - debounce_time) > pdMS_TO_TICKS(200))) {
      screen_idx++;
      ESP_LOGI(TAG, "Screen idx is now: %d", screen_idx);
      if (xQueueSend(screen_queue, (void *) &screen_idx, pdMS_TO_TICKS(100) ) != pdPASS) {
        ESP_LOGW(TAG, "Failed to update screen_idx");
      }
      // Reset edge counter
      portENTER_CRITICAL_ISR(&rmux);
      rnum_edges=0;
      portEXIT_CRITICAL_ISR(&rmux);
      vTaskDelay(pdMS_TO_TICKS(10));
    } 
    else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

void pulseLock(void* param) {
  /*
    This function is responsible for pulsing the lock when the hardware
    timer alarm is triggered. Pulsing the lock will dispense one portion
    of hay. If the power goes out for an undetermined amount of time,
    all locks with time stamps smaller than the current time should also
    be pulsed.
    
    the strategy to determine
    which lock will be pulsed will be as follows:

    1. Parameter time_arr is to given as non-sorted version of the array
    2. Sort the array, first element should be closest to the next lock release
    3. Associate lock_idx = 0 with this time, this assures 
  */
  time_t* t_arr = (time_t*) param;
  time_t now = 0, diff = 0;
  int lock_idx = 0;
  int idxToPin[NUM_LOCKS] = { LOCK_GPIO_0, LOCK_GPIO_1, LOCK_GPIO_2, LOCK_GPIO_3, LOCK_GPIO_4, LOCK_GPIO_5};

  for (;;) {
    // Wait until ISR gives semaphore, increment lock_idx counter
    xSemaphoreTake(s_timer_semaphore, portMAX_DELAY);
    ESP_LOGI(TAG_LOCK, "INTERRUPT FROM TIMER");
    ESP_LOGI(TAG_LOCK, "RELEASING LOCK ON GPIO IDX: %d", *(idxToPin+lock_idx));
    gpio_set_level(*(idxToPin+lock_idx), 1); // MOSFET drivers are active high 
    vTaskDelay(pdMS_TO_TICKS(LOCK_MS));
    gpio_set_level(*(idxToPin+lock_idx), 0);
    // Increment lock idx, loop back to zero if over range
    if (lock_idx >= 5) {
      lock_idx=0; 
    }
    else {
      lock_idx++;
    }
    now = time(NULL);
    diff = *(t_arr+lock_idx) - now;
    if (diff > 0) {
      if (xQueueSend(update_queue, (void *) &diff, pdMS_TO_TICKS(100) ) != pdPASS) {
        ESP_LOGW(TAG_LOCK, "FAILED TO UPDATE ALARM!");
      }
    } 
    else {
      ESP_LOGI(TAG_LOCK, "Time diff was negative! (%ld seconds)", diff);
      ESP_LOGI(TAG_LOCK, "Increasing time_arr values by one day");
      for (uint8_t i = 0; i < NUM_LOCKS; i++) {
        *(t_arr+i) += T_OFFS;
      }
      sort_time(t_arr, NUM_LOCKS);
      now=time(NULL);
      diff = *(t_arr+lock_idx) - now;
      if (xQueueSend(update_queue, (void *) &diff, pdMS_TO_TICKS(100) ) != pdPASS) {
        ESP_LOGW(TAG_LOCK, "FAILED TO UPDATE ALARM!");
      }
    }
  }
}

void LCD_updater(void* param)
/*
   This task should run every 60 seconds or so
   to update the time reading on the LCD screen
   TODO: Add a queue/semaphore to be given by ISR.
   Based on that, increment/decrement screen idx
   and display alarm times on the LCD screen.
*/
{
  char txtBuf[16];
  int screen_idx=0;
  time_t now;
  struct tm timeinfo;
  TickType_t xLastWakeTime;
  // Calculate ticks until next full minute
  TickType_t xFreq = 0;
  xLastWakeTime = xTaskGetTickCount();
  while (1) {
    // Wait until it's time to update or we receive index from queue
    now=time(NULL);
    xFreq = pdMS_TO_TICKS((60 - (now - (now / 60)*60))*(1000));
    ESP_LOGI(TAG, "Waiting %d milliseconds until next screen refresh", xFreq);
    if (xQueueReceive(screen_queue, &screen_idx, xFreq) == pdFALSE) {
      if (screen_idx == 0) {
        vTaskDelayUntil(&xLastWakeTime, xFreq);
        LCD_setCursor(0, 1);
        now=time(NULL);
        localtime_r(&now, &timeinfo);
        sprintf(txtBuf, "TIME: %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        LCD_writeStr(txtBuf);
      }
    } 
    // Received something from queue, display alarm times
    else {
      switch (screen_idx) {
        case 1:
          break;
        case 2:
          break;
        case 3:
          break;
        case 4:
          break;
        case 5:
          break;
        case 6:
          break;
        default:
          screen_idx=0;
          break;
      } 
    }
  }
}
