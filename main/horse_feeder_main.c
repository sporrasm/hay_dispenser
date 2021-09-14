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
#include "AIP31068L.h"

#define LCD_ADDR 0x3e
#define SDA_PIN  19
#define SCL_PIN  18
#define LCD_COLS 16 
#define LCD_ROWS 2

void LCD_updater(void* param);
time_t* timeFromString(char** times, unsigned int len);

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

void app_main(void)
{
  // The parameter to strptime must given as hour (0-23) and
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
    xTaskCreate(&LCD_updater, "LCD_updater", 2048, NULL, 5, NULL);
    for (time_t* i=arr; *i; i++) {
      printf("%s", ctime(i)); 
      printf("%ld\n", *i); 
    }
  }
}

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
