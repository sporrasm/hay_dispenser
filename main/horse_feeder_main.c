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


void app_main(void)
{
    int init_stat=init_horse_feeder();
    if (init_stat != 0) {
      ESP_LOGW(TAG, "Initialization failed!");
    }
    int wifi_conn_stat=0;
    printf("Initializing WiFi\n");
    wifi_conn_stat=conn_wifi();
    if (wifi_conn_stat != 0) {
        printf("Connection failed!\n");
        //printf("Trying again!\n");
        //vTaskDelay(5000 / portTICK_PERIOD_MS);
    }   
    else {
        printf("Connected to WiFi\n");
        printf("Getting current time\n");
        time_t time = get_time();
        ESP_LOGI(TAG, "Setting system time to %s", ctime(&time));
        struct timeval tv, currtv;
        tv.tv_sec=time;
        settimeofday(&tv, NULL);
        while (1) {
          gettimeofday(&currtv, NULL);
          printf("Current time is %s", ctime(&currtv.tv_sec));
          vTaskDelay(5000/portTICK_PERIOD_MS);
        }
    }
}
