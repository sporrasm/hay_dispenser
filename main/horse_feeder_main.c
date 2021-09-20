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
#include <driver/i2c.h>
#include "driver/gpio.h"
#include "timer.h"
#include "AIP31068L.h"

#define LCD_ADDR 0x3e
#define SDA_PIN  19
#define SCL_PIN  18
#define LCD_COLS 16 
#define LCD_ROWS 2
#define GPIO_OUTPUT_33 33
#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_33))

void LCD_updater(void* param);
void pulseLock(void* param);
// Global definition for timer semaphore
SemaphoreHandle_t s_timer_semaphore;

static const char* TAG="MAIN";

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
    s_timer_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(&LCD_updater, "LCD_updater", 2048, sec_tupdate, 5, NULL);
    xTaskCreate(&pulseLock , "pulseLock", 2048, lock_idx, 5, NULL);
    gpio_config_t io_conf;
    io_conf.intr_type=GPIO_INTR_DISABLE;
    io_conf.mode=GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask=GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en=0;
    io_conf.pull_up_en=0;
    gpio_config(&io_conf);
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
      gpio_set_level(GPIO_OUTPUT_33, 1);
    }
    else {
      (*lock_idx)++;
      gpio_set_level(GPIO_OUTPUT_33, 0);
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
