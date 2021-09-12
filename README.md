
Readme for project horse_feeder

Remeber to run the export script from ESP install dir!

To configure (once per project), run

```
idf.py set-target esp32
idf.py menuconfig
```
Alternatively, WiFi SSID and password can be
set with `wifi_config.sh`. Place SSID and password
in `wifi_info.txt` in the following format:

```
SSID=YOUR_NETWORK_SSID
PASS=YOUR_PASSWORD
```
`wifi_info.txt` is automatically ignored in version
control and thus your password is safe.

To build, run
```
idf.py build
```
Finally, to flash and monitor, run

```
idf.py -p ESP_SERIAL_PORT flash monitor
```

