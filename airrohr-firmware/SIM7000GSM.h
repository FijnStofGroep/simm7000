/*
 * @file SIM7000GSM.h
 *
 * Written by R.Dieperink, Rolenco Leusden
 * Date: 2024-04-26
 *
 * Version: 1.0.14
 *
 * Copyright (C) 2024 ~ 2025
 *
 * 
 * https://www.w3.org/Protocols/rfc2616/rfc2616.html
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
    extern unsigned time_for_wifi_config;

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

    extern char lteapn[LEN_SIMM7000];
    extern char lteUser[LEN_SIMM7000];
    extern char ltePass[LEN_SIMM7000];
    extern char ltePIN[LEN_SEN5X_SYM];

    extern unsigned sim_type;
    extern unsigned mode_selection;
    extern unsigned communication_type;

    // init: set default values to options.
    extern void initNonTrivials();
}

extern unsigned long act_milli;

// internal defines.


//***************************************************************************************************************************************************
enum SETUP_STATE
{
    INIT = 0,
    RESTART = 1
};

/*
    ESP8266 serial speed to SIM7000 = Default baud rate is 115200 bps

    NOTE: Software serial is not reliable on 115200 baud and therefore changes it to a lower value.
          9600 works well in almost all applications, but 115200 works great with Hardware serial.
*/
boolean Sim7000_setup(int state);

/*
    Get GPS Location. timeout = max. 10 sec.
*/
boolean GetGPSLocation(float * latitude, float * longitude, float * altitude, String & timestamp);

/* 
    return: total send time.
*/
int32_t sendDataByLTE(const LoggerEntry logger, const String &data, const int pin,
							const char *host, const int portnr, const char *url);

/// @brief 
/// @param topic 
/// @param payload 
/// @return 
boolean sendDataByMQTT( const char *topic, const char *payload);

/// @brief 
/// @param  
void SyncNTPTime(void);

//--------------  Internal use ----------
/// @brief 
/// @param  
void setNTPTimeSync(void);

/* 
    BK-Sim7000 Modem Power Off.
*/
void modemPowerOff();

/// @brief Restart BK-SIM7000 PCB.
bool RestartLTEModem();

/// @brief 
/// @param  
/// @return 
int32_t GetWiFi_RSSI( void);

/// @brief 
/// @param  
/// @return 
String GetLTELocalIP(void);

/// @brief 
/// @param  
void LTEmodePowerSave(void);

/// @brief 
/// @param  
/// @return 
String GetSimDriverName(void);

/// @brief 
/// @param  
/// @return 
u_int32_t GetLTE_RestartCounter(void);

/// @brief Get temperature of the SIM-module.
/// @param  
/// @return temperature in C.
uint16_t GetModuleTemperature(void);

#endif // _SIM7000GSM_H
