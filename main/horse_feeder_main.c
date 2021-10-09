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
#define STRLEN 8 // Length of one time stamp string, not counting the null byte
#define LOCK_MS 500 // Time to pulse the lock in milliseconds

void LCD_updater(void* param);
void pulseLock(void* param);
void updateAlarm(void* param);
void updateScreenIdxLeft(void* param);
void updateScreenIdxRight(void* param);
void updateSelectMode(void* param);
//void writeIdxToFlash(void* param);
// Global definition for timer semaphore
SemaphoreHandle_t s_timer_semaphore;
// Global definition for timer update queue (used for sending next alarm value between tasks)
QueueHandle_t update_queue;
// Global definition for screen update queue (used for sending screen idx and mode between tasks)
QueueHandle_t screen_queue;
// Used for sending information about the mode flag:
QueueHandle_t select_mode_queue;


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
portMUX_TYPE screen_mux=portMUX_INITIALIZER_UNLOCKED;

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
  //LCD_quick_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);
  struct LCD_config lcd_conf;
  lcd_conf.bitmode_flag=1;
  lcd_conf.two_line_flag=1;
  lcd_conf.display_flag=0;
  lcd_conf.pixel_flag=1;
  lcd_conf.cursor_flag=0;
  lcd_conf.blink_flag=0;
  lcd_conf.entrymode_flag=1;
  lcd_conf.shiftmode_flag=0;

  LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, &lcd_conf);
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
    // Create array of integers for the screen flags
    // We want to pass them as pointers to tasks, so
    // they get updated in each task properly
    // There are a total of three
    // 1. Screen index == which "page" is displayed on screen
    // 2. Select index == cursor position when setting the alarm
    // 3. Mode flag == controls the behaviour of L/R buttons
    int* screen_flags = pvPortMalloc(3*sizeof(int));
    screen_flags[0] = 0;
    screen_flags[1] = 0;
    screen_flags[2] = 0;
    
    s_timer_semaphore = xSemaphoreCreateBinary();
    update_queue = xQueueCreate(4, sizeof(time_t));
    screen_queue = xQueueCreate(4, sizeof(int));
    select_mode_queue = xQueueCreate(4, sizeof(int));
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
    if (select_mode_queue == NULL) {
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

    char** time_arr = pvPortMalloc(NUM_LOCKS * sizeof(char*));
    for (uint8_t i = 0; i < NUM_LOCKS; i++) {
      time_arr[i] = pvPortMalloc((STRLEN+1)*sizeof(char));
      if (strcpy(time_arr[i], times[i]) != NULL) {
        ESP_LOGI(TAG,"Succesfully dynamically allocated. Array element at idx %d is %s", i, time_arr[i]);
      }
    }
    xTaskCreate(&LCD_updater, "LCD_updater", 2048, time_arr, 5, NULL);
    xTaskCreate(&pulseLock , "pulseLock", 2048, arr, 5, NULL);
    xTaskCreate(&updateAlarm, "updateAlarm", 2048, NULL, 5, NULL);
    xTaskCreate(&updateScreenIdxLeft, "updateScreenIdxLeft", 2048, screen_flags, 5, NULL);
    xTaskCreate(&updateScreenIdxRight, "updateScreenIdxRight", 2048, screen_flags, 5, NULL);
    int* mode_flag=&screen_flags[2];
    xTaskCreate(&updateSelectMode, "updateSelectMode", 2048, mode_flag, 5, NULL);
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

void updateScreenIdxLeft(void* param) {
  int* screen_flags = (int*) param;
  int* screen_idx = &screen_flags[0];
  int* select_idx = &screen_flags[1]; 
  int* mode_flag = &screen_flags[2]; 
  int curr_state=0;
  int num_edges=0;
  int last_state=0;
  int idx=0;
  int debounce_time=0;
  for (;;) {
    // Ensure that ISR doesn't write to variables
    portENTER_CRITICAL(&lmux);
    // Read the last status from button
    num_edges=lnum_edges;
    debounce_time=ldebounce_tick;
    last_state=lstate;
    portEXIT_CRITICAL(&lmux);
    // Read the current state of the button
    curr_state=gpio_get_level(LEFT_PIN);
    // Debounce switch in software
    if ((num_edges != 0) && (curr_state==last_state) && \
        ((xTaskGetTickCount() - debounce_time) > pdMS_TO_TICKS(DEBOUNCE_MS))) {
      if (!(*mode_flag)) { // Mode flag was zero
        (*screen_idx)--;
        if (*screen_idx<0) {
            *screen_idx=6;
          }
          idx=*screen_idx;
          ESP_LOGI(TAG, "screen idx is now: %d", idx);
        }
      else { // Mode flag was non-zero
        (*select_idx)--;
        if (*select_idx < 0) {
          *select_idx=4;
        }
        else if (*select_idx == 2) {
          (*select_idx)--; // Jump over the ":" character
        }
        idx=*select_idx;
        ESP_LOGI(TAG, "select idx is now: %d", idx);
        }
      if (xQueueSend(screen_queue, (void *) &idx, pdMS_TO_TICKS(100) ) != pdPASS) {
        ESP_LOGW(TAG, "Failed to update screen_idx");
      }
      // Reset edge counter
      portENTER_CRITICAL(&lmux);
      lnum_edges=0;
      portEXIT_CRITICAL(&lmux);
      vTaskDelay(pdMS_TO_TICKS(10));
    } 
    else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

void updateSelectMode(void* param) {
  int* mode_flag = (int*) param;
  int num_edges=0;
  int debounce_time=0;
  int last_state=0;
  int curr_state=0;
  for(;;) {
    portENTER_CRITICAL(&cmux);
    num_edges=cnum_edges;
    debounce_time=cdebounce_tick;
    last_state=cstate;
    portEXIT_CRITICAL(&cmux);
    curr_state=gpio_get_level(CENTER_PIN);
    if ((num_edges != 0) && (curr_state==last_state) && \
        ((xTaskGetTickCount() - debounce_time) > pdMS_TO_TICKS(DEBOUNCE_MS))) {
      // if the button is registered as pushed, toggle between the modes
      *mode_flag = (*mode_flag == 0) ? 1 : 0;
      //ESP_LOGI(TAG, "Mode flag toggled! Is now: %d", *mode_flag);
      int mode = *mode_flag;
      xQueueSend(select_mode_queue, (void *) &mode, portMAX_DELAY);
      portENTER_CRITICAL(&cmux);
      cnum_edges=0;
      portEXIT_CRITICAL(&cmux);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

void updateScreenIdxRight(void* param) {
  int* screen_flags = (int*) param;
  int* screen_idx = &screen_flags[0];
  int* select_idx = &screen_flags[1]; 
  int* mode_flag = &screen_flags[2]; 
  int curr_state=0;
  int num_edges=0;
  int last_state=0;
  int debounce_time=0;
  int idx=0;
  for (;;) {
    // Ensure that ISR doesn't write to variables
    portENTER_CRITICAL(&rmux);
    // Read the last status from button
    num_edges=rnum_edges;
    debounce_time=rdebounce_tick;
    last_state=rstate;
    portEXIT_CRITICAL(&rmux);
    // Read the current state of the button
    curr_state=gpio_get_level(LEFT_PIN);
    // Debounce switch in software
    if ((num_edges != 0) && (curr_state==last_state) && ((xTaskGetTickCount() - debounce_time) > pdMS_TO_TICKS(DEBOUNCE_MS))) {
      if (!(*mode_flag)) { // Mode flag was zero
        (*screen_idx)++;
        if (*screen_idx>6) {
            *screen_idx=0;
          }
          idx=*screen_idx;
          ESP_LOGI(TAG, "screen idx is now: %d", idx);
        }
      else { // Mode flag was non-zero
        (*select_idx)++;
        if (*select_idx > 4) {
          *select_idx=0;
        }
        else if (*select_idx == 2) {
          (*select_idx)++; // Jump over the ":" character
        }
        idx=*select_idx;
        ESP_LOGI(TAG, "select idx is now: %d", idx);
        }
      if (xQueueSend(screen_queue, (void *) &idx, pdMS_TO_TICKS(100) ) != pdPASS) {
        ESP_LOGW(TAG, "Failed to update screen_idx");
      }
      // Reset edge counter
      portENTER_CRITICAL(&rmux);
      rnum_edges=0;
      portEXIT_CRITICAL(&rmux);
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

void LCD_print(int screen_idx, int mode_flag, char* timestamp) {
  time_t now;
  struct tm timeinfo;
  char txtbuf[20] = {0};
  ESP_LOGI(TAG, "Went to print");
  ESP_LOGI(TAG, "mode_flag is %d", mode_flag);
  ESP_LOGI(TAG, "str is %s", timestamp);
  if (mode_flag==0) {
    if (screen_idx == 0) {
      LCD_clearScreen();
      LCD_setCursor(0,0);
      LCD_writeStr("PonyFeeder 3000");
      LCD_setCursor(0, 1);
      now=time(NULL);
      localtime_r(&now, &timeinfo);
      sprintf(txtbuf, "TIME: %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
      LCD_writeStr(txtbuf);
    }
    else {
      LCD_clearScreen();
      LCD_setCursor(0,0);
      LCD_writeStr("Alarm time for");
      LCD_setCursor(0,1);
      sprintf(txtbuf, "LOCK%d: %s", screen_idx, timestamp);
      LCD_writeStr(txtbuf);
    }
  }
  else {
    if (screen_idx==0) {
      screen_idx++; // display first alarm time
    }
    LCD_clearScreen();
    LCD_setCursor(0,1);
    LCD_writeStr("      ^    ");
    LCD_setCursor(0,0);
    sprintf(txtbuf, "LOCK%d: %s", screen_idx, timestamp);
    LCD_writeStr(txtbuf);
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
  char** times= (char **) param;
  for (uint8_t i = 0; i < NUM_LOCKS; i++) {
    ESP_LOGI(TAG,"String at idx %d is %s", i, *(times+i));
  }
  char txtbuf[16];
  int screen_idx=0;
  int select_idx=0;
  int idx=0;
  int mode_changed=0;
  int mode_flag=0;
  int first=1;
  time_t now;
  struct tm timeinfo;
  TickType_t xLastWakeTime;
  // Calculate ticks until next full minute
  TickType_t xFreq = 0;
  xLastWakeTime = xTaskGetTickCount();
  while (1) {
    // Wait until it's time to update or we receive index from queue
    now=time(NULL);
    if (mode_changed) {
      xFreq=pdMS_TO_TICKS(10);
    }
    else {
      xFreq = pdMS_TO_TICKS((60 - (now - (now / 60)*60))*(1000));
    }
    if (xQueueReceive(screen_queue, &idx, xFreq) == pdFALSE) {
      if (xQueueReceive(select_mode_queue, &mode_flag, pdMS_TO_TICKS(10))==pdFALSE) {
        screen_idx=idx;
        mode_changed=0;
        first=1;
        if (screen_idx == 0) {
          vTaskDelayUntil(&xLastWakeTime, xFreq);
          LCD_clearScreen();
          LCD_setCursor(0,0);
          LCD_writeStr("PonyFeeder 3000");
          LCD_setCursor(0, 1);
          now=time(NULL);
          localtime_r(&now, &timeinfo);
          sprintf(txtbuf, "TIME: %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
          LCD_writeStr(txtbuf);
        }
      }
      else {
        mode_changed=1;
        first=1;
        select_idx=idx;
        ESP_LOGI(TAG, "Changed mode!");
      }
    } 
    // Received something from queue, display alarm times
    else {
      if (first) {
        xQueueReceive(select_mode_queue, &mode_flag, pdMS_TO_TICKS(10));
        if (mode_flag==0) {
          screen_idx=idx;
        }
        else {
          select_idx=idx;
          ESP_LOGI(TAG,"select_idx is: %d", select_idx);
          ESP_LOGI(TAG,"screen_idx is: %d", screen_idx);
          ESP_LOGI(TAG,"Mode_flag is: %d", mode_flag);
        }
        first=0;
        mode_changed=0;
        if (screen_idx>0) {
          LCD_print(screen_idx, mode_flag, times[screen_idx-1]);
        }
        else {
          LCD_print(screen_idx, mode_flag, times[0]);
        }
      }
      else if (xQueueReceive(select_mode_queue, &mode_flag, pdMS_TO_TICKS(10))==pdFALSE) {
        if (mode_flag==0) {
          screen_idx=idx;
        }
        else {
          select_idx=idx;
        }
        mode_changed=0;
        if (screen_idx>0) {
          LCD_print(screen_idx, mode_flag, times[screen_idx-1]);
        }
        else {
          LCD_print(screen_idx, mode_flag, times[0]);
        }
      }
      else {
        mode_changed=1;
        first=1;
        screen_idx=idx;
        ESP_LOGI(TAG, "Changed mode!");
      }
    }
  }
}
