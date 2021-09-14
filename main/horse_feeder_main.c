#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "wifi_connect.h"
#include "ntp_client.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/i2c.h>
#include "AIP31068L.h"

#define LCD_ADDR 0x3e
#define SDA_PIN  19
#define SCL_PIN  18
#define LCD_COLS 16 
#define LCD_ROWS

void LCD_updater(void* param);
time_t* timeFromString();

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
    char* times[6] = {"09:00", "12:00", "15:00", "18:00", "21:00", "00:00"};
    int init_stat=init_horse_feeder();
    if (init_stat != 0) {
      ESP_LOGW(TAG, "Initialization failed!");
    }
    init_LCD();
    int wifi_conn_stat=0;
    printf("Initializing WiFi\n");
    wifi_conn_stat=conn_wifi();
    if (wifi_conn_stat != 0) {
        printf("Connection failed!\n");
        printf("Please restart uC!\n");
    }   
    else {
        printf("Connected to WiFi\n");
        printf("Getting current time\n");
        time_t currtime = get_time();
        ESP_LOGI(TAG, "Setting system time to %s", ctime(&currtime));
        struct timeval tv;
        tv.tv_sec=currtime;
        settimeofday(&tv, NULL);
        // Apparently thse need be called before calling time() in a task
        struct tm curr_t;
        time_t now=time(NULL);
        if (localtime_r(&now, &curr_t) == NULL)
          printf("localtime_r failed!\n");
        // Init LCD, start task to update time
        LCD_home();
        LCD_clearScreen();
        LCD_writeStr("PonyFeeder 3000");
        LCD_setCursor(0, 1);
        xTaskCreate(&LCD_updater, "LCD_updater", 2048, NULL, 5, NULL);
        while (1) {
          time_t* arr = timeFromString();
          vTaskDelay(5000 / portTICK_RATE_MS);
        }
    }
}

time_t* timeFromString() {
    char* times[] = {"23:00", "00:00"};
    time_t time_arr[6] = { 0 }; // Array for storing outputs in ctime
    struct tm currtime,desttime;
    memset(&currtime, 0, sizeof(struct tm));
    time_t now = time(NULL);
    if (localtime_r(&now, &currtime) == NULL) {
      printf("localtime_r failed\n");
      return NULL;
    }
    // Process strings 
    //char* datebuf=malloc(sizeof(char)*16);
    char datebuf[80] = { 0 };
    for (int i=0; i<sizeof(times) / sizeof(times[0]); i++) {
      memset(&desttime, 0, sizeof(struct tm));
      sprintf(datebuf, "%04d %02d %02d %s", currtime.tm_year+1900,
              currtime.tm_mon+1, currtime.tm_mday,
              times[i]);
      printf("Time set: %s\n", datebuf);
      if (strptime(datebuf, "%Y %m %d %H:%M ", &desttime) == NULL) {
        printf("Strptime failed!\n");
        return NULL;
      }
      else {
        desttime.tm_isdst=currtime.tm_isdst;
        time_t tstamp = mktime(&desttime);
        printf("Time: %s\n", ctime(tstamp));
        time_arr[0]=tstamp;
      }
    }
    return time_arr;
}

//void activateFeeder(void* param) {
//  time_t now = time(NULL);
//  double seconds = difftime(now, mktime()
//
//  //while (1) {
//  //xEventGroupWaitBits(eg,   
//
//  //}
//}

void LCD_updater(void* param)
{
    char txtBuf[16];
    while (1) {
          time_t now = time(NULL);
          struct tm timeinfo;
          localtime_r(&now, &timeinfo);
          //printf("Time with localtime() %02d:%02d:%02d\n",timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
          sprintf(txtBuf, "TIME: %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
          LCD_writeStr(txtBuf);
          LCD_setCursor(0, 1);
          vTaskDelay(10000 / portTICK_RATE_MS);
        }
}
