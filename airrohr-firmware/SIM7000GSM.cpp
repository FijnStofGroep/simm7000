/*
 * @file SIM7000GSM.cpp
 *
 * Written by R.Dieperink, Rolenco Leusden.
 * Date: 2024-04-26
 *
 * Version: 1.0.13
 *
 * Copyright (C) 2024 ~ 2025
 * 
 * GPRS stands for General Packet Radio Service:
 *  - is a technology that enables mobile devices to access data services over a cellular network.
 *  - It is the modified version of GSM architecture. GPRS is a packet-oriented mobile data mechanism, 
 *    that can carry data packets as well.
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

// see "SOFTWARE_VERSION_STR" in airrohr-firmware.ino file.
extern String SOFTWARE_VERSION;

extern int last_sendData_returncode;
extern String esp_chipid;
extern String esp_mac_id;

extern int last_signal_strength;
extern unsigned long WiFi_error_count;
extern unsigned long last_page_load;

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
#define    HTTP_CODE_REQUEST_TIMEOUT   408

// internal defines.
unsigned long m_starttime;
boolean m_statusSend = true;     // force to send status message to MQTT broker.

unsigned long last_status_attempt = 0;
unsigned long wait_NTP_sync_time = ONE_DAY_IN_MS;

u_int32_t m_cnt_LTE_Restarts = 0;
bool lte_init_failed = false;
char m_imei[16] = {0};                        // Use this for LTE modem device ID.

/*
    BK-SIM70XX settings.
*/
namespace cfg7
{
    bool s7000_has_gps = HAS_GPS;

    char gprsapn[LEN_SIMM7000];
    char gprsUser[LEN_SIMM7000];
    char gprsPass[LEN_SIMM7000];
    // set GSM PIN, if any (#define GSM_PIN "")
    char gprsPIN[LEN_SEN5X_SYM];

    // 1 = SIM7000_RXD5_TXD6    (default)
	// 2 = SIM7000_RXD6_TXD5
	// 3 = SIM7080_RXD5_TXD6
	// 4 = SIM7080_RXD6_TXD5
	unsigned sim_type = 1;

    // 2  - Automatic
    // 13 - GSM only
    // 38 - LTE only            (default)
    // 51 - GSM and LTE only
	unsigned mode_selection = 38;        

    // 1 - CAT-M                (default)
    // 2 - NB-IoT
    // 3 - CAT-M and NB-IoT
    unsigned communication_type = 1;

    // init: set default values to options.
	void initNonTrivials()
    {
        strcpy_P(gprsapn, GPRSAPNCODE);
        strcpy_P(gprsUser, WWW_USERNAME);
        strcpy_P(gprsPass, WWW_PASSWORD);
        
        strcpy_P(gprsPIN, SIM7_PIN);
    }
}

// NodeMCU ESP8266 Serial port instance. (set baudrate, data lenght, ...)
// Serial instance for Communication between nodeMCU and BK-Sim70XX PCB.
SoftwareSerial SerialSIM;

// LTEmodem instance of SIM70XX module. (BK-SIM70XX development PCB)
BK_modem * LTEmodem = NULL; 

//***************************************************************************************************************************************************

/*****************************************************************
 *   GPRS Modem Info()                                           *
******************************************************************/
void Display_GPRSModemInfo()
{
    // if (cfg::debug < DEBUG_MAX_INFO)
    // {
    //     // String local = LTEmodem.getGPRSIP();
    //     // debug_outln_verbose(F("Local IP: "), local);

    //     String oper = LTEmodem->getOperator();
    //     debug_outln_verbose(F("Operator: "), oper);

    //     last_signal_strength = GetWiFi_RSSI();

    //     return;
    // }

    int16_t imode;
    RESERVE_STRING(smode, MED_STR);

    debug_outln_verbose(F("\n--- Display GPRS Information ---"));

    if (LTEmodem->isGprsConnected())
    {
        debug_outln_verbose(F("GPRS status: connected."));
    }
    else
    {
        debug_outln_verbose(F("GPRS status: not connected."));
    }

    debug_outln_verbose(F("Software ") + LTEmodem->getModemSoftware_Revision());

    imode = LTEmodem->getNetworkMode();
    debug_outln_verbose(F("Network Modes: \"2 Automatic , 13 GSM only , 38 LTE only , 51 GSM and LTE only\".\n\t\tNetwork Mode => "), String(imode));

    uint8_t epsStatus = LTEmodem->getNetworkStatus();
    debug_out(F("\t\tNetwork status code: ") + String(epsStatus) + F(" => "), DEBUG_MED_INFO);

    if (epsStatus == 0) 
        debug_outln_verbose(F("Not registered"));
    else if (epsStatus == 1) 
        debug_outln_verbose(F("Registered (home)"));
    else if (epsStatus == 2) 
        debug_outln_verbose(F("Not registered (searching)"));
    else if (epsStatus == 3) 
        debug_outln_verbose(F("Denied"));
    else if (epsStatus == 4) 
        debug_outln_verbose(F("Unknown"));
    else if (epsStatus == 5) 
        debug_outln_verbose(F("Registered roaming"));

    char status[13];
    imode = LTEmodem->getNetworkSystemMode(status);
    debug_outln_verbose(F("\t\tNetwork System Mode: "), String(imode) + F(" => ") + String(status));

    smode = LTEmodem->getPreferredModes();
    imode = LTEmodem->getPreferredMode();
    debug_outln_verbose("Availlable Preferred Modes: " + smode + ".\n\t\tCurrent Preferred Mode = " + String(imode));

    char ccid[64];
    LTEmodem->getSIMCCID(ccid);
    debug_outln_verbose(F("CCID: "), String(ccid));

    LTEmodem->getIMEI(m_imei);
    debug_outln_verbose(F("IMEI: "), String(m_imei));

    smode.clear();
    smode = LTEmodem->getOperator();
    debug_outln_verbose(F("Operator: "), smode);

    smode.clear();
    smode = LTEmodem->getSIMCOMATI();
    debug_outln_verbose(F("SIMCOMATI:\n"), smode);
    
    last_signal_strength = GetWiFi_RSSI();

    // Get connection type and band.
    smode.clear();
    LTEmodem->getNetworkInfo(smode);
    debug_outln_verbose(F("The current network parameters: "), smode);

    debug_outln_verbose(F("--- End GPRS Display Information ---\n"));
}

/// @brief 
///         getSignalQuality(): Returns the RSSI value in dBm (decibels relative to 1 milliwatt).
///             - Closer to 0: Stronger signal (e.g., -30 dBm is excellent).
///             - Lower values: Weaker signal (e.g., -90 dBm is poor). 
///
/// @return : RSSI value.
int32_t GetWiFi_RSSI( void)
{
    if (lte_init_failed)
    {
        debug_outln_verbose(F("RSSI: LTE module (GPRS) \"NOT\" connected.."));
        return 0;
    }

    uint8_t csq;
    int8 rssi = 0; 
    LTEmodem->getSignalQuality(&csq, &rssi);

    debug_outln_verbose( F("LTE signal strength: ") + String(rssi) + F("dBM") + F(", Signal quality: ") + String(csq) + F("%"));

    return rssi;
}

/// @brief 
/// @return : Sim7000 LTE module IP address.
String GetLTELocalIP(void)
{
    if (lte_init_failed)
    {
        debug_outln_info(F("IP: LTE module (GPRS) \"NOT\" connected.."));
        return String("0.0.0.0");
    }

    return LTEmodem->getGPRSIP();
}

/// @brief 
/// @param  
/// @return 
String GetSimDriverName(void)
{
    return LTEmodem->Name();
}

/// @brief Software Restart LTE modem counter.
/// @param  
/// @return 
u_int32_t GetLTE_RestartCounter(void)
{
    return m_cnt_LTE_Restarts;
}


/// @brief BK-SIM70XX PCB Power OFF.
void modemPowerOff()
{
    if(LTEmodem != NULL)
    {
        LTEmodem->modemPowerOff();
    }
}

/// @brief Restart BK-SIM70XX LTE modem.
bool RestartLTEModem()
{
    if (LTEmodem != NULL)
    {
        debug_outln_info(F("Restart LTE Modem SIM70xx firmware process."));

        // LTEmodem->modemPowerRestart();
        LTEmodem->modemPowerOff();

        m_cnt_LTE_Restarts++;

        return Sim7000_setup(SETUP_STATE::RESTART);
    }
    else
    {
        return true;
    }
}

/// @brief 
/// @return
inline boolean GPRSConnect()
{
    debug_outln_info(F("GPRS Wait for network connection..."));

    // Connect to cell network and verify connection
    // If unsuccessful, retrying max 3 times with 2 sec. delay, to a connection is made.
    int retry = 1;

    while (!LTEmodem->isNetworkConnected())
    {
        if (--retry < 0)
        {
            debug_outln_info(F("Failed to connect to LTE network: restart connection with SIM70xx module..."));

            if( GetWiFi_RSSI() == 0)
            {
                WiFi_error_count++;
            }

            if ( retry < -1 || !RestartLTEModem())
            {
                return false;
            }

            continue;
        }

        debug_outln_info(F("Failed to connect to LTE network: retrying..."));
        delay(2000);
    }

    debug_outln_verbose(F("Connected to LTE network!"));

    // Disable data just to make sure it was actually off so that we can turn it on
    // LTEmodem.openWirelessConnection(false);

    // Open wireless connection if not already activated.
    if (!LTEmodem->wirelessConnStatus())
    {
        retry = 5;

        while (retry > 0 && !LTEmodem->openWirelessConnection(true))
        {
            int16_t error = LTEmodem->getCME_ErrorCode();
            debug_outln_info(F("Failed to Open Wireless Connection, retrying... Error code = ") + String(error));

            if(error > 750 && error < 800)
            {// CME ERROR: 751 till 799 are AT command syntax/missing prameters.
                debug_outln_info(F("This could be happend when wrong SIM70x0 driver selected."));
                return false;
            }
            
            delay(2000); // Retry every 2s

            retry--;
        }

        if (retry == 0)
        {
            LTEmodem->openWirelessConnection(false);
            WiFi_error_count++;
            return false;
        }

        debug_outln_verbose( F("GPRS-IP address: ") + LTEmodem->getGPRSIP());

        debug_outln_info(F("GPRS connection Enabled."));

        wdt_reset();        // watchdog timer reset => nodemcu ESP8266 still alive.
    }
    else
    {
        debug_outln_info(F("GPRS connection already enabled."));
    }

    return true;
}

/// @brief : Open GPRS / LTE Network.
/// @param  
/// @return : true = connected, false = NOT connected.
boolean OpenGPRSNetwork(void)
{
    if (!GPRSConnect())
    {
        debug_outln_info(F("--- GPRS connection ERROR ---"));
        return false;
    }

    int res;
    if ((res = LTEmodem->getBearerStatus()) > 1)
    {
        if (res == 2)
        { // bearer is closing.
            delay(2000);
        }

        // Bearer is closed.
        // enable data
        if (!LTEmodem->enableGPRS(true))
        {
            debug_outln_info(F("--- GPRS Failed to turn on ---"));
        }
    }

    return true;
}

/**************************************************************************************************************
    ESP8266 serial speed to BK-SIM70XX, default baud rate is 115200 bps.

    NOTE: Software serial is not reliable on 115200 baud and therefore changes it to a lower value.
          9600 works well in almost all applications.

          But 115200 works great with Hardware serial pins.

***************************************************************************************************************/
/// @brief :
/// @param : state INIT
///                RESTART
/// @return 
bool Sim7000_setup( int state)
{
#if defined(ESP8266)

#ifndef BK_MODEM_DEBUG
    debug_outln_info(F("BK_Sim7000 Modem connect start process:"));
#else
    debug_outln_info(F("Start SIM-Modem connect process: type = ") + String(cfg7::sim_type));
#endif

    if(LTEmodem != NULL )
    {
        debug_outln_info( F("Remove from Heap-memory: SIM70XX Driver instance = ") + LTEmodem->Name());
        delete LTEmodem;        // clean-up all "BK_modem" resources.
        LTEmodem = NULL;
    }

    uint16_t rx = 0;
    uint16_t tx = 0;

    if (LTEmodem == NULL)
    {
        switch (cfg7::sim_type)
        {
        default:
        case 1:
            rx = SIM_PIN_RX;
            tx = SIM_PIN_TX;
            goto exit_7000;

        case 2:
            rx = SIM_PIN_SRX;
            tx = SIM_PIN_STX;

        exit_7000:
            LTEmodem = new BK_modem_7000(); // create LTEmodem 7000 instance.
            break;

        case 3:
            rx = SIM_PIN_RX;
            tx = SIM_PIN_TX;
            goto exit_7080;

        case 4:
            rx = SIM_PIN_SRX;
            tx = SIM_PIN_STX;

        exit_7080:
            LTEmodem = new BK_modem_7080(); // create LTEmodem 7080 instance.
            break;
        }
    }

    debug_outln_info( F("\tSIM70XX Driver Instance = ") + LTEmodem->Name());

    debug_outln_info(F("Set-Up communication with BK_SIM70XX Modem PCB..."));

    // initialize LTEmodem settings.
    LTEmodem->init(SerialSIM, Debug, SIM_PIN_PWR);

    LTEmodem->LTE_modem_PowerUp();

    // set RS-232 port settings between NodeMCU-ESP8266 and BK-SIM70XX module.
    SerialSIM.begin(SERIALSIM_BAUD, SWSERIAL_8N1, rx, tx); // start with default SIM70XX shield baudrate.
    delay(50);

    // Set BK-SIM70XX LTE-modem baudrate to lower value.
    LTEmodem->setBaudrate(LTEMODEM_BAUD);

    // set NodeMCU serial port to same baudrate.
    SerialSIM.begin(LTEMODEM_BAUD, SWSERIAL_8N1);
    delay(50);

    debug_outln_info(F("Initializing BK_SIM70XX Modem PCB..."));

    if (!LTEmodem->begin())
    {
        debug_outln_info(F("LTE Modem init Failed.."));
        lte_init_failed = true;
        return false;
    }

    // Flush: SerialSIM receive buffer.
    LTEmodem->flush();

    String name = LTEmodem->getModemName();
    debug_outln_info(F("Modem Name: "), name);

    String modemInfo = LTEmodem->getModemInfo();
    debug_outln_info(F("LTE Modem Info: "), modemInfo);

    // Unlock your SIM card with a PIN if needed
    int8_t stat = LTEmodem->getPINStatus();
    debug_outln_info(F("PIN Status: "), String(stat));
    // debug_outln_info(F("GPRS PIN: "), String(strlen(cfg7::gprsPIN)));

    if ( strlen(cfg7::gprsPIN) > 0 && stat != 3)
    {
        LTEmodem->unlockSIM(cfg7::gprsPIN);
    }

    if( stat < PinStatus::SIM_READY)    
    {// Pin SIM_ERROR.
        if( stat == PinStatus::SIM_ERROR)
        {
            stat = (int8_t)LTEmodem->getCME_ErrorCode();
        }

        debug_outln_info(F("Pin-Status Error code: ") + String(stat));
        lte_init_failed = true;
        return false;
    }

    Debug.print(F("Waiting for network provider..."));

    RegStatus epsStatus = (RegStatus)LTEmodem->getNetworkStatus();
    if (epsStatus == RegStatus::REG_DENIED || epsStatus == RegStatus::REG_UNKNOWN)
    {
        debug_outln_info(F("Failed error code: ") + String(epsStatus));
        lte_init_failed = true;

        return false;
    }

    debug_outln_info(F(" Success."));

    // GPRS connection parameters are usually set after network registration
    debug_outln_info(F("GPRS \"APN\" config parameter: "), cfg7::gprsapn);
    //debug_out(F("gprsConnect => set APN, User, Password value to open GPRS network connection to provider "), DEBUG_MIN_INFO);

    if( strlen(cfg7::gprsUser) > 0)
    {
        LTEmodem->setNetworkSettings(FPSTR(cfg7::gprsapn), FPSTR(cfg7::gprsUser), FPSTR(cfg7::gprsPass));
    }
    else
    {   //LTEmodem.setNetworkSettings(F(GPRSAPNCODE));
        LTEmodem->setNetworkSettings(FPSTR(cfg7::gprsapn));
    }

    LTEmodem->setPreferredMode(cfg7::mode_selection );                      // 38 = LTE only.
    LTEmodem->setPreferredLTEMode(cfg7::communication_type );               // 1 = LTE CAT-M, not NB-IoT
    //LTEmodem->setOperatingBand("CAT-M", 12);                              // ex. AT&T uses band 12.

    if (cfg7::s7000_has_gps)
    { // Perform first-time GPS/data setup if the shield is going to remain on,
      // otherwise these won't be enabled in loop() and it won't work!
      // Enable GPS, first time take some time to start GPS process in sim70xx module.
      // max 60 sec.

        for(int reply = 30; reply > 0; reply--)
        {
            if( LTEmodem->enableGPS() )
            {
                debug_outln_info(F("GPS is Turned on."));
                break;
            }
            
            debug_outln_info(F("Failed to turn-on GPS, retrying..."));
            delay(2000);                        // Retry every 2sec.
        }
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
      //LTEmodem.setNetLED(true, 2, 64, 3000);        // on/off, mode, timer_on, timer_off
      //LTEmodem.setNetLED(false);                    // Disable network status LED

    lte_init_failed = false;

    wdt_reset();                                      // watchdog timer reset => nodemcu ESP8266 still alive.

    if( state == SETUP_STATE::INIT)
    {
        Display_GPRSModemInfo();
    }

    wait_NTP_sync_time = 15000;                       // wait 15 sec. before the first call to "setNTPTimeSync();""
    m_starttime = millis();                           // set the start time for get new NTP time.

    return true;

#else
    return true;
#endif
}

/*****************************************************************
    Get GPS Location. timeout = max. 5 sec.
    to fast now.
******************************************************************/
/// @brief : Get GPS Location.
/// @param latitude 
/// @param longitude 
/// @param altitude 
/// @param timestamp 
/// @return 
boolean GetGPSLocation(float *latitude, float *longitude, float *altitude, String &timestamp)
{
    if (lte_init_failed)
    {
        debug_outln_info(F("GPS module \"NOT\" connected.."));
        return false;
    }

    debug_outln_info(F("Start positioning. Make sure that sensor locate is outdoors."));

    // turn on NMEA output.
    //LTEmodem.enableGPSNMEA(255);

    // GPS function will be enabled.
    int reply = 10;
    while (--reply > 0)
    {
        if(LTEmodem->enableGPS())
        {
            break;
        }

        debug_outln_info(F("Failed to turn on GPS, retrying..."));
        delay(2000);                    // Retry every 2sec.
    }

    if(reply == 0)
    {
        return false;
    }

    float heading = 0;
    float speed = 0;

    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t min = 0;
    uint8_t sec = 0;
    bool gpsOke = false;

    for (int cnt = 10; cnt > 0; cnt--)
    {
        if (LTEmodem->getGPS(latitude, longitude, &speed, &heading, altitude,
                            &year, &month, &day, &hour, &min, &sec))
        {
            char gps_timestamp[25] = {0};
            sprintf_P(gps_timestamp, PSTR("%04d-%02d-%02dT%02d:%02d:%02d.000"),year, month, day, hour, min, sec);
            timestamp = String(gps_timestamp);

            debug_outln_info(F("GPS location has been locked..."));

            if (cfg::debug == DEBUG_ENGINEER_INFO)
            {
                debug_outln_info(F("GPS data:"));
                debug_outln_info(F("latitude: "), String(*latitude, 8));
                debug_outln_info(F("longitude: "), String(*longitude, 8));
                debug_outln_info(F("Altitude:: "), String(*altitude, 4));
                debug_outln_info(F("Date time: "), timestamp);
            }

            gpsOke = true;
            
            break;
        }

        if( cnt == 1)
        {// last try.
            debug_outln_info(F("Could NOT locked a GPS satellite...."));
            continue;
        }

        delay(2000);
    }

    // GPS function will be disable.
    // not good idea to do that, make system slow.
    //LTEmodem.disableGPS();

    return gpsOke;
}

/// @brief : send sensor data, status message one /hour, LWT only by start-up
/// @param : topic
/// @param : payload
/// @return
boolean sendDataByMQTT(const char *topic, const char *payload)
{
    if (lte_init_failed)
    {
        debug_outln_info(F("MQTT: LTE module (GPRS) \"NOT\" connected.. "));
        return false;
    }

    char *ptr = strstr(topic, "/sensor");
    if (ptr == 0)
    {
        if (m_statusSend)
        {// send status data to MQTT server.
            last_status_attempt = act_milli;
            m_statusSend = false;
        }
        else
        {
            ptr = strstr(topic, "/status");
            if (ptr > 0)
            {// send "status" one time/8-hours.
                if ((act_milli - last_status_attempt) < ONE_DAY_IN_MS / 3 ) // 60000 * 60
                {
                    return true;
                }

                last_status_attempt = act_milli;
            }
            else
            {// no MQTT action.
                return true;
            }
        }
    }

    debug_outln_info(F("Start send Data By MQTT process."));

    if (!OpenGPRSNetwork())
    {
        debug_outln_info(F("--- MQTT connection ERROR (EINDE) ---"));

        m_statusSend = true;                     //Restart sending MQTT data
        return false;
    }

    // If not already connected, connect to MQTT.
    if (!LTEmodem->MQTT_connectionStatus())
    {
        uint32_t keepAlive = 60;

        // Set up MQTT parameters (see MQTT app note for explanation of parameter values)

        LTEmodem->MQTT_setParameter("CLIENTID", "airRohr_001"); // Client connection id.

        LTEmodem->MQTT_setParameter("URL", cfg::mqtt_server, cfg::mqtt_port); // MQTT_SERVER, MQTT_PORT

        // Set up MQTT username and password if necessary
        LTEmodem->MQTT_setParameter("USERNAME", cfg::mqtt_user); // MQTT_USERNAME
        LTEmodem->MQTT_setParameter("PASSWORD", cfg::mqtt_pwd);  // MQTT_PASSWORD

        // keepAlive = > 100 geeft dit een response "ERROR"
        LTEmodem->MQTT_setParameter("KEEPTIME", keepAlive); // Time to connect to server, 60s by default
        LTEmodem->MQTT_setParameter("CLEANSS", 1);
        LTEmodem->MQTT_setParameter("QOS", 1);

#ifdef VS_DEBUG
#ifdef BK_MODEM_DEBUG
        debug_outln_info(F("Connecting to MQTT broker...\nDriver Parameters:\n")); // Display is done by BK_SIM7000 driver.
        LTEmodem->MQTT_getParameters();
#else
        debug_outln_info(F("Connecting to MQTT broker...\nMQTT Parameters:\n") + LTEmodem->MQTT_getParameters());
#endif
#else
        debug_outln_info(F("Connecting to MQTT broker..."));
#endif

        //uint16_t timeOut = cfg7::sim_type[0] == '2' ? 35000 : 15000;

        if (!LTEmodem->MQTT_connect(true))
        {
            debug_outln_info(F("Failed to connect to MQTT broker!"));

            uint16_t error = LTEmodem->getCME_ErrorCode();

            if (error != 3)
            { // MQTT broker could not made connection.
                
                LTEmodem->MQTT_connect(false);

                m_statusSend = true;             //Restart sending MQTT data
                return false;
            }
     
            // MQTT broker name not correct. check it.
            return true;
        }
    }
    else
    {
        debug_outln_info(F("Already connected to MQTT Broker Server!"));
    }

    // Now publish all the sensor values like PM and temperature data to their respective topics!
    // Parameters for MQTT_publish: Topic, payload (0-512 bytes), payload length, QoS (0-2), retain (0-1)
    if (!LTEmodem->MQTT_publish(topic, payload, strlen(payload), 1, 1))
    {
        debug_outln_info(F("Failed to publish!")); // Send GPS location
    }

    LTEmodem->MQTT_connect(false);

    debug_outln_info(F("End sending Data to MQTT Broker."));

    return true;
}

/// @brief send data to rest api => By LTE (4G)
/// @param logger 
/// @param str_JsonData = sensor data send with HTTP POST to a sever
/// @param pin 
/// @param host 
/// @param portnr 
/// @param url 
/// @return     total send time
int32_t sendDataByLTE(const LoggerEntry logger, const String &str_JsonData, const int pin,
                      const char *host, const int portnr, const char *url)
{
    if (lte_init_failed)
    {
        debug_outln_info(F("API's: LTE module (GPRS) \"NOT\" connected.. "));
        return 0;
    }

    unsigned long start_send = millis();

    debug_outln_info(F("-------- HTTP START -------------"));

    if (!OpenGPRSNetwork())
    {
        debug_outln_info(F("-------- HTTP connection ERROR (EINDE) -------------"));
        return 0;
    }

    debug_outln_info(F("-------- HTTP process under construction -------------"));
    return millis() - start_send;

    uint16_t statuscode = -1;
    int16_t respLength;
    bool send_success = false;
    const __FlashStringHelper *contentType;

    RESERVE_STRING(addUserHeader, XLARGE_STR);
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
        contentType = FPSTR(TXT_CONTENT_TYPE_JSON);    // ('Content-Type: text/html; charset=utf-8') or application/json; charset=utf-8
        break;
    }

    addUserHeader.clear();

    // SIM7000 firmware can't handle these settings.
    //addUserHeader = F("Connection:") + String(F("close"));                 // reuse ? F("keep-alive") : F("close");
    //addUserHeader += F("Keep-Alive:") + String("timeout=20, max=1000");    // 20 * 1000
    //addUserHeader += F("cache-control:") + String(F("no-cache"));
    //addUserHeader += F("Authorization: Bearer Fijnstof_19570913-191800;");

    addUserHeader += F("X-Sensor: ") + String(F(SENSOR_BASENAME)) + esp_chipid + F(";");
    addUserHeader += F("X-MAC-ID: ") + String(F(SENSOR_BASENAME)) + esp_mac_id + F(";");

    if (pin)
    {
        addUserHeader += F("X-PIN: ") + String(pin) + F(";");
    }
   
    String s_userAgent = SOFTWARE_VERSION + String('/') + esp_chipid + String('/') + esp_mac_id;
    LTEmodem->setUserAgent(FPSTR(s_userAgent.c_str()));
    //LTEmodem.setClientID( 1 );                                               // 
    //LTEmodem.setHTTPSRedirect(true);                                         // redirect (ssl) ???

    // Post data to website.
    if (portnr == 80)
    {
        sprintf(s_url, "http://%s:%d%s", host, portnr, url);                   // HTTP Format URI
    }
    else
    {
        sprintf(s_url, "https://%s:%d%s", host, portnr, url);                  // HTTPS Format URI
    }

    debug_outln_info(F("**** URL: "), s_url);
    debug_outln_info(F("**** Header: "), F("Content-Type: ") + String(contentType) + String(" ") + FPSTR(addUserHeader.c_str()));
    debug_outln_info(F("**** Body lengte: "), String(str_JsonData.length()));
    debug_outln_info(F("**** Body: "), str_JsonData);

    //addUserHeader.replace(";","\r\n");
    //String contentTmp = String(contentType) + String("\r\n") + addUserHeader + String("\r\n");

    // POST sensor data to ex. sensor.community server.
    if ( !LTEmodem->HTTP_POST_start(s_url, contentType,                  // FPSTR(contentTmp.c_str()),
                                   addUserHeader, 
                                   (uint8_t *)str_JsonData.c_str(), str_JsonData.length(), 
                                   &statuscode, (uint16_t *)&respLength) )
    {
        debug_outln_info("POST Failed!");
        //return 0;
    }

    if (statuscode >= HTTP_CODE_OK && statuscode <= HTTP_CODE_ALREADY_REPORTED)
    {
        debug_outln_info(F("Succeeded - "), String(host));
        send_success = true;
    }
    else // if (result >= HTTP_CODE_BAD_REQUEST)
    {
        debug_outln_info(F("Request failed with error: "), String(statuscode));
        debug_outln_info(F("Details: "), LTEmodem->getResponseMessage());
    }

    debug_outln_info(F("POST end ****"));

    // Http close
    LTEmodem->HTTP_POST_end();

	if (!send_success && statuscode != 0)
	{
		loggerConfigs[logger].errors++;
		last_sendData_returncode = statuscode;
	}

    debug_outln_info(F("-------- HTTP EINDE -------------"));

    return millis() - start_send;
}

/// @brief : sec = 1724850620 => Wed Aug 28 15:10:20 2024
/// @param  : NTP server : 2.pool.ntp.org
void setNTPTimeSync(void)
{
    if ( lte_init_failed || !OpenGPRSNetwork())
    {
        debug_outln_info(F("--- NTP network \"NOT\" connected ---"));
        return;
    }

     // enable NTP time sync. + time zone.
     // NTP servers operate always in UTC time.(ipv4)
    if ( !LTEmodem->enableNTPTimeSync(true, FPSTR((String(NTP_SERVER_2)).c_str()), 1) )
    {
        debug_outln_info(F("--- Failed to enable NTP. ---"));
        return;
    }

    char timeBuffer[25];
    LTEmodem->getTime(timeBuffer, 24); // "24/08/28,11:21:26+04"

    debug_outln_info(F("Raw NTP Date_Time value: ") + String(timeBuffer));

    timeBuffer[3] = 0x00;
    if(atoi(&timeBuffer[1]) == 80)
    {// return value = base date/time.
        wait_NTP_sync_time = cfg::sending_intervall_ms + 5000;
        return;                        // NTP server timed out.
    }

    // set terminator char. for each field.
    //timeBuffer[3] = 0x00;
    timeBuffer[6] = 0x00;
    timeBuffer[9] = 0x00;
    timeBuffer[12] = 0x00;
    timeBuffer[15] = 0x00;
    timeBuffer[18] = 0x00;

    struct tm tmStruct;
    tmStruct.tm_year = atoi(&timeBuffer[1]) + 100; // = year 2024  (base 1900)
    tmStruct.tm_mon = atoi(&timeBuffer[4]);        // = month
    tmStruct.tm_mday = atoi(&timeBuffer[7]);       // = 28th day
    tmStruct.tm_hour = atoi(&timeBuffer[10]);      // = 11 hours
    tmStruct.tm_min = atoi(&timeBuffer[13]);       // = 21 minutes
    tmStruct.tm_sec = atoi(&timeBuffer[16]);       // = 26 secs

    if (tmStruct.tm_mon > 2 && tmStruct.tm_mon < 11)
    {// Day light Saving Time On, from April till October
        tmStruct.tm_isdst = 1;                     // Tells mktime() the input date is in day light saving time.
    }
    else
    {// Day light Saving Time Off, January, February, March and November, December are out.
        tmStruct.tm_isdst = 0;                     // Tells mktime() the input date is NOT in day light saving time.
    }

    setTZ(MY_TZ);                                  // set Timezone: Europe/Amsterdam.

#if defined(VS_DEBUG)
    char tmBuffer[90];
    sprintf_P(tmBuffer, PSTR("%d-%d-%d %d:%d:%d+dl%d"), 
                                                    tmStruct.tm_year - 100,
                                                    tmStruct.tm_mon,
                                                    tmStruct.tm_mday,
                                                    tmStruct.tm_hour,
                                                    tmStruct.tm_min,
                                                    tmStruct.tm_sec,
                                                    tmStruct.tm_isdst);
    debug_outln_info(F("tmStruct: ") + String(tmBuffer));
 #endif

    // function to parse a datetime to ticker value. (1724850620 => Wed Aug 28 15:10:20 2024)
    tmStruct.tm_mon -= 1;                           // = month-1 (jan = 0)
    time_t parsedTime = mktime(&tmStruct);
    
    //debug_outln_info(F("NTP ticker value: ") + String(parsedTime));

    // struct tm* timeinfo = localtime(&parsedTime);
    // char buffer[90];
    // strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    // debug_outln_info(F("NTP format value: ") + String(buffer));

    struct timeval tv;
    tv.tv_sec = parsedTime;
    tv.tv_usec = 0;

    // Set internal timer value.
    settimeofday(&tv,NULL);

    wait_NTP_sync_time = ONE_DAY_IN_MS;

    m_statusSend = true;                     // Resend MQTT date/time status payload message.
}

/// @brief : sync NTP time one time a day.
/// @param
void SyncNTPTime(void)
{
    if ((act_milli - m_starttime) > wait_NTP_sync_time)
    {
        if (wait_NTP_sync_time == ONE_DAY_IN_MS)
        {
            debug_outln_info(F("Restart SIM70xx PCB firmware."));

            RestartLTEModem();              // Restart SIM70xx PCB firmware every day.
            return;
        }

        debug_outln_info(F("Start: Sync NTP Date/Time process."));
        setNTPTimeSync();

        // set new date/time value.
        m_starttime = millis();
    }
}

/// @brief : ESP8266 Turn off WiFi to save power
///          Wifi APmode PowerSave in case of SIM7xxx
///    
/// @param
void LTEmodePowerSave(void)
{
    if( lte_init_failed)
    {// SIM-7xxx module NOT connected.
        return;
    }

    // TODO: must be tested, for now skiped.
    // if (false && (last_page_load > -1UL) && (millis() - last_page_load) > cfg::time_for_wifi_config + 500)
    // { // after 10 minutes waiting on server commando's => stop WIFI process.
    //     debug_outln_info(F("Disconnecting AP mode and stop WIFI process."));

    //     WiFi.softAPdisconnect(true);
    //     WiFi.mode(WIFI_OFF);

    //     // Disable WiFi
    //     wifi_station_disconnect();
    //     wifi_set_opmode(NULL_MODE);
    //     wifi_set_sleep_type(MODEM_SLEEP_T);
    //     wifi_fpm_open();
    //     wifi_fpm_do_sleep(ESP.deepSleepMax());          // FPM_SLEEP_MAX_TIME

    //     if( WiFi.status() == WL_CONNECTED)
    //     {
    //         debug_outln_info(F("Wifi Connection is still alive..."));
    //     }
    //     else
    //     {
    //         debug_outln_info( F("Wifi Connection successfully terminated."));
    //     }

    //     last_page_load = -1UL;
    // }
}
