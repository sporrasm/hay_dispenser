EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 3 4
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Switch:SW_Push SW?
U 1 1 61FA5A53
P 4900 3300
AR Path="/61FA5A53" Ref="SW?"  Part="1" 
AR Path="/61EBBE65/61FA5A53" Ref="SW1"  Part="1" 
F 0 "SW1" V 4946 3252 50  0000 R CNN
F 1 "SW_Push" V 4855 3252 50  0000 R CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 4900 3500 50  0001 C CNN
F 3 "~" H 4900 3500 50  0001 C CNN
	1    4900 3300
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 61FA5A5E
P 4600 3300
AR Path="/61FA5A5E" Ref="#PWR?"  Part="1" 
AR Path="/61EBBE65/61FA5A5E" Ref="#PWR05"  Part="1" 
F 0 "#PWR05" H 4600 3050 50  0001 C CNN
F 1 "GND" H 4605 3127 50  0000 C CNN
F 2 "" H 4600 3300 50  0001 C CNN
F 3 "" H 4600 3300 50  0001 C CNN
	1    4600 3300
	0    1    1    0   
$EndComp
Wire Wire Line
	4600 3300 4700 3300
Text Notes 4500 3100 0    50   ~ 0
Sisäinen ylösvetovastus.\n
$Comp
L devkits:ESP32DevKitCV4 U4
U 1 1 61FD35B3
P 6100 4050
F 0 "U4" H 6100 5125 50  0000 C CNN
F 1 "ESP32DevKitCV4" H 6100 5034 50  0000 C CNN
F 2 "devkits:ESP32_DevkitC_ExtraPins" H 6150 4000 50  0001 C CNN
F 3 "" H 6150 4000 50  0001 C CNN
	1    6100 4050
	1    0    0    -1  
$EndComp
Wire Wire Line
	5700 3200 5600 3200
Wire Wire Line
	5100 3300 5700 3300
Text Label 6900 4000 0    50   ~ 0
SCL
Text Label 6900 3900 0    50   ~ 0
SDA
Wire Wire Line
	5700 5000 5100 5000
Text Label 5100 5000 0    50   ~ 0
+5V
Text Label 5400 3200 0    50   ~ 0
3V3
$Comp
L Connector:Conn_01x04_Male J10
U 1 1 62009678
P 5800 1900
F 0 "J10" H 5908 2181 50  0000 C CNN
F 1 "Conn_01x04_Male" H 5908 2090 50  0000 C CNN
F 2 "Connector_PinHeader_2.00mm:PinHeader_1x04_P2.00mm_Vertical" H 5800 1900 50  0001 C CNN
F 3 "~" H 5800 1900 50  0001 C CNN
	1    5800 1900
	1    0    0    -1  
$EndComp
Wire Wire Line
	6000 1800 6200 1800
Wire Wire Line
	6000 1900 6200 1900
Wire Wire Line
	6000 2000 6200 2000
Wire Wire Line
	6000 2100 6200 2100
Text Label 6200 1800 0    50   ~ 0
GND
Text Label 6200 1900 0    50   ~ 0
3V3
Text Label 6200 2000 0    50   ~ 0
SDA
Text Label 6200 2100 0    50   ~ 0
SCL
Text Notes 5500 2300 0    50   ~ 0
Liitin näytölle (varaus)
Wire Wire Line
	6500 4400 6900 4400
Wire Wire Line
	6500 3700 6900 3700
Wire Wire Line
	6500 3400 6900 3400
Wire Wire Line
	6500 3300 6900 3300
Wire Wire Line
	5700 3900 5300 3900
Wire Wire Line
	5700 3800 5300 3800
Wire Wire Line
	6500 3200 6900 3200
Text Label 6900 3200 0    50   ~ 0
GND
Wire Wire Line
	6500 3800 6900 3800
Text Label 6900 3800 0    50   ~ 0
GND
NoConn ~ 6500 4100
NoConn ~ 6500 4200
NoConn ~ 6500 4300
NoConn ~ 6500 4500
NoConn ~ 6500 4600
NoConn ~ 6500 4800
NoConn ~ 6500 4900
NoConn ~ 6500 5000
NoConn ~ 5700 4400
NoConn ~ 5700 4700
NoConn ~ 5700 4800
NoConn ~ 5700 4900
Wire Wire Line
	5700 4500 5100 4500
Text Label 5100 4500 0    50   ~ 0
GND
Text HLabel 5300 3800 0    50   Output ~ 0
LOCK5
Text HLabel 5300 3900 0    50   Output ~ 0
LOCK4
Text HLabel 6900 3400 2    50   Output ~ 0
LOCK3
Text HLabel 6900 3300 2    50   Output ~ 0
LOCK2
Text HLabel 6900 3700 2    50   Output ~ 0
LOCK1
Text HLabel 6900 4400 2    50   Output ~ 0
LOCK0
Text HLabel 5450 3400 0    50   Input ~ 0
LOCK_DET0
Text HLabel 5450 3500 0    50   Input ~ 0
LOCK_DET1
Text HLabel 5450 3600 0    50   Input ~ 0
LOCK_DET2
Text HLabel 5450 3700 0    50   Input ~ 0
LOCK_DET3
Wire Wire Line
	5700 4100 5450 4100
Wire Wire Line
	5700 4000 5450 4000
Wire Wire Line
	5450 3700 5700 3700
Wire Wire Line
	5450 3600 5700 3600
Wire Wire Line
	5450 3500 5700 3500
Wire Wire Line
	5450 3400 5700 3400
Text HLabel 5450 4000 0    50   Input ~ 0
LOCK_DET4
Text HLabel 5450 4100 0    50   Input ~ 0
LOCK_DET5
Wire Wire Line
	5700 4300 5350 4300
Wire Wire Line
	5700 4200 5350 4200
Text HLabel 5350 4200 0    50   Input ~ 0
LEFT
Text HLabel 5350 4300 0    50   Input ~ 0
RIGHT
Wire Wire Line
	5700 4600 5300 4600
Text HLabel 5300 4600 0    50   Input ~ 0
SELECT
NoConn ~ 6500 4700
NoConn ~ 6500 3500
NoConn ~ 6500 3600
$Comp
L power:+3.3V #PWR0101
U 1 1 61F3650F
P 5600 3200
F 0 "#PWR0101" H 5600 3050 50  0001 C CNN
F 1 "+3.3V" H 5615 3373 50  0000 C CNN
F 2 "" H 5600 3200 50  0001 C CNN
F 3 "" H 5600 3200 50  0001 C CNN
	1    5600 3200
	1    0    0    -1  
$EndComp
Connection ~ 5600 3200
Wire Wire Line
	5600 3200 5400 3200
Wire Wire Line
	6500 3900 7350 3900
$Comp
L Device:R R28
U 1 1 61E5E901
P 7350 3750
F 0 "R28" H 7420 3796 50  0000 L CNN
F 1 "3k3" H 7420 3705 50  0000 L CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 7280 3750 50  0001 C CNN
F 3 "~" H 7350 3750 50  0001 C CNN
	1    7350 3750
	1    0    0    -1  
$EndComp
$Comp
L Device:R R29
U 1 1 61E5F0E3
P 7575 3850
F 0 "R29" H 7645 3896 50  0000 L CNN
F 1 "3k3" H 7645 3805 50  0000 L CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" V 7505 3850 50  0001 C CNN
F 3 "~" H 7575 3850 50  0001 C CNN
	1    7575 3850
	1    0    0    -1  
$EndComp
Wire Wire Line
	6500 4000 7575 4000
Wire Wire Line
	7575 3700 7575 3575
Wire Wire Line
	7350 3600 7350 3575
Wire Wire Line
	7350 3575 7575 3575
Text Label 7450 3575 0    50   ~ 0
3V3
$EndSCHEMATC
