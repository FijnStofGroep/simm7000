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

#include "./utils.h"
#include "./ext_def.h"

#include <SoftwareSerial.h>

SoftwareSerial serialSIM;
TinyGsm        LTEmodem(serialSIM);         // Serial port instance/object.
TinyGsmClient  LTEclient(LTEmodem);         // Http.

// namespace cfg 
// {
// 	extern char wlanssid[];
// }

//***************************************************************************************************************************************************

/*
	ESP8266 serial speed to SIM7000 = Default baud rate is 115200 bps

	NOTE: Software serial is not reliable on 115200 baud and therefore changes it to a lower value. 
		  9600 works well in almost all applications, but 115200 works great with Hardware serial.
*/
inline boolean SIM7000LTEConnect() 
{
#if defined(ESP8266)
	//debug_outln_info(F("SIM700 Connecting to "), String(cfg::wlanssid));	// ???

	pinMode(SIM_PIN_PWR, OUTPUT);						// Set Power-On/Off SIM7000 board.

	serialSIM.begin(SERIALSIM_BAUD, SWSERIAL_8N1, SIM_PIN_RX, SIM_PIN_TX);	// start with default SIM7000 shield baud rate.
	delay(100);
	LTEmodem.setBaud(LTEMODEM_BAUD);					// Set SIM7000 LTE-modem baud rate to lower value.
	delay(100);
	serialSIM.begin(LTEMODEM_BAUD, SWSERIAL_8N1);		// set serial port to same baud.

	// Restart takes internal quite some time.
	// LTEmodem.restart();
  	// To skip it, call init() instead of restart()
  	LTEmodem.init();

	String modemInfo = LTEmodem.getModemInfo();
  	debug_outln_info(F("LTE Modem Info: "), modemInfo);

	// Your GPRS credentials, if any
	const char apn[]      = "YourAPN";
	const char gprsUser[] = "";
	const char gprsPass[] = "";

	LTEmodem.gprsConnect(apn, gprsUser, gprsPass);

	debug_outln_info(F("Waiting for network..."),"");
	if (!LTEmodem.waitForNetwork())
	{
		//display_debug(F(" Fail "), "");     // display on OLED display.
        //delay(10000);

        debug_outln_info(F(" Fail "), "");
		return false;
	}

	debug_outln_info(F(" success"),"");

	if (LTEmodem.isGprsConnected())
	{
		debug_outln_info(F("GPRS connected."),"");
	}

#endif

	return true;
}


#endif // _SIM7000LTE_H 
