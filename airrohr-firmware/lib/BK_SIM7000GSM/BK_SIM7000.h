// Adaptation Botletics hardware
// Original text below:

/************************************************************************************
  These displays use TTL Serial to communicate, 2 pins are required to interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ************************************************************************************/

#ifndef BK_SIM7000_MODEM_H
#define BK_SIM7000_MODEM_H

/*
 * BK_MODEM_DEBUG
 * Causes extensive debug output on the DebugStream;
 * set in the appropriate header.
 * 
 * Moved to platformio.ini file: [common] build_flags -D BK_MODEM_DEBUG
 */
//#define BK_MODEM_DEBUG

#include "BK_Common.h"

// memory size of internet, at least 512
#define RECEIVE_BUFFER_LENGHT_MAX 512

#define GSMMODEM_BAUD 9600
#define SERIALSIM_BAUD 115200

// Set the preferred SMS storage.
//   Use "SM" for storage on the SIM.
//   Use "ME" for internal storage on the chip
#define MODEM_PREF_SMS_STORAGE "\"SM\""
// #define MODEM_PREF_SMS_STORAGE "\"ME\""

#define HEADSETAUDIO 0
#define EXTAUDIO 1

#define STTONE_DIALTONE 1
#define STTONE_BUSY 2
#define STTONE_CONGESTION 3
#define STTONE_PATHACK 4
#define STTONE_DROPPED 5
#define STTONE_ERROR 6
#define STTONE_CALLWAIT 7
#define STTONE_RINGING 8
#define STTONE_BEEP 16
#define STTONE_POSTONE 17
#define STTONE_ERRTONE 18
#define STTONE_INDIANDIALTONE 19
#define STTONE_USADIALTONE 20

#define BK_SIM7000_DEFAULT_TIMEOUT_MS   500       // wait responce time: 500msec.
#define BK_SIM7000_TIMEOUT_1500MS      1500       // wait responce time: 1500msec.

#define _HTTP_GET 0
#define _HTTP_POST 1
#define _HTTP_HEAD 2

#define CALL_READY 0
#define CALL_FAILED 1
#define CALL_UNKNOWN 2
#define CALL_RINGING 3
#define CALL_INPROGRESS 4

// see ESP8266HTTPClient.H
#define HTTP_CODE_OK 200
#define HTTP_CODE_REQUEST_TIMEOUT    408

enum PinStatus
{
    SIM_ERROR = -2,
    SIM_UNKNOWN = -1,
    SIM_READY = 0,
    SIM_PIN = 1,
    SIM_PUK = 2,            // SIM_LOCKED
    SIM_PH_PIN = 3,
    SIM_PH_PUK = 4,
    SIM_PIN2 = 5,
    SIM_PUK2 = 6,
};

enum RegStatus {
  REG_NO_RESULT    = -1,
  REG_UNREGISTERED = 0,
  REG_SEARCHING    = 2,
  REG_DENIED       = 3,
  REG_OK_HOME      = 1,
  REG_OK_ROAMING   = 5,
  REG_UNKNOWN      = 4,
};

#ifndef LTE_GSM_YIELD
    #define LTE_GSM_YIELD_MS 0
    #define LTE_GSM_YIELD() \
    { delay(LTE_GSM_YIELD_MS); }      // #define TINY_GSM_YIELD_MS 0
#endif

#define RESERVE_STRING(name, size) String name((const char*)nullptr); name.reserve(size)

// SSL set
#define BK_SSL 0 // If http:
// #define BK_SSL 1        // If https:

/*

*/
class BK_modem : public BK_SIM7000_StreamType
{
public:
    BK_modem(void);
    ~BK_modem();

    void init(BK_SIM7000_StreamType &comPort, HardwareSerial &debugPort, uint8_t pin_pwr);
    boolean begin();
    boolean TestAT( void);

    // Stream functions (these are virtual functions() see Stream.h file)
    int available(void);
    size_t write(uint8_t x);
    int read(void);
    int peek(void);
    void flush();

    // requirements
    boolean setBaudrate(uint32_t baud);

    String getModemName();
    String getModemInfo();
    String getModemSoftware_Revision();
    String getOperator(void);
    String getManufacturer_Identification();
    String getSIMCOMATI(void);
    String getNTPgetserver(void);

    // Power, battery, and ADC
    void modemPowerOn();
    void modemPowerOff();
    void restartPowerOff();
    void modemRestart();
    

    boolean getADCVoltage(uint16_t *v);
    boolean getBattPercent(uint16_t *p);
    boolean getBattVoltage(uint16_t *v);

    // Functionality and operation mode settings
    boolean setFunctionality(uint8_t option);                                                           // AT+CFUN command
    boolean enableSleepMode(bool onoff);                                                                // AT+CSCLK command
    boolean set_eDRX(uint8_t mode, uint8_t connType, char *eDRX_val);                                   // AT+CEDRXS command
    boolean enablePSM(bool onoff);                                                                      // AT+CPSMS command
    boolean enablePSM(bool onoff, char *TAU_val, char *activeTime_val);                                 // AT+CPSMS command
    boolean getMobileEquipmentError(uint8_t chkcode);
    boolean setMobileEquipmentError(uint8_t code);

    boolean setNetLED(bool onoff, uint8_t mode = 0, uint16_t timer_on = 64, uint16_t timer_off = 3000); // AT+CNETLIGHT and AT+SLEDS commands

    // SIM query
    uint8_t unlockSIM(const char *pin);
    int8_t getPINStatus();
    uint8_t getSIMCCID(char *ccid);
    uint8_t getNetworkStatus(void);
    void getSignalQuality(uint8_t *quality, int8_t *rssi = nullptr);

    // IMEI
    uint8_t getIMEI(char *imei);

    // SMS handling
    boolean setSMSInterrupt(uint8_t i);
    uint8_t getSMSInterrupt(void);
    int8_t getNumSMS(void);
    boolean readSMS(uint8_t i, char *smsbuff, uint16_t max, uint16_t *readsize);
    boolean sendSMS(const char *smsaddr, const char *smsmsg);
    boolean deleteSMS(uint8_t i);
    boolean deleteAllSMS(void);
    boolean getSMSSender(uint8_t i, char *sender, int senderlen);
    boolean sendUSSD(char *ussdmsg, char *ussdbuff, uint16_t maxlen, uint16_t *readlen);

    // Time
    boolean enableNetworkTimeSync(boolean onoff);
    uint8_t getNTPstatus();
    boolean enableNTPTimeSync(boolean onoff, FStringPtr ntpserver = 0, uint16 timeZone = 0);
    boolean getTime(char *buff, uint16_t maxlen);

    // RTC
    boolean enableRTC(uint8_t i);
    boolean readRTC(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *hr, uint8_t *min, uint8_t *sec, int8_t *tz);

    // GPRS handling
    boolean enableGPRS(boolean onoff);
    int8_t GPRSstate(void);
    bool isGprsConnected();
    boolean getGSMLoc(uint16_t *replycode, char *buff, uint16_t maxlen);
    boolean getGSMLoc(float *lat, float *lon);
    void setNetworkSettings(FStringPtr apn, FStringPtr username = 0, FStringPtr password = 0);
    boolean postData(const char *request_type, const char *URL, const char *body = "", const char *token = "", uint32_t bodylen = 0);
    boolean postData(const char *server, uint16_t port, const char *connType, const char *URL, const char *body = "");
    int8_t getNetworkSystemMode(char *typeStringBuffer);

    int8_t getBearerStatus(void);
    boolean getIPv4(char *ipStringBuffer, size_t bufferLength);
    void getNetworkInfo(String &infoLine);
    bool getNetworkInfoLong(void);

    // Network connection (AT+CNACT)
    boolean openWirelessConnection(bool onoff);
    boolean wirelessConnStatus(void);

    // GPS handling
    boolean enableGPS()     { return enableGPS(true); }
    boolean disableGPS()    { return enableGPS(false); }

    int8_t GPSstatus(void);
    uint8_t getGPS_Navigation_Information(uint8_t arg, char *buffer, uint8_t maxbuff);
    boolean getGPS(float *lat, float *lon, float *speed_kph, float *heading, float *altitude,
                   uint16_t *year = NULL, uint8_t *month = NULL, uint8_t *day = NULL, 
                   uint8_t *hour = NULL,  uint8_t *min = NULL, uint8_t *sec = NULL);

    boolean enableGPSNMEA(uint8_t nmea);

    // UDP raw connections
    boolean UDPconnect(char *server, uint16_t port);

    // TCP raw connections
    boolean TCPconnect(char *server, uint16_t port);
    boolean TCPclose(void);
    boolean TCPconnected(void);
    boolean TCPsend(char *packet, uint8_t len);
    uint16_t TCPavailable(void);
    uint16_t TCPread(uint8_t *buff, uint8_t len);
    boolean TCPdns(char *hostname, char *buff, uint8_t len);
    boolean addRootCA(const char *root_cert);

    // MQTT
    boolean MQTTconnect(const char *protocol, const char *clientID, const char *username = "", const char *password = "");
    boolean MQTTdisconnect(void);
    boolean MQTTpublish(const char *topic, const char *message);
    boolean MQTTsubscribe(const char *topic, byte QoS);
    boolean MQTTunsubscribe(const char *topic);
    boolean MQTTreceive(const char *topic, const char *buf, int maxlen);

    // FTP
    boolean FTP_Connect(const char *serverIP, uint16_t port, const char *username, const char *password);
    boolean FTP_Quit();
    boolean FTP_Rename(const char *filePath, const char *oldName, const char *newName);
    boolean FTP_Delete(const char *fileName, const char *filePath);
    boolean FTP_MDTM(const char *fileName, const char *filePath, uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *minute, uint8_t *second);
    // boolean FTP_GET(const char* fileName, const char* filePath, uint16_t numBytes, char * replybuffer);
    const char *FTP_GET(const char *fileName, const char *filePath, uint16_t numBytes);
    boolean FTP_PUT(const char *fileName, const char *filePath, char *content, size_t numBytes);

    // HTTP low level interface.
    boolean HTTP_init();
    boolean HTTP_term();
    void HTTP_para_start(FStringPtr parameter, boolean quoted = true);
    boolean HTTP_para_end(boolean quoted = true);
    boolean HTTP_para(FStringPtr parameter, const char *value);
    boolean HTTP_para(FStringPtr parameter, FStringPtr value);
    boolean HTTP_para(FStringPtr parameter, int32_t value);
    boolean HTTP_data(uint32_t size, uint32_t maxTime = 10000);
    boolean HTTP_action(uint8_t method, uint16_t *status, uint16_t *datalen, int32_t timeout = 10000);
    boolean HTTP_readall(uint16_t *datalen);
    boolean HTTP_ssl(boolean onoff);

    // HTTP high level interface (easier to use, less flexible).
    boolean HTTP_GET_start(char *url, uint16_t *status, uint16_t *datalen);
    void HTTP_GET_end(void);
    boolean HTTP_POST_start(char *url, FStringPtr contenttype, 
                            const uint8_t *headerdata, uint16_t headerdatalen, 
                            const uint8_t *postdata, uint16_t postdatalen, 
                            uint16_t *status, uint16_t *datalen);
    void HTTP_POST_end(void);
    void setUserAgent(FStringPtr useragent);
    void setClientID(uint32_t clientID);

    // HTTPS
    void setHTTPSRedirect(boolean onoff);

    // PWM (buzzer)
    boolean setPWM(uint16_t period, uint8_t duty = 50);

    // Helper functions to verify responses.
    boolean expectReply(FStringPtr reply, uint16_t timeout = 10000);
    boolean sendCheckReply(const char *send, const char *reply, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS);
    boolean sendCheckReply(FStringPtr send, FStringPtr reply, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS);
    boolean sendCheckReply(const char *send, FStringPtr reply, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS);

protected:
    //int8_t m_rstpin;
    //uint8_t m_type;
    uint8_t m_https_SSL;
    uint8_t m_PIN_PWR;

    char m_replybuffer[RECEIVE_BUFFER_LENGHT_MAX];
    FStringPtr m_apn;
    FStringPtr m_apnusername;
    FStringPtr m_apnpassword;
    boolean m_httpsredirect;
    FStringPtr m_useragent;
    uint32_t m_clientID;
    FStringPtr m_ok_reply;

    boolean enableGPS(boolean onoff);
    uint16_t getCME_ErrorCode();

    // HTTP helpers
    boolean HTTP_setup(char *url);

    void flushInput();
    uint16_t readRaw(uint16_t cnt);
    uint16_t readRaw(uint8_t *rspbuffer, uint16_t cnt);
    uint16_t readline(uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS, boolean multiline = false);

    uint8_t getReply(const char *send, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS, boolean multiline = false);
    uint8_t getReply(FStringPtr send, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS, boolean multiline = false);
    uint8_t getReply(FStringPtr prefix, char *suffix, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS);
    uint8_t getReply(FStringPtr prefix, int32_t suffix, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS, boolean multiline = false);
    uint8_t getReply(FStringPtr prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout); // Don't set default value or else function call is ambiguous.
    uint8_t getReplyQuoted(FStringPtr prefix, FStringPtr suffix, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS);

    boolean sendCheckReply(FStringPtr prefix, char *suffix, FStringPtr reply, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS);
    boolean sendCheckReply(FStringPtr prefix, int32_t suffix, FStringPtr reply, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS);
    boolean sendCheckReply(FStringPtr prefix, int32_t suffix, int32_t suffix2, FStringPtr reply, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS);
    boolean sendCheckReplyQuoted(FStringPtr prefix, FStringPtr suffix, FStringPtr reply, uint16_t timeout = BK_SIM7000_DEFAULT_TIMEOUT_MS);

    void mqtt_connect_message(const char *protocol, byte *mqtt_message, const char *client_id, const char *username, const char *password);
    void mqtt_publish_message(byte *mqtt_message, const char *topic, const char *message);
    void mqtt_subscribe_message(byte *mqtt_message, const char *topic, byte QoS);
    void mqtt_disconnect_message(byte *mqtt_message);
    boolean mqtt_sendPacket(byte *packet, byte len);

    boolean parseReply(FStringPtr toreply, uint16_t *v, char divider = ',', uint8_t index = 0);
    boolean parseReplyFloat(FStringPtr toreply, float *f, char divider, uint8_t index);
    boolean parseReply(FStringPtr toreply, char *v, char divider = ',', uint8_t index = 0);
    boolean parseReplyQuoted(FStringPtr toreply, char *v, int maxlen, char divider, uint8_t index);

    boolean sendParseReply(FStringPtr tosend,
                           FStringPtr toreply,
                           uint16_t *v, 
                           char divider = ',', 
                           uint8_t index = 0, 
                           boolean multiline = false);

    boolean sendParseReplyFloat(FStringPtr tosend,
                                FStringPtr toreply,
                                float *f, 
                                char divider = ',', 
                                uint8_t index = 0);

    // Power, battery, and ADC
    void powerOn(uint8_t BK_PWRKEY);
    void powerDown(void);

    static boolean _incomingCall;
    static void onIncomingCall();

#ifdef BK_MODEM_DEBUG
// DebugStream sets the Stream output to use
// for debug (only applies when BK_MODEM_DEBUG is defined in platformIO.ini file)
    HardwareSerial *DebugStream;
#endif

    // create 'SerialSIM' instance pointer on Heap.
    // NodeMCU ESP8266 Serial port instance. (set baudrate, data lenght, ...)
    BK_SIM7000_StreamType   *SerialSIM;

};

/*
    BK Sim7000 modem LTE class.
*/
class BK_modem_LTE : public BK_modem
{

public:
    BK_modem_LTE();

    boolean isNetworkConnected();
    uint8_t waitForNetworkConnection(bool check_signal = false, uint32_t timeout_ms = 60000L);
    String  getNetworkModes(void);
    int16_t getNetworkMode(void);
    String  getPreferredModes(void);
    int16_t getPreferredMode(void);
    boolean setPreferredMode(uint8_t mode);
    boolean setPreferredLTEMode(uint8_t mode);
    boolean setOperatingBand(const char *mode, uint8_t band);
    boolean setBaudrate(uint32_t baud);
    boolean hangUp(void);
    String  getGPRSIP(void);

    // MQTT
    String  MQTT_getParameters(void);
    boolean MQTT_setParameter(const char *paramTag, const char *paramValue, uint16_t port = 0);
    boolean MQTT_setParameter(const char *paramTag, uint32_t paramValue);
    boolean MQTT_connect(bool yesno);
    boolean MQTT_connectionStatus(void);
    boolean MQTT_subscribe(const char *topic, byte QoS);
    boolean MQTT_unsubscribe(const char *topic);
    boolean MQTT_publish(const char *topic, const char *payload, uint16_t contentLength, byte QoS, byte retain);
    boolean MQTT_dataFormatHex(bool yesno);

    // HTTP
    boolean HTTP_connect(const char *server);
    boolean HTTP_addHeader(const char *type, const char *value, uint16_t maxlen); // max length of value
    boolean HTTP_addBody(const char *body, uint16_t bodylen);
    boolean HTTP_addPara(const char *key, const char *value, uint16_t maxlen);    // max length of value
    boolean HTTP_GET(const char *URI);
    boolean HTTP_POST(const char *URI, const char *body, uint8_t bodylen);
};

#endif // BK_SIM7000_MODEM_H
