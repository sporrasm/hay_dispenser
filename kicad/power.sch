EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 2 4
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
L Regulator_Linear:L7809 U5
U 1 1 61E80D2A
P 3000 1400
F 0 "U5" H 3000 1642 50  0000 C CNN
F 1 "L7809" H 3000 1551 50  0000 C CNN
F 2 "Package_TO_SOT_THT:TO-220-3_Vertical" H 3025 1250 50  0001 L CIN
F 3 "http://www.st.com/content/ccc/resource/technical/document/datasheet/41/4f/b3/b0/12/d4/47/88/CD00000444.pdf/files/CD00000444.pdf/jcr:content/translations/en.CD00000444.pdf" H 3000 1350 50  0001 C CNN
	1    3000 1400
	1    0    0    -1  
$EndComp
$Comp
L Regulator_Linear:L7805 U6
U 1 1 61E81799
P 5750 1400
F 0 "U6" H 5750 1642 50  0000 C CNN
F 1 "L7805" H 5750 1551 50  0000 C CNN
F 2 "Package_TO_SOT_THT:TO-220-3_Vertical" H 5775 1250 50  0001 L CIN
F 3 "http://www.st.com/content/ccc/resource/technical/document/datasheet/41/4f/b3/b0/12/d4/47/88/CD00000444.pdf/files/CD00000444.pdf/jcr:content/translations/en.CD00000444.pdf" H 5750 1350 50  0001 C CNN
	1    5750 1400
	1    0    0    -1  
$EndComp
Wire Wire Line
	3000 1700 3000 1950
Wire Wire Line
	5750 1950 5750 1700
Connection ~ 5750 1950
Wire Wire Line
	6050 1400 6350 1400
$Comp
L Device:C C9
U 1 1 61E830DC
P 6350 1700
F 0 "C9" H 6465 1746 50  0000 L CNN
F 1 "100u" H 6465 1655 50  0000 L CNN
F 2 "Capacitor_THT:CP_Radial_D5.0mm_P2.50mm" H 6388 1550 50  0001 C CNN
F 3 "~" H 6350 1700 50  0001 C CNN
	1    6350 1700
	1    0    0    -1  
$EndComp
$Comp
L Device:C C10
U 1 1 61E83434
P 6700 1700
F 0 "C10" H 6815 1746 50  0000 L CNN
F 1 "47u" H 6815 1655 50  0000 L CNN
F 2 "Capacitor_THT:CP_Radial_D5.0mm_P2.50mm" H 6738 1550 50  0001 C CNN
F 3 "~" H 6700 1700 50  0001 C CNN
	1    6700 1700
	1    0    0    -1  
$EndComp
$Comp
L Device:C C12
U 1 1 61E83A54
P 7400 1700
F 0 "C12" H 7515 1746 50  0000 L CNN
F 1 "C" H 7515 1655 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 7438 1550 50  0001 C CNN
F 3 "~" H 7400 1700 50  0001 C CNN
	1    7400 1700
	1    0    0    -1  
$EndComp
$Comp
L Device:C C13
U 1 1 61E83D78
P 7800 1700
F 0 "C13" H 7915 1746 50  0000 L CNN
F 1 "C" H 7915 1655 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 7838 1550 50  0001 C CNN
F 3 "~" H 7800 1700 50  0001 C CNN
	1    7800 1700
	1    0    0    -1  
$EndComp
$Comp
L Device:C C8
U 1 1 61E84086
P 5350 1700
F 0 "C8" H 5465 1746 50  0000 L CNN
F 1 "4.7u" H 5465 1655 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 5388 1550 50  0001 C CNN
F 3 "~" H 5350 1700 50  0001 C CNN
	1    5350 1700
	1    0    0    -1  
$EndComp
$Comp
L Device:C C4
U 1 1 61E844EB
P 3900 1700
F 0 "C4" H 4015 1746 50  0000 L CNN
F 1 "100u" H 4015 1655 50  0000 L CNN
F 2 "Capacitor_THT:CP_Radial_D5.0mm_P2.50mm" H 3938 1550 50  0001 C CNN
F 3 "~" H 3900 1700 50  0001 C CNN
	1    3900 1700
	1    0    0    -1  
$EndComp
$Comp
L Device:C C2
U 1 1 61E84C7C
P 2200 1700
F 0 "C2" H 2315 1746 50  0000 L CNN
F 1 "100n" H 2315 1655 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 2238 1550 50  0001 C CNN
F 3 "~" H 2200 1700 50  0001 C CNN
	1    2200 1700
	1    0    0    -1  
$EndComp
Wire Wire Line
	2200 1950 2200 1850
Wire Wire Line
	2200 1950 2500 1950
Wire Wire Line
	2500 1850 2500 1950
Connection ~ 2500 1950
Wire Wire Line
	2500 1950 2700 1950
Wire Wire Line
	2500 1400 2500 1550
Connection ~ 2500 1400
Wire Wire Line
	2500 1400 2700 1400
Wire Wire Line
	2200 1400 2200 1550
Connection ~ 2200 1400
Wire Wire Line
	2200 1400 2500 1400
$Comp
L power:GND #PWR04
U 1 1 61E86ED8
P 2700 1950
F 0 "#PWR04" H 2700 1700 50  0001 C CNN
F 1 "GND" H 2705 1777 50  0000 C CNN
F 2 "" H 2700 1950 50  0001 C CNN
F 3 "" H 2700 1950 50  0001 C CNN
	1    2700 1950
	1    0    0    -1  
$EndComp
Connection ~ 2700 1950
Wire Wire Line
	2700 1950 3000 1950
$Comp
L power:+12V #PWR01
U 1 1 61E87563
P 1600 1400
F 0 "#PWR01" H 1600 1250 50  0001 C CNN
F 1 "+12V" H 1615 1573 50  0000 C CNN
F 2 "" H 1600 1400 50  0001 C CNN
F 3 "" H 1600 1400 50  0001 C CNN
	1    1600 1400
	1    0    0    -1  
$EndComp
$Comp
L power:+9V #PWR02
U 1 1 61E88F56
P 3700 1400
F 0 "#PWR02" H 3700 1250 50  0001 C CNN
F 1 "+9V" H 3715 1573 50  0000 C CNN
F 2 "" H 3700 1400 50  0001 C CNN
F 3 "" H 3700 1400 50  0001 C CNN
	1    3700 1400
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR03
U 1 1 61E89A1A
P 7000 1400
F 0 "#PWR03" H 7000 1250 50  0001 C CNN
F 1 "+5V" H 7015 1573 50  0000 C CNN
F 2 "" H 7000 1400 50  0001 C CNN
F 3 "" H 7000 1400 50  0001 C CNN
	1    7000 1400
	1    0    0    -1  
$EndComp
Wire Wire Line
	6350 1550 6350 1400
Connection ~ 6350 1400
Wire Wire Line
	6350 1400 6700 1400
Wire Wire Line
	6700 1550 6700 1400
Connection ~ 6700 1400
Wire Wire Line
	7050 1550 7050 1400
Wire Wire Line
	6700 1400 7000 1400
Connection ~ 7000 1400
Wire Wire Line
	7000 1400 7050 1400
Wire Wire Line
	7400 1550 7400 1400
Connection ~ 7050 1400
Wire Wire Line
	7800 1550 7800 1400
Wire Wire Line
	7050 1400 7400 1400
Connection ~ 7400 1400
Wire Wire Line
	7400 1400 7800 1400
Wire Wire Line
	7800 1850 7800 1950
Wire Wire Line
	5750 1950 6350 1950
Wire Wire Line
	7400 1850 7400 1950
Connection ~ 7400 1950
Wire Wire Line
	7400 1950 7800 1950
Wire Wire Line
	7050 1850 7050 1950
Connection ~ 7050 1950
Wire Wire Line
	7050 1950 7400 1950
Wire Wire Line
	6700 1850 6700 1950
Connection ~ 6700 1950
Wire Wire Line
	6700 1950 7050 1950
Wire Wire Line
	6350 1850 6350 1950
Connection ~ 6350 1950
Wire Wire Line
	6350 1950 6700 1950
Connection ~ 3700 1400
Wire Wire Line
	3000 1950 3250 1950
Connection ~ 3000 1950
$Comp
L Device:C C5
U 1 1 61E96991
P 4300 1700
F 0 "C5" H 4415 1746 50  0000 L CNN
F 1 "100u" H 4415 1655 50  0000 L CNN
F 2 "Capacitor_THT:CP_Radial_D5.0mm_P2.50mm" H 4338 1550 50  0001 C CNN
F 3 "~" H 4300 1700 50  0001 C CNN
	1    4300 1700
	1    0    0    -1  
$EndComp
$Comp
L Device:C C7
U 1 1 61E97281
P 5050 1700
F 0 "C7" H 5165 1746 50  0000 L CNN
F 1 "100n" H 5165 1655 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 5088 1550 50  0001 C CNN
F 3 "~" H 5050 1700 50  0001 C CNN
	1    5050 1700
	1    0    0    -1  
$EndComp
Wire Wire Line
	5350 1550 5350 1400
Wire Wire Line
	3700 1400 3900 1400
Connection ~ 5350 1400
Wire Wire Line
	5350 1400 5450 1400
Wire Wire Line
	5050 1550 5050 1400
Connection ~ 5050 1400
Wire Wire Line
	5050 1400 5350 1400
Wire Wire Line
	4700 1550 4700 1400
Connection ~ 4700 1400
Wire Wire Line
	4700 1400 5050 1400
Wire Wire Line
	4300 1550 4300 1400
Connection ~ 4300 1400
Wire Wire Line
	4300 1400 4700 1400
Wire Wire Line
	3900 1550 3900 1400
Connection ~ 3900 1400
Wire Wire Line
	3900 1400 4300 1400
Wire Wire Line
	3900 1850 3900 1950
Connection ~ 3900 1950
Wire Wire Line
	3900 1950 4300 1950
Wire Wire Line
	4300 1850 4300 1950
Connection ~ 4300 1950
Wire Wire Line
	4300 1950 4700 1950
Wire Wire Line
	4700 1850 4700 1950
Connection ~ 4700 1950
Wire Wire Line
	4700 1950 5050 1950
Wire Wire Line
	5050 1850 5050 1950
Connection ~ 5050 1950
Wire Wire Line
	5050 1950 5350 1950
Wire Wire Line
	5350 1850 5350 1950
Connection ~ 5350 1950
Wire Wire Line
	5350 1950 5750 1950
$Comp
L Device:C C3
U 1 1 61E84951
P 2500 1700
F 0 "C3" H 2615 1746 50  0000 L CNN
F 1 "100p" H 2615 1655 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 2538 1550 50  0001 C CNN
F 3 "~" H 2500 1700 50  0001 C CNN
	1    2500 1700
	1    0    0    -1  
$EndComp
$Comp
L Device:C C1
U 1 1 61EA2DC4
P 1800 1700
F 0 "C1" H 1915 1746 50  0000 L CNN
F 1 "100u" H 1915 1655 50  0000 L CNN
F 2 "Capacitor_THT:CP_Radial_D5.0mm_P2.50mm" H 1838 1550 50  0001 C CNN
F 3 "~" H 1800 1700 50  0001 C CNN
	1    1800 1700
	1    0    0    -1  
$EndComp
Wire Wire Line
	1800 1550 1800 1400
Wire Wire Line
	1800 1400 2000 1400
Wire Wire Line
	1800 1850 1800 1950
Connection ~ 2200 1950
Wire Wire Line
	1600 1400 1800 1400
Connection ~ 1800 1400
$Comp
L Device:C C6
U 1 1 61E96D73
P 4700 1700
F 0 "C6" H 4815 1746 50  0000 L CNN
F 1 "100u" H 4815 1655 50  0000 L CNN
F 2 "Capacitor_THT:CP_Radial_D5.0mm_P2.50mm" H 4738 1550 50  0001 C CNN
F 3 "~" H 4700 1700 50  0001 C CNN
	1    4700 1700
	1    0    0    -1  
$EndComp
$Comp
L Device:C C11
U 1 1 61E8376D
P 7050 1700
F 0 "C11" H 7165 1746 50  0000 L CNN
F 1 "4.7u" H 7165 1655 50  0000 L CNN
F 2 "Capacitor_THT:CP_Radial_D5.0mm_P2.50mm" H 7088 1550 50  0001 C CNN
F 3 "~" H 7050 1700 50  0001 C CNN
	1    7050 1700
	1    0    0    -1  
$EndComp
Text Notes 1050 900  0    50   ~ 0
This is a very shoddy power supply. Excessive caps at 9 V output are needed for smoothing the current spikes when locks open.
Wire Wire Line
	3300 1400 3700 1400
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 62218D81
P 2000 1400
F 0 "#FLG0101" H 2000 1475 50  0001 C CNN
F 1 "PWR_FLAG" H 2000 1573 50  0000 C CNN
F 2 "" H 2000 1400 50  0001 C CNN
F 3 "~" H 2000 1400 50  0001 C CNN
	1    2000 1400
	1    0    0    -1  
$EndComp
Connection ~ 2000 1400
Wire Wire Line
	2000 1400 2200 1400
$Comp
L power:PWR_FLAG #FLG0102
U 1 1 6222A2EA
P 3250 1950
F 0 "#FLG0102" H 3250 2025 50  0001 C CNN
F 1 "PWR_FLAG" H 3250 2123 50  0000 C CNN
F 2 "" H 3250 1950 50  0001 C CNN
F 3 "~" H 3250 1950 50  0001 C CNN
	1    3250 1950
	1    0    0    -1  
$EndComp
Connection ~ 3250 1950
Wire Wire Line
	3250 1950 3900 1950
$Comp
L Connector:Barrel_Jack_Switch_Pin3Ring J9
U 1 1 62262890
P 950 1700
F 0 "J9" H 1007 2017 50  0000 C CNN
F 1 "Barrel_Jack_Switch_Pin3Ring" H 1007 1926 50  0000 C CNN
F 2 "connectors:BarrelJack_CUI_PJ-102AH_Horizontal" H 1000 1660 50  0001 C CNN
F 3 "~" H 1000 1660 50  0001 C CNN
	1    950  1700
	1    0    0    -1  
$EndComp
Wire Wire Line
	1250 1700 1250 1800
Connection ~ 1250 1800
Wire Wire Line
	1250 1800 1250 1950
Connection ~ 1800 1950
Wire Wire Line
	1800 1950 2200 1950
Wire Wire Line
	1250 1600 1250 1400
Wire Wire Line
	1250 1400 1600 1400
Connection ~ 1600 1400
$Comp
L Connector:Barrel_Jack_Switch_Pin3Ring J8
U 1 1 61DC9BFA
P 850 1200
F 0 "J8" H 907 1517 50  0000 C CNN
F 1 "Barrel_Jack_Switch_Pin3Ring" H 907 1426 50  0000 C CNN
F 2 "Connector_BarrelJack:BarrelJack_CUI_PJ-102AH_Horizontal" H 900 1160 50  0001 C CNN
F 3 "~" H 900 1160 50  0001 C CNN
	1    850  1200
	1    0    0    -1  
$EndComp
Wire Wire Line
	1150 1200 1150 1300
Wire Wire Line
	1150 1100 1250 1100
Wire Wire Line
	1250 1100 1250 1400
Connection ~ 1250 1400
Wire Wire Line
	1150 1300 1150 1450
Wire Wire Line
	1150 1450 650  1450
Wire Wire Line
	650  1450 650  1950
Wire Wire Line
	650  1950 1250 1950
Connection ~ 1150 1300
Connection ~ 1250 1950
Wire Wire Line
	1250 1950 1800 1950
$EndSCHEMATC
