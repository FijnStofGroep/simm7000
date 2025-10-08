

# Software for Sensor.Community / Luftdaten.Info Sensor

## airrohr-firmware

The maintained main firmware for the Luftdaten.Info Sensor. 

## SIMM70xx driver for
* BK-SIM7000 PCB module
* BK-SIM7080 PCB module

* Power solar panel:
*	For powering air quality and sound sensors with 4G/LTE in a food forest, a small solar setup works well: most sensors use under 0.5 W, 
* 	while a 4G modem averages 1–2 W (peaking higher during transmission). 
* 	A 20–30 W solar panel with a 12V 7Ah battery and a compact charge controller is usually enough for continuous operation. 
* 	To save power, configure the modem to transmit data at intervals (e.g., every 5–10 minutes) instead of streaming constantly, 
*	ensuring reliable performance even on cloudy days.

## New Sensors
* SEN5X => PM1.0,PM2.5,PM4,PM10,Relative Humidity,Temperature,VOC Index,NOx Index
* SCD30 => CO2 

## Plugins
* MQTT
* Static IP
* Power save

## Directories 
* /bin      flasher (FlashESP8266) and firmware bin
* /Doc      manual/handleiding 
* /Doc/PCB  schematic and Gerberfile


## WIFI standard process
* SSID is airRohr-<MCU ID>
* password is 'airrohrcfg'

## WIFI AP-mode LTE process
* SSID is FWL_Device-<MCU ID>
* password is 'airrohrcfg'

## Wiki
https://github.com/FijnStofGroep/sensors-software-Leusden/wiki



