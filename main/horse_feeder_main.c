#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "wifi_connect.h"
#include "ntp_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void app_main(void)
{
    int wifi_conn_stat=0;
    char* env;
    printf("Initializing WiFi\n");
    wifi_conn_stat=conn_wifi();
    env=getenv("TZ");
    printf("Env TZ: %s\n", env);
    if (wifi_conn_stat != 0) {
        printf("Connection failed!\n");
        //printf("Trying again!\n");
        //vTaskDelay(5000 / portTICK_PERIOD_MS);
    }   
    else {
        printf("Connected to WiFi\n");
        printf("Getting current time\n");
        while (1) {
          time_t time = get_time();
          printf("Current time is %s", ctime(&time));
          vTaskDelay(5000/portTICK_PERIOD_MS);
        }
    }
}
