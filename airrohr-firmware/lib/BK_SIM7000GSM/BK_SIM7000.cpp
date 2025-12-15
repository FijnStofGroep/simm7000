
/*********************************************************************************************************************
 * @file BK-SIM7000.cpp
 *
 * Written by R.Dieperink, Rolenco Leusden
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
 * BK_SIM7000 library for GPRS/GSM modules, that just works.
 * GPRS stands for "General Packet Radio Service". 
 * It’s a mobile data standard that enhances the existing GSM (Global System for Mobile Communications) network. 
 * 
 * Support SIM7000E/G GSM, LTE, and WiFi modules with AT command interfaces.
 * based on Adafruit_FONA.
 *
 *  AND Technologies Co., ltd, Breakout SIM7000 PCB board
 *  https://www.and-global.com
 *  https://www.and-global.com/sim7000e-breakout-board-sim7000-core-kit-with-nb-and-gps-antenna.html
 *	see file: BK-SIM7000E DOC-V2.0.zip
 *
 *---------------------------------------------------------------------------------------------------------------------
 *
 * Adaptation Botletics hardware
 * Original text below:
 *
 * These displays use TTL Serial to communicate, 2 pins are required to interface
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing products from Adafruit!
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * BSD license, all text above must be included in any redistribution
 *
 *********************************************************************************************************************/

#include "BK_SIM7000.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-overflow"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wattributes"

#ifdef BK_SSL // #ifdef BK_SIM7000_SSL
char *m_server_CA;
uint16_t m_port_CA = 0;
int m_CID_CA = 0;
char *m_rootCA;
#endif

#pragma region "BK_MODEM"

//********************************* BK_modem *********************************************************************************** */
/* 
  constructor class 'BK_modem'
*/
BK_modem::BK_modem()
{
    m_apn = F("");
    m_apnusername = 0;
    m_apnpassword = 0;
    SerialSIM = NULL;
    m_httpsredirect = false;
    m_https_SSL = 0;
    m_useragent = F("Fijnstof_Leusden");
    m_clientID = 1;                         // HTTP session id.

    m_PIN_PWR = 0;
    m_ok_reply = F("OK");
}

/*
  Destructor: clean-up all resource of this class.
*/
BK_modem::~BK_modem()
{
}

/// @brief
///    ESP8266 serial speed to SIM7000 = Default baud rate is 115200 bps
///
///    NOTE: Software serial is not reliable on 115200 baud and therefore changes it to a lower value.
///          9600 works well in almost all applications, but 115200 works great with Hardware serial.
///
/// @param comPort
/// @param DebugInfo => Serial port for send log data.
/// @param pin_pwr
/// @return
void BK_modem::init(BK_SIM7000_StreamType &comPort, HardwareSerial &debugPort, uint8_t pin_pwr)
{
    if (SerialSIM == NULL)
    {
        SerialSIM = &comPort;
    }

#ifdef BK_MODEM_DEBUG
    // Set DebugStream instance to Stream output to use for debug log info.
    if( DebugStream == NULL)
    {
        DebugStream = &debugPort;
    }
#endif

    m_PIN_PWR = pin_pwr;
    pinMode(m_PIN_PWR, OUTPUT);             // set GPIO property.
    digitalWrite(m_PIN_PWR, LOW);           // LOW = Default OFF state.
}

/// @brief  : SIM7000 takes about 3s to turn on.
/// @return
boolean BK_modem::begin()
{
    //BK_DEBUG_PRINTLN(F("BK_modem::Begin() start..."));

    if (SerialSIM == NULL)
    {
        BK_DEBUG_PRINTLN(F("ERROR => First call init() function...."));
        return false;
    }

    // Send 'AT' command to modem, to check communication or to wake-up.
    if (!TestAT())
    { // no replay from modem. (could be PCB power down, wrong Buadrate, sim7000 firmware in dead loop, ....)
        return false;
    }

    // Set Echo Off"
    //sendCheckReply(F("ATE0"), m_ok_reply);

    // turn on error result codes.
    setMobileEquipmentError(1);

    //Default: SIM7XXXG will do: nothing when the 'DTR' pin is pulling up.
    enableSleepMode(0);       

    // if the sim isn't ready and a pin has been provided, try to unlock the sim
    int8_t ret = getPINStatus();
    if (ret != SIM_READY)
    {
        BK_DEBUG_PRINTLN(F("### Sim Error Status: ") + String(ret));
    }
    else
    {
        // if the sim is ready, or it's locked but no pin has been provided,
        String tmp = ((ret == SIM_READY) ? F("SIM READY") : F("SIM LOCKED"));
        BK_DEBUG_PRINTLN(F("### Sim Status: ") + tmp);
        // return (ret == SIM_READY || ret == SIM_PUK);
    }

    //BK_DEBUG_PRINTLN(F("BK_modem::Begin() End..."));
    return true;
}

/********* Power On/off ****************************************************/

/// @brief 
/// @param timeout default: 15 seconds.
/// @return 
boolean BK_modem::TestAT(int16_t timeout)
{
    BK_DEBUG_PRINTLN(F("Attempting to open comm with ATs"));

    while (timeout > 0)
    {
        while (SerialSIM->available())
        {
            SerialSIM->read();
        }

        if (sendCheckReply(F("AT"), m_ok_reply))
        {// exit.
            break;
        }

        while (SerialSIM->available())
        {
            SerialSIM->read();
        }

        if (sendCheckReply(F("AT"), F("AT")))
        {
            sendCheckReply(F("ATE0"), m_ok_reply);  // set "Echo Off"
            //break;
        }

        delay(500);

        flush();

        timeout -= 500;
    }

    if (timeout <= 0)
    {
        BK_DEBUG_PRINTLN(F("Timeout: No response to AT... last ditch attempt."));

        do
        {
            if (sendCheckReply(F("AT"), m_ok_reply))
            {
                break;
            }

            delay(100);
            if (sendCheckReply(F("AT"), m_ok_reply))
            {
                break;
            }

            delay(100);
            if (sendCheckReply(F("AT"), m_ok_reply))
            {
                break;
            }

            sendCheckReply(F("ATE0"), m_ok_reply); // Echo Off
            
            return false;

        } while (false);
    }

    return true;
}

/*****************************************************************
    BK-Sim7000 modem Power Off.
******************************************************************/
void BK_modem::modemPowerOff()
{
    // All UART communication gives some time Exception (28): when firmware restart().
    if (SerialSIM != NULL)
    {// SIM-70xx PCB communication active.
        AT_powerDown();
        delay(1200);

        LTE_modem_PowerOff();

        BK_DEBUG_PRINTLN(F("BK-Sim70XX Power-Off process."));
    }
}

/*****************************************************************
    BK-Sim7000 Modem Restart:
            by Power Off / Power On
******************************************************************/
void BK_modem::modemPowerRestart()
{
    if (SerialSIM == NULL)
    {// SIM-70xx PCB NOT connected.
        return;
    }

    BK_DEBUG_PRINTLN(F("BK-Sim70XX PCB Power Restart process START"));

    flushInput();

    modemPowerOff();            // BK-modem Software Abort

    LTE_modem_PowerUp();        // BK-modem Hardware restart.

    BK_DEBUG_PRINTLN(F("BK-Sim70XX PCB Power Restart process ENDED"));
}

/// @brief 
/// @param 
void BK_modem::base_stop()
{
    BK_DEBUG_PRINTLN(F("BK_modem => base_stop()"));

    // disconnect all sockets and close bearer and detach from GPRS Service.
    sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 8000);
}

/********* Serial port ****************************************************/
/// @brief
/// @param baud
/// @return
boolean BK_modem::setBaudrate(uint32_t baud)
{
    return sendCheckReply(F("AT+IPR="), baud, m_ok_reply);
}

/********* POWER, BATTERY & ADC ********************************************/
/// @brief 
/// @param _value 
/// @return value in mV (uint16_t)
boolean BK_modem::getBattVoltage(uint16_t *_value)
{
    return sendParseReply(F("AT+CBC"), F("+CBC: "), _value, ',', 2);
}

/*****************************************************************
    BK-Sim7000 By restart firmware Power Off/On.
******************************************************************/
/// @brief Hardware PCB:  Power-Off / Power-ON
/// @attention: DON'T change the delay() timer values.
void BK_modem::LTE_modem_PowerUp()
{
    BK_DEBUG_PRINTLN(F("LTE_modem PCB Power OFF..."));

    digitalWrite(m_PIN_PWR, LOW);
    delay(3000);                          // Datasheet Toned seconds = minimal 3sec.

    BK_DEBUG_PRINTLN(F("LTE_modem PCB Power ON..."));
    digitalWrite(m_PIN_PWR, HIGH);
    delay(4000);                          // wait that sim70xx mcu start-up. 

    BK_DEBUG_PRINTLN(F("LTE_modem, wait till MCU firmware running..."));
                                          // See spec sheets for your particular module
    delay(5000);                          // Datasheet Toned seconds = minimal 5Sec. (9000)
}

/// @brief Hardware PCB:  Power-Off 
void BK_modem::LTE_modem_PowerOff()
{
    digitalWrite(m_PIN_PWR, LOW);
    delay(1200);
}

/// @brief : powers down the SIM7000 module.
///          Module is powered down by AT command "AT+CPOWD=1"
/// @param  
void BK_modem::AT_powerDown(void)
{
    // disconnect all sockets and close bearer and detach from GPRS Service.
    stop();

    delay(1);                           // to reset Soft WDT

    sendCheckReply(F("AT+CPOWD=1"), F("NORMAL POWER DOWN"));     // Normal power off
}

/// @brief returns the percentage charge of battery.
/// @param pValue 
/// @return 
boolean BK_modem::getBattPercent(uint16_t *pValue)
{
    return sendParseReply(F("AT+CBC"), F("+CBC: "), pValue, ',', 1);
}

/// @brief
/// @param adcValue
/// @return
boolean BK_modem::getADCVoltage(uint16_t *adcValue)
{
    return sendParseReply(F("AT+CADC?"), F("+CADC: 1,"), adcValue);
}

/********* NETWORK AND WIRELESS CONNECTION SETTINGS ***********************/
// Set Phone Functionality:
// Uses the AT+CFUN command to set functionality (refer to AT+CFUN in manual)
//              0 --> Minimum functionality
//              1 --> Full functionality
//              4 --> Disable RF
//              5 --> Factory test mode
//              6 --> Restarts module
//              7 --> Offline mode
boolean BK_modem::setFunctionality(uint8_t option)
{
    return sendCheckReply(F("AT+CFUN="), option, m_ok_reply);
}

/// @brief Sleep mode reduces power consumption significantly while remaining registered to the network
/// Disable or enable slow clock
///                 0 Disable slow clock, module will not enter sleep mode. 
///                 1 Enable slow clock, it is controlled by DTR. 
///                         When DTR is high, module can enter sleep mode. 
///                         When DTR changes to low level, module can quit sleep mode.
/// @param onoff 
/// @return 
boolean BK_modem::enableSleepMode(int8_t onoff)
{
    uint16_t n_value;
    sendParseReply(F("AT+CSCLK?"), F("+CSCLK: "), &n_value, ',', 1); // Read Configure Slow Clock value
    
    if( n_value != onoff)
    {
        sendCheckReply(F("AT+CSCLK="), onoff, m_ok_reply);
    }

    return true;
}

//
// Extended-DRX Setting:
//
// Set e-RX parameters
// Mode options:
//          0  Disable the use of eDRX
//          1  Enable the use of eDRX
//          2  Enable the use of eDRX and auto report
//          3  Disable the use of eDRX(Reserved)
//
// Connection type options:
//          4 - CAT-M
//          5 - NB-IoT
//
// See AT command manual for eDRX values (options 0-15)
//
// NOTE: Network must support eDRX mode
//
boolean BK_modem::set_eDRX(uint8_t mode, uint8_t connType, char *eDRX_val)
{
    if (strlen(eDRX_val) > 4)
    {
        return false;
    }

    char auxStr[21];

    sprintf(auxStr, "AT+CEDRXS=%i,%i,\"%s\"", mode, connType, eDRX_val);

    return sendCheckReply(auxStr, m_ok_reply);
}

// NOTE: Network must support PSM and modem needs to restart before it takes effect
boolean BK_modem::enablePSM(bool onoff)
{
    return sendCheckReply(F("AT+CPSMS="), onoff, m_ok_reply);
}

// Set PSM with custom TAU and active time
// For both TAU and Active time, leftmost 3 bits represent the multiplier and rightmost 5 bits represent the value in bits.

// For TAU, left 3 bits:
// 000 10min
// 001 1hr
// 010 10hr
// 011 2s
// 100 30s
// 101 1min
// For Active time, left 3 bits:
// 000 2s
// 001 1min
// 010 6min
// 111 disabled

// Note: Network decides the final value of the TAU and active time.
boolean BK_modem::enablePSM(bool onoff, char *TAU_val, char *activeTime_val)
{ // AT+CPSMS command
    if (strlen(activeTime_val) > 8)
    {
        return false;
    }

    if (strlen(TAU_val) > 8)
    {
        return false;
    }

    char auxStr[35];
    sprintf(auxStr, "AT+CPSMS=%i,,,\"%s\",\"%s\"", onoff, TAU_val, activeTime_val);

    return sendCheckReply(auxStr, m_ok_reply);
}

//
// Enable, disable, or set the blinking frequency of the network status LED
// Default settings are the following:
//   mode:    0  set default netlight blinking periode: 64ms on/300ms off
//            1  Not connected to network --> 1,64,800
//            2  Connected to network     --> 2,64,3000
//            3  Data connection enabled  --> 3,64,300
boolean BK_modem::setNetLED(bool onoff, uint8_t mode, uint16_t timer_on, uint16_t timer_off)
{
    if (onoff)
    {
        getReply(F("AT+CSGS?"));    // Netlight Indication of GPRS Status.

        uint16_t stsMode;
        parseReply(F("+CSGS:"), &stsMode);
        /*
            <stsMode>
                0 Disable
                1 Enable, the netlight will be forced to enter into 64ms on/300ms off
                  blinking state in GPRS data transmission service.
                  Otherwise, the netlight state is not restricted.
                2 Enable, the netlight will blink according to "AT+SLEDS" in GPRS data transmission service.
        */

        if (mode == 0 )
        {
            sendCheckReply(F("AT+CSGS=1"), m_ok_reply);
            sendCheckReply(F("AT+CNETLIGHT=1"), m_ok_reply);        // Open It to Shining)
            return true;
        }
        else
        {
            if (!sendCheckReply(F("AT+CSGS=2"), m_ok_reply) ||      // set Slow Clock value = Enable
                !sendCheckReply(F("AT+CNETLIGHT=1"), m_ok_reply))   // Open It to Shining)
            {
                return false;
            }

            char auxStr[25];

            sprintf(auxStr, "AT+SLEDS=%i,%i,%i", mode, timer_on, timer_off);
            return sendCheckReply(auxStr, m_ok_reply);
        }

        return true;
    }
    else
    { // Close the Net Light
        sendCheckReply(F("AT+CSGS=0"), m_ok_reply);
        return sendCheckReply(F("AT+CNETLIGHT=0"), m_ok_reply);
    }
}

/// @brief Read Report Mobile Equipment Error code <n>
/// @param chkcode
///         +CMEE: <n>
/// @return
bool BK_modem::getMobileEquipmentError(uint8_t chkcode)
{
    // return sendCheckReply(F("AT+CMEE?"), F("+CMEE: 2"), 20000)

    // Check config error
    // String send = F("AT+CMEE?");
    // char sendArray[10];
    // strcpy_P(sendArray, (char *)pgm_read_ptr(&(send)));
    // or
    // strcpy_P(sendArray, (char *)pgm_read_ptr(FPSTR("AT+CMEE?")));

    String reply = F("+CMEE: ") + String(chkcode);
    return sendCheckReply((char *)pgm_read_ptr(FPSTR("AT+CMEE?")), (char *)pgm_read_ptr(&(reply)), 5000);
}

/// @brief : set Report Mobile Equipment Error
///     +CMEE=[<n>]:
///             0 Disable +CME ERROR: <err> result code and use ERROR instead.
///             1 Enable +CME ERROR: <err> result code and use numeric <err>
///             2 Enable +CME ERROR: <err> result code and use verbose <err> values
/// @return
bool BK_modem::setMobileEquipmentError(uint8_t code)
{
    // turn off error result codes and use "ERROR" instead.
    return sendCheckReply(F("AT+CMEE="), code, m_ok_reply);
}

/********* SIM ***********************************************************/

// Return status of PIN requirement
// -2 - Command returned with an error
// -1 - Unknown status
// 0 - MT is not pending for any password
// 1 - MT is waiting SIM PIN to be given
// 2 - MT is waiting for SIM PUK to be given
// 3 - ME is waiting for phone to SIM card (antitheft)
// 4 - ME is waiting for SIM PUK (antitheft)
// 5 - PIN2, e.g. for editing the FDN book possible only if preceding Command was acknowledged with +CME ERROR:17
// 6 - PUK2. Possible only if preceding Command was acknowledged with error +CME ERROR: 18.
int8_t BK_modem::getPINStatus()
{
    getReply(F("AT+CPIN?"), (uint16_t)5000);

    if (strncmp(m_replybuffer, "+CPIN: ", 7) != 0)
    {
        return PinStatus::SIM_ERROR;
    }

    char *returnVal = m_replybuffer + 7;

    if (strcmp(returnVal, "READY") == 0)
    {
        return PinStatus::SIM_READY;
    }
    else if (strcmp(returnVal, "SIM PIN") == 0)
    {
        return PinStatus::SIM_PIN;
    }
    else if (strcmp(returnVal, "SIM PUK") == 0)
    {
        return PinStatus::SIM_PUK;
    }
    else if (strcmp(returnVal, "PH_SIM PIN") == 0)
    {
        return PinStatus::SIM_PH_PIN;
    }
    else if (strcmp(returnVal, "PH_SIM PUK") == 0)
    {
        return PinStatus::SIM_PH_PUK;
    }
    else if (strcmp(returnVal, "SIM PIN2") == 0)
    {
        return PinStatus::SIM_PIN2;
    }
    else if (strcmp(returnVal, "SIM PUK2") == 0)
    {
        return PinStatus::SIM_PUK2;
    }

    return PinStatus::SIM_UNKNOWN;
}

/// @brief
/// @param pin
/// @return
uint8_t BK_modem::unlockSIM(const char *pin)
{
    char sendbuff[16] = "AT+CPIN=";

    // sendbuff[8] = '\"';
    // sendbuff[9] = pin[0];
    // sendbuff[10] = pin[1];
    // sendbuff[11] = pin[2];
    // sendbuff[12] = pin[3];
    // sendbuff[13] = '\"';
    // sendbuff[14] = 0x00;

    sendbuff[8]  = pin[0];
    sendbuff[9]  = pin[1];
    sendbuff[10] = pin[2];
    sendbuff[11] = pin[3];
    sendbuff[12] = '\0';

    return sendCheckReply(sendbuff, m_ok_reply);
}

/// @brief  : get ICCID
/// @param  : ccid[]
/// @return : lenght
uint8_t BK_modem::getSIMCCID(char *ccid)
{
    getReply(F("AT+CCID"), uint16_t(2000));

    // up to 28 chars for reply, 20 char total CCID
    // Response:  Ccid data [898600810906F8048812]
    if (m_replybuffer[0] == '+')
    {
        strncpy(ccid, m_replybuffer + 8, 20);
    }
    else
    {
        strncpy(ccid, m_replybuffer, 20);
    }

    ccid[20] = 0;

    readline(); // eat 'OK'

    return strlen(ccid);
}

/********* IMEI **********************************************************/

/// @brief
/// @param imei
/// @return
uint8_t BK_modem::getIMEI(char *imei)
{
    getReply(F("AT+GSN"));

    // up to 15 chars
    strncpy(imei, m_replybuffer, 15);
    imei[15] = 0;

    readline(); // eat 'OK'

    return strlen(imei);
}

/********* NETWORK *******************************************************/

/// @brief : Gets Network Registration Status
/// @param
/// @return : uint8_t The nework status:
///             0 = Not registered
///             1 = Registered (home network)
///             2 = Not registered (searching)
///             3 = Registration denied
///             4 = Unknown
///             5 = Registered, roaming
///
uint8_t BK_modem::getNetworkStatus(void)
{
    uint16_t status;

    if (!sendParseReply(F("AT+CGREG?"), F("+CGREG: "), &status, ',', 1))
    {
        return 0;
    }

    readline(); // eat 'OK'

    return status;
}

/// @brief set Network Registration Status
/// @param  option:
//           0 Disable network registration unsolicited result code.
//           1 Enable network registration unsolicited, result code +CGREG: <stat>
//           2 Enable network registration and location information unsolicited, result code +CGREG: <stat>[,<lac>,<ci>,<netact>]
//           4 Enable display gprs time and periodic RAU.
/// @return true = OK, false = Error
bool BK_modem::setNetworkStatus(uint8_t option)
{
    // set Network Registration Status
    if (!sendCheckReply(F("AT+CGREG="), option, m_ok_reply))
    {
        return true;
    }

    return false;
}

/// @brief : Signal Quality report:
///     rssi:
///             0       -115 dBm or less
///             1       -111 dBm
///             2...30  -109 ... -53 dBm
///             31      -54 dBm or greater
///             99      not known or not detectable
///
/// @param *quality
/// @param *rssi
/// @return
void BK_modem::getSignalQuality(uint8_t *quality, int8_t *rssi)
{
    uint16_t reply = 0;

    if (!sendParseReply(F("AT+CSQ"), F("+CSQ: "), &reply))
    {
        *quality = *rssi = 0;
        return;
    }

    *rssi = (reply * 2 - 4) - 109;
    *quality = reply;
}

/********* PWM/BUZZER **************************************************/

boolean BK_modem::setPWM(uint16_t period, uint8_t duty)
{
    if (period > 2000)
    {
        return false;
    }

    if (duty > 100)
    {
        return false;
    }

    return sendCheckReply(F("AT+SPWM=0,"), period, duty, m_ok_reply);
}

/********* SMS **********************************************************/

/// @brief 
/// @param  
/// @return 
uint8_t BK_modem::getSMSInterrupt(void)
{
    uint16_t reply;

    if (!sendParseReply(F("AT+CFGRI?"), F("+CFGRI: "), &reply))
    {
        return 0;
    }

    return reply;
}

/// @brief 
/// @param i 
/// @return 
boolean BK_modem::setSMSInterrupt(uint8_t i)
{
    return sendCheckReply(F("AT+CFGRI="), i, m_ok_reply);
}

/// @brief
/// @param
/// @return
int8_t BK_modem::getNumSMS(void)
{
    uint16_t numsms;

    // get into text mode
    if (!sendCheckReply(F("AT+CMGF=1"), m_ok_reply))
    {
        return -1;
    }

    // ask how many sms are stored
    if (sendParseReply(F("AT+CPMS?"), F(MODEM_PREF_SMS_STORAGE ","), &numsms))
    {
        return numsms;
    }
    else if (sendParseReply(F("AT+CPMS?"), F("\"SM\","), &numsms))
    {
        return numsms;
    }
    else if (sendParseReply(F("AT+CPMS?"), F("\"SM_P\","), &numsms))
    {
        return numsms;
    }
    else
    {
        return -1;
    }
}

// Reading SMS's is a bit involved so we don't use helpers that may cause delays or debug
// printouts!
boolean BK_modem::readSMS(uint8_t i, char *smsbuff,
                          uint16_t maxlen, uint16_t *readlen)
{
    // text mode
    if (!sendCheckReply(F("AT+CMGF=1"), m_ok_reply))
    {
        return false;
    }

    // show all text mode parameters
    if (!sendCheckReply(F("AT+CSDH=1"), m_ok_reply))
    {
        return false;
    }

    // parse out the SMS len
    uint16_t thesmslen = 0;

    BK_DEBUG_PRINT(F("\t---> "));
    BK_DEBUG_PRINT(F("AT+CMGR="));
    BK_DEBUG_PRINTLN(i);

    // getReply(F("AT+CMGR="), i, 1000);  //  do not print debug!
    SerialSIM->print(F("AT+CMGR="));
    SerialSIM->println(i);
    readline(1000); // timeout

    // DEBUG_PRINT(F("Reply: ")); DEBUG_PRINTLN(m_replybuffer);
    //  parse it out...

    BK_DEBUG_PRINTLN(m_replybuffer);

    if (!parseReply(F("+CMGR:"), &thesmslen, ',', 11))
    {
        *readlen = 0;
        return false;
    }

    readRaw(thesmslen);

    flushInput();

    uint16_t thelen = min(maxlen, (uint16_t)strlen(m_replybuffer));
    strncpy(smsbuff, m_replybuffer, thelen);
    smsbuff[thelen] = 0; // end the string

    BK_DEBUG_PRINTLN(m_replybuffer);

    *readlen = thelen;
    return true;
}

// Retrieve the sender of the specified SMS message and copy it as a string to
// the sender buffer.  Up to senderlen characters of the sender will be copied
// and a null terminator will be added if less than senderlen charactesr are
// copied to the result.  Returns true if a result was successfully retrieved,
// otherwise false.
boolean BK_modem::getSMSSender(uint8_t i, char *sender, int senderlen)
{
    // Ensure text mode and all text mode parameters are sent.
    if (!sendCheckReply(F("AT+CMGF=1"), m_ok_reply))
    {
        return false;
    }
    
    if (!sendCheckReply(F("AT+CSDH=1"), m_ok_reply))
    {
        return false;
    }

    BK_DEBUG_PRINT(F("\t---> "));
    BK_DEBUG_PRINT(F("AT+CMGR="));
    BK_DEBUG_PRINTLN(i);

    // Send command to retrieve SMS message and parse a line of response.
    SerialSIM->print(F("AT+CMGR="));
    SerialSIM->println(i);
    readline(1000);

    BK_DEBUG_PRINTLN(m_replybuffer);

    // Parse the second field in the response.
    boolean result = parseReplyQuoted(F("+CMGR:"), sender, senderlen, ',', 1);

    // Drop any remaining data from the response.
    flushInput();
    return result;
}

/// @brief 
/// @param smsaddr 
/// @param smsmsg 
/// @return 
boolean BK_modem::sendSMS(const char *smsaddr, const char *smsmsg)
{
    if (!sendCheckReply(F("AT+CMGF=1"), m_ok_reply))
    {
        return false;
    }

    char sendcmd[30] = "AT+CMGS=\"";
    strncpy(sendcmd + 9, smsaddr, 30 - 9 - 2); // 9 bytes beginning, 2 bytes for close quote + null
    sendcmd[strlen(sendcmd)] = '\"';

    if (!sendCheckReply(sendcmd, F("> ")))
    {
        return false;
    }

    BK_DEBUG_PRINT(F("\t---> "));
    BK_DEBUG_PRINTLN(smsmsg);

    // no need for extra NEWLINE characters SerialSIM->println(smsmsg);
    // no need for extra NEWLINE characters SerialSIM->println();
    SerialSIM->print(smsmsg);
    SerialSIM->write(0x1A);

    // DEBUG_PRINTLN("^Z");

    // Eat two sets of CRLF
    readline(200);
    // DEBUG_PRINT("Line 1: "); DEBUG_PRINTLN(strlen(m_replybuffer));
    readline(200);
    // DEBUG_PRINT("Line 2: "); DEBUG_PRINTLN(strlen(m_replybuffer));

    readline(30000); // read the +CMGS reply, wait up to 30s

    // DEBUG_PRINT("Line 3: "); DEBUG_PRINTLN(strlen(m_replybuffer));
    if (strstr(m_replybuffer, "+CMGS") == 0)
    {
        return false;
    }

    readline(1000); // read OK
    // DEBUG_PRINT("* "); DEBUG_PRINTLN(m_replybuffer);

    if (strcmp(m_replybuffer, "OK") != 0)
    {
        return false;
    }

    return true;
}

/// @brief 
/// @param i 
/// @return 
boolean BK_modem::deleteSMS(uint8_t i)
{
    if (!sendCheckReply(F("AT+CMGF=1"), m_ok_reply))
    {
        return false;
    }

    // delete an sms
    char sendbuff[12] = "AT+CMGD=000";
    sendbuff[8] = (i / 100) + '0';
    i %= 100;
    sendbuff[9] = (i / 10) + '0';
    i %= 10;
    sendbuff[10] = i + '0';

    return sendCheckReply(sendbuff, m_ok_reply, 2000);
}

/// @brief 
/// @return 
boolean BK_modem::deleteAllSMS()
{
    if (!sendCheckReply(F("AT+CMGF=1"), m_ok_reply))
    {
        return false;
    }

    return sendCheckReply("AT+CMGD=1,4", m_ok_reply, 2000);
}

/********* USSD *********************************************************/

boolean BK_modem::sendUSSD(char *ussdmsg, char *ussdbuff, uint16_t maxlen, uint16_t *readlen)
{
    if (!sendCheckReply(F("AT+CUSD=1"), m_ok_reply))
    {
        return false;
    }

    char sendcmd[30] = "AT+CUSD=1,\"";
    strncpy(sendcmd + 11, ussdmsg, 30 - 11 - 2); // 11 bytes beginning, 2 bytes for close quote + null
    sendcmd[strlen(sendcmd)] = '\"';

    if (!sendCheckReply(sendcmd, m_ok_reply))
    {
        *readlen = 0;
        return false;
    }
    else
    {
        readline(10000); // read the +CUSD reply, wait up to 10 seconds!!!
        // DEBUG_PRINT("* "); DEBUG_PRINTLN(m_replybuffer);

        char *p = prog_char_strstr(m_replybuffer, PSTR("+CUSD: "));
        if (p == 0)
        {
            *readlen = 0;
            return false;
        }

        p += 7; //+CUSD
        // Find " to get start of ussd message.
        p = strchr(p, '\"');

        if (p == 0)
        {
            *readlen = 0;
            return false;
        }

        p += 1; //"
        // Find " to get end of ussd message.
        char *strend = strchr(p, '\"');
        // long long len = strend - p;

        uint16_t lentocopy = min(maxlen - 1, strend - p);
        strncpy(ussdbuff, p, lentocopy + 1);
        ussdbuff[lentocopy] = 0;
        *readlen = lentocopy;
    }

    return true;
}

/********* TIME **********************************************************/

/// @brief  Get Local Timestamp
///         Set AT+CLTS=1, it means user can receive network time updating and
///         use AT+CCLK to show current time.
/// @param onoff : true/false
/// @return : true = OK
boolean BK_modem::enableNetworkTimeSync(boolean onoff)
{
    //std::is_base_of<Base, Derived>::value → true if Base is a base class of Derived (or the same type), otherwise false.
    //if constexpr (std::is_base_of<BK_modem, BK_modem_7600>::value)
    // if (m_Sim_Type == SIM_TYPE::SIM7600)
    // {
    //     BK_DEBUG_PRINTLN(F("*** enableNetworkTimeSync : SIM-7600 ***"));
    //     return true;
    // }

// dit werkt, set ; Enable RTTI
//   if (typeid(this) == typeid(BK_modem_7600))
//  or 
//   if (dynamic_cast<BK_modem_7600*>(this) != nullptr)
//   {
//     //BK_DEBUG_PRINT(F("1: "));
//     BK_DEBUG_PRINTLN(F("*** enableNetworkTimeSync : SIM-7600 ***"));
//     return true;
//   }


  if (onoff) 
  {
    if (! sendCheckReply(F("AT+CLTS=1"), m_ok_reply))
    {
      return false;
    }
  } 
  else 
  {
    if (! sendCheckReply(F("AT+CLTS=0"), m_ok_reply))
    {
      return false;
    }
  }

  flushInput(); // eat any 'Unsolicted Result Code'

 //   String res; res.replace("\r\nOK\r\n", "");
 //   res.replace("\r\n", "");

  return true;
}

/// @brief : TODO: Function not finisch yet
/// @param  
String BK_modem::getNTPgetserver(void)
{
    if (!sendCheckReply(F("AT+CNTP?"), m_ok_reply, 10000))
    {
        return "";
    }

    // Response:   + CNTP: <ntp sever>,<time zone>,<cid>,<mode>
    String ntp_sever;
    readline(10000);        // wait for +CNTP: ......

    // if (!parseReply(F("+CNTP: "), &status))
    // {
    //     return 0;
    // }

    return "";
}


// Returns the status of the NTP module:
// 1 Network time synchronization is successful
// 61 Network Error
// 62 DNS resolution error
// 63 Connection Erro
// 64 Service response error
// 65 Service Response Timeout
// see AT Command manual 1.04 p.204
uint8_t BK_modem::getNTPstatus()
{
    if (!sendCheckReply(F("AT+CNTP"), m_ok_reply, 10000))
    {
        return 0;
    }

    uint16_t status;
    readline(10000);
    if (!parseReply(F("+CNTP: "), &status))
    {
        return 0;
    }

    return status;
}

/// @brief : time zone = 0 (time zone value multiplay by 4.)
/// @param onoff 
/// @param ntpserver 
/// @param timeZone
/// @param cid => for sim-7080 refer to <pdpidx> of AT+CNACT.
///               99 = sim-7000
/// @return 
boolean BK_modem::enableNTPTimeSync(boolean onoff, FStringPtr ntpserver, uint16 timeZone, uint16_t cid)
{
    if (onoff)
    {
        // Set NTP service url and local time zone
        if (ntpserver == 0)
        {
            ntpserver = F("pool.ntp.org");
        }

        char _sendbuffer[64] = {0};
        if (cid == 7000)
        {// SIM7000
            // Set GPRS bearer = 1 profile to associate with NTP sync.
            if (!sendCheckReply(F("AT+CNTPCID=1"), m_ok_reply))
            {
                return false;
            }

            sprintf(_sendbuffer, PSTR("AT+CNTP=\"%s\",%0d"), ntpserver, timeZone * 4); // time zone = 0 (time zone value multiplay by 4.)
            // sprintf(_sendbuffer, PSTR("AT+CNTP=\"%s\",%0d"), FPSTR(ntpserver), timeZone * 4);  // time zone = 0 (time zone value multiplay by 4.)
        }
        else
        {
            // Set GPRS bearer = 0..3 profile to associate with NTP sync.
            if (!sendCheckReply(F("AT+CNTPCID="), cid, m_ok_reply))
            {
                return false;
            }

            sprintf(_sendbuffer, PSTR("AT+CNTP=\"%s\",%0d,%d"), ntpserver, timeZone * 4, cid); // time zone = 0 (time zone value multiplay by 4.)
        }

        getReply(_sendbuffer, BK_SIM7000_TIMEOUT_1500MS);

        // char _timezone[8];
        // sprintf(_timezone, PSTR("\",%0d"), timeZone * 4);  // time zone = 0 (time zone value multiplay by 4.)

        // SerialSIM->print(F("AT+CNTP=\""));
        // if (ntpserver != 0)
        // {
        //     SerialSIM->print(ntpserver);
        // }
        // else
        // {
        //     SerialSIM->print(F("pool.ntp.org"));
        // }

        // SerialSIM->println(_timezone);                 
        //readline(BK_SIM7000_DEFAULT_TIMEOUT_MS);

        if (strcmp_P(m_replybuffer, String(m_ok_reply).c_str()) != 0)
        {
            return false;
        }

        // Execution command => Start Synchronize network time.
        if (!sendCheckReply(F("AT+CNTP"), m_ok_reply, 10000))
        {
            return false;
        }

        uint16_t status;
        readline(10000);                            // +CNTP: 1
        if (!parseReply(F("+CNTP:"), &status))      // Synchronize Network Time
        {
            return false;
        }

        if( status == 0)
        {
            return false;
        }
    }
    else
    { // Reset NTP Use bear profile 1
        if (cid == 7000)
        {// SIM7000
            if (!sendCheckReply(F("AT+CNTPCID=0"), m_ok_reply))
            {
                return false;
            }
        }
    }

    return true;
}

/// @brief 
/// @param buff 
/// @param maxlen 
/// @return 
boolean BK_modem::getTime(char *buff, uint16_t maxlen)
{
    getReply(F("AT+CCLK?"), (uint16_t)10000);
    if (strncmp(m_replybuffer, "+CCLK: ", 7) != 0)
    {
        return false;
    }

    char *bufptr = m_replybuffer + 7;
    uint16_t lentocopy = min(maxlen - 1, (int)strlen(bufptr));
    strncpy(buff, bufptr, lentocopy + 1);
    buff[lentocopy] = 0;

    readline();                 // eat OK

    return true;
}

/********* Real Time Clock ********************************************/

/// @brief 
/// @param year 
/// @param month 
/// @param date 
/// @param hr 
/// @param min 
/// @param sec 
/// @param tz 
/// @return 
boolean BK_modem::readRTC(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *hr, uint8_t *min, uint8_t *sec, int8_t *tz)
{
    getReply(F("AT+CCLK?"), (uint16_t)10000); // Get RTC timeout 10 sec

    if (strncmp(m_replybuffer, "+CCLK: ", 7) != 0)
    {
        return false;
    }

    char *p = m_replybuffer + 8; // skip +CCLK: "

    // Parse date
    int reply = atoi(p);    // get year
    *year = (uint8_t)reply; // save as year
    p += 3;                 // skip 3 char
    reply = atoi(p);
    *month = (uint8_t)reply;
    p += 3;
    reply = atoi(p);
    *date = (uint8_t)reply;
    p += 3;
    reply = atoi(p);
    *hr = (uint8_t)reply;
    p += 3;
    reply = atoi(p);
    *min = (uint8_t)reply;
    p += 3;
    reply = atoi(p);
    *sec = (uint8_t)reply;
    p += 3;
    reply = atoi(p);
    *tz = reply;

    readline(); // eat OK

    return true;
}

/// @brief Get Local Timestamp
/// @param mode : 0 Disable
///               1 Enable
/// @return 
boolean BK_modem::enableRTC(uint8_t mode)
{
    if (!sendCheckReply(F("AT+CLTS="), mode, m_ok_reply))
    {
        return false;
    }

     return true;               // NOT used: sendCheckReply(F("AT&W"), m_ok_reply);
}

/********* GPS **********************************************************/
/// @brief 
/// @param onoff 
/// @return 
boolean BK_modem::enableGPS(boolean onoff)
{
    uint16_t state;

    // First check if its already on or off.
    if (!sendParseReply(F("AT+CGNSPWR?"), F("+CGNSPWR: "), &state))
    {
        return false;
    }

    if (onoff && !state)
    {
        if (!sendCheckReply(F("AT+CGNSPWR=1"), m_ok_reply))
        {
            return false;
        }
    }
    else if (!onoff && state)
    {
        if (!sendCheckReply(F("AT+CGNSPWR=0"), m_ok_reply))
        {
            return false;
        }
    }

    return true;
}

/// @brief GNSS Navigation Information Parsed From NMEA Sentences
/// @param arg
/// @param buffer
/// @param maxbuff
/// @return
uint8_t BK_modem::getGPS_Navigation_Information(uint8_t arg, char *buffer, uint8_t maxbuff)
{
    getReply(F("AT+CGNSINF"));

    char *ptr = prog_char_strstr(m_replybuffer, (prog_char *)F("SINF"));

    if (ptr == 0)
    {
        buffer[0] = 0;
        return 0;
    }

    ptr += 6;

    uint8_t len = max(maxbuff - 1, (int)strlen(ptr));
    strncpy(buffer, ptr, len);
    buffer[len] = 0;

    readline(); // eat 'OK'

    return len;
}

/// @brief  See table 2-3 from here for format:
///         http://www.adafruit.com/datasheets/SIM800%20Series_GNSS_Application%20Note%20V1.00.pdf
///
///         +CGNSINF: 1,1,20240807090310.000,55.121596,5.805641,8.000,0.00,324.5,1,,0.8,1.0,0.7,,21,8,5,,34,,
///
/// @param lat
/// @param lon
/// @param speed_kph
/// @param heading
/// @param altitude
/// @param year
/// @param month
/// @param day
/// @param hour
/// @param min
/// @param sec
/// @return
boolean BK_modem::getGPS(float *lat, float *lon, 
                         float *speed_kph, float *heading, float *altitude,
                         uint16_t *year, uint8_t *month, uint8_t *day, 
                         uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    char *ptrtoken;             // token pointer.
    char gpsbuffer[120];

    // grab the mode 2^5 gps csv from the sim808
    uint8_t res_len = getGPS_Navigation_Information(32, gpsbuffer, 120);

    // make sure we have a response
    if (res_len == 0)
    {
        return false;
    }

    // skip GPS run status
    ptrtoken = strtok(gpsbuffer, ",");
    if (!ptrtoken)
    {
        return false;
    }

    // skip fix status
    ptrtoken = strtok(NULL, ",");
    if (!ptrtoken)
    {
        return false;
    }

    // skip date
    // tok = strtok(NULL, ",");
    // if (! tok) return false;

    // only grab date and time if needed
    if ((year != NULL) && (month != NULL) && (day != NULL) && (hour != NULL) && (min != NULL) && (sec != NULL))
    {
        char *date = strtok(NULL, ",");
        if (!date)
        {
            return false;
        }

        // Seconds
        char *ptr = date + 12;
        *sec = atof(ptr);

        // Minutes
        ptr[0] = 0;
        ptr = date + 10;
        *min = atoi(ptr);

        // Hours
        ptr[0] = 0;
        ptr = date + 8;
        *hour = atoi(ptr);

        // Day
        ptr[0] = 0;
        ptr = date + 6;
        *day = atoi(ptr);

        // Month
        ptr[0] = 0;
        ptr = date + 4;
        *month = atoi(ptr);

        // Year
        ptr[0] = 0;
        ptr = date;
        *year = atoi(ptr);
    }
    else
    {
        // skip date
        ptrtoken = strtok(NULL, ",");
        if (!ptrtoken)
        {
            return false;
        }
    }

    // grab the latitude
    char *latp = strtok(NULL, ",");
    if (!latp)
    {
        return false;
    }

    // grab longitude
    char *longp = strtok(NULL, ",");
    if (!longp)
    {
        return false;
    }

    *lat = atof(latp);
    *lon = atof(longp);

    // only grab altitude if needed
    if (altitude != NULL)
    {
        // grab altitude
        char *altp = strtok(NULL, ",");
        if (!altp)
        {
            return false;
        }

        *altitude = atof(altp);
    }

    // only grab speed if needed
    if (speed_kph != NULL)
    {
        // grab the speed in km/h
        char *speedp = strtok(NULL, ",");
        if (!speedp)
        {
            return false;
        }

        *speed_kph = atof(speedp);
    }

    // only grab heading if needed
    if (heading != NULL)
    {

        // grab the speed in knots
        char *coursep = strtok(NULL, ",");
        if (!coursep)
        {
            return false;
        }

        *heading = atof(coursep);
    }

    (void)ptrtoken;

    return true;
}

/// @brief  : GNSS NMEA Out configure
/// @param  : input = 0, 1 or 2
/// @return :
boolean BK_modem::enableGPSNMEA(uint8_t input)
{
    if( input )
    {                                                   // AT+CGPSNMEA? => 7600
        sendCheckReply(F("AT+CGNSCFG=1"), m_ok_reply);          // Turn on GNSS NMEA Data Output to USB port.
        sendCheckReply(F("AT+CGNSTST=1"), m_ok_reply);          // Turn on GNSS NMEA Data Output to AT port.

        return true;
    }
    else
    {
        return sendCheckReply(F("AT+CGNSTST=0"), m_ok_reply);   // Turn off GNSS NMEA Data Output to AT port
    }
}

/// @brief 
/// @param voltage_mv => default 2800mV
void BK_modem::getGPSAntennaVoltage(uint16_t* voltage_mv)
{
    *voltage_mv = 2800;
}

/********* GPRS **********************************************************/

/// @brief : Request Manufacturer Identification
/// @return
String BK_modem::getManufacturer_Identification()
{
    int16_t len = getReply(F("GMI"), uint16_t(5000), true);

    String res = F("**** SIMCOM_Ltd **** ");

    if (len > 0)
    {
        res = String(m_replybuffer);

        // Do the replaces twice so we cover both \r and \r\n type endings
        res.replace("\r\nOK\r\n", "");
        res.replace("\rOK\r", "");
        res.replace("\r\n", " ");
        res.replace("\r", " ");
        res.trim();
    }

    return res;
}

/// @brief : GMM = Request Modem(TA) Model Identiofication.
/// @return
String BK_modem::getModemName()
{
    int16_t len = getReply(F("AT+GMM"), uint16_t(5000));

    String name = F("**** SIMCom SIM7000 ****");

    if (len > 0)
    {
        // if (prog_char_strstr(m_replybuffer, (prog_char *)F("SIM7000")) != 0)
        // {
        //      m_type = SIM7000;
        // }

        // content m_replybuffer = "SIM7000_R1351"
        String res2 = String(m_replybuffer);
        res2.replace("\r\nOK\r\n", "");
        res2.replace("_", " ");
        res2.trim();
        name = res2;
    }

    // Eat 'OK'
    readline();

    return name;
}

/// @brief: Display Product Identification Information
/// @return
String BK_modem::getModemInfo()
{
    uint16_t len = getReply(F("ATI"), uint16_t(5000), true);

    String res = F("****SIMCom SIM7000 INFO****: ") + String(len);

    if (len > 0)
    {
        res = String(m_replybuffer);

        // Delete these characters from string.
        res.replace(F("+GCAP: +CGSM"), "");
        res.replace(F("\n\nOK\n"), "");
        //res.replace("\r", "xA");
        //res.replace("\n", "xD");

        res.trim();
    }

    return res;
}

/// @brief Request TA Revision Identification of Software Release
/// @return
String BK_modem::getModemSoftware_Revision()
{
    sendCheckReply(F("AT+GMR"), m_ok_reply);

    //BK_DEBUG_PRINTLN(m_replybuffer);

    return String(m_replybuffer);
}

// Show Network System Mode
// Returns type of the network the module is connected to
//               +CNSMOD: <n>,<stat>
// -1 error
// 0 no service
// 1 GSM
// 3 EGPRS
// 7 LTE M1
// 9 LTE NB
// You can pass a string of sufficient length to receive a text copy as well
// NOTE: Only tested on SIM7000E
uint8_t BK_modem::getNetworkSystemMode(char *typeStringBuffer)
{
    uint16_t stat;

    if (!sendParseReply(F("AT+CNSMOD?"), F("+CNSMOD:"), &stat, ',', 1))
    {
        return -1;
    }

    if (typeStringBuffer != NULL)
    {
        switch (stat)
        {
        case 0:
            strcpy_P(typeStringBuffer, (prog_char *)F("no service"));
            break;
        case 1:
            strcpy_P(typeStringBuffer, (prog_char *)F("GSM"));
            break;
        case 2:
            strcpy_P(typeStringBuffer, (prog_char *)F("GPRS"));
            break;
        case 3:
            strcpy_P(typeStringBuffer, (prog_char *)F("EGPRS"));
            break;
        case 7:
            strcpy_P(typeStringBuffer, (prog_char *)F("LTE M1"));
            break;
        case 8:
            strcpy_P(typeStringBuffer, (prog_char *)F("LTE"));
            break;
        case 9:
            strcpy_P(typeStringBuffer, (prog_char *)F("LTE NB"));
            break;
        default:
            strcpy_P(typeStringBuffer, (prog_char *)F("unknown"));
            break;
        }
    }

    return (int8_t)stat;
}

// Bearer Settings for Applications Based on IP
//   Query the GPRS bearer context status
// cid = 1  => Bearer profile identifier:
// Returns bearer status: 
//                       1 Bearer is connected
//
int8_t BK_modem::getBearerStatus(void)
{
    return 1;
}

// Query IP address and copy it into the passed buffer.
// Buffer needs to be at least 16 chars long.
// Returns true on success.
boolean BK_modem::getIPv4(char *ipStringBuffer, size_t bufferLength)
{
    if (ipStringBuffer == NULL || bufferLength < 16)
    {
        return false;
    }

    getReply(F("AT+SAPBR=2,1"));

    strtok(m_replybuffer, "\"");
    char *temp = strtok(NULL, "\"");

    if (temp == NULL)
    {
        return false;
    }

    strncpy(ipStringBuffer, temp, bufferLength);

    return true;
}

/// @brief : Inquiring UE System Information
///
///  +CPSI: <System Mode>,<Operation Mode>,<MCC>-<MNC>,<TAC>,<SCellID>,<PCellID>,<Frequency Band>,<earfcn>,<dlbw>,<ulbw>,
///         <RSRQ>,<RSRP>,<RSSI>,<RSSNR>
///      format: "+CPSI: GSM,Online,204-04,0x00c9,61828,1002 EGSM 900,-67,0,44-44"   OK
///
/// If no service:
///                 +CPSI: NO SERVICE,Online
///
/// @param infoLine
/// @param infoProvider
void BK_modem::getNetworkInfo(String &infoLine)
{
    getReply(F("AT+CPSI?"));

    size_t len = strlen(m_replybuffer);
    if (len > 7)
    {
        infoLine = String(m_replybuffer).substring(7, len);
    }

    // Eat 'OK'
    readline();
}

/// @brief : Gets the current network operator via the TS command "AT+COPS"
/// @param  
/// @return 
String BK_modem::getOperator(void)
{
    if (!sendCheckReply(F("AT+COPS?"), F("+COPS:")))
    {
        return "";
    }

    size_t len = strlen(m_replybuffer);
    RESERVE_STRING(res, len + 2);

    if (len > 11)
    { // +COPS: 0,0,"vodafone NL 1nce.net",3
        res = String(m_replybuffer).substring(12, len - 3);
    }

     readline();     // Eat 'OK'

    return res;
}

/// @brief : undocumented item: "AT+SIMCOMATI"
/// @param
/// @return : data.
String BK_modem::getSIMCOMATI(void)
{
    int16_t len = getReply(F("AT+SIMCOMATI"), uint16_t(5000), true); // wait max. 5 sec.

    //BK_DEBUG_PRINTLN(F("***SIMCOMATI*** ") + String(m_replybuffer));

    if (len > 0)
    {
        RESERVE_STRING(res, len + 2);

        res = String(m_replybuffer);
        res.replace(F("+GCAP: +CGSM"),"");
        res.replace(F("\n\nOK\n"), "");
        res.replace(F("\n\n"), "\n");
        res.trim();

        return res;
    }

    return F("No Information available.");
}

/// @brief
/// @param
/// @return
bool BK_modem::getNetworkInfoLong(void)
{
    if (!sendCheckReply(F("AT+COPS=?"), m_ok_reply, 2000))
    {
        return false;
    }

    return true;
}

/// @brief 
/// @param  
/// @return 
String BK_modem::getResponseMessage(void)
{
    if(m_responcebuffer[0] != 0x00)
    {
        return String(m_responcebuffer);
    }
    else
    {
        return F("");
    }
}

/// @brief: Attach or Detach to GPRS/EPS service.
/// @param
/// @return: 0 = Detached, 1 = Attached
int8_t BK_modem::GPRSstate(void)
{
    uint16_t state;

    uint16_t len = getReply(F("AT+CGATT?"), uint16_t(75000)); // timeout mx. 75 sec.

    //  parse it out...
    // BK_DEBUG_PRINTLN(m_replybuffer);

    if (len == 0 || !parseReply(F("+CGATT: "), &state, ',', 7))
    {
        return false;
    }

    return state;
}

/// @brief Checks if current attached to GPRS/EPS service
/// @return
bool BK_modem::isGprsConnected()
{
    int8_t state = GPRSstate();

    if (state != 0)
    {
        return false;
    }

    return true;
}

/// @brief
/// @param apn
/// @param username
/// @param password
void BK_modem::setNetworkSettings(FStringPtr apn, FStringPtr username, FStringPtr password)
{
    this->m_apn = apn;
    this->m_apnusername = username;
    this->m_apnpassword = password;

    // Define the PDP context (PDP Packet Data Protocol)
    sendCheckReplyQuoted(F("AT+CGDCONT=1,\"IP\","), apn, m_ok_reply, 10000);
}

/// @brief
/// @param errorcode
/// @param buff
/// @param maxlen
/// @return
boolean BK_modem::getGSMLoc(uint16_t *errorcode, char *buff, uint16_t maxlen)
{
    getReply(F("AT+CIPGSMLOC=1,1"), (uint16_t)10000);

    if (!parseReply(F("+CIPGSMLOC: "), errorcode))
    {
        return false;
    }

    char *p = m_replybuffer + 14;
    uint16_t lentocopy = min(maxlen - 1, (int)strlen(p));
    strncpy(buff, p, lentocopy + 1);

    readline(); // eat OK

    return true;
}

/// @brief
/// @param ptr to lat
/// @param ptr to lon
/// @return
boolean BK_modem::getGSMLoc(float *lat, float *lon)
{
    uint16_t returncode;
    char gpsbuffer[120];

    // make sure we could get a response
    if (!getGSMLoc(&returncode, gpsbuffer, 120))
        return false;

    // make sure we have a valid return code
    if (returncode != 0)
    {
        return false;
    }

    // +CIPGSMLOC: 0,-74.007729,40.730160,2015/10/15,19:24:55
    // tokenize the gps buffer to locate the lat & long
    char *longp = strtok(gpsbuffer, ",");
    if (!longp)
    {
        return false;
    }

    char *latp = strtok(NULL, ",");
    if (!latp)
    {
        return false;
    }

    *lat = atof(latp);
    *lon = atof(longp);

    return true;
}

// Query wireless connection status
boolean BK_modem::wirelessConnStatus(void)
{
    getReply(F("AT+CNACT?"));
    // Format of response:
    // +CNACT: <status>,<ip_addr>
    if (strstr(m_replybuffer, "+CNACT: 1") == NULL)
    {
        return false;
    }

    return true;
}

/*
    eventueel string mee geven for responce data.
    Fast way to Get/Post to webserver.
*/
boolean BK_modem::postData(const char *request_type, const char *URL, const char *body, const char *token, uint32_t bodylen)
{
    // NOTE: Need to open socket/enable GPRS before using this function
    // char auxStr[64];

    // Make sure HTTP service is terminated so initialization will run.
    sendCheckReply(F("AT+HTTPTERM"), m_ok_reply, 10000);

    // Initialize HTTP service
    if (!sendCheckReply(F("AT+HTTPINIT"), m_ok_reply, 10000))
    {
        return false;
    }

    // Set HTTP parameters => bearer profile indentifier.
    if (!sendCheckReply(F("AT+HTTPPARA=\"CID\",1"), m_ok_reply, 10000))
    {
        return false;
    }

    // Specify URL
    char urlBuff[strlen(URL) + 22];
    sprintf(urlBuff, "AT+HTTPPARA=\"URL\",\"%s\"", URL);

    if (!sendCheckReply(urlBuff, m_ok_reply, 10000))
        return false;

    // Perform request based on specified request Type
    if (strlen(body) > 0)
    {
        bodylen = strlen(body);
    }

    if (strcmp(request_type, "GET") == 0)
    {
        if (!sendCheckReply(F("AT+HTTPACTION=0"), m_ok_reply, 10000))
        {
            return false;
        }
    }
    else if (strcmp(request_type, "POST") == 0 && bodylen > 0)
    { // POST with content body
        if (!sendCheckReply(F("AT+HTTPPARA=\"CONTENT\",\"application/json\""), m_ok_reply, 10000))
        {
            return false;
        }

        if (strlen(token) > 0)
        {
            char tokenStr[strlen(token) + 55];

            sprintf(tokenStr, "AT+HTTPPARA=\"USERDATA\",\"Authorization: Bearer %s\"", token);

            if (!sendCheckReply(tokenStr, m_ok_reply, 10000))
            {
                return false;
            }
        }

        char dataBuff[sizeof(bodylen) + 20];

        sprintf(dataBuff, "AT+HTTPDATA=%lu,9900", (long unsigned int)bodylen);

        if (!sendCheckReply(dataBuff, "DOWNLOAD", 10000))
        {
            return false;
        }

        delay(100); // Needed for fast baud rates (ex: 115200 baud with SAMD21 hardware serial)

        if (!sendCheckReply(body, m_ok_reply, 10000))
        {
            return false;
        }

        if (!sendCheckReply(F("AT+HTTPACTION=1"), m_ok_reply, 10000))
        {
            return false;
        }
    }
    else if (strcmp(request_type, "POST") == 0 && bodylen == 0)
    { // POST with query parameters
        if (!sendCheckReply(F("AT+HTTPACTION=1"), m_ok_reply, 10000))
        {
            return false;
        }
    }
    else if (strcmp(request_type, "HEAD") == 0)
    {
        if (!sendCheckReply(F("AT+HTTPACTION=2"), m_ok_reply, 10000))
        {
            return false;
        }
    }

    // wait for +HTTPACTION Responce.
    readline(10000);
    BK_DEBUG_PRINT(F("HTTP Responce 1: "));
    BK_DEBUG_PRINTLN(m_replybuffer); // Print out server reply

    //  Parse response status and size
    uint16_t status, datalen;
    if (!parseReply(F("+HTTPACTION:"), &status, ',', 1))
    {
        return false;
    }

    if (!parseReply(F("+HTTPACTION:"), &datalen, ',', 2))
    {
        return false;
    }

    BK_DEBUG_PRINT(F("HTTP status: "));
    BK_DEBUG_PRINTLN(status);
    BK_DEBUG_PRINT(F("Data length: "));
    BK_DEBUG_PRINTLN(datalen);

    // HTTP responce code: HTTP_CODE_OK
    if (status != HTTP_CODE_OK)
    {
        return false;
    }

    RESERVE_STRING(totalBuffer, datalen + 2);
    totalBuffer.clear();

    uint16_t timeout = 10000;

    getReply(F("AT+HTTPREAD")); // Start reading responce data from website.

    do
    {
        uint16_t len = readline(timeout); // Get Responce body: ........

        if (len > 0)
        {
            totalBuffer.concat(m_replybuffer);
        }
        else
        {
            continue;
        }

        timeout = 2000;

        BK_DEBUG_PRINT(F("Resp. length: "));
        BK_DEBUG_PRINTLN(len);
        BK_DEBUG_PRINTLN(m_replybuffer); // Print out server reply

        if (strstr(m_replybuffer, "OK") != 0)
        { // end.
            break;
        }

    } while (SerialSIM->available()); // check if still data in SerialSIM receive buffer.

    // BK_DEBUG_PRINT(F("responce length: "));
    // BK_DEBUG_PRINTLN(totlen);
    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(totalBuffer); // Print out server reply

    // BK_DEBUG_PRINT(F("\t<--- "));
    // BK_DEBUG_PRINTLN(m_replybuffer); // Print out server reply

    // Terminate HTTP service
    sendCheckReply(F("AT+HTTPTERM"), m_ok_reply, 10000);

    return true;
}

/********************************* HTTPS FUNCTION *********************************/

/// @brief https:// by default uses port 443, so you may omit it in the URL, otherwise it would of looked like
///        this https://data.sensor.community:443/airrohr/v1/sensor/79729/
///             Protocol: https://
///             Hostname: data.sensor.community
///             Port: 443
///             Host: data.sensor.community or data.sensor.community:443
///             Hostport: data.sensor.community:443
///             Path: airrohr/v1/sensor/79729/
///
/// @param server
/// @param port
/// @param connType
/// @param URL
/// @param body
/// @return
boolean BK_modem::postData(const char *server, uint16_t port, const char *connType, const char *URL, const char *body)
{
    // Start HTTPS stack

    if (!sendCheckReply(F("AT+CHTTPSSTART"), m_ok_reply, 10000))
        return false;

    BK_DEBUG_PRINTLN(F("Waiting 1s to ensure connection..."));
    delay(1000);

    // Construct the AT command based on function parameters
    char auxStr[200];
    uint8_t connTypeNum = 1;

    if (strcmp(connType, "HTTP") == 0)
    {
        connTypeNum = 1;
    }

    if (strcmp(connType, "HTTPS") == 0)
    {
        connTypeNum = 2;
    }

    sprintf(auxStr, "AT+CHTTPSOPSE=\"%s\",%d,%d", server, port, connTypeNum);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return false;

    BK_DEBUG_PRINTLN(F("Waiting 1s to make sure it works..."));
    delay(1000);

    // Send data to server
    sprintf(auxStr, "+CHTTPSSEND=%i", strlen(URL) + strlen(body)); // URL and body must include \r\n as needed

    if (!sendCheckReply(auxStr, ">", 10000))
    {
        return false;
    }

    if (!sendCheckReply(URL, m_ok_reply, 10000))
        return false;

    delay(1000);

    // Check server response length
    uint16_t replyLen;
    sendParseReply(F("AT+CHTTPSRECV?"), F("+CHTTPSRECV: LEN,"), &replyLen);

    // Get server response content
    sprintf(auxStr, "AT+CHTTPSRECV=%i", replyLen);
    getReply(auxStr, 2000);

    if (replyLen > 0)
    {
        readRaw(replyLen);
        flushInput();

        BK_DEBUG_PRINT(F("\t<--- "));
        BK_DEBUG_PRINTLN(m_replybuffer);
    }

    // Close HTTP/HTTPS session
    sendCheckReply(F("AT+CHTTPSCLSE"), m_ok_reply, 10000);
    // if (! sendCheckReply(F("AT+CHTTPSCLSE"), m_ok_reply, 10000))
    //   return false;

    readline(10000);

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    // Stop HTTP/HTTPS stack
    if (!sendCheckReply(F("AT+CHTTPSSTOP"), F("+CHTTPSSTOP: 0"), 10000))
        return false;

    readline(); // Eat OK

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    return (replyLen > 0);
}

/********* FTP FUNCTIONS  ************************************/
/// @brief 
/// @param serverIP 
/// @param port 
/// @param username 
/// @param password 
/// @return 
boolean BK_modem::FTP_Connect(const char *serverIP, uint16_t port, const char *username, const char *password)
{
    char auxStr[100];

    // if (! sendCheckReply(F("AT+FTPCID=1"), m_ok_reply, 10000))
    //   return false;

    sendCheckReply(F("AT+FTPCID=1"), m_ok_reply, 10000); // Don't return false in case this is a reconnect attempt

    sprintf(auxStr, "AT+FTPSERV=\"%s\"", serverIP);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return false;

    if (port != 21)
    {
        sprintf(auxStr, "AT+FTPPORT=%i", port);

        if (!sendCheckReply(auxStr, m_ok_reply, 10000))
            return false;
    }

    if (strlen(username) > 0)
    {
        sprintf(auxStr, "AT+FTPUN=\"%s\"", username);

        if (!sendCheckReply(auxStr, m_ok_reply, 10000))
            return false;
    }

    if (strlen(password) > 0)
    {
        sprintf(auxStr, "AT+FTPPW=\"%s\"", password);

        if (!sendCheckReply(auxStr, m_ok_reply, 10000))
            return false;
    }

    return true;
}

/// @brief 
/// @return 
boolean BK_modem::FTP_Quit()
{
    if (!sendCheckReply(F("AT+FTPQUIT"), m_ok_reply, 10000))
        return false;

    return true;
}

/// @brief 
/// @param filePath 
/// @param oldName 
/// @param newName 
/// @return 
boolean BK_modem::FTP_Rename(const char *filePath, const char *oldName, const char *newName)
{
    char auxStr[50];

    sprintf(auxStr, "AT+FTPGETPATH=\"%s\"", filePath);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return false;

    sprintf(auxStr, "AT+FTPGETNAME=\"%s\"", oldName);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return false;

    sprintf(auxStr, "AT+FTPPUTNAME=\"%s\"", newName);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return false;

    if (!sendCheckReply(F("AT+FTPRENAME"), m_ok_reply, 2000))
        return false;

    if (!expectReply(F("+FTPRENAME: 1,0")))
        return false;

    return true;
}

/// @brief 
/// @param fileName 
/// @param filePath 
/// @return 
boolean BK_modem::FTP_Delete(const char *fileName, const char *filePath)
{
    char auxStr[50];

    sprintf(auxStr, "AT+FTPGETNAME=\"%s\"", fileName);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return false;

    sprintf(auxStr, "AT+FTPGETPATH=\"%s\"", filePath);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return false;

    if (!sendCheckReply(F("AT+FTPDELE"), m_ok_reply, 2000)) // It's NOT AT+FTPDELE=1
        return false;

    if (!expectReply(F("+FTPDELE: 1,0")))
        return false;

    return true;
}

/// @brief 
/// @param fileName 
/// @param filePath 
/// @param year 
/// @param month 
/// @param day 
/// @param hour 
/// @param minute 
/// @param second 
/// @return 
boolean BK_modem::FTP_MDTM(const char *fileName, const char *filePath, uint16_t *year,
                           uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    char auxStr[50];

    sprintf(auxStr, "AT+FTPGETNAME=\"%s\"", fileName);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return false;

    sprintf(auxStr, "AT+FTPGETPATH=\"%s\"", filePath);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return false;

    if (!sendCheckReply(F("AT+FTPMDTM"), m_ok_reply, 2000))
        return false;

    readline(10000);
    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    if (strstr(m_replybuffer, "+FTPMDTM: 1,0,") == NULL)
        return false;

    char timestamp[20];
    strcpy(timestamp, m_replybuffer + 14);
    // BK_DEBUG_PRINTLN(timestamp); // DEBUG

    // Timestamp format for SIM7000 is YYYYMMDDHHMMSS
    memset(auxStr, 0, sizeof(auxStr)); // Clear auxStr contents
    strncpy(auxStr, timestamp, 4);
    *year = atoi(auxStr);

    memset(auxStr, 0, sizeof(auxStr));
    strncpy(auxStr, timestamp + 4, 2);
    *month = atoi(auxStr);

    strncpy(auxStr, timestamp + 6, 2);
    *day = atoi(auxStr);

    strncpy(auxStr, timestamp + 8, 2);
    *hour = atoi(auxStr);

    strncpy(auxStr, timestamp + 10, 2);
    *minute = atoi(auxStr);

    strncpy(auxStr, timestamp + 12, 2);
    *second = atoi(auxStr);

    return true;
}

/// @brief 
/// @param fileName 
/// @param filePath 
/// @param numBytes 
/// @return 
const char *BK_modem::FTP_GET(const char *fileName, const char *filePath, uint16_t numBytes)
{
    char auxStr[100];
    const char *err = "error";

    sprintf(auxStr, "AT+FTPGETNAME=\"%s\"", fileName);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return err;

    sprintf(auxStr, "AT+FTPGETPATH=\"%s\"", filePath);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return err;

    if (!sendCheckReply(F("AT+FTPGET=1"), m_ok_reply, 10000))
        return err;

    if (!expectReply(F("+FTPGET: 1,1")))
        return err;

    if (numBytes <= 1024)
    {
        sprintf(auxStr, "AT+FTPGET=2,%i", numBytes);
        getReply(auxStr, 10000);

        if (strstr(m_replybuffer, "+FTPGET: 2,") == NULL)
            return err;
    }
    else
    {
        sprintf(auxStr, "AT+FTPEXTGET=2,%i,10000", numBytes);
        getReply(auxStr, 10000);

        if (strstr(m_replybuffer, "+FTPEXTGET: 2,") == NULL)
            return err;
    }

    readline(10000);

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    // if (! expectReply(m_ok_reply)) return err;

    // if (! expectReply(F("+FTPGET: 1,0"))) return err;

    return m_replybuffer;
}

/// @brief 
/// @param fileName 
/// @param filePath 
/// @param content 
/// @param numBytes 
/// @return 
boolean BK_modem::FTP_PUT(const char *fileName, const char *filePath, char *content, size_t numBytes)
{
    char auxStr[100];

    sprintf(auxStr, "AT+FTPPUTNAME=\"%s\"", fileName);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return false;

    sprintf(auxStr, "AT+FTPPUTPATH=\"%s\"", filePath);

    if (!sendCheckReply(auxStr, m_ok_reply, 10000))
        return false;

    // Use extended PUT method if there's more than 1024 bytes to send
    if (numBytes >= 1024)
    {
        // Repeatedly PUT data until all data is sent
        uint32_t remBytes = numBytes;
        uint16_t offset = 0; // Data offset
        char sendArray[strlen(content) + 1];
        strcpy(sendArray, content);

        while (remBytes > 0)
        {
            if (!sendCheckReply(F("AT+FTPEXTPUT=1"), m_ok_reply, 10000))
                return false;

            if (remBytes >= 300000)
            {
                sprintf(auxStr, "AT+FTPEXTPUT=2,%i,300000,10000", offset); // Extended PUT handles up to 300k
                offset = offset + 300000;
                remBytes = remBytes - 300000;

                strcpy(sendArray, content - offset); // Chop off the beginning
                if (strlen(sendArray) > 300000)
                    strcpy(sendArray, sendArray - 300000); // Chop off the end
            }
            else
            {
                sprintf(auxStr, "AT+FTPEXTPUT=2,%i,%i,10000", offset, remBytes);
                remBytes = 0;
            }

            if (!sendCheckReply(auxStr, F("+FTPEXTPUT: 0,"), 10000))
                return false;

            if (!sendCheckReply(sendArray, m_ok_reply, 10000))
                return false;
        }
    }

    if (!sendCheckReply(F("AT+FTPPUT=1"), m_ok_reply, 10000))
        return false;

    uint16_t maxlen;
    readline(10000);
    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    // Use regular FTPPUT method if there is less than 1024 bytes of data to send
    if (numBytes < 1024)
    {
        if (!parseReply(F("+FTPPUT: 1,1"), &maxlen, ',', 1))
            return false;

        // BK_DEBUG_PRINTLN(maxlen); // DEBUG

        // Repeatedly PUT data until all data is sent
        uint16_t remBytes = numBytes;

        while (remBytes > 0)
        {
            if (remBytes > maxlen)
                sprintf(auxStr, "AT+FTPPUT=2,%i", maxlen);
            else
                sprintf(auxStr, "AT+FTPPUT=2,%i", remBytes);

            getReply(auxStr);

            uint16_t sentBytes;
            if (!parseReply(F("+FTPPUT: 2"), &sentBytes, ',', 1))
                return false;

            // BK_DEBUG_PRINTLN(sentBytes); // DEBUG

            if (!sendCheckReply(content, m_ok_reply, 10000))
                return false;

            remBytes = remBytes - sentBytes; // Decrement counter

            // Check again for max length to send, repeat if needed
            // readline(10000);
            // BK_DEBUG_PRINT(F("\t<--- ")); BK_DEBUG_PRINTLN(m_replybuffer);
            // if (! parseReply(F("+FTPPUT: 1,1"), &maxlen, ',', 1))
            //   return false;
        }

        // No more data to be uploaded
        if (!sendCheckReply(F("AT+FTPPUT=2,0"), m_ok_reply, 10000))
            return false;

        if (!expectReply(F("+FTPPUT: 1,0")))
            return false;
    }
    else
    {
        if (strcmp(m_replybuffer, "+FTPPUT: 1,0") != 0)
            return false;

        if (!sendCheckReply(F("AT+FTPEXTPUT=0"), m_ok_reply, 10000))
            return false;
    }

    return true;
}

/********* MQTT FUNCTIONS  ************************************/

////////////////////////////////////////////////////////////
/// @brief 
/// @param protocol 
/// @param mqtt_message 
/// @param clientID 
/// @param username 
/// @param password 
void BK_modem::mqtt_connect_message(const char *protocol, byte *mqtt_message, const char *clientID, const char *username, const char *password)
{
    uint8_t i = 0;
    byte protocol_length = strlen(protocol);
    byte ID_length = strlen(clientID);
    byte username_length = strlen(username);
    byte password_length = strlen(password);

    mqtt_message[0] = 16; // MQTT message type CONNECT

    byte rem_length = 6 + protocol_length;
    // Each parameter will add 2 bytes + parameter length
    if (ID_length > 0)
    {
        rem_length += 2 + ID_length;
    }
    if (username_length > 0)
    {
        rem_length += 2 + username_length;
    }
    if (password_length > 0)
    {
        rem_length += 2 + password_length;
    }

    mqtt_message[1] = rem_length;      // Remaining length of message
    mqtt_message[2] = 0;               // Protocol name length MSB
    mqtt_message[3] = protocol_length; // Protocol name length LSB

    // Use the given protocol name (for example, "MQTT" or "MQIsdp")
    for (int i = 0; i < protocol_length; i++)
    {
        mqtt_message[4 + i] = byte(protocol[i]);
    }

    mqtt_message[4 + protocol_length] = 3; // MQTT protocol version

    if (username_length > 0 && password_length > 0)
    {                                            // has everything
        mqtt_message[5 + protocol_length] = 194; // Connection flag with username and password (11000010)
    }
    else if (password_length == 0)
    {                                            // Only has username
        mqtt_message[5 + protocol_length] = 130; // Connection flag with username only (10000010)
    }
    else if (username_length == 0)
    {                                           // Only has password
        mqtt_message[5 + protocol_length] = 66; // Connection flag with password only (01000010)
    }

    mqtt_message[6 + protocol_length] = 0;         // Keep-alive time MSB
    mqtt_message[7 + protocol_length] = 15;        // Keep-alive time LSB
    mqtt_message[8 + protocol_length] = 0;         // Client ID length MSB
    mqtt_message[9 + protocol_length] = ID_length; // Client ID length LSB

    // Client ID
    for (i = 0; i < ID_length; i++)
    {
        mqtt_message[10 + protocol_length + i] = clientID[i];
    }

    // Username
    if (username_length > 0)
    {
        mqtt_message[10 + protocol_length + ID_length] = 0;               // username length MSB
        mqtt_message[11 + protocol_length + ID_length] = username_length; // username length LSB

        for (i = 0; i < username_length; i++)
        {
            mqtt_message[12 + protocol_length + ID_length + i] = username[i];
        }
    }

    // Password
    if (password_length > 0)
    {
        mqtt_message[12 + protocol_length + ID_length + username_length] = 0;               // password length MSB
        mqtt_message[13 + protocol_length + ID_length + username_length] = password_length; // password length LSB

        for (i = 0; i < password_length; i++)
        {
            mqtt_message[14 + protocol_length + ID_length + username_length + i] = password[i];
        }
    }
}

/// @brief 
/// @param mqtt_message 
/// @param topic 
/// @param message 
void BK_modem::mqtt_publish_message(byte *mqtt_message, const char *topic, const char *message)
{
    uint8_t i = 0;
    byte topic_length = strlen(topic);
    byte message_length = strlen(message);

    mqtt_message[0] = 48;                                // MQTT Message Type PUBLISH
    mqtt_message[1] = 2 + topic_length + message_length; // Remaining length
    mqtt_message[2] = 0;                                 // Topic length MSB
    mqtt_message[3] = topic_length;                      // Topic length LSB

    // Topic
    for (i = 0; i < topic_length; i++)
    {
        mqtt_message[4 + i] = topic[i];
    }

    // Message
    for (i = 0; i < message_length; i++)
    {
        mqtt_message[4 + topic_length + i] = message[i];
    }
}

/// @brief 
/// @param mqtt_message 
/// @param topic 
/// @param QoS 
void BK_modem::mqtt_subscribe_message(byte *mqtt_message, const char *topic, byte QoS)
{
    uint8_t i = 0;
    byte topic_length = strlen(topic);

    mqtt_message[0] = 130;              // MQTT Message Type SUBSCRIBE
    mqtt_message[1] = 5 + topic_length; // Remaining length
    mqtt_message[2] = 0;                // Packet ID MSB
    mqtt_message[3] = 1;                // Packet ID LSB
    mqtt_message[4] = 0;                // Topic length MSB
    mqtt_message[5] = topic_length;     // Topic length LSB

    // Topic
    for (i = 0; i < topic_length; i++)
    {
        mqtt_message[6 + i] = topic[i];
    }

    mqtt_message[6 + topic_length] = QoS; // QoS byte
}

/// @brief 
/// @param mqtt_message 
void BK_modem::mqtt_disconnect_message(byte *mqtt_message)
{
    mqtt_message[0] = 0xE0; // msgtype = connect
    mqtt_message[1] = 0x00; // length of message (?)
}

/// @brief 
/// @param packet 
/// @param len 
/// @return 
boolean BK_modem::mqtt_sendPacket(byte *packet, byte len)
{
    // Send packet and get response
    BK_DEBUG_PRINT(F("\t---> "));

    for (int j = 0; j < len; j++)
    {
        // if (packet[j] == NULL) break; // We've reached the end of the actual content
        SerialSIM->write(packet[j]); // Needs to be "write" not "print"
        BK_DEBUG_PRINT(packet[j]);   // Message contents
        BK_DEBUG_PRINT(" ");         // Space out the bytes
    }

    SerialSIM->write(byte(26)); // End of packet
    BK_DEBUG_PRINT(byte(26));

    readline(3000); // Wait up to 3 seconds to send the data
    BK_DEBUG_PRINTLN("");
    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    return (strcmp(m_replybuffer, "SEND OK") == 0);
}

////////////////////////////////////////////////////////////
/// @brief 
/// @param protocol 
/// @param clientID 
/// @param username 
/// @param password 
/// @return 
boolean BK_modem::MQTTconnect(const char *protocol, const char *clientID, const char *username, const char *password)
{
    flushInput();

    SerialSIM->println(F("AT+CIPSEND"));
    readline();

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);
    if (m_replybuffer[0] != '>')
        return false;

    byte mqtt_message[127];
    mqtt_connect_message(protocol, mqtt_message, clientID, username, password);

    if (!mqtt_sendPacket(mqtt_message, 14 + strlen(protocol) + strlen(clientID) + strlen(username) + strlen(password)))
        return false;

    return true;
}

/// @brief
/// @param topic
/// @param message
/// @return
boolean BK_modem::MQTTpublish(const char *topic, const char *message)
{
    flushInput();
    SerialSIM->println(F("AT+CIPSEND"));
    readline();
    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);
    if (m_replybuffer[0] != '>')
        return false;

    byte mqtt_message[127];
    mqtt_publish_message(mqtt_message, topic, message);

    if (!mqtt_sendPacket(mqtt_message, 4 + strlen(topic) + strlen(message)))
        return false;

    return true;
}

/// @brief
/// @param topic
/// @param QoS
/// @return
boolean BK_modem::MQTTsubscribe(const char *topic, byte QoS)
{
    flushInput();
    SerialSIM->println(F("AT+CIPSEND"));
    readline();
    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);
    if (m_replybuffer[0] != '>')
        return false;

    byte mqtt_message[127];
    mqtt_subscribe_message(mqtt_message, topic, QoS);

    if (!mqtt_sendPacket(mqtt_message, 7 + strlen(topic)))
        return false;

    return true;
}

boolean BK_modem::MQTTunsubscribe(const char *topic)
{
    return false;
}

boolean BK_modem::MQTTreceive(const char *topic, const char *buf, int maxlen)
{
    return false;
}

boolean BK_modem::MQTTdisconnect(void)
{
    return false;
}

/********* SSL FUNCTIONS  ************************************/
boolean BK_modem::addRootCA(const char *root_cert)
{
    char rootCA[10240];
    strcpy(rootCA, root_cert);
    m_rootCA = rootCA;

    if (!strlen(m_rootCA))
    {
        return false;
    }

    return true;
}

/********* UDP FUNCTIONS  ************************************/
/// @brief
/// @param server
/// @param port
/// @return
boolean BK_modem::UDPconnect(char *server, uint16_t port)
{
    flushInput();

    // close all old connections
    stop();
    delay(100);

    // single connection at a time
    if (!sendCheckReply(F("AT+CIPMUX=0"), m_ok_reply))
        return false;

    // manually read data
    if (!sendCheckReply(F("AT+CIPRXGET=1"), m_ok_reply))
        return false;

    BK_DEBUG_PRINT(F("AT+CIPSTART=\"UDP\",\""));
    BK_DEBUG_PRINT(server);
    BK_DEBUG_PRINT(F("\",\""));
    BK_DEBUG_PRINT(port);
    BK_DEBUG_PRINTLN(F("\""));

    SerialSIM->print(F("AT+CIPSTART=\"UDP\",\""));
    SerialSIM->print(server);
    SerialSIM->print(F("\",\""));
    SerialSIM->print(port);
    SerialSIM->println(F("\""));

    if (!expectReply(m_ok_reply))
        return false;
    if (!expectReply(F("CONNECT OK")))
        return false;

    // looks like it was a success (?)
    return true;
}

/**************** TCP FUNCTIONS + SSL *************************/

 /// @brief Start a TCP connection
 /// @param server Pointer to a buffer with the server to connect to
 /// @param port Pointer to a buffer witht the port to connect to
 /// @return true: success, false: failure
boolean BK_modem::TCPconnect(char *server, uint16_t port, uint8_t mux)
{
    if (BK_SSL)
    { // https:
        flushInput();

        //  Report Mobile Equipment Error
        if (!setMobileEquipmentError(1)) // turn on error result codes and use nummeric values.
        {
            return false;
        }

        // Check config error

        // if (!sendCheckReply(F("AT+CMEE?"), F("+CMEE: 2"), 20000))
        if (getMobileEquipmentError(2))
        {
            return false;
        }

        // Set TCP/UDP Identifier
        //if (!sendCheckReply(F("AT+CACID=1"), m_ok_reply))
        if (!sendCheckReply(F("AT+CACID="), mux, m_ok_reply))
        {
            return false;
        }

        m_CID_CA = 1;

        // Configure SSL Parameters of a Context Identifier
        if (!sendCheckReply(F("AT+CSSLCFG=\"sslversion\",1,3"), m_ok_reply))
        {
            return false;
        }

        if (!sendCheckReply(F("AT+CSSLCFG=\"protocol\",1,1"), m_ok_reply))
        {
            return false;
        }

        SerialSIM->println(F("AT+CSSLCFG=\"ctxindex\",1"));
        readline();
        if (!expectReply(m_ok_reply))
        {
            return false;
        }

        // Init FS AT command
        if (!sendCheckReply(F("AT+CFSINIT"), m_ok_reply))
        {
            return false;
        }

        // Load CA
        SerialSIM->print(F("AT+CFSWFILE=3,\"ca.crt\",0,\""));
        SerialSIM->print(strlen(m_rootCA));
        SerialSIM->print(F("\",\""));
        SerialSIM->print(5000);
        SerialSIM->println(F("\""));

        if (!expectReply(F("DOWNLOAD")))
        {
            return false;
        }

        SerialSIM->print(m_rootCA);     // After 'download', sent certificate file through the "SerialSIM" serial port.
        readline(2000, true);           // wait for "OK"

        if (!((m_replybuffer[0] == 'O') && (m_replybuffer[1] == 'K')))
        {
            return false;
        }

        // Free data buffer
        if (!sendCheckReply(F("AT+CFSTERM"), m_ok_reply))
        {
            return false;
        }

        // Init FS AT command
        if (!sendCheckReply(F("AT+CFSINIT"), m_ok_reply))
        {
            return false;
        }

        char CF[20] = "+CFSGFIS: ";
        itoa((int)strlen(m_rootCA), CF + 10, 10);

        // if (! sendCheckReply(F("AT+CFSGFIS=3,\"ca.crt\""), (char*)CF, 300)) return false; // Get cert file size.
        if (!sendCheckReply(F("AT+CFSTERM"), m_ok_reply))
            return false;

        // Conversion CA certificate format. 2 means CA type. 
        if (!sendCheckReply(F("AT+CSSLCFG=\"convert\",2,\"ca.crt\""), m_ok_reply))
        {
            return false;
        }

        // Set SSL certificate and timeout parameters
        if (!sendCheckReply(F("AT+CASSLCFG=1,\"cacert\",\"ca.crt\""), m_ok_reply))
            return false;

        if (!sendCheckReply(F("AT+CASSLCFG=1,\"ssl\",1"), m_ok_reply))
            return false;

        if (!sendCheckReply(F("AT+CASSLCFG=1,\"crindex\",1"), m_ok_reply))
            return false;
            
        if (!sendCheckReply(F("AT+CASSLCFG=1,\"protocol\",0"), m_ok_reply))
            return false;

        // if (! openWirelessConnection(true)) return false;
        // if (! wirelessConnStatus()) return false;

        char server_f[100];
        strcpy(server_f, server);
        m_server_CA = server_f;
        m_port_CA = port;

        SerialSIM->print(F("AT+CAOPEN=1,\""));
        SerialSIM->print(server);
        SerialSIM->print(F("\",\""));
        SerialSIM->print(port);
        SerialSIM->println(F("\""));

        if (!expectReply(F("+CAOPEN: 1,0")))
        {
            return false;
        }
    }
    else
    { // http:
        flushInput();

        // close all old connections
        stop();
        delay(100);

        // single connection at a time
        if (!sendCheckReply(F("AT+CIPMUX=0"), m_ok_reply))
        {
            return false;
        }

        // manually read data
        if (!sendCheckReply(F("AT+CIPRXGET=1"), m_ok_reply))
        {
            return false;
        }

        BK_DEBUG_PRINT(F("AT+CIPSTART=\"TCP\",\""));
        BK_DEBUG_PRINT(server);
        BK_DEBUG_PRINT(F("\",\""));
        BK_DEBUG_PRINT(port);
        BK_DEBUG_PRINTLN(F("\""));

        SerialSIM->print(F("AT+CIPSTART=\"TCP\",\""));
        SerialSIM->print(server);
        SerialSIM->print(F("\",\""));
        SerialSIM->print(port);
        SerialSIM->println(F("\""));

        if (!expectReply(m_ok_reply))
        {
            return false;
        }

        if (!expectReply(F("CONNECT OK")))
        {
            return false;
        }
    }

    // looks like it was a success (?)
    return true;
}

/// @brief
/// @param
/// @return
boolean BK_modem::TCPclose(void)
{
    return sendCheckReply(F("AT+CIPCLOSE"), F("CLOSE OK"));
}

/// @brief
/// @param
/// @return
boolean BK_modem::TCPconnected(void)
{
    if (BK_SSL)
    {
        char CA[100] = "+CAOPEN: ";
        itoa(m_CID_CA, CA + 9, 10);
        strcat(CA, ",\"");
        strcat(CA, m_server_CA);
        strcat(CA, "\",");
        char _port_CA_p[10];
        itoa((int)m_port_CA, _port_CA_p, 10);
        strcat(CA, _port_CA_p);

        getReply(F("AT+CAOPEN?"));

        if (strstr(m_replybuffer, CA) == NULL)
        {
            return false;
        }
    }
    else
    {
        if (!sendCheckReply(F("AT+CIPSTATUS"), m_ok_reply, 100))
        {
            return false;
        }
    }

    readline(100);
    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    return (strcmp(m_replybuffer, "STATE: CONNECT OK") == 0);
}

/// @brief
/// @param packet
/// @param len
/// @return
boolean BK_modem::TCPsend(char *packet, uint8_t len)
{
    BK_DEBUG_PRINT(F("AT+CIPSEND="));
    BK_DEBUG_PRINTLN(len);

#ifdef BK_MODEM_DEBUG
    for (uint16_t i = 0; i < len; i++)
    {
        BK_DEBUG_PRINT(F(" 0x"));
        BK_DEBUG_PRINT(packet[i], HEX);
    }
#endif

    BK_DEBUG_PRINTLN();

    if (BK_SSL)
    { // https
        flushInput();

        SerialSIM->print(F("AT+CASEND=1,\""));
        SerialSIM->print(len);
        SerialSIM->println(F("\""));
        readline();
    }
    else
    { // http
        SerialSIM->print(F("AT+CIPSEND="));
        SerialSIM->println(len);
        readline();
    }

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    if (m_replybuffer[0] != '>')
        return false;

    SerialSIM->write(packet, len);
    readline(3000); // wait up to 3 seconds to send the data

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    if (BK_SSL)
    {
        return (strcmp(m_replybuffer, "OK") == 0);
    }
    else
    {
        return (strcmp(m_replybuffer, "SEND OK") == 0);
    }
}

uint16_t BK_modem::TCPavailable(void)
{
    uint16_t avail;

    if (!sendParseReply(F("AT+CIPRXGET=4"), F("+CIPRXGET: 4,"), &avail, ',', 0))
        return false;

    BK_DEBUG_PRINT(avail);
    BK_DEBUG_PRINTLN(F(" bytes available"));

    return avail;
}

uint16_t BK_modem::TCPread(uint8_t *buff, uint8_t len)
{
    uint16_t avail;

    SerialSIM->print(F("AT+CIPRXGET=2,"));
    SerialSIM->println(len);
    readline();

    if (!parseReply(F("+CIPRXGET: 2,"), &avail, ',', 0))
    {
        return false;
    }

    readRaw(avail);

#ifdef BK_MODEM_DEBUG
    BK_DEBUG_PRINT(avail);
    BK_DEBUG_PRINTLN(F(" bytes read"));

    for (uint8_t i = 0; i < avail; i++)
    {
        BK_DEBUG_PRINT(F(" 0x"));
        BK_DEBUG_PRINT(m_replybuffer[i], HEX);
    }

    BK_DEBUG_PRINTLN();

#endif

    memcpy(buff, m_replybuffer, avail);

    return avail;
}

boolean BK_modem::TCPdns(char *hostname, char *buff, uint8_t len)
{
    SerialSIM->print(F("AT+CDNSGIP="));
    SerialSIM->println(hostname);

    if (!expectReply(m_ok_reply))
        return false;

    readline();

    if (!parseReplyQuoted(F("+CDNSGIP: 1,"), buff, len, ',', 1))
    {
        return false;
    }

    return true;
}

/********* HTTP LOW LEVEL FUNCTIONS  ************************************/

/// @brief  : HTTP connection Open 
/// @return 
boolean BK_modem::HTTP_init()
{
    return sendCheckReply(F("AT+HTTPINIT"), m_ok_reply);
}

/// @brief : HTTP connection closed
/// @return 
boolean BK_modem::HTTP_term()
{
    return sendCheckReply(F("AT+HTTPTERM"), m_ok_reply);
}

/// @brief 
/// @param parameter 
/// @param quoted 
void BK_modem::HTTP_para_start(FStringPtr parameter, boolean quoted)
{
    flushInput();

    BK_DEBUG_PRINT(F("\t---> "));
    BK_DEBUG_PRINT(F("AT+HTTPPARA=\""));
    // BK_DEBUG_PRINT(parameter);
    // BK_DEBUG_PRINTLN('"');

    SerialSIM->print(F("AT+HTTPPARA=\""));
    SerialSIM->print(parameter);

    if (quoted)
    {
        SerialSIM->print(F("\",\""));
    }
    else
    {
        SerialSIM->print(F("\","));
    }
}

/// @brief 
/// @param quoted 
/// @return 
boolean BK_modem::HTTP_para_end(boolean quoted)
{
    if (quoted)
    {
        SerialSIM->println('"');
    }
    else
    {
        SerialSIM->println();
    }

    return expectReply(m_ok_reply);
}

/// @brief 
/// @param parameter 
/// @param value 
/// @return 
boolean BK_modem::HTTP_para(FStringPtr parameter, const char *value)
{
    HTTP_para_start(parameter, true);
    SerialSIM->print(value);

    BK_DEBUG_PRINTLN(parameter + String(F("\":")) + String(value));

    return HTTP_para_end(true);
}

/// @brief 
/// @param parameter 
/// @param value 
/// @return 
boolean BK_modem::HTTP_para(FStringPtr parameter, FStringPtr value)
{
    HTTP_para_start(parameter, true);
    SerialSIM->print(value);

    BK_DEBUG_PRINTLN(parameter + String(F("\":\"")) + String(value) + String(F("\"")));

    return HTTP_para_end(true);
}

/// @brief 
/// @param parameter 
/// @param value 
/// @return 
boolean BK_modem::HTTP_para(FStringPtr parameter, int32_t value)
{
    HTTP_para_start(parameter, false);
    SerialSIM->print(value);

    BK_DEBUG_PRINTLN(parameter + String(F("\":")) + String(value));

    return HTTP_para_end(false);
}

/// @brief 
/// @param size 
/// @param maxTime 
/// @return 
boolean BK_modem::HTTP_data(uint32_t size, uint32_t maxTime)
{
    flushInput();

    BK_DEBUG_PRINT(F("\t---> "));
    BK_DEBUG_PRINT(F("AT+HTTPDATA="));
    BK_DEBUG_PRINT(size);
    BK_DEBUG_PRINT(',');
    BK_DEBUG_PRINTLN(maxTime);

    SerialSIM->print(F("AT+HTTPDATA="));
    SerialSIM->print(size);
    SerialSIM->print(",");
    SerialSIM->println(maxTime);

    return expectReply(F("DOWNLOAD"));
}

/// @brief 
/// @param method 
/// @param status 
/// @param datalen 
/// @param timeout 
/// @return 
boolean BK_modem::HTTP_action(uint8_t method, uint16_t *status,
                              uint16_t *datalen, int32_t timeout)
{
    // Send GET/POST request to Server website.
    if (!sendCheckReply(F("AT+HTTPACTION="), method, m_ok_reply, timeout))
    {
        return false;
    }

    for(int reply = 3; reply; reply--)
    {
        // Parse response status and size.
        if( readline(timeout) > 0)
        {
            BK_DEBUG_PRINTLN( F("HTTPACTION responce: ") + String(m_replybuffer));
            break;
        }

        if (sendCheckReply(F("AT+HTTPSTATUS?"), F("+HTTPSTATUS: ")))        // Read HTTP status.
        {
            parseReply(F("+HTTPSTATUS: "), status, ',', 1);         
            readline();     // eat 'OK'

            BK_DEBUG_PRINTLN( F("HTTPSTATUS: ") + String(*status));

            // Check: The status of getting or posting session is in progress ?
            if( status == 0)
            {
                break;                                              // The status of getting or posting session is over.
            }
        }
    }

    if (!parseReply(F("+HTTPACTION:"), status, ',', 1))
    {
        return false;
    }

    if (!parseReply(F("+HTTPACTION:"), datalen, ',', 2))
    {
        return false;
    }

    return true;
}

/// @brief
/// @param datalen
/// @return
boolean BK_modem::HTTP_readall(uint16_t *datalen)
{
    getReply(F("AT+HTTPREAD"));

    if (!parseReply(F("+HTTPREAD:"), datalen, ',', 0))
    {
        return false;
    }

    // Clear buffer.
    memset( m_responcebuffer, 0x00, RECEIVE_BUFFER_LENGHT_MAX-1);
    readRaw( &m_responcebuffer[0], *datalen);

    BK_DEBUG_PRINT(F("Responce message: "));
    BK_DEBUG_PRINTLN((char*)m_responcebuffer);
    //BK_DEBUG_PRINTLN(m_replybuffer);
 
    return true;
}

/**
 * @brief Enable or disable SSL
 *
 * @param onoff true: enable false: disable
 * @return true: success, false: failure
 */
boolean BK_modem::HTTP_ssl(boolean onoff)
{
    // tell the modem to accept all certs with
    // AT+SHSSL=1,""
    return sendCheckReply(F("AT+HTTPSSL="), onoff ? 1 : 0, m_ok_reply);
}

/********* HTTP HIGH LEVEL FUNCTIONS ***************************/

boolean BK_modem::HTTP_GET_start(char *url, uint16_t *status, uint16_t *datalen)
{
    if (!HTTP_setup(url))
    {
        return false;
    }

    // HTTP GET
    if (!HTTP_action(_HTTP_GET, status, datalen, 30000))
    {
        return false;
    }

    BK_DEBUG_PRINT(F("Status: "));
    BK_DEBUG_PRINTLN(*status);
    BK_DEBUG_PRINT(F("Len: "));
    BK_DEBUG_PRINTLN(*datalen);

    // HTTP response data
    if (!HTTP_readall(datalen))
    {
        return false;
    }

    return true;
}

/// @brief 
/// @param  
void BK_modem::HTTP_GET_end(void)
{
    HTTP_term();
}

/// @brief 
/// @param url              ; Server URL string
/// @param contenttype      
/// @param headerdata       : HTTP User header.
/// @param postdata         : HTTP Body.
/// @param postdatalen 
/// @param status           ; HTTP response status code
/// @param datalen          : Responce datalen
/// @return 
boolean BK_modem::HTTP_POST_start(char *url,
                                  FStringPtr contenttype,
                                  String &headerdata,
                                  const uint8_t *postdata, uint16_t postdatalen,
                                  uint16_t *status, uint16_t *datalen)
{
    // Open HTTP connection.
    if (!HTTP_setup(url))
    {
        *status = -100;
        return false;
    }

    if (!HTTP_para(F("CONTENT"), contenttype))
    {
        *status = -101;
        return false;
    }

    // set HTTP customer header contents.
    //BK_DEBUG_PRINTLN(F("HTTP Header info: ") + headerdata);

    int endIdx = 0;
    for(uint startIdx = 0; startIdx < headerdata.length(); startIdx++)
    {
        if( (endIdx = headerdata.indexOf(';', startIdx)) < 0)
        {
            break;
        }

        char headerStr[ 30 + (endIdx - startIdx)];

        String sendbuf = headerdata.substring( startIdx, endIdx);
        sprintf(headerStr, PSTR("AT+HTTPPARA=\"USERDATA\",\"%s\""), sendbuf.c_str());

        //BK_DEBUG_PRINTLN(F("\t---> Send header: ") + String(headerStr));
        sendCheckReply(headerStr, m_ok_reply, 10000);
    
        startIdx = endIdx;
    }

    // HTTP POST data
    if (!HTTP_data(postdatalen, 10000))
    {
         *status = -102;
        return false;
    }

    BK_DEBUG_PRINTLN(F("\t---> Send Body: ") + String((char *)postdata));
    SerialSIM->write(postdata, postdatalen);

    if (!expectReply(m_ok_reply, 15000))
    {
        *status = HTTP_CODE_REQUEST_TIMEOUT;                                      // time out.
        return false;
    }

    // HTTP POST
    HTTP_action(_HTTP_POST, status, datalen, 15000);

    BK_DEBUG_PRINT(F("Status: "));
    BK_DEBUG_PRINTLN(*status);
    BK_DEBUG_PRINT(F("Responce Len: "));
    BK_DEBUG_PRINTLN(*datalen);

    // HTTP response data ?
    if (*datalen > 0)
    {
        HTTP_readall(datalen);
    }

    if (*status != HTTP_CODE_OK)
    {
        return false;
    }

    return true;
}

/// @brief 
/// @param  
void BK_modem::HTTP_POST_end(void)
{
    HTTP_term();
}

/// @brief Set parameter for HTTP session
/// @param clientID 
void BK_modem::setClientID(uint32_t clientID)
{
    this->m_clientID = clientID;
}

/// @brief 
/// @param useragent 
void BK_modem::setUserAgent(FStringPtr useragent)
{
    this->m_useragent = useragent;
}

/// @brief 
/// @param onoff 
void BK_modem::setHTTPSRedirect(boolean onoff)
{
    m_httpsredirect = onoff;
}

/********* HTTP HELPERS ****************************************/

boolean BK_modem::HTTP_setup(char *url)
{
    // Handle any pending
    HTTP_term();

    // Initialize and set parameters
    if (!HTTP_init())
    {
        return false;
    }

    if (!HTTP_para(F("CID"), m_clientID))       // HTTP session ID.
    {
        return false;
    }

    if (!HTTP_para(F("UA"), m_useragent))
    {
        return false;
    }

    if (!HTTP_para(F("URL"), url))
    {
        return false;
    }

    // HTTPS redirect
    if (m_httpsredirect)
    {
        HTTP_para(F("REDIR"), 1);

        // if (!HTTP_para(F("REDIR"), 1))
        // {
        //     return false;
        // }

        // not working for Sim7000E
        // if (!HTTP_ssl(true))
        // {
        //     return false;
        // }
    }

    return true;
}

/********* HELPERS *********************************************/

/// @brief 
/// @param reply 
/// @param timeout 
/// @return 
boolean BK_modem::expectReply(FStringPtr reply, uint16_t timeout)
{
    readline(timeout);

    BK_DEBUG_PRINT(F("\tR5<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    return (prog_char_strcmp(m_replybuffer, (prog_char *)reply) == 0);
}

/********* LOW LEVEL *******************************************/

/// @brief  declare available(void) => Stream: virtual int available();
/// @param
/// @return
inline int BK_modem::available(void)
{
    return SerialSIM->available();
}

/// @brief
/// @param x
/// @return
inline size_t BK_modem::write(uint8_t x)
{
    return SerialSIM->write(x);
}

inline int BK_modem::read(void)
{
    return SerialSIM->read();
}

inline int BK_modem::peek(void)
{
    return SerialSIM->peek();
}

/// @brief
inline void BK_modem::flush()
{
    SerialSIM->flush();
}

/// @brief Flush: SerialSIM receive buffer is empty.
void BK_modem::flushInput()
{
    // Read all available serial input to flush pending data.
    uint16_t timeoutloop = 0;
    while (timeoutloop++ < 40)
    {
        while (available())
        {
            read();
            timeoutloop = 0;    // If char was received reset the timer.
        }

        delay(1);
    }
}

/// @brief :
/// @param cnt
/// @return : xx char in m_replybuffer.
uint16_t BK_modem::readRaw(uint16_t cnt)
{
    uint16_t idx = 0;

    while (cnt && (idx < sizeof(m_replybuffer) - 1))
    {
        if (SerialSIM->available())
        {
            m_replybuffer[idx] = SerialSIM->read();
            idx++;

            cnt--;
        }
    }

    m_replybuffer[idx] = 0;

    return idx;
}

/// @brief 
/// @param  : SIM7000 receive buffer.
/// @param cnt 
/// @return : xx char in rspbuffer.
uint16_t BK_modem::readRaw(char *rspbuffer, uint16_t cnt)
{
    uint16_t len = readRaw(cnt);
    memcpy(rspbuffer, m_replybuffer, len);

    return len;
}

/// @brief Read lines from COM port (USB<->RS232)
///
///    Receive buffer lenght max. 512 => index >= 512
///    '\r\n' chars skip from responce.
///
///    \r is carriage return (CR(13))
///    \n is line feed (LF(10)).
///
/// @param timeout 
/// @param multiline 
/// @return 
uint16_t BK_modem::readline(uint16_t timeout, boolean multiline)
{
    uint32_t index = 0;
    uint8_t ready = false;
    uint8_t svtimeout = timeout;

    // for (uint32_t start = millis(); millis() - start < timeout;)

#ifdef BK_MODEM_HEXDEBUG
    BK_DEBUG_PRINT(F("\t\t<")); // start pos. receive chars. --> HEX format.
#endif

    while (timeout--)
    {
        if (index >= RECEIVE_BUFFER_LENGHT_MAX - 1)
        {
            // BK_DEBUG_PRINTLN(F("SPACE"));
            ready = true;
            break;
        }

        svtimeout--;

        while (SerialSIM->available())
        {
            char chr = SerialSIM->read();
            // String hlp = SerialSIM->readString();     // dynamische memory.

#ifdef BK_MODEM_HEXDEBUG
            if (chr < 0x20)
            { // char values below 0x20
                char format[3];
                sprintf(format, "%02x", chr); // 0%x
                BK_DEBUG_PRINT(format);
            }
            else
            {
                // BK_DEBUG_PRINT(chr, HEX);     // display Hex value char.
                BK_DEBUG_PRINT(chr);             // display ASCII char.
            }
#endif

            if (chr == '\r')
            { // Ignore Carriage Return. (0x0D char.)
                if (multiline)
                {
                    timeout = 100;   // next char. must reveived in 100mse else time-out end.
                }

                continue;
            }

            if (chr == '\n')
            { // End of line Line Feed (new line) (0x0A char.)
                if (index == 0)
                {
                    continue;       // the first 0x0A is ignored
                }

                if (!multiline)
                {
                    timeout = 0;    // the second 0x0A is the end of the line.
                    break;
                }
            }
            else
            {
                if (multiline && svtimeout > 90)
                {
                    timeout = svtimeout;
                }
            }

            m_replybuffer[index] = chr;
            // BK_DEBUG_PRINT(chr, HEX);
            // BK_DEBUG_PRINT(" => #");
            // BK_DEBUG_PRINTLN(chr);

            // increment "index" ctr.
            if (++index >= RECEIVE_BUFFER_LENGHT_MAX - 1)
            {
                break;
            }

        } // while serial

        if (timeout == 0)
        { // end
            ready = true;
            break;
        }

        delay(1);

        if ((timeout % 1000) == 0)
        {                    // Yield to other tasks
                             // timeout value dividing by 1000 results in integer value, with a remainder of 0.
            //yield();       // Delete: Sometime MCU in panic mode.
            LTE_GSM_YIELD(); // Soluction: give waiting thread(s) CPU time, every 1 sec.
        }

    } // while timeout [process]

#ifdef BK_MODEM_HEXDEBUG
    BK_DEBUG_PRINTLN(F(">"));
#endif

    m_replybuffer[index] = 0x00; // set null terminator.

    return ready ? index : 0;
}

/// @brief
/// @param send
/// @param timeout
/// @return
uint16_t BK_modem::getReply(const char *send, uint16_t timeout, boolean multiline)
{
    flushInput();

    BK_DEBUG_PRINT(F("\tS1---> "));
    BK_DEBUG_PRINTLN(send);

    SerialSIM->println(send);       // include send '\r' => <CR>.
    // SerialSIM->write(send, strlen(send));

    uint16_t len = readline(timeout, multiline);

    BK_DEBUG_PRINT(F("\tR1<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    LTE_GSM_YIELD();

    return len;
}

/// @brief
/// @param send
/// @param timeout
/// @param multiline
/// @return
uint16_t BK_modem::getReply(FStringPtr send, uint16_t timeout, boolean multiline)
{
    flushInput();

    BK_DEBUG_PRINT(F("\tS2:---> "));
    BK_DEBUG_PRINTLN(send);

    SerialSIM->println(send);

    uint16_t len = readline(timeout, multiline);

    BK_DEBUG_PRINT(F("\tR2:<--- "));
    if( len != 0)
    {
        BK_DEBUG_PRINTLN(m_replybuffer);
    }
    else
    {
        BK_DEBUG_PRINTLN(F("Time-Out"));
    }

    return len;
}

// Send prefix, suffix[], and newline. Return response (and also set m_replybuffer with response).
uint16_t BK_modem::getReply(FStringPtr prefix, char *suffix, uint16_t timeout)
{
    flushInput();

    BK_DEBUG_PRINT(F("\tS3:---> "));
    BK_DEBUG_PRINT(prefix);
    BK_DEBUG_PRINTLN(suffix);

    SerialSIM->print(prefix);
    SerialSIM->println(suffix);

    uint16_t l = readline(timeout);

    BK_DEBUG_PRINT(F("\tR3:<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    LTE_GSM_YIELD();

    return l;
}

// Send prefix, int suffix, and newline. Return response (and also set m_replybuffer with response).
uint16_t BK_modem::getReply(FStringPtr prefix, int32_t suffix, uint16_t timeout, boolean multiline)
{
    flushInput();

    BK_DEBUG_PRINT(F("\tS4:---> "));
    BK_DEBUG_PRINT(prefix);
    BK_DEBUG_PRINTLN(suffix, DEC);

    SerialSIM->print(prefix);
    SerialSIM->println(suffix, DEC);

    uint16_t len = readline(timeout, multiline);

    BK_DEBUG_PRINT(F("\tR4:<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    return len;
}

// Send prefix, int suffix, int suffix2, and newline. Return response (and also set m_replybuffer with response).
uint16_t BK_modem::getReply(FStringPtr prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout)
{
    // to besure SerialSIM receive buffer is empty.
    flushInput();

    BK_DEBUG_PRINT(F("\tS6---> "));
    BK_DEBUG_PRINT(prefix);
    BK_DEBUG_PRINT(suffix1, DEC);
    BK_DEBUG_PRINT(',');
    BK_DEBUG_PRINTLN(suffix2, DEC);

    SerialSIM->print(prefix);
    SerialSIM->print(suffix1);
    SerialSIM->print(',');
    SerialSIM->println(suffix2, DEC);

    uint16_t l = readline(timeout);

    BK_DEBUG_PRINT(F("\tR6<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    return l;
}

// Send prefix, ", suffix, ", and newline. Return response (and also set m_replybuffer with response).
uint16_t BK_modem::getReplyQuoted(FStringPtr prefix, FStringPtr suffix, uint16_t timeout)
{
    flushInput();

    BK_DEBUG_PRINT(F("\tS7---> "));
    BK_DEBUG_PRINT(prefix);
    BK_DEBUG_PRINT('"');
    BK_DEBUG_PRINT(suffix);
    BK_DEBUG_PRINTLN('"');

    SerialSIM->print(prefix);
    SerialSIM->print('"');
    SerialSIM->print(suffix);
    SerialSIM->println('"'); // and newline

    uint16_t l = readline(timeout);

    BK_DEBUG_PRINT(F("\tR7<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    return l;
}

/// @brief
/// @param send
/// @param reply
/// @param timeout
/// @return
boolean BK_modem::sendCheckReply(const char *send, const char *reply, uint16_t timeout)
{
    if (!getReply(send, timeout))
    {
        return false;
    }

    /*
        for (uint8_t i=0; i<strlen(m_replybuffer); i++)
        {
            BK_DEBUG_PRINT(m_replybuffer[i], HEX); BK_DEBUG_PRINT(" ");
        }

        BK_DEBUG_PRINTLN();
        for (uint8_t i=0; i<strlen(reply); i++)
        {
            BK_DEBUG_PRINT(reply[i], HEX); BK_DEBUG_PRINT(" ");
        }

        BK_DEBUG_PRINTLN();
    */

    size_t len = strlen(reply);
    return (strncmp(m_replybuffer, reply, len) == 0);
}

/// @brief 
/// @param send 
/// @param reply 
/// @param timeout 
/// @return : return 0 then timeout else receive data.
boolean BK_modem::sendCheckReply(FStringPtr send, FStringPtr reply, uint16_t timeout, boolean multiline)
{
    if (!getReply(send, timeout, multiline))
    {
        return false;
    }

    size_t len = prog_char_strlen((prog_char *)reply);

    // BK_DEBUG_PRINTLN(F("receive: ") + String(m_replybuffer) + F(" | reply: ") + String(reply));
    // BK_DEBUG_PRINTLN(F("compare result: ") + String((prog_char_strncmp(m_replybuffer, (prog_char *)reply, len))));

    if ((prog_char_strncmp(m_replybuffer, (prog_char *)reply, len) == 0))
    {
        return true;
    }
    else if ((prog_char_strncmp(m_replybuffer, (prog_char *)F("RDY"), 3) == 0))
    {// Only AT Command received through serial port after SIM7xxx Series is powered on and Unsolicited Result
     // Code "RDY" is received from serial port.
     // Problems with sim7000 PCB power, to low or unstabiel?

        // Send 'AT' command to modem, to check communication.
        if (!TestAT(2000))
        { // no replay from modem. (could be PCB power down, wrong Buadrate, sim7000 firmware in dead loop, ....)
            return false;
        }

        delay(500);

        // resend command.
        if (!getReply(send, timeout))
        {
            return false;
        }

        return (prog_char_strncmp(m_replybuffer, (prog_char *)reply, len) == 0);
    }

    return false;
}

boolean BK_modem::sendCheckReply(const char *send, FStringPtr reply, uint16_t timeout)
{
    if (!getReply(send, timeout))
    {
        return false;
    }

    size_t len = prog_char_strlen((prog_char *)reply);
    return (prog_char_strncmp(m_replybuffer, (prog_char *)reply, len) == 0);
}

// Send prefix, suffix, and newline.  Verify modem response matches reply parameter.
boolean BK_modem::sendCheckReply(FStringPtr prefix, char *suffix, FStringPtr reply, uint16_t timeout)
{
    getReply(prefix, suffix, timeout);

    size_t len = prog_char_strlen((prog_char *)reply);
    return (prog_char_strncmp(m_replybuffer, (prog_char *)reply, len) == 0);
}

// Send prefix, suffix, and newline.  Verify modem response matches reply parameter.
boolean BK_modem::sendCheckReply(FStringPtr prefix, int32_t suffix, FStringPtr reply, uint16_t timeout)
{
    getReply(prefix, suffix, timeout);

    size_t len = prog_char_strlen((prog_char *)reply);
    return (prog_char_strncmp(m_replybuffer, (prog_char *)reply, len) == 0);
}


// Send prefix, suffix, and newline.  Verify modem response matches reply or reply2 parameter.
// uint8_t BK_modem::sendCheckReply(FStringPtr prefix, int32_t suffix, FStringPtr reply, FStringPtr reply2, uint16_t timeout)
// {
//     getReply(prefix, suffix, timeout);
//
//     size_t len = prog_char_strlen((prog_char *)reply);
//
//     if( len > 0)
//     {
//           if(prog_char_strncmp(m_replybuffer, (prog_char *)reply, len) == 0)
//           {
//               return (1);
//          }
//     }
//
//     len = prog_char_strlen((prog_char *)reply2);
//     if( len > 0)
//     {
//         if( (prog_char_strncmp(m_replybuffer, (prog_char *)reply2, len) == 0))
//         {
//              return 2;
//         }
//     }
//
//     return 0;
// }

// Send prefix, suffix, suffix2, and newline.  Verify modem response matches reply parameter.
boolean BK_modem::sendCheckReply(FStringPtr prefix, int32_t suffix1, int32_t suffix2, FStringPtr reply, uint16_t timeout)
{
    getReply(prefix, suffix1, suffix2, timeout);

    size_t len = prog_char_strlen((prog_char *)reply);
    return (prog_char_strncmp(m_replybuffer, (prog_char *)reply, len) == 0);
}

// Send prefix, ", suffix, ", and newline.  Verify modem response matches reply parameter.
boolean BK_modem::sendCheckReplyQuoted(FStringPtr prefix, FStringPtr suffix, FStringPtr reply, uint16_t timeout)
{
    getReplyQuoted(prefix, suffix, timeout);

    size_t len = prog_char_strlen((prog_char *)reply);
    return (prog_char_strncmp(m_replybuffer, (prog_char *)reply, len) == 0);
}

/// @brief
/// @param toreply
/// @param v
/// @param divider
/// @param index
/// @return
boolean BK_modem::parseReply(FStringPtr toreply, uint16_t *v, char divider, uint8_t index)
{
    char *ptr = prog_char_strstr(m_replybuffer, (prog_char *)toreply); // get the m_replybuffer pointer to start toreply.

    if (ptr == 0)
    {
        return false;
    }

    ptr += prog_char_strlen((prog_char *)toreply); // pointer to end of toreply string.
    // BK_DEBUG_PRINTLN(p);

    for (uint8_t i = 0; i < index; i++)
    {
        // increment dividers
        ptr = strchr(ptr, divider);

        if (!ptr)
        {
            return false;
        }

        ptr++;
        // BK_DEBUG_PRINTLN(p);
    }

    *v = atoi(ptr);

    return true;
}

/// @brief
/// @param toreply
/// @param v
/// @param divider
/// @param index
/// @return
boolean BK_modem::parseReply(FStringPtr toreply, char *vdest, char divider, uint8_t index)
{
    uint8_t idx = 0;
    char *ptr = prog_char_strstr(m_replybuffer, (prog_char *)toreply);

    if (ptr == 0)
    {
        return false;
    }

    ptr += prog_char_strlen((prog_char *)toreply);

    for (idx = 0; idx < index; idx++)
    {
        // increment dividers
        ptr = strchr(ptr, divider);

        if (!ptr)
        {
            return false;
        }

        ptr++;
    }

    for (idx = 0; idx < strlen(ptr); idx++)
    {
        if (ptr[idx] == divider)
        {
            break;
        }

        vdest[idx] = ptr[idx];
    }

    vdest[idx] = '\0'; // set terminator char.

    return true;
}

// Parse a quoted string in the response fields and copy its value (without quotes)
// to the specified character array (v).  Only up to maxlen characters are copied
// into the result buffer, so make sure to pass a large enough buffer to handle the
// response.
boolean BK_modem::parseReplyQuoted(FStringPtr toreply, char *vptr, int maxlen, char divider, uint8_t index)
{
    uint8_t i = 0, j = 0;

    // Verify response starts with toreply.
    char *ptr = prog_char_strstr(m_replybuffer, (prog_char *)toreply);

    if (ptr == 0)
    {
        return false;
    }

    ptr += prog_char_strlen((prog_char *)toreply);

    // Find location of desired response field.
    for (i = 0; i < index; i++)
    {
        // increment dividers
        ptr = strchr(ptr, divider);
        if (!ptr)
        {
            return false;
        }

        ptr++;
    }

    // Copy characters from response field into result string.
    for (i = 0, j = 0; j < maxlen && i < strlen(ptr); ++i)
    {
        // Stop if a divier is found.
        if (ptr[i] == divider)
        {
            break;
        }
        // Skip any quotation marks.
        else if (ptr[i] == '"')
        {
            continue;
        }

        vptr[j++] = ptr[i];
    }

    // Add a null terminator if result string buffer was not filled.
    if (j < maxlen)
    {
        vptr[j] = '\0';
    }

    return true;
}

/// @brief
/// @param tosend
/// @param toreply
/// @param v
/// @param divider  : Scheidings token.
/// @param index
/// @param multiline
/// @return
boolean BK_modem::sendParseReply(FStringPtr tosend,
                                 FStringPtr toreply,
                                 uint16_t *value,
                                 char divider,
                                 uint8_t index,
                                 boolean multiline)
{
    getReply(tosend, uint16_t(BK_SIM7000_TIMEOUT_1500MS), multiline);

    if (!parseReply(toreply, value, divider, index))
    {
        return false;
    }

    readline();                 // eat 'OK'

    return true;
}

/// @brief
/// @param toreply
/// @param f
/// @param divider
/// @param index
/// @return
boolean BK_modem::parseReplyFloat(FStringPtr toreply,
                                  float *f, char divider, uint8_t index)
{
    char *ptr = prog_char_strstr(m_replybuffer, (prog_char *)toreply); // get the pointer to the voltage.

    if (ptr == 0)
    {
        return false;
    }

    ptr += prog_char_strlen((prog_char *)toreply);

    // BK_DEBUG_PRINTLN(p);

    for (uint8_t i = 0; i < index; i++)
    {
        // increment dividers
        ptr = strchr(ptr, divider);
        if (!ptr)
        {
            return false;
        }

        ptr++;

        // BK_DEBUG_PRINTLN(p);
    }

    *f = atof(ptr);

    return true;
}

/// @brief needed for CBC and others
/// @param tosend
/// @param toreply
/// @param f
/// @param divider
/// @param index
/// @return
boolean BK_modem::sendParseReplyFloat(FStringPtr tosend,
                                      FStringPtr toreply,
                                      float *f, char divider, uint8_t index)
{
    getReply(tosend);

    if (!parseReplyFloat(toreply, f, divider, index))
    {
        return false;
    }

    readline();                 // eat 'OK'

    return true;
}

/// @brief Final result code "+CME ERROR: <err>" indicates an error related to mobile equipment or network.
/// @return error code.
uint16_t BK_modem::getCME_ErrorCode()
{
    uint16_t error = -1;
    if(parseReply(F("+CME ERROR: "), &error))
    {
        return error;
    }

    return UINT16_MAX;
}

/// @brief
/// @param
/// @return
int16_t BK_modem::getNetworkMode(void)
{
    uint16_t mode;
    if (!sendParseReply(F("AT+CNMP?"), F("+CNMP:"), &mode))
    {
        return -1;
    }

    return mode;
}

/// @brief : 2 Automatic
///          13 GSM only
///          38 LTE only
///          51 GSM and LTE only:
/// @param
/// @return
String BK_modem::getNetworkModes(void)
{
    // Get the help string, not the setting value
    if (!sendCheckReply(F("AT+CNMP=?"), F("+CNMP:")))
    {
        return "";
    }

    String res = String(m_replybuffer);

    return res;
}

/// @brief
/// @param
/// @return
String BK_modem::getPreferredModes(void)
{
    // Get the help string, not the setting value
    if (!sendCheckReply(F("AT+CMNB=?"), F("+CMNB:")))
    {
        return "";
    }

    String res = String(m_replybuffer);
    return res.substring(7, res.length());
}

/// @brief
/// @param
/// @return
int16_t BK_modem::getPreferredMode(void)
{
    uint16_t mode;
    if (!sendParseReply(F("AT+CMNB?"), F("+CMNB:"), &mode))
    {
        return -1;
    }

    return mode;
}

// 2  - Automatic
// 13 - GSM only
// 38 - LTE only 
// 51 - GSM and LTE only
boolean BK_modem::setPreferredMode(uint8_t mode)
{
    return sendCheckReply(F("AT+CNMP="), mode, m_ok_reply);
}

// 1 - CAT-M
// 2 - NB-IoT
// 3 - CAT-M and NB-IoT
boolean BK_modem::setPreferredLTEMode(uint8_t mode)
{
    return sendCheckReply(F("AT+CMNB="), mode, m_ok_reply);
}

/// @brief  Get Local IP Address extend.
/// @param
/// @return <IP address> A string parameter which indicates the IP address assigned from GPRS.
String BK_modem::getGPRSIP(void)
{
    // if (!getReply(F("AT+CIFSR")))        // 2 commands: "CIFSR".
    // {
    //      return "13.0.0.13";
    // }

    // String res = String(m_replybuffer);
    // return res.substring(0, res.length()-4);

    if (!getReply(F("AT+CNACT?")))
    {
        return "0.0.0.0";
    }

    // Response:
    //      +CNACT: <status>,<ip_addr>   (+CNACT: 1,"10.36.213.2")
    String res = String(m_replybuffer);
    String tmpstr = res.substring(10, res.length());    // input: "10.36.213.2"
    return tmpstr.substring(1, tmpstr.length()-1);      // skip '"' chars
}

/// @brief Get current CAT-M or NB-IOT Band configuration
/// @return 
String BK_modem::getOperatingBand()
{
    if (!sendCheckReply(F("AT+CBANDCFG?"), F("+CBANDCFG:")))
    {
        return "";
    }

    // Response:
    //      +CBANDCFG: "CAT-M",1,2,3,4,5,8,12,13,14,18,19,20,25,26,27,28,66,85
    String res = String(m_replybuffer);

    readline();         // eat 'OK'

    String tmpstr = res.substring(10, res.length());    // input: (CAT-M,NB-IOT),(1,2,3,4,5,8,12,13,14,18,19,20,25,26,27,28,66,71,85)
    return tmpstr;                                      //tmpstr.substring(1, tmpstr.length()-1);      // skip '"' chars
}

/// @brief Useful for choosing a certain carrier only
///        For example, AT&T uses band 12 in the US for LTE CAT-M whereas Verizon uses band 13
///        Mode: "CAT-M" or "NB-IOT"
///        Band: The cellular EUTRAN band number.
/// @param mode => String type  
/// @param band 
/// @return 
boolean BK_modem::setOperatingBand(const char *mode, uint8_t band)
{
    char cmdBuff[24];
    sprintf(cmdBuff, "AT+CBANDCFG=\"%s\",%i", mode, band);

    return sendCheckReply(cmdBuff, m_ok_reply);
}

/// @brief
/// @param check_signal
/// @param timeout_ms
/// @return 0 = OK, 1 = NO LTE connection, 2 = No good RSSI signal
uint8_t BK_modem::waitForNetworkConnection(bool check_signal, uint32_t timeout_ms)
{
    if (check_signal)
    {
        uint8_t quality;
        getSignalQuality(&quality);

        if (quality == 99)
        {
            return 2;
        }
    }

    if (isNetworkConnected())
    {
        return 0;
    }

    // for (uint32_t start = millis(); millis() - start < timeout_ms;)
    // {
    //     if (check_signal)
    //     {
    //         getSignalQuality();
    //     }
    //     if (isNetworkConnected())
    //     {
    //         return true;
    //     }

    //     delay(250);
    // }

    return 1;
}

/// @brief
/// @return
boolean BK_modem::isNetworkConnected()
{
    for(int reply = 5; reply; reply--)
    {
        RegStatus epsStatus = (RegStatus)getNetworkStatus();
        if(epsStatus == REG_SEARCHING)
        {
            uint16_t registration = 0;
            parseReply(F("+CGREG: "), &registration);       // Read NetworkStatus Response.
            setNetworkStatus(2);
            delay(1000);
            setNetworkStatus(registration);
            delay(1000);

            continue;
        }

        return (epsStatus == REG_OK_HOME || epsStatus == REG_OK_ROAMING);
    }

    return false;
}

/********* SIM7000 MQTT FUNCTIONS  ************************************/
// Set MQTT parameters
// Parameter tags can be "URL", "CLEANSS", "USERNAME", "PASSWORD", "TOPIC" or "MESSAGE"
boolean BK_modem::MQTT_setParameter(const char *paramTag, const char *paramValue, uint16_t port)
{
    char cmdStr[255];

    if (port == 0)
    {
        sprintf(cmdStr, "AT+SMCONF=\"%s\",\"%s\"", paramTag, paramValue); // Quoted paramValue
    }
    else
    {
        //sprintf(cmdStr, "AT+SMCONF=\"%s\",\"%s\",\"%i\"", paramTag, paramValue, port);
        sprintf(cmdStr, "AT+SMCONF=\"%s\",\"%s\",%i", paramTag, paramValue, port);
    }

    if (!sendCheckReply(cmdStr, m_ok_reply))
    {
        return false;
    }

    // if (strcmp(paramTag, "CLIENTID") == 0 ||
    //      strcmp(paramTag, "URL") == 0 ||
    //      strcmp(paramTag, "TOPIC") == 0 ||
    //      strcmp(paramTag, "MESSAGE") == 0 )
    // {
    //   if (port == 0)
    //      sprintf(cmdStr, "AT+SMCONF=\"%s\",\"%s\"", paramTag, paramValue); // Quoted paramValue
    //   else
    //      sprintf(cmdStr, "AT+SMCONF=\"%s\",\"%s\",%i", paramTag, paramValue, port);
    //
    //   if (! sendCheckReply(cmdStr, m_ok_reply))
    //      return false;
    // }
    // else
    // {
    //   sprintf(cmdStr, "AT+SMCONF=\"%s\",%s", paramTag, paramValue); // Unquoted paramValue
    //   if (! sendCheckReply(cmdStr, m_ok_reply)) return false;
    // }

    return true;
}

// Set MQTT parameters
//  Parameter tags can be "CLIENTID", "KEEPTIME", "CLEANSS", "QOS" or "RETAIN"
//          // AT+SMCONF="KEEPTIME",60
boolean BK_modem::MQTT_setParameter(const char *paramTag, uint32_t paramValue)
{
    char cmdStr[255];

    sprintf(cmdStr, "AT+SMCONF=\"%s\",%i", paramTag, paramValue); // Quoted paramValue

    if (!sendCheckReply(cmdStr, m_ok_reply))
    {
        return false;
    }

    return true;
}

/// @brief : TODO: password hide.
/// @param
/// @return: MQTT Parameters.
String BK_modem::MQTT_getParameters(void)
{
    sendCheckReply(F("AT+SMCONF?"), m_ok_reply, 5000, true);

    // skip: leading: "+SMCONF: \r\n" and end: "\r\nOK\r\n"
    String res = String(m_replybuffer);
    return res.substring(9, (res.length() > 9 ? res.length() - 5 : res.length()));
    //return String(m_replybuffer+10);
}

/// @brief : Connect or Disconnect to MQTT Broker Server.
/// @param : timeout
/// @return : true = connected, false = not connected
boolean BK_modem::MQTT_connect( uint16_t timeout)
{
    return sendCheckReply(F("AT+SMCONN"), m_ok_reply, timeout);
}

/// @brief 
/// @param  
void BK_modem::MQTT_disconnect(void)
{
    sendCheckReply(F("AT+SMDISC"), m_ok_reply);
}

/// @brief : Query MQTT connection status
/// @param  
/// @return 
boolean BK_modem::MQTT_connectionStatus(void)
{
    if (!sendCheckReply(F("AT+SMSTATE?"), F("+SMSTATE: 1")))
    {
        return false;
    }

    return true;
}

// Subscribe to specified MQTT topic
// QoS can be from 0-2
boolean BK_modem::MQTT_subscribe(const char *topic, byte QoS)
{
    char cmdStr[127];
    sprintf(cmdStr, "AT+SMSUB=\"%s\",%i", topic, QoS);

    if (!sendCheckReply(cmdStr, m_ok_reply))
    {
        return false;
    }

    return true;
}

// Unsubscribe from specified MQTT topic
boolean BK_modem::MQTT_unsubscribe(const char *topic)
{
    char cmdStr[64];
    sprintf(cmdStr, "AT+SMUNSUB=\"%s\"", topic);

    if (!sendCheckReply(cmdStr, m_ok_reply))
    {
        return false;
    }

    return true;
}

// Publish to specified topic
// Message length can be from 0-512 bytes
// QoS can be from 0-2
// Server hold message flag can be 0 or 1
boolean BK_modem::MQTT_publish(const char *topic, const char *payload, uint16_t contentLength, byte QoS, byte retain)
{
    char cmdStr[127];
    sprintf(cmdStr, "AT+SMPUB=\"%s\",%i,%i,%i", topic, contentLength, QoS, retain);

    // send "command" and wait for "OK" and ">" char.
    getReply(cmdStr, 2000);

    // Wait for "> " to send payload message.
    if (strstr(m_replybuffer, ">") == NULL)
    {
        return false;
    }

    // Send payload message.
    if (!sendCheckReply(payload, m_ok_reply, 15000))
    {// Sending Error
        return false;
    }

    return true;
}

// Change MQTT data format to hex
// Enter "true" if you want hex, "false" if you don't
boolean BK_modem::MQTT_dataFormatHex(bool yesno)
{
    return sendCheckReply(F("AT+SMPUBHEX="), yesno, m_ok_reply);
}

//********************************  HTTP / HTTPS ****************************************/
/// @brief  call function:
            // .HTTP_addHeader("User-Agent", "SIM7000", 7);
            // .HTTP_addHeader("Cache-control", "no-cache", 8);
            // .HTTP_addHeader("Connection", "keep-alive", 10);
            // .HTTP_addHeader("Accept", "*/*, 3);
/// @param type
/// @param value
/// @param maxlen
/// @return
boolean BK_modem::HTTP_addHeader(const char *type, const char *value, uint16_t maxlen)
{
    // Use .HTTP_addHeader() as needed before using this function
    // .HTTP_connect() to connect to the server first
    char cmdStr[2 * maxlen + 17];

    sprintf(cmdStr, "AT+SHAHEAD=\"%s\",\"%s\"", type, value);

    if (!sendCheckReply(cmdStr, m_ok_reply, 10000))
    {
        return false;
    }

    return true;
}

/// @brief
/// @param body
/// @param bodylen
/// @return
boolean BK_modem::HTTP_addBody(const char *body, uint16_t bodylen)
{
    // Use .HTTP_addHeader() as needed before using this function
    // Then use .HTTP_connect() to connect to the server first
    char cmdBuff[150]; // Make sure this is large enough for URI

    sprintf(cmdBuff, "AT+SHBOD=%i,10000", bodylen);
    getReply(cmdBuff, 10000);

    if (strstr(m_replybuffer, ">") == NULL)
    {
        return false; // Wait for ">" to send message
    }

    sendCheckReply(body, m_ok_reply, 2000);

    return true;
}

/// @brief
/// @param key
/// @param value
/// @param maxlen
/// @return
boolean BK_modem::HTTP_addPara(const char *key, const char *value, uint16_t maxlen)
{
    char cmdStr[2 * maxlen + 16];

    sprintf(cmdStr, "AT+SHPARA=\"%s\",\"%s\"", key, value);

    if (!sendCheckReply(cmdStr, m_ok_reply, 10000))
    {
        return false;
    }

    return true;
}

/// @brief
/// @param server
/// @return
boolean BK_modem::HTTP_connect(const char *server)
{
    // Set up server URL
    char urlBuff[100];

    sendCheckReply(F("AT+SHDISC"), m_ok_reply, 10000); // Disconnect HTTP

    if (BK_SSL)
    {
        sendCheckReply(F("AT+CSSLCFG=\"sslversion\",1,3"), m_ok_reply);
        // Set HTTP SSL Configure
        sendCheckReply(F("AT+SHSSL=1,\"\""), m_ok_reply, 10000);
    }

    // URL format must "http://xxx.xx.xx" or "https://xxx.xx.xx:pppp"
    sprintf(urlBuff, "AT+SHCONF=\"URL\",\"%s\"", server);

    if (!sendCheckReply(urlBuff, m_ok_reply, 10000))
    {
        return false;
    }

    // Hold once request time. Unit is second. Default 60s. range: 30-1800
    sendCheckReply(F("AT+SHCONF=\"TIMEOUT\",90"), m_ok_reply, 10000);   

    // Set max HTTP body length
    sendCheckReply(F("AT+SHCONF=\"BODYLEN\",1024"), m_ok_reply, 10000); // Max 1024.

    // Set max HTTP header length
    sendCheckReply(F("AT+SHCONF=\"HEADERLEN\",350"), m_ok_reply, 10000); // Max 350.

    // HTTP(s) Connection
    sendCheckReply(F("AT+SHCONN"), m_ok_reply, 20000);

    // Get HTTP status: 1 = connected, 0 = Not connected.
    if (!sendCheckReply(F("AT+SHSTATE?"), F("+SHSTATE: 1")))
    {
        return false;
    }

    readline(); // Eat 'OK'

    // Clear HTTP header (HTTP header is appended.)
    if (!sendCheckReply(F("AT+SHCHEAD"), m_ok_reply, 10000))
    {
        return false;
    }

    return true;
}

/// @brief 
/// @param  
/// @return 
boolean BK_modem::GetHTTP_status(void)
{
    // Get HTTP status
    if (!sendCheckReply(F("AT+SHSTATE?"), F("+SHSTATE: 1")))
    {
        return false;
    }

    readline(); // Eat 'OK'

    return true;
}

/// @brief
/// @param URI
/// @return
boolean BK_modem::HTTP_GET(const char *URI)
{
    // Use .HTTP_addHeader() as needed before using this function
    // Then use .HTTP_connect() to connect to the server first
    char cmdBuff[150];

    sprintf(cmdBuff, "AT+SHREQ=\"%s\",1", URI);
    sendCheckReply(cmdBuff, m_ok_reply, 10000);

    // Parse response status and size
    // Example reply --> "+SHREQ: "GET",200,387"
    uint16_t status, datalen;
    readline(10000);

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    if (!parseReply(F("+SHREQ: \"GET\""), &status, ',', 1))
        return false;

    if (!parseReply(F("+SHREQ: \"GET\""), &datalen, ',', 2))
        return false;

    BK_DEBUG_PRINT(F("HTTP GET status: "));
    BK_DEBUG_PRINTLN(status);
    BK_DEBUG_PRINT(F("Data length: "));
    BK_DEBUG_PRINTLN(datalen);

    if (status != HTTP_CODE_OK)
        return false;

    // Read server response
    getReply(F("AT+SHREAD=0,"), datalen, 10000);

    readline();                 // Eat 'OK'

    readline();                 // read incomming HTTP messages

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer); // +SHREAD: <datalen>

    readline(10000);

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer); // Print out server reply

    sendCheckReply(F("AT+SHDISC"), m_ok_reply, 10000); // Disconnect HTTP

    return true;
}

/// @brief
/// @param URI
/// @param body
/// @param bodylen
/// @return
boolean BK_modem::HTTP_POST(const char *URI, const char *body, uint8_t bodylen, uint16_t *status, uint16_t *datalen)
{
    // Use .HTTP_addHeader() as needed before using this function
    // Then use .HTTP_connect() to connect to the server first
    char cmdBuff[150]; // Make sure this is large enough for URI
    uint8_t reply = 10;

    sprintf(cmdBuff, "AT+SHBOD=%i, 10000", bodylen);
    getReply(cmdBuff, (uint16_t)10000);

    while (reply--)
    {
        readline();

        if (strstr(m_replybuffer, ">") != NULL)
        {
            break; // Wait for ">" to send message.
        }

        delay(200);
    }

    sendCheckReply(body, m_ok_reply, (uint16_t)2000);         // Now send the JSON body

    memset(cmdBuff, 0, sizeof(cmdBuff));            // Clear URI char array
    sprintf(cmdBuff, "AT+SHREQ=\"%s\",3", URI);

    if (!sendCheckReply(cmdBuff, m_ok_reply, (uint16_t)10000))
    {
        return false;
    }

    // Parse response status and size
    // Example reply --> "+SHREQ: "POST",200,452"
    readline(10000);

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer);

    // AT+SHREQ="/post",3
    if (!parseReply(F("+SHREQ: \"POST\""), status, ',', 1))
        return false;

    if (!parseReply(F("+SHREQ: \"POST\""), datalen, ',', 2))
        return false;

    BK_DEBUG_PRINT(F("HTTP POST status: "));
    BK_DEBUG_PRINTLN(*status);
    BK_DEBUG_PRINT(F("Data length: "));
    BK_DEBUG_PRINTLN(*datalen);

    if (*status != HTTP_CODE_OK)
    {
        return false;
    }

    // Read server response
    getReply(F("AT+SHREAD=0,"), *datalen, 10000U);
    readline();

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer); // +SHREAD: <datalen>

    readline(10000);

    memset( m_responcebuffer, 0x00, *datalen);
    memcpy( m_responcebuffer, m_replybuffer, *datalen);

    BK_DEBUG_PRINT(F("\t<--- "));
    BK_DEBUG_PRINTLN(m_replybuffer); // Print out server reply

    sendCheckReply(F("AT+SHDISC"), m_ok_reply, (uint16_t)10000); // Disconnect HTTP

    return true;
}

#pragma endregion

/*-------------------------------- SIM7000E ---------------------------------------------------------------------*/
#pragma region "SIM7000E"
/*
  Destructor: clean-up all resource of this class.
*/
// BK_modem_7000::~BK_modem_7000()
// {
// }

/// @brief 
/// @return 
boolean BK_modem_7000::begin()
{
    BK_DEBUG_PRINTLN(F("BK_modem_7000::Begin() start..."));

    bool result = BK_modem::begin();

    if (result)
    {
        /* When "get local timestamp" function is enabled, the following URC may be
           reported if network sends the message to the MS to provide the MS with
           subscriber specific information.

           Enable Local Time Stamp for getting network time.
        */
        if (!enableNetworkTimeSync(false))
        {
            BK_DEBUG_PRINTLN(F("### Enable Local Time Stamp for getting network time faild. **"));
            // return false;
        }

        // Set the network status green LED blinking pattern while connected to a network.
        setNetLED(true, 2, 500, 3000);  // Connected to network: on/off, mode, timer_on, timer_off
        setNetLED(true, 3, 1000, 5000); // Data connection enabled: on/off, mode, timer_on, timer_off  (default: 64, 300)
    }

    BK_DEBUG_PRINTLN( F("BK_modem_7000::Begin() End...") + String(result ? F("OK") : F("FAILED")));

    return result;
}

/// @brief 
/// @return SIM-7000 PCB name.
String BK_modem_7000::Name()
{
    return String(F("SIM-7000G"));
}

/// @brief : override base stop() function.
void BK_modem_7000::stop()
{
    BK_DEBUG_PRINTLN(F("BK_modem_7000 => stop()"));
    base_stop();
}

// Bearer Settings for Applications Based on IP
//   Query the GPRS bearer context status
// cid = 1  => Bearer profile identifier:
// Returns bearer status
//                       -1 Command returned with an error
//                        0 Bearer is connecting
//                        1 Bearer is connected
//                        2 Bearer is closing
//                        3 Bearer is closed
//
int8_t BK_modem_7000::getBearerStatus(void)
{
    uint16_t state;

    if (!sendParseReply(F("AT+SAPBR=2,1"), F("+SAPBR: "), &state, ',', 1))
    {
        return -1;
    }

    return (int8_t)state;
}

/// @brief
/// @param onoff
/// @return
boolean BK_modem_7000::enableGPRS(boolean onoff)
{
    if (onoff)
    {
        // disconnect all sockets and close bearer and detach from GPRS Service.
        stop();
        delay(100);

        // set bearer profile! connection type GPRS
        if (!sendCheckReply(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""), m_ok_reply, (uint16_t)10000))
        {
            return false;
        }

        delay(200); // This seems to help the next line run.

        // set bearer profile access point name
        if (m_apn)
        {// Configure bearer profile 1
            // Send command AT+SAPBR=3,1,"APN","<apn value>" where <apn value> is the configured APN value.
            if (!sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"APN\","), m_apn, m_ok_reply, (uint16_t)10000))
            {
                return false;
            }

            // set username/password
            if (m_apnusername /*&& strlen_P((const char *)m_apnusername) > 0*/)
            {
                // Send command AT+SAPBR=3,1,"USER","<user>" where <user> is the configured APN username.
                if (!sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"USER\","), m_apnusername, m_ok_reply, (uint16_t)10000))
                {
                    return false;
                }
            }

            if (m_apnpassword /*&& strlen_P((const char *)m_apnpassword) > 0*/)
            {
                // Send command AT+SAPBR=3,1,"PWD","<password>" where <password> is the configured APN password.
                if (!sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"PWD\","), m_apnpassword, m_ok_reply, (uint16_t)10000))
                {
                    return false;
                }
            }

            flushInput();

            // Start the TCP application toolkit task and set APN, USER NAME, PASSWORD
            SerialSIM->print(F("AT+CSTT=\""));
            SerialSIM->print(m_apn);

            if (m_apnusername)
            {
                SerialSIM->print(F("\",\""));
                SerialSIM->print(m_apnusername);
            }

            if (m_apnpassword)
            {
                SerialSIM->print(F("\",\""));
                SerialSIM->print(m_apnpassword);
            }

            SerialSIM->println(F("\"")); // set end char.

#ifdef BK_MODEM_DEBUG
            BK_DEBUG_PRINT(F("\tS5---> "));
            BK_DEBUG_PRINT(F("AT+CSTT=\""));
            BK_DEBUG_PRINT(m_apn);

            if (m_apnusername)
            {
                BK_DEBUG_PRINT(F("\",\""));
                BK_DEBUG_PRINT(m_apnusername);
            }

            if (m_apnpassword)
            {
                BK_DEBUG_PRINT(F("\",\""));
                BK_DEBUG_PRINT(m_apnpassword);
            }

            BK_DEBUG_PRINTLN(F("\""));
#endif

            if (!expectReply(m_ok_reply))
            {
                return false;
            }
        }

        // Attach to GPRS service.
        if (!sendCheckReply(F("AT+CGATT=1"), m_ok_reply, (uint16_t)75000))
        {
            return false;
        }

        // Query the GPRS bearer context status
        int8_t status = getBearerStatus();
        BK_DEBUG_PRINTLN(F("Bearer Status: ") + String(status));

        // Open the definied GPRS bearer context.
        if (status > 1 && !sendCheckReply(F("AT+SAPBR=1,1"), m_ok_reply, uint16_t(85000)))
        {
            return false;
        }

        // Query the GPRS bearer context status
        status = getBearerStatus();
        // if(status != 1)
        // {
        //      return false;
        // }

        // Bring up the TCP application toolkit wireless connection with GPRS or CSD.
        if (!sendCheckReply(F("AT+CIICR"), m_ok_reply, (uint16_t)10000))
        {
            return false;
        }

        if( !wirelessConnStatus())
        {
            openWirelessConnection(true);
        }
    }
    else
    {
        // disconnect all sockets.
        stop();
        delay(100);

        // close bearer
        if (!sendCheckReply(F("AT+SAPBR=0,1"), m_ok_reply, (uint16_t)10000))
        {
            if( getCME_ErrorCode() != 3)
            {
                return false;
            }
        }

        delay(200);
 
        //Detach from GPRS Service
        if (!sendCheckReply(F("AT+CGATT=0"), m_ok_reply, (uint16_t)10000))
        {
            return false;
        }

        // Close wireless data connection.
        //openWirelessConnection(false);
    }

    return true;
}

/*
  Open or close wireless data connection
  PDP => Packet Data Protocol
  
  SIM7000
        AT+CNACT=<mode>,<action>
            <action> "apn"

 @brief 
 @param onoff 
 @return 
*/
boolean BK_modem_7000::openWirelessConnection(bool onoff)
{
    if (!onoff)
    { // Disconnect wireless.
        char buff[strlen_P((prog_char *)m_apn) + 16];
        sprintf(buff, "AT+CNACT=0,\"%s\"", (prog_char *)m_apn);

        if (!sendCheckReply(buff, m_ok_reply, (uint16_t)10000))
        {
            return false;
        }

        readline(); // wait for reponse message "DEACTIVE".

        BK_DEBUG_PRINT(F("\rM1<--- "));
        BK_DEBUG_PRINTLN(m_replybuffer);

        if (strstr(m_replybuffer, "PDP: DEACTIVE") == NULL) // +APP PDP: DEACTIVE
        {
            if (strstr(m_replybuffer, ",DEACTIVE") == NULL) // +APP PDP: <pdpidx>,DEACTIVE
            {
                return false;
            }
        }
    }
    else
    { // Connect wireless.

        char buff[strlen_P((prog_char *)m_apn) + 16];
        sprintf(buff, "AT+CNACT=1,\"%s\"", (prog_char *)m_apn);

        if (!sendCheckReply(buff, m_ok_reply, 10000)) // if (!sendCheckReply(F("AT+CNACT=0,1"), m_ok_reply))
        {
            return false;
        }

        readline(5000); // wait for reponse message "PDP: ACTIVE".

        BK_DEBUG_PRINT(F("\tM1<--- "));
        BK_DEBUG_PRINTLN(m_replybuffer);

        if (strstr(m_replybuffer, "PDP: ACTIVE") == NULL) // +APP PDP: ACTIVE
        {
            if (strstr(m_replybuffer, ",ACTIVE") == NULL) // +APP PDP: <pdpidx>,ACTIVE
            {
                return false;
            }
        }
    }

    return true;
}

/// @brief 
/// @param onoff 
/// @param ntpserver 
/// @param timeZone 
/// @return 
boolean BK_modem_7000::enableNTPTimeSync(boolean onoff, FStringPtr ntpserver, uint16 timeZone)
{
    return BK_modem::enableNTPTimeSync( onoff,  ntpserver,  timeZone, 7000);
}

#pragma endregion

/*-------------------------------- SIM7080E --------------------------------------------------------------------*/
#pragma region "Skip_SIM7080E"
#if Skip_SIM7080
// 20-11-2025, Replaced for SIM7600.

/// @brief : base constructor
BK_modem_7080::BK_modem_7080() : BK_modem()
{
    this->mux = 0;
    this->pdpidx = 0;
}

/// @brief : overload constructor: call first base contructor.
/// @param mux => open connections that can run in the same time.
BK_modem_7080::BK_modem_7080(uint8_t mux) : BK_modem_7080()
{
    this->mux = (mux < 0 || mux > GSM_MUX_COUNT) ? 0 : mux;
}

/*
  Destructor: clean-up all resource of this class and base class.
  destructors are called automatically in the reverse order of construction. 
  (Base classes last).
*/
BK_modem_7080::~BK_modem_7080()
{
    // Call base destrcutor
    //BK_modem::~BK_modem()
}

/// @brief 
/// @return : SIM-7080 PCB name
String BK_modem_7080::Name()
{
    return String(F("SIM-7080G"));
}

//**************************************************************************************************************

/// @brief : 
/// @param maxWaitMs
void BK_modem_7080::stop(uint32_t maxWaitMs)
{
    BK_DEBUG_PRINTLN(F("BK_modem_7080 => stop()"));

    //sendCheckReply(F("AT+CACLOSE="), mux, m_ok_reply, maxWaitMs); //Close a TCP/UDP Connection
    char buff[16];
    sprintf(buff, "AT+CNACT=%d,0", pdpidx);            // AT+CNACT=<pdpidx>,<action>
    sendCheckReply(buff, m_ok_reply, maxWaitMs);       //Close APP Network Active.
}

/// @brief : override base stop() function.
void BK_modem_7080::stop()
{
    stop(5000L);
}

/// @brief : powers down the SIM7000 module.
/// @param  
void BK_modem_7080::AT_powerDown(void)
{
    // disconnect all sockets and close bearer and detach from GPRS Service.
    stop(9000L);
}

// APP Network Active context status
// Format of response: +CNACT: <pdpidx>,<statusx>,<addressx>
//
// Returns state
//
// int8_t BK_modem_7080::getBearerStatus(void)
// {
//     uint16_t state;

//     getReply(F("AT+CNACT?"));

//     if (!parseReply(F("+CNACT:"), &state, ',', 1))      // get <statusx>
//     {
//         return 0;   // Deactived
//     }

//     if (state == 0)
//     {
//         char buff[16];
//         sprintf(buff, "AT+CNACT=%d,1", pdpidx);
//         if (!sendCheckReply(buff, m_ok_reply, (uint16_t)10000))
//         {
//             return 0;
//         }

//         delay(200);

//         if (!parseReply(F("+CNACT:"), &state, ',', 1))
//         {
//             return 2;
//         }
//     }

//     return (int8_t)state;
// }

// Query wireless connection status
// Format of response: +CNACT: <pdpidx>,<statusx>,<addressx>
boolean BK_modem_7080::wirelessConnStatus(void)
{
    getReply(F("AT+CNACT?"));

    uint16_t status;
    if (!parseReply(F("+CNACT:"), &status, ',', 1))
    {
        return false;
    }

    if (status != 1)
    {
        return false;
    }

    return true;
}

/// @brief 
/// @param  
/// @return : IP address
String BK_modem_7080::getGPRSIP(void)
{
    if (!getReply(F("AT+CNACT?")))
    {
        return "0.0.0.0";
    }

    // Format of response:
    // +CNACT: <pdpidx>,<status>,<ip_addr>   (+CNACT: 0,1,"10.36.213.1")
    String res = String(m_replybuffer);
    String tmpstr = res.substring(12, res.length());    // input: "10.36.213.1"
    return tmpstr.substring(1, tmpstr.length()-1);      // skip '"' chars
}

/// @brief
/// @param onoff
/// @return
boolean BK_modem_7080::enableGPRS(boolean onoff)
{
    if (onoff)
    {
        // disconnect all sockets and close bearer and detach from GPRS Service.
        // Shut down the general application TCP/IP connection
        // CNACT will close *all* open application TCP/UDP connections
        sendCheckReply(F("AT+CNACT=0,0"), m_ok_reply, (uint16_t)60000);

        // Detach from GPRS Service
        if (!sendCheckReply(F("AT+CGATT=0"), m_ok_reply, (uint16_t)60000))
        {
            return false;
        }

        // Define the PDP context
        char buff[strlen_P( (prog_char *)m_apn) + 16];
        sprintf(buff, "AT+CGDCONT=1,\"IP\",\"%s\"", (prog_char *)m_apn);

        if (!sendCheckReply(buff, m_ok_reply, (uint16_t)10000))
        {
            return false;
        }

        // Attach to GPRS Service
        if (!sendCheckReply(F("AT+CGATT=1"), m_ok_reply, (uint16_t)60000))
        {
            return false;
        }

        // test
        GPRSstate();

        // NOTE:  **DO NOT** activate the PDP context
        // For who only knows what reason, doing so screws up the rest of the process

        // Check the APN returned by the server
        // Get Network APN in CAT-M or NB-IOT => "+CGNAPN: <valid>,<Network_APN>"
        if (!sendCheckReply(F("AT+CGNAPN"), m_ok_reply, (uint16_t)0000L, true))
        {
            // TO DO: Lof line.
            // return false;
        }

        // Bearer settings for applications based on IP
        // Set the user name and password
        // AT+CNCFG=<pdpidx>,<ip_type>,[<APN>,[<usename>,<password>,[<authentication>]]]
        // <pdpidx> PDP Context Identifier - for reasons not understood by me,
        //          use PDP context identifier of 0 for what we defined as 1 above
        // <ip_type> 0: Dual PDN Stack
        //           1: Internet Protocol Version 4
        //           2: Internet Protocol Version 6
        // <authentication> 0: NONE
        //                  1: PAP
        //                  2: CHAP
        //                  3: PAP or CHAP
        flushInput();

        // Start the TCP application toolkit task and set APN, USER NAME, PASSWORD
        String sendBuffer;
        sendBuffer.reserve(512);

        // Set PDP Configure data:
        // -Set the APN
        sendBuffer = F("AT+CNCFG=0,1,\"") + String((prog_char *)m_apn);
        pdpidx = 0;

        if (m_apnusername)
        { // -Set the user name
            sendBuffer += F("\",\"") + String((prog_char *)m_apnusername);
        }

        if (m_apnpassword)
        { // -Set the password
            sendBuffer += F("\",\"") + String((prog_char *)m_apnpassword);
        }

        sendBuffer += (F("\"")); // set end char.

        if (!sendCheckReply(FPSTR(sendBuffer.c_str()), m_ok_reply, (uint16_t)60000))
        {
            // TODO: ??
        }

#ifdef BK_MODEM_DEBUG
        // BK_DEBUG_PRINT(F("\t---> "));
        // BK_DEBUG_PRINTLN(sendBuffer);

        // if (getReply(F("AT+CNCFG?")))
        // { // skip "+CNCFG: "
        //     BK_DEBUG_PRINTLN(F("PDP Configure: ") + String(m_replybuffer + 8));
        // }
#endif

        // Activate application network connection.
        //openWirelessConnection(true);
    }
    else
    {
        // disconnect all sockets.

        if (sendCheckReply(F("AT+CNACT=0,0"), m_ok_reply, (uint16_t)60000L))
        {
            delay(200);
        }

        // Detach from GPRS Service
        if (!sendCheckReply(F("AT+CGATT=0"), m_ok_reply, (uint16_t)10000))
        {
            return false;
        }
    }

    return true;
}

/* 
 Open or close wireless data connection
 PDP => Packet Data Protocol

 SIM7080:
        Activate application network connection
        AT+CNACT=<pdpidx>,<action>
            <pdpidx> PDP Context Identifier - for reasons not understood by me,
                    use PDP context identifier of '0' for what we defined as 1 above.
            <action> 0: Deactive
                     1: Active
                     2: Auto Active
*/
inline boolean BK_modem_7080::openWirelessConnection(bool onoff)
{
    if (onoff)
    { // Connect wireless.
        bool res = false;
        int ntries = 5;

        enableGPRS(true);

        while (!res && ntries > 0)
        {
            if ( !(res = sendCheckReply(F("AT+CNACT=0,1"), m_ok_reply)))
            {// No response.
                ntries--;
                delay(500);
                continue;
            }

            readline(60000L, true); // wait for reponse "PDP: ACTIVE".

            BK_DEBUG_PRINT(F("\tR83<--- "));
            BK_DEBUG_PRINTLN(m_replybuffer);

            if (strstr(m_replybuffer, "PDP: ACTIVE") == NULL) // +APP PDP: ACTIVE
            {
                if (strstr(m_replybuffer, ",ACTIVE") == NULL) // +APP PDP: <pdpidx>,ACTIVE
                {
                    res = false;
                    ntries--;
                }
            }
        }

        if( ntries == 0)
        {// to many retries.
            return false;
        }
    }
    else
    { // Disconnect wireless.
        // CNACT will close *all* open application connections.
        if (!sendCheckReply(F("AT+CNACT=0,0"), m_ok_reply, (uint16_t)60000))
        {
            return false;
        }

        readline(); // wait for reponse "DEACTIVE".

        BK_DEBUG_PRINT(F("\tR81<--- "));
        BK_DEBUG_PRINTLN(m_replybuffer);

        // if (strstr(m_replybuffer, "PDP: DEACTIVE") == NULL) // +APP PDP: DEACTIVE
        // {
        //     if (strstr(m_replybuffer, ",DEACTIVE") == NULL) // +APP PDP: <pdpidx>,DEACTIVE
        //     {
        //         return false;
        //     }
        // }

        if (!sendCheckReply(F("AT+CGATT=0"), m_ok_reply, (uint16_t)60000)) // Deactivate the bearer context
        {
            return false;
        }

        BK_DEBUG_PRINT(F("\tR82<--- "));
        BK_DEBUG_PRINTLN(m_replybuffer);
    }

    return true;
}

/// @brief 
/// @param onoff 
/// @param ntpserver 
/// @param timeZone 
/// @return 
boolean BK_modem_7080::enableNTPTimeSync(boolean onoff, FStringPtr ntpserver, uint16 timeZone)
{
    return BK_modem::enableNTPTimeSync( onoff,  ntpserver,  timeZone, pdpidx);
}
#endif
#pragma endregion

/*-------------------------------- SIM7600E --------------------------------------------------------------------*/
#pragma region "SIM7600E"

/// @brief base constructor
BK_modem_7600::BK_modem_7600() : BK_modem()
{
    this->mux = 0;
    this->pdpidx = 0;
    this->client_index = 0;             // identifies a client '0'
    this->setWillOnlyOnce = false;
}

/// @brief : overload constructor: call first base contructor.
/// @param mux => open connections that can run in the same time.
BK_modem_7600::BK_modem_7600(uint8_t mux) : BK_modem_7600()
{
    this->mux = (mux < 0 || mux > GSM_MUX_COUNT) ? 0 : mux;
}

/*
  Destructor: clean-up all resource of this class and base class.
  destructors are called automatically in the reverse order of construction. 
  (Base classes last).
*/
BK_modem_7600::~BK_modem_7600()
{
    // Call base destrcutor.
    //BK_modem::~BK_modem()
}

/// @brief 
/// @return 
boolean BK_modem_7600::begin()
{
    BK_DEBUG_PRINTLN(F("BK_modem_7600::Begin() start..."));

    bool result = BK_modem::begin();        //call base class.

    if(result)
    {
        // Disable time and time zone URC's.
        sendCheckReply(F("AT+CTZR="), 0, m_ok_reply, (uint16_t)10000);

        // Enable automatic time zome update.
        sendCheckReply(F("AT+CTZU="), 1, m_ok_reply, (uint16_t)10000);

        stop( BK_SIM7000_TIMEOUT_1500MS);
    }

    BK_DEBUG_PRINTLN(F("BK_modem_7600::Begin() End... Status: ") + String(result ? F("OK") : F("FAILED"))); 
    
    return result;
}

/// @brief 
/// @return : SIM-7600 PCB name
String BK_modem_7600::Name()
{
    return String(F("SIM-7600E-LTE"));
}

/// @brief 
/// @return : SIM7600E
SIMTYPE BK_modem_7600::Sim_Type()  
{ 
    return SIMTYPE::SIM7600E; 
}

//**************************************************************************************************************
/// @brief 
/// @param maxWaitMs 
/*protected*/void BK_modem_7600::stop(uint32_t maxWaitMs)
{
    getReply(F("AT+NETCLOSE"), (uint16_t)maxWaitMs);
    delay(20);
    getReply(F("AT+CNETSTOP"), (uint16_t)maxWaitMs, true);
}

/// @brief
/// @param onoff
/// @return
boolean BK_modem_7600::enableGPRS(boolean onoff)
{
    return true;
}

/// @brief 
/// @param onoff 
/// @return
inline boolean BK_modem_7600::openWirelessConnection(bool onoff)
{
    // // Enable full functionality
    BK_modem::setFunctionality(1);

    // Attach to network
    if (!sendCheckReply(F("AT+CGATT=1"), m_ok_reply, (uint16_t)10000))
    {
        return false;
    }

    if (!sendCheckReply(F("AT+CNETSTART"), F("+CNETSTART: 2"), (uint16_t)10000))
    {// Open network => TODO error afhandeling (Not +CNETSTART: 2 then wait and check again)
        // response = "OK"
        for (int ctr = 5; ctr > 0; ctr--)
        {
            if (wirelessConnStatus())
            {
                break;
            }

            delay(1000);

            if (ctr == 1)
            {
                BK_DEBUG_PRINT(F("** NETSTART TimeOut ERROR **"));
                return false;
            }
        }
    }

    getReply(F("AT+NETOPEN?"), (uint16_t)12000);
    if (strstr(m_replybuffer, "+NETOPEN: 1") == NULL)
    { // TCPIP service deactivated => network close
      // Start data connection.
      // Format of response: +CNETSTART: <net_stat>
        if (!sendCheckReply(F("AT+NETOPEN"), F("+NETOPEN"), (uint16_t)5000))
        {// response = "OK"
            uint16_t len = readline( (uint16_t)SIM7600_TCP_MAX_RESPONSE_TIME_MS); // wait for +NETOPEN: 0
            BK_DEBUG_PRINTLN(F("\tRx<--- ") + String(m_replybuffer));

            if (len == 0 || m_replybuffer[10] != '0')
            {
                BK_DEBUG_PRINTLN(F("** NETOPEN ERROR **"));
                return false;
            }
        }
    }

    return true;
}

/*
    Open network connection status
    +CNETSTART: <net_stat> 
                    0 – network close (deactivated)
                    1 – network is opening
                    2 – network open(activated)
                    3 – network is closing
*/
boolean BK_modem_7600::wirelessConnStatus(void)
{
    bool _okay = true;

    getReply(F("AT+CNETSTART?"), (uint16_t)12000);
    // Format of response: +CNETSTART: <net_stat>
    if ( strstr(m_replybuffer, "+CNETSTART: 2") == NULL )
    {
        _okay = false;
    }

    //readline();                 // eat "OK"

    return _okay;
}

/// @brief : override base stop() function.
void BK_modem_7600::stop()
{
    stop(5000L);
}

/********* GPS **********************************************************/
/// @brief 
/// @param onoff 
/// @return 
boolean BK_modem_7600::enableGPS(boolean onoff)
{
    uint16_t state;

    // First check if its already on or off.
    if (!sendParseReply(F("AT+CGPS?"), F("+CGPS:"), &state))
    {
        return false;
    }

    if (onoff && !state)
    {
        if (!sendCheckReply(F("AT+CGPS=1,1"), m_ok_reply, true))
        {
            return false;
        }
    }
    else if (!onoff && state)
    {
        if (!sendCheckReply(F("AT+CGPS=0,1"), m_ok_reply, true))
        {
            return false;
        }
    }

    return true;
}

/// @brief Get GNSS fixed position information, get current position related information.
/// @param arg
/// @param buffer
///               Format: +CGNSSINFO:[<mode>],[<GPS-SVs>],[<GLONASS-SVs>],[<BEIDOU-SVs>],
///                                  [<lat>],[<N/S>],[<log>],[<E/W>],[<date>],[<UTC-time>],[<alt>],
///                                  [<speed>],[<course>],[<PDOP>],[HDOP],[VDOP]
/// @param maxbuff
/// @return
uint8_t BK_modem_7600::getGPS_Navigation_Information(uint8_t arg, char *buffer, uint8_t maxbuff)
{
    getReply(F("AT+CGNSSINFO"));

    char *ptr = prog_char_strstr(m_replybuffer, (prog_char *)F("+CGNSSINFO:"));

    if (ptr == 0)
    {
        buffer[0] = 0;
        return 0;
    }

    ptr += 12;

    uint8_t len = max(maxbuff - 1, (int)strlen(ptr));
    strncpy(buffer, ptr, len);
    buffer[len] = 0;

    readline();                     // eat 'OK'

    BK_DEBUG_PRINTLN(F("\tGPS-Buffer: ") + String(buffer) + F("\n\tBuffer lenght: ") + String(len));

    return len;
}

/// @brief 
/// @param lat 
/// @param lon 
/// @param speed_kph 
/// @param heading 
/// @param altitude 
/// @param year 
/// @param month 
/// @param day 
/// @param hour 
/// @param min 
/// @param sec 
/// @return 
boolean BK_modem_7600::getGPS(float *latitude, float *longitude, 
                              float *speed_kph, float *heading, float *altitude,
                              uint16_t *year, uint8_t *month, uint8_t *day, 
                              uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    char n_s = 'N';
    char e_w = 'E';
    char *ptrtoken;             // token pointer.
    char gpsbuffer[150];

    // grab the mode 2^5 gps csv from the sim808
    uint8_t res_len = getGPS_Navigation_Information(32, gpsbuffer, sizeof(gpsbuffer));

    //BK_DEBUG_PRINTLN(F("getGP receive GPS data buffer size: ") + String(res_len));

    // make sure we have a response
    if (res_len == 0)
    {
        return false;
    }

    // skip mode
    ptrtoken = strtok(gpsbuffer, ",");      // set strtok pointer to gpsbuffer[0].
    if (!ptrtoken)
    {
        return false;
    }

    //BK_DEBUG_PRINTLN(F("ptrtoken_1: ") + String(ptrtoken));

    // skip GPS satellite valid numbers
    ptrtoken = strtok(NULL, ",");
    if (!ptrtoken)
    {
        return false;
    }

    // skip GLONASS satellite valid numbers
    ptrtoken = strtok(NULL, ",");
    if (!ptrtoken)
    {
        return false;
    }

    // skip BEIDOU satellite valid numbers
    ptrtoken = strtok(NULL, ",");
    if (!ptrtoken)
    {
        return false;
    }

    // grab Latitude of current position.
    char *ptrLatitude = strtok(NULL, ",");
    if (!ptrLatitude)
    {
        return false;
    }

    // N/S Indicator, N=north or S=south
    ptrtoken = strtok(NULL, ",");
    if (!ptrtoken)
    {
        return false;
    }
    n_s = ptrtoken[0];

    // grab Longitude of current position.
    char *ptrLongitude = strtok(NULL, ",");
    if (!ptrLongitude)
    {
        return false;
    }

    // E/W Indicator, E=east or W=west
    ptrtoken = strtok(NULL, ",");
    if (!ptrtoken)
    {
        return false;
    }
    e_w = ptrtoken[0];

    //BK_DEBUG_PRINTLN(F("ptrtoken_8: ") + String(ptrtoken));

    // only grab date and time if needed
    if ((year != NULL) && (month != NULL) && (day != NULL) && (hour != NULL) && (min != NULL) && (sec != NULL))
    {
        char *dateTime = strtok(NULL, ",");
        if (!dateTime)
        {
            return false;
        }

        //BK_DEBUG_PRINTLN(F("Date: ") + String(dateTime));   

        // Date value : 121225
        // Year
        char *ptr = dateTime + 4;
        *year = 2000 + atoi(ptr);       // add eeuw.

        // Month
        ptr[0] = 0;
        ptr = dateTime + 2;
        *month = atoi(ptr);

        // Day
        ptr[0] = 0;
        ptr = dateTime;
        *day = atoi(ptr);

        // Time value : 191237.0
        dateTime = strtok(NULL, ",");
        if (!dateTime)
        {
            return false;
        }

        //BK_DEBUG_PRINTLN(F("Time: ") + String(dateTime));

        // Seconds
        ptr = dateTime + 4;
        ptr[2] = 0;
        *sec = atof(ptr);

        // Minutes
        ptr[0] = 0;
        ptr = dateTime + 2;
        *min = atoi(ptr);

        // Hours
        ptr[0] = 0;
        ptr = dateTime;
        *hour = atoi(ptr);

        //BK_DEBUG_PRINTLN(F("*** END DateTime set ****")); 
    }
    else
    {
        // skip date
        ptrtoken = strtok(NULL, ",");
        if (!ptrtoken)
        {
            return false;
        }
    }

    // Convert latitude and longitude from degrees (DMM) to decimal (DD).
    double dblatatitude  = atof(ptrLatitude);
    double dblonongitude = atof(ptrLongitude);
    /*
        Formule: convert GPS coordination format DMM to DD:
            ex:  5207.297464,N
                                52 + (07.297464 / 60) = 52 + 0.121624 = 52.121624
    */
    *latitude  = ((((uint)dblatatitude)  / 100) + (fmod(dblatatitude, 100)  / 60)) * (n_s == 'N' ? 1 : -1);
    *longitude = ((((uint)dblonongitude) / 100) + (fmod(dblonongitude, 100) / 60)) * (e_w == 'E' ? 1 : -1);

    // only grab altitude if needed
    if (altitude != NULL)
    {
        // grab altitude
        char *altp = strtok(NULL, ",");
        if (!altp)
        {
            return false;
        }

        *altitude = atof(altp);
    }

    // only grab speed if needed
    if (speed_kph != NULL)
    {
        // grab the speed in km/h
        char *speedp = strtok(NULL, ",");
        if (!speedp)
        {
            return false;
        }

        *speed_kph = atof(speedp);
    }

    // only grab Course. Degrees. if needed
    if (heading != NULL)
    {

        // grab the speed in knots
        char *coursep = strtok(NULL, ",");
        if (!coursep)
        {
            return false;
        }

        *heading = atof(coursep);
    }

    (void)ptrtoken;
    
    //BK_DEBUG_PRINTLN(F("**** getGPS END ****"));

    return true;
}

/// @brief 
/// @param voltage_mv =
/// @return 
void BK_modem_7600::getGPSAntennaVoltage(uint16_t * voltage_mv)
{
    *voltage_mv = 0;
    sendParseReply(F("AT+CVAUXV?"), F("+CVAUXV:"), voltage_mv, ',', 0, true);   // include wait for "OK"
    //readline();             // eat OK
}
/********* END-GPS **********************************************************/

/// @brief 
/// @param onoff 
/// @param ntpserver 
/// @param timeZone 
/// @return 
boolean BK_modem_7600::enableNTPTimeSync(boolean onoff, FStringPtr ntpserver, uint16 timeZone)
{
     char _sendbuffer[64] = {0};

    // Set NTP server and timezone
    sprintf(_sendbuffer, PSTR("AT+CNTP=\"%s\",%0d"), ntpserver, timeZone * 4); // time zone = 0 (time zone value multiplay by 4.)
    sendCheckReply(_sendbuffer, m_ok_reply, (uint16_t)BK_SIM7000_TIMEOUT_1500MS);

    // Sync time command.
    sendCheckReply(F("AT+CNTP"), m_ok_reply, (uint16_t)BK_SIM7000_TIMEOUT_1500MS*2);

    // Wait for sync result.
    //delay(3000);
    sendCheckReply(F("AT+CNTP?"), F("+CNTP:"), (uint16_t)BK_SIM7000_TIMEOUT_1500MS * 3);

    return true;
}

/// @brief Get IP address of LTE module
/// @param  
/// @return IP address
String BK_modem_7600::getGPRSIP(void)
{
    if (!getReply(F("AT+CNETIPADDR?"))) 
    {   // or "AT+IPADDR?" Get IP address of PDP context
        if (!getReply(F("AT+IPADDR?")))
        {
            return "0.0.0.0";
        }
    }

    // Response:
    //          +CNETIPADDR: <ip_address> || +IPADDR: <ip_address>
    String res = String(m_replybuffer);
    int len = res.indexOf(':');
    String tmpstr = res.substring(len+1, res.length());    // input: "10.36.213.2"

    readline();                                             // eat OK

    return tmpstr.substring(1, tmpstr.length());            // skip '"' chars
}

/// @brief : "CMQTTSTART" is used to start MQTT service by activating PDP context.
/// @param mqtt_server 
/// @param mqtt_port 
/// @param mqtt_user 
/// @param mqtt_psw 
/// @param keepAlive 
/// @param onoff    : on = connect, off = disconnect
/// @param timeout 
/// @return
boolean BK_modem_7600::MQTT_connect(const char *mqtt_server, uint mqtt_port,
                                    const char *mqtt_user, const char *mqtt_psw,
                                    uint16_t keepAlive, uint16_t timeout)
{
    bool _result = false;

    for (int reply = 3; reply > 0; reply--)
    {
        sendCheckReply(F("AT+CMQTTSTART"), F("+CMQTTSTART:"), timeout, true);
        char *p = prog_char_strstr(m_replybuffer, PSTR("+CMQTTSTART:"));
        int error_code = atoi((p + 13));
        //Error code:
        //           00 - Okay
        //           23 – network is opened.
        if (p != 0 && (error_code == 0 || error_code == 23) )
        {
            _result = true;
            break;
        }

        if( error_code == 21)
        {//21 – client not release.
            MQTT_disconnect();                  // Stop MQTT server connection.
        }
        
        delay(100);
    }

    if (_result)
    {
        _result = false;
        char _sendbuffer[256] = {0};

        // Configure client id =>           <clientId>
        sprintf(_sendbuffer, PSTR("AT+CMQTTACCQ=%d,\"airRohr_001\",0,3"), client_index);     // Length of the topic string
        sendCheckReply( _sendbuffer, m_ok_reply, (uint16_t)BK_SIM7000_TIMEOUT_1500MS);

        //Set will topic and message. (F.F.U)
        //SetWillTopicAndMessage("Leusden_Sim7000/airRohr-7406283/LWT","Online");

        // Connect to MQTT server.
        //                         AT+CMQTTCONNECT=0,"tcp://fijnstofleusden.nl:48612",90,0,"volt","hYn16_7bESTmQt"
        sprintf(_sendbuffer, PSTR("AT+CMQTTCONNECT=%d,\"tcp://%s:%0d\",%d,0,\"%s\",\"%s\""), client_index, mqtt_server, mqtt_port, keepAlive, mqtt_user, mqtt_psw);
        if (sendCheckReply(_sendbuffer, m_ok_reply, (uint16_t)BK_SIM7000_TIMEOUT_1500MS))
        {
            readline( (uint16_t)BK_SIM7000_TIMEOUT_1500MS * 10);                  // wait on response msg.
            BK_DEBUG_PRINTLN(F("\tRx<--- ") + String(m_replybuffer));

            char *p = prog_char_strstr(m_replybuffer, PSTR("+CMQTTCONNECT:"));
            if (p != 0 && *(p + 17) == '0')
            {
                _result = true;
            }
        }
        else
        {
            // send LWT= Offline
            BK_DEBUG_PRINT(F("ERROR= "));
            BK_DEBUG_PRINTLN(m_replybuffer);
        }
    }
    else
    {
         BK_DEBUG_PRINTLN(F("CMQTTSTART Error:Rx<--- ") + String(m_replybuffer));
    }

    return _result;
}

/// @brief
void BK_modem_7600::MQTT_disconnect()
{
    char _sendbuffer[128] = {0};

    sprintf(_sendbuffer, PSTR("AT+CMQTTDISC=%d,60"), client_index);               //
    sendCheckReply(_sendbuffer, m_ok_reply, (uint16_t)BK_SIM7000_TIMEOUT_1500MS); // Disconnect Server
    //readline((uint16_t)BK_SIM7000_TIMEOUT_1500MS * 10);                         // wait on response msg.
    //BK_DEBUG_PRINTLN(F("\tRx<--- ") + String(m_replybuffer));

    sprintf(_sendbuffer, PSTR("AT+CMQTTREL=%d"), client_index);
    sendCheckReply(_sendbuffer, m_ok_reply, (uint16_t)BK_SIM7000_TIMEOUT_1500MS); // Release Client

    // getReply(F("AT+CMQTTSTOP"));           // Stop MQTT 4G connection.
    sendCheckReply(F("AT+CMQTTSTOP"), m_ok_reply, (uint16_t)BK_SIM7000_TIMEOUT_1500MS);
}

/// @brief MQTT publish sensor data to Broker.
/// @param topic 
/// @param payload 
/// @return 0 = Ok
///         1 = not Ok
///         2 = socket is closed passively.
int8_t BK_modem_7600::MQTT_publish(const char *topic, const char *payload)
{
    char cmdStr[128] = {0};
    sprintf(cmdStr, PSTR("AT+CMQTTTOPIC=%d,%i"), client_index, strlen(topic));     // Length of the topic string
    getReply(cmdStr, (uint16_t)BK_SIM7000_DEFAULT_TIMEOUT_MS, true);               // send "command" and wait ">" char.

    // Wait for ">" to send topic message.
    if (strstr(m_replybuffer, ">") == NULL)
    {
        return 1;
    }

    if (!sendCheckReply(topic, m_ok_reply, (uint16_t)15000))
    {// Error: no connection.
        BK_DEBUG_PRINTLN(F("\tRx<--- ") + String(m_replybuffer));

        char *p1 = prog_char_strstr(m_replybuffer, PSTR("+CMQTTCONNLOST:"));
        char *p2 = prog_char_strstr(m_replybuffer, PSTR("+CMQTTTOPIC: 0,11"));
        if (p1 != 0  || p2 != 0)
        {// When client disconnect passively, URC “+CMQTTCONNLOST” will be reported, then user need to connect MQTT server again.
         // socket is closed passively => first close current MQTT connection.
            MQTT_disconnect();
            return 2;
        }

        return 1;
    }

    delay(20);

    sprintf(cmdStr, PSTR("AT+CMQTTPAYLOAD=%d,%i\r"), client_index, strlen(payload));    // Length of the payload
    getReply(cmdStr, (uint16_t)BK_SIM7000_DEFAULT_TIMEOUT_MS, true);                    // send "command" and wait for ">" char.

    // Wait for ">" to send payload message.
    if (strstr(m_replybuffer, ">") == NULL)
    {
        return 1;
    }

    // Send payload message.
    if (!sendCheckReply(payload, m_ok_reply, (uint16_t)15000U))
    {// Error: no connection.
        BK_DEBUG_PRINTLN(F("\tRx<--- ") + String(m_replybuffer));

        char *p = prog_char_strstr(m_replybuffer, PSTR("+CMQTTPAYLOAD: 0,11"));
        if (p != 0 )
        {// no connection, then user need to connect MQTT server again.
         // first close current MQTT connection.
            MQTT_disconnect();
            return 2;
        }

        return 1;
    }

    delay(20);

                                   //[client_index],<qos>,<pub_timeout>[, <ratained>
    sprintf(cmdStr, PSTR("AT+CMQTTPUB=%d,1,120"), client_index);                 //
    if (sendCheckReply(cmdStr, m_ok_reply, (uint16_t)BK_SIM7000_TIMEOUT_1500MS)) // Publish to the MQTT Broker server
    {
        readline((uint16_t)120500U);                                             // wait on response msg.
        BK_DEBUG_PRINTLN(F("\tRx<--- ") + String(m_replybuffer));

        char *p = prog_char_strstr(m_replybuffer, PSTR("+CMQTTPUB:"));
        if (p != 0 && *(p + 13) == '0')
        {
            return 0;
        }
    }

    return 1;
}

/// @brief : Send Will Topic and -Message only once to MQTT Broker.
/// @param willtopic 
/// @param willmessage 
/// @param qos => (default AtLeastOnce = 1)
/*protected*/void BK_modem_7600::SetWillTopicAndMessage(const char *willTopic, const char *willMessage, const uint8_t qos)
{
    if (!setWillOnlyOnce)
    {
        char cmdStr[128] = {0};
        sprintf(cmdStr, PSTR("AT+CMQTTWILLTOPIC=%u,%u"), client_index, strlen(willTopic)); // Length of the topic string
        getReply(cmdStr, (uint16_t)BK_SIM7000_DEFAULT_TIMEOUT_MS, true );                  // send "command" and wait for ">" char.

        // Wait for ">" to send topic message.
        if (strstr(m_replybuffer, ">") != NULL)
        {// Send data after '>' prompt 
            sendCheckReply(willTopic, m_ok_reply, (uint16_t)15000);
        }

        sprintf(cmdStr, PSTR("AT+CMQTTWILLMSG=%u,%u,%u"), client_index, strlen(willMessage), qos); // Length of the topic string
        getReply(cmdStr, (uint16_t)BK_SIM7000_DEFAULT_TIMEOUT_MS, true);                     // send "command" and wait for ">" char.

        // Wait for ">" to send topic message.
        if (strstr(m_replybuffer, ">") != NULL)
        {// Send data after '>' prompt 
            sendCheckReply(willMessage, m_ok_reply, (uint16_t)15000);
        }

        setWillOnlyOnce = true;
    }
}
#pragma endregion

#pragma GCC diagnostic pop
