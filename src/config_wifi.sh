#!/bin/bash

if [ -z "$1" ]; then
    file="wifi_info.txt"
else
    file="$1"
fi

file="wifi_info.txt"

while IFS='=' read -ra CONF; do
    if [ ${CONF[0]} == 'SSID' ]; then
        sed -i "s/\"myssid\"/${CONF[1]}/g" sdkconfig
        #sed -i 's/^CONFIG_ESP_WIFI_SSID=.*$/CONFIG_ESP_WIFI_SSID="${CONF[1]}"/g' sdkconfig
    fi
    if [ ${CONF[0]} == "PASS" ]; then
        sed -i "s/\"mypassword\"/${CONF[1]}/g" sdkconfig
        #sed -i 's/^CONFIG_ESP_WIFI_PASSWORD=.*$/CONFIG_ESP_WIFI_PASSWORD="${CONF[1]}"/g' sdkconfig
    fi
done < "$file"
