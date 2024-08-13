/*
 * @file SIM7000GSM.cpp
 *
 * Written by R.Dieperink, Rolenco Leusden.
 * Date: 2024-04-26
 *
 * Version: 1.0.13
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
 * a small BK_SIM7000 library for GPRS modules, that just works.
 * Support SIM7000E/G GSM, LTE, and WiFi modules with AT command interfaces.
 * based on Adafruit_FONA
 *
 *  AND Technologies Co., ltd, Breakout SIM7000 PCB board
 *  https://www.and-global.com
 *  https://www.and-global.com/sim7000e-breakout-board-sim7000-core-kit-with-nb-and-gps-antenna.html
 *	see file: BK-SIM7000E DOC-V2.0.zip
 */


#include "SIM7000GSM.h"

#include <SoftwareSerial.h>

//***************************************************************************************************************************************************
//#define SENSOR_BASENAME "esp8266-" // see defines.h
//#define DEBUG_MAX_INFO 5

#define LoggerCount  7                          // see enum LoggerEntry {..}
extern LoggerConfig loggerConfigs[LoggerCount];

// original in 
//#define SOFTWARE_VERSION_STR "FWL-2024-05-B1"
extern String SOFTWARE_VERSION;

extern int last_sendData_returncode;
extern String esp_chipid;
extern String esp_mac_id;

extern char mqtt_client_id[LEN_MQTT_HEADER];
extern char mqtt_header[LEN_MQTT_LARGE_HEADER];
extern char mqtt_lwt_header[LEN_MQTT_LARGE_HEADER];
extern const char mqtt_lwt[5] = MQTT_LWT;

// original in html-content.h
extern const char TXT_CONTENT_TYPE_JSON[] PROGMEM = "application/json";
extern const char TXT_CONTENT_TYPE_INFLUXDB[] PROGMEM = "application/x-www-form-urlencoded";
extern const char TXT_CONTENT_TYPE_TEXT_PLAIN[] PROGMEM = "text/plain";

//***************************************************************************************************************************************************
 /// HTTP codes see RFC7231
 #define    HTTP_CODE_OK                200
 #define    HTTP_CODE_ALREADY_REPORTED  208
 #define    HTTP_CODE_BAD_REQUEST       400

/*
    BK-SIM7000 settings.
*/
namespace cfg7
{
	bool s7000_has_gps = HAS_GPS;

    char gprsapn[LEN_SIMM7000];
    char gprsUser[LEN_SIMM7000];
    char gprsPass[LEN_SIMM7000];

	char s7000_type[LEN_SIMM7000];
	char s7000_mode[LEN_SIMM7000];

    // set GSM PIN, if any (#define GSM_PIN "")
    char gprsPIN[LEN_SEN5X_SYM];

    // init: set default values to options.
	void initNonTrivials()
    {
        strcpy_P(gprsapn, GPRSAPNCODE);
        strcpy_P(gprsapn, HOST_AIRCMS);
        strcpy_P(gprsUser, WWW_USERNAME);
        strcpy_P(gprsPass, WWW_PASSWORD);
        strcpy_P(s7000_type, SIM7_TYPE);
        strcpy_P(s7000_mode, SIM7_MODE);
    }
}

// NodeMCU ESP8266 Serial port instance. (set baudrate, data lenght, ...)
// Serial instance for Communication between nodeMCU and BK-Sim7000 PCB.
SoftwareSerial SerialSIM;

// Use this one for LTE CAT-M/NB-IoT modules (BK-SIM7000 development PCB) 
BK_modem_LTE GSMmodem = BK_modem_LTE(); 

bool gsm_init_failed = false;
char m_imei[16] = {0};                        // Use this for LTE modem device ID.

//***************************************************************************************************************************************************

/*****************************************************************
    GPRS Modem Info()
******************************************************************/
void GPRSModemInfo()
{
    if (cfg::debug < DEBUG_MAX_INFO)
    {
        // String local = GSMmodem.getGPRSIP();
        // debug_outln_info(F("Local IP: "), local);

        String oper = GSMmodem.getOperator();
        debug_outln_info(F("Operator: "), oper);

        return;
    }

    int16_t imode;
    RESERVE_STRING(smode, MED_STR);

    debug_outln_info(F("\n--- Display GPRS Information ---"));

    if (GSMmodem.isGprsConnected())
    {
        debug_outln_info(F("GPRS status: connected."));
    }
    else
    {
        debug_outln_info(F("GPRS status: not connected."));
    }

    debug_outln_info(F("Software ") + GSMmodem.getModemSoftware_Revision());

    imode = GSMmodem.getNetworkMode();
    debug_outln_info(F("2 Automatic , 13 GSM only , 38 LTE only , 51 GSM and LTE only:\nCurrent Network Mode = "), String(imode));

    uint8_t epsStatus = GSMmodem.getNetworkStatus();
    debug_out(F("Network status code: ") + String(epsStatus) + F(" => "), DEBUG_MIN_INFO);

    if (epsStatus == 0) 
        debug_outln_info(F("Not registered"));
    else if (epsStatus == 1) 
        debug_outln_info(F("Registered (home)"));
    else if (epsStatus == 2) 
        debug_outln_info(F("Not registered (searching)"));
    else if (epsStatus == 3) 
        debug_outln_info(F("Denied"));
    else if (epsStatus == 4) 
        debug_outln_info(F("Unknown"));
    else if (epsStatus == 5) 
        debug_outln_info(F("Registered roaming"));

    smode = GSMmodem.getPreferredModes();
    debug_outln_info("Availlable Preferred Modes: " + smode);

    imode = GSMmodem.getPreferredMode();
    debug_outln_info("Current Preferred Mode = " + String(imode));

    char status[13];
    imode = GSMmodem.getNetworkSystemMode(status);
    debug_outln_info(F("SystemMode: "), String(imode) + F(" => ") + String(status));

    char ccid[64];
    GSMmodem.getSIMCCID(ccid);
    debug_outln_info(F("CCID: "), String(ccid));

    GSMmodem.getIMEI(m_imei);
    debug_outln_info(F("IMEI: "), String(m_imei));

    smode.clear();
    smode = GSMmodem.getOperator();
    debug_outln_info(F("Operator: "), smode);

    smode.clear();
    smode = GSMmodem.getSIMCOMATI();
    debug_outln_info(F("SIMCOMATI:\n"), smode);
    
    uint8_t csq;
    int8 dBm; 
    GSMmodem.getSignalQuality(&csq, &dBm);
    debug_outln_info(F("Signal quality: "), String(csq) + "%" + F(", RSSI dBm: ") + String(dBm) + "dBM");

    // Get connection type and band.
    smode.clear();
    GSMmodem.getNetworkInfo(smode);
    debug_outln_info(F("The current network parameter: "), smode);

    debug_outln_info(F("--- End of GPRS Information ---\n"));
}

/// @brief BK-SIM7000 PCB Power OFF.
void modemPowerOff()
{
    GSMmodem.modemPowerOff();
}

/// @brief 
/// @return
inline boolean GPRSConnect()
{
    debug_outln_info(F("GPRSConnect(): Waiting for network..."));

    // Connect to cell network and verify connection
    // If unsuccessful, keep retrying every 2s until a connection is made.
    int retry = 20;

    while (!GSMmodem.isNetworkConnected())
    {
        if (--retry < 0)
        {
            debug_outln_info(F("Failed to connect to cell network, restart connection with SIM7000 module..."));
            if (GSMmodem.openWirelessConnection(false))
            {
                break;
            }

            return false;
        }

        debug_outln_info(F("Failed to connect to cell network, retrying..."));
        delay(2000); // Retry every 2s
    }

    debug_outln_info(F("Connected to LTE cell network!"));

    // Disable data just to make sure it was actually off so that we can turn it on
    // GSMmodem.openWirelessConnection(false);

    // Open wireless connection if not already activated.
    if (!GSMmodem.wirelessConnStatus())
    {
        int retry = 5;

        while (retry > 0 && !GSMmodem.openWirelessConnection(true))
        {
            debug_outln_info(F("Failed to Open Wireless Connection, retrying..."));
            delay(2000); // Retry every 2s

            retry--;
        }

        if (retry == 0)
        {
            GSMmodem.openWirelessConnection(false);
            return false;
        }

        debug_outln_info( F("GPRS-IP address: ") + GSMmodem.getGPRSIP());

        debug_outln_info(F("GSM/LTE connection Enabled."));

        wdt_reset(); // watchdog timer reset => nodemcu ESP8266 still alive.
    }
    else
    {
        debug_outln_info(F("GSM/LTE connection already enabled."));
    }

    return true;
}

/**************************************************************************************************************
    ESP8266 serial speed to BK-SIM7000, default baud rate is 115200 bps.

    NOTE: Software serial is not reliable on 115200 baud and therefore changes it to a lower value.
          9600 works well in almost all applications.

          But 115200 works great with Hardware serial pins.
***************************************************************************************************************/
bool BK_Sim7000_setup()
{
#if defined(ESP8266)
    debug_outln_info(F("BK_Sim7000 Modem connect start process: "), "");

    // set RS-232 port settings between ESP8266 and BK-SIM7000 module.
    SerialSIM.begin(SERIALSIM_BAUD, SWSERIAL_8N1, SIM_PIN_RX, SIM_PIN_TX); // start with default SIM7000 shield baud rate.

    // initialize GSMmodem instance.
    GSMmodem.init(SerialSIM, Debug, SIM_PIN_PWR);
    // Set BK-SIM7000 GSM-modem baud rate to lower value.
    GSMmodem.setBaudrate(GSMMODEM_BAUD);
    delay(100);

    // set NodeMCU serial port to same baud.
    SerialSIM.begin(GSMMODEM_BAUD, SWSERIAL_8N1); 
    delay(100);

    debug_outln_info(F("Initializing BK_Sim7000 Modem PCB..."));

    if (!GSMmodem.begin())
    {
        debug_outln_info(F("GSM Modem init Failed.."));
        gsm_init_failed = true;
        return false;
    }

    String name = GSMmodem.getModemName();
    debug_outln_info(F("Modem Name: "), name);

    String modemInfo = GSMmodem.getModemInfo();
    debug_outln_info(F("GSM Modem Info: "), modemInfo);

    // Unlock your SIM card with a PIN if needed
    int8_t stat = GSMmodem.getPINStatus();
    debug_outln_info(F("PIN Status: "), String(stat));
    // debug_outln_info(F("gprs PIN: "), String(strlen(cfg7::gprsPIN)));
    if ( strlen(cfg7::gprsPIN) > 0 && stat != 3)
    {
        GSMmodem.unlockSIM(cfg7::gprsPIN);
    }

    Debug.print(F("Waiting for network..."));
    int res;
    if ( (res = GSMmodem.waitForNetworkConnection()) > 0)
    {
        debug_outln_info(F(" Fail. error code: ") + String(res));
        gsm_init_failed = true;
        return false;
    }

    debug_outln_info("Success.");

    // GPRS connection parameters are usually set after network registration
    debug_outln_info(F("GPRS \"APN\" connection parameter: "), cfg7::gprsapn);
    //debug_out(F("gprsConnect => set APN, User, Password value to open GPRS network connection to provider "), DEBUG_MIN_INFO);

    if( strlen(cfg7::gprsUser) > 0)
    {
        GSMmodem.setNetworkSettings(FPSTR(cfg7::gprsapn), FPSTR(cfg7::gprsUser), FPSTR(cfg7::gprsPass));
    }
    else
    {   //GSMmodem.setNetworkSettings(F(GPRSAPNCODE));
        GSMmodem.setNetworkSettings(FPSTR(cfg7::gprsapn));
    }

    if (cfg7::s7000_has_gps)
    { // Perform first-time GPS/data setup if the shield is going to remain on,
      // otherwise these won't be enabled in loop() and it won't work!
      // Enable GPS, first time take some time to start GPS process in sim7000 module

        while ( !GSMmodem.enableGPS() )
        {
            debug_outln_info(F("SETUP(): Failed to turn on GPS, retrying..."));
            delay(2000);                // Retry every 2sec.
        }

        debug_outln_info(F("GPS is Turned on."));
    }

    if (cfg::send2mqtt)
    {
        // ++ Set-Up Topic header for MQTT Broker.
        strcpy(mqtt_client_id, SSID_BASENAME);
		strcat(mqtt_client_id, esp_chipid.c_str());			// airRohr-<chipid>

        RESERVE_STRING(_header, MED_STR);
        _header = String(cfg::mqtt_topic) + "/" + String(mqtt_client_id);
        strcpy(mqtt_header, _header.c_str());

        _header += "/" + String(mqtt_lwt);
        strcpy(mqtt_lwt_header, _header.c_str());
        // -- Set-Up Topic header for MQTT Broker
    }

      // Set the network status LED blinking pattern while connected to a network (see AT+SLEDS command)
      //GSMmodem.setNetLED(true, 2, 64, 3000);        // on/off, mode, timer_on, timer_off
      //GSMmodem.setNetLED(false);                    // Disable network status LED

    wdt_reset();            // watchdog timer reset => nodemcu ESP8266 still alive.

    GPRSModemInfo();

    if (gsm_init_failed)
    {
        return false;
    }

    return true;

#else
    return true;
#endif
}

/*****************************************************************
    Get GPS Location. timeout = max. 5 sec.
    to fast now.
******************************************************************/
void GetGPSLocation(float *latitude, float *longitude, float *altitude, String &timestamp)
{
    if (gsm_init_failed)
    {
        debug_outln_info(F("GSM module (GPRS) \"NOT\" connected.. "));
        return;
    }

    debug_outln_info(F("Start positioning . Make sure to locate outdoors."));

    // turn on NMEA output
    // GSMmodem.enableGPSNMEA(255);

    // GPS function will be enabled.
    while (!GSMmodem.enableGPS())
    {
        debug_outln_info(F("LOOP(1): Failed to turn on GPS, retrying..."));
        delay(2000); // Retry every 2s
    }

    float heading = 0;
    float speed = 0;

    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t min = 0;
    uint8_t sec = 0;

    for (int cnt = 50; cnt > 0; cnt--)
    {
        if (GSMmodem.getGPS(latitude, longitude, &speed, &heading, altitude,
                            &year, &month, &day, &hour, &min, &sec))
        {
            debug_outln_info(F("The location has been locked, the latitude and longitude are:"));
            debug_outln_info(F("latitude: "), String(*latitude));
            debug_outln_info(F("longitude: "), String(*longitude));
            timestamp = String(year) + "-" + String(month) + "-" + String(day) + "-" + String(hour) + "-" + String(min) + "-" + String(sec) + ".000";
            debug_outln_info(F("Date time: "), timestamp);

            break;
        }

        delay(100); // max. 5 sec.
    }

    // GPS function will be disable.
    GSMmodem.disableGPS();
}

/// @brief
/// @param topic
/// @param payload
/// @return
uint8_t sendDataByMQTT(const char *topic, const char *payload)
{
    if (gsm_init_failed)
    {
        debug_outln_info(F("GSM module (GPRS) \"NOT\" connected.. "));
        return -100;
    }

    debug_outln_info(F("Start send Data By MQTT process."));

    GPRSConnect();

    // If not already connected, connect to MQTT.
    if (!GSMmodem.MQTT_connectionStatus())
    {
        uint32_t keepAlive = 60;

        // Set up MQTT parameters (see MQTT app note for explanation of parameter values)

        GSMmodem.MQTT_setParameter("CLIENTID", "airRohr_001"); // Client connection id.

        GSMmodem.MQTT_setParameter("URL", cfg::mqtt_server, cfg::mqtt_port); // MQTT_SERVER, MQTT_PORT

        // Set up MQTT username and password if necessary
        GSMmodem.MQTT_setParameter("USERNAME", cfg::mqtt_user); // MQTT_USERNAME
        GSMmodem.MQTT_setParameter("PASSWORD", cfg::mqtt_pwd);  // MQTT_PASSWORD

        // keepAlive = > 100 geeft dit een response "ERROR"
        GSMmodem.MQTT_setParameter("KEEPTIME", keepAlive); // Time to connect to server, 60s by default
        GSMmodem.MQTT_setParameter("CLEANSS", 1);
        GSMmodem.MQTT_setParameter("QOS", 1);

        String stmp = GSMmodem.MQTT_getParameters();

        debug_outln_info(F("Connecting to MQTT broker...") + stmp);

        if (!GSMmodem.MQTT_connect(true))
        {
            debug_outln_info(F("Failed to connect to broker!"));
            GSMmodem.MQTT_connect(false);
            return -1;
        }
    }
    else
    {
        debug_outln_info(F("Already connected to MQTT server!"));
    }

    // Now publish all the sensor values like PM and temperature data to their respective topics!
    // Parameters for MQTT_publish: Topic, payload (0-512 bytes), payload length, QoS (0-2), retain (0-1)
    if (!GSMmodem.MQTT_publish(topic, payload, strlen(payload), 1, 1))
    {
        debug_outln_info(F("Failed to publish!")); // Send GPS location
    }

    // Disconnect from MQTT
    GSMmodem.MQTT_connect(false);

    debug_outln_info(F("End send Data By MQTT process."));

    return 0;
}

/// @brief send data to rest api => By GSM -> LTE (4G)
/// @param logger 
/// @param str_JsonData = sensor data send with HTTP POST to a sever
/// @param pin 
/// @param host 
/// @param portnr 
/// @param url 
/// @return     total send time
int32_t sendDataByGSM(const LoggerEntry logger, const String &str_JsonData, const int pin,
                      const char *host, const int portnr, const char *url)
{
    if (gsm_init_failed)
    {
        debug_outln_info(F("GSM module (GPRS) \"NOT\" connected.. "));
        return 0;
    }

    unsigned long start_send = millis();

    debug_outln_info(F("-------- HTTP START -------------"));

    if (!GPRSConnect())
    {
        debug_outln_info(F("-------- HTTP connection ERROR (EINDE) -------------"));
        return 0;
    }

    if (GSMmodem.getBearerStatus() > 1)
    {
        // Bearer is closed.
        // disable data
        int reply = 5;
        while (--reply)
        {
            if (GSMmodem.enableGPRS(false))
            {
                break;
            }

            debug_outln_info(F("Failed to turn off"));
            delay(2000);
        }

        delay(200);

        // enable data
        if (!GSMmodem.enableGPRS(true))
        {
            debug_outln_info(F("Failed to turn on"));
        }
    }

    uint16_t statuscode;
    int16_t length;
    bool send_success = false;
    const __FlashStringHelper *contentType;

    RESERVE_STRING(addHeader, XLARGE_STR);
    char s_url[150];

    switch (logger)
    {
    case Loggeraircms:
        contentType = FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN);
        break;

    case LoggerInflux:
        contentType = FPSTR(TXT_CONTENT_TYPE_INFLUXDB);
        break;

    default:
        contentType = FPSTR(TXT_CONTENT_TYPE_JSON);
        break;
    }

    addHeader.clear();
    // format header: ('Content-Type: text/html; charset=utf-8');
    addHeader = F("Content-Type: ") + String(contentType);
    addHeader += F("; X-Sensor: ") + String(F(SENSOR_BASENAME)) + esp_chipid;
    addHeader += F("; X-MAC-ID: ") + String(F(SENSOR_BASENAME)) + esp_mac_id;
    //addHeader += F("; Connection: ") + String("Keep-Alive");                  // "Connection:" => geeft error. reuse ? F("keep-alive") : F("close");
    //addHeader += F("; Keep-Alive: ") + String("timeout=20, max=1000");        // 20 * 1000

    if (pin)
    {
        addHeader += (F("; X-PIN: ") + String(pin));
    }

    String s_userAgent = SOFTWARE_VERSION + String('/') + esp_chipid + String('/') + esp_mac_id;
    GSMmodem.setUserAgent(FPSTR(s_userAgent.c_str()));
    //GSMmodem.setHTTPSRedirect(true);                                            // redirect (ssl)

    // Post data to website
    sprintf(s_url, "%s:%d%s", host, portnr, url); // Format URI

    debug_outln_info(F("**** HTTP URL: "), s_url);
    debug_outln_info(F("**** HTTP header: "), FPSTR(addHeader.c_str()));
    debug_outln_info(F("**** HTTP body lengte: "), String(str_JsonData.length()));
    debug_outln_info(F("**** HTTP body: "), str_JsonData);

    // POST sensor data to ex. sensor.community server.
    if (!GSMmodem.HTTP_POST_start(s_url, FPSTR(addHeader.c_str()), (uint8_t *)str_JsonData.c_str(), str_JsonData.length(), &statuscode, (uint16_t *)&length))
    {
        debug_outln_info("POST Failed!");
        return 0;
    }

    if (statuscode >= HTTP_CODE_OK && statuscode <= HTTP_CODE_ALREADY_REPORTED)
    {
        debug_outln_info(F("Succeeded - "), String(host));
        send_success = true;
    }
    else // if (result >= HTTP_CODE_BAD_REQUEST)
    {
        debug_outln_info(F("Request failed with error: "), String(statuscode));
        //debug_outln_info(F("Details:"), http.getString());
    }

    debug_outln_info(F("\n  POST end ****"));

    // Http close
    GSMmodem.HTTP_POST_end();

	if (!send_success && statuscode != 0)
	{
		loggerConfigs[logger].errors++;
		last_sendData_returncode = statuscode;
	}

    debug_outln_info(F("-------- HTTP EINDE -------------"));

    return millis() - start_send;;
}

void enableNTPTimeSync()
{
    // enable NTP time sync
    // if ( !GSMmodem.enableNTPTimeSync(true, NTP_SERVER_1) )       //F("pool.ntp.org")
    // {
    //     Serial.println(F("Failed to enable"));
    // }
}

void getTime()
{
    // read the time
    // char buffer[23];

    // GSMmodem.getTime(buffer, 23); // make sure replybuffer is at least 23 bytes!
    // Serial.print(F("Time = "));
    // Serial.println(buffer);
}
