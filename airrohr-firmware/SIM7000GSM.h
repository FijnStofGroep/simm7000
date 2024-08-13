/*
 * @file SIM7000GSM.h
 *
 * Written by R.Dieperink, Rolenco Leusden
 * Date: 2024-04-26
 *
 * Version: 1.0.14
 *
 * Copyright (C) 2024
 *
 */

#ifndef _SIM7000GSM_H
#define _SIM7000GSM_H

#include "BK_SIM7000.h"

#include "./utils.h"
#include "defines.h"
#include "ext_def.h"

// external define cfg::member type.
namespace cfg 
{
    extern unsigned sending_intervall_ms;

    extern bool gps_read;
    extern bool send2mqtt;

    extern char fs_ssid[LEN_FS_SSID];           // sensor device ID

	/*	MQTT  */
	extern char mqtt_server[LEN_HOST_CUSTOM];
	extern unsigned mqtt_port;
	extern char mqtt_user[LEN_USER_INFLUX];
	extern char mqtt_pwd[LEN_PASS_INFLUX];
	extern char mqtt_topic[LEN_MQTT_HEADER];
}

/*
    BK-SIM7000 settings.
*/
namespace cfg7
{
    extern bool s7000_has_gps;

    extern char gprsapn[LEN_SIMM7000];
    extern char gprsUser[LEN_SIMM7000];
    extern char gprsPass[LEN_SIMM7000];
    extern char gprsPIN[LEN_SEN5X_SYM];

    extern char s7000_type[LEN_SIMM7000];
    extern char s7000_mode[LEN_SIMM7000];

    // init: set default values to options.
    extern void initNonTrivials();
}

//***************************************************************************************************************************************************

/*
    ESP8266 serial speed to SIM7000 = Default baud rate is 115200 bps

    NOTE: Software serial is not reliable on 115200 baud and therefore changes it to a lower value.
          9600 works well in almost all applications, but 115200 works great with Hardware serial.
*/
boolean BK_Sim7000_setup();

/*
    Get GPS Location. timeout = max. 10 sec.
*/
void GetGPSLocation(float * latitude, float * longitude, float * altitude, String & timestamp);

/* 
    return: total send time.
*/
int32_t sendDataByGSM(const LoggerEntry logger, const String &data, const int pin,
							const char *host, const int portnr, const char *url);

uint8_t sendDataByMQTT( const char *topic, const char *payload);

/* 
    BK-Sim7000 Modem Power Off.
*/
void modemPowerOff();


#endif // _SIM7000GSM_H

