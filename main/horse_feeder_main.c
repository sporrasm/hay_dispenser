#include <stdio.h>
#include "wifi_connect.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void app_main(void)
{
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
    }
}
