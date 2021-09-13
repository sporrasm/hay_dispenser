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
#define LCD_ROWS 2

void LCD_updater(void* param);
void timeFromString();

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
    char txtBuf[16];
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
        LCD_home();
        LCD_clearScreen();
        LCD_writeStr("PonyFeeder 3000");
        LCD_setCursor(0, 1);
        const char times[] = "Mon Sep 13 10:00:00 2021";
        xTaskCreate(&LCD_updater, "LCD_updater", 2048, NULL, 5, NULL);
        while (1) {
          timeFromString();
          vTaskDelay(2000 / portTICK_RATE_MS);
        }
        //  vTaskDelay(10000 / portTICK_RATE_MS);
        //  //struct timeval curr;
        //  time_t now = time(NULL);
        //  struct tm timeinfo;
        //  localtime_r(&now, &timeinfo);
        //  printf("Time with localtime() %02d:%02d:%02d\n",timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        //  sprintf(txtBuf, "TIME: %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        //  LCD_writeStr(txtBuf);
        //  LCD_setCursor(0, 1);
        //}
    }
}

void timeFromString() {
    char* times[] = {"10:00", "12:00"};
    struct tm currtime,desttime;
    memset(&desttime, 0, sizeof(struct tm));
    memset(&currtime, 0, sizeof(struct tm));
    time_t now = time(NULL);
    if (localtime_r(&now, &currtime) == NULL) {
      printf("localtime_r failed\n");
      return;
    }
    // Process strings 
    //char* datebuf=malloc(sizeof(char)*16);
    char datebuf[80];
    for (int i=0; i<sizeof(times) / sizeof(times[0]); i++) {
      //memset(datebuf, 0, sizeof(*datebuf));
      sprintf(datebuf, "%04d %02d %02d %s", currtime.tm_year+1900, currtime.tm_mon+1, currtime.tm_mday, times[i] );
      printf("Time set: %s\n", datebuf);
      if (strptime(datebuf, "%Y %m %d %H:%M", &desttime) == NULL)
        printf("Strptime failed!\n");
      time_t ptr = mktime(&desttime);
      printf("Time: %s\n", ctime(&ptr));
    }
    //printf("Time set: %s\n", times);
    //if (strptime(times, "%a %b %d %H:%M:%S %Y", &time) == NULL)
    //  printf("Strptime failed!\n");
    //printf("tm_sec %d\n", time.tm_sec);
    //printf("tm_min %d\n", time.tm_min);
    //printf("tm_hour %d\n", time.tm_hour);
    //printf("tm_mday %d\n", time.tm_mday);
    //printf("tm_year %d\n", time.tm_year);
  //char *times[] = {"10:00", "12:00", "13:00", NULL};
//  }
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
