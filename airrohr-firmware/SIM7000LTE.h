/*
 * @file SIM7000LTE.h
 *
 * Written by R.Dieperink, Rolenco Leusden
 * Date: 2024-04-26
 *
 * Version: 1.0.13
 *
 * SIM7000LTE.h
 *
 * Copyright (C) 2024
 *
 * This source code is provided 'as-is', without any express or implied
 * warranty. In no event will the author be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * TinyGSM a small Arduino library for GPRS modules, that just works.
 * Support SIM7000E/G GSM, LTE, and WiFi modules with AT command interfaces.
 * based on TinyGSM @ ^0.11.7
 */

#ifndef _SIM7000LTE_H
#define _SIM7000LTE_H

// Select LTE/GSM modem type: SIM7000E
#define TINY_GSM_MODEM_SIM7000              // version does not support SSL but supports up to 8 simultaneous connections.
// #define TINY_GSM_MODEM_SIM7000SSL        // version supports both SSL and unsecured connections with up to 2 simultaneous connections.

#include "TinyGSM.h"

#include "defines.h"
#include "ext_def.h"

SoftwareSerial SerialSIM;           // Serial port instance. (set baudrate, data lenght, ...)
TinyGsm LTEmodem(SerialSIM);        // LTEModem instance => connected to serial port.
TinyGsmClient LTEclient(LTEmodem);  // LTE Client instance (sens data to http/https)

// namespace cfg
// {
//     extern char wlanssid[];
//     extern char wlanpwd[];
// }

// SIM7000 settings.
namespace cfg7
{
	bool s7000_has_gps = HAS_GPS;

    char gprsapn[LEN_SIMM7000];
    char gprsUser[LEN_SIMM7000];
    char gprsPass[LEN_SIMM7000];

	char s7000_type[LEN_SIMM7000];
	char s7000_mode[LEN_SIMM7000];

    // init: set default values to options.
	void initNonTrivials()
    {
        strcpy_P(cfg7::gprsapn, HOST_AIRCMS);
        strcpy_P(cfg7::gprsUser, WWW_USERNAME);
        strcpy_P(cfg7::gprsPass, WWW_PASSWORD);
        strcpy_P(cfg7::s7000_type, SIM7_TYPE);
        strcpy_P(cfg7::s7000_mode, SIM7_MODE);
    }
}

//***************************************************************************************************************************************************

/*
    modem Power On.
*/
inline void modemPowerOn()
{
    debug_outln_info(F("modem Power-On process."), "");

    pinMode(SIM_PIN_PWR, OUTPUT);
    digitalWrite(SIM_PIN_PWR, HIGH);
    delay(1000); // Datasheet Toned mintues = 1S
    digitalWrite(SIM_PIN_PWR, LOW);
}

/*
    modem Power Off.
*/
inline void modemPowerOff()
{
    debug_outln_info(F("modem Power-Off process."), "");

    pinMode(SIM_PIN_PWR, OUTPUT);
    digitalWrite(SIM_PIN_PWR, HIGH);
    delay(1500); // Datasheet Toned mintues = minimal 1.2S
    digitalWrite(SIM_PIN_PWR, LOW);
}

/*
    Test response to 'AT' commands.

*/
inline bool IsModemActive()
{
    if (!LTEmodem.testAT())
    {
        debug_outln_info(F("Test AT command past \"NOT OK\""), "");

#ifdef DEBUG
        // Range to attempt to autobaud
        // NOTE:  DO NOT AUTOBAUD in production code.
        //
        // Once you've established communication, set a fixed baud rate using modem.setBaud(#).

        // Set GSM module baud rate and start communication.
        uint32_t baud = TinyGsmAutoBaud(SerialSIM, LTEMODEM_BAUD, SERIALSIM_BAUD);
        debug_outln_info(F("Modem baudrate is: "), String(baud));
#endif

        delay(5000);
        return false;
    }

    debug_outln_info(F("Test AT command past \"OK\", Modem baudrate is: "), String(LTEMODEM_BAUD));
    return true;
}

/*
    modem Restart:
        by Power Off / Power On
*/
inline void modemRestart()
{
    debug_outln_info(F("modem Restart process START"), "");

    modemPowerOff();
    delay(1000);
    modemPowerOn();

    debug_outln_info(F("modem Restart process ENDED"), "");
}

/*
    ESP8266 serial speed to SIM7000 = Default baud rate is 115200 bps

    NOTE: Software serial is not reliable on 115200 baud and therefore changes it to a lower value.
          9600 works well in almost all applications, but 115200 works great with Hardware serial.
*/
inline boolean SIM7000LTEConnect()
{
    
#if defined(ESP8266)
    debug_outln_info(F("LTE Modem connect start process: "), "");
    // debug_outln_info(F("SIM700 Connecting to "), String(cfg::wlanssid));	// ???

    modemRestart();

    SerialSIM.begin(SERIALSIM_BAUD, SWSERIAL_8N1, SIM_PIN_RX, SIM_PIN_TX); // start with default SIM7000 shield baud rate.
    delay(200);
    LTEmodem.setBaud(LTEMODEM_BAUD); // Set SIM7000 LTE-modem baud rate to lower value.
    delay(100);

    SerialSIM.begin(LTEMODEM_BAUD, SWSERIAL_8N1); // set serial port to same baud.

    String modemInfo = LTEmodem.getModemInfo();
    debug_outln_info(F("LTE Modem Info: "), modemInfo);

    // Restart takes internal quite some time.
    // LTEmodem.restart();
    // To skip it, call init() instead of restart()
    if (!LTEmodem.init())
    {
        debug_outln_info(F(" Modem init Failed "), "");
        return false;
    }


    debug_outln_info(F("gprsConnect => set apn, user, pass value to open network connection to provide..."), "");
    LTEmodem.gprsConnect(cfg7::gprsapn, cfg7::gprsUser, cfg7::gprsPass);

    debug_outln_info(F("Waiting: provider acccept all credentials network active..."), "");
    if (!LTEmodem.waitForNetwork())
    {
        // display_debug(F(" Fail "), "");     // display on OLED display.
        // delay(10000);

        debug_outln_info(F(" gprsConnect network connection Failed "), "");
        return false;
    }

    debug_outln_info(F("GPRS network success"), "");

    if (LTEmodem.isGprsConnected())
    {
        debug_outln_info(F("GPRS connected."), "");
        return true;
    }
    else
    {
        debug_outln_info(F("GPRS NOT connected"), ", FAILED.");
        return false;
    }

#else
    return true;
#endif
}

inline void enableGPS(void)
{
    // Set Modem GPS Power Control Pin to HIGH, turn on GPS power
    LTEmodem.enableGPS();
}

inline void disableGPS(void)
{
    // Set Modem GPS Power Control Pin to LOW ,turn off GPS power
    LTEmodem.disableGPS();
}

/*
    Get GPS Location.

    max. 10 sec.
*/
inline void GetGPSLocation(float * latitude, float * longitude, float * altitude)
{
    debug_outln_info(F("Start positioning . Make sure to locate outdoors."));

    enableGPS();

    float speed;

    for(int cnt = 5; cnt > 0; cnt--)
    {
        if ( LTEmodem.getGPS(latitude, longitude, &speed, altitude) )
        {
            debug_outln_info(F("The location has been locked, the latitude and longitude are:"));
            debug_outln_info(F("latitude: "), String( *latitude));
            debug_outln_info(F("longitude: "), String( *longitude));

            break;
        }

        delay(2000);

        yield();							// give waiting thread(s) CPU time.
    }

    disableGPS();
}

#endif // _SIM7000LTE_H
