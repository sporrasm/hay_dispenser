#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "wifi_connect.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/i2c.h>
#include "timer_handler.h"
#include "ntp_client.h"
//#include "gpio_funcs.h"
#include "TCA6408A.h"
#include "AIP31068L.h"
//#include "wifi_logger.h"

#define LCD_ADDR 0x3e
#ifndef CONFIG_I2C_PULLUP_EN
#define PULLUP_EN 1
#else
#define PULLUP_EN 0
#endif
#define SDA_PIN CONFIG_SDA_PIN //19
#define SCL_PIN CONFIG_SCL_PIN //18
#define LCD_COLS 16 
#define LCD_ROWS 2
#define NUM_LOCKS 8// Number of locks in dispenser array
#define STRLEN 8 // Length of one time stamp string, not counting the null byte
#define LOCK_MS 500 // Time to pulse the lock in milliseconds
#define LOCK_OUTPUT_ADDR 0x21
#define LOCK_INPUT_ADDR 0x20

void LCD_updater(void* param);
void pulseLock(void* param);
void updateAlarm(void* param);
void updateScreenIdxLeft(void* param);
void updateScreenIdxRight(void* param);
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

int set_timezone(void) {
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

void init_i2c_interface(void)
{
  // Initialize i2c interface with esp32 in master mode
  i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = SDA_PIN,
    .scl_io_num = SCL_PIN,
    .sda_pullup_en = PULLUP_EN,
    .scl_pullup_en = PULLUP_EN,
    .master.clk_speed = 100000
  };

  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
}


void init_LCD(int i2c_inited) {
  ESP_LOGI(TAG, "Starting up LCD");
  struct LCD_config lcd_conf;
  lcd_conf.rows=LCD_ROWS;
  lcd_conf.cols=LCD_COLS;
  lcd_conf.bitmode_flag=1;
  lcd_conf.two_line_flag=1;
  lcd_conf.display_flag=0;
  lcd_conf.pixel_flag=1;
  lcd_conf.cursor_flag=0;
  lcd_conf.blink_flag=0;
  lcd_conf.entrymode_flag=1;
  lcd_conf.shiftmode_flag=0;

  LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, &lcd_conf, i2c_inited);
  LCD_clearScreen();
  LCD_home();
  LCD_writeStr("HorseFeeder 3000");
  LCD_setCursor(0, 1);
  LCD_writeStr("Initializing ...");
}

void app_main(void)
{
  // The parameter to strptime must given as hour (0-23) and minute (0-59)
  char* times[NUM_LOCKS] = {"21:00:00", "00:00:00", "03:00:00", "06:00:00", "09:00:00", "12:00:00",
    "15:00:00", "18:00:00"};
  // Table mapping lock indices to GPIO pin numbers
  char buf[6] = { 0 };
  int init_stat=set_timezone();
  if (init_stat != 0) {
    ESP_LOGW(TAG, "Initialization failed!");
  }
  init_i2c_interface();
  init_output_controllers(LOCK_OUTPUT_ADDR);
  int i2c_inited = 1;
#ifdef CONFIG_LCD_ENABLED
  ESP_LOGI(TAG, "LCD configured through menuconfig! Initializing LCD.");
  init_LCD(i2c_inited);
#endif
  int wifi_conn_stat=0;
  ESP_LOGI(TAG, "Initializing WiFi");
  wifi_conn_stat=conn_wifi();
  //start_wifi_logger();
  if (wifi_conn_stat != 0) {
    ESP_LOGW(TAG, "Connection failed!");
    ESP_LOGI(TAG, "Restarting in 5 seconds ...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    esp_restart();
  }   
  else {
    esp_log_level_set(TAG_LOCK, ESP_LOG_DEBUG);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    ESP_LOGI(TAG, "Connected to WiFi");
    ESP_LOGI(TAG, "Getting current time");
    time_t currtime = get_time();
    ESP_LOGI(TAG, "Setting system time to %s", ctime(&currtime));
    struct timeval tv;
    memset(&tv, 0, sizeof(tv)); // Initialize
    tv.tv_sec=currtime;
    settimeofday(&tv, NULL);

    time_t* arr = NULL;
#ifdef CONFIG_FEEDER_TESTMODE
    arr = malloc(sizeof(time_t) * (NUM_LOCKS+1));
    memset(arr, 0, (NUM_LOCKS+1)*sizeof(time_t));
    for (int i = 0; i < NUM_LOCKS; i++) {
      arr[i] = currtime + (i+1)*CONFIG_TESTMODE_INTERVAL;
    }
#else
    arr=timeFromString(times,sizeof(times)/sizeof(times[0]));
    time_t* arr_cp = NULL;
    arr_cp = malloc(sizeof(time_t) * (NUM_LOCKS+1));
    memset(arr_cp, 0, (NUM_LOCKS+1)*sizeof(time_t));
    arr_cp = timeFromString(times,sizeof(times)/sizeof(times[0]));
#endif
    sort_time(arr, sizeof(times) / sizeof(times[0])); 
#ifndef CONFIG_FEEDER_TESTMODE
    // Try obtain the current lock index corresponding to the index of the original time array
    int lock_idx=0;
    for (int i=0;i<NUM_LOCKS+1;i++) {
      ESP_LOGD(TAG, "Index %d", i);
      ESP_LOGD(TAG, "arr   : %lld", *(arr+i));
      ESP_LOGD(TAG, "arr_cp: %lld", *(arr_cp+i));
      if (*(arr) == *(arr_cp+i)) {
        lock_idx=i;
        break;
      }
    }
    ESP_LOGD(TAG, "The lock to be pulsed is: %d", lock_idx);
    free(arr_cp);
#endif
    //  start task to update time on LCD
#ifdef CONFIG_LCD_ENABLED
      LCD_home();
      LCD_clearScreen();
      LCD_writeStr("PonyFeeder 3000");
      LCD_setCursor(0, 1);
      if (strftime(buf, 6, "%H:%M", localtime(&currtime))!=0) {
        LCD_writeStr("TIME: ");
        LCD_setCursor(6, 1);
        LCD_writeStr(buf);
    }
      // Create array of strings for displaying the alarm times
      char** time_arr = pvPortMalloc(NUM_LOCKS * sizeof(char*));
      for (uint8_t i = 0; i < NUM_LOCKS; i++) {
        time_arr[i] = pvPortMalloc((STRLEN+1)*sizeof(char));
        if (strcpy(time_arr[i], times[i]) != NULL) {
          ESP_LOGD(TAG,"Succesfully dynamically allocated. Array element at idx %d is %s", i, time_arr[i]);
        }
      }
      // Create LCD updater task
      xTaskCreate(&LCD_updater, "LCD_updater", 2048, time_arr, 5, NULL);
#endif
    
    // Time semaphore and update queue
    s_timer_semaphore = xSemaphoreCreateBinary();
    if (s_timer_semaphore == NULL) {
      ESP_LOGW(TAG, "Semaphore creation failed, restarting in 5 s...");
      vTaskDelay(pdMS_TO_TICKS(5000));
      esp_restart();
    }
    update_queue = xQueueCreate(4, sizeof(time_t));
    if (update_queue == NULL) {
      ESP_LOGW(TAG, "Queue creation failed, restarting in 5 s...");
      vTaskDelay(pdMS_TO_TICKS(5000));
      esp_restart();
    }
    // Init timers
    time_t now = time(NULL);
    time_t diff = *arr - now;
    gptimer_handle_t timer_handle=NULL;
    if (diff > 0) {
      timer_handle = ini_timer(diff);
      ESP_ERROR_CHECK(gptimer_enable(timer_handle));
      ESP_ERROR_CHECK(gptimer_start(timer_handle));
    }
    else {
      ESP_LOGW(TAG, "Time diff was negative! (%lld seconds)", diff);
    }
    // Create tasks to run in background
    if (xTaskCreate(&pulseLock , "pulseLock", 2048, arr, 5, NULL) != pdPASS) {
      ESP_LOGW(TAG, "Couldn't create task for function pulseLock");
      esp_restart();
    }
    if (xTaskCreate(&updateAlarm, "updateAlarm", 2048, timer_handle, 5, NULL) != pdPASS) {
      ESP_LOGW(TAG, "Couldn't create task for function pulseLock");
      esp_restart();
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
  time_t now = 0, diff = 0, offs=0;
  int lock_idx = 0;
  int idxToPin[NUM_LOCKS] = {8,4,2,1,128,64,32,16};
#ifdef CONFIG_FEEDER_TESTMODE
  offs = CONFIG_TESTMODE_INTERVAL*CONFIG_NUM_LOCKS;
#else
  offs = 24*3600;
#endif
  for (;;) {
    // Wait until ISR gives semaphore, increment lock_idx counter
    ESP_LOGD(TAG_LOCK, "WAITING FOR INTERRUPT");
    xSemaphoreTake(s_timer_semaphore, portMAX_DELAY);
    ESP_LOGD(TAG_LOCK, "INTERRUPT FROM TIMER");
    ESP_LOGD(TAG_LOCK, "RELEASING LOCK WITH PORT MASK: %X", *(idxToPin+lock_idx));
    write_output(LOCK_OUTPUT_ADDR,*(idxToPin+lock_idx));
    vTaskDelay(pdMS_TO_TICKS(LOCK_MS));
    write_output(LOCK_OUTPUT_ADDR,0);
    // Increment lock idx, loop back to zero if over range
    ESP_LOGD(TAG_LOCK, "LOCK IDX WAS: %d, Incrementing.", lock_idx);
    if (lock_idx >= NUM_LOCKS-1) {
      lock_idx=0; 
      ESP_LOGD(TAG_LOCK, "LOCK IDX OVERRANGE, Looping back to zero");
    }
    else {
      lock_idx++;
    }
    ESP_LOGD(TAG_LOCK, "LOCK IDX IS: %d", lock_idx);
    now = time(NULL);
    diff = *(t_arr+lock_idx) - now;
    if (diff > 0) {
      if (xQueueSend(update_queue, (void *) &diff, pdMS_TO_TICKS(100) ) != pdPASS) {
        ESP_LOGW(TAG_LOCK, "FAILED TO UPDATE ALARM!");
      }
    } 
    else {
      ESP_LOGD(TAG_LOCK, "Time diff was negative! (%lld seconds)", diff);
      ESP_LOGD(TAG_LOCK, "Increasing time_arr values %lld seconds.", offs);
      for (uint8_t i = 0; i < NUM_LOCKS; i++) {
        *(t_arr+i) += offs;
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

void LCD_updater(void* param) {
/*
   This task should run every 60 seconds or so
   to update the time reading on the LCD screen
   TODO: Add a queue/semaphore to be given by ISR.
   Based on that, increment/decrement screen idx
   and display alarm times on the LCD screen.
*/
  char** times= (char **) param;
  for (uint8_t i = 0; i < NUM_LOCKS; i++) {
    ESP_LOGD(TAG,"String at idx %d is %s", i, *(times+i));
  }
  char txtbuf[16];
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
    ESP_LOGV(TAG, "Waiting %ld milliseconds until next screen refresh", xFreq);
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

void updateAlarm(void* param) {
  uint64_t val = 0;
  uint64_t curr_val = 0;
  gptimer_handle_t timer_handle = (gptimer_handle_t) param;
  while (1) {
    ESP_LOGD(TAG_ALARM, "Waiting on update_queue");
    xQueueReceive(update_queue, &val, portMAX_DELAY);
    ESP_LOGD(TAG_ALARM, "Received from update_queue: %lld", val);
    if (val > 0) {
      gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1000 * 1000 * val
      };
      ESP_ERROR_CHECK(gptimer_set_alarm_action(timer_handle, &alarm_config));
      ESP_LOGD(TAG_ALARM, "Setting timer to %lld seconds", val);
      ESP_ERROR_CHECK(gptimer_get_raw_count(timer_handle, &curr_val));
      ESP_LOGD(TAG_ALARM, "Current timer value was %lld microseconds", curr_val);
      if (curr_val != 0) {
        ESP_ERROR_CHECK(gptimer_set_raw_count(timer_handle, 0));
        ESP_ERROR_CHECK(gptimer_start(timer_handle));
      }
      else {
        ESP_LOGD(TAG_ALARM, "Timer already set to 0, starting.");
        ESP_ERROR_CHECK(gptimer_start(timer_handle));
      }
    }
    else {
      ESP_LOGW(TAG_ALARM,"Invalid time value for alarm: %lld seconds!!", val);
    }
  }
}
