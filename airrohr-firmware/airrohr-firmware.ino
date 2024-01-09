/************************************************************************
 *                                                                      *
 *  This source code needs to be compiled for the board                 *
 *  NodeMCU 1.0 (ESP-12E Module)                                        *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 *    airRohr firmware                                                  *
 *    Copyright (C) 2016-2021  Code for Stuttgart a.o.                  *
 *    Copyright (C) 2019-2020  Dirk Mueller                             *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 *                                                                      *
 ************************************************************************
 * OK LAB Particulate Matter Sensor                                     *
 *      - nodemcu-LoLin board                                           *
 *      - Nova SDS0111                                                  *
 *  http://inovafitness.com/en/Laser-PM2-5-Sensor-SDS011-35.html        *
 *                                                                      *
 * Wiring Instruction see included Readme.md                            *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 * Alternative                                                          *
 *      - nodemcu-LoLin board                                           *
 *                                                                      *
 * Wiring Instruction:                                                  *
 *      Pin 2 of dust sensor PM2.5 -> Digital 6 (PWM)                   *
 *      Pin 3 of dust sensor       -> +5V                               *
 *      Pin 4 of dust sensor PM1   -> Digital 3 (PMW)                   *
 *                                                                      *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 * Please check Readme.md for other sensors and hardware                *
 *                                                                      *
 * 2023-03-13: RD														*
 * SEN55: devided in SPS30 for PM, TS, NOx	(pin 1)						*
 * 					 SCD30 for temperature, humidity, CO2(NOx) (pin 17)	*
 * 			Change to													*
 * 					 SHT30 for temperature, humidity (pin 7)			*
 * 																		*
 * Remark: SEN5X sensor start/stop is enabled then Nox value = 0.		*
 * startUp time = 35 sec. then 9 times read PM/NC, Temp., Hum value.	*
 * Nox startUp time at least 60 sec. then read Nox value (F.F.U)		*
 * 																		*
 * Wifi signal MUST be strong.											*
 * 																		*
 * 																		*
 * 2023-08-12															*
 * Add MQTT	RD/FvD														*
 * 2023-09-17															*
 * Add Simm7000 Webservice												*
 * Add WiFiMulti used to connect to a WiFi network with strongest 		*
 * WiFi signal (RSSI). 													*
 * 																		*
 ************************************************************************
 * 																		*
 * latest build using lib 3.1.0											*
 * DATA:    [====      ]  41.5% (used 34000 bytes from 81920 bytes)		*
 * PROGRAM: [======    ]  58.0% (used 605529 bytes from 1044464 bytes)	*
 * 																		*
 * latest build using lib 3.1.0 / 2023-06-11							*
 * RAM:     [====      ]  44.7% (used 36648 bytes from 81920 bytes)		*
 * PROGRAM: [======    ]  60.5% (used 631589 bytes from 1044464 bytes)	*
 * 																		*
 * latest build using lib 3.1.0 / 2023-11-13							*
 * RAM:     [=====     ]  46.0% (used 37696 bytes from 81920 bytes)		*
 * PROGRAM: [======    ]  61.6% (used 643167 bytes from 1044464 bytes)	*
 * 																		*
 * latest build 2024-01-13												*
 * PLATFORM: Espressif 8266 (3.1.0) > NodeMCU 1.0 (ESP-12E Module)		*
 * RAM:     [=====     ]  47.0% (used 38488 bytes from 81920 bytes)		*
 * PROGRAM: [======    ]  62.3% (used 650991 bytes from 1044464 bytes)	*
 ************************************************************************/

// VS: Convert Arduino file to C++ manually.
#include <Arduino.h>

#include <WString.h>
#include <pgmspace.h>

// increment on change
#define SOFTWARE_VERSION_STR "FWL-2024-01-B1"
String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);

/*****************************************************************
 * Includes                                                      *
 *****************************************************************/
#if defined(ESP8266)
#include <FS.h> // must be first

#include <ESP8266HTTPClient.h>
//#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#include <Hash.h>
#include <ctime>
#include <coredecls.h>
#include <sntp.h>

/*********************************************/
/* MQTT header file
**********************************************/
#include <PubSubClient.h>

#endif

#if defined(ESP32)
#define FORMAT_SPIFFS_IF_FAILED true
#include <FS.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HardwareSerial.h>
#include <hwcrypto/sha.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#endif

#include "defines.h"

// includes common to ESP8266 and ESP32 (especially external libraries)
#include "./oledfont.h" 	// avoids including the default Arial font, needs to be included before SSD1306.h
#include <SSD1306.h>
#include <SH1106.h>
#include <LiquidCrystal_I2C.h>

#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#define ARDUINOJSON_DECODE_UNICODE 0
#include <ArduinoJson.h>
#include <DNSServer.h>
#include "./DHT.h"
#include <Adafruit_HTU21DF.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_SHT31.h>
#include <StreamString.h>
#include <DallasTemperature.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <SensirionI2CSen5x.h>
#include <TinyGPS++.h>					//  Arduino library for parsing NMEA data streams provided by GPS modules. 
#include <TinyGSM.h>

// local/modified header files.
#include "./bmx280_i2c.h"
#include "./sps30_i2c.h"
#include "./dnms_i2c.h"
#include "./intl.h"
#include "./utils.h"
//#include "defines.h"
#include "ext_def.h"
#include "html-content.h"
#include "./airrohr-cfg7000.h"
#include "./RCWL-0516.h"

// Temp language fields
#include "./intl_new.h"

/******************************************************************
 * The variables inside the cfg namespace are persistent          *
 * configuration values. They have defaults which can be          *
 * configured at compile-time via the ext_def.h file              *
 * They can be changed by the user via the web interface, the     *
 * changes are persisted to the flash and read back after reboot. *
 * Note that the names of these variables can't be easily changed *
 * as they are part of the json format used to persist the data.  *
 ******************************************************************/
namespace cfg
{
	unsigned debug = DEBUG;
	
	unsigned time_for_wifi_config = 600000;
	unsigned sending_intervall_ms = 145000;
	bool powersave;

	char current_lang[3];

	// credentials for basic auth of internal web server
	bool www_basicauth_enabled = WWW_BASICAUTH_ENABLED;
	char www_username[LEN_WWW_USERNAME];
	char www_password[LEN_CFG_PASSWORD];

	// wifi credentials
	char wlanssid[LEN_WLANSSID];
	char wlanpwd[LEN_CFG_PASSWORD];
	char wlanssid_2[LEN_WLANSSID];
	char wlanpwd_2[LEN_CFG_PASSWORD];
	char wlanssid_3[LEN_WLANSSID];
	char wlanpwd_3[LEN_CFG_PASSWORD];
	bool has_morewifi = HAS_MOREWIFI;

	char static_ip[LEN_STATIC_ADRESS];
	char static_subnet[LEN_STATIC_ADRESS];
	char static_gateway[LEN_STATIC_ADRESS];
	char static_dns[LEN_STATIC_ADRESS];

	// credentials of the sensor in Access Point (AP) mode.
	char fs_ssid[LEN_FS_SSID] = FS_SSID;
	char fs_pwd[LEN_CFG_PASSWORD] = FS_PWD;

	// (in)active sensors
	bool dht_read = DHT_READ;
	bool htu21d_read = HTU21D_READ;
	bool ppd_read = PPD_READ;
	bool sds_read = SDS_READ;
	bool pms_read = PMS_READ;
	bool hpm_read = HPM_READ;
	bool npm_read = NPM_READ;
	bool npm_fulltime = NPM_FULLTIME;
	bool ips_read = IPS_READ;
	bool sen5x_read = SEN5X_READ;
	bool sen5x_on = SEN5X_ON;
	char sen5x_sym_pm[LEN_SEN5X_SYM] = SEN5X_SYM_PM;
	char sen5x_sym_th[LEN_SEN5X_SYM] = SEN5X_SYM_TH;
	bool sps30_read = SPS30_READ;
	bool bmp_read = BMP_READ;
	bool bmx280_read = BMX280_READ;
	char height_above_sealevel[LEN_HEIGHT_ABOVE_SEALEVEL] = "0";
	bool sht3x_read = SHT3X_READ;
	bool scd30_read = SCD30_READ;
	bool ds18b20_read = DS18B20_READ;
	bool dnms_read = DNMS_READ;
	char dnms_correction[LEN_DNMS_CORRECTION] = DNMS_CORRECTION;
	bool gps_read = GPS_READ;
	char temp_correction[LEN_TEMP_CORRECTION] = TEMP_CORRECTION;

	char scd30_co2_correction[LEN_DNMS_CORRECTION] = CO2_CORRECTION;
	char scd30_temp_correction[LEN_TEMP_CORRECTION] = TEMP_CORRECTION;

	// send to "APIs"
	bool send2dusti = SEND2SENSORCOMMUNITY;
	bool send2madavi = SEND2MADAVI;
	bool send2sensemap = SEND2SENSEMAP;				// OpenSensemap
	bool send2fsapp = SEND2FSAPP;
	bool send2aircms = SEND2AIRCMS;
	bool send2custom = SEND2CUSTOM;
	bool send2influx = SEND2INFLUX;
	bool send2csv = SEND2CSV;
	bool send2mqtt = SEND2MQTT;

	bool auto_update = AUTO_UPDATE;
	bool use_beta = USE_BETA;

	// (in)active displays
	bool has_display = HAS_DISPLAY; // OLED with SSD1306 and I2C
	bool has_sh1106 = HAS_SH1106;
	bool has_flipped_display = HAS_FLIPPED_DISPLAY;
	bool has_lcd1602 = HAS_LCD1602;
	bool has_lcd1602_27 = HAS_LCD1602_27;
	bool has_lcd2004 = HAS_LCD2004;
	bool has_lcd2004_27 = HAS_LCD2004_27;

	bool display_wifi_info = DISPLAY_WIFI_INFO;
	bool display_device_info = DISPLAY_DEVICE_INFO;

	// API settings
	bool ssl_madavi = SSL_MADAVI;
	bool ssl_dusti = SSL_SENSORCOMMUNITY;
	char senseboxid[LEN_SENSEBOXID] = SENSEBOXID;

	char host_influx[LEN_HOST_INFLUX];
	char url_influx[LEN_URL_INFLUX];
	unsigned port_influx = PORT_INFLUX;
	char user_influx[LEN_USER_INFLUX] = USER_INFLUX;
	char pwd_influx[LEN_PASS_INFLUX] = PWD_INFLUX;
	char measurement_name_influx[LEN_MEASUREMENT_NAME_INFLUX];
	bool ssl_influx = SSL_INFLUX;
	bool has_fix_ip = HAS_FIX_IP;
	bool has_s7000 = HAS_S7000;

	char host_custom[LEN_HOST_CUSTOM];
	char url_custom[LEN_URL_CUSTOM];
	bool ssl_custom = SSL_CUSTOM;
	unsigned port_custom = PORT_CUSTOM;
	char user_custom[LEN_USER_CUSTOM] = USER_CUSTOM;
	char pwd_custom[LEN_CFG_PASSWORD] = PWD_CUSTOM;

	// Radar motion setting
	bool has_radarmotion = HAS_RADARMOTION;
	char host_radar[LEN_HOST_CUSTOM];
	unsigned  port_radar = PORT_RADAR;
	unsigned  motion_wait_time = 15;					// default wait 15 sec. before sent to MQTT broker.
	char user_radar[LEN_USER_CUSTOM] = USER_RADAR;
	char pwd_radar[LEN_CFG_PASSWORD] = PWD_RADAR;

#if defined(ESP8266)
	/*	MQTT  */
	char mqtt_server[LEN_HOST_CUSTOM];
	unsigned mqtt_port = MQTT_PORT;
	char mqtt_user[LEN_USER_INFLUX] = MQTT_USER;
	char mqtt_pwd[LEN_PASS_INFLUX] = MQTT_PWD;
	char mqtt_topic[LEN_MQTT_HEADER] = MQTT_TOPIC;

#endif

	// init: set default values to options.
	void initNonTrivials(const char *id)
	{
		strcpy(cfg::current_lang, CURRENT_LANG);
		strcpy_P(cfg::www_username, WWW_USERNAME);
		strcpy_P(cfg::www_password, WWW_PASSWORD);
		strcpy_P(cfg::wlanssid, WLANSSID);
		strcpy_P(cfg::wlanpwd, WLANPWD);
		strcpy_P(cfg::wlanssid_2, WLANPWD);
		strcpy_P(cfg::wlanpwd_2, WLANPWD);
		strcpy_P(cfg::wlanssid_3, WLANPWD);
		strcpy_P(cfg::wlanpwd_3, WLANPWD);
		strcpy_P(cfg::host_custom, HOST_CUSTOM);
		strcpy_P(cfg::url_custom, URL_CUSTOM);
		strcpy_P(cfg::host_influx, HOST_INFLUX);
		strcpy_P(cfg::url_influx, URL_INFLUX);
		strcpy_P(cfg::measurement_name_influx, MEASUREMENT_NAME_INFLUX);

		strcpy_P(cfg::mqtt_server, SERVER_MQTT);

		strcpy_P(cfg::static_ip, STATIC_IP);
		strcpy_P(cfg::static_subnet, STATIC_SUBNET);
		strcpy_P(cfg::static_gateway, STATIC_GATEWAY);
		strcpy_P(cfg::static_dns, STATIC_DNS);

		strcpy_P(cfg::host_radar, HOST_RADAR);

		if (!*cfg::fs_ssid)
		{
			strcpy(cfg::fs_ssid, SSID_BASENAME);
			strcat(cfg::fs_ssid, id);				// chipid
		}
	}

} // namespace cfg


//*************************************************************************************************************************************************

#define JSON_BUFFER_SIZE 3500					// 2300 -> 3500	=> increase: 11-11-2023
#define JSON_BUFFER_SIZE_SIMM7000 500			// Simm7000 module settings.

ESP8266WiFiMulti wifiMulti;

LoggerConfig loggerConfigs[LoggerCount];

long int sample_count = 0;
bool htu21d_init_failed = false;
bool bmp_init_failed = false;
bool bmx280_init_failed = false;
bool sht3x_init_failed = false;
bool scd30_init_failed = false;
bool dnms_init_failed = false;
bool gps_init_failed = false;
bool airrohr_selftest_failed = false;

#if defined(ESP8266)
ESP8266WebServer server(80);

// MQTT
#define MAX_MQTT_BUFFER_SIZE	512
const char mqtt_lwt[5] = MQTT_LWT;
//const char mqtt_lwt_message_off[10] = MQTT_LWT_MESSAGE_OFF;
//const char mqtt_lwt_message_on[10] = MQTT_LWT_MESSAGE_ON;

char mqtt_client_id[LEN_MQTT_HEADER] = MQTT_CLIENT_ID;
char mqtt_header[LEN_MQTT_LARGE_HEADER] = MQTT_TOPIC;
char mqtt_lwt_header[LEN_MQTT_LARGE_HEADER] = MQTT_TOPIC;

WiFiClient  mqtt_wifi;
PubSubClient mqtt_client(mqtt_wifi);
#endif

#if defined(ESP32)
WebServer server(80);
#endif

// default IPv4 address (ex. 192.168.4.1)
const uint8_t default_ip_first_octet = 192;
const uint8_t default_ip_second_octet = 168;
const uint8_t default_ip_third_octet = 4;
const uint8_t default_ip_fourth_octet = 1;

#include "./sen5x_html.h"
#include "./airrohr-cfg.h"


/*****************************************************************
 * Variables for Noise Measurement DNMS                          *
 *****************************************************************/
float last_value_dnms_laeq = -1.0;
float last_value_dnms_la_min = -1.0;
float last_value_dnms_la_max = -1.0;

/*****************************************************************
 * Display definitions                                           *
 *****************************************************************/
SSD1306 *oled_ssd1306 = nullptr;
SH1106 *oled_sh1106 = nullptr;
LiquidCrystal_I2C *lcd_1602 = nullptr;
LiquidCrystal_I2C *lcd_2004 = nullptr;
const uint8_t lcd_1602_default_i2c_address = 0x3f;
const uint8_t lcd_1602_alternate_i2c_address = 0x27;
const uint8_t lcd_1602_columns = 16;
const uint8_t lcd_1602_rows = 2;
const uint8_t lcd_2004_default_i2c_address = 0x3f;
const uint8_t lcd_2004_alternate_i2c_address =  0x27;
const uint8_t lcd_2004_columns = 20;
const uint8_t lcd_2004_rows = 4;

/*****************************************************************
 * Serial declarations                                           *
 *****************************************************************/
#if defined(ESP8266)
SoftwareSerial serialSDS;
SoftwareSerial *serialGPS;
SoftwareSerial serialNPM;
SoftwareSerial serialIPS;

SoftwareSerial serialSIM;
TinyGsm        LTEmodem(serialSIM);
//TinyGsmClient  LTEclient(LTEmodem);
#endif

#if defined(ESP32)
#define serialSDS (Serial1)
#define serialGPS (&(Serial2))
#define serialNPM (Serial1)
#define serialIPS (Serial1)
#endif

/*****************************************************************
 * DHT declaration                                               *
 *****************************************************************/
DHT dht(ONEWIRE_PIN, DHT_TYPE);

/*****************************************************************
 * HTU21D declaration                                            *
 *****************************************************************/
Adafruit_HTU21DF htu21d;

/*****************************************************************
 * BMP declaration                                               *
 *****************************************************************/
Adafruit_BMP085 bmp;

/*****************************************************************
 * BMP/BME280 declaration  (BMX280_i2c.h)                        *
 *****************************************************************/
BMX280 bmx280;
const uint8_t bmx280_default_i2c_address = 0x77;
const uint8_t bmx280_alternate_i2c_address = 0x76;

/*****************************************************************
 * SHT3x declaration                                             *
 *****************************************************************/
Adafruit_SHT31 sht3x;

/*****************************************************************
 * DS18B20 declaration                                            *
 *****************************************************************/
OneWire oneWire;
DallasTemperature ds18b20(&oneWire);

/*****************************************************************
 * SEN5X declaration                                             *
 *****************************************************************/
SensirionI2CSen5x sen5x;

/*****************************************************************
 * SCD30 declaration                                             *
 *****************************************************************/
SCD30 scd30;

/*****************************************************************
 * GPS declaration                                               *
 *****************************************************************/
TinyGPSPlus gps;

/*****************************************************************
 * Variable Definitions for PPD24NS                              *
 * P1 for PM10 & P2 for PM25                                     *
 *****************************************************************/

boolean trigP1 = false;
boolean trigP2 = false;
unsigned long trigOnP1;
unsigned long trigOnP2;

unsigned long lowpulseoccupancyP1 = 0;
unsigned long lowpulseoccupancyP2 = 0;

bool send_now = false;
unsigned long starttime;
unsigned long time_point_device_start_ms;
unsigned long starttime_SDS;
unsigned long starttime_GPS;
unsigned long starttime_NPM;
unsigned long starttime_IPS;
unsigned long starttime_MQTT;

unsigned long act_micro;
unsigned long act_milli;
unsigned long last_micro = 0;
unsigned long min_micro = 1000000000;
unsigned long max_micro = 0;

bool is_SDS_running = true;
bool is_SEN5X_running = true;

enum SDS_WAITING
{
	SDS_REPLY_HDR = 10,
	SDS_REPLY_BODY = 8
} SDS_waiting_for; 	//for header/body

// To read NPM responses
enum NPM_WAITING_16
{
	NPM_REPLY_HEADER_16 = 16,
	NPM_REPLY_STATE_16 = 14,
	NPM_REPLY_BODY_16 = 13,
	NPM_REPLY_CHECKSUM_16 = 1
} NPM_waiting_for_16; //for concentration

enum NPM_WAITING_4
{
	NPM_REPLY_HEADER_4 = 4,
	NPM_REPLY_STATE_4 = 2,
	NPM_REPLY_CHECKSUM_4 = 1
} NPM_waiting_for_4; //for change

enum NPM_WAITING_5
{
	NPM_REPLY_HEADER_5 = 5,
	NPM_REPLY_STATE_5 = 3,
	NPM_REPLY_DATA_5 = 2,
	NPM_REPLY_CHECKSUM_5 = 1
} NPM_waiting_for_5; //for fan speed

enum NPM_WAITING_6
{
	NPM_REPLY_HEADER_6 = 6,
	NPM_REPLY_STATE_6 = 4,
	NPM_REPLY_DATA_6 = 3,
	NPM_REPLY_CHECKSUM_6 = 1
} NPM_waiting_for_6; // for version

enum NPM_WAITING_8
{
	NPM_REPLY_HEADER_8 = 8,
	NPM_REPLY_STATE_8 = 6,
	NPM_REPLY_BODY_8 = 5,
	NPM_REPLY_CHECKSUM_8 = 1
} NPM_waiting_for_8; // for temperature/humidity


//ENUM POUR IPS??

String current_state_npm;
String current_th_npm;

bool is_PMS_running = true;
bool is_HPM_running = true;
bool is_NPM_running = false;
bool is_IPS_running;

unsigned long sending_time = 0;
unsigned long last_update_attempt;
int last_update_returncode;
int last_sendData_returncode;

float last_value_BMP_T = -128.0;
float last_value_BMP_P = -1.0;
float last_value_BMX280_T = -128.0;
float last_value_BMX280_P = -1.0;
float last_value_BME280_H = -1.0;
float last_value_DHT_T = -128.0;
float last_value_DHT_H = -1.0;
float last_value_DS18B20_T = -1.0;
float last_value_HTU21D_T = -128.0;
float last_value_HTU21D_H = -1.0;
float last_value_SHT3X_T = -128.0;
float last_value_SHT3X_H = -1.0;
float last_value_SCD30_T = -128.0;
float last_value_SCD30_H = -1.0;
uint16_t last_value_SCD30_CO2 = 0;

uint32_t sds_pm10_sum = 0;
uint32_t sds_pm25_sum = 0;
uint32_t sds_val_count = 0;
uint32_t sds_pm10_max = 0;
uint32_t sds_pm10_min = 20000;
uint32_t sds_pm25_max = 0;
uint32_t sds_pm25_min = 20000;

int pms_pm1_sum = 0;
int pms_pm10_sum = 0;
int pms_pm25_sum = 0;
int pms_val_count = 0;
int pms_pm1_max = 0;
int pms_pm1_min = 20000;
int pms_pm10_max = 0;
int pms_pm10_min = 20000;
int pms_pm25_max = 0;
int pms_pm25_min = 20000;

int hpm_pm10_sum = 0;
int hpm_pm25_sum = 0;
int hpm_val_count = 0;
int hpm_pm10_max = 0;
int hpm_pm10_min = 20000;
int hpm_pm25_max = 0;
int hpm_pm25_min = 20000;

uint32_t npm_pm1_sum = 0;
uint32_t npm_pm10_sum = 0;
uint32_t npm_pm25_sum = 0;
uint32_t npm_pm1_sum_pcs = 0;
uint32_t npm_pm10_sum_pcs = 0;
uint32_t npm_pm25_sum_pcs = 0;
uint16_t npm_val_count = 0;
uint16_t npm_pm1_max = 0;
uint16_t npm_pm1_min = 20000;
uint16_t npm_pm10_max = 0;
uint16_t npm_pm10_min = 20000;
uint16_t npm_pm25_max = 0;
uint16_t npm_pm25_min = 20000;
uint16_t npm_pm1_max_pcs = 0;
uint16_t npm_pm1_min_pcs = 60000;
uint16_t npm_pm10_max_pcs = 0;
uint16_t npm_pm10_min_pcs = 60000;
uint16_t npm_pm25_max_pcs = 0;
uint16_t npm_pm25_min_pcs = 60000;

float ips_pm01_sum = 0;
float ips_pm03_sum = 0;
float ips_pm05_sum = 0;
float ips_pm1_sum = 0;
float ips_pm25_sum = 0;
float ips_pm5_sum = 0;
float ips_pm10_sum = 0;
unsigned long ips_pm01_sum_pcs = 0;
unsigned long ips_pm03_sum_pcs = 0;
unsigned long ips_pm05_sum_pcs = 0;
unsigned long ips_pm1_sum_pcs = 0;
unsigned long ips_pm25_sum_pcs = 0;
unsigned long ips_pm5_sum_pcs = 0;
unsigned long ips_pm10_sum_pcs = 0;
uint16_t ips_val_count = 0;
float ips_pm01_max = 0;
float ips_pm01_min = 200;
float ips_pm03_max = 0;
float ips_pm03_min = 200;
float ips_pm05_max = 0;
float ips_pm05_min = 200;
float ips_pm1_max = 0;
float ips_pm1_min = 200;
float ips_pm25_max = 0;
float ips_pm25_min = 200;
float ips_pm5_max = 0;
float ips_pm5_min = 200;
float ips_pm10_max = 0;
float ips_pm10_min = 200;
unsigned long ips_pm01_max_pcs = 0;
unsigned long  ips_pm01_min_pcs = 4000000000;
unsigned long  ips_pm03_max_pcs = 0;
unsigned long  ips_pm03_min_pcs = 4000000000;
unsigned long  ips_pm05_max_pcs = 0;
unsigned long  ips_pm05_min_pcs = 4000000000;
unsigned long  ips_pm1_max_pcs = 0;
unsigned long  ips_pm1_min_pcs = 4000000000;
unsigned long  ips_pm25_max_pcs = 0;
unsigned long  ips_pm25_min_pcs = 4000000000;
unsigned long  ips_pm5_max_pcs = 0;
unsigned long ips_pm5_min_pcs = 4000000000;
unsigned long  ips_pm10_max_pcs = 0;
unsigned long  ips_pm10_min_pcs = 4000000000;

float last_value_SEN5X_P0 = -1.0;
float last_value_SEN5X_P1 = -1.0;
float last_value_SEN5X_P2 = -1.0;
float last_value_SEN5X_P4 = -1.0;
float last_value_SEN5X_N05 = -1.0;
float last_value_SEN5X_N1 = -1.0;
float last_value_SEN5X_N25 = -1.0;
float last_value_SEN5X_N4 = -1.0;
float last_value_SEN5X_N10 = -1.0;
float last_value_SEN5X_TS = -1.0;
float last_value_SEN5X_T = -128.0;
float last_value_SEN5X_H = -1.0;
float last_value_SEN5X_VOC = -1.0;
float last_value_SEN5X_NOX = -1.0;
float value_SEN5X_P0 = 0.0;
float value_SEN5X_P1 = 0.0;
float value_SEN5X_P2 = 0.0;
float value_SEN5X_P4 = 0.0;
float value_SEN5X_N05 = 0.0;
float value_SEN5X_N1 = 0.0;
float value_SEN5X_N25 = 0.0;
float value_SEN5X_N4 = 0.0;
float value_SEN5X_N10 = 0.0;
float value_SEN5X_TS = 0.0;
float value_SEN5X_T = 0.0;
float value_SEN5X_H = 0.0;
float value_SEN5X_VOC = 0.0;
float value_SEN5X_NOX = 0.0;

uint16_t SEN5X_measurement_count = 0;
unsigned long SEN5X_read_counter = 0;
unsigned long SEN5X_read_error_counter = 0;
unsigned long SEN5X_read_timer = 0;
bool is_Sen5x_init_failed = false;
bool is_Sen5x_running = true;						// FFU SEN5X Start/Stop moet er nog in. (ivm energie verbruik)

float last_value_SPS30_P0 = -1.0;
float last_value_SPS30_P1 = -1.0;
float last_value_SPS30_P2 = -1.0;
float last_value_SPS30_P4 = -1.0;
float last_value_SPS30_N05 = -1.0;
float last_value_SPS30_N1 = -1.0;
float last_value_SPS30_N25 = -1.0;
float last_value_SPS30_N4 = -1.0;
float last_value_SPS30_N10 = -1.0;
float last_value_SPS30_TS = -1.0;
float value_SPS30_P0 = 0.0;
float value_SPS30_P1 = 0.0;
float value_SPS30_P2 = 0.0;
float value_SPS30_P4 = 0.0;
float value_SPS30_N05 = 0.0;
float value_SPS30_N1 = 0.0;
float value_SPS30_N25 = 0.0;
float value_SPS30_N4 = 0.0;
float value_SPS30_N10 = 0.0;
float value_SPS30_TS = 0.0;

uint16_t SPS30_measurement_count = 0;
unsigned long SPS30_read_counter = 0;
unsigned long SPS30_read_error_counter = 0;
unsigned long SPS30_read_timer = 0;
bool sps30_init_failed = false;

float last_value_PPD_P1 = -1.0;
float last_value_PPD_P2 = -1.0;
float last_value_SDS_P1 = -1.0;
float last_value_SDS_P2 = -1.0;
float last_value_PMS_P0 = -1.0;
float last_value_PMS_P1 = -1.0;
float last_value_PMS_P2 = -1.0;
float last_value_HPM_P1 = -1.0;
float last_value_HPM_P2 = -1.0;
float last_value_NPM_P0 = -1.0;
float last_value_NPM_P1 = -1.0;
float last_value_NPM_P2 = -1.0;
float last_value_NPM_N1 = -1.0;
float last_value_NPM_N10 = -1.0;
float last_value_NPM_N25 = -1.0;

float last_value_IPS_P0 = -1.0; //PM1
float last_value_IPS_P1 = -1.0;	//PM10
float last_value_IPS_P2 = -1.0;	//PM2.5
float last_value_IPS_P01 = -1.0; //PM0.1
float last_value_IPS_P03 = -1.0; //PM0.3 //ATTENTION P4 = PM4 POUR SPS30
float last_value_IPS_P05 = -1.0; //PM0.5
float last_value_IPS_P5 = -1.0; //PM5
float last_value_IPS_N1 = -1.0;
float last_value_IPS_N10 = -1.0;
float last_value_IPS_N25 = -1.0;
float last_value_IPS_N01 = -1.0;
float last_value_IPS_N03 = -1.0; //ATTENTION P4 = PM4 POUR SPS30
float last_value_IPS_N05 = -1.0;
float last_value_IPS_N5 = -1.0;

float last_value_GPS_alt = -1000.0;
double last_value_GPS_lat = -200.0;
double last_value_GPS_lon = -200.0;
String last_value_GPS_timestamp;
String last_data_string;
int last_signal_strength;
int last_disconnect_reason;

String esp_chipid;
String esp_mac_id;
String last_value_SDS_version;
String last_value_NPM_version;
String last_value_IPS_version;

unsigned long SDS_error_count;
unsigned long NPM_error_count;
unsigned long IPS_error_count;
unsigned long WiFi_error_count;

unsigned long last_page_load = millis();

bool wificonfig_loop = false;
uint8_t sntp_time_set = 0;

unsigned long count_sends = 0;
unsigned long last_display_millis = 0;
uint8_t next_display_count = 0;

struct struct_wifiInfo
{
	char ssid[LEN_WLANSSID];
	uint8_t encryptionType;
	int32_t RSSI;
	int32_t channel;
#if defined(ESP8266)
	bool isHidden;
	uint8_t unused[3];
#endif
};

String json_config_memory_used;					// Status web
String json_config7000_memory_used;				// Status web

struct struct_wifiInfo *wifiInfo;
uint8_t count_wifiInfo;

IPAddress addr_static_ip;
IPAddress addr_static_subnet;
IPAddress addr_static_gateway;
IPAddress addr_static_dns;

#define msSince(timestamp_before) (act_milli - (timestamp_before))

const char data_first_part[] PROGMEM = "{\"software_version\": \"" SOFTWARE_VERSION_STR "\", \"sensordatavalues\":[";
const char JSON_SENSOR_DATA_VALUES[] PROGMEM = "sensordatavalues";

/*****************************************************************
 * Display values for debugging.                                 *
 *****************************************************************/
static void display_debug(const String &text1, const String &text2)
{
	debug_outln_info(F("output debug text to displays..."));

	if (oled_ssd1306)
	{
		oled_ssd1306->clear();
		oled_ssd1306->displayOn();
		oled_ssd1306->setTextAlignment(TEXT_ALIGN_LEFT);
		oled_ssd1306->drawString(0, 12, text1);
		oled_ssd1306->drawString(0, 24, text2);
		oled_ssd1306->display();
	}

	if (oled_sh1106)
	{
		oled_sh1106->clear();
		oled_sh1106->displayOn();
		oled_sh1106->setTextAlignment(TEXT_ALIGN_LEFT);
		oled_sh1106->drawString(0, 12, text1);
		oled_sh1106->drawString(0, 24, text2);
		oled_sh1106->display();
	}

	if (lcd_1602)
	{
		lcd_1602->clear();
		lcd_1602->setCursor(0, 0);
		lcd_1602->print(text1);
		lcd_1602->setCursor(0, 1);
		lcd_1602->print(text2);
	}

	if (lcd_2004)
	{
		lcd_2004->clear();
		lcd_2004->setCursor(0, 0);
		lcd_2004->print(text1);
		lcd_2004->setCursor(0, 1);
		lcd_2004->print(text2);
	}
}

/*****************************************************************
 * read SDS011 sensor serial and firmware date                   *
 *****************************************************************/
static String SDS_version_date()
{
	if (cfg::sds_read && !last_value_SDS_version.length())
	{
		debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(DBG_TXT_SDS011_VERSION_DATE));
		is_SDS_running = SDS_cmd(PmSensorCmd::Start);
		delay(250);

#if defined(ESP8266)
		serialSDS.perform_work();
#endif

		serialSDS.flush();

		// Query Version/Date
		SDS_rawcmd(0x07, 0x00, 0x00);
		delay(400);
		const constexpr uint8_t header_cmd_response[2] = {0xAA, 0xC5};

		while (serialSDS.find(header_cmd_response, sizeof(header_cmd_response)))
		{
			uint8_t data[8];
			unsigned r = serialSDS.readBytes(data, sizeof(data));

			if (r == sizeof(data) && data[0] == 0x07 && SDS_checksum_valid(data))
			{
				char tmp[20];
				snprintf_P(tmp, sizeof(tmp), PSTR("%02d-%02d-%02d(%02x%02x)"),
						   data[1], data[2], data[3], data[4], data[5]);
				last_value_SDS_version = tmp;
				break;
			}
		}

		debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(DBG_TXT_SDS011_VERSION_DATE));
	}

	return last_value_SDS_version;
}

/*****************************************************************
 * read Next PM sensor serial and firmware date                   *
 *****************************************************************/

static uint8_t NPM_get_state()
{
	uint8_t result = 0;
	NPM_waiting_for_4 = NPM_REPLY_HEADER_4;
	debug_outln_info(F("State NPM..."));
	NPM_cmd(PmSensorCmd2::State);

	while (!serialNPM.available())
	{
		debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
	}

	while (serialNPM.available() >= NPM_waiting_for_4)
	{
		const uint8_t constexpr header[2] = {0x81, 0x16};
		uint8_t state[1];
		uint8_t checksum[1];
		uint8_t test[4];

		switch (NPM_waiting_for_4)
		{
		case NPM_REPLY_HEADER_4:
			if (serialNPM.find(header, sizeof(header)))
				NPM_waiting_for_4 = NPM_REPLY_STATE_4;
			break;

		case NPM_REPLY_STATE_4:
			serialNPM.readBytes(state, sizeof(state));
			NPM_state(state[0]);
			result = state[0];
			NPM_waiting_for_4 = NPM_REPLY_CHECKSUM_4;
			break;

		case NPM_REPLY_CHECKSUM_4:
			serialNPM.readBytes(checksum, sizeof(checksum));
			memcpy(test, header, sizeof(header));
			memcpy(&test[sizeof(header)], state, sizeof(state));
			memcpy(&test[sizeof(header) + sizeof(state)], checksum, sizeof(checksum));
			NPM_data_reader(test, 4);
			NPM_waiting_for_4 = NPM_REPLY_HEADER_4;

			if (NPM_checksum_valid_4(test))
			{
				debug_outln_info(F("Checksum OK..."));
			}
			break;
		}
	}

	return result;
}

static bool NPM_start_stop()
{
	bool result = false;
	NPM_waiting_for_4 = NPM_REPLY_HEADER_4;
	debug_outln_info(F("Switch start/stop NPM..."));
	NPM_cmd(PmSensorCmd2::Change);

	while (!serialNPM.available())
	{
		debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
	}

	while (serialNPM.available() >= NPM_waiting_for_4)
	{
		const uint8_t constexpr header[2] = {0x81, 0x15};
		uint8_t state[1];
		uint8_t checksum[1];
		uint8_t test[4];

		switch (NPM_waiting_for_4)
		{
		case NPM_REPLY_HEADER_4:
			if (serialNPM.find(header, sizeof(header)))
				NPM_waiting_for_4 = NPM_REPLY_STATE_4;
			break;

		case NPM_REPLY_STATE_4:
			serialNPM.readBytes(state, sizeof(state));
			NPM_state(state[0]);

			if (bitRead(state[0], 0) == 0)
			{
				debug_outln_info(F("NPM start..."));
				result = true;
			}
			else if (bitRead(state[0], 0) == 1)
			{
				debug_outln_info(F("NPM stop..."));
				result = false;
			}
			else
			{
				result = !is_NPM_running; //DANGER BECAUSE NON INITIALISED
			}

			NPM_waiting_for_4 = NPM_REPLY_CHECKSUM_4;
			break;

		case NPM_REPLY_CHECKSUM_4:
			serialNPM.readBytes(checksum, sizeof(checksum));
			memcpy(test, header, sizeof(header));
			memcpy(&test[sizeof(header)], state, sizeof(state));
			memcpy(&test[sizeof(header) + sizeof(state)], checksum, sizeof(checksum));
			NPM_data_reader(test, 4);
			NPM_waiting_for_4 = NPM_REPLY_HEADER_4;
			if (NPM_checksum_valid_4(test))
			{
				debug_outln_info(F("Checksum OK..."));
			}
			result = true;
			break;
		}
	}

	return result; //ATTENTION
}

static String NPM_version_date()
{
	//debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(DBG_TXT_NPM_VERSION_DATE));
	delay(250);
	NPM_waiting_for_6 = NPM_REPLY_HEADER_6;
	debug_outln_info(F("Version NPM..."));
	NPM_cmd(PmSensorCmd2::Version);

	while (!serialNPM.available())
	{
		debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
	}

	while (serialNPM.available() >= NPM_waiting_for_6)
	{
		const uint8_t constexpr header[2] = {0x81, 0x17};
		uint8_t state[1];
		uint8_t data[2];
		uint8_t checksum[1];
		uint8_t test[6];

		switch (NPM_waiting_for_6)
		{
		case NPM_REPLY_HEADER_6:
			if (serialNPM.find(header, sizeof(header)))
				NPM_waiting_for_6 = NPM_REPLY_STATE_6;
			break;
			
		case NPM_REPLY_STATE_6:
			serialNPM.readBytes(state, sizeof(state));
			NPM_state(state[0]);
			NPM_waiting_for_6 = NPM_REPLY_DATA_6;
			break;

		case NPM_REPLY_DATA_6:
			if (serialNPM.readBytes(data, sizeof(data)) == sizeof(data))
			{
				NPM_data_reader(data, 2);
				uint16_t NPMversion = word(data[0], data[1]);
				last_value_NPM_version = String(NPMversion);
				//debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(DBG_TXT_NPM_VERSION_DATE));
				debug_outln_info(F("Next PM Firmware: "), last_value_NPM_version);
			}
			NPM_waiting_for_6 = NPM_REPLY_CHECKSUM_6;
			break;

		case NPM_REPLY_CHECKSUM_6:
			serialNPM.readBytes(checksum, sizeof(checksum));
			memcpy(test, header, sizeof(header));
			memcpy(&test[sizeof(header)], state, sizeof(state));
			memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
			memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));
			NPM_data_reader(test, 6);
			NPM_waiting_for_6 = NPM_REPLY_HEADER_6;
			if (NPM_checksum_valid_6(test))
			{
				debug_outln_info(F("Checksum OK..."));
			}
			break;
		}
	}
	return last_value_NPM_version;
}

#pragma GCC diagnostic ignored "-Wunused-function"
/*
*
*/
static void NPM_fan_speed()
{
	NPM_waiting_for_5 = NPM_REPLY_HEADER_5;
	debug_outln_info(F("Set fan speed to 50 %..."));
	NPM_cmd(PmSensorCmd2::Speed);

	while (!serialNPM.available())
	{
		debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
	}

	while (serialNPM.available() >= NPM_waiting_for_5)
	{
		const uint8_t constexpr header[2] = {0x81, 0x21};
		uint8_t state[1];
		uint8_t data[1];
		uint8_t checksum[1];
		uint8_t test[5];

		switch (NPM_waiting_for_5)
		{
		case NPM_REPLY_HEADER_5:
			if (serialNPM.find(header, sizeof(header)))
				NPM_waiting_for_5 = NPM_REPLY_STATE_5;
			break;

		case NPM_REPLY_STATE_5:
			serialNPM.readBytes(state, sizeof(state));
			NPM_state(state[0]);
			NPM_waiting_for_5 = NPM_REPLY_DATA_5;
			break;

		case NPM_REPLY_DATA_5:
			if (serialNPM.readBytes(data, sizeof(data)) == sizeof(data))
			{
				NPM_data_reader(data, 1);
			}
			NPM_waiting_for_5 = NPM_REPLY_CHECKSUM_5;
			break;

		case NPM_REPLY_CHECKSUM_5:
			serialNPM.readBytes(checksum, sizeof(checksum));
			memcpy(test, header, sizeof(header));
			memcpy(&test[sizeof(header)], state, sizeof(state));
			memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
			memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));
			NPM_data_reader(test, 5);
			NPM_waiting_for_5 = NPM_REPLY_HEADER_5;
			if (NPM_checksum_valid_5(test))
			{
				debug_outln_info(F("Checksum OK..."));
			}
			break;
		}
	}
}

#pragma GCC diagnostic pop


static String NPM_temp_humi()
{
	uint16_t NPM_temp = 0;
	uint16_t NPM_humi = 0;
	NPM_waiting_for_8 = NPM_REPLY_HEADER_8;
	debug_outln_info(F("Temperature/Humidity in Next PM..."));

	NPM_cmd(PmSensorCmd2::Temphumi);
	while (!serialNPM.available())
	{
		debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
	}

	while (serialNPM.available() >= NPM_waiting_for_8)
	{
		const uint8_t constexpr header[2] = {0x81, 0x14};
		uint8_t state[1];
		uint8_t data[4];
		uint8_t checksum[1];
		uint8_t test[8];

		switch (NPM_waiting_for_8)
		{
		case NPM_REPLY_HEADER_8:
			if (serialNPM.find(header, sizeof(header)))
				NPM_waiting_for_8 = NPM_REPLY_STATE_8;
			break;

		case NPM_REPLY_STATE_8:
			serialNPM.readBytes(state, sizeof(state));
			NPM_state(state[0]);
			NPM_waiting_for_8 = NPM_REPLY_BODY_8;
			break;

		case NPM_REPLY_BODY_8:
			if (serialNPM.readBytes(data, sizeof(data)) == sizeof(data))
			{
				NPM_data_reader(data, 4);
				NPM_temp = word(data[0], data[1]);
				NPM_humi = word(data[2], data[3]);

				debug_outln_verbose(F("Temperature (°C): "), String(NPM_temp / 100.0f));
				debug_outln_verbose(F("Relative humidity (%): "), String(NPM_humi / 100.0f));
			}
			NPM_waiting_for_8 = NPM_REPLY_CHECKSUM_8;
			break;

		case NPM_REPLY_CHECKSUM_16:
			serialNPM.readBytes(checksum, sizeof(checksum));
			memcpy(test, header, sizeof(header));
			memcpy(&test[sizeof(header)], state, sizeof(state));
			memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
			memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));
			NPM_data_reader(test, 8);
			if (NPM_checksum_valid_8(test))
			{
				debug_outln_info(F("Checksum OK..."));
			}

			NPM_waiting_for_8 = NPM_REPLY_HEADER_8;
			break;
		}
	}

	return String(NPM_temp / 100.0f) + " / " + String(NPM_humi / 100.0f);
}

/*****************************************************************
 * read IPS-7100 sensor serial and firmware date                   *
*****************************************************************/

static String IPS_version_date()
{
	debug_outln_info(F("Version IPS..."));
	String serial_data;

	IPS_cmd(PmSensorCmd3::Reset);

	if (serialIPS.available() > 0)
	{
		serial_data = serialIPS.readString();
		//Debug.println(serial_data);
	}

	int index1 = serial_data.indexOf("VERSION_NUMBER ");
	int index2 = serial_data.indexOf("\nSERIAL_NUMBER ");
	int index3 = serial_data.indexOf("\nD:");

	String version_ips = serial_data.substring(index1+15,index2-1);
	String serial_ips = serial_data.substring(index2+15,index3-1);

	last_value_IPS_version = version_ips + "/" + serial_ips;

	debug_outln_info(F("IPS-7100 Version/Serial Number: "), last_value_IPS_version);

	return last_value_IPS_version;
}


/*****************************************************************
 * read SEN5X sensor serial and firmware date                    *
 *****************************************************************/

unsigned char SEN5X_type[6];

#define MAXBUF_REQUIREMENT 48

#if (defined(I2C_BUFFER_LENGTH) && (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) ||  \
    (defined(BUFFER_LENGTH)     && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
 #define USE_PRODUCT_INFO
#endif

void printModuleVersions() 
{
    uint16_t error;
    char errorMessage[256];

    unsigned char productName[32];
    uint8_t productNameSize = 32;

    error = sen5x.getProductName(productName, productNameSize);

    if (error) 
	{
        Debug.print("Error trying to execute getProductName(): ");
        errorToString(error, errorMessage, 256);
        Debug.println(errorMessage);
    } 
	else
	{
        Debug.print("ProductName:");
        Debug.println((char*)productName);
		memcpy(SEN5X_type, productName, 6);

		//Debug.println((char*)SEN5X_type);
    }

    uint8_t firmwareMajor;
    uint8_t firmwareMinor;
    bool firmwareDebug;
    uint8_t hardwareMajor;
    uint8_t hardwareMinor;
    uint8_t protocolMajor;
    uint8_t protocolMinor;

    error = sen5x.getVersion(firmwareMajor, firmwareMinor, firmwareDebug,
                             hardwareMajor, hardwareMinor, protocolMajor,
                             protocolMinor);
    if (error) 
	{
        Debug.print("Error trying to execute getVersion(): ");
        errorToString(error, errorMessage, 256);
        Debug.println(errorMessage);
    } 
	else 
	{
        Debug.print("Firmware: ");
        Debug.print(firmwareMajor);
        Debug.print(".");
        Debug.print(firmwareMinor);
        Debug.print(", ");

        Debug.print("Hardware: ");
        Debug.print(hardwareMajor);
        Debug.print(".");
        Debug.println(hardwareMinor);
    }
}

void printSerialNumber() 
{
    uint16_t error;
    char errorMessage[256];
    unsigned char serialNumber[32];
    uint8_t serialNumberSize = 32;

    error = sen5x.getSerialNumber(serialNumber, serialNumberSize);

    if (error) 
	{
        Debug.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Debug.println(errorMessage);
    } 
	else 
	{
        Debug.print("SerialNumber:");
        Debug.println((char*)serialNumber);
    }
}


/*****************************************************************
 * disable unneeded NMEA sentences, TinyGPS++ needs GGA, RMC     *
 *****************************************************************/
static void disable_unneeded_nmea()
{
	serialGPS->println(F("$PUBX,40,GLL,0,0,0,0*5C")); // Geographic position, latitude / longitude
													  //	serialGPS->println(F("$PUBX,40,GGA,0,0,0,0*5A"));       // Global Positioning System Fix Data
	serialGPS->println(F("$PUBX,40,GSA,0,0,0,0*4E")); // GPS DOP and active satellites
													  //	serialGPS->println(F("$PUBX,40,RMC,0,0,0,0*47"));       // Recommended minimum specific GPS/Transit data
	serialGPS->println(F("$PUBX,40,GSV,0,0,0,0*59")); // GNSS satellites in view
	serialGPS->println(F("$PUBX,40,VTG,0,0,0,0*5E")); // Track made good and ground speed
}

/*****************************************************************
 * read config from spiffs                                       *
 *****************************************************************/

/* backward compatibility for the times when we stored booleans as strings */
static bool boolFromJSON(const DynamicJsonDocument &json, const __FlashStringHelper *key)
{
	if (json[key].is<const char *>())
	{
		return !strcmp_P(json[key].as<const char *>(), PSTR("true"));
	}
	return json[key].as<bool>();
}

/*****************************************************************
 * read config from spiffs                                       *
 *****************************************************************/
static void readConfig(bool oldconfig = false)
{
	//debug_outln_info(F("*** call readConfigBase()... ***"));
	readConfigBase( oldconfig);

	//debug_outln_info(F("*** call readConfigS7000()... ***"));
	readConfigS7000( oldconfig);
}

/*****************************************************************
/// @brief 
/// Read config data from SPIFFS E-memory.
/// @param oldconfig
******************************************************************/
static void readConfigBase(bool oldconfig)
{
	bool rewriteConfig = false;

	String cfgName(F("/config.json"));

	if (oldconfig)
	{
		cfgName += F("/config.json.old");
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

	File configFile = SPIFFS.open(cfgName, "r");

	if (!configFile)
	{
		if (!oldconfig)
		{	// call 
			debug_outln_info(F("Try to open Config file: "), cfgName );
			return readConfigBase(true /* oldconfig */);
		}

		debug_outln_error(F("Failed to open config file."));
		return;
	}

	debug_outln_info(F("Opened config file..."));

	DynamicJsonDocument json(JSON_BUFFER_SIZE);
	DeserializationError err = deserializeJson(json, configFile.readString());

	debug_outln_info(F("Read JSON format.....\nJson memory size: "), String(json.memoryUsage()) + 
					 " | Elements in array: " + String(json.size()) + 
					 String(" | Error Code = ") + err.code() + " => " + err.f_str() );

	json_config_memory_used = String(json.memoryUsage());

	configFile.seek(0);				// set file pointer back to begin file.
	debug_outln_info(F("Read(): Config file content: ***\n"), configFile.readString() + String("\n***") );
	configFile.close();

	if (err.code() == DeserializationError::InvalidInput)
	{// Check Json string
		String json_string;
		serializeJson(json, json_string);
		debug_outln_info(F("readConfig():Parse => [JSON] input: \n"), json_string.c_str());

		if (json_string.startsWith("{") && json_string.endsWith("}"))
		{ // still a good Json format
			err = DeserializationError(DeserializationError::Ok);
		}
	}

#pragma GCC diagnostic pop

	if ( !err )
	{
		serializeJsonPretty(json, Debug);		// display all members + value of config file.
		debug_outln_info(F("\nparsed json...\nJson memory size: "), String(json.memoryUsage()) + String(" char."));

		// "configShape" memory array[], defined in airrohr-cfg.h
		for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e)
		{
			ConfigShapeEntry c;
			memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));

			if (json[c.cfg_key()].isNull())
			{
				debug_outln_info(F("Key NOT in configration file:..."), FPSTR(c.cfg_key()));
				continue;
			}

			switch (c.cfg_type)
			{
				case Config_Type_Bool:
					*(c.cfg_val.as_bool) = boolFromJSON(json, c.cfg_key());
					break;

				case Config_Type_UInt:
				case Config_Type_Time:
					*(c.cfg_val.as_uint) = json[c.cfg_key()].as<unsigned int>();
					break;

				case Config_Type_String:
				case Config_Type_Password:
					strncpy(c.cfg_val.as_str, json[c.cfg_key()].as<const char *>(), c.cfg_len);
					c.cfg_val.as_str[c.cfg_len] = '\0';	// set terminator char.
					break;

			};
		}

		String writtenVersion(json["SOFTWARE_VERSION"].as<const char *>());
		
		if (writtenVersion.length() && writtenVersion[0] == 'N' && SOFTWARE_VERSION != writtenVersion)
		{
			debug_outln_info(F("Rewriting old config from: "), writtenVersion);
			// would like to do that, but this would wipe firmware.old which the two stage loader
			// might still need
			// SPIFFS.format();
			rewriteConfig = true;
		}

		if (cfg::sending_intervall_ms < READINGTIME_SDS_MS)
		{
			cfg::sending_intervall_ms = READINGTIME_SDS_MS;
		}

		if (strcmp_P(cfg::senseboxid, PSTR("00112233445566778899aabb")) == 0)
		{
			cfg::senseboxid[0] = '\0';
			cfg::send2sensemap = false;
			rewriteConfig = true;
		}

		if (strlen(cfg::measurement_name_influx) == 0)
		{
			strcpy_P(cfg::measurement_name_influx, MEASUREMENT_NAME_INFLUX);
			rewriteConfig = true;
		}

		if (strcmp_P(cfg::host_influx, PSTR("api.luftdaten.info")) == 0)
		{
			cfg::host_influx[0] = '\0';
			cfg::send2influx = false;
			rewriteConfig = true;
		}

		if (boolFromJSON(json, F("pm24_read")) || boolFromJSON(json, F("pms32_read")))
		{
			cfg::pms_read = true;
			rewriteConfig = true;
		}

		if (boolFromJSON(json, F("bmp280_read")) || boolFromJSON(json, F("bme280_read")))
		{
			cfg::bmx280_read = true;
			rewriteConfig = true;
		}
	}
	else
	{
		debug_outln_error(F("Failed to load json config"));
		debug_outln_info(F("Config file: "), cfgName );

		if (!oldconfig)
		{
			debug_outln_error(F("Return, call readConfig(true /* oldconfig */"));
			return readConfigBase(true /* oldconfig */);
		}
	}

	if (rewriteConfig)
	{
		writeConfigBase();
	}

	debug_outln_info(F("Exit: readConfigBase() methode."));

}	// readConfigBase()

/*****************************************************************
/// @brief 
/// Read config S7000 data from SPIFFS E-memory.
/// @param oldconfig
******************************************************************/
static void readConfigS7000(bool oldconfig)
{
	bool rewriteConfig = false;

	String cfgName(F("/simm7000.json"));

	if (oldconfig)
	{
		cfgName += F("/simm7000.json.old");
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

	File configFile= SPIFFS.open(cfgName, "r");

	if (!configFile)
	{
		if (!oldconfig)
		{	// call 
			return readConfigS7000(true /* configFile */);
		}

		debug_outln_error(F("Failed to open config S7000 file."));
		return;
	}

	debug_outln_info(F("Opened config S7000 file..."));

	DynamicJsonDocument json(JSON_BUFFER_SIZE_SIMM7000);
	DeserializationError err = deserializeJson(json, configFile.readString());

	debug_outln_info(F("Read JSON S7000 format.....\nJson memory size: "), String(json.memoryUsage()) + 
					 " | Elementen in array: " + String(json.size()) + 
					 String(" | Error Code = ") + err.code() + " => " + err.f_str() );

	json_config7000_memory_used = String(json.memoryUsage());

	configFile.seek(0);				// set file pointer back to begin file.
	debug_outln_info(F("Read(): Config file content: ***\n"), configFile.readString() + String("\n***") );
	configFile.close();

	if (err.code() == DeserializationError::InvalidInput)
	{// Check Json string
		String json_string;
		serializeJson(json, json_string);
		debug_outln_info(F("readConfig():Parse => [JSON] input: \n"), json_string.c_str());

		if (json_string.startsWith("{") && json_string.endsWith("}"))
		{ // still a good Json format
			err = DeserializationError(DeserializationError::Ok);
		}
	}

#pragma GCC diagnostic pop

	if ( !err )
	{
		serializeJsonPretty(json, Debug);					// display all members + value of config file.
		debug_outln_info(F("\nparsed json7...\nJson memory size: "), String(json.memoryUsage()) + String(" char."));

		// "configShape" memory array[], defined in airrohr-cfg.h
		for (unsigned e = 0; e < sizeof(configShape7) / sizeof(configShape7[0]); ++e)
		{
			Config7000ShapeEntry c;
			memcpy_P(&c, &configShape7[e], sizeof(Config7000ShapeEntry));

			if (json[c.cfg_key()].isNull())
			{
				debug_outln_info(F("Key NOT in configration file:..."), FPSTR(c.cfg_key()));
				continue;
			}

			switch (c.cfg_type)
			{
				case Config7_Type_Bool:
					*(c.cfg_val.as_bool) = boolFromJSON(json, c.cfg_key());
					break;

				case Config7_Type_UInt:
					*(c.cfg_val.as_uint) = json[c.cfg_key()].as<unsigned int>();
					break;

				case Config7_Type_String:
					strncpy(c.cfg_val.as_str, json[c.cfg_key()].as<const char *>(), c.cfg_len);
					c.cfg_val.as_str[c.cfg_len] = '\0';			// set terminator char.
					break;
			};
		}
	}
	else
	{
		debug_outln_error(F("failed to load JSON S7000 config"));

		if (!oldconfig)
		{
			debug_outln_error(F("Simm7000 -return readConfig(true /* oldconfig */"));
			return readConfigS7000(true /* oldconfig */);
		}
	}

// End Simm7000

	if (rewriteConfig)
	{
		writeConfigS7000();
	}

	debug_outln_info(F("Exit: readConfigS7000() methode."));
	
}	// readConfigS7000()

/*****************************************************************
/// @brief 
/// Init config data from SPIFFS E-memory.
/// @param None
******************************************************************/
static void init_config()
{
	debug_outln_info(F("mounting FS..."));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#if defined(ESP32)
	bool spiffs_begin_ok = SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED);
#else
	bool spiffs_begin_ok = SPIFFS.begin();
#endif

	if (!spiffs_begin_ok)
	{
		debug_outln_error(F("failed to mount FS"));
		return;
	}

	debug_outln_info(F("mounting FS done, read config values."));

	FSInfo fs_info;
	SPIFFS.info(fs_info);

	debug_outln_info(F("fs_info.totalBytes = "), String(fs_info.totalBytes));
	debug_outln_info(F("fs_info.usedBytes = "), String(fs_info.usedBytes));
	debug_outln_info(F("fs_info.blockSize = "), String(fs_info.blockSize));
	debug_outln_info(F("fs_info.pageSize = "), String(fs_info.pageSize));
	debug_outln_info(F("fs_info.maxOpenFiles = "), String(fs_info.maxOpenFiles));
	debug_outln_info(F("fs_info.maxPathLength = "), String(fs_info.maxPathLength));

#pragma GCC diagnostic pop

	readConfig();
}


/*****************************************************************
 * write config to spiffs                                        *
 *****************************************************************/
static bool writeConfig()
{
	bool ret = writeConfigBase();
	ret |= writeConfigS7000();

	return ret;
}

/*****************************************************************
 * write config to spiffs                                        *
 *****************************************************************/
static bool writeConfigBase()
{
	DynamicJsonDocument json(JSON_BUFFER_SIZE);

	debug_outln_info(F("Saving config..."));

	json["SOFTWARE_VERSION"] = SOFTWARE_VERSION;

	for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e)
	{
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));

		switch (c.cfg_type)
		{
			case Config_Type_Bool:
				json[c.cfg_key()].set(*c.cfg_val.as_bool);
				break;

			case Config_Type_UInt:
			case Config_Type_Time:
				json[c.cfg_key()].set(*c.cfg_val.as_uint);
				break;

			case Config_Type_Password:
			case Config_Type_String:
				json[c.cfg_key()].set(c.cfg_val.as_str);
				break;
		};
	}

	debug_outln_info(F("Write JSON format.....\nJson memory size: "), String(json.memoryUsage()) + 
					 " | Elementen in array: " + String(json.size()) );

  	String json_string;
  	serializeJson(json, json_string);
	debug_outln_info(F("writeConfigBase() => [JSON] format: \n"), json_string.c_str());

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

	SPIFFS.remove(F("/config.json.old"));
	SPIFFS.rename(F("/config.json"), F("/config.json.old"));

	File configFile = SPIFFS.open(F("/config.json"), "w");
	if (configFile)
	{
		serializeJson(json, configFile);
		
		debug_outln_info(F("Wait 2 second, before close Config file."));
		delay(2000);
		configFile.close();

		debug_outln_info(F("Write config json... => Json memory size: "), String(json.memoryUsage()) + String(" char."));
		debug_outln_info(F("Config written successfully."));
	}
	else
	{
		debug_outln_error(F("writeConfigBase():failed to open config file for writing"));
		return false;
	}

#pragma GCC diagnostic pop

	return true;
}

/*****************************************************************
 * write config to spiffs                                        *
 *****************************************************************/
static bool writeConfigS7000()
{
	DynamicJsonDocument json(JSON_BUFFER_SIZE_SIMM7000);

	debug_outln_info(F("Saving S7000 config..."));

	for (unsigned e = 0; e < sizeof(configShape7) / sizeof(configShape7[0]); ++e)
	{
		Config7000ShapeEntry c;
		memcpy_P(&c, &configShape7[e], sizeof(Config7000ShapeEntry));

		switch (c.cfg_type)
		{
			case Config7_Type_Bool:
				json[c.cfg_key()].set(*c.cfg_val.as_bool);
				break;

			case Config7_Type_UInt:
				json[c.cfg_key()].set(*c.cfg_val.as_uint);
				break;

			case Config7_Type_String:
				json[c.cfg_key()].set(c.cfg_val.as_str);
				break;
		};
	}

	// debug_outln_info(F("JSON 7000 format.....\nJson memory size: "), String(json.memoryUsage()) + 
	// 				 " | Elementen in array: " + String(json.size()));

  	// String json_string;
  	// serializeJson(json, json_string);
	// debug_outln_info(F("writeConfigS7000() => [JSON] format: \n"), json_string.c_str());


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"


	SPIFFS.remove(F("/simm7000.json.old"));
	SPIFFS.rename(F("/simm7000.json"), F("/simm7000.json.old"));

	File configFile = SPIFFS.open(F("/simm7000.json"), "w");

	if (configFile)
	{
		// write JSON content into configfile.
		serializeJson(json, configFile);
		
		debug_outln_info(F("Wait 2 second, before close Config file."));
		delay(2000);
		configFile.close();

		debug_outln_info(F("Write JSON 7000 format.....\nJson memory size: "), String(json.memoryUsage()) + 
					 	 F(" | Elementen in array: ") + String(json.size()) +
						 F("\nConfig written successfully."));

		String json_string;
  		serializeJson(json, json_string);
		debug_outln_info(F("writeConfigS7000() => [JSON] S7000 format: \n"), json_string.c_str());
	}
	else
	{
		debug_outln_error(F("writeConfigS7000():failed to open config S7000 file for writing"));
		return false;
	}

#pragma GCC diagnostic pop

	return true;
}


/*****************************************************************
 * Prepare information for data Loggers                          *
 *****************************************************************/
static void createLoggerConfigs()
{
#if defined(ESP8266)
	auto new_session = []()
	{ return new BearSSL::Session; };
#else
	auto new_session = []()
	{ return nullptr; };
#endif

	if (cfg::send2dusti)
	{
		loggerConfigs[LoggerSensorCommunity].destport = 80;
		if (cfg::ssl_dusti)
		{
			loggerConfigs[LoggerSensorCommunity].destport = 443;
			loggerConfigs[LoggerSensorCommunity].session = new_session();
		}
	}

	loggerConfigs[LoggerMadavi].destport = PORT_MADAVI;
	if (cfg::send2madavi && cfg::ssl_madavi)
	{
		loggerConfigs[LoggerMadavi].destport = 443;
		loggerConfigs[LoggerMadavi].session = new_session();
	}

	loggerConfigs[LoggerSensemap].destport = PORT_SENSEMAP;
	loggerConfigs[LoggerSensemap].session = new_session();
	loggerConfigs[LoggerFSapp].destport = PORT_FSAPP;
	loggerConfigs[Loggeraircms].destport = PORT_AIRCMS;
	loggerConfigs[LoggerInflux].destport = cfg::port_influx;

	if (cfg::send2influx && cfg::ssl_influx)
	{
		loggerConfigs[LoggerInflux].session = new_session();
	}

	loggerConfigs[LoggerCustom].destport = cfg::port_custom;
	if (cfg::send2custom && (cfg::ssl_custom || (cfg::port_custom == 443)))
	{
		loggerConfigs[LoggerCustom].session = new_session();
	}
}

/*****************************************************************
 * dew point helper function                                     *
 *****************************************************************/
static float dew_point(const float temperature, const float humidity)
{
	float dew_temp;
	const float k2 = 17.62;
	const float k3 = 243.12;

	dew_temp = k3 * (((k2 * temperature) / (k3 + temperature)) + log(humidity / 100.0f)) / (((k2 * k3) / (k3 + temperature)) - log(humidity / 100.0f));

	return dew_temp;
}

/*****************************************************************
 * dew point helper function                                     *
 *****************************************************************/
static float pressure_at_sealevel(const float temperature, const float pressure)
{
	float pressure_at_sealevel;

	pressure_at_sealevel = pressure * pow(((temperature + 273.15f) / (temperature + 273.15f + (0.0065f * readCorrectionOffset(cfg::height_above_sealevel)))), -5.255f);

	return pressure_at_sealevel;
}

/*****************************************************************
 * start html helper functions                                   *
 *****************************************************************/
static void start_html_page(String &page_content, const String &title)
{
	last_page_load = millis();

	RESERVE_STRING(s, LARGE_STR);
	s = FPSTR(WEB_PAGE_HEADER);
	s.replace("{t}", title);
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), s);

	server.sendContent_P(WEB_PAGE_HEADER_HEAD);

	s = FPSTR(WEB_PAGE_HEADER_BODY);
	s.replace("{t}", title);
	
	if (title != " ")
	{
		s.replace("{n}", F("&raquo;"));
	}
	else
	{
		s.replace("{n}", emptyString);
	}

	s.replace("{id}", esp_chipid);
	s.replace("{macid}", esp_mac_id);
	s.replace("{mac}", WiFi.macAddress());
	page_content += s;
}

/*****************************************************************
 * end html helper functions                                     *
 *****************************************************************/
static void end_html_page(String &page_content)
{
	if (page_content.length())
	{
		server.sendContent(page_content);
	}
	
	server.sendContent_P(WEB_PAGE_FOOTER);
}

/*****************************************************************
 * add html helper functions                                     *
 *****************************************************************/
static void add_form_input( String &page_content, 
							const ConfigShapeId cfgid, 
							const __FlashStringHelper *info, 
							const int length)
{
	RESERVE_STRING(s, MED_STR);

	s = F("<tr>"
		  "<td title='[&lt;= {l}]'>{i}:&nbsp;</td>"
		  "<td style='width:{l}em'>"
		  "<input type='{t}' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/>"
		  "</td></tr>");

	String t_value;
	ConfigShapeEntry c;
	memcpy_P(&c, &configShape[cfgid], sizeof(ConfigShapeEntry));

	switch (c.cfg_type)
	{
	case Config_Type_UInt:
		t_value = String(*c.cfg_val.as_uint);
		s.replace("{t}", F("number"));
		break;

	case Config_Type_Time:
		t_value = String((*c.cfg_val.as_uint) / 1000);
		s.replace("{t}", F("number"));
		break;

	default:
		if (c.cfg_type == Config_Type_Password)
		{
			s.replace("{t}", F("password"));
			info = FPSTR(INTL_PASSWORD);
		}
		else
		{
			t_value = c.cfg_val.as_str;
			t_value.replace("'", "&#39;");
			s.replace("{t}", F("text"));
		}
	}

	s.replace("{i}", info);
	s.replace("{n}", String(c.cfg_key()));
	s.replace("{v}", t_value);
	s.replace("{l}", String(length));
	page_content += s;
}

/*
  return value:
			str = fill webpage value.
*/
static String form_checkbox(const ConfigShapeId cfgid, const String &info, const bool linebreak)
{
	RESERVE_STRING(s, MED_STR);

	s = F("<label for='{n}'>"
		  "<input type='checkbox' name='{n}' value='1' id='{n}' {c}/>"
		  "<input type='hidden' name='{n}' value='0'/>"
		  "{i}</label><br/>");

	if (*configShape[cfgid].cfg_val.as_bool)
	{
		s.replace("{c}", F(" checked='checked'"));
	}
	else
	{
		s.replace("{c}", emptyString);
	};

	s.replace("{i}", info);
	s.replace("{n}", String(configShape[cfgid].cfg_key()));

	if (!linebreak)
	{
		s.replace("<br/>", emptyString);
	}

	return s;
}


/*
  Input:
		value => pointer to HTTP webpage memory.
*/
static String form_submit(const String &value)
{
	String s = F("<tr>"
				 "<td>&nbsp;</td>"
				 "<td>"
				 "<input type='submit' name='submit' value='{v}' />"
				 "</td>"
				 "</tr>");

	s.replace("{v}", value);
	
	return s;
}

// Disable Firmware opties (FvD)
/*
static String form_select_lang()
{
	String s_select = F(" selected='selected'");
	String s = F("<tr>"
				 "<td>" INTL_LANGUAGE ":&nbsp;</td>"
				 "<td>"
				 "<select id='current_lang' name='current_lang'>"
				 "<option value='BG'>Bulgarian (BG)</option>"
				 "<option value='CN'>中文 (CN)</option>"
				 "<option value='CZ'>Český (CZ)</option>"
				 "<option value='DE'>Deutsch (DE)</option>"
				 "<option value='DK'>Dansk (DK)</option>"
				 "<option value='EE'>Eesti keel (EE)</option>"
				 "<option value='EN'>English (EN)</option>"
				 "<option value='ES'>Español (ES)</option>"
				 "<option value='FR'>Français (FR)</option>"
				 "<option value='GR'>Ελληνικά (GR)</option>"
				 "<option value='IT'>Italiano (IT)</option>"
				 "<option value='JP'>日本語 (JP)</option>"
				 "<option value='LT'>Lietuvių kalba (LT)</option>"
				 "<option value='LU'>Lëtzebuergesch (LU)</option>"
				 "<option value='LV'>Latviešu valoda (LV)</option>"
				 "<option value='NL'>Nederlands (NL)</option>"
				 "<option value='HU'>Magyar (HU)</option>"
				 "<option value='PL'>Polski (PL)</option>"
				 "<option value='PT'>Português (PT)</option>"
				 "<option value='RO'>Română (RO)</option>"
				 "<option value='RS'>Srpski (RS)</option>"
				 "<option value='RU'>Русский (RU)</option>"
				 "<option value='SI'>Slovenščina (SI)</option>"
				 "<option value='SK'>Slovák (SK)</option>"
				 "<option value='SE'>Svenska (SE)</option>"
				 "<option value='TR'>Türkçe (TR)</option>"
				 "<option value='UA'>український (UA)</option>"
				 "</select>"
				 "</td>"
				 "</tr>");

	s.replace("'" + String(cfg::current_lang) + "'>", "'" + String(cfg::current_lang) + "'" + s_select + ">");
	return s;
}
*/

/*
  Input:
		page_content => pointer to HTTP webpage memory.
*/
static void add_warning_first_cycle(String &page_content)
{
	String s = FPSTR(INTL_TIME_TO_FIRST_MEASUREMENT);
	unsigned int time_to_first = cfg::sending_intervall_ms - msSince(starttime);

	if (time_to_first > cfg::sending_intervall_ms)
	{
		time_to_first = 0;
	}

	s.replace("{v}", String(((time_to_first + 500) / 1000)));
	page_content += s;
}


/*
  Input:
		sourceStr => pointer to HTTP webpage memory.
*/
static void add_age_last_values(String &sourceStr)
{
	sourceStr += "<b>";
	unsigned int time_since_last = msSince(starttime);
	if (time_since_last > cfg::sending_intervall_ms)
	{
		time_since_last = 0;
	}
	
	time_t now = time(nullptr);

	sourceStr += String((time_since_last + 500) / 1000);
	sourceStr += FPSTR(INTL_TIME_SINCE_LAST_MEASUREMENT);
	sourceStr += "<br/><br/>";
	sourceStr += FPSTR(INTL_TIME_UTC);
	sourceStr += "&nbsp;";
	sourceStr += String(ctime(&now));

	sourceStr += FPSTR(WEB_B_BR_BR);
}

/*****************************************************************
 * Webserver request auth: prompt for BasicAuth
 *
 * -Provide BasicAuth for all page contexts except /values and images
 *****************************************************************/
static bool webserver_request_auth()
{
	if (cfg::www_basicauth_enabled && !wificonfig_loop)
	{
		debug_outln_info(F("validate request auth..."));
		
		if (!server.authenticate(cfg::www_username, cfg::www_password))
		{
			server.requestAuthentication(BASIC_AUTH, "Sensor Login", F("Authentication failed"));
			return false;
		}
	}

	return true;
}

/************************************
 * resend http page again to client *
*************************************/
static void sendHttpRedirect() 
{
	const IPAddress defaultIP(
								default_ip_first_octet, 
								default_ip_second_octet, 
								default_ip_third_octet, 
								default_ip_fourth_octet
							 );

	String defaultAddress = F("http://") + defaultIP.toString() + F("/config");
	server.sendHeader(F("Location"), defaultAddress);
	server.send(302, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), emptyString);
}

/*****************************************************************
 * Webserver root: show all options                              *
 *****************************************************************/
static void webserver_root()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		sendHttpRedirect();
	}
	else
	{
		if (!webserver_request_auth())
		{
			return;
		}

		RESERVE_STRING(page_content, XLARGE_STR);
		start_html_page(page_content, emptyString);
		debug_outln_info(F("ws: root ..."));

		// Enable Pagination
		if (cfg::has_s7000)
		{
		 	page_content += FPSTR(WEB_ROOT_PAGE_CONTENT_S7000);
		}
		else
		{
		 	page_content += FPSTR(WEB_ROOT_PAGE_CONTENT);	
		}

		page_content.replace(F("{t}"), FPSTR(INTL_CURRENT_DATA));
		page_content.replace(F("{s}"), FPSTR(INTL_DEVICE_STATUS));
		page_content.replace(F("{conf}"), FPSTR(INTL_CONFIGURATION));
		page_content.replace(F("{s7000}"), FPSTR(INTL_SIM7000_CONFIGURATION));
		page_content.replace(F("{restart}"), FPSTR(INTL_RESTART_SENSOR));
		page_content.replace(F("{debug}"), FPSTR(INTL_DEBUG_LEVEL));
		
		end_html_page(page_content);
	}
}

/*****************************************************************
 * Webserver config: show config page                            *
 *****************************************************************/
static void webserver_config_send_body_get(String &page_content)
{
	auto add_form_checkbox = [&page_content](const ConfigShapeId cfgid, const String &info)
	{
		page_content += form_checkbox(cfgid, info, true);
	};

	auto add_form_checkbox_sensor = [&add_form_checkbox](const ConfigShapeId cfgid, __const __FlashStringHelper *info)
	{
		add_form_checkbox(cfgid, add_sensor_type(info));
	};

	debug_outln_info(F("begin webserver_config_body_get ..."));

	page_content += F("<form method='POST' action='/config' style='width:100%;'>\n"
					  "<input class='radio' id='r1' name='group' type='radio' checked>"
					  "<input class='radio' id='r2' name='group' type='radio'>"
					  "<input class='radio' id='r3' name='group' type='radio'>"
					  "<input class='radio' id='r4' name='group' type='radio'>"
					  "<div class='tabs'>"
					  "<label class='tab' id='tab1' for='r1'>" INTL_WIFI_SETTINGS "</label>"
					  "<label class='tab' id='tab2' for='r2'>");
	page_content += FPSTR(INTL_MORE_SETTINGS);
	page_content += F("</label>"
					  "<label class='tab' id='tab3' for='r3'>");
	page_content += FPSTR(INTL_SENSORS);
	page_content += F("</label>"
					  "<label class='tab' id='tab4' for='r4'>APIs"
					  "</label></div><div class='panels'>"
					  "<div class='panel' id='panel1'>");

	if (wificonfig_loop)
	{ // scan for wlan ssids
		page_content += F("<div id='wifilist'>" INTL_WIFI_NETWORKS "</div><br/>");
	}

	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_checkbox(Config_has_morewifi, FPSTR(INTL_ENABLE_MOREWIFI));
	add_form_input(page_content, Config_wlanssid, FPSTR(INTL_FS_WIFI_NAME), LEN_WLANSSID - 1);
	add_form_input(page_content, Config_wlanpwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);

	if (cfg::has_morewifi)
	{
		add_form_input(page_content, Config_wlanssid_2, FPSTR(INTL_FS_WIFI_NAME_2), LEN_WLANSSID - 1);
		add_form_input(page_content, Config_wlanpwd_2, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
		add_form_input(page_content, Config_wlanssid_3, FPSTR(INTL_FS_WIFI_NAME_3), LEN_WLANSSID - 1);
		add_form_input(page_content, Config_wlanpwd_3, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
	}

	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(WEB_BR_LF_B);
	page_content += F("<hr/>");


	page_content += FPSTR(INTL_AB_HIER_NUR_ANDERN);
	page_content += FPSTR(WEB_B_BR);
	page_content += FPSTR(BR_TAG);

	// Paginate page after ~ 1500 Bytes
	server.sendContent(page_content);
	page_content = emptyString;

	add_form_checkbox(Config_www_basicauth_enabled, FPSTR(INTL_BASICAUTH));
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_www_username, FPSTR(INTL_USER), LEN_WWW_USERNAME - 1);
	add_form_input(page_content, Config_www_password, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(BR_TAG);

	// Paginate page after ~ 1500 Bytes
	server.sendContent(page_content);

	if (!wificonfig_loop)
	{
		page_content = FPSTR(INTL_FS_WIFI_DESCRIPTION);
		page_content += FPSTR(BR_TAG);

		page_content += FPSTR(TABLE_TAG_OPEN);
		add_form_input(page_content, Config_fs_ssid, FPSTR(INTL_FS_WIFI_NAME), LEN_FS_SSID - 1);
		add_form_input(page_content, Config_fs_pwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
		page_content += FPSTR(TABLE_TAG_CLOSE_BR);

		// Paginate page after ~ 1500 Bytes
		server.sendContent(page_content);
	}

	// Add IP static (FVD)
	// add checkbox

	page_content = FPSTR(WEB_BR_LF);
	page_content += F("<hr/>");
	page_content += FPSTR(WEB_BR_LF);
	add_form_checkbox(Config_has_s7000, FPSTR(INTL_ENABLE_S7000));
	page_content += FPSTR(WEB_BR_LF);
	add_form_checkbox(Config_has_radarmotion, FPSTR(INTL_ENABLE_RCWL_0516));
	//page_content += FPSTR(WEB_BR_LF_B);

	if (cfg::has_radarmotion)
	{
		page_content += FPSTR(TABLE_TAG_OPEN);
		add_form_input(page_content, Config_host_radar, FPSTR(INTL_SERVER), LEN_HOST_CUSTOM - 1);
		add_form_input(page_content, Config_port_radar, FPSTR(INTL_PORT), MAX_PORT_DIGITS);
		add_form_input(page_content, Config_motion_wait_time, FPSTR(INTL_MOTION_WAIT_TIME), MAX_PORT_DIGITS);
		add_form_input(page_content, Config_user_radar, FPSTR(INTL_USER), LEN_USER_CUSTOM - 1);
		add_form_input(page_content, Config_pwd_radar,  FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
		page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	}

	page_content += FPSTR(WEB_BR_LF_B);
	page_content += F("<hr/>");

	add_form_checkbox(Config_has_fix_ip, FPSTR(INTL_STATIC_IP_TEXT));
	page_content += FPSTR(TABLE_TAG_OPEN);

	add_form_input(page_content, Config_static_ip, FPSTR(INTL_STATIC_IP), 15);
	add_form_input(page_content, Config_static_subnet, FPSTR(INTL_STATIC_SUBNET), 15);
	add_form_input(page_content, Config_static_gateway, FPSTR(INTL_STATIC_GATEWAY), 15);
	add_form_input(page_content, Config_static_dns, FPSTR(INTL_STATIC_DNS), 15);

	page_content += FPSTR(TABLE_TAG_CLOSE_BR);

	server.sendContent(page_content);

	page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(2));

	add_form_checkbox(Config_has_display, FPSTR(INTL_DISPLAY));
	add_form_checkbox(Config_has_sh1106, FPSTR(INTL_SH1106));
	add_form_checkbox(Config_has_flipped_display, FPSTR(INTL_FLIP_DISPLAY));
	add_form_checkbox(Config_has_lcd1602_27, FPSTR(INTL_LCD1602_27));
	add_form_checkbox(Config_has_lcd1602, FPSTR(INTL_LCD1602_3F));

	// Paginate page after ~ 1500 Bytes
	server.sendContent(page_content);
	page_content = emptyString;

	add_form_checkbox(Config_has_lcd2004_27, FPSTR(INTL_LCD2004_27));
	add_form_checkbox(Config_has_lcd2004, FPSTR(INTL_LCD2004_3F));
	add_form_checkbox(Config_display_wifi_info, FPSTR(INTL_DISPLAY_WIFI_INFO));
	add_form_checkbox(Config_display_device_info, FPSTR(INTL_DISPLAY_DEVICE_INFO));
	page_content += FPSTR(WEB_BR_LF);

	server.sendContent(page_content);
	page_content = FPSTR(WEB_BR_LF);

	page_content += F("<hr/>");
	page_content += FPSTR(INTL_AB_HIER_NUR_ANDERN);
	page_content += FPSTR(WEB_B_BR);
	page_content += FPSTR(BR_TAG);

	add_form_checkbox(Config_powersave, FPSTR(INTL_POWERSAVE));
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_debug, FPSTR(INTL_DEBUG_LEVEL), 1);
	add_form_input(page_content, Config_sending_intervall_ms, FPSTR(INTL_MEASUREMENT_INTERVAL), 5);
	add_form_input(page_content, Config_time_for_wifi_config, FPSTR(INTL_DURATION_ROUTER_MODE), 5);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);

	server.sendContent(page_content);

	page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(3));

	add_form_checkbox_sensor(Config_sen5x_read, FPSTR(INTL_SEN5X));
	page_content += FPSTR(WEB_NBSP_NBSP);
	add_form_checkbox_sensor(Config_sen5x_on, FPSTR(INTL_SEN5X_ON));
	page_content += FPSTR(TABLE_TAG_OPEN);

	page_content += form_select_mode_SEN5PM();
	page_content += form_select_mode_SEN5TH();

	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += F("<hr/>");
	page_content += FPSTR(WEB_BR_LF);

	add_form_checkbox_sensor(Config_sds_read, FPSTR(INTL_SDS011));
	add_form_checkbox_sensor(Config_sps30_read, FPSTR(INTL_SPS30));
	add_form_checkbox_sensor(Config_hpm_read, FPSTR(INTL_HPM));


	// Paginate page after ~ 1500 Bytes
	server.sendContent(page_content);
	page_content = emptyString;
	add_form_checkbox_sensor(Config_pms_read, FPSTR(INTL_PMS));
	add_form_checkbox_sensor(Config_npm_read, FPSTR(INTL_NPM));
	add_form_checkbox_sensor(Config_npm_fulltime, FPSTR(INTL_NPM_FULLTIME));
	add_form_checkbox_sensor(Config_ips_read, FPSTR(INTL_IPS));
	page_content += FPSTR(WEB_BR_LF);
	page_content += F("<hr/>");
	page_content += FPSTR(WEB_BR_LF);

	add_form_checkbox_sensor(Config_scd30_read, FPSTR(INTL_SCD30));
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, ConfigShapeId::Config_scd30_temp_correction, FPSTR(INTL_TEMP_CORRECTION), LEN_TEMP_CORRECTION - 1);
	add_form_input(page_content, ConfigShapeId::Config_scd30_co2_correction, FPSTR(INTL_SCD30_CO2_CORRECTION), LEN_DNMS_CORRECTION - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);

	// Paginate page after ~ 1500 Bytes
	server.sendContent(page_content);
	page_content = emptyString;
	page_content += FPSTR(WEB_BR_LF);
	add_form_checkbox_sensor(Config_dnms_read, FPSTR(INTL_DNMS));
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_dnms_correction, FPSTR(INTL_DNMS_CORRECTION), LEN_DNMS_CORRECTION - 1);
	add_form_input(page_content, Config_temp_correction, FPSTR(INTL_TEMP_CORRECTION), LEN_TEMP_CORRECTION - 1);
	add_form_input(page_content, Config_height_above_sealevel, FPSTR(INTL_HEIGHT_ABOVE_SEALEVEL), LEN_HEIGHT_ABOVE_SEALEVEL - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(WEB_BR_LF);
	add_form_checkbox(Config_gps_read, FPSTR(INTL_NEO6M));

	page_content += FPSTR(WEB_BR_LF_B);
	page_content += FPSTR(INTL_MORE_TEMP_SENSORS);
	page_content += F("<hr/>");
	page_content += FPSTR(WEB_B_BR);

	// More Sensors on web page:
	add_form_checkbox_sensor(Config_bmx280_read, FPSTR(INTL_BMX280));
	add_form_checkbox_sensor(Config_ds18b20_read, FPSTR(INTL_DS18B20));
	add_form_checkbox_sensor(Config_bmp_read, FPSTR(INTL_BMP180));
	add_form_checkbox_sensor(Config_dht_read, FPSTR(INTL_DHT22));
	add_form_checkbox_sensor(Config_htu21d_read, FPSTR(INTL_HTU21D));
	add_form_checkbox_sensor(Config_sht3x_read, FPSTR(INTL_SHT3X));


	// Paginate page after ~ 1500 Bytes
	server.sendContent(page_content);

	page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(4));

	page_content += tmpl(FPSTR(INTL_SEND_TO), F("APIs"));
	page_content += FPSTR(BR_TAG);
	page_content += form_checkbox(Config_send2dusti, FPSTR(WEB_SENSORCOMMUNITY), false);
	page_content += FPSTR(WEB_NBSP_NBSP_BRACE);
	page_content += form_checkbox(Config_ssl_dusti, FPSTR(WEB_HTTPS), false);
	page_content += FPSTR(WEB_BRACE_BR);
	page_content += form_checkbox(Config_send2madavi, FPSTR(WEB_MADAVI), false);
	page_content += FPSTR(WEB_NBSP_NBSP_BRACE);
	page_content += form_checkbox(Config_ssl_madavi, FPSTR(WEB_HTTPS), false);
	page_content += FPSTR(WEB_BRACE_BR);
	add_form_checkbox(Config_send2csv, FPSTR(WEB_CSV));
	add_form_checkbox(Config_send2fsapp, FPSTR(WEB_FEINSTAUB_APP));
	add_form_checkbox(Config_send2aircms, FPSTR(WEB_AIRCMS));
	add_form_checkbox(Config_send2sensemap, FPSTR(WEB_OPENSENSEMAP));
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_senseboxid, F("senseBox&nbsp;ID"), LEN_SENSEBOXID - 1);

	server.sendContent(page_content);
	page_content = FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(BR_TAG);
	page_content += F("<hr/>");
	page_content += form_checkbox(Config_send2custom, FPSTR(INTL_SEND_TO_OWN_API), false);
	page_content += FPSTR(WEB_NBSP_NBSP_BRACE);
	page_content += form_checkbox(Config_ssl_custom, FPSTR(WEB_HTTPS), false);
	page_content += FPSTR(WEB_BRACE_BR);

	server.sendContent(page_content);
	page_content = FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_host_custom, FPSTR(INTL_SERVER), LEN_HOST_CUSTOM - 1);
	add_form_input(page_content, Config_url_custom, FPSTR(INTL_PATH), LEN_URL_CUSTOM - 1);
	add_form_input(page_content, Config_port_custom, FPSTR(INTL_PORT), MAX_PORT_DIGITS);
	add_form_input(page_content, Config_user_custom, FPSTR(INTL_USER), LEN_USER_CUSTOM - 1);
	add_form_input(page_content, Config_pwd_custom, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	//page_content += FPSTR(BR_TAG);
	
	server.sendContent(page_content);

	// Add MQTT
	// Test page_content = emptyString;
	page_content = emptyString;
	page_content += FPSTR(BR_TAG);
	page_content += F("<hr/>");
	page_content += form_checkbox(Config_send2mqtt, FPSTR(INTL_SEND_TO_MQTT), false);
	page_content += FPSTR(BR_TAG);
	
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_mqtt_server, FPSTR(INTL_SERVER), LEN_HOST_CUSTOM - 1);
	add_form_input(page_content, Config_mqtt_topic, FPSTR(INTL_TOPIC), LEN_URL_CUSTOM - 1);
	add_form_input(page_content, Config_mqtt_port, FPSTR(INTL_PORT), MAX_PORT_DIGITS);
	add_form_input(page_content, Config_mqtt_user, FPSTR(INTL_USER), LEN_USER_CUSTOM - 1);
	add_form_input(page_content, Config_mqtt_pwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(BR_TAG);
	page_content += F("<hr/>");
	server.sendContent(page_content);
	// End MQTT

	page_content = form_checkbox(Config_send2influx, tmpl(FPSTR(INTL_SEND_TO), F("InfluxDB")), false);

	page_content += FPSTR(WEB_NBSP_NBSP_BRACE);
	page_content += form_checkbox(Config_ssl_influx, FPSTR(WEB_HTTPS), false);
	page_content += FPSTR(WEB_BRACE_BR);
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_host_influx, FPSTR(INTL_SERVER), LEN_HOST_INFLUX - 1);
	add_form_input(page_content, Config_url_influx, FPSTR(INTL_PATH), LEN_URL_INFLUX - 1);
	add_form_input(page_content, Config_port_influx, FPSTR(INTL_PORT), MAX_PORT_DIGITS);
	add_form_input(page_content, Config_user_influx, FPSTR(INTL_USER), LEN_USER_INFLUX - 1);
	add_form_input(page_content, Config_pwd_influx, FPSTR(INTL_PASSWORD), LEN_PASS_INFLUX - 1);
	add_form_input(page_content, Config_measurement_name_influx, FPSTR(INTL_MEASUREMENT), LEN_MEASUREMENT_NAME_INFLUX - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += F("</div></div>");
	page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
	page_content += FPSTR(BR_TAG);
	page_content += FPSTR(WEB_BR_FORM);

	if (wificonfig_loop)
	{ // scan for wlan ssids
		page_content += F("<script>window.setTimeout(load_wifi_list,1000);</script>");
	}

	server.sendContent(page_content);

	page_content = emptyString;
}

/*****************************************************************************
 * Webserver config: post the changed page to config file and restart appl.   *
 *****************************************************************************/
static void webserver_config_send_body_post(String &page_content)
{
	String masked_pwd;

	for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e)
	{
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		const String s_param(c.cfg_key());

		if (!server.hasArg(s_param))
		{
			continue;
		}

		const String server_arg(server.arg(s_param));

		switch (c.cfg_type)
		{
		case Config_Type_UInt:
			*(c.cfg_val.as_uint) = server_arg.toInt();
			break;

		case Config_Type_Time:
			*(c.cfg_val.as_uint) = server_arg.toInt() * 1000;
			break;

		case Config_Type_Bool:
			*(c.cfg_val.as_bool) = (server_arg == "1");
			break;

		case Config_Type_String:
			strncpy(c.cfg_val.as_str, server_arg.c_str(), c.cfg_len);
			c.cfg_val.as_str[c.cfg_len] = '\0';
			break;

		case Config_Type_Password:
			if (server_arg.length())
			{
				server_arg.toCharArray(c.cfg_val.as_str, LEN_CFG_PASSWORD);
			}
			break;
		}
	}

	page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);

	server.sendContent(page_content);
	page_content = emptyString;
}

/*****************************************************************
 * Webserver sim7000 config: show sim7000 config page            *
 *****************************************************************/
static void webserver_config_send_body_get7(String &page_content)
{
	// auto add_form_checkbox7 = [&page_content](const ConfigShape7Id cfgid, const String &info)
	// {
	// 	page_content += form_checkbox7(cfgid, info, true);
	// };

	// auto add_form_checkbox_sensor7 = [&add_form_checkbox7](const ConfigShape7Id cfgid, __const __FlashStringHelper *info)
	// {
	// 	add_form_checkbox7(cfgid, add_sensor_type(info));
	// };

	debug_outln_info(F("begin webserver_simm7000_body_get ..."));
	debug_outln_info(F("SIM7000 enable: "), cfg::has_s7000);

	page_content += F("<form method='POST' action='/s7000' style='width:100%;'>\n");
	page_content += FPSTR(WEB_BR_BR);
	page_content += FPSTR(TABLE_TAG_OPEN);
	// add_form_input7(page_content, Config7000_mode, FPSTR(INTL_SIM_MODE), LEN_SIMM7000 - 1);
	add_form_input7(page_content, Config7000_apn, FPSTR(INTL_SIM_APN), LEN_SIMM7000 - 1);
	add_form_input7(page_content, Config7000_type, FPSTR(INTL_SIM_TYPE), LEN_SIMM7000 - 1);
	page_content += form_select_mode7();
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(WEB_BR_BR);
	page_content += form_checkbox7(Config7000_has_gps, FPSTR(INTL_SIM_GPS), false);
	page_content += F("</div></div>");
	page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
	page_content += FPSTR(WEB_BR_FORM);

	debug_outln_info(F("End webserver_simm7000 ..."));

	server.sendContent(page_content);

	page_content = emptyString;
}

/*********************************************************************************************
 * Webserver sim7000 config: post the changed page to sim7000 config file and restart appl.  *
 *********************************************************************************************/
static void webserver_config_send_body_post7(String &page_content)
{
	String masked_pwd;

	for (unsigned e = 0; e < sizeof(configShape7) / sizeof(configShape7[0]); ++e)
	{
		Config7000ShapeEntry c;
		memcpy_P(&c, &configShape7[e], sizeof(Config7000ShapeEntry));
		const String s_param(c.cfg_key());

		if (!server.hasArg(s_param))
		{
			continue;
		}

		const String server_arg(server.arg(s_param));

		switch (c.cfg_type)
		{
		case Config7_Type_UInt:
			*(c.cfg_val.as_uint) = server_arg.toInt();
			break;

		case Config7_Type_Bool:
			*(c.cfg_val.as_bool) = (server_arg == "1");
			break;

		case Config7_Type_String:
			strncpy(c.cfg_val.as_str, server_arg.c_str(), c.cfg_len);
			c.cfg_val.as_str[c.cfg_len] = '\0';
			break;

	/*	case Config7_Type_Password:
			if (server_arg.length())
			{
				server_arg.toCharArray(c.cfg_val.as_str, LEN_CFG_PASSWORD);
			}
			break;
	*/
		}

	}

	page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);

	server.sendContent(page_content);
	page_content = emptyString;
}

/****************************************************
 *	Write webside settings into Config file. 		*
*****************************************************/
static void webserver_config()
{
	if (!webserver_request_auth())
	{
		return;
	}

	debug_outln_info(F("ws: config page ..."));

	server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));
	// Enable Pagination (Chunked Transfer)
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);

	RESERVE_STRING(page_content, XLARGE_STR);

	start_html_page(page_content, FPSTR(INTL_CONFIGURATION));

	if (wificonfig_loop)
	{ // scan for wlan ssids
		page_content += FPSTR(WEB_CONFIG_SCRIPT);
	}

	if (server.method() == HTTP_GET)
	{
		webserver_config_send_body_get(page_content);
	}
	else
	{
		webserver_config_send_body_post(page_content);
	}

	end_html_page(page_content);

	if (server.method() == HTTP_POST)
	{
		display_debug(F("Writing config"), emptyString);

		if (cfg::has_radarmotion)
		{
			debug_outln_info(F("STOP Radar motion sensor (RCWL_0516) process."));
			RCWL0516.end();
		}

		if (writeConfig())
		{ // TODO: devide in two section to know which writeconfig has a error.
			display_debug(F("Writing config"), F("and restarting"));
			sensor_restart();
		}
		else
		{
			display_debug(F("ERROR Writing config"), F("For restart Power OFF/ON"));
		}
	}
}

/*
	Write webside settings into ConfigS7000 file.
*/
static void webserver_config7()
{
	if (!webserver_request_auth())
	{
		return;
	}

	debug_outln_info(F("ws: config page S7000..."));

	server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));
	// Enable Pagination (Chunked Transfer)
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);

	RESERVE_STRING(page_content, XLARGE_STR);

	start_html_page(page_content, FPSTR(INTL_CONFIGURATION));

	if (wificonfig_loop)
	{ // scan for wlan ssids
		page_content += FPSTR(WEB_CONFIG_SCRIPT);
	}

	if (server.method() == HTTP_GET)
	{
		debug_outln_info(F("HTTP_GET"));
		webserver_config_send_body_get7(page_content);

	}
	else
	{
		debug_outln_info(F("HTTP_POST7"));
		webserver_config_send_body_post7(page_content);
	}

	end_html_page(page_content);

	if (server.method() == HTTP_POST)
	{
		display_debug(F("Writing config"), emptyString);

		if (cfg::has_radarmotion)
		{
			debug_outln_info(F("STOP Radar motion sensor (RCWL_0516) process."));
			RCWL0516.end();
		}

		if (writeConfig())
		{ // TODO: devide in two section to know which writeconfig has a error.
			display_debug(F("Writing config"), F("and restarting"));
			sensor_restart();
		}
		else
		{
			display_debug(F("ERROR Writing config"), F("For restart Power OFF/ON"));
		}
	}
}
/*

*/
static void sensor_restart()
{
#if defined(ESP8266)
	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
	delay(100);
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

	SPIFFS.end();

#pragma GCC diagnostic pop

	if (cfg::npm_read)
	{
		serialNPM.end();
	}
	else
	{
		serialSDS.end();
	}

	if (cfg::has_radarmotion)
	{// Stop Radar motion Event process.
		RCWL0516.end();
	}

	debug_outln_info(F("Restart."));
	delay(500);

	ESP.restart();

	// should not be reached, forever loop.
	while (true)
	{
		yield();
	}
}

/*****************************************************************
 * Webserver wifi: show available wifi networks                  *
 *****************************************************************/
static void webserver_wifi()
{
	String page_content;

	debug_outln_info(F("wifi networks found: "), String(count_wifiInfo));
	if (count_wifiInfo == 0)
	{
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(INTL_NO_NETWORKS);
		page_content += FPSTR(BR_TAG);
	}
	else
	{
		std::unique_ptr<int[]> indices(new int[count_wifiInfo]);
		debug_outln_info(F("ws: wifi ..."));
		for (unsigned i = 0; i < count_wifiInfo; ++i)
		{
			indices[i] = i;
		}
		for (unsigned i = 0; i < count_wifiInfo; i++)
		{
			for (unsigned j = i + 1; j < count_wifiInfo; j++)
			{
				if (wifiInfo[indices[j]].RSSI > wifiInfo[indices[i]].RSSI)
				{
					std::swap(indices[i], indices[j]);
				}
			}
		}
		int duplicateSsids = 0;
		for (int i = 0; i < count_wifiInfo; i++)
		{
			if (indices[i] == -1)
			{
				continue;
			}
			for (int j = i + 1; j < count_wifiInfo; j++)
			{
				if (strncmp(wifiInfo[indices[i]].ssid, wifiInfo[indices[j]].ssid, sizeof(wifiInfo[0].ssid)) == 0)
				{
					indices[j] = -1; // set dup aps to index -1
					++duplicateSsids;
				}
			}
		}

		page_content += FPSTR(INTL_NETWORKS_FOUND);
		page_content += String(count_wifiInfo - duplicateSsids);
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(TABLE_TAG_OPEN);
		//if (n > 30) n=30;
		for (int i = 0; i < count_wifiInfo; ++i)
		{
			if (indices[i] == -1
#if defined(ESP8266)
				|| wifiInfo[indices[i]].isHidden
#endif
			)
			{
				continue;
			}
			// Print SSID and RSSI for each network found
#if defined(ESP8266)
			page_content += wlan_ssid_to_table_row(wifiInfo[indices[i]].ssid, ((wifiInfo[indices[i]].encryptionType == ENC_TYPE_NONE) ? " " : u8"🔒"), wifiInfo[indices[i]].RSSI);
#endif
#if defined(ESP32)
			page_content += wlan_ssid_to_table_row(wifiInfo[indices[i]].ssid, ((wifiInfo[indices[i]].encryptionType == WIFI_AUTH_OPEN) ? " " : u8"🔒"), wifiInfo[indices[i]].RSSI);
#endif
		}
		page_content += FPSTR(TABLE_TAG_CLOSE_BR);
		page_content += FPSTR(BR_TAG);
	}
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

/*****************************************************************
 * Webserver root: Show current/latest values                    *
 *****************************************************************/
static void webserver_values()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		sendHttpRedirect();
		return;
	}

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_CURRENT_DATA));
	const String unit_Deg("°");
	const String unit_P("hPa");
	const String unit_T("°C");
	const String unit_CO2("ppm");
	const String unit_NC();
	const String unit_LA(F("dB(A)"));
	float dew_point_temp;

	const int signal_quality = calcWiFiSignalQuality(last_signal_strength);
	debug_outln_info(F("ws: values ..."));

	if (!count_sends)
	{
		page_content += F("<b style='color:red'>");
		add_warning_first_cycle(page_content);
		page_content += FPSTR(WEB_B_BR_BR);
	}
	else
	{
		add_age_last_values(page_content);
	}

	auto add_table_pm_value = [&page_content](const __FlashStringHelper *sensor, const __FlashStringHelper *param, const float &value)
	{
		add_table_row_from_value(page_content, sensor, param, check_display_value(value, -1, 1, 0), F("µg/m³"));
	};

	auto add_table_nc_value = [&page_content](const __FlashStringHelper *sensor, const __FlashStringHelper *param, const float value)
	{
		add_table_row_from_value(page_content, sensor, param, check_display_value(value, -1, 1, 0), F("#/cm³"));
	};

	auto add_table_t_value = [&page_content](const __FlashStringHelper *sensor, const __FlashStringHelper *param, const float value)
	{
		add_table_row_from_value(page_content, sensor, param, check_display_value(value, -128, 1, 0), "°C");
	};

	auto add_table_h_value = [&page_content](const __FlashStringHelper *sensor, const __FlashStringHelper *param, const float value)
	{
		add_table_row_from_value(page_content, sensor, param, check_display_value(value, -1, 1, 0), "%");
	};

	auto add_table_value = [&page_content](const __FlashStringHelper *sensor, const __FlashStringHelper *param, const String &value, const String &unit)
	{
		add_table_row_from_value(page_content, sensor, param, value, unit);
	};

	server.sendContent(page_content);
	page_content = F("<table cellspacing='0' cellpadding='5' class='v'>\n"
					 "<thead><tr><th>" INTL_SENSOR "</th><th> " INTL_PARAMETER "</th><th>" INTL_VALUE "</th></tr></thead>");

	if (cfg::ppd_read)
	{
		add_table_value(FPSTR(SENSORS_PPD42NS), FPSTR(WEB_PM1), check_display_value(last_value_PPD_P1, -1, 1, 0), FPSTR(INTL_PARTICLES_PER_LITER));
		add_table_value(FPSTR(SENSORS_PPD42NS), FPSTR(WEB_PM25), check_display_value(last_value_PPD_P2, -1, 1, 0), FPSTR(INTL_PARTICLES_PER_LITER));
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::sds_read)
	{
		add_table_pm_value(FPSTR(SENSORS_SDS011), FPSTR(WEB_PM25), last_value_SDS_P2);
		add_table_pm_value(FPSTR(SENSORS_SDS011), FPSTR(WEB_PM10), last_value_SDS_P1);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::pms_read)
	{
		add_table_pm_value(FPSTR(SENSORS_PMSx003), FPSTR(WEB_PM1), last_value_PMS_P0);
		add_table_pm_value(FPSTR(SENSORS_PMSx003), FPSTR(WEB_PM25), last_value_PMS_P2);
		add_table_pm_value(FPSTR(SENSORS_PMSx003), FPSTR(WEB_PM10), last_value_PMS_P1);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::hpm_read)
	{
		add_table_pm_value(FPSTR(SENSORS_HPM), FPSTR(WEB_PM25), last_value_HPM_P2);
		add_table_pm_value(FPSTR(SENSORS_HPM), FPSTR(WEB_PM10), last_value_HPM_P1);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::npm_read)
	{
		add_table_pm_value(FPSTR(SENSORS_NPM), FPSTR(WEB_PM1), last_value_NPM_P0);
		add_table_pm_value(FPSTR(SENSORS_NPM), FPSTR(WEB_PM25), last_value_NPM_P2);
		add_table_pm_value(FPSTR(SENSORS_NPM), FPSTR(WEB_PM10), last_value_NPM_P1);
		add_table_nc_value(FPSTR(SENSORS_NPM), FPSTR(WEB_NC1k0), last_value_NPM_N1);
		add_table_nc_value(FPSTR(SENSORS_NPM), FPSTR(WEB_NC2k5), last_value_NPM_N25);
		add_table_nc_value(FPSTR(SENSORS_NPM), FPSTR(WEB_NC10), last_value_NPM_N10);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::ips_read)
	{
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM01), last_value_IPS_P01);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM03), last_value_IPS_P03);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM05), last_value_IPS_P05);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM1), last_value_IPS_P0);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM25), last_value_IPS_P2);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM5), last_value_IPS_P5);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM10), last_value_IPS_P1);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC0k1), last_value_IPS_N01);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC0k3), last_value_IPS_N03);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC0k5), last_value_IPS_N05);	
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC1k0), last_value_IPS_N1);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC2k5), last_value_IPS_N25);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC5k0), last_value_IPS_N5);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC10), last_value_IPS_N10);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::sen5x_read)
	{
		// Debug.println((char*)SEN5X_type);
		// Debug.println(memcmp(SEN5X_type,"SEN50",6));

		if (memcmp(SEN5X_type, "SEN50", 6) == 0)
		{
			add_table_pm_value(FPSTR(SENSORS_SEN50), FPSTR(WEB_PM1), last_value_SEN5X_P0);
			add_table_pm_value(FPSTR(SENSORS_SEN50), FPSTR(WEB_PM25), last_value_SEN5X_P2);
			add_table_pm_value(FPSTR(SENSORS_SEN50), FPSTR(WEB_PM4), last_value_SEN5X_P4);
			add_table_pm_value(FPSTR(SENSORS_SEN50), FPSTR(WEB_PM10), last_value_SEN5X_P1);
			add_table_nc_value(FPSTR(SENSORS_SEN50), FPSTR(WEB_NC0k5), last_value_SEN5X_N05);
			add_table_nc_value(FPSTR(SENSORS_SEN50), FPSTR(WEB_NC1k0), last_value_SEN5X_N1);
			add_table_nc_value(FPSTR(SENSORS_SEN50), FPSTR(WEB_NC2k5), last_value_SEN5X_N25);
			add_table_nc_value(FPSTR(SENSORS_SEN50), FPSTR(WEB_NC4k0), last_value_SEN5X_N4);
			add_table_nc_value(FPSTR(SENSORS_SEN50), FPSTR(WEB_NC10), last_value_SEN5X_N10);
			add_table_value(FPSTR(SENSORS_SEN50), FPSTR(WEB_TPS), check_display_value(last_value_SEN5X_TS, -1, 1, 0), "µm");
			page_content += FPSTR(EMPTY_ROW);
		}

		if (memcmp(SEN5X_type, "SEN54", 6) == 0)
		{
			add_table_pm_value(FPSTR(SENSORS_SEN54), FPSTR(WEB_PM1), last_value_SEN5X_P0);
			add_table_pm_value(FPSTR(SENSORS_SEN54), FPSTR(WEB_PM25), last_value_SEN5X_P2);
			add_table_pm_value(FPSTR(SENSORS_SEN54), FPSTR(WEB_PM4), last_value_SEN5X_P4);
			add_table_pm_value(FPSTR(SENSORS_SEN54), FPSTR(WEB_PM10), last_value_SEN5X_P1);
			add_table_nc_value(FPSTR(SENSORS_SEN54), FPSTR(WEB_NC0k5), last_value_SEN5X_N05);
			add_table_nc_value(FPSTR(SENSORS_SEN54), FPSTR(WEB_NC1k0), last_value_SEN5X_N1);
			add_table_nc_value(FPSTR(SENSORS_SEN54), FPSTR(WEB_NC2k5), last_value_SEN5X_N25);
			add_table_nc_value(FPSTR(SENSORS_SEN54), FPSTR(WEB_NC4k0), last_value_SEN5X_N4);
			add_table_nc_value(FPSTR(SENSORS_SEN54), FPSTR(WEB_NC10), last_value_SEN5X_N10);
			add_table_value(FPSTR(SENSORS_SEN54), FPSTR(WEB_TPS), check_display_value(last_value_SEN5X_TS, -1, 1, 0), "µm");
			add_table_t_value(FPSTR(SENSORS_SEN54), FPSTR(INTL_TEMPERATURE), last_value_SEN5X_T);
			add_table_h_value(FPSTR(SENSORS_SEN54), FPSTR(INTL_HUMIDITY), last_value_SEN5X_H);
			add_table_value(FPSTR(SENSORS_SEN54), FPSTR(INTL_VOC), check_display_value(last_value_SEN5X_VOC, 0, 0, 0), "(index)");
			page_content += FPSTR(EMPTY_ROW);
		}

		if (memcmp(SEN5X_type, "SEN55", 6) == 0)
		{
			add_table_pm_value(FPSTR(SENSORS_SEN55), FPSTR(WEB_PM1), last_value_SEN5X_P0);
			add_table_pm_value(FPSTR(SENSORS_SEN55), FPSTR(WEB_PM25), last_value_SEN5X_P2);
			add_table_pm_value(FPSTR(SENSORS_SEN55), FPSTR(WEB_PM4), last_value_SEN5X_P4);
			add_table_pm_value(FPSTR(SENSORS_SEN55), FPSTR(WEB_PM10), last_value_SEN5X_P1);
			add_table_nc_value(FPSTR(SENSORS_SEN55), FPSTR(WEB_NC0k5), last_value_SEN5X_N05);
			add_table_nc_value(FPSTR(SENSORS_SEN55), FPSTR(WEB_NC1k0), last_value_SEN5X_N1);
			add_table_nc_value(FPSTR(SENSORS_SEN55), FPSTR(WEB_NC2k5), last_value_SEN5X_N25);
			add_table_nc_value(FPSTR(SENSORS_SEN55), FPSTR(WEB_NC4k0), last_value_SEN5X_N4);
			add_table_nc_value(FPSTR(SENSORS_SEN55), FPSTR(WEB_NC10), last_value_SEN5X_N10);
			add_table_value(FPSTR(SENSORS_SEN55), FPSTR(WEB_TPS), check_display_value(last_value_SEN5X_TS, -1, 1, 0), "µm");
			add_table_t_value(FPSTR(SENSORS_SEN55), FPSTR(INTL_TEMPERATURE), last_value_SEN5X_T);
			add_table_h_value(FPSTR(SENSORS_SEN55), FPSTR(INTL_HUMIDITY), last_value_SEN5X_H);
			add_table_value(FPSTR(SENSORS_SEN55), FPSTR(INTL_VOC), check_display_value(last_value_SEN5X_VOC, 0, 0, 0), "(index)");
			add_table_value(FPSTR(SENSORS_SEN55), FPSTR(INTL_NOX), check_display_value(last_value_SEN5X_NOX, 0, 0, 0), "ppm");
			page_content += FPSTR(EMPTY_ROW);
		}
	}

	if (cfg::sps30_read)
	{
		add_table_pm_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_PM1), last_value_SPS30_P0);
		add_table_pm_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_PM25), last_value_SPS30_P2);
		add_table_pm_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_PM4), last_value_SPS30_P4);
		add_table_pm_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_PM10), last_value_SPS30_P1);
		add_table_nc_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_NC0k5), last_value_SPS30_N05);
		add_table_nc_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_NC1k0), last_value_SPS30_N1);
		add_table_nc_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_NC2k5), last_value_SPS30_N25);
		add_table_nc_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_NC4k0), last_value_SPS30_N4);
		add_table_nc_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_NC10), last_value_SPS30_N10);
		add_table_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_TPS), check_display_value(last_value_SPS30_TS, -1, 1, 0), "µm");
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::dht_read)
	{
		add_table_t_value(FPSTR(SENSORS_DHT22), FPSTR(INTL_TEMPERATURE), last_value_DHT_T);
		add_table_h_value(FPSTR(SENSORS_DHT22), FPSTR(INTL_HUMIDITY), last_value_DHT_H);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::htu21d_read)
	{
		add_table_t_value(FPSTR(SENSORS_HTU21D), FPSTR(INTL_TEMPERATURE), last_value_HTU21D_T);
		add_table_h_value(FPSTR(SENSORS_HTU21D), FPSTR(INTL_HUMIDITY), last_value_HTU21D_H);
		dew_point_temp = dew_point(last_value_HTU21D_T, last_value_HTU21D_H);
		add_table_value(FPSTR(SENSORS_HTU21D), FPSTR(INTL_DEW_POINT), isnan(dew_point_temp) ? "-" : String(dew_point_temp, 1), unit_T);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::bmp_read)
	{
		add_table_t_value(FPSTR(SENSORS_BMP180), FPSTR(INTL_TEMPERATURE), last_value_BMP_T);
		add_table_value(FPSTR(SENSORS_BMP180), FPSTR(INTL_PRESSURE), check_display_value(last_value_BMP_P / 100.0f, (-1 / 100.0f), 2, 0), unit_P);
		add_table_value(FPSTR(SENSORS_BMP180), FPSTR(INTL_PRESSURE_AT_SEALEVEL), last_value_BMP_P != -1.0f ? String(pressure_at_sealevel(last_value_BMP_T, last_value_BMP_P / 100.0f), 2) : "-", unit_P);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::bmx280_read)
	{
		const char *const sensor_name = (bmx280.sensorID() == BME280_SENSOR_ID) ? SENSORS_BME280 : SENSORS_BMP280;
		add_table_t_value(FPSTR(sensor_name), FPSTR(INTL_TEMPERATURE), last_value_BMX280_T);
		add_table_value(FPSTR(sensor_name), FPSTR(INTL_PRESSURE), check_display_value(last_value_BMX280_P / 100.0f, (-1 / 100.0f), 2, 0), unit_P);
		add_table_value(FPSTR(sensor_name), FPSTR(INTL_PRESSURE_AT_SEALEVEL), last_value_BMX280_P != -1.0f ? String(pressure_at_sealevel(last_value_BMX280_T, last_value_BMX280_P / 100.0f), 2) : "-", unit_P);

		if (bmx280.sensorID() == BME280_SENSOR_ID)
		{
			add_table_h_value(FPSTR(sensor_name), FPSTR(INTL_HUMIDITY), last_value_BME280_H);
			dew_point_temp = dew_point(last_value_BMX280_T, last_value_BME280_H);
			add_table_value(FPSTR(sensor_name), FPSTR(INTL_DEW_POINT), isnan(dew_point_temp) ? "-" : String(dew_point_temp, 1), unit_T);
		}

		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::sht3x_read)
	{
		add_table_t_value(FPSTR(SENSORS_SHT3X), FPSTR(INTL_TEMPERATURE), last_value_SHT3X_T);
		add_table_h_value(FPSTR(SENSORS_SHT3X), FPSTR(INTL_HUMIDITY), last_value_SHT3X_H);
		dew_point_temp = dew_point(last_value_SHT3X_T, last_value_SHT3X_H);
		add_table_value(FPSTR(SENSORS_SHT3X), FPSTR(INTL_DEW_POINT), isnan(dew_point_temp) ? "-" : String(dew_point_temp, 1), unit_T);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::scd30_read)
	{
		add_table_t_value(FPSTR(SENSORS_SCD30), FPSTR(INTL_TEMPERATURE), last_value_SCD30_T);
		add_table_h_value(FPSTR(SENSORS_SCD30), FPSTR(INTL_HUMIDITY), last_value_SCD30_H);
		add_table_value(FPSTR(SENSORS_SCD30), FPSTR(INTL_CO2_PPM), check_display_value(last_value_SCD30_CO2, 0, 0, 0), unit_CO2);
		dew_point_temp = dew_point(last_value_SCD30_T, last_value_SCD30_H);
		add_table_value(FPSTR(SENSORS_SCD30), FPSTR(INTL_DEW_POINT), isnan(dew_point_temp) ? "-" : String(dew_point_temp, 1), unit_T);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::ds18b20_read)
	{
		add_table_t_value(FPSTR(SENSORS_DS18B20), FPSTR(INTL_TEMPERATURE), last_value_DS18B20_T);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::dnms_read)
	{
		add_table_value(FPSTR(SENSORS_DNMS), FPSTR(INTL_LEQ_A), check_display_value(last_value_dnms_laeq, -1, 1, 0), unit_LA);
		add_table_value(FPSTR(SENSORS_DNMS), FPSTR(INTL_LA_MIN), check_display_value(last_value_dnms_la_min, -1, 1, 0), unit_LA);
		add_table_value(FPSTR(SENSORS_DNMS), FPSTR(INTL_LA_MAX), check_display_value(last_value_dnms_la_max, -1, 1, 0), unit_LA);
		page_content += FPSTR(EMPTY_ROW);
	}

	if (cfg::gps_read)
	{
		add_table_value(FPSTR(WEB_GPS), FPSTR(INTL_LATITUDE), check_display_value(last_value_GPS_lat, -200.0, 6, 0), unit_Deg);
		add_table_value(FPSTR(WEB_GPS), FPSTR(INTL_LONGITUDE), check_display_value(last_value_GPS_lon, -200.0, 6, 0), unit_Deg);
		add_table_value(FPSTR(WEB_GPS), FPSTR(INTL_ALTITUDE), check_display_value(last_value_GPS_alt, -1000.0, 2, 0), "m");
		add_table_value(FPSTR(WEB_GPS), FPSTR(INTL_TIME_UTC), last_value_GPS_timestamp, emptyString);
		page_content += FPSTR(EMPTY_ROW);
	}

	server.sendContent(page_content);
	page_content = emptyString;

	add_table_value(F("WiFi"), FPSTR(INTL_SIGNAL_STRENGTH), String(last_signal_strength), "dBm");
	add_table_value(F("WiFi"), FPSTR(INTL_SIGNAL_QUALITY), String(signal_quality), "%");

	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(BR_TAG);
	end_html_page(page_content);
}

/*****************************************************************
 * Webserver root: show device status
 * call by webserver command: /status
 *****************************************************************/
static void webserver_status()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		sendHttpRedirect();

		debug_outln_info(F("ws: status => WiFi not connected ..."));
		return;
	}

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DEVICE_STATUS));

	debug_outln_info(F("ws: status => create webpage ..."));

	server.sendContent(page_content);
	page_content = F("<table cellspacing='0' cellpadding='5' class='v'>\n"
					 "<thead><tr><th> " INTL_PARAMETER "</th><th>" INTL_VALUE "</th></tr></thead>");

	String versionHtml(SOFTWARE_VERSION);
	versionHtml += F("/ST:");
	versionHtml += String(!airrohr_selftest_failed);
	versionHtml += '/';

#if defined(ESP8266)
	versionHtml += ESP.getFullVersion();
#endif

	versionHtml.replace("/", FPSTR(BR_TAG));

	add_table_row_from_value(page_content, FPSTR(INTL_FIRMWARE), versionHtml);
	add_table_row_from_value(page_content, F("Free Memory"), String(ESP.getFreeHeap()));
	add_table_row_from_value(page_content, F("Used json.config (used/max)"), String(json_config_memory_used) + String(" / ") + String(JSON_BUFFER_SIZE)+ String("  char") );
	add_table_row_from_value(page_content, F("Used Simm7000.config (used/max)"), String(json_config7000_memory_used) + String(" / ") + String(JSON_BUFFER_SIZE_SIMM7000) + String("  char") );
	
#if defined(ESP8266)
	add_table_row_from_value(page_content, F("Heap Fragmentation"), String(ESP.getHeapFragmentation()), "%");
#endif

	if (cfg::auto_update)
	{
		add_table_row_from_value(page_content, F("Last OTA"), delayToString(millis() - last_update_attempt));
	}

#if defined(ESP8266)
	add_table_row_from_value(page_content, F("NTP Sync"), String(sntp_time_set));
	StreamString ntpinfo;

	for (unsigned i = 0; i < SNTP_MAX_SERVERS; i++)
	{
		const ip_addr_t *sntp = sntp_getserver(i);
		if (sntp && !ip_addr_isany(sntp))
		{
			const char *name = sntp_getservername(i);
			if (!ntpinfo.isEmpty())
			{
				ntpinfo.print(FPSTR(BR_TAG));
			}

			ntpinfo.printf_P(PSTR("%s (%s)"), IPAddress(*sntp).toString().c_str(), name ? name : "?");
			ntpinfo.printf_P(PSTR(" reachable: %s"), sntp_getreachability(i) ? "Yes" : "No");
		}
	}

	add_table_row_from_value(page_content, F("NTP Info"), ntpinfo);

#endif

	time_t now = time(nullptr);
	add_table_row_from_value(page_content, FPSTR(INTL_TIME_UTC), ctime(&now));
	add_table_row_from_value(page_content, F("Uptime"), delayToString(millis() - time_point_device_start_ms));

#if defined(ESP8266)
	add_table_row_from_value(page_content, F("Reset Reason"), ESP.getResetReason());
#endif

	if (cfg::sds_read)
	{
		page_content += FPSTR(EMPTY_ROW);
		add_table_row_from_value(page_content, FPSTR(SENSORS_SDS011), last_value_SDS_version);
	}

	if (cfg::npm_read)
	{
		page_content += FPSTR(EMPTY_ROW);
		add_table_row_from_value(page_content, FPSTR(SENSORS_NPM), last_value_NPM_version);
	}

	if (cfg::scd30_read)
	{// set data for webpage section SCD-30

		page_content += FPSTR(EMPTY_ROW);

		add_table_row_from_value(page_content,  FPSTR( SENSORS_SCD30), emptyString);

		uint16_t settingVal = 0;

		scd30.getFirmwareVersion(&settingVal);
		versionHtml = F("Firmware Version:   V ") + String( ((float)settingVal) / 100);
		versionHtml += String( BR_TAG);
		versionHtml += F("Auto Calibration = ");
		versionHtml += scd30.getAutoSelfCalibration() == true ? F("enabled") : F("disabled");
		versionHtml += String( BR_TAG);
		scd30.getMeasurementInterval(&settingVal);
		versionHtml += F("Measurement interval =  ") + String( settingVal) + String("s");
		versionHtml += String( BR_TAG);
		float offsetTemp = scd30.getTemperatureOffset();
		versionHtml += F("Temperature offset = ") + String( offsetTemp) + String("°C");
		versionHtml += String( BR_TAG);
		scd30.getForcedRecalibration(&settingVal);
		versionHtml += F("CO₂ offset = ") + String( settingVal) + String("ppm");

		add_table_row_from_value(page_content, F("Settings"), versionHtml);

		// dit geeft een browser fout, 1 line to much, page not open in browser, WHY?????.
		//scd30.getFirmwareVersion(&settingVal);
		//add_table_row_from_value(page_content, F("Firmware Version"), String( ((float)settingVal) / 100));

		// if (scd30.getAutoSelfCalibration() == true)
		// 	add_table_row_from_value(page_content, F("Auto Calibration"), "enabled");
		// else
		// 	add_table_row_from_value(page_content, F("Auto Calibration"), "disabled");
			
		//scd30.getMeasurementInterval(&settingVal);
		//add_table_row_from_value(page_content, F("Measurement interval"), String( settingVal));

		//float offsetTemp = scd30.getTemperatureOffset();
		//add_table_row_from_value(page_content, F("Temperature offset"), String( offsetTemp));

		//scd30.getForcedRecalibration(&settingVal);
		//add_table_row_from_value(page_content, F("CO₂ offset"), String( settingVal) + String(" ppm"));
	}

	page_content += FPSTR(EMPTY_ROW);
	page_content += F("<tr><td colspan='2'><b>" INTL_ERROR "</b></td></tr>");

	String wifiStatus(WiFi_error_count);
	wifiStatus += '/';
	wifiStatus += String(last_signal_strength);
	wifiStatus += '/';
	wifiStatus += String(last_disconnect_reason);
	add_table_row_from_value(page_content, F("WiFi"), wifiStatus);

	if (last_update_returncode != 0)
	{
		add_table_row_from_value(page_content, F("OTA Return"),
								 last_update_returncode > 0 ? String(last_update_returncode) : HTTPClient::errorToString(last_update_returncode));
	}

	for (unsigned int i = 0; i < LoggerCount; ++i)
	{
		if (loggerConfigs[i].errors)
		{
			const __FlashStringHelper *logger = loggerDescription(i);
			if (logger)
			{
				add_table_row_from_value(page_content, logger, String(loggerConfigs[i].errors));
			}
		}
	}

	if (last_sendData_returncode != 0)
	{
		add_table_row_from_value(page_content, F("Data Send Return"),
								 last_sendData_returncode > 0 ? String(last_sendData_returncode) : HTTPClient::errorToString(last_sendData_returncode));
	}

	if (cfg::sds_read)
	{
		add_table_row_from_value(page_content, FPSTR(SENSORS_SDS011), String(SDS_error_count));
	}

	if (cfg::npm_read)
	{
		add_table_row_from_value(page_content, FPSTR(SENSORS_NPM), String(NPM_error_count));
	}

	if (cfg::ips_read)
	{
		add_table_row_from_value(page_content, FPSTR(SENSORS_IPS), String(IPS_error_count));
	}

	if (cfg::sen5x_read)
	{
		if(memcmp(SEN5X_type,"SEN50",6) == 0)
		{
			add_table_row_from_value(page_content, FPSTR(SENSORS_SEN50), String(SEN5X_read_error_counter));
		}

		if(memcmp(SEN5X_type,"SEN54",6)== 0)
		{
			add_table_row_from_value(page_content, FPSTR(SENSORS_SEN54), String(SEN5X_read_error_counter));
		}

		if(memcmp(SEN5X_type,"SEN55",6)== 0)
		{
			add_table_row_from_value(page_content, FPSTR(SENSORS_SEN55), String(SEN5X_read_error_counter));
		}
	}

	if (cfg::sps30_read)
	{
		add_table_row_from_value(page_content, FPSTR(SENSORS_SPS30), String(SPS30_read_error_counter));
	}

	server.sendContent(page_content);
	page_content = emptyString;

	if (count_sends > 0)
	{
		page_content += FPSTR(EMPTY_ROW);
		add_table_row_from_value(page_content, F(INTL_NUMBER_OF_MEASUREMENTS), String(count_sends));

		if (sending_time > 0)
		{
			add_table_row_from_value(page_content, F(INTL_TIME_SENDING_MS), String(sending_time), "ms");
		}

		if (cfg::has_radarmotion)
		{
			add_table_row_from_value(page_content, F(INTL_NUMBER_OF_RADARMOTION), String(RCWL0516.GetMotionCount()));
		}
	}

	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	end_html_page(page_content);

	debug_outln_info(F("ws: status => webpage send too ..."));
}

/*****************************************************************
 * Webserver read serial ring buffer                             *
 *****************************************************************/
static void webserver_serial()
{
	String s(Debug.popLines());

	server.send(s.length() ? 200 : 204, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), s);
}

/*****************************************************************
 * Webserver set debug level                                     *
 *****************************************************************/
static void webserver_debug_level()
{
	if (!webserver_request_auth())
	{
		return;
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DEBUG_LEVEL));

	if (server.hasArg("lvl"))
	{
		debug_outln_info(F("ws: debug level ..."));

		const int lvl = server.arg("lvl").toInt();
		if (lvl >= 0 && lvl <= 5)
		{
			cfg::debug = lvl;
			page_content += F("<h3>");
			page_content += FPSTR(INTL_DEBUG_SETTING_TO);
			page_content += ' ';

			const __FlashStringHelper *lvlText;

			switch (lvl)
			{
			case DEBUG_ERROR:
				lvlText = F(INTL_ERROR);
				break;
			case DEBUG_WARNING:
				lvlText = F(INTL_WARNING);
				break;
			case DEBUG_MIN_INFO:
				lvlText = F(INTL_MIN_INFO);
				break;
			case DEBUG_MED_INFO:
				lvlText = F(INTL_MED_INFO);
				break;
			case DEBUG_MAX_INFO:
				lvlText = F(INTL_MAX_INFO);
				break;
			default:
				lvlText = F(INTL_NONE);
			}

			page_content += lvlText;
			page_content += F(".</h3>");
		}
	}

	page_content += F("<br/><pre id='slog' class='panels'>");
	page_content += Debug.popLines();
	page_content += F("</pre>");
	page_content += F("<script>"
					  "function slog_update() {"
					  "fetch('/serial').then(r => r.text()).then((r) => {"
					  "document.getElementById('slog').innerText += r;}).catch(err => console.log(err));};"
					  "setInterval(slog_update, 3000);"
					  "</script>");
	page_content += F("<h4>");
	page_content += FPSTR(INTL_DEBUG_SETTING_TO);
	page_content += F("</h4>"
					  "<table style='width:100%;'>"
					  "<tr><td style='width:25%;'><a class='b' href='/debug?lvl=0'>" INTL_NONE "</a></td>"
					  "<td style='width:25%;'><a class='b' href='/debug?lvl=1'>" INTL_ERROR "</a></td>"
					  "<td style='width:25%;'><a class='b' href='/debug?lvl=3'>" INTL_MIN_INFO "</a></td>"
					  "<td style='width:25%;'><a class='b' href='/debug?lvl=5'>" INTL_MAX_INFO "</a></td>"
					  "</tr><tr>"
					  "</tr>"
					  "</table>");

	end_html_page(page_content);
}

/*****************************************************************
 * Webserver remove config                                       *
 *****************************************************************/
static void webserver_removeConfig()
{
	if (!webserver_request_auth())
	{
		return;
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DELETE_CONFIG));
	debug_outln_info(F("ws: removeConfig ..."));

	if (server.method() == HTTP_GET)
	{
		page_content += FPSTR(WEB_REMOVE_CONFIG_CONTENT);
	}
	else
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		// Silently remove the desaster backup
		SPIFFS.remove(F("/config.json.old"));
		if (SPIFFS.exists(F("/config.json")))
		{ // file exists
			debug_outln_info(F("removing current config.json..."));

			if (SPIFFS.remove(F("/config.json")))
			{
				page_content += F("<h3>" INTL_CONFIG_DELETED ".</h3>");
			}
			else
			{
				page_content += F("<h3>" INTL_CONFIG_CAN_NOT_BE_DELETED ".</h3>");
			}
		}
		else
		{
			page_content += F("<h3>" INTL_CONFIG_NOT_FOUND ".</h3>");
		}
#pragma GCC diagnostic pop
	}
	end_html_page(page_content);
}

/*****************************************************************
 * Webserver reset NodeMCU                                       *
 *****************************************************************/
static void webserver_reset()
{
	if (!webserver_request_auth())
	{
		return;
	}

	String page_content;
	page_content.reserve(512);

	start_html_page(page_content, FPSTR(INTL_RESTART_SENSOR));
	debug_outln_info(F("ws: reset ..."));

	if (server.method() == HTTP_GET)
	{
		page_content += FPSTR(WEB_RESET_CONTENT);
	}
	else
	{
		sensor_restart();
	}

	end_html_page(page_content);
}

/*****************************************************************
 * Webserver data.json                                           *
 *****************************************************************/
static void webserver_data_json()
{
	String s1;
	unsigned long age = 0;

	debug_outln_info(F("ws: data json..."));

	if (!count_sends)
	{
		s1 = FPSTR(data_first_part);
		s1 += "]}";
		age = cfg::sending_intervall_ms - msSince(starttime);

		if (age > cfg::sending_intervall_ms)
		{
			age = 0;
		}

		age = 0 - age;
	}
	else
	{
		s1 = last_data_string;
		age = msSince(starttime);
		
		if (age > cfg::sending_intervall_ms)
		{
			age = 0;
		}
	}

	String s2 = F(", \"age\":\"");
	s2 += String((long)((age + 500) / 1000));
	s2 += F("\", \"sensordatavalues\"");
	s1.replace(F(", \"sensordatavalues\""), s2);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), s1);
}

/*****************************************************************
 * Webserver metrics endpoint                                    *
 *****************************************************************/
static void webserver_metrics_endpoint()
{
	debug_outln_info(F("ws: /metrics"));
	RESERVE_STRING(page_content, XLARGE_STR);
	page_content = F("software_version{version=\"" SOFTWARE_VERSION_STR "\",$i} 1\nuptime_ms{$i} $u\nsending_intervall_ms{$i} $s\nnumber_of_measurements{$i} $c\n");
	String id(F("node=\"" SENSOR_BASENAME));
	id += esp_chipid;
	id += '\"';
	page_content.replace("$i", id);
	page_content.replace("$u", String(msSince(time_point_device_start_ms)));
	page_content.replace("$s", String(cfg::sending_intervall_ms));
	page_content.replace("$c", String(count_sends));
	DynamicJsonDocument json2data(JSON_BUFFER_SIZE);

	DeserializationError err = deserializeJson(json2data, last_data_string);

	if (!err)
	{
		for (JsonObject measurement : json2data[FPSTR(JSON_SENSOR_DATA_VALUES)].as<JsonArray>())
		{
			page_content += measurement["value_type"].as<const char *>();
			page_content += '{';
			page_content += id;
			page_content += "} ";
			page_content += measurement["value"].as<const char *>();
			page_content += '\n';
		}

		page_content += F("last_sample_age_ms{");
		page_content += id;
		page_content += "} ";
		page_content += String(msSince(starttime));
		page_content += '\n';
	}
	else
	{
		debug_outln_error(FPSTR(DBG_TXT_DATA_READ_FAILED));
	}

	page_content += F("# EOF\n");
	debug_outln(page_content, DEBUG_MED_INFO);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), page_content);
}

/*****************************************************************
 * Webserver Images                                              *
 *****************************************************************/
static void webserver_favicon()
{
	server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));

	server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG,
				  LUFTDATEN_INFO_LOGO_PNG, LUFTDATEN_INFO_LOGO_PNG_SIZE);
}

/*

*/
static void webserver_static()
{
	server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));

	if (server.arg(String('r')) == F("logo"))
	{
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG,
					  LUFTDATEN_INFO_LOGO_PNG, LUFTDATEN_INFO_LOGO_PNG_SIZE);
	}
	else if (server.arg(String('r')) == F("css"))
	{
		server.send_P(200, TXT_CONTENT_TYPE_TEXT_CSS,
					  WEB_PAGE_STATIC_CSS, sizeof(WEB_PAGE_STATIC_CSS) - 1);
	}
	else
	{
		webserver_not_found();
	}
}

/*****************************************************************
 * Webserver page not found                                      *
 *****************************************************************/
static void webserver_not_found()
{
	last_page_load = millis();
	debug_outln_info(F("ws: not found ..."));

	if (WiFi.status() != WL_CONNECTED)
	{
		if ((server.uri().indexOf(F("success.html")) != -1) || (server.uri().indexOf(F("detect.html")) != -1))
		{
			server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), FPSTR(WEB_IOS_REDIRECT));
		}
		else
		{
			sendHttpRedirect();
		}
	}
	else
	{
		server.send(404, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), F("Not found."));
	}
}

/*****************************************************************
 * Webserver setup menu function table list                      *
 *****************************************************************/
static void setup_webserver()
{
	server.on("/", webserver_root);
	server.on(F("/config"), webserver_config);
	server.on(F("/s7000"), webserver_config7);
	server.on(F("/wifi"), webserver_wifi);
	server.on(F("/values"), webserver_values);
	server.on(F("/status"), webserver_status);
	server.on(F("/generate_204"), webserver_config);
	server.on(F("/fwlink"), webserver_config);
	server.on(F("/debug"), webserver_debug_level);
	server.on(F("/serial"), webserver_serial);
	server.on(F("/removeConfig"), webserver_removeConfig);
	server.on(F("/reset"), webserver_reset);
	server.on(F("/data.json"), webserver_data_json);
	server.on(F("/metrics"), webserver_metrics_endpoint);
	server.on(F("/favicon.ico"), webserver_favicon);
	server.on(F(STATIC_PREFIX), webserver_static);

	server.onNotFound(webserver_not_found);

	debug_outln_info(F("Station (STA) Mode: Starting Webserver... "), WiFi.localIP().toString());
	debug_outln_info(F("Access Point (AP) Mode: Starting Webserver... "), WiFi.softAPIP().toString());

	server.begin();
}

/*
	Set Up connection to a MQTT broker.
	like Mosquitto.
*/
static void setup_mqtt_broker(const char *host, const int port)
{
#if defined(ESP8266)
	if (cfg::send2mqtt && !mqtt_client.connected())
	{
		debug_outln_info(F("\n** Start Initialize MQTT Broker connection **"));

		// ++ Set-Up Topic header for MQTT Broker
		String _header = String(cfg::mqtt_topic) + "/" + String(mqtt_client_id);
		if (_header.length() <= LEN_MQTT_LARGE_HEADER)
		{
			strcpy(mqtt_header, _header.c_str());
		}

		_header += "/" + String(mqtt_lwt);
		strcpy(mqtt_lwt_header, _header.c_str());
		// -- Set-Up Topic header for MQTT Broker

		mqtt_client.setServer(host, port);

		String mess_off = INTL_OFFLINE;

		if (mqtt_client.connect(mqtt_client_id, cfg::mqtt_user, cfg::mqtt_pwd, mqtt_lwt_header, 1, true, mess_off.c_str(), true))
		{
			// Set keep Alive setKeepAlive() default 15 seconds
			// cfg::sending_intervall_ms delen door 1000 * 2 = eepalive
			int16_t keepAlive = cfg::sending_intervall_ms * 0.002;
			mqtt_client.setBufferSize(MAX_MQTT_BUFFER_SIZE);
			mqtt_client.setKeepAlive(keepAlive);

			for(int cnt = 5;cnt > 0;cnt--)
			{
				if( mqtt_client.connected())
				{
					break;
				}

				debug_outln_info(F("** Not connected to MQTT Broker, wait ** state: "), String(mqtt_client.state()));
				delay(500);
			}

			debug_outln_info(F("KeepAlive  - "), String(keepAlive) + F(" sec."));
			debug_outln_info(F("** MQTT Broker connected ** C_flag: "), String(mqtt_client.connected()));
		}
		else
		{
			debug_outln_info(F("MQTT Broker connecting failed, rc= "), String(mqtt_client.state()));
		}
	}
#endif
}
/*
	select Channel For App.
	return channel nr: 1 or 6 or 11
*/
static int selectChannelForAp()
{
	std::array<int, 14> channels_rssi;
	std::fill(channels_rssi.begin(), channels_rssi.end(), -100);

	for (unsigned i = 0; i < std::min((uint8_t)14, count_wifiInfo); i++)
	{
		if (wifiInfo[i].RSSI > channels_rssi[wifiInfo[i].channel])
		{
			channels_rssi[wifiInfo[i].channel] = wifiInfo[i].RSSI;
		}
	}

	if ((channels_rssi[1] < channels_rssi[6]) && (channels_rssi[1] < channels_rssi[11]))
	{
		return 1;
	}
	else if ((channels_rssi[6] < channels_rssi[1]) && (channels_rssi[6] < channels_rssi[11]))
	{
		return 6;
	}
	else
	{
		return 11;
	}
}

/*****************************************************************
 * WifiConfig                                                    *
 *****************************************************************/
static void wifiConfig()
{
	debug_outln_info(F("Starting WiFiManager"));
	debug_outln_info(F("AP ID: "), String(cfg::fs_ssid));
	debug_outln_info(F("Password: "), String(cfg::fs_pwd));

	wificonfig_loop = true;

	WiFi.disconnect(true);
	debug_outln_info(F("scan for wifi networks..."));
	int8_t scanReturnCode = WiFi.scanNetworks(false /* scan async */, true /* show hidden networks */);

	if (scanReturnCode < 0)
	{
		debug_outln_error(F("WiFi scan failed. Treating as empty. "));
		count_wifiInfo = 0;
	}
	else
	{
		count_wifiInfo = (uint8_t)scanReturnCode;
	}

	delete[] wifiInfo;
	wifiInfo = new struct_wifiInfo[std::max(count_wifiInfo, (uint8_t)1)];

	for (unsigned i = 0; i < count_wifiInfo; i++)
	{
		String SSID;
		uint8_t *BSSID;

		memset(&wifiInfo[i], 0, sizeof(struct_wifiInfo));

#if defined(ESP8266)
		WiFi.getNetworkInfo(i, SSID, wifiInfo[i].encryptionType,
							wifiInfo[i].RSSI, BSSID, wifiInfo[i].channel,
							wifiInfo[i].isHidden);
#else
		WiFi.getNetworkInfo(i, SSID, wifiInfo[i].encryptionType,
							wifiInfo[i].RSSI, BSSID, wifiInfo[i].channel);
#endif

		SSID.toCharArray(wifiInfo[i].ssid, sizeof(wifiInfo[0].ssid));
	}

	// Use 13 channels if locale is not "EN"
	wifi_country_t wifi;
	wifi.policy = WIFI_COUNTRY_POLICY_MANUAL;
	strcpy(wifi.cc, INTL_LANG);
	wifi.nchan = /* (INTL_LANG[0] == 'E' && INTL_LANG[1] == 'N') ? 11 : */ 13;
	wifi.schan = 1;

#if defined(ESP8266)
	wifi_set_country(&wifi);
#endif

	/*
		Access Point (AP).
		In this mode, ESP8266 will advertise its WiFi hotspot with a custom SSID and Password. 
		Other smart devices will be able to connect, and consequently establish communication with the ESP8266 WiFi module.
	*/
	WiFi.mode(WIFI_AP);
	const IPAddress apIP(
						default_ip_first_octet, 
						default_ip_second_octet, 
						default_ip_third_octet, 
						default_ip_fourth_octet);
		
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP(cfg::fs_ssid, cfg::fs_pwd, selectChannelForAp());

	// In case we create a unique password at first start
	debug_outln_info(F("AP Password is: "), cfg::fs_pwd);

	DNSServer dnsServer;
	// Ensure we don't poison the client DNS cache
	dnsServer.setTTL(0);
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(53, "*", apIP); 					// 53 is port for DNS server

	setup_webserver();

	// 10 minutes timeout for wifi config.
	last_page_load = millis();

	while ((millis() - last_page_load) < cfg::time_for_wifi_config + 500)
	{
		dnsServer.processNextRequest();
		server.handleClient();

#if defined(ESP8266)
		wdt_reset(); 		// nodemcu is alive
		MDNS.update();
#endif

		yield();

		if( millis() % last_page_load == 0)
		{
			debug_out("-", DEBUG_MIN_INFO);
			delay(5);
		}
	}

	debug_outln_info(emptyString);			// LF/CR char.

// after 10 minutes waiting on server commando's => restart current configuration settings.
	WiFi.softAPdisconnect(true);

	wifi.policy = WIFI_COUNTRY_POLICY_MANUAL;
	strcpy(wifi.cc, INTL_LANG);
	wifi.nchan = 13;
	wifi.schan = 1;

#if defined(ESP8266)
	wifi_set_country(&wifi);
#endif

	/*
		Station (STA) Mode:
	 	In station mode, ESP8266 will act just like your smartphone or laptop. 
	 	It will connect to an existing WiFi channel, or in most cases, the WiFi advertised by your router.
	*/
	WiFi.mode(WIFI_STA);

	dnsServer.stop();
	delay(100);

	if (cfg::has_fix_ip)
	{
		WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
		waitForWifiToConnect(20);
	}
	else
	{
		// Register multi WiFi networks
		RegisterMultiWiFiNetworks(WIFI_MAX_RETRY);
	}

	// debug_outln_info(FPSTR(DBG_TXT_CONNECTING_TO), cfg::wlanssid);
	debug_outln_info(FPSTR(DBG_TXT_CONNECTING_TO), WiFi.SSID());

	debug_outln_info(F("---- Result Webconfig ----"));
	debug_outln_info(F("WLANSSID: "), cfg::wlanssid);
	debug_outln_info(F("WLANSSID_2: "), cfg::wlanssid_2);
	debug_outln_info(F("WLANSSID_3: "), cfg::wlanssid_3);
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_info_bool(F("PPD: "), cfg::ppd_read);
	debug_outln_info_bool(F("SDS: "), cfg::sds_read);
	debug_outln_info_bool(F("SEN5X: "), cfg::sen5x_read);
	debug_outln_info_bool(F("PMS: "), cfg::pms_read);
	debug_outln_info_bool(F("HPM: "), cfg::hpm_read);
	debug_outln_info_bool(F("SPS30: "), cfg::sps30_read);
	debug_outln_info_bool(F("NPM: "), cfg::npm_read);
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_info_bool(F("DHT: "), cfg::dht_read);
	debug_outln_info_bool(F("DS18B20: "), cfg::ds18b20_read);
	debug_outln_info_bool(F("HTU21D: "), cfg::htu21d_read);
	debug_outln_info_bool(F("BMP: "), cfg::bmp_read);
	debug_outln_info_bool(F("BMX280: "), cfg::bmx280_read);
	debug_outln_info_bool(F("SCD30: "), cfg::scd30_read);
	debug_outln_info_bool(F("SHT3X: "), cfg::sht3x_read);			// SEN3X: Temperature, Humidity
	debug_outln_info_bool(F("DNMS: "), cfg::dnms_read);
	debug_outln_info_bool(F("SHT5X: "), cfg::sen5x_read);			// SEN5X: Temperature, Humidity, CO2 as NOx
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_info_bool(F("SensorCommunity: "), cfg::send2dusti);
	debug_outln_info_bool(F("Madavi: "), cfg::send2madavi);
	debug_outln_info_bool(F("CSV: "), cfg::send2csv);
	debug_outln_info_bool(F("MQTT: "), cfg::send2mqtt);
	debug_outln_info_bool(F("Fix IP address: "), cfg::has_fix_ip);
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_info_bool(F("SIMM-7000: "), cfg::has_s7000);
	debug_outln_info_bool(F("RCWL-0516: "), cfg::has_radarmotion);
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_info_bool(F("Autoupdate: "), cfg::auto_update);
	debug_outln_info_bool(F("Display: "), cfg::has_display);
	debug_outln_info_bool(F("LCD 1602: "), !!lcd_1602);
	debug_outln_info(F("Debug: "), String(cfg::debug));
	debug_outln_info(FPSTR(DBG_TXT_SEP));

	wificonfig_loop = false;
}

/*****************************************************************
	Wait For Wifi To Connect.
******************************************************************/
static void waitForWifiToConnect(int maxRetries)
{
	int retryCount = 0;
	while ((WiFi.status() != WL_CONNECTED) && (retryCount < maxRetries))
	{
		delay(500);
		debug_out(".", DEBUG_MIN_INFO);
		++retryCount;
	}
}

/***************************************************************************************************************************
*	Wait For MultiWiFi To Connect/Reconnect to a WiFi network.
*
*	connectTimeOutPerAP => Defines the TimeOut(ms) which will be used to try and connect with any specific Access Point.
****************************************************************************************************************************/
static void waitForMultiWiFiToConnect(int maxRetries, uint16_t connectTimeOutPerAP = 2000)
{
	int retryCount = 0;

	// Wait for ESP8266 to scan the local area and connect with the strongest of the networks defined above
	while ((retryCount < maxRetries) && wifiMulti.run(connectTimeOutPerAP) != WL_CONNECTED )
	{
		//delay(50);
		debug_out("*", DEBUG_MIN_INFO);
		retryCount++;
	}

	debug_outln_info(emptyString);
}

/*****************************************************************
	Adding the WiFi networks to the MultiWiFi instance
******************************************************************/
static void RegisterMultiWiFiNetworks(int maxRetries)
{
	uint16_t connectTimeOutPerAP = WIFI_CONNECT_TIMEOUT_MS; // Defines the TimeOut(ms) which will be used to try and connect with any specific Access Point.

	wifiMulti.addAP(cfg::wlanssid, cfg::wlanpwd); 			// Open/Start WiFI coonection to router/modem. (default: WiFi Network 1)

	if (cfg::has_morewifi)
	{
		debug_outln_info(F("Register to Multi WiFi Network."));

		if (strlen(cfg::wlanpwd_2) != 0)
		{
			wifiMulti.addAP(cfg::wlanssid_2, cfg::wlanpwd_2);
			//connectTimeOutPerAP = WIFI_CONNECT_TIMEOUT_MS;
			debug_outln_info(F("Set WiFi Network 2."));
		}

		if (strlen(cfg::wlanpwd_3) != 0)
		{
			wifiMulti.addAP(cfg::wlanssid_3, cfg::wlanpwd_3);
			//connectTimeOutPerAP = WIFI_CONNECT_TIMEOUT_MS;
			debug_outln_info(F("Set WiFi Network 3."));
		}
	}
	else
	{
		debug_outln_info(F("Register to Single WiFi Network."));
	}

	waitForMultiWiFiToConnect( maxRetries,  connectTimeOutPerAP );
}

/*****************************************************************
 * WiFi auto connecting script                                   *
 *****************************************************************/

static WiFiEventHandler disconnectEventHandler;

/*****************************************************************
*	connect to Wifi network.									 *
******************************************************************/
static void connectWifi()
{
	display_debug(F("Connecting to"), String(cfg::wlanssid));

#if defined(ESP8266)
	// Enforce Rx/Tx calibration
	system_phy_set_powerup_option(1);

	// 20dBM == 100mW == max tx power allowed in europe
	WiFi.setOutputPower(20.0f);

	if (cfg::powersave) 
	{
		WiFi.setSleepMode(WIFI_MODEM_SLEEP);
	} 
	else 
	{
		WiFi.setSleepMode(WIFI_NONE_SLEEP);
	}

	WiFi.setPhyMode(WIFI_PHY_MODE_11N);
	delay(100);

	disconnectEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &evt)
															{ last_disconnect_reason = evt.reason; });
#endif

	if (WiFi.getAutoConnect())
	{
		WiFi.setAutoConnect(false);
	}

	if (!WiFi.getAutoReconnect())
	{
		WiFi.setAutoReconnect(true);
	}

	// Use 13 channels for connect to known AP
	wifi_country_t wifi;
	wifi.policy = WIFI_COUNTRY_POLICY_MANUAL;
	strcpy(wifi.cc, INTL_LANG);
	wifi.nchan = 13;
	wifi.schan = 1;

#if defined(ESP8266)
	wifi_set_country(&wifi);
#endif

	WiFi.mode(WIFI_STA);

#if defined(ESP8266)
	if (cfg::has_fix_ip)
	{
		debug_outln_info(F("IP is static?"));
		debug_outln_info(F("IP adres "), String(cfg::static_ip));
		debug_outln_info(F("Gateway server "), String(cfg::static_gateway));
		debug_outln_info(F("subnet "), String(cfg::static_subnet));
		debug_outln_info(F("DNS server "), String(cfg::static_dns));
	}
	
	WiFi.hostname(cfg::fs_ssid);

	if (cfg::has_fix_ip &&
		addr_static_ip.fromString(cfg::static_ip) && 
		addr_static_subnet.fromString(cfg::static_subnet) && 
		addr_static_gateway.fromString(cfg::static_gateway) && 
		addr_static_dns.fromString(cfg::static_dns))
	{
		//WiFi.config(addr_static_ip, addr_static_gateway, addr_static_subnet, addr_static_dns, addr_static_dns);
		WiFi.config(addr_static_ip, addr_static_gateway, addr_static_subnet, addr_static_dns);
		
		WiFi.begin(cfg::wlanssid, cfg::wlanpwd); 				// Open/Start WiFI coonection to router/modem.

		waitForWifiToConnect(40);
		debug_outln_info(emptyString);
	}
	else
	{
		// Register multi WiFi networks.
		RegisterMultiWiFiNetworks(WIFI_MAX_RETRY);
	}
#endif

#if defined(ESP32)
	WiFi.setHostname(cfg::fs_ssid);
	WiFi.begin(cfg::wlanssid, cfg::wlanpwd); 				// Open/Start WiFI coonection to router/modem.

	waitForWifiToConnect(40);
	debug_outln_info(emptyString);
#endif

	//debug_outln_info(FPSTR(DBG_TXT_CONNECTING_TO), cfg::wlanssid);
	debug_outln_info(FPSTR(DBG_TXT_CONNECTING_TO), WiFi.SSID());

	if (WiFi.status() != WL_CONNECTED)
	{
		String fss(cfg::fs_ssid);
		display_debug(fss.substring(0, 16), fss.substring(16));

		wifi.policy = WIFI_COUNTRY_POLICY_AUTO;

#if defined(ESP8266)
		wifi_set_country(&wifi);
#endif

		wifiConfig();

		if (WiFi.status() != WL_CONNECTED)
		{
			waitForWifiToConnect(20);
			debug_outln_info(emptyString);
		}
	}

	debug_outln_info(F("WiFi connected, IP is: "), WiFi.localIP().toString());
	last_signal_strength = WiFi.RSSI();

	if (MDNS.begin(cfg::fs_ssid))
	{// setUp Configuration Server.
		MDNS.addService("http", "tcp", 80);
		MDNS.addServiceTxt("http", "tcp", "PATH", "/config");
	}
}

/*

*/
static WiFiClient *getNewLoggerWiFiClient(const LoggerEntry logger)
{

	WiFiClient *_client;

	if (loggerConfigs[logger].session)
	{
		_client = new WiFiClientSecure;

#if defined(ESP8266)
		static_cast<WiFiClientSecure *>(_client)->setSession(loggerConfigs[logger].session);
		static_cast<WiFiClientSecure *>(_client)->setBufferSizes(1024, TCP_MSS > 1024 ? 2048 : 1024);
		static_cast<WiFiClientSecure *>(_client)->setSSLVersion(BR_TLS12, BR_TLS12);

		switch (logger)
		{
			case Loggeraircms:
			case LoggerInflux:
			case LoggerCustom:
			case LoggerFSapp:
				static_cast<WiFiClientSecure *>(_client)->setInsecure();
				break;

			default:
				configureCACertTrustAnchor(static_cast<WiFiClientSecure *>(_client));
		}
#endif

	}
	else
	{
		_client = new WiFiClient;
	}

	return _client;
}

/*
	ESP8266 serial speed to SIM7000 = Default baud rate is 115200 bps

	NOTE: Software serial is not reliable on 115200 baud and therefore changes it to a lower value. 
		  9600 works well in almost all applications, but 115200 works great with Hardware serial.
*/
#define LTEMODEM_BAUD	9600
#define SERIALSIM_BAUD	115200

static boolean SIM700LTEConnect() 
{
	debug_outln_info(F("SIM700 Connecting to "), String(cfg::wlanssid));	// ???

	pinMode(SIM_PIN_PWR, OUTPUT);						// Set Power-On/Off SIM7000 board.

	serialSIM.begin(SERIALSIM_BAUD, SWSERIAL_8N1, SIM_PIN_RX, SIM_PIN_TX);	// start with default SIM7000 shield baud rate.
	delay(100);
	LTEmodem.setBaud(LTEMODEM_BAUD);					// Set LTEmodem baud rate to lower value.
	delay(100);
	serialSIM.begin(LTEMODEM_BAUD, SWSERIAL_8N1);		// set same baud value for SIM7000 shield.

	// Restart takes internal quite some time.
	//LTEmodem.restart();
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
		display_debug(F(" fail"),"");
		//delay(10000);
		return false;
	}

	debug_outln_info(F(" success"),"");

	if (LTEmodem.isGprsConnected())
	{
		debug_outln_info(F("GPRS connected."),"");
	}

	return true;
}

/*****************************************************************
 * send data to rest api => By Wifi                              *
 * return: total send time										 *
 *****************************************************************/
static unsigned long sendData(const LoggerEntry logger, const String &data, const int pin,
							  const char *host, const char *url)
{
	unsigned long start_send = millis();
	const __FlashStringHelper *contentType;
	int result = 0;

	String s_Host(FPSTR(host));
	String s_url(FPSTR(url));

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

	// create 'WiFiClient' instance.
	std::unique_ptr<WiFiClient> _client(getNewLoggerWiFiClient(logger));	

	HTTPClient http;
	http.setTimeout(20 * 1000);
	http.setUserAgent(SOFTWARE_VERSION + '/' + esp_chipid + '/' + esp_mac_id);
	http.setReuse(false);
	bool send_success = false;

	if (logger == LoggerCustom && (*cfg::user_custom || *cfg::pwd_custom))
	{
		http.setAuthorization(cfg::user_custom, cfg::pwd_custom);
	}

	if (logger == LoggerInflux && (*cfg::user_influx || *cfg::pwd_influx))
	{
		http.setAuthorization(cfg::user_influx, cfg::pwd_influx);
	}

	if (http.begin(*_client, s_Host, loggerConfigs[logger].destport, s_url, !!loggerConfigs[logger].session))
	{
		http.addHeader(F("Content-Type"), contentType);
		http.addHeader(F("X-Sensor"), String(F(SENSOR_BASENAME)) + esp_chipid);
		http.addHeader(F("X-MAC-ID"), String(F(SENSOR_BASENAME)) + esp_mac_id);

		if (pin)
		{
			http.addHeader(F("X-PIN"), String(pin));
		}

		// POST sensor data to sensor.community server.
		result = http.POST(data);

		if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED)
		{
			debug_outln_info(F("Succeeded - "), s_Host);
			send_success = true;
		}
		else if (result >= HTTP_CODE_BAD_REQUEST)
		{
			debug_outln_info(F("Request failed with error: "), String(result));
			debug_outln_info(F("Details:"), http.getString());
		}

		http.end();
	}
	else
	{
		debug_outln_info(F("Failed connecting to "), s_Host);
	}

	if (!send_success && result != 0)
	{
		loggerConfigs[logger].errors++;
		last_sendData_returncode = result;
	}

	return millis() - start_send;
}

/*****************************************************************
 * send single sensor data to sensor.community api               *
 *****************************************************************/
static unsigned long sendSensorCommunity(const String &data, const int pin, const __FlashStringHelper *sensorname, const char *replace_str)
{
	unsigned long sum_send_time = 0;

	if (cfg::send2dusti && data.length())
	{
		RESERVE_STRING(data_sensorcommunity, LARGE_STR);
		data_sensorcommunity = FPSTR(data_first_part);

							// DBG_TXT_SENDING_TO
		debug_outln_info(F("## Sending to sensor.community - "), FPSTR((String(sensorname) + String(", PIN: ") + String(pin)).c_str()) );

		data_sensorcommunity += data;
		data_sensorcommunity.remove(data_sensorcommunity.length() - 1);		// remove ',' char.
		data_sensorcommunity.replace(replace_str, emptyString);
		data_sensorcommunity += "]}";

		sum_send_time = sendData(LoggerSensorCommunity, data_sensorcommunity, pin, HOST_SENSORCOMMUNITY, URL_SENSORCOMMUNITY);

		debug_outln_info( F("SensorCommunity data: "), data_sensorcommunity);
	}

	return sum_send_time;
}

/*****************************************************************
 * send data to mqtt api                                         *
 * return: total working/send time.								 *
/*****************************************************************/
static unsigned long sendmqtt(const String &data)
{
#if defined(ESP8266)

	starttime_MQTT = millis();

	if ( !mqtt_client.connected())
	{// after x time MQTT connection will be lost wifi connection.... but why ????
		setup_mqtt_broker( cfg::mqtt_server, cfg::mqtt_port);
	}

	if (mqtt_client.connected())
	{
		// data is ~ 1550 bytes, max_size for publish is MQTT_MAX_PACKET_SIZE (128)
		// upgrading to ArduinoJson version 6.21.2.
		DynamicJsonDocument json2data(JSON_BUFFER_SIZE);
		DeserializationError errorJson = deserializeJson(json2data, data);

		if (!errorJson)
		{
			String key, val, payload, header, status_header, payload_status, mqtt_error;
	
			payload = "{";

			for (JsonObject measurement : json2data[FPSTR(JSON_SENSOR_DATA_VALUES)].as<JsonArray>())
			{
				key = measurement["value_type"].as<const char *>();
				val = measurement["value"].as<const char *>();

				int spc = val.indexOf(' ');

				if (spc >= 0)
				{
					val = val.substring(0, spc);
				}

				// Format mqtt message
				payload += "\"" + key + "\":\"" + val + "\",";
				//payload += "\"sensor\":\"" + key + "\",\"value\":" + val + ",";

			}

			header = mqtt_header;
			header += "/sensor";

			payload.remove(payload.length() - 1, 1);	// delete last char ','.
			payload += "}";								// set end char. Json format

			debug_outln_info(F("\npublishing To MQTT Broker = ... "));
			debug_outln_info(F("- topic = "), (String &)header);
			debug_outln_info(F("- payload = "), (String &)payload);

			if (mqtt_client.publish(header.c_str(), payload.c_str()))
			{
				debug_outln_info(F("payload send ok..."));
				mqtt_error = "ok";
			}
			else
			{
				debug_outln_info(F("payload send failed..."));
				mqtt_error = "failed";
			}

			status_header = mqtt_header;
			status_header += "/status";

			payload_status = "{\"";
			payload_status += FPSTR(INTL_STATIC_IP);
			payload_status += "\":\"";
			payload_status += WiFi.localIP().toString();
			payload_status += "\",\"";
			payload_status += INTL_FIRMWARE;
			payload_status += "\":\"";
			payload_status += SOFTWARE_VERSION_STR;
			payload_status += "/";
			payload_status += INTL_LANG;
			payload_status += " ";
			payload_status += __DATE__;
			payload_status += "\",\"";
			payload_status += FPSTR(INTL_MQTT_STAT);
			payload_status += "\":\"" + mqtt_error + "\"}";

			debug_outln_info(F("- status topic = "), (String &)status_header);
			debug_outln_info(F("- status payload = "), (String &)payload_status);

			if(mqtt_client.publish(status_header.c_str(), payload_status.c_str()))
			{
				debug_outln_info(F("Status send ok..."));
				//mqtt_error = "ok";
			}
			else
			{
				debug_outln_info(F("Status send failed..."));
				//mqtt_error = "failed";
			}

			// default LWT online
			//debug_outln_info(F("Send online LWT"));
			debug_outln_info(F("- LWT topic = "), mqtt_lwt_header);

			String payload_mess_on = INTL_ONLINE;
			if( mqtt_client.publish(mqtt_lwt_header, payload_mess_on.c_str()))
			{
				debug_outln_info(F("lwt send ok..."));
				//mqtt_error = "ok";
			}
			else
			{
				debug_outln_info(F("lwt send failed..."));
				//mqtt_error = "failed";
			}
		}
	}

	return micros() - starttime_MQTT;
#endif
}

/*****************************************************************
 * create influxdb string from data                              *
 *****************************************************************/
static void create_influxdb_string_from_data(String &data_4_influxdb, const String &data)
{
	debug_outln_verbose(F("Parse JSON for influx DB: "), data);
	DynamicJsonDocument json2data(JSON_BUFFER_SIZE);
	DeserializationError err = deserializeJson(json2data, data);

	if (!err)
	{
		data_4_influxdb += cfg::measurement_name_influx;
		data_4_influxdb += F(",node=" SENSOR_BASENAME);
		data_4_influxdb += esp_chipid + " ";

		for (JsonObject measurement : json2data[FPSTR(JSON_SENSOR_DATA_VALUES)].as<JsonArray>())
		{
			data_4_influxdb += measurement["value_type"].as<const char *>();
			data_4_influxdb += "=";

			if (isNumeric(measurement["value"]))
			{
				// send numerics without quotes
				data_4_influxdb += measurement["value"].as<const char *>();
			}
			else
			{
				// quote string values
				data_4_influxdb += "\"";
				data_4_influxdb += measurement["value"].as<const char *>();
				data_4_influxdb += "\"";
			}

			data_4_influxdb += ",";
		}

		if ((unsigned)(data_4_influxdb.lastIndexOf(',') + 1) == data_4_influxdb.length())
		{
			data_4_influxdb.remove(data_4_influxdb.length() - 1);
		}

		data_4_influxdb += '\n';
	}
	else
	{
		debug_outln_error(FPSTR(DBG_TXT_DATA_READ_FAILED));
	}
}

/*****************************************************************
 * send data as csv to serial out                                *
 *****************************************************************/
static void send_csv(const String &data)
{
	DynamicJsonDocument json2data(JSON_BUFFER_SIZE);
	DeserializationError err = deserializeJson(json2data, data);
	debug_outln_info(F("CSV Output: "), data);

	if (!err)
	{
		String headline = F("Timestamp_ms;");
		String valueline(act_milli);
		valueline += ';';

		for (JsonObject measurement : json2data[FPSTR(JSON_SENSOR_DATA_VALUES)].as<JsonArray>())
		{
			headline += measurement["value_type"].as<const char *>();
			headline += ';';
			valueline += measurement["value"].as<const char *>();
			valueline += ';';
		}

		static bool first_csv_line = true;
		if (first_csv_line)
		{
			if (headline.length() > 0)
			{
				headline.remove(headline.length() - 1);
			}

			Debug.println(headline);
			first_csv_line = false;
		}

		if (valueline.length() > 0)
		{
			valueline.remove(valueline.length() - 1);
		}

		// send "valueline" as csv to serial port.
		Debug.println(valueline);
	}
	else
	{
		debug_outln_error(FPSTR(DBG_TXT_DATA_READ_FAILED));
	}
}

/*****************************************************************
 * read DHT22 sensor values                                      *
 *****************************************************************/
static void fetchSensorDHT(String &s)
{
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_DHT22));

	// Check if valid number if non NaN (not a number) will be send.
	last_value_DHT_T = -128;
	last_value_DHT_H = -1;

	int count = 0;
	const int MAX_ATTEMPTS = 5;

	while ((count++ < MAX_ATTEMPTS))
	{
		auto t = dht.readTemperature();
		auto h = dht.readHumidity();

		if (isnan(t) || isnan(h))
		{
			delay(100);
			t = dht.readTemperature(false);
			h = dht.readHumidity();
		}

		if (isnan(t) || isnan(h))
		{
			debug_outln_error(F("DHT11/DHT22 read failed"));
		}
		else
		{
			debug_outln_info(FPSTR(DBG_TXT_SEP));

			last_value_DHT_T = t + readCorrectionOffset(cfg::temp_correction);
			last_value_DHT_H = h;
			add_Value2Json(s, F("temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_DHT_T);
			add_Value2Json(s, F("humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_DHT_H);

			debug_outln_info(FPSTR(DBG_TXT_SEP));

			break;
		}
	}
	

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_DHT22));
}

/*****************************************************************
 * read HTU21D sensor values                                     *
 *****************************************************************/
static void fetchSensorHTU21D(String &s)
{
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_HTU21D));

	const auto t = htu21d.readTemperature();
	const auto h = htu21d.readHumidity();
	if (isnan(t) || isnan(h))
	{
		last_value_HTU21D_T = -128.0;
		last_value_HTU21D_H = -1.0;
		debug_outln_error(F("HTU21D read failed"));
	}
	else
	{
		debug_outln_info(FPSTR(DBG_TXT_SEP));

		last_value_HTU21D_T = t;
		last_value_HTU21D_H = h;
		add_Value2Json(s, F("HTU21D_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_HTU21D_T);
		add_Value2Json(s, F("HTU21D_humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_HTU21D_H);

		debug_outln_info(FPSTR(DBG_TXT_SEP));
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_HTU21D));
}

/*****************************************************************
 * read BMP180 sensor values                                     *
 *****************************************************************/
static void fetchSensorBMP(String &s)
{
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_BMP180));

	const auto p = bmp.readPressure();
	const auto t = bmp.readTemperature();
	if (isnan(p) || isnan(t))
	{
		last_value_BMP_T = -128.0;
		last_value_BMP_P = -1.0;
		debug_outln_error(F("BMP180 read failed"));
	}
	else
	{
		debug_outln_info(FPSTR(DBG_TXT_SEP));

		last_value_BMP_T = t;
		last_value_BMP_P = p;
		add_Value2Json(s, F("BMP_pressure"), FPSTR(DBG_TXT_PRESSURE), last_value_BMP_P);
		add_Value2Json(s, F("BMP_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_BMP_T);

		debug_outln_info(FPSTR(DBG_TXT_SEP));
	}


	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_BMP180));
}

/*****************************************************************
 * read SHT3x sensor values                                      *
 *****************************************************************/
static void fetchSensorSHT3x(String &s)
{
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SHT3X));

	const auto t = sht3x.readTemperature();
	const auto h = sht3x.readHumidity();

	if (isnan(h) || isnan(t))
	{
		last_value_SHT3X_T = -128.0;
		last_value_SHT3X_H = -1.0;
		debug_outln_error(F("SHT3X read failed"));
	}
	else
	{
		debug_outln_info(FPSTR(DBG_TXT_SEP));

		last_value_SHT3X_T = t;
		last_value_SHT3X_H = h;
		add_Value2Json(s, F("SHT3X_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_SHT3X_T);
		add_Value2Json(s, F("SHT3X_humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_SHT3X_H);

		debug_outln_info(FPSTR(DBG_TXT_SEP));
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SHT3X));
}

/*****************************************************************
 * read SCD30 sensor values                                      *
 *****************************************************************/
static void fetchSensorSCD30(String &s)
{
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SCD30));

	const auto t = scd30.getTemperature();
	const auto h = scd30.getHumidity();
	const auto c = scd30.getCO2();

	if (isnan(h) || isnan(t) || isnan(c))
	{
		last_value_SCD30_T = -128.0;
		last_value_SCD30_H = -1.0;
		last_value_SCD30_CO2 = 0;
		debug_outln_error(F("SCD30 read failed"));
	}
	else
	{
		debug_outln_info(FPSTR(DBG_TXT_SEP));

		last_value_SCD30_T = t;
		last_value_SCD30_H = h;
		last_value_SCD30_CO2 = c;

		add_Value2Json(s, F("SCD30_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_SCD30_T);
		add_Value2Json(s, F("SCD30_humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_SCD30_H);
		add_Value2Json(s, F("SCD30_co2_ppm"), FPSTR(DBG_TXT_CO2PPM), last_value_SCD30_CO2);

		debug_outln_info(FPSTR(DBG_TXT_SEP));
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SCD30));
}

/*****************************************************************
 * read BMP280/BME280 sensor values                              *
 *****************************************************************/
static void fetchSensorBMX280(String &s)
{
	const char *const sensor_name = (bmx280.sensorID() == BME280_SENSOR_ID) ? SENSORS_BME280 : SENSORS_BMP280;

	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(sensor_name));

	bmx280.takeForcedMeasurement();
	const auto t = bmx280.readTemperature();
	const auto p = bmx280.readPressure();
	const auto h = bmx280.readHumidity();

	if (isnan(t) || isnan(p))
	{
		last_value_BMX280_T = -128.0;
		last_value_BMX280_P = -1.0;
		last_value_BME280_H = -1.0;

		debug_outln_error(F("BMP/BME280 read failed"));
	}
	else
	{
		debug_outln_info(FPSTR(DBG_TXT_SEP));

		last_value_BMX280_T = t + readCorrectionOffset(cfg::temp_correction);

		last_value_BMX280_P = p;

		if (bmx280.sensorID() == BME280_SENSOR_ID)
		{
			add_Value2Json(s, F("BME280_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_BMX280_T);
			add_Value2Json(s, F("BME280_pressure"), FPSTR(DBG_TXT_PRESSURE), last_value_BMX280_P);
			last_value_BME280_H = h;
			add_Value2Json(s, F("BME280_humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_BME280_H);
		}
		else
		{
			add_Value2Json(s, F("BMP280_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_BMX280_T);
			add_Value2Json(s, F("BMP280_pressure"), FPSTR(DBG_TXT_PRESSURE), last_value_BMX280_P);
		}

		debug_outln_info(FPSTR(DBG_TXT_SEP));
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(sensor_name));
}

/*****************************************************************
 * read DS18B20 sensor values                                    *
 *****************************************************************/
static void fetchSensorDS18B20(String &s)
{
	float t;
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_DS18B20));

	// it's very unlikely (-127: impossible) to get these temperatures in reality. Most times this means that the sensor is currently faulty
	// try 5 times to read the sensor, otherwise fail
	const int MAX_ATTEMPTS = 5;
	int count = 0;
	do
	{
		ds18b20.requestTemperatures();
		// for now, we want to read only the first sensor
		t = ds18b20.getTempCByIndex(0);
		++count;
		debug_outln_info(F("DS18B20 trying...."));
	} while (count < MAX_ATTEMPTS && (isnan(t) || t >= 85.0f || t <= (-127.0f)));

	if (count == MAX_ATTEMPTS)
	{
		last_value_DS18B20_T = -128.0;
		debug_outln_error(F("DS18B20 read failed"));
	}
	else
	{
		debug_outln_info(FPSTR(DBG_TXT_SEP));

		last_value_DS18B20_T = t + readCorrectionOffset(cfg::temp_correction);
		add_Value2Json(s, F("DS18B20_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_DS18B20_T);

		debug_outln_info(FPSTR(DBG_TXT_SEP));
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_DS18B20));
}

/*****************************************************************
 * read SDS011 sensor values                                     *
 *****************************************************************/
static void fetchSensorSDS(String &s)
{
	if (cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS) &&
		msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS)))
	{
		if (is_SDS_running)
		{
			is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
		}
	}
	else
	{
		if (!is_SDS_running)
		{
			is_SDS_running = SDS_cmd(PmSensorCmd::Start);
			SDS_waiting_for = SDS_REPLY_HDR;
		}

		while (serialSDS.available() >= SDS_waiting_for)
		{
			const uint8_t constexpr hdr_measurement[2] = {0xAA, 0xC0};
			uint8_t data[8];

			switch (SDS_waiting_for)
			{
			case SDS_REPLY_HDR:
				if (serialSDS.find(hdr_measurement, sizeof(hdr_measurement)))
					SDS_waiting_for = SDS_REPLY_BODY;
				break;

			case SDS_REPLY_BODY:
				debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SDS011));
				
				if (serialSDS.readBytes(data, sizeof(data)) == sizeof(data) && SDS_checksum_valid(data))
				{
					uint32_t pm25_serial = data[0] | (data[1] << 8);
					uint32_t pm10_serial = data[2] | (data[3] << 8);

					if (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_SDS_MS))
					{
						debug_outln_info(FPSTR(DBG_TXT_SEP));

						sds_pm10_sum += pm10_serial;
						sds_pm25_sum += pm25_serial;
						UPDATE_MIN_MAX(sds_pm10_min, sds_pm10_max, pm10_serial);
						UPDATE_MIN_MAX(sds_pm25_min, sds_pm25_max, pm25_serial);
						debug_outln_verbose(F("PM10 (sec.) : "), String(pm10_serial / 10.0f));
						debug_outln_verbose(F("PM2.5 (sec.): "), String(pm25_serial / 10.0f));
						sds_val_count++;

						debug_outln_info(FPSTR(DBG_TXT_SEP));
					}
				}

				debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SDS011));

				SDS_waiting_for = SDS_REPLY_HDR;
				break;
			}
		}
	}

	if (send_now)
	{
		last_value_SDS_P1 = -1;
		last_value_SDS_P2 = -1;

		if (sds_val_count > 2)
		{
			sds_pm10_sum = sds_pm10_sum - sds_pm10_min - sds_pm10_max;
			sds_pm25_sum = sds_pm25_sum - sds_pm25_min - sds_pm25_max;
			sds_val_count = sds_val_count - 2;
		}
		
		if (sds_val_count > 0)
		{
			debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SDS011));
			debug_outln_info(FPSTR(DBG_TXT_SEP));

			last_value_SDS_P1 = float(sds_pm10_sum) / (sds_val_count * 10.0f);
			last_value_SDS_P2 = float(sds_pm25_sum) / (sds_val_count * 10.0f);
			add_Value2Json(s, F("SDS_P1"), F("PM10:  "), last_value_SDS_P1);
			add_Value2Json(s, F("SDS_P2"), F("PM2.5: "), last_value_SDS_P2);
			debug_outln_info(FPSTR(DBG_TXT_SEP));

			if (sds_val_count < 3)
			{
				SDS_error_count++;
			}

			debug_outln_info(FPSTR(DBG_TXT_SEP));
			debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SDS011));
		}
		else
		{
			SDS_error_count++;
		}

		sds_pm10_sum = 0;
		sds_pm25_sum = 0;
		sds_val_count = 0;
		sds_pm10_max = 0;
		sds_pm10_min = 20000;
		sds_pm25_max = 0;
		sds_pm25_min = 20000;

		if ((cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS)))
		{

			if (is_SDS_running)
			{
				is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
			}
		}
	}
}

/*****************************************************************
 * read Plantronic PM sensor sensor values                       *
 *****************************************************************/
static __noinline void fetchSensorPMS(String &s)
{
	char buffer;
	int value;
	int len = 0;
	int pm1_serial = 0;
	int pm10_serial = 0;
	int pm25_serial = 0;
	int checksum_is = 0;
	int checksum_should = 0;
	bool checksum_ok = false;
	int frame_len = 24; // min. frame length

	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_PMSx003));

	if (msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS)))
	{
		if (is_PMS_running)
		{
			is_PMS_running = PMS_cmd(PmSensorCmd::Stop);
		}
	}
	else
	{
		if (!is_PMS_running)
		{
			is_PMS_running = PMS_cmd(PmSensorCmd::Start);
		}

		while (serialSDS.available() > 0)
		{
			buffer = serialSDS.read();
			debug_outln(String(len) + " - " + String(buffer, DEC) + " - " + String(buffer, HEX) + " - " + int(buffer) + " .", DEBUG_MAX_INFO);

			//		"aa" = 170, "ab" = 171, "c0" = 192
			value = int(buffer);
			switch (len)
			{
			case 0:
				if (value != 66)
				{
					len = -1;
				};
				break;
			case 1:
				if (value != 77)
				{
					len = -1;
				};
				break;
			case 2:
				checksum_is = value;
				break;
			case 3:
				frame_len = value + 4;
				break;
			case 10:
				pm1_serial += (value << 8);
				break;
			case 11:
				pm1_serial += value;
				break;
			case 12:
				pm25_serial = (value << 8);
				break;
			case 13:
				pm25_serial += value;
				break;
			case 14:
				pm10_serial = (value << 8);
				break;
			case 15:
				pm10_serial += value;
				break;
			case 22:
				if (frame_len == 24)
				{
					checksum_should = (value << 8);
				};
				break;
			case 23:
				if (frame_len == 24)
				{
					checksum_should += value;
				};
				break;
			case 30:
				checksum_should = (value << 8);
				break;
			case 31:
				checksum_should += value;
				break;
			}
			if ((len > 2) && (len < (frame_len - 2)))
			{
				checksum_is += value;
			}
			len++;

			if (len == frame_len)
			{
				debug_outln_verbose(FPSTR(DBG_TXT_CHECKSUM_IS), String(checksum_is + 143));
				debug_outln_verbose(FPSTR(DBG_TXT_CHECKSUM_SHOULD), String(checksum_should));

				if (checksum_should == (checksum_is + 143))
				{
					checksum_ok = true;
				}
				else
				{
					len = 0;
				}

				if (checksum_ok && (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_SDS_MS)))
				{
					if ((!isnan(pm1_serial)) && (!isnan(pm10_serial)) && (!isnan(pm25_serial)))
					{
						pms_pm1_sum += pm1_serial;
						pms_pm10_sum += pm10_serial;
						pms_pm25_sum += pm25_serial;

						UPDATE_MIN_MAX(pms_pm1_min, pms_pm1_max, pm1_serial);
						UPDATE_MIN_MAX(pms_pm25_min, pms_pm25_max, pm25_serial);
						UPDATE_MIN_MAX(pms_pm10_min, pms_pm10_max, pm10_serial);

						debug_outln_verbose(F("PM1 (sec.): "), String(pm1_serial));
						debug_outln_verbose(F("PM2.5 (sec.): "), String(pm25_serial));
						debug_outln_verbose(F("PM10 (sec.) : "), String(pm10_serial));
						pms_val_count++;
					}

					len = 0;
					checksum_ok = false;
					pm1_serial = 0;
					pm10_serial = 0;
					pm25_serial = 0;
					checksum_is = 0;
				}
			}

			yield();
		}
	}

	if (send_now)
	{
		last_value_PMS_P0 = -1;
		last_value_PMS_P1 = -1;
		last_value_PMS_P2 = -1;
		if (pms_val_count > 2)
		{
			pms_pm1_sum = pms_pm1_sum - pms_pm1_min - pms_pm1_max;
			pms_pm10_sum = pms_pm10_sum - pms_pm10_min - pms_pm10_max;
			pms_pm25_sum = pms_pm25_sum - pms_pm25_min - pms_pm25_max;
			pms_val_count = pms_val_count - 2;
		}

		if (pms_val_count > 0)
		{
			debug_outln_info(FPSTR(DBG_TXT_SEP));
			last_value_PMS_P0 = float(pms_pm1_sum) / float(pms_val_count);
			last_value_PMS_P1 = float(pms_pm10_sum) / float(pms_val_count);
			last_value_PMS_P2 = float(pms_pm25_sum) / float(pms_val_count);
			add_Value2Json(s, F("PMS_P0"), F("PM1:   "), last_value_PMS_P0);
			add_Value2Json(s, F("PMS_P1"), F("PM10:  "), last_value_PMS_P1);
			add_Value2Json(s, F("PMS_P2"), F("PM2.5: "), last_value_PMS_P2);
			debug_outln_info(FPSTR(DBG_TXT_SEP));
		}

		pms_pm1_sum = 0;
		pms_pm10_sum = 0;
		pms_pm25_sum = 0;
		pms_val_count = 0;
		pms_pm1_max = 0;
		pms_pm1_min = 20000;
		pms_pm10_max = 0;
		pms_pm10_min = 20000;
		pms_pm25_max = 0;
		pms_pm25_min = 20000;

		if (cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))
		{
			is_PMS_running = PMS_cmd(PmSensorCmd::Stop);
		}
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_PMSx003));
}

/*****************************************************************
 * read Honeywell PM sensor sensor values                        *
 *****************************************************************/
static __noinline void fetchSensorHPM(String &s)
{
	char buffer;
	int value;
	int len = 0;
	int pm10_serial = 0;
	int pm25_serial = 0;
	int checksum_is = 0;
	int checksum_should = 0;
	bool checksum_ok = false;

	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_HPM));

	if (msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS)))
	{
		if (is_HPM_running)
		{
			is_HPM_running = HPM_cmd(PmSensorCmd::Stop);
		}
	}
	else
	{
		if (!is_HPM_running)
		{
			is_HPM_running = HPM_cmd(PmSensorCmd::Start);
		}

		while (serialSDS.available() > 0)
		{
			buffer = serialSDS.read();
			debug_outln(String(len) + " - " + String(buffer, DEC) + " - " + String(buffer, HEX) + " - " + int(buffer) + " .", DEBUG_MAX_INFO);
			//			"aa" = 170, "ab" = 171, "c0" = 192
			value = int(buffer);
			switch (len)
			{
			case 0:
				if (value != 66)
				{
					len = -1;
				};
				break;
			case 1:
				if (value != 77)
				{
					len = -1;
				};
				break;
			case 2:
				checksum_is = value;
				break;
			case 6:
				pm25_serial += (value << 8);
				break;
			case 7:
				pm25_serial += value;
				break;
			case 8:
				pm10_serial = (value << 8);
				break;
			case 9:
				pm10_serial += value;
				break;
			case 30:
				checksum_should = (value << 8);
				break;
			case 31:
				checksum_should += value;
				break;
			}
			if (len > 2 && len < 30)
			{
				checksum_is += value;
			}
			len++;
			if (len == 32)
			{
				debug_outln_verbose(FPSTR(DBG_TXT_CHECKSUM_IS), String(checksum_is + 143));
				debug_outln_verbose(FPSTR(DBG_TXT_CHECKSUM_SHOULD), String(checksum_should));

				if (checksum_should == (checksum_is + 143))
				{
					checksum_ok = true;
				}
				else
				{
					len = 0;
				}

				if (checksum_ok && (long(msSince(starttime)) > (long(cfg::sending_intervall_ms) - long(READINGTIME_SDS_MS))))
				{
					if ((!isnan(pm10_serial)) && (!isnan(pm25_serial)))
					{
						hpm_pm10_sum += pm10_serial;
						hpm_pm25_sum += pm25_serial;
						UPDATE_MIN_MAX(hpm_pm10_min, hpm_pm10_max, pm10_serial);
						UPDATE_MIN_MAX(hpm_pm25_min, hpm_pm25_max, pm25_serial);
						debug_outln_verbose(F("PM2.5 (sec.): "), String(pm25_serial));
						debug_outln_verbose(F("PM10 (sec.) : "), String(pm10_serial));
						hpm_val_count++;
					}

					len = 0;
					checksum_ok = false;
					pm10_serial = 0;
					pm25_serial = 0;
					checksum_is = 0;
				}
			}

			yield();
		}
	}

	if (send_now)
	{
		last_value_HPM_P1 = -1.0f;
		last_value_HPM_P2 = -1.0f;
		if (hpm_val_count > 2)
		{
			hpm_pm10_sum = hpm_pm10_sum - hpm_pm10_min - hpm_pm10_max;
			hpm_pm25_sum = hpm_pm25_sum - hpm_pm25_min - hpm_pm25_max;
			hpm_val_count = hpm_val_count - 2;
		}

		if (hpm_val_count > 0)
		{
			debug_outln_info(FPSTR(DBG_TXT_SEP));
			last_value_HPM_P1 = float(hpm_pm10_sum) / float(hpm_val_count);
			last_value_HPM_P2 = float(hpm_pm25_sum) / float(hpm_val_count);
			add_Value2Json(s, F("HPM_P1"), F("PM2.5: "), last_value_HPM_P1);
			add_Value2Json(s, F("HPM_P2"), F("PM10:  "), last_value_HPM_P2);
			debug_outln_info(FPSTR(DBG_TXT_SEP));
		}

		hpm_pm10_sum = 0;
		hpm_pm25_sum = 0;
		hpm_val_count = 0;
		hpm_pm10_max = 0;
		hpm_pm10_min = 20000;
		hpm_pm25_max = 0;
		hpm_pm25_min = 20000;

		if (cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))
		{
			is_HPM_running = HPM_cmd(PmSensorCmd::Stop);
		}
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_HPM));
}

/*****************************************************************
 * read Tera Sensor Next PM sensor sensor values                 *
 *****************************************************************/
static void fetchSensorNPM(String &s)
{
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_NPM));

	if (cfg::sending_intervall_ms > (WARMUPTIME_NPM_MS + READINGTIME_NPM_MS) && msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_NPM_MS + READINGTIME_NPM_MS)))
	{
		if (is_NPM_running && !cfg::npm_fulltime)
		{
			debug_outln_info(F("Change NPM to stop..."));
			is_NPM_running = NPM_start_stop();
		}
	}
	else
	{
		if (!is_NPM_running && !cfg::npm_fulltime)
		{
			debug_outln_info(F("Change NPM to start..."));
			is_NPM_running = NPM_start_stop();
			NPM_waiting_for_16 = NPM_REPLY_HEADER_16;
		}

		if (is_NPM_running && cfg::npm_fulltime)
		{
			NPM_waiting_for_16 = NPM_REPLY_HEADER_16;
		}

		if (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_NPM_MS))
		{ // DIMINUER LE READING TIME

			debug_outln_info(F("Concentration NPM..."));
			NPM_cmd(PmSensorCmd2::Concentration);
			while (!serialNPM.available())
			{
				debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
			}

			while (serialNPM.available() >= NPM_waiting_for_16)
			{
				const uint8_t constexpr header[2] = {0x81, 0x11};
				uint8_t state[1];
				uint8_t data[12];
				uint8_t checksum[1];
				uint8_t test[16];
				uint16_t N1_serial = 0;
				uint16_t N25_serial = 0;
				uint16_t N10_serial = 0;
				uint16_t pm1_serial = 0;
				uint16_t pm25_serial = 0;
				uint16_t pm10_serial = 0;

				switch (NPM_waiting_for_16)
				{
				case NPM_REPLY_HEADER_16:
					if (serialNPM.find(header, sizeof(header)))
						NPM_waiting_for_16 = NPM_REPLY_STATE_16;
					break;
				case NPM_REPLY_STATE_16:
					serialNPM.readBytes(state, sizeof(state));
					current_state_npm = NPM_state(state[0]);
					NPM_waiting_for_16 = NPM_REPLY_BODY_16;
					break;
				case NPM_REPLY_BODY_16:
					if (serialNPM.readBytes(data, sizeof(data)) == sizeof(data))
					{
						NPM_data_reader(data, 12);
						N1_serial = word(data[0], data[1]);
						N25_serial = word(data[2], data[3]);
						N10_serial = word(data[4], data[5]);

						pm1_serial = word(data[6], data[7]);
						pm25_serial = word(data[8], data[9]);
						pm10_serial = word(data[10], data[11]);

						debug_outln_info(F("Next PM Measure..."));

						debug_outln_verbose(F("PM1 (μg/m3) : "), String(pm1_serial / 10.0f));
						debug_outln_verbose(F("PM2.5 (μg/m3): "), String(pm25_serial / 10.0f));
						debug_outln_verbose(F("PM10 (μg/m3) : "), String(pm10_serial / 10.0f));

						debug_outln_verbose(F("PM1 (pcs/L) : "), String(N1_serial));
						debug_outln_verbose(F("PM2.5 (pcs/L): "), String(N25_serial));
						debug_outln_verbose(F("PM10 (pcs/L) : "), String(N10_serial));
					}
					NPM_waiting_for_16 = NPM_REPLY_CHECKSUM_16;
					break;

				case NPM_REPLY_CHECKSUM_16:
					serialNPM.readBytes(checksum, sizeof(checksum));
					memcpy(test, header, sizeof(header));
					memcpy(&test[sizeof(header)], state, sizeof(state));
					memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
					memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));
					NPM_data_reader(test, 16);
					if (NPM_checksum_valid_16(test))
					{
						debug_outln_info(F("Checksum OK..."));
						npm_pm1_sum += pm1_serial;
						npm_pm25_sum += pm25_serial;
						npm_pm10_sum += pm10_serial;

						npm_pm1_sum_pcs += N1_serial;
						npm_pm25_sum_pcs += N25_serial;
						npm_pm10_sum_pcs += N10_serial;

						UPDATE_MIN_MAX(npm_pm1_min, npm_pm1_max, pm1_serial);
						UPDATE_MIN_MAX(npm_pm25_min, npm_pm25_max, pm25_serial);
						UPDATE_MIN_MAX(npm_pm10_min, npm_pm10_max, pm10_serial);

						UPDATE_MIN_MAX(npm_pm1_min_pcs, npm_pm1_max_pcs, N1_serial);
						UPDATE_MIN_MAX(npm_pm25_min_pcs, npm_pm25_max_pcs, N25_serial);
						UPDATE_MIN_MAX(npm_pm10_min_pcs, npm_pm10_max_pcs, N10_serial);

						npm_val_count++;
						debug_outln(String(npm_val_count), DEBUG_MAX_INFO);
					}
					NPM_waiting_for_16 = NPM_REPLY_HEADER_16;
					break;
				}
			}
		}
	}

	if (send_now)
	{
		last_value_NPM_P0 = -1.0f;
		last_value_NPM_P1 = -1.0f;
		last_value_NPM_P2 = -1.0f;
		last_value_NPM_N1 = -1.0f;
		last_value_NPM_N10 = -1.0f;
		last_value_NPM_N25 = -1.0f;

		if (npm_val_count > 2)
		{
			npm_pm1_sum = npm_pm1_sum - npm_pm1_min - npm_pm1_max;
			npm_pm10_sum = npm_pm10_sum - npm_pm10_min - npm_pm10_max;
			npm_pm25_sum = npm_pm25_sum - npm_pm25_min - npm_pm25_max;
			npm_pm1_sum_pcs = npm_pm1_sum_pcs - npm_pm1_min_pcs - npm_pm1_max_pcs;
			npm_pm10_sum_pcs = npm_pm10_sum_pcs - npm_pm10_min_pcs - npm_pm10_max_pcs;
			npm_pm25_sum_pcs = npm_pm25_sum_pcs - npm_pm25_min_pcs - npm_pm25_max_pcs;
			npm_val_count = npm_val_count - 2;
		}

		if (npm_val_count > 0)
		{
			debug_outln_info(FPSTR(DBG_TXT_SEP));

			last_value_NPM_P0 = float(npm_pm1_sum) / (npm_val_count * 10.0f);
			last_value_NPM_P1 = float(npm_pm10_sum) / (npm_val_count * 10.0f);
			last_value_NPM_P2 = float(npm_pm25_sum) / (npm_val_count * 10.0f);

			last_value_NPM_N1 = float(npm_pm1_sum_pcs) / (npm_val_count * 1000.0f);
			last_value_NPM_N10 = float(npm_pm10_sum_pcs) / (npm_val_count * 1000.0f);
			last_value_NPM_N25 = float(npm_pm25_sum_pcs) / (npm_val_count * 1000.0f);

			add_Value2Json(s, F("NPM_P0"), F("PM1: "), last_value_NPM_P0);
			add_Value2Json(s, F("NPM_P1"), F("PM10:  "), last_value_NPM_P1);
			add_Value2Json(s, F("NPM_P2"), F("PM2.5: "), last_value_NPM_P2);

			add_Value2Json(s, F("NPM_N1"), F("NC1.0: "), last_value_NPM_N1);
			add_Value2Json(s, F("NPM_N10"), F("NC10:  "), last_value_NPM_N10);
			add_Value2Json(s, F("NPM_N25"), F("NC2.5: "), last_value_NPM_N25);

			debug_outln_info(FPSTR(DBG_TXT_SEP));
			
			if (npm_val_count < 3)
			{
				NPM_error_count++;
			}
		}
		else
		{
			NPM_error_count++;
		}

		npm_pm1_sum = 0;
		npm_pm10_sum = 0;
		npm_pm25_sum = 0;

		npm_val_count = 0;

		npm_pm1_max = 0;
		npm_pm1_min = 20000;
		npm_pm10_max = 0;
		npm_pm10_min = 20000;
		npm_pm25_max = 0;
		npm_pm25_min = 20000;

		npm_pm1_sum_pcs = 0;
		npm_pm10_sum_pcs = 0;
		npm_pm25_sum_pcs = 0;

		npm_pm1_max_pcs = 0;
		npm_pm1_min_pcs = 60000;
		npm_pm10_max_pcs = 0;
		npm_pm10_min_pcs = 60000;
		npm_pm25_max_pcs = 0;
		npm_pm25_min_pcs = 60000;

		if (cfg::sending_intervall_ms > (WARMUPTIME_NPM_MS + READINGTIME_NPM_MS))
		{
			debug_outln_info(F("Temperature and humidity in NPM after measure..."));
			current_th_npm = NPM_temp_humi();
			if (is_NPM_running && !cfg::npm_fulltime)
			{
				debug_outln_info(F("Change NPM to stop after measure..."));
				is_NPM_running = NPM_start_stop();
			}
		}
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_NPM));
}

/*****************************************************************
 * read Piera Systems IPS-7100 sensor sensor values                 *
 *****************************************************************/

static void fetchSensorIPS(String &s)
{
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_IPS));

	while (serialIPS.available() > 0)
	{
		serialIPS.read();
	}

	if (cfg::sending_intervall_ms > (WARMUPTIME_IPS_MS + READINGTIME_IPS_MS) && msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_IPS_MS + READINGTIME_IPS_MS)))
	{
		if (is_IPS_running)
		{
			debug_outln_info(F("Change IPS to stop..."));
			IPS_cmd(PmSensorCmd3::Stop);
			is_IPS_running = false;
		}
	}
	else
	{
		if (!is_IPS_running)
		{
			debug_outln_info(F("Change IPS to start..."));
			IPS_cmd(PmSensorCmd3::Start);
			is_IPS_running = true;
		}

		// VIDER LE BUFFER DU START?

		if (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_IPS_MS))
		{ // DIMINUER LE READING TIME

			debug_outln_info(F("Concentration IPS..."));
			String serial_data;
			// String serial_data2;

			// while (Serial.available() > 0) {
			// 	Serial.read();
			// }

			IPS_cmd(PmSensorCmd3::Get);

			if (serialIPS.available() > 0)
			{
				serial_data = serialIPS.readString();
			}

			// while (serialIPS.available() > 0)
			// {
			// 	serialIPS.read();
			// }

			Debug.println(serial_data);

			// 		if(serial_data.length()>240){

			int index1 = serial_data.indexOf("PC0.1,");
			int index2 = serial_data.indexOf(",PC0.3,");
			int index3 = serial_data.indexOf(",PC0.5,");
			int index4 = serial_data.indexOf(",PC1.0,");
			int index5 = serial_data.indexOf(",PC2.5,");
			int index6 = serial_data.indexOf(",PC5.0,");
			int index7 = serial_data.indexOf(",PC10,");
			int index8 = serial_data.indexOf(",PM0.1,");
			int index9 = serial_data.indexOf(",PM0.3,");
			int index10 = serial_data.indexOf(",PM0.5,");
			int index11 = serial_data.indexOf(",PM1.0,");
			int index12 = serial_data.indexOf(",PM2.5,");
			int index13 = serial_data.indexOf(",PM5.0,");
			int index14 = serial_data.indexOf(",PM10,");
			int index15 = serial_data.indexOf(",IPS");

			String N01_serial = serial_data.substring(index1 + 6, index2);
			String N03_serial = serial_data.substring(index2 + 7, index3);
			String N05_serial = serial_data.substring(index3 + 7, index4);
			String N1_serial = serial_data.substring(index4 + 7, index5);
			String N25_serial = serial_data.substring(index5 + 7, index6);
			String N5_serial = serial_data.substring(index6 + 7, index7);
			String N10_serial = serial_data.substring(index7 + 6, index8);

			String pm01_serial = serial_data.substring(index8 + 7, index9 - 6);
			String pm03_serial = serial_data.substring(index9 + 7, index10 - 6);
			String pm05_serial = serial_data.substring(index10 + 7, index11 - 6);
			String pm1_serial = serial_data.substring(index11 + 7, index12 - 6);
			String pm25_serial = serial_data.substring(index12 + 7, index13 - 6);
			String pm5_serial = serial_data.substring(index13 + 7, index14 - 6);
			String pm10_serial = serial_data.substring(index14 + 6, index15 - 6);

			debug_outln_verbose(F("PM0.1 (μg/m3) : "), pm01_serial);
			debug_outln_verbose(F("PM0.3 (μg/m3): "), pm03_serial);
			debug_outln_verbose(F("PM0.5 (μg/m3) : "), pm05_serial);
			debug_outln_verbose(F("PM1 (μg/m3) : "), pm1_serial);
			debug_outln_verbose(F("PM2.5 (μg/m3): "), pm25_serial);
			debug_outln_verbose(F("PM5 (μg/m3) : "), pm5_serial);
			debug_outln_verbose(F("PM10 (μg/m3) : "), pm10_serial);

			debug_outln_verbose(F("PM0.1 (pcs/L) : "), N01_serial);
			debug_outln_verbose(F("PM0.3 (pcs/L): "), N03_serial);
			debug_outln_verbose(F("PM0.5(pcs/L) : "), N05_serial);
			debug_outln_verbose(F("PM1 (pcs/L) : "), N1_serial);
			debug_outln_verbose(F("PM2.5 (pcs/L): "), N25_serial);
			debug_outln_verbose(F("PM5 (pcs/L) : "), N5_serial);
			debug_outln_verbose(F("PM10 (pcs/L) : "), N10_serial);

			ips_pm01_sum += pm01_serial.toFloat();
			ips_pm03_sum += pm03_serial.toFloat();
			ips_pm05_sum += pm05_serial.toFloat();
			ips_pm1_sum += pm1_serial.toFloat();
			ips_pm25_sum += pm25_serial.toFloat();
			ips_pm5_sum += pm5_serial.toFloat();
			ips_pm10_sum += pm10_serial.toFloat();

			// char *ptr;

			// SI EXCEPTION 28 DECLENCHÈE FLASHER SUR AUTRE PRISE USB.

			ips_pm01_sum_pcs += strtoul(N01_serial.c_str(), NULL, 10);
			ips_pm03_sum_pcs += strtoul(N03_serial.c_str(), NULL, 10);
			ips_pm05_sum_pcs += strtoul(N05_serial.c_str(), NULL, 10);
			ips_pm1_sum_pcs += strtoul(N1_serial.c_str(), NULL, 10);
			ips_pm25_sum_pcs += strtoul(N25_serial.c_str(), NULL, 10);
			ips_pm5_sum_pcs += strtoul(N5_serial.c_str(), NULL, 10);
			ips_pm10_sum_pcs += strtoul(N10_serial.c_str(), NULL, 10);

			UPDATE_MIN_MAX(ips_pm01_min, ips_pm01_max, pm01_serial.toFloat());
			UPDATE_MIN_MAX(ips_pm03_min, ips_pm03_max, pm03_serial.toFloat());
			UPDATE_MIN_MAX(ips_pm05_min, ips_pm05_max, pm05_serial.toFloat());
			UPDATE_MIN_MAX(ips_pm1_min, ips_pm1_max, pm1_serial.toFloat());
			UPDATE_MIN_MAX(ips_pm25_min, ips_pm25_max, pm25_serial.toFloat());
			UPDATE_MIN_MAX(ips_pm5_min, ips_pm5_max, pm5_serial.toFloat());
			UPDATE_MIN_MAX(ips_pm10_min, ips_pm10_max, pm10_serial.toFloat());

			UPDATE_MIN_MAX(ips_pm01_min_pcs, ips_pm01_max_pcs, strtoul(N01_serial.c_str(), NULL, 10));
			UPDATE_MIN_MAX(ips_pm03_min_pcs, ips_pm03_max_pcs, strtoul(N03_serial.c_str(), NULL, 10));
			UPDATE_MIN_MAX(ips_pm05_min_pcs, ips_pm05_max_pcs, strtoul(N05_serial.c_str(), NULL, 10));
			UPDATE_MIN_MAX(ips_pm1_min_pcs, ips_pm1_max_pcs, strtoul(N1_serial.c_str(), NULL, 10));
			UPDATE_MIN_MAX(ips_pm25_min_pcs, ips_pm25_max_pcs, strtoul(N25_serial.c_str(), NULL, 10));
			UPDATE_MIN_MAX(ips_pm5_min_pcs, ips_pm5_max_pcs, strtoul(N5_serial.c_str(), NULL, 10));
			UPDATE_MIN_MAX(ips_pm10_min_pcs, ips_pm10_max_pcs, strtoul(N10_serial.c_str(), NULL, 10));

			debug_outln_info(F("IPS Measure..."));
			ips_val_count++;
			debug_outln(String(ips_val_count), DEBUG_MAX_INFO);
		}
	}

	if (send_now)
	{

		last_value_IPS_P0 = -1.0;  // PM1
		last_value_IPS_P1 = -1.0;  // PM10
		last_value_IPS_P2 = -1.0;  // PM2.5
		last_value_IPS_P01 = -1.0; // PM0.1
		last_value_IPS_P03 = -1.0; // PM0.3
		last_value_IPS_P05 = -1.0; // PM0.5
		last_value_IPS_P5 = -1.0;  // PM5
		last_value_IPS_N1 = -1.0;
		last_value_IPS_N10 = -1.0;
		last_value_IPS_N25 = -1.0;
		last_value_IPS_N01 = -1.0;
		last_value_IPS_N03 = -1.0;
		last_value_IPS_N05 = -1.0;
		last_value_IPS_N5 = -1.0;

		if (ips_val_count > 2)
		{
			ips_pm01_sum = ips_pm01_sum - ips_pm01_min - ips_pm01_max;
			ips_pm03_sum = ips_pm03_sum - ips_pm03_min - ips_pm03_max;
			ips_pm05_sum = ips_pm05_sum - ips_pm05_min - ips_pm05_max;
			ips_pm1_sum = ips_pm1_sum - ips_pm1_min - ips_pm1_max;
			ips_pm25_sum = ips_pm25_sum - ips_pm25_min - ips_pm25_max;
			ips_pm5_sum = ips_pm5_sum - ips_pm5_min - ips_pm5_max;
			ips_pm10_sum = ips_pm10_sum - ips_pm10_min - ips_pm10_max;

			ips_pm01_sum_pcs = ips_pm01_sum_pcs - ips_pm01_min_pcs - ips_pm01_max_pcs;
			ips_pm03_sum_pcs = ips_pm03_sum_pcs - ips_pm03_min_pcs - ips_pm03_max_pcs;
			ips_pm05_sum_pcs = ips_pm05_sum_pcs - ips_pm05_min_pcs - ips_pm05_max_pcs;
			ips_pm1_sum_pcs = ips_pm1_sum_pcs - ips_pm1_min_pcs - ips_pm1_max_pcs;
			ips_pm25_sum_pcs = ips_pm25_sum_pcs - ips_pm25_min_pcs - ips_pm25_max_pcs;
			ips_pm5_sum_pcs = ips_pm5_sum_pcs - ips_pm5_min_pcs - ips_pm5_max_pcs;
			ips_pm10_sum_pcs = ips_pm10_sum_pcs - ips_pm10_min_pcs - ips_pm10_max_pcs;

			ips_val_count = ips_val_count - 2;
		}

		if (ips_val_count > 0)
		{
			debug_outln_info(FPSTR(DBG_TXT_SEP));
			
			last_value_IPS_P0 = float(ips_pm1_sum) / (ips_val_count);
			last_value_IPS_P1 = float(ips_pm10_sum) / (ips_val_count);
			last_value_IPS_P2 = float(ips_pm25_sum) / (ips_val_count);
			last_value_IPS_P01 = float(ips_pm01_sum) / (ips_val_count);
			last_value_IPS_P03 = float(ips_pm03_sum) / (ips_val_count);
			last_value_IPS_P05 = float(ips_pm05_sum) / (ips_val_count);
			last_value_IPS_P5 = float(ips_pm5_sum) / (ips_val_count);

			last_value_IPS_N1 = float(ips_pm1_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N10 = float(ips_pm10_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N25 = float(ips_pm25_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N01 = float(ips_pm01_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N03 = float(ips_pm03_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N05 = float(ips_pm05_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N5 = float(ips_pm5_sum_pcs) / (ips_val_count * 1000.0f);

			add_Value2Json(s, F("IPS_P0"), F("PM1: "), last_value_IPS_P0);
			add_Value2Json(s, F("IPS_P1"), F("PM10:  "), last_value_IPS_P1);
			add_Value2Json(s, F("IPS_P2"), F("PM2.5: "), last_value_IPS_P2);
			add_Value2Json(s, F("IPS_P01"), F("PM0.1: "), last_value_IPS_P01);
			add_Value2Json(s, F("IPS_P03"), F("PM0.3:  "), last_value_IPS_P03);
			add_Value2Json(s, F("IPS_P05"), F("PM0.5: "), last_value_IPS_P05);
			add_Value2Json(s, F("IPS_P5"), F("PM5: "), last_value_IPS_P5);

			add_Value2Json(s, F("IPS_N1"), F("NC1.0: "), last_value_IPS_N1);
			add_Value2Json(s, F("IPS_N10"), F("NC10:  "), last_value_IPS_N10);
			add_Value2Json(s, F("IPS_N25"), F("NC2.5: "), last_value_IPS_N25);
			add_Value2Json(s, F("IPS_N01"), F("NC0.1: "), last_value_IPS_N01);
			add_Value2Json(s, F("IPS_N03"), F("NC0.3:  "), last_value_IPS_N03);
			add_Value2Json(s, F("IPS_N05"), F("NC0.5: "), last_value_IPS_N05);
			add_Value2Json(s, F("IPS_N5"), F("NC5: "), last_value_IPS_N5);

			debug_outln_info(FPSTR(DBG_TXT_SEP));

			if (ips_val_count < 3)
			{
				IPS_error_count++;
			}
		}
		else
		{
			IPS_error_count++;
		}

		ips_pm01_sum = 0;
		ips_pm03_sum = 0;
		ips_pm05_sum = 0;
		ips_pm1_sum = 0;
		ips_pm25_sum = 0;
		ips_pm5_sum = 0;
		ips_pm10_sum = 0;

		ips_val_count = 0;

		ips_pm01_max = 0;
		ips_pm01_min = 200;
		ips_pm03_max = 0;
		ips_pm03_min = 200;
		ips_pm05_max = 0;
		ips_pm05_min = 200;
		ips_pm1_max = 0;
		ips_pm1_min = 200;
		ips_pm25_max = 0;
		ips_pm25_min = 200;
		ips_pm5_max = 0;
		ips_pm5_min = 200;
		ips_pm10_max = 0;
		ips_pm10_min = 200;

		ips_pm01_sum_pcs = 0;
		ips_pm03_sum_pcs = 0;
		ips_pm05_sum_pcs = 0;
		ips_pm1_sum_pcs = 0;
		ips_pm25_sum_pcs = 0;
		ips_pm5_sum_pcs = 0;
		ips_pm10_sum_pcs = 0;

		ips_pm01_max_pcs = 0;
		ips_pm01_min_pcs = 4000000000;
		ips_pm03_max_pcs = 0;
		ips_pm03_min_pcs = 4000000000;
		ips_pm05_max_pcs = 0;
		ips_pm05_min_pcs = 4000000000;
		ips_pm1_max_pcs = 0;
		ips_pm1_min_pcs = 4000000000;
		ips_pm25_max_pcs = 0;
		ips_pm25_min_pcs = 4000000000;
		ips_pm5_max_pcs = 0;
		ips_pm5_min_pcs = 4000000000;
		ips_pm10_max_pcs = 0;
		ips_pm10_min_pcs = 4000000000;

		if (cfg::sending_intervall_ms > (WARMUPTIME_IPS_MS + READINGTIME_IPS_MS))
		{
			if (is_IPS_running)
			{
				debug_outln_info(F("Change IPS to stop after measure..."));
				IPS_cmd(PmSensorCmd3::Stop);
				is_IPS_running = false;
			}
		}
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_IPS));
}

/*****************************************************************
   read SEN5X PM sensor values

   Send NOx value to outside
 *****************************************************************/
static void fetchSensorSEN5X(String &s)
{
	String result_SEN5X = emptyString;

	if (memcmp(SEN5X_type, "SEN50", 6) == 0)
	{
		result_SEN5X = F("SEN50_");
		debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SEN50));
	}

	if (memcmp(SEN5X_type, "SEN54", 6) == 0)
	{
		result_SEN5X = F("SEN54_");
		debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SEN54));
	}

	if (memcmp(SEN5X_type, "SEN55", 6) == 0)
	{
		result_SEN5X = F("SEN55_");
		debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SEN55));
	}

	last_value_SEN5X_P0 = value_SEN5X_P0 / SEN5X_measurement_count;
	last_value_SEN5X_P2 = value_SEN5X_P2 / SEN5X_measurement_count;
	last_value_SEN5X_P4 = value_SEN5X_P4 / SEN5X_measurement_count;
	last_value_SEN5X_P1 = value_SEN5X_P1 / SEN5X_measurement_count;
	last_value_SEN5X_N05 = value_SEN5X_N05 / SEN5X_measurement_count;
	last_value_SEN5X_N1 = value_SEN5X_N1 / SEN5X_measurement_count;
	last_value_SEN5X_N25 = value_SEN5X_N25 / SEN5X_measurement_count;
	last_value_SEN5X_N4 = value_SEN5X_N4 / SEN5X_measurement_count;
	last_value_SEN5X_N10 = value_SEN5X_N10 / SEN5X_measurement_count;
	last_value_SEN5X_TS = value_SEN5X_TS / SEN5X_measurement_count;

	last_value_SEN5X_VOC = value_SEN5X_VOC / SEN5X_measurement_count;

	debug_outln_info(FPSTR(DBG_TXT_SEP));

	add_Value2Json(s, FPSTR((result_SEN5X + F("P0")).c_str()),  F("PM1.0: "), last_value_SEN5X_P0);
	add_Value2Json(s, FPSTR((result_SEN5X + F("P2")).c_str()),  F("PM2.5: "), last_value_SEN5X_P2);
	add_Value2Json(s, FPSTR((result_SEN5X + F("P4")).c_str()),  F("PM4.0: "), last_value_SEN5X_P4);
	add_Value2Json(s, FPSTR((result_SEN5X + F("P1")).c_str()),  F("PM 10: "), last_value_SEN5X_P1);
	add_Value2Json(s, FPSTR((result_SEN5X + F("N05")).c_str()), F("NC0.5: "), last_value_SEN5X_N05);
	add_Value2Json(s, FPSTR((result_SEN5X + F("N1")).c_str()),  F("NC1.0: "), last_value_SEN5X_N1);
	add_Value2Json(s, FPSTR((result_SEN5X + F("N25")).c_str()), F("NC2.5: "), last_value_SEN5X_N25);
	add_Value2Json(s, FPSTR((result_SEN5X + F("N4")).c_str()),  F("NC4.0: "), last_value_SEN5X_N4);
	add_Value2Json(s, FPSTR((result_SEN5X + F("N10")).c_str()), F("NC10:  "), last_value_SEN5X_N10);
	add_Value2Json(s, FPSTR((result_SEN5X + F("TS")).c_str()),  F("TPS:   "), last_value_SEN5X_TS);

	// For test fase split sensor data in two sections (PM and temp) moved to fetchSensorSEN5X_HT().
	// if (memcmp(SEN5X_type, "SEN50", 6) != 0)
	// {
	// 	add_Value2Json(s, FPSTR((result_SEN5X + F("temperature")).c_str()), FPSTR(DBG_TXT_TEMPERATURE), last_value_SEN5X_T);
	// 	add_Value2Json(s, FPSTR((result_SEN5X + F("humidity")).c_str()), FPSTR(DBG_TXT_HUMIDITY), last_value_SEN5X_H);
	// }

	// ["\"voc\" is not a valid choice. for SEN54/SEN55, type NOT in sensor community table."]
	// if (memcmp(SEN5X_type, "SEN54", 6) == 0 || memcmp(SEN5X_type, "SEN55", 6) == 0)
	// {
	// 	add_Value2Json(s, FPSTR((result_SEN5X + F("VOC")).c_str()), FPSTR(DBG_TXT_VOCINDEX), last_value_SEN5X_VOC);
	// }

	// if (memcmp(SEN5X_type, "SEN55", 6) == 0)
	// {// NOx 
	// 	add_Value2Json(s, FPSTR((result_SEN5X + F("CO2")).c_str()), FPSTR(DBG_TXT_NOX), last_value_SEN5X_NOX);
	// }

	debug_outln_info( FPSTR((result_SEN5X + " read counter: ").c_str()), String(SEN5X_read_counter));
	debug_outln_info( FPSTR((result_SEN5X + " read error counter: ").c_str()), String(SEN5X_read_error_counter));

	SEN5X_read_counter = 0;
	SEN5X_read_error_counter = 0;
	value_SEN5X_P0 = value_SEN5X_P1 = value_SEN5X_P2 = value_SEN5X_P4 = 0.0;
	value_SEN5X_N05 = value_SEN5X_N1 = value_SEN5X_N25 = value_SEN5X_N10 = value_SEN5X_N4 = 0.0;
	value_SEN5X_TS = 0.0;
	value_SEN5X_VOC = 0.0;

	debug_outln_info(FPSTR(DBG_TXT_SEP));

	if (memcmp(SEN5X_type, "SEN50", 6) == 0)
	{
		debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SEN50));
	}

	if (memcmp(SEN5X_type, "SEN54", 6) == 0)
	{
		debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SEN54));
	}

	if (memcmp(SEN5X_type, "SEN55", 6) == 0)
	{
		debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SEN55));
	}
}

/*
 	Temperature and Humidity and Co2 as NOx.

	fetchSensorSEN5X() => set last_value_SEN5X_T and last_value_SEN5X_H value.

*/
static void fetchSensorSEN5X_THN(String &s)
{
	if (memcmp(SEN5X_type, "SEN55", 6) == 0 || memcmp(SEN5X_type, "SEN54", 6) == 0)
	{
		last_value_SEN5X_T = value_SEN5X_T / SEN5X_measurement_count;
		last_value_SEN5X_H = value_SEN5X_H / SEN5X_measurement_count;
		last_value_SEN5X_NOX = value_SEN5X_NOX / SEN5X_measurement_count;

		debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SEN55));

		String result_SEN5X = F("SEN5X_");
		add_Value2Json(s, FPSTR((result_SEN5X + F("temperature")).c_str()), FPSTR(DBG_TXT_TEMPERATURE), last_value_SEN5X_T);
		add_Value2Json(s, FPSTR((result_SEN5X + F("humidity")).c_str()),    FPSTR(DBG_TXT_HUMIDITY),    last_value_SEN5X_H);
		add_Value2Json(s, FPSTR((result_SEN5X + F("co2_ppm")).c_str()), 	FPSTR(DBG_TXT_NOX), 		last_value_SEN5X_NOX);	// NOx

		debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SEN55));
	}

	value_SEN5X_H = value_SEN5X_T = value_SEN5X_NOX = 0.0;
	SEN5X_measurement_count = 0;
}

/*****************************************************************
 * read PPD42NS sensor values                                    *
 *****************************************************************/
static __noinline void fetchSensorPPD(String &s)
{
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_PPD42NS));

	if (msSince(starttime) <= SAMPLETIME_MS)
	{

		// Read pins connected to ppd42ns
		boolean valP1 = digitalRead(PPD_PIN_PM1);
		boolean valP2 = digitalRead(PPD_PIN_PM2);

		if (valP1 == LOW && trigP1 == false)
		{
			trigP1 = true;
			trigOnP1 = act_micro;
		}

		if (valP1 == HIGH && trigP1 == true)
		{
			lowpulseoccupancyP1 += act_micro - trigOnP1;
			trigP1 = false;
		}

		if (valP2 == LOW && trigP2 == false)
		{
			trigP2 = true;
			trigOnP2 = act_micro;
		}

		if (valP2 == HIGH && trigP2 == true)
		{
			unsigned long durationP2 = act_micro - trigOnP2;
			lowpulseoccupancyP2 += durationP2;
			trigP2 = false;
		}
	}

	// Checking if it is time to sample
	if (send_now)
	{
		auto calcConcentration = [](const float ratio)
		{
			/* spec sheet curve*/
			return (1.1f * ratio * ratio * ratio - 3.8f * ratio * ratio + 520.0f * ratio + 0.62f);
		};

		debug_outln_info(FPSTR(DBG_TXT_SEP));

		last_value_PPD_P1 = -1;
		last_value_PPD_P2 = -1;
		float ratio = lowpulseoccupancyP1 / (SAMPLETIME_MS * 10.0f);
		float concentration = calcConcentration(ratio);

		// json for push to api / P1
		last_value_PPD_P1 = concentration;
		add_Value2Json(s, F("durP1"), F("LPO P10    : "), lowpulseoccupancyP1);
		add_Value2Json(s, F("ratioP1"), F("Ratio PM10%: "), ratio);
		add_Value2Json(s, F("P1"), F("PM10 Count : "), last_value_PPD_P1);

		ratio = lowpulseoccupancyP2 / (SAMPLETIME_MS * 10.0f);
		concentration = calcConcentration(ratio);

		// json for push to api / P2
		last_value_PPD_P2 = concentration;
		add_Value2Json(s, F("durP2"), F("LPO PM25   : "), lowpulseoccupancyP2);
		add_Value2Json(s, F("ratioP2"), F("Ratio PM25%: "), ratio);
		add_Value2Json(s, F("P2"), F("PM25 Count : "), last_value_PPD_P2);

		debug_outln_info(FPSTR(DBG_TXT_SEP));
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_PPD42NS));
}

/*****************************************************************
   read SPS30 PM sensor values
 *****************************************************************/
static void fetchSensorSPS30(String &s)
{
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SPS30));
	debug_outln_info(FPSTR(DBG_TXT_SEP));

	last_value_SPS30_P0 = value_SPS30_P0 / SPS30_measurement_count;
	last_value_SPS30_P2 = value_SPS30_P2 / SPS30_measurement_count;
	last_value_SPS30_P4 = value_SPS30_P4 / SPS30_measurement_count;
	last_value_SPS30_P1 = value_SPS30_P1 / SPS30_measurement_count;
	last_value_SPS30_N05 = value_SPS30_N05 / SPS30_measurement_count;
	last_value_SPS30_N1 = value_SPS30_N1 / SPS30_measurement_count;
	last_value_SPS30_N25 = value_SPS30_N25 / SPS30_measurement_count;
	last_value_SPS30_N4 = value_SPS30_N4 / SPS30_measurement_count;
	last_value_SPS30_N10 = value_SPS30_N10 / SPS30_measurement_count;
	last_value_SPS30_TS = value_SPS30_TS / SPS30_measurement_count;

	add_Value2Json(s, F("SPS30_P0"),  F("PM1.0: "), last_value_SPS30_P0);
	add_Value2Json(s, F("SPS30_P2"),  F("PM2.5: "), last_value_SPS30_P2);
	add_Value2Json(s, F("SPS30_P4"),  F("PM4.0: "), last_value_SPS30_P4);
	add_Value2Json(s, F("SPS30_P1"),  F("PM 10: "), last_value_SPS30_P1);
	add_Value2Json(s, F("SPS30_N05"), F("NC0.5: "), last_value_SPS30_N05);
	add_Value2Json(s, F("SPS30_N1"),  F("NC1.0: "), last_value_SPS30_N1);
	add_Value2Json(s, F("SPS30_N25"), F("NC2.5: "), last_value_SPS30_N25);
	add_Value2Json(s, F("SPS30_N4"),  F("NC4.0: "), last_value_SPS30_N4);
	add_Value2Json(s, F("SPS30_N10"), F("NC10:  "), last_value_SPS30_N10);
	add_Value2Json(s, F("SPS30_TS"),  F("TPS:   "), last_value_SPS30_TS);

	debug_outln_info(F("SPS30 read counter: "), String(SPS30_read_counter));
	debug_outln_info(F("SPS30 read error counter: "), String(SPS30_read_error_counter));

	SPS30_measurement_count = 0;
	SPS30_read_counter = 0;
	SPS30_read_error_counter = 0;
	value_SPS30_P0 = value_SPS30_P1 = value_SPS30_P2 = value_SPS30_P4 = 0.0;
	value_SPS30_N05 = value_SPS30_N1 = value_SPS30_N25 = value_SPS30_N10 = value_SPS30_N4 = 0.0;
	value_SPS30_TS = 0.0;

	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SPS30));
}

/*****************************************************************
   read DNMS values
 *****************************************************************/
static void fetchSensorDNMS(String &s)
{
	static bool dnms_error = false;
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_DNMS));

	last_value_dnms_laeq = -1.0;
	last_value_dnms_la_min = -1.0;
	last_value_dnms_la_max = -1.0;

	if (dnms_calculate_leq() != 0)
	{
		dnms_error = true;
	}

	uint16_t data_ready = 0;
	dnms_error = true;

	for (unsigned i = 0; i < 20; i++)
	{
		delay(2);
		int16_t ret_dnms = dnms_read_data_ready(&data_ready);
		if ((ret_dnms == 0) && (data_ready != 0))
		{
			dnms_error = false;
			break;
		}
	}
	
	if (!dnms_error)
	{
		struct dnms_measurements dnms_values;
		if (dnms_read_leq(&dnms_values) == 0)
		{
			float dnms_corr_value = readCorrectionOffset(cfg::dnms_correction);
			last_value_dnms_laeq = dnms_values.leq_a + dnms_corr_value;
			last_value_dnms_la_min = dnms_values.leq_a_min + dnms_corr_value;
			last_value_dnms_la_max = dnms_values.leq_a_max + dnms_corr_value;
		}
		else
		{
			dnms_error = true;
		}
	}

	if (dnms_error)
	{
		dnms_reset(); // try to reset dnms
		debug_outln_error(F("DNMS read failed"));
	}
	else
	{
		add_Value2Json(s, F("DNMS_noise_LAeq"), F("LAeq: "), last_value_dnms_laeq);
		add_Value2Json(s, F("DNMS_noise_LA_min"), F("LA_MIN: "), last_value_dnms_la_min);
		add_Value2Json(s, F("DNMS_noise_LA_max"), F("LA_MAX: "), last_value_dnms_la_max);
	}

	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_DNMS));
}

/*****************************************************************
 * read GPS sensor values                                        *
 *****************************************************************/
static __noinline void fetchSensorGPS(String &s)
{
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), "GPS");

	if (gps.location.isUpdated())
	{
		if (gps.location.isValid())
		{
			last_value_GPS_lat = gps.location.lat();
			last_value_GPS_lon = gps.location.lng();
		}
		else
		{
			last_value_GPS_lat = -200;
			last_value_GPS_lon = -200;
			debug_outln_verbose(F("Lat/Lng INVALID"));
		}
		if (gps.altitude.isValid())
		{
			last_value_GPS_alt = gps.altitude.meters();
		}
		else
		{
			last_value_GPS_alt = -1000;
			debug_outln_verbose(F("Altitude INVALID"));
		}
		if (!gps.date.isValid())
		{
			debug_outln_verbose(F("Date INVALID"));
		}
		if (!gps.time.isValid())
		{
			debug_outln_verbose(F("Time: INVALID"));
		}
		if (gps.date.isValid() && gps.time.isValid())
		{
			char gps_datetime[37];
			snprintf_P(gps_datetime, sizeof(gps_datetime), PSTR("%04d-%02d-%02dT%02d:%02d:%02d.%03d"),
					   gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond());
			last_value_GPS_timestamp = gps_datetime;
		}
		else
		{
			// define a default value
			last_value_GPS_timestamp = F("2023-01-01T00:00:00.000");
		}
	}

	if (send_now)
	{
		debug_outln_info(F("Lat: "), String(last_value_GPS_lat, 6));
		debug_outln_info(F("Lng: "), String(last_value_GPS_lon, 6));
		debug_outln_info(F("DateTime: "), last_value_GPS_timestamp);

		add_Value2Json(s, F("GPS_lat"), String(last_value_GPS_lat, 6));
		add_Value2Json(s, F("GPS_lon"), String(last_value_GPS_lon, 6));
		add_Value2Json(s, F("GPS_height"), F("Altitude: "), last_value_GPS_alt);
		add_Value2Json(s, F("GPS_timestamp"), last_value_GPS_timestamp);
		debug_outln_info(FPSTR(DBG_TXT_SEP));
	}

	if (count_sends > 0 && gps.charsProcessed() < 10)
	{
		debug_outln_error(F("No GPS data received: check wiring"));
		gps_init_failed = true;
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), "GPS");
}

/*****************************************************************
 * Get SEN5X sensor values.                                      *
 * include Start/Stop system.									 *
 * wait at least 25 sec. after start Measurement.				 *
 *****************************************************************/
static void GetSen5XSensorData()
{
	if (cfg::sending_intervall_ms > (SEN5X_WAITING_AFTER_LAST_READ + READINGTIME_SEN5X_MS) &&
		msSince(starttime) < (cfg::sending_intervall_ms - (SEN5X_WAITING_AFTER_LAST_READ + READINGTIME_SEN5X_MS)))
	{
		if (is_SEN5X_running)
		{
			if (!cfg::sen5x_on)
			{
				debug_outln_info(F("SEN5X STOP Measurement. time: "), String(msSince(starttime)));
	
				sen5x.stopMeasurement();
			}

				is_SEN5X_running = false;
		}
	}
	else if (is_SEN5X_running && (msSince(starttime) - SEN5X_read_timer) > SEN5X_WAITING_AFTER_LAST_READ)
	{
		debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SEN55));
		debug_outln_info(FPSTR(DBG_TXT_SEP));
		debug_outln_info(F("SEN5X START sensor readings. time: "), String((msSince(starttime) - (SEN5X_read_timer + (SEN5X_WAITING_AFTER_LAST_READ - SAMPLETIME_SEN5X_MS)))) + F(" msec.") );

		uint16_t error;
		char errorMessage[256];

		float massConcentrationPm1p0;
		float massConcentrationPm2p5;
		float massConcentrationPm4p0;
		float massConcentrationPm10p0;
		float numberConcentrationPm0p5;
		float numberConcentrationPm1p0;
		float numberConcentrationPm2p5;
		float numberConcentrationPm4p0;
		float numberConcentrationPm10p0;
		float typicalParticleSize;
		float ambientHumidity;
		float ambientTemperature;
		float vocIndex;
		float noxIndex;


		error = sen5x.readMeasuredPmValues(massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0, massConcentrationPm10p0,
										   numberConcentrationPm0p5, numberConcentrationPm1p0, numberConcentrationPm2p5, 
										   numberConcentrationPm4p0, numberConcentrationPm10p0, 
										   typicalParticleSize);
		SEN5X_read_counter++;

		if (error)
		{
			Debug.print("Error trying to execute readMeasuredPmValues(): ");
			errorToString(error, errorMessage, sizeof(errorMessage));
			Debug.println(errorMessage);
		}
		else
		{
			value_SEN5X_P0 += massConcentrationPm1p0;
			value_SEN5X_P1 += massConcentrationPm10p0;
			value_SEN5X_P2 += massConcentrationPm2p5;
			value_SEN5X_P4 += massConcentrationPm4p0;
			value_SEN5X_N05 += numberConcentrationPm0p5;
			value_SEN5X_N1 += numberConcentrationPm1p0;
			value_SEN5X_N25 += numberConcentrationPm2p5;
			value_SEN5X_N4 += numberConcentrationPm4p0;
			value_SEN5X_N10 += numberConcentrationPm10p0;
			value_SEN5X_TS += typicalParticleSize;
		}

		error = sen5x.readMeasuredValues(massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0, massConcentrationPm10p0,
										 ambientHumidity, ambientTemperature, 
										 vocIndex, noxIndex);

		if (error)
		{
			Debug.print("Error trying to execute readMeasuredTHValues(): ");
			errorToString(error, errorMessage, sizeof(errorMessage));
			Debug.println(errorMessage);
		}
		else
		{
			value_SEN5X_T += ambientTemperature;
			value_SEN5X_H += ambientHumidity;
			value_SEN5X_VOC += vocIndex;
			value_SEN5X_NOX += noxIndex;
		}

		SEN5X_measurement_count++;

		// Set sensor read time on 1 sec. => 5 reads => Nox value = 0 (start/stop)
		SEN5X_read_timer = msSince(starttime + (SEN5X_WAITING_AFTER_LAST_READ - SAMPLETIME_SEN5X_MS));
		
		debug_outln_info(FPSTR(DBG_TXT_SEP));
		debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SEN55));
	}
	else
	{
		if (!is_SEN5X_running)
		{
			if (!cfg::sen5x_on)
			{
				debug_outln_info(F("SEN5X START Measurement. Time: "), String(msSince(starttime)));
				sen5x.startMeasurement();
			}

			SEN5X_read_timer = msSince(starttime);
			is_SEN5X_running = true;
		}
	}
}

/*****************************************************************
 * OTA-Update                                                    *
 * client => wifi intstance									 	 *
 * url => URL command string									 *
 * ostream => File stream									 	 *
 *****************************************************************/
static bool fwDownloadStream(WiFiClientSecure &client, const String &url, Stream *ostream)
{
	HTTPClient http;
	int bytes_written = -1;

	// work with 128kbit/s downlinks
	http.setTimeout(60 * 1000);
	String agent(SOFTWARE_VERSION);
	agent += ' ';
	agent += esp_chipid;
	agent += "/";
	agent += esp_mac_id;
	agent += ' ';
	
	if (cfg::npm_read)
	{
		agent += NPM_version_date();
	}
	else if (cfg::ips_read)
	{
		agent += IPS_version_date();
	}
	else if (cfg::sds_read)
	{
		agent += SDS_version_date();
	}
	else
	{
		agent += "Fijnstof Leusden/13-09-2023";
	}

	agent += ' ';
	agent += String(cfg::current_lang);
	agent += ' ';
	agent += String(CURRENT_LANG);
	agent += ' ';

	if (cfg::use_beta)
	{
		agent += F("BETA");
	}

	http.setUserAgent(agent);
	http.setReuse(false);

	debug_outln_verbose(F("HTTP GET: "), String(FPSTR(FW_DOWNLOAD_HOST)) + ':' + String(FW_DOWNLOAD_PORT) + url);
	//debug_outln_info(F("HTTP GET: "), String(FPSTR(FW_DOWNLOAD_HOST)) + ':' + String(FW_DOWNLOAD_PORT) + url);

	// example Update firmware url address:  https://firmware.sensor.community:443/airrohr/update/latest_nl.bin	
	if (http.begin(client, FPSTR(FW_DOWNLOAD_HOST), FW_DOWNLOAD_PORT, url))
	{
		int resp = http.GET();

		debug_outln_verbose(F("GET responce code: "), String(resp));
		//debug_outln_info(F("GET responce code: "), String(resp));

		last_update_returncode = resp;

		if (resp == HTTP_CODE_OK)
		{
			debug_outln_verbose(F("Start writeToStream(***): "));
			bytes_written = http.writeToStream(ostream);		// data stored in file in SPIFF memory drive.
			debug_outln_verbose(F("End writeToStream(**): ret code: "), String(bytes_written));
		}

		http.end();
	}

	debug_outln_verbose(F("HTTP End: read chars = "), String(bytes_written));

	if (bytes_written > 0)
	{
		return true;
	}

	return false;
}

/// @brief 
/// @param client 
/// @param url 
/// @param fname 
/// @return 
static bool fwDownloadStreamFile(WiFiClientSecure &client, const String &url, const String &fname)
{
	debug_outln_verbose(F("fwDownloadStreamFile(): URL | file name "), String(url) + " | " + String(fname));

	String fname_new(fname);
	fname_new += F(".new");
	bool downloadSuccess = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

	File fwFile = SPIFFS.open(fname_new, "w");

	// bool fileExist = SPIFFS.exists(fname_new);
	// debug_outln_verbose(F("Check if File Open, File Name: "), fname_new + F(", Exist: ") + String(fileExist));

	debug_outln_verbose(F("File created in SPIFFS, Stream to File object: "), fwFile.fullName());

	if (fwFile)
	{
		downloadSuccess = fwDownloadStream(client, url, &fwFile);
		fwFile.close();

		if (downloadSuccess)
		{
			SPIFFS.remove(fname);
			SPIFFS.rename(fname_new, fname);
			debug_outln_info(F("Success downloading: "), url);
		}
	}
	else
	{
		debug_outln_verbose(F("File open failed"));
	}

	debug_outln_verbose(F("fwDownloadStreamFile():Ending, status: "), String(downloadSuccess));

	if (downloadSuccess)
	{
		return true;
	}

	SPIFFS.remove(fname_new);
	
#pragma GCC diagnostic pop

	return false;
}

/*
	airrohr-update-loader:
	A transitional firmware which will look for a firmware file stored on SPIFFS to replace itself with for next reboot 
	or do an endless loop of panic LED blinking if this fails.

	This allows to do an Over-the-air (OTA) procedure on setups that have a 1M/3M split layout (rather the more modern 2M/2M) 
	for firmwares larger than 512k (up to ~ 740k).

*/
static void twoStageOTAUpdate()
{
	// always return => process become dead could not load binary data into FS.
	// TODO: to find out what are the problem.
	return;

	if (!cfg::auto_update)
	{// NO auto firmware update.
		return;
	}

#if defined(ESP8266)
	debug_outln_info(F("twoStageOTAUpdate"));

	String lang_variant(cfg::current_lang);
	if (lang_variant.length() != 2)
	{
		lang_variant = CURRENT_LANG;
	}

	lang_variant.toLowerCase();

	String fetch_name(F(OTA_BASENAME "/update/latest_"));
	if (cfg::use_beta)
	{
		fetch_name = F(OTA_BASENAME "/beta/latest_");
	}

	// OTA HTTP server URL
	// https://firmware.sensor.community:443/airrohr/update/latest_nl.bin
	fetch_name += lang_variant;
	fetch_name += F(".bin");

	WiFiClientSecure client;
	BearSSL::Session clientSession;

	client.setBufferSizes(1024, TCP_MSS > 1024 ? 2048 : 1024);
	client.setSession(&clientSession);
	configureCACertTrustAnchor(&client);

	String fetch_md5_name(fetch_name);
	fetch_md5_name += F(".md5");

	StreamString newFwmd5;
	if (!fwDownloadStream(client, fetch_md5_name, &newFwmd5))
	{
		return;
	}

	newFwmd5.trim();
	if (newFwmd5 == ESP.getSketchMD5())
	{
		display_debug(FPSTR(DBG_TXT_UPDATE), FPSTR(DBG_TXT_UPDATE_NO_UPDATE));
		debug_outln_verbose(F("No newer version available."));
		return;
	}

	debug_outln_info(F("Update md5: "), newFwmd5);
	debug_outln_info(F("Current Sketch md5: "), ESP.getSketchMD5());

	// We're entering update phase, kill off everything else
	WiFiUDP::stopAll();
	WiFiClient::stopAllExcept(&client);
	delay(100);

	debug_outln_verbose(F("Start DownloadStreamFilev process.."));

	String firmware_name(F("/firmware.bin"));
	String firmware_md5(F("/firmware.bin.md5"));
	String loader_name(F("/loader.bin"));

	debug_outln_verbose(F("Start DownloadStreamFile() process.. "), String(fetch_name) + " | " + String(firmware_name));

//---------------- TEST TEST -----File write / read works OK ------------------------------------------------------------------------------------------------------------
#if TEST
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wsign-compare"

	// Create New File And Write Data to It
	// w=Write Open file for writing
	File f = SPIFFS.open(firmware_name, "w");

	if (!f)
	{
		debug_outln_verbose(F("file open failed"));
	}
	else
	{
		// Write data to file
		debug_outln_verbose(F("Writing Data to File"));
		f.print("This is sample data which is written in file");
		f.close(); // Close file
	}

	delay(2000);

	// Read File data
	File rf = SPIFFS.open(firmware_name, "r");

	if (!rf)
	{
		debug_outln_verbose(F("file open failed"));
	}
	else
	{
		debug_outln_verbose(F("Reading Data from File:"));

		String tmp = "";

		// Data from file.
		for (int i = 0; i < rf.size(); i++) // Read upto complete file size
		{
			tmp += (char)rf.read();
		}

		debug_outln_info(F("file contents: "), tmp);

		rf.close(); // Close file
		debug_outln_info(F("File Closed"));
	}

	debug_outln_info(F("Remove File from FS: "), firmware_name);
	SPIFFS.remove(firmware_name);

	delay(1000);

	return;

	#pragma GCC diagnostic pop

#endif
//--------------------TEST TEST -------------------------------------------------------------------------------------------------------------


	if (!fwDownloadStreamFile(client, fetch_name, firmware_name))
	{
		debug_outln_verbose(F("Failed DownloadStreamFile() firmware_name file.."));
		return;
	}

	if (!fwDownloadStreamFile(client, fetch_md5_name, firmware_md5))
	{
		debug_outln_verbose(F("Failed DownloadStreamFile() firmware_md5 file.."));
		return;
	}

	if (!fwDownloadStreamFile(client, FPSTR(FW_2ND_LOADER_URL), loader_name))
	{
		debug_outln_verbose(F("Failed DownloadStreamFile() loader_name file.."));
		return;
	}

	debug_outln_verbose(F("All Files are downloaded.. "), String(firmware_name));

	// SPIFFS is deprecated, we know
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	File fwFile = SPIFFS.open(firmware_name, "r");

	if (!fwFile)
	{
		SPIFFS.remove(firmware_name);
		SPIFFS.remove(firmware_md5);
		debug_outln_error(F("Failed reopening fw file.."));
		return;
	}

	size_t fwSize = fwFile.size();
	MD5Builder md5;
	md5.begin();
	md5.addStream(fwFile, fwSize);
	md5.calculate();
	fwFile.close();
	String md5String = md5.toString();

	// Firmware is always at least 128 kB and padded to 16 bytes
	if (fwSize < (1 << 17) || (fwSize % 16 != 0) || newFwmd5 != md5String)
	{
		debug_outln_info(F("FW download failed validation.. deleting"));
		SPIFFS.remove(firmware_name);
		SPIFFS.remove(firmware_md5);
		return;
	}

	StreamString loaderMD5;
	if (!fwDownloadStream(client, String(FPSTR(FW_2ND_LOADER_URL)) + F(".md5"), &loaderMD5))
	{
		return;
	}

	loaderMD5.trim();

	debug_outln_info(F("launching 2nd stage"));
	if (!launchUpdateLoader(loaderMD5))
	{
		debug_outln_error(FPSTR(DBG_TXT_UPDATE_FAILED));
		display_debug(FPSTR(DBG_TXT_UPDATE), FPSTR(DBG_TXT_UPDATE_FAILED));
		SPIFFS.remove(firmware_name);
		SPIFFS.remove(firmware_md5);
		return;
	}
#pragma GCC diagnostic pop

	sensor_restart();
#endif
}

static String displayGenerateFooter(unsigned int screen_count)
{
	String display_footer;
	for (unsigned int i = 0; i < screen_count; ++i)
	{
		display_footer += (i != (next_display_count % screen_count)) ? " . " : " o ";
	}
	return display_footer;
}

/*****************************************************************
 * display values                                                *
 *****************************************************************/
static void display_values()
{
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

	float t_value = -128.0;
	float h_value = -1.0;
	float p_value = -1.0;
	float voc_value = -1.0;
	float nox_value = -1.0;
	String t_sensor, h_sensor, p_sensor, voc_sensor, nox_sensor;

	/* no diagnostic for this section till "GCC diagnostic pop" */
	float pm001_value = -1.0;
	float pm003_value = -1.0;
	float pm005_value = -1.0;
	float pm25_value = -1.0;
	float pm01_value = -1.0;
	float pm04_value = -1.0;
	float pm05_value = -1.0;
	float pm10_value = -1.0;

	String pm01_sensor;
	String pm10_sensor;
	String pm25_sensor;
	String pm001_sensor;
	String pm003_sensor;
	String pm005_sensor;
	String pm05_sensor;

	float nc001_value = -1.0;
	float nc003_value = -1.0;
	float nc005_value = -1.0;
	float nc010_value = -1.0;
	float nc025_value = -1.0;
	float nc040_value = -1.0;
	float nc050_value = -1.0;
	float nc100_value = -1.0;
	float la_eq_value = -1.0;
	float la_max_value = -1.0;
	float la_min_value = -1.0;

#pragma GCC diagnostic pop

	String la_sensor;

	float tps_value = -1.0;
	double lat_value = -200.0;
	double lon_value = -200.0;
	double alt_value = -1000.0;
	String display_header;
	String display_lines[3] = {"", "", ""};
	uint8_t screen_count = 0;
	uint8_t screens[13];
	int line_count = 0;

	debug_outln_info(F("output values to display..."));

	if (cfg::ppd_read)
	{
		pm10_value = last_value_PPD_P1;
		pm10_sensor = FPSTR(SENSORS_PPD42NS);
		pm25_value = last_value_PPD_P2;
		pm25_sensor = FPSTR(SENSORS_PPD42NS);
	}

	if (cfg::pms_read)
	{
		pm10_value = last_value_PMS_P1;
		pm10_sensor = FPSTR(SENSORS_PMSx003);
		pm25_value = last_value_PMS_P2;
		pm25_sensor = FPSTR(SENSORS_PMSx003);
	}

	if (cfg::hpm_read)
	{
		pm10_value = last_value_HPM_P1;
		pm10_sensor = FPSTR(SENSORS_HPM);
		pm25_value = last_value_HPM_P2;
		pm25_sensor = FPSTR(SENSORS_HPM);
	}

	if (cfg::npm_read)
	{
		pm01_value = last_value_NPM_P0;
		pm10_value = last_value_NPM_P1;
		pm25_value = last_value_NPM_P2;
		pm01_sensor = FPSTR(SENSORS_NPM);
		pm10_sensor = FPSTR(SENSORS_NPM);
		pm25_sensor = FPSTR(SENSORS_NPM);
		nc010_value = last_value_NPM_N1;
		nc100_value = last_value_NPM_N10;
		nc025_value = last_value_NPM_N25;
	}

	if (cfg::ips_read)
	{
		pm01_value = last_value_IPS_P0;
		pm10_value = last_value_IPS_P1;
		pm25_value = last_value_IPS_P2;
		pm001_value = last_value_IPS_P01;
		pm003_value = last_value_IPS_P03;
		pm005_value = last_value_IPS_P05;
		pm05_value = last_value_IPS_P5;

		pm001_sensor = FPSTR(SENSORS_IPS);
		pm003_sensor = FPSTR(SENSORS_IPS);
		pm005_sensor = FPSTR(SENSORS_IPS);
		pm01_sensor = FPSTR(SENSORS_IPS);
		pm10_sensor = FPSTR(SENSORS_IPS);
		pm25_sensor = FPSTR(SENSORS_IPS);
		pm05_sensor = FPSTR(SENSORS_IPS);

		nc010_value = last_value_IPS_N1;
		nc100_value = last_value_IPS_N10;
		nc025_value = last_value_IPS_N25;
		nc001_value = last_value_IPS_N01;
		nc003_value = last_value_IPS_N03;
		nc005_value = last_value_IPS_N05;
		nc050_value = last_value_IPS_N5;
	}

	if (cfg::sen5x_read)
	{
		if (memcmp(SEN5X_type, "SEN50", 6) == 0)
		{
			pm01_sensor = FPSTR(SENSORS_SEN50);
			pm10_sensor = FPSTR(SENSORS_SEN50);
			pm25_sensor = FPSTR(SENSORS_SEN50);
			t_sensor = h_sensor = voc_sensor = FPSTR(SENSORS_SEN50);
		}

		if (memcmp(SEN5X_type, "SEN54", 6) == 0)
		{
			pm01_sensor = FPSTR(SENSORS_SEN54);
			pm10_sensor = FPSTR(SENSORS_SEN54);
			pm25_sensor = FPSTR(SENSORS_SEN54);
			t_sensor = h_sensor = voc_sensor = FPSTR(SENSORS_SEN54);
		}

		if (memcmp(SEN5X_type, "SEN55", 6) == 0)
		{
			pm01_sensor = FPSTR(SENSORS_SEN55);
			pm10_sensor = FPSTR(SENSORS_SEN55);
			pm25_sensor = FPSTR(SENSORS_SEN55);
			t_sensor = h_sensor = voc_sensor = nox_sensor = FPSTR(SENSORS_SEN55);
		}

		pm01_value = last_value_SEN5X_P0;
		pm25_value = last_value_SEN5X_P2;
		pm04_value = last_value_SEN5X_P4;
		pm10_value = last_value_SEN5X_P1;

		nc005_value = last_value_SEN5X_N05;
		nc010_value = last_value_SEN5X_N1;
		nc025_value = last_value_SEN5X_N25;
		nc040_value = last_value_SEN5X_N4;
		nc100_value = last_value_SEN5X_N10;

		tps_value = last_value_SEN5X_TS;
		t_value = last_value_SEN5X_T;
		h_value = last_value_SEN5X_H;
		voc_value = last_value_SEN5X_VOC;
		nox_value = last_value_SEN5X_NOX;			// OLED display => NOx value.
	}

	if (cfg::sps30_read)
	{
		pm10_sensor = FPSTR(SENSORS_SPS30);
		pm25_sensor = FPSTR(SENSORS_SPS30);
		pm01_value = last_value_SPS30_P0;
		pm25_value = last_value_SPS30_P2;
		pm04_value = last_value_SPS30_P4;
		pm10_value = last_value_SPS30_P1;
		nc005_value = last_value_SPS30_N05;
		nc010_value = last_value_SPS30_N1;
		nc025_value = last_value_SPS30_N25;
		nc040_value = last_value_SPS30_N4;
		nc100_value = last_value_SPS30_N10;
		tps_value = last_value_SPS30_TS;
	}

	if (cfg::sds_read)
	{
		pm10_sensor = pm25_sensor = FPSTR(SENSORS_SDS011);
		pm10_value = last_value_SDS_P1;
		pm25_value = last_value_SDS_P2;
	}

	if (cfg::dht_read)
	{
		t_sensor = h_sensor = FPSTR(SENSORS_DHT22);
		t_value = last_value_DHT_T;
		h_value = last_value_DHT_H;
	}

	if (cfg::ds18b20_read)
	{
		t_sensor = FPSTR(SENSORS_DS18B20);
		t_value = last_value_DS18B20_T;
	}

	if (cfg::htu21d_read)
	{
		h_sensor = t_sensor = FPSTR(SENSORS_HTU21D);
		t_value = last_value_HTU21D_T;
		h_value = last_value_HTU21D_H;
	}
	if (cfg::bmp_read)
	{
		t_sensor = h_sensor = FPSTR(SENSORS_BMP180);
		t_value = last_value_BMP_T;
		p_value = last_value_BMP_P;
	}
	if (cfg::bmx280_read)
	{
		t_sensor = p_sensor = FPSTR(SENSORS_BMP280);
		t_value = last_value_BMX280_T;
		p_value = last_value_BMX280_P;
		if (bmx280.sensorID() == BME280_SENSOR_ID)
		{
			h_sensor = t_sensor = FPSTR(SENSORS_BME280);
			h_value = last_value_BME280_H;
		}
	}
	if (cfg::sht3x_read)
	{
		h_sensor = t_sensor = FPSTR(SENSORS_SHT3X);
		t_value = last_value_SHT3X_T;
		h_value = last_value_SHT3X_H;
	}

	if (cfg::dnms_read)
	{
		la_sensor = FPSTR(SENSORS_DNMS);
		la_eq_value = last_value_dnms_laeq;
		la_max_value = last_value_dnms_la_max;
		la_min_value = last_value_dnms_la_min;
	}
	if (cfg::gps_read)
	{
		lat_value = last_value_GPS_lat;
		lon_value = last_value_GPS_lon;
		alt_value = last_value_GPS_alt;
	}
	if (cfg::ppd_read || cfg::pms_read || cfg::hpm_read || cfg::sds_read)
	{
		screens[screen_count++] = 1;
	}

	if (cfg::npm_read)
	{
		screens[screen_count++] = 9;
		screens[screen_count++] = 10;
	}

	if (cfg::ips_read)
	{
		screens[screen_count++] = 11; // A VOIR POUR AJOUTER DES ÈCRANS
	}

	if (cfg::sen5x_read)
	{
		screens[screen_count++] = 12;
		screens[screen_count++] = 13;
	}

	if (cfg::sps30_read)
	{
		screens[screen_count++] = 2;
	}

	if (cfg::dht_read || cfg::ds18b20_read || cfg::htu21d_read || cfg::bmp_read || cfg::bmx280_read || cfg::sht3x_read)
	{
		screens[screen_count++] = 3;
	}

	if (cfg::scd30_read)
	{
		screens[screen_count++] = 4;
	}

	if (cfg::gps_read)
	{
		screens[screen_count++] = 5;
	}

	if (cfg::dnms_read)
	{
		screens[screen_count++] = 6;
	}

	if (cfg::display_wifi_info)
	{
		screens[screen_count++] = 7; // Wifi info
	}
	
	if (cfg::display_device_info)
	{
		screens[screen_count++] = 8; 		// chipID, firmware and count of measurements
	}

	// update size of "screens" when adding more screens!
	if (cfg::has_display || cfg::has_sh1106 || lcd_2004)
	{
		switch (screens[next_display_count % screen_count])
		{
		case 1:
			display_header = pm25_sensor;
			if (pm25_sensor != pm10_sensor)
			{
				display_header += " / " + pm10_sensor;
			}
			display_lines[0] = std::move(tmpl(F("PM2.5: {v} µg/m³"), check_display_value(pm25_value, -1, 1, 6)));
			display_lines[1] = std::move(tmpl(F("PM10: {v} µg/m³"), check_display_value(pm10_value, -1, 1, 6)));
			display_lines[2] = emptyString;
			break;

		case 2:
			display_header = FPSTR(SENSORS_SPS30);
			display_lines[0] = "PM: " + check_display_value(pm01_value, -1, 1, 4) + " " + check_display_value(pm25_value, -1, 1, 4) + " " + check_display_value(pm04_value, -1, 1, 4) + " " + check_display_value(pm10_value, -1, 1, 4);
			display_lines[1] = "NC: " + check_display_value(nc005_value, -1, 0, 3) + " " + check_display_value(nc010_value, -1, 0, 3) + " " + check_display_value(nc025_value, -1, 0, 3) + " " + check_display_value(nc040_value, -1, 0, 3) + " " + check_display_value(nc100_value, -1, 0, 3);
			display_lines[2] = std::move(tmpl(F("TPS: {v} µm"), check_display_value(tps_value, -1, 2, 5)));
			break;

		case 3:
			display_header = t_sensor;

			if (h_sensor && t_sensor != h_sensor)
			{
				display_header += " / " + h_sensor;
			}
			if ((h_sensor && p_sensor && (h_sensor != p_sensor)) || (h_sensor == "" && p_sensor && (t_sensor != p_sensor)))
			{
				display_header += " / " + p_sensor;
			}
			if (t_sensor != "")
			{
				display_lines[line_count] = "Temp.: ";
				display_lines[line_count] += check_display_value(t_value, -128, 1, 6);
				display_lines[line_count++] += " °C";
			}
			if (h_sensor != "")
			{
				display_lines[line_count] = "Hum.:  ";
				display_lines[line_count] += check_display_value(h_value, -1, 1, 6);
				display_lines[line_count++] += " %";
			}
			if (p_sensor != "")
			{
				display_lines[line_count] = "Pres.: ";
				display_lines[line_count] += check_display_value(p_value / 100, (-1 / 100.0), 1, 6);
				display_lines[line_count++] += " hPa";
			}
			while (line_count < 3)
			{
				display_lines[line_count++] = emptyString;
			}
			break;

		case 4:
			display_header = "SCD30";
			display_lines[0] = "Temp.: ";
			display_lines[0] += check_display_value(last_value_SCD30_T, -128, 1, 5);
			display_lines[0] += " °C";
			display_lines[1] = "Hum.:  ";
			display_lines[1] += check_display_value(last_value_SCD30_H, -1, 1, 5);
			display_lines[1] += " %";
			display_lines[2] = "CO2:   ";
			display_lines[2] += check_display_value(last_value_SCD30_CO2, 0, 0, 5);
			display_lines[2] += " ppm";
			break;

		case 5:
			display_header = "NEO6M";
			display_lines[0] = "Lat: ";
			display_lines[0] += check_display_value(lat_value, -200.0, 6, 10);
			display_lines[1] = "Lon: ";
			display_lines[1] += check_display_value(lon_value, -200.0, 6, 10);
			display_lines[2] = "Alt: ";
			display_lines[2] += check_display_value(alt_value, -1000.0, 2, 10);
			break;

		case 6:
			display_header = FPSTR(SENSORS_DNMS);
			display_lines[0] = std::move(tmpl(F("LAeq: {v} db(A)"), check_display_value(la_eq_value, -1, 1, 6)));
			display_lines[1] = std::move(tmpl(F("LA_max: {v} db(A)"), check_display_value(la_max_value, -1, 1, 6)));
			display_lines[2] = std::move(tmpl(F("LA_min: {v} db(A)"), check_display_value(la_min_value, -1, 1, 6)));
			break;

		case 7:
			display_header = F("Wifi info");
			display_lines[0] = "IP: ";
			display_lines[0] += WiFi.localIP().toString();
			display_lines[1] = "SSID: ";
			display_lines[1] += WiFi.SSID();
			display_lines[2] = std::move(tmpl(F("Signal: {v} %"), String(calcWiFiSignalQuality(last_signal_strength))));
			break;

		case 8:
			display_header = F("Device Info");
			display_lines[0] = "ID: ";
			display_lines[0] += esp_chipid;
			display_lines[1] = "FW: ";
			display_lines[1] += SOFTWARE_VERSION;
			display_lines[2] = F("Measurements: ");
			display_lines[2] += String(count_sends);
			break;

		case 9:
			display_header = F("Tera Next PM");
			display_lines[0] = std::move(tmpl(F("PM1: {v} µg/m³"), check_display_value(pm01_value, -1, 1, 6)));
			display_lines[1] = std::move(tmpl(F("PM2.5: {v} µg/m³"), check_display_value(pm25_value, -1, 1, 6)));
			display_lines[2] = std::move(tmpl(F("PM10: {v} µg/m³"), check_display_value(pm10_value, -1, 1, 6)));
			break;

		case 10:
			display_header = F("Tera Next PM");
			display_lines[0] = current_state_npm;
			display_lines[1] = F("T_NPM / RH_NPM");
			display_lines[2] = current_th_npm;
			break;

		case 11:
			display_header = F("Piera IPS-7100");
			display_lines[0] = std::move(tmpl(F("PM1: {v} µg/m³"), check_display_value(pm01_value, -1, 1, 6)));
			display_lines[1] = std::move(tmpl(F("PM2.5: {v} µg/m³"), check_display_value(pm25_value, -1, 1, 6)));
			display_lines[2] = std::move(tmpl(F("PM10: {v} µg/m³"), check_display_value(pm10_value, -1, 1, 6)));
			break;

		case 12:
			display_header = F("Sensirion SEN5X");
			display_lines[0] = std::move(tmpl(F("PM1: {v} µg/m³"), check_display_value(pm01_value, -1, 1, 6)));
			display_lines[1] = std::move(tmpl(F("PM2.5: {v} µg/m³"), check_display_value(pm25_value, -1, 1, 6)));
			display_lines[2] = std::move(tmpl(F("PM10: {v} µg/m³"), check_display_value(pm10_value, -1, 1, 6)));
			break;

		case 13:
			display_header = F("Sensirion SEN5X");

			if (memcmp(SEN5X_type, "SEN50", 6) == 0)
			{
			}
			else if (memcmp(SEN5X_type, "SEN54", 6) == 0)
			{
				display_lines[0] = std::move(tmpl(F("Temp.: {v} °C"), check_display_value(t_value, -128, 1, 6)));
				display_lines[1] = std::move(tmpl(F("Humi: {v} %"), check_display_value(h_value, -1, 1, 6)));
				display_lines[2] = std::move(tmpl(F("VOC: {v} (index)"), check_display_value(voc_value, -1, 1, 6)));
			}
			else if (memcmp(SEN5X_type, "SEN55", 6) == 0)
			{
				// display_lines[0] = std::move(tmpl(F("Humi: {v} %"), check_display_value(h_value, -1, 1, 6)));
				// display_lines[1] = std::move(tmpl(F(": {v} (index)"), check_display_value(voc_value, -1, 1, 6)));
				// display_lines[2] = std::move(tmpl(F("NO2: {v} (ppm)"), check_display_value(nox_value, -1, 1, 6)));

				display_lines[0] = std::move(tmpl(F("Temp.: {v} °C"), check_display_value(t_value, -128, 1, 6)));
				display_lines[1] = std::move(tmpl(F("Humi: {v} %"), check_display_value(h_value, -1, 1, 6)));
				display_lines[2] = std::move(tmpl(F("VOC: {v} (index)"), check_display_value(voc_value, -1, 1, 6)));
			}

			break;
		}

		// send display data to selected OLED hardware.
		if (oled_ssd1306)
		{
			oled_ssd1306->clear();
			oled_ssd1306->displayOn();
			oled_ssd1306->setTextAlignment(TEXT_ALIGN_CENTER);
			oled_ssd1306->drawString(64, 1, display_header);
			oled_ssd1306->setTextAlignment(TEXT_ALIGN_LEFT);
			oled_ssd1306->drawString(0, 16, display_lines[0]);
			oled_ssd1306->drawString(0, 28, display_lines[1]);
			oled_ssd1306->drawString(0, 40, display_lines[2]);
			oled_ssd1306->setTextAlignment(TEXT_ALIGN_CENTER);
			oled_ssd1306->drawString(64, 52, displayGenerateFooter(screen_count));
			oled_ssd1306->display();
		}

		if (oled_sh1106)
		{
			oled_sh1106->clear();
			oled_sh1106->displayOn();
			oled_sh1106->setTextAlignment(TEXT_ALIGN_CENTER);
			oled_sh1106->drawString(64, 1, display_header);
			oled_sh1106->setTextAlignment(TEXT_ALIGN_LEFT);
			oled_sh1106->drawString(0, 16, display_lines[0]);
			oled_sh1106->drawString(0, 28, display_lines[1]);
			oled_sh1106->drawString(0, 40, display_lines[2]);
			oled_sh1106->setTextAlignment(TEXT_ALIGN_CENTER);
			oled_sh1106->drawString(64, 52, displayGenerateFooter(screen_count));
			oled_sh1106->display();
		}

		if (lcd_2004)
		{
			display_header = std::move(String((next_display_count % screen_count) + 1) + '/' + String(screen_count) + ' ' + display_header);
			display_lines[0].replace(" µg/m³", emptyString);
			display_lines[0].replace("°", String(char(223)));
			display_lines[1].replace(" µg/m³", emptyString);
			display_lines[2].replace(" µg/m³", emptyString);
			lcd_2004->clear();
			lcd_2004->setCursor(0, 0);
			lcd_2004->print(display_header);
			lcd_2004->setCursor(0, 1);
			lcd_2004->print(display_lines[0]);
			lcd_2004->setCursor(0, 2);
			lcd_2004->print(display_lines[1]);
			lcd_2004->setCursor(0, 3);
			lcd_2004->print(display_lines[2]);
		}
	}

	// ----5----0----5----0
	// PM10/2.5: 1999/999
	// T/H: -10.0°C/100.0%
	// T/P: -10.0°C/1000hPa

	if (lcd_1602)
	{
		switch (screens[next_display_count % screen_count])
		{
		case 1:
			display_lines[0] = "PM2.5: ";
			display_lines[0] += check_display_value(pm25_value, -1, 1, 6);
			display_lines[1] = "PM10:  ";
			display_lines[1] += check_display_value(pm10_value, -1, 1, 6);
			break;
		case 2:
			display_lines[0] = "PM1.0: ";
			display_lines[0] += check_display_value(pm01_value, -1, 1, 4);
			display_lines[1] = "PM4: ";
			display_lines[1] += check_display_value(pm04_value, -1, 1, 4);
			break;

		case 3:
			display_lines[0] = std::move(tmpl(F("T: {v} °C"), check_display_value(t_value, -128, 1, 6)));
			display_lines[1] = std::move(tmpl(F("H: {v} %"), check_display_value(h_value, -1, 1, 6)));
			break;

		case 4:
			display_lines[0] = std::move(tmpl(F("T/H: {v}"), check_display_value(last_value_SCD30_T, -128, 1, 5) + " / " + check_display_value(last_value_SCD30_H, -1, 0, 3)));
			display_lines[1] = std::move(tmpl(F("CO2: {v} ppm"), check_display_value(last_value_SCD30_CO2, 0, 0, 6)));
			break;

		case 5:
			display_lines[0] = "Lat: ";
			display_lines[0] += check_display_value(lat_value, -200.0, 6, 11);
			display_lines[1] = "Lon: ";
			display_lines[1] += check_display_value(lon_value, -200.0, 6, 11);
			break;

		case 6:
			display_lines[0] = std::move(tmpl(F("LAeq: {v} db(A)"), check_display_value(la_eq_value, -1, 1, 6)));
			display_lines[1] = std::move(tmpl(F("LA_max: {v} db(A)"), check_display_value(la_max_value, -1, 1, 6)));
			break;

		case 7:
			display_lines[0] = WiFi.localIP().toString();
			display_lines[1] = WiFi.SSID();
			break;

		case 8:
			display_lines[0] = "ID: ";
			display_lines[0] += esp_chipid;
			display_lines[1] = "FW: ";
			display_lines[1] += SOFTWARE_VERSION;
			break;

		case 9:
			display_lines[0] = "PM1: ";
			display_lines[0] += check_display_value(pm01_value, -1, 1, 6);
			display_lines[1] = "PM2.5: ";
			display_lines[1] += check_display_value(pm25_value, -1, 1, 6);
			break;

		case 10:
			display_lines[0] = current_state_npm;
			display_lines[1] = current_th_npm;
			break;

		case 11:
			display_lines[0] = "PM1: ";
			display_lines[0] += check_display_value(pm01_value, -1, 1, 6);
			display_lines[1] = "PM2.5: ";
			display_lines[1] += check_display_value(pm25_value, -1, 1, 6);
			break;
			
		case 12:
			display_lines[0] = "PM1: ";
			display_lines[0] += check_display_value(pm01_value, -1, 1, 6);
			display_lines[1] = "PM2.5: ";
			display_lines[1] += check_display_value(pm25_value, -1, 1, 6);
			break;
		case 13:
			display_lines[0] = "Temp.: ";
			display_lines[0] += check_display_value(pm01_value, -1, 1, 6);
			display_lines[1] = "Humi.: ";
			display_lines[1] += check_display_value(pm25_value, -1, 1, 6);
			break;
		}

		display_lines[0].replace("°", String(char(223)));

		lcd_1602->clear();
		lcd_1602->setCursor(0, 0);
		lcd_1602->print(display_lines[0]);
		lcd_1602->setCursor(0, 1);
		lcd_1602->print(display_lines[1]);
	}

	yield();						// run other waiting threads (gives CPU time)
	next_display_count++;
}

/*****************************************************************
 * Init LCD/OLED display                                         *
 *****************************************************************/
static void init_display()
{
	if (cfg::has_display)
	{
		oled_ssd1306 = new SSD1306(0x3c, I2C_PIN_SDA, I2C_PIN_SCL);
		oled_ssd1306->init();

		if (cfg::has_flipped_display)
		{
			oled_ssd1306->flipScreenVertically();
		}
	}

	if (cfg::has_sh1106)
	{
		oled_sh1106 = new SH1106(0x3c, I2C_PIN_SDA, I2C_PIN_SCL);
		oled_sh1106->init();
		if (cfg::has_flipped_display)
		{
			oled_sh1106->flipScreenVertically();
		}
	}

	if (cfg::has_lcd1602)
	{
		lcd_1602 = new LiquidCrystal_I2C(
			lcd_1602_default_i2c_address,
			lcd_1602_columns,
			lcd_1602_rows);
	}
	else if (cfg::has_lcd1602_27)
	{
		lcd_1602 = new LiquidCrystal_I2C(
			lcd_1602_alternate_i2c_address,
			lcd_1602_columns,
			lcd_1602_rows);
	}

	if (lcd_1602)
	{
		lcd_1602->init();
		lcd_1602->backlight();
	}

	if (cfg::has_lcd2004)
	{
		lcd_2004 = new LiquidCrystal_I2C(
			lcd_2004_default_i2c_address,
			lcd_2004_columns,
			lcd_2004_rows);
	}
	else if (cfg::has_lcd2004_27)
	{
		lcd_2004 = new LiquidCrystal_I2C(
			lcd_2004_alternate_i2c_address,
			lcd_2004_columns,
			lcd_2004_rows);
	}

	if (lcd_2004)
	{
		lcd_2004->init();
		lcd_2004->backlight();
	}

	// reset back to 100k as the OLEDDisplay initialization is
	// modifying the I2C speed to 400k, which overwhelms some of the
	// sensors.
	Wire.setClock(100000);
	Wire.setClockStretchLimit(150000);
}

/*****************************************************************
 * Init BMP280/BME280                                            *
 *****************************************************************/
static bool initBMX280(char addr)
{
	debug_out(String(F("Trying BMx280 sensor on ")) + String(addr, HEX), DEBUG_MIN_INFO);

	if (bmx280.begin(addr))
	{
		debug_outln_info(FPSTR(DBG_TXT_FOUND));
		bmx280.setSampling(
			BMX280::MODE_FORCED,
			BMX280::SAMPLING_X1,
			BMX280::SAMPLING_X1,
			BMX280::SAMPLING_X1);
		return true;
	}
	else
	{
		debug_outln_info(FPSTR(DBG_TXT_NOT_FOUND));
		return false;
	}
}

/*****************************************************************
   Init SEN5X Sensor
 *****************************************************************/
static void initSEN5X()
{
	debug_outln(F("Start to Initialize SEN5X sensor."), DEBUG_MIN_INFO);

	sen5x.begin(Wire);

	uint16_t error;
	char errorMessage[256];

	error = sen5x.deviceReset();
	if (error)
	{
		Debug.print("SEN5X_DeviceReset() return Error: ");
		errorToString(error, errorMessage, 256);
		Debug.println(errorMessage);
		debug_outln_error(F("Check SEN5x sensor is NOT connected!"));

		is_Sen5x_init_failed = true;
		return;
	}

#ifdef USE_PRODUCT_INFO
	printSerialNumber();
	printModuleVersions();
#endif

	if (sen5x.setFanAutoCleaningInterval(SEN5X_AUTO_CLEANING_INTERVAL) != 0)
	{
		debug_outln_error(F("setting of Auto Cleaning Intervall SEN5X failed!"));
		is_Sen5x_init_failed = true;
		return;
	}

    // Adjust tempOffset to account for additional temperature offsets
    // exceeding the SEN module's self heating.
    float tempOffset = readCorrectionOffset(cfg::temp_correction);		//String(cfg::temp_correction).toFloat();
    error = sen5x.setTemperatureOffsetSimple(tempOffset);

    if (error) 
	{
        Debug.print("Error trying to execute setTemperatureOffsetSimple(): ");
        errorToString(error, errorMessage, 256);
        Debug.println(errorMessage);
    } 
	else 
	{
        Debug.print("Temperature Offset = ");
        Debug.print(tempOffset);
        Debug.println(" deg. Celsius (SEN54/SEN55 only)");
    }

	error = sen5x.startMeasurement();

	if (error)
	{
		Debug.print("Error trying to execute startMeasurement(): ");
		errorToString(error, errorMessage, 256);
		Debug.println(errorMessage);
		is_Sen5x_init_failed = true;
		//return;
	}
	else
	{
		debug_outln(F("SEN5X sensor active. Sensor Fan Cleaning and Warm-Up for the first Measurement."), DEBUG_MIN_INFO);
		sen5x.startFanCleaning();
		is_SEN5X_running = false;
	}
}

/*****************************************************************
   Init SPS30 PM Sensor
 *****************************************************************/
static void initSPS30()
{
	char serial[SPS_MAX_SERIAL_LEN];
	debug_out(F("Trying SPS30 sensor on 0x69H "), DEBUG_MIN_INFO);
	sps30_reset();
	delay(200);

	if (sps30_get_serial(serial) != 0)
	{
		debug_outln_info(FPSTR(DBG_TXT_NOT_FOUND));

		debug_outln_info(F("Check SPS30 wiring"));
		sps30_init_failed = true;
		return;
	}

	debug_outln_info(F(" ... found, Serial-No.: "), String(serial));
	if (sps30_set_fan_auto_cleaning_interval(SPS30_AUTO_CLEANING_INTERVAL) != 0)
	{
		debug_outln_error(F("setting of Auto Cleaning Intervall SPS30 failed!"));
		sps30_init_failed = true;
		return;
	}

	delay(100);
	if (sps30_start_measurement() != 0)
	{
		debug_outln_error(F("SPS30 error starting measurement"));
		sps30_init_failed = true;
		return;
	}
}

/*****************************************************************
   Init DNMS - Digital Noise Measurement Sensor
 *****************************************************************/
static void initDNMS()
{
	char dnms_version[DNMS_MAX_VERSION_LEN + 1];

	debug_out(F("Trying DNMS sensor on 0x55H "), DEBUG_MIN_INFO);
	dnms_reset();
	delay(1000);
	if (dnms_read_version(dnms_version) != 0)
	{
		debug_outln_info(FPSTR(DBG_TXT_NOT_FOUND));
		debug_outln_error(F("Check DNMS wiring"));
		dnms_init_failed = true;
	}
	else
	{
		dnms_version[DNMS_MAX_VERSION_LEN] = 0;
		debug_outln_info(FPSTR(DBG_TXT_FOUND), String(": ") + String(dnms_version));
	}
}

/*****************************************************************
   Functions: Power-On Test Sensors.
 *****************************************************************/
static void powerOnTestSensors()
{
	if (cfg::ppd_read)
	{
		pinMode(PPD_PIN_PM1, INPUT_PULLUP); // Listen at the designated PIN
		pinMode(PPD_PIN_PM2, INPUT_PULLUP); // Listen at the designated PIN
		debug_outln_info(F("Read PPD..."));
	}

	if (cfg::sds_read)
	{
		debug_outln_info(F("Read SDS...: "), SDS_version_date());
		SDS_cmd(PmSensorCmd::ContinuousMode);
		delay(100);
		debug_outln_info(F("Stopping SDS011..."));
		is_SDS_running = SDS_cmd(PmSensorCmd::Stop);

		if( !is_SDS_running)
		{
			debug_outln_info(F("SDS011 ready for readings..."));
		}
	}

	if (cfg::pms_read)
	{
		debug_outln_info(F("Read PMS(1,3,5,6,7)003..."));
		PMS_cmd(PmSensorCmd::Start);
		delay(100);
		PMS_cmd(PmSensorCmd::ContinuousMode);
		delay(100);
		debug_outln_info(F("Stopping PMS..."));
		is_PMS_running = PMS_cmd(PmSensorCmd::Stop);
	}

	if (cfg::hpm_read)
	{
		debug_outln_info(F("Read HPM..."));
		HPM_cmd(PmSensorCmd::Start);
		delay(100);
		HPM_cmd(PmSensorCmd::ContinuousMode);
		delay(100);
		debug_outln_info(F("Stopping HPM..."));
		is_HPM_running = HPM_cmd(PmSensorCmd::Stop);
	}

	if (cfg::npm_read)
	{
		uint8_t test_state;
		delay(15000); // wait a bit to be sure Next PM is ready to receive instructions.
		test_state = NPM_get_state();
		if (test_state == 0x00)
		{
			debug_outln_info(F("NPM already started..."));
			is_NPM_running = true;
		}
		else if (test_state == 0x01)
		{
			debug_outln_info(F("Force start NPM...")); // to read the firmware version
			is_NPM_running = NPM_start_stop();
		}
		else
		{
			if (bitRead(test_state, 1) == 1)
			{
				debug_outln_info(F("Degraded state"));
			}
			else
			{
				debug_outln_info(F("Default state"));
			}
			if (bitRead(test_state, 2) == 1)
			{
				debug_outln_info(F("Not ready"));
			}
			if (bitRead(test_state, 3) == 1)
			{
				debug_outln_info(F("Heat error"));
			}
			if (bitRead(test_state, 4) == 1)
			{
				debug_outln_info(F("T/RH error"));
			}

			if (bitRead(test_state, 5) == 1)
			{
				debug_outln_info(F("Fan error"));

				// if (bitRead(test_state, 0) == 1)
				//{
				// 	debug_outln_info(F("Force start NPM..."));
				// 	is_NPM_running => Stop
				// 	NPM_start_stop();
				// 	delay(5000);
				// }
				//
				// NPM_fan_speed();
				// delay(5000);
			}

			if (bitRead(test_state, 6) == 1)
			{
				debug_outln_info(F("Memory error"));
			}

			if (bitRead(test_state, 7) == 1)
			{
				debug_outln_info(F("Laser error"));
			}

			if (bitRead(test_state, 0) == 0)
			{
				debug_outln_info(F("NPM already started..."));
			}
			else
			{
				debug_outln_info(F("Force start NPM..."));
				is_NPM_running = NPM_start_stop();
			}
		}

		delay(15000); // prevent any buffer overload on ESP82666
		NPM_version_date();
		delay(3000); // prevent any buffer overload on ESP82666
		NPM_temp_humi();
		delay(2000);

		if (!cfg::npm_fulltime)
		{
			is_NPM_running = NPM_start_stop();
			delay(2000); // prevent any buffer overload on ESP82666
		}
		else
		{
			is_NPM_running = true;
		}
	}

	if (cfg::ips_read)
	{
		IPS_cmd(PmSensorCmd3::Factory); // set to Factory
		delay(1000);
		IPS_version_date();
		delay(1000);
		IPS_cmd(PmSensorCmd3::Smoke); // no smoke detection
		delay(1000);
		IPS_cmd(PmSensorCmd3::Interval); // Set interval to 0 = manual mode
		delay(1000);
		IPS_cmd(PmSensorCmd3::Stop);
		delay(1000);
		is_IPS_running = false;
	}

	if (cfg::sen5x_read)
	{
		debug_outln_info(F("Read SEN5X..."));
		initSEN5X();
	}

	if (cfg::sps30_read)
	{
		debug_outln_info(F("Read SPS30..."));
		initSPS30();
	}

	if (cfg::dht_read)
	{
		dht.begin(); // Start DHT
		debug_outln_info(F("Read DHT..."));
	}

	if (cfg::htu21d_read)
	{
		debug_outln_info(F("Read HTU21D..."));
		// begin() might return false when using Si7021
		// so validate reading via Humidity (will return 0.0 when failed)
		if (!htu21d.begin() && htu21d.readHumidity() < 1.0f)
		{
			debug_outln_error(F("Check HTU21D wiring"));
			htu21d_init_failed = true;
		}
	}

	if (cfg::bmp_read)
	{
		debug_outln_info(F("Read BMP..."));
		if (!bmp.begin())
		{
			debug_outln_error(F("No valid BMP085 sensor, check wiring!"));
			bmp_init_failed = true;
		}
	}

	if (cfg::bmx280_read)
	{
		debug_outln_info(F("Read BMX280..."));
		if (!initBMX280(bmx280_default_i2c_address) && !initBMX280(bmx280_alternate_i2c_address))
		{
			debug_outln_error(F("Check BMX280 wiring"));
			bmx280_init_failed = true;
		}
		else
		{
			debug_outln_info(F("BMX280 connected..."));
		}
	}

	if (cfg::sht3x_read)
	{
		debug_outln_info(F("Read SHT3x..."));
		if (!sht3x.begin())
		{
			debug_outln_error(F("Check SHT3x wiring"));
			sht3x_init_failed = true;
		}
		else
		{
			debug_outln_info(F("SHT3x connected..."));
		}
	}

	if (cfg::scd30_read)
	{
		debug_outln_info(F("Read SCD30..."));

		if (!scd30.begin())		//set autoCalibrate flag to false (default).
		{
			debug_outln_error(F("Check SCD30 wiring"));
			scd30_init_failed = true;
		}
		else
		{
			if( scd30.isConnected())
			{
				// set Measurement Time Interval to 30 seconds between measurements.
				scd30.setMeasurementInterval(MEASUREMENT_INTERVAL_SCD30_S);

				// Optionally set temperature offset to ec. 5°C.
				float tempOffset = readCorrectionOffset(cfg::scd30_temp_correction);
    			scd30.setTemperatureOffset(tempOffset);

				// set Recalibration reference value to SCD30. (standard 400ppm)
				tempOffset = readCorrectionOffset(cfg::scd30_co2_correction);
				scd30.setForcedRecalibrationFactor((uint16_t)tempOffset);

				uint16_t settingVal = 0;
				scd30.getFirmwareVersion(&settingVal);
				debug_outln_info(F("SCD30 connected... Version: ") + String( ((float)settingVal) / 100));
			}
			else
			{
				debug_outln_info(F("SCD30 NOT connected, restart Device..."));
			}
		}
	}

	if (cfg::ds18b20_read)
	{
		oneWire.begin(ONEWIRE_PIN);
		ds18b20.begin(); // Start DS18B20
		debug_outln_info(F("Read DS18B20..."));
	}

	if (cfg::dnms_read)
	{
		debug_outln_info(F("Read DNMS..."));
		initDNMS();
	}
}

/*
	Display list of API's servers
*/
static void logEnabledAPIs()
{
	debug_outln_info(F("Send to :"));

	if (cfg::send2dusti)
	{
		debug_outln_info(F("sensor.community"));
	}

	if (cfg::send2fsapp)
	{
		debug_outln_info(F("Feinstaub-App"));
	}

	if (cfg::send2madavi)
	{
		debug_outln_info(F("Madavi.de"));
	}

	if (cfg::send2csv)
	{
		debug_outln_info(F("Serial as CSV"));
	}

	if (cfg::send2custom)
	{
		debug_outln_info(F("custom API"));
	}
	
	if (cfg::send2mqtt)
	{
		debug_outln_info(F("MQTT broker: "), String(cfg::mqtt_server));
		debug_outln_info(F(" - Port: "), String(cfg::mqtt_port));
		debug_outln_info(F(" - User: "), String(cfg::mqtt_user));
//		debug_outln_info(F(" - Pasword: "), String(cfg::mqtt_pwd));
		debug_outln_info(F(" - Topic: "), String(cfg::mqtt_topic));
		debug_outln_info(F(" - MQTT-client: "), String(mqtt_client_id));

	}

	if (cfg::send2aircms)
	{
		debug_outln_info(F("aircms API"));
	}

	if (cfg::send2influx)
	{
		debug_outln_info(F("custom influx DB"));
	}

	if (cfg::send2sensemap)
	{
		debug_outln_info(F("OpenSenseMap.org"));
	}

	debug_outln_info(FPSTR(DBG_TXT_SEP));

/*
	if (cfg::auto_update)
	{
		debug_outln_info(F("Auto-Update active..."));
	}
*/

}

/*

*/
static void logEnabledDisplays()
{
	if (cfg::has_display || cfg::has_sh1106)
	{
		debug_outln_info(F("Show on OLED..."));
	}

	if (lcd_1602)
	{
		debug_outln_info(F("Show on LCD 1602 ..."));
	}

	if (lcd_2004)
	{
		debug_outln_info(F("Show on LCD 2004 ..."));
	}
}

static void setupNetworkTime()
{
	// server name ptrs must be persisted after the call to configTime because internally
	// the pointers are stored see implementation of lwip sntp_setservername()
	static char ntpServer1[18], ntpServer2[18];

#if defined(ESP8266)
	settimeofday_cb([]()						// optional: set callback function if time was sent from NTPSERVER.
						{
							if (!sntp_time_set)
							{
								time_t now = time(nullptr);
								debug_outln_info(F("SNTP synced: "), ctime(&now));
								twoStageOTAUpdate();
								last_update_attempt = millis();
							}

							sntp_time_set++; 
						});
#endif

	strcpy_P(ntpServer1, NTP_SERVER_1);
	strcpy_P(ntpServer2, NTP_SERVER_2);
	//configTime(0, 0, ntpServer1, ntpServer2);

	configTime(MY_TZ, 0, ntpServer1, ntpServer2);	// set Daylight Saving => NTP with auto-switching between summer/winter time.
}

/*
	send sensor value to other web-server's to store sensor-data into databases.
*/
static unsigned long sendDataToOptionalApis(const String &data)
{
	unsigned long sum_send_time = 0;

	if (cfg::send2madavi)
	{
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("madavi.de: "));

		if (cfg::sen5x_read && (!is_Sen5x_init_failed))
		{
			debug_outln_info(F("Emulate SEN55:"));
			debug_outln_info(F("TH - "), FPSTR(cfg::sen5x_sym_th));
			debug_outln_info(F("PM - "), FPSTR(cfg::sen5x_sym_pm));

			RESERVE_STRING(data_sensemap, LARGE_STR);
			data_sensemap = data;
			data_sensemap.replace("SEN55", cfg::sen5x_sym_pm);	// set PM sensor Name.
			data_sensemap.replace("SEN5X", cfg::sen5x_sym_th);	// set temp/hummidity/NOx sensor Name.

			sum_send_time += sendData(LoggerMadavi, data_sensemap, 0, HOST_MADAVI, URL_MADAVI);
		}
		else
		{
			sum_send_time += sendData(LoggerMadavi, data, 0, HOST_MADAVI, URL_MADAVI);
		}
	}

	if (cfg::send2sensemap && (cfg::senseboxid[0] != '\0'))
	{
		if (cfg::sen5x_read && (!is_Sen5x_init_failed))
		{	// OpenSenseMap
			// RESERVE_STRING(data_sensemap, MED_STR);			// LARGE_STR
			// data_sensemap = FPSTR( (String("{ \"") + String(JSON_SENSOR_DATA_VALUES) + String("\":[")).c_str() );	//FPSTR(data_first_part);
			// this works
			// add_Value2Json(data_sensemap, F("SPS30_P0"), F("PM1.0: "), last_value_SEN5X_P0);
			// add_Value2Json(data_sensemap, F("SPS30_P2"), F("PM2.5: "), last_value_SEN5X_P2);
			// add_Value2Json(data_sensemap, F("SPS30_P4"), F("PM4.0: "), last_value_SEN5X_P4);
			// add_Value2Json(data_sensemap, F("SPS30_P1"), F("PM 10: "), last_value_SEN5X_P1);
			// add_Value2Json(data_sensemap, F("SCD30_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_SEN5X_T);
			// add_Value2Json(data_sensemap, F("SCD30_humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_SEN5X_H);

			// data_sensemap.remove(data_sensemap.length() - 1);			// remove ',' char.
			// data_sensemap += "]}";										// Add end markers
			// or this

			RESERVE_STRING(data_sensemap, LARGE_STR);
			data_sensemap = data;
			data_sensemap.replace("SEN55", cfg::sen5x_sym_pm); 				// set PM sensor Name
			data_sensemap.replace("SEN5X", cfg::sen5x_sym_th);				// set temp/hummidity/NOx sensor Name

			data_sensemap.replace("signal", "wifi_signal");					// Wifi signal info
			
				// end

			debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("opensensemap: "));
			// debug_outln_info(F("Emulate SEN55 -"));
			// debug_outln_info(F("TH - "), FPSTR(cfg::sen5x_sym_th));
			// debug_outln_info(F("PM - "), FPSTR(cfg::sen5x_sym_pm));

			String sensemap_path(tmpl(FPSTR(URL_SENSEMAP), cfg::senseboxid));
			sum_send_time += sendData(LoggerSensemap, data_sensemap, 0, HOST_SENSEMAP, sensemap_path.c_str());

			debug_outln_info(F("opensensemap data: "), data_sensemap);
		}
		else
		{
			debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("opensensemap: "));
			
			String sensemap_path(tmpl(FPSTR(URL_SENSEMAP), cfg::senseboxid));
			sum_send_time += sendData(LoggerSensemap, data, 0, HOST_SENSEMAP, sensemap_path.c_str());
		}
	}

	if (cfg::send2fsapp)
	{
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("Server FS App: "));
		sum_send_time += sendData(LoggerFSapp, data, 0, HOST_FSAPP, URL_FSAPP);
	}

	if (cfg::send2aircms)
	{
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("aircms.online: "));
		unsigned long ts = millis() / 1000;
		String token = WiFi.macAddress();
		String aircms_data("L=");
		aircms_data += esp_chipid;
		aircms_data += "&t=";
		aircms_data += String(ts, DEC);
		aircms_data += F("&airrohr=");
		aircms_data += data;
		String aircms_url(FPSTR(URL_AIRCMS));
		aircms_url += hmac1(sha1Hex(token), aircms_data + token);

		sum_send_time += sendData(Loggeraircms, aircms_data, 0, HOST_AIRCMS, aircms_url.c_str());
	}

	if (cfg::send2influx)
	{
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("custom influx db: "));
		RESERVE_STRING(data_4_influxdb, LARGE_STR);
		create_influxdb_string_from_data(data_4_influxdb, data);
		sum_send_time += sendData(LoggerInflux, data_4_influxdb, 0, cfg::host_influx, cfg::url_influx);
	}

	if (cfg::send2custom)
	{
		String data_to_send = data;
		data_to_send.remove(0, 1);
		String data_4_custom(F("{\"esp8266id\": \""));
		data_4_custom += esp_chipid;
		data_4_custom += "\", ";
		data_4_custom += data_to_send;
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("custom api: "));
		sum_send_time += sendData(LoggerCustom, data_4_custom, 0, cfg::host_custom, cfg::url_custom);
	}

	if (cfg::send2csv)
	{
		debug_outln_info(F("## Sending as csv: "));
		send_csv(data);
	}

#if defined(ESP8266)
	// MQTT send process.
	if (cfg::send2mqtt)
	{
		debug_out(String(DBG_TXT_SENDING_TO) + String("mqtt: "), DEBUG_MIN_INFO);
		sum_send_time += sendmqtt(data);
	}
#endif

	return sum_send_time;
}

/*****************************************************************
 * The Setup() => OS call                                        *
 *****************************************************************/
void setup(void)
{
	Debug.begin(9600); 												// Output to Serial at 9600 baud

#if defined(ESP8266)
	esp_chipid = std::move(String(ESP.getChipId()));				// get MCU chip serial number.
	esp_mac_id = std::move(String(WiFi.macAddress().c_str()));		// get WIFI chip mac. number.
	esp_mac_id.replace(":", "");
	esp_mac_id.toLowerCase();
#endif

#if defined(ESP32)
	uint64_t chipid_num;
	chipid_num = ESP.getEfuseMac();
	esp_chipid = String((uint16_t)(chipid_num >> 32), HEX);
	esp_chipid += String((uint32_t)chipid_num, HEX);
#endif

	cfg::initNonTrivials(esp_chipid.c_str());

	debug_outln_info(F("airRohr: " SOFTWARE_VERSION_STR "/"), String(CURRENT_LANG));

	// TEST TEST
	delay(2000);

#if defined(ESP8266)
	if ((airrohr_selftest_failed = !ESP.checkFlashConfig() /* after 2.7.0 update: || !ESP.checkFlashCRC() */ ))
	{
		debug_outln_error(F("ERROR: SELF TEST FAILED!"));
		SOFTWARE_VERSION += F("-STF");
	}
#endif

	// Init: get configuration settings from FileSystem
	init_config();

	Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL);

	if (cfg::npm_read)
	{
#if defined(ESP8266)
		serialNPM.begin(115200, SWSERIAL_8E1, PM_SERIAL_RX, PM_SERIAL_TX);
		serialNPM.enableIntTx(false);
#endif

#if defined(ESP32)
		serialNPM.begin(115200, SERIAL_8E1, PM_SERIAL_RX, PM_SERIAL_TX);
#endif
		Debug.println("Read Next PM... serialNPM 115200 8E1");
		serialNPM.setTimeout(400);
	}
	else if (cfg::ips_read)
	{
// #define SERIAL_BUFFER_SIZE 256
#if defined(ESP8266)
		serialIPS.begin(115200, SWSERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX);
		serialIPS.enableIntTx(false);
#endif

#if defined(ESP32)
		serialIPS.begin(115200, SERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX);
#endif
		Debug.println("Read IPS... serialIPS 115200 8N1"); // will be set to 9600 8N1 afterwards
		serialIPS.setTimeout(900);						   // Which timeout?
	}
	else
	{

#if defined(ESP8266)
		serialSDS.begin(9600, SWSERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX);
		serialSDS.enableIntTx(true);
#endif

#if defined(ESP32)
		serialSDS.begin(9600, SERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX);
#endif
		Debug.println("No Next PM... serialSDS 9600 8N1");
		serialSDS.setTimeout((4 * 12 * 1000) / 9600);
	}

#if defined(WIFI_LoRa_32_V2)
	// reset the OLED display, e.g. of the heltec_wifi_lora_32 board
	pinMode(RST_OLED, OUTPUT);
	digitalWrite(RST_OLED, LOW);
	delay(50);
	digitalWrite(RST_OLED, HIGH);
#endif

/*	Debug -Tijdelijk verplaatst naar regel 8212 ivm Wifi start langzaam/actief ... 5 minuten..?
	if(cfg::has_s7000)
	{
		SIM700LTEConnect();
	}
*/

	init_display();
	setupNetworkTime();			// set Callback function ptr into NTPSERVER function callback table.
	connectWifi();
	setup_webserver();
	createLoggerConfigs();

	debug_outln_info(F("\nChipId: "), esp_chipid);
	debug_outln_info(F("\nMAC Id: "), esp_mac_id);

#if defined(ESP8266)
	if(cfg::send2mqtt)
	{// MQTT => set Client_id.
		strcpy(mqtt_client_id, SSID_BASENAME);
		strcat(mqtt_client_id, esp_chipid.c_str());			// airRohr-<chipid>
		debug_outln_info(F("MQTT Client_id = ") + String(mqtt_client_id));

		if (cfg::has_radarmotion)
		{
			// implementation of MQTT communication.
			setup_mqtt_broker( cfg::mqtt_server, cfg::mqtt_port);
			RCWL0516.setMQTTClient(mqtt_client, mqtt_header, mqtt_lwt_header);
			debug_outln_info(F("RCWL_0516 => set MQTT Client instance."));
		}
	}
#endif

	if (cfg::has_s7000)
	{
		debug_outln_info(F("\nSim7000 try to connect."));
		SIM700LTEConnect();
	}

	if (cfg::gps_read)
	{
#if defined(ESP8266)
		serialGPS = new SoftwareSerial;
		serialGPS->begin(9600, SWSERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX, false, 128);
#endif

#if defined(ESP32)
		serialGPS->begin(9600, SERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX);
#endif

		debug_outln_info(F("Read GPS..."));
		disable_unneeded_nmea();
	}

	powerOnTestSensors();
	logEnabledAPIs();
	logEnabledDisplays();

	delay(50);

	starttime = millis(); // store the start time
	last_update_attempt = time_point_device_start_ms = starttime;
	
	if (cfg::npm_read)
	{
		last_display_millis = starttime_NPM = starttime;
	}
	else
	{
		last_display_millis = starttime_SDS = starttime;
	}

	// Radar Motion.
	if (cfg::has_radarmotion)
	{
		debug_outln_info(F("Start to Initialize Radar motion sensor (RCWL_0516)."));

		RCWL0516.init(cfg::motion_wait_time); // set wait max time value. in sec.

		if(!RCWL0516.begin(cfg::host_radar, cfg::port_radar))
		{
			debug_outln_info(F("Couldn't connected to Motion Server: "), String(cfg::host_radar) + F(":") + String(cfg::port_radar));
		}
		else
		{
			debug_outln_info(F("RCWL_0516 => Radar motion driver started."));
		}
	}

} // end setup()

/*****************************************************************
 * OS callback: message Loop => Take action                      *
 *****************************************************************/
void loop(void)
{
	unsigned long sleep = SLEEPTIME_MS;
	String result_PPD, result_SDS, result_PMS, result_HPM, result_NPM, result_IPS;
	String result_GPS, result_DNMS;

	unsigned sum_send_time = 0;

	act_micro = micros();
	act_milli = millis();
	send_now = msSince(starttime) > cfg::sending_intervall_ms;

	if (send_now)
	{
		sleep = 0;
	}

	// Wait at least 30s for each NTP server to sync
	if (!sntp_time_set && send_now &&
		msSince(time_point_device_start_ms) < 1000 * 2 * 30 + 5000)
	{
		debug_outln_info(F("NTP sync not finished yet, skipping send"));
		send_now = false;
		starttime = act_milli;
	}

	sample_count++;
	if (last_micro != 0)
	{
		unsigned long diff_micro = act_micro - last_micro;
		UPDATE_MIN_MAX(min_micro, max_micro, diff_micro);
	}

	last_micro = act_micro;

	if (cfg::sps30_read && (!sps30_init_failed))
	{
		if ((msSince(starttime) - SPS30_read_timer) > SPS30_WAITING_AFTER_LAST_READ)
		{
			struct sps30_measurement sps30_values;
			int16_t ret_SPS30;

			SPS30_read_timer = msSince(starttime);

			ret_SPS30 = sps30_read_measurement(&sps30_values);
			++SPS30_read_counter;

			if (ret_SPS30 < 0)
			{
				debug_outln_info(F("SPS30 error reading measurement"));
				SPS30_read_error_counter++;
			}
			else
			{
				if (SPS_IS_ERR_STATE(ret_SPS30))
				{
					debug_outln_info(F("SPS30 measurements may not be accurate"));
					SPS30_read_error_counter++;
				}

				value_SPS30_P0 += sps30_values.mc_1p0;
				value_SPS30_P2 += sps30_values.mc_2p5;
				value_SPS30_P4 += sps30_values.mc_4p0;
				value_SPS30_P1 += sps30_values.mc_10p0;
				value_SPS30_N05 += sps30_values.nc_0p5;
				value_SPS30_N1 += sps30_values.nc_1p0;
				value_SPS30_N25 += sps30_values.nc_2p5;
				value_SPS30_N4 += sps30_values.nc_4p0;
				value_SPS30_N10 += sps30_values.nc_10p0;
				value_SPS30_TS += sps30_values.tps;

				++SPS30_measurement_count;
			}
		}
	}

	if (cfg::sen5x_read && (!is_Sen5x_init_failed))
	{// get SEN5X sensor values.
		GetSen5XSensorData();
	}

	if (cfg::ppd_read)
	{
		fetchSensorPPD(result_PPD);
	}

	if (cfg::npm_read)
	{
		if ((msSince(starttime_NPM) > SAMPLETIME_NPM_MS) || send_now)
		{
			starttime_NPM = act_milli;
			fetchSensorNPM(result_NPM);
		}
	}
	else if (cfg::ips_read)
	{
		if ( (msSince(starttime_IPS) > SAMPLETIME_IPS_MS) || send_now)
		{
			starttime_IPS = act_milli;
			fetchSensorIPS(result_IPS);
		}
	}
	else
	{
		if ((msSince(starttime_SDS) > SAMPLETIME_SDS_MS) || send_now)
		{
			starttime_SDS = act_milli;

			if (cfg::sds_read)
			{
				fetchSensorSDS(result_SDS);
			}

			if (cfg::pms_read)
			{
				fetchSensorPMS(result_PMS);
			}

			if (cfg::hpm_read)
			{
				fetchSensorHPM(result_HPM);
			}
		}
	}

	if (cfg::gps_read && !gps_init_failed)
	{
		// process serial GPS data..
		while (serialGPS->available() > 0)
		{
			gps.encode(serialGPS->read());
		}

		if ((msSince(starttime_GPS) > SAMPLETIME_GPS_MS) || send_now)
		{
			// getting GPS coordinates
			fetchSensorGPS(result_GPS);
			starttime_GPS = act_milli;
		}
	}

	if ( (msSince(last_display_millis) > DISPLAY_UPDATE_INTERVAL_MS) &&
		 (cfg::has_display || cfg::has_sh1106 || lcd_1602 || lcd_2004) )
	{
		display_values();
		last_display_millis = act_milli;
	}

	server.handleClient();				// when a connection is make by iphone/tablet/... to the home webpage of sensor app.

	yield();

	if (send_now)
	{
		last_signal_strength = WiFi.RSSI();
		RESERVE_STRING(data, LARGE_STR);
		data = FPSTR(data_first_part);
		RESERVE_STRING(result, MED_STR);

		if (cfg::ppd_read)
		{
			data += result_PPD;
			sum_send_time += sendSensorCommunity(result_PPD, PPD_API_PIN, FPSTR(SENSORS_PPD42NS), "PPD_");
		}

		if (cfg::sds_read)
		{
			data += result_SDS;
			sum_send_time += sendSensorCommunity(result_SDS, SDS_API_PIN, FPSTR(SENSORS_SDS011), "SDS_");
		}

		if (cfg::pms_read)
		{
			data += result_PMS;
			sum_send_time += sendSensorCommunity(result_PMS, PMS_API_PIN, FPSTR(SENSORS_PMSx003), "PMS_");
		}

		if (cfg::hpm_read)
		{
			data += result_HPM;
			sum_send_time += sendSensorCommunity(result_HPM, HPM_API_PIN, FPSTR(SENSORS_HPM), "HPM_");
		}
		if (cfg::npm_read)
		{
			data += result_NPM;
			sum_send_time += sendSensorCommunity(result_NPM, NPM_API_PIN, FPSTR(SENSORS_NPM), "NPM_");
		}

		if (cfg::ips_read)
		{
			data += result_IPS;
			sum_send_time += sendSensorCommunity(result_IPS, IPS_API_PIN, FPSTR(SENSORS_IPS), "IPS_");
		}

		if (cfg::sen5x_read && (!is_Sen5x_init_failed))
		{
			fetchSensorSEN5X(result); // edit and format sensor type/value for sending to webserver.

			data += result;

			if (memcmp(SEN5X_type, "SEN50", 6) == 0)
			{
				sum_send_time += sendSensorCommunity(result, SEN5X_PM_API_PIN, FPSTR(SENSORS_SEN50), "SEN50_");
			}

			if (memcmp(SEN5X_type, "SEN54", 6) == 0)
			{
				sum_send_time += sendSensorCommunity(result, SEN5X_PM_API_PIN, FPSTR(SENSORS_SEN54), "SEN54_");
			}

			if (memcmp(SEN5X_type, "SEN55", 6) == 0)
			{
				sum_send_time += sendSensorCommunity(result, SEN5X_PM_API_PIN, FPSTR(SENSORS_SEN55), "SEN55_");
			}

			result = emptyString;

			// Getting temperature and humidity.
			debug_outln_info(FPSTR(DBG_TXT_SEP));

			fetchSensorSEN5X_THN(result);

			int pin = memcmp(cfg::sen5x_sym_th, "SCD30", 6) == 0 ? SEN5X_SCD30_TH_API_PIN : SEN5X_SHT35_TH_API_PIN;

			data += result;
			sum_send_time += sendSensorCommunity(result, pin, FPSTR(SENSORS_SEN5X_TH), "SEN5X_");

			result = emptyString;

			debug_outln_info(FPSTR(DBG_TXT_SEP));
		}

		if (cfg::sps30_read && (!sps30_init_failed))
		{
			fetchSensorSPS30(result);

			data += result;
			sum_send_time += sendSensorCommunity(result, SPS30_API_PIN, FPSTR(SENSORS_SPS30), "SPS30_");
			result = emptyString;
		}

		if (cfg::dht_read)
		{
			// getting temperature and humidity (optional)
			fetchSensorDHT(result);

			data += result;
			sum_send_time += sendSensorCommunity(result, DHT_API_PIN, FPSTR(SENSORS_DHT22), "DHT_");
			result = emptyString;
		}

		if (cfg::htu21d_read && (!htu21d_init_failed))
		{
			// getting temperature and humidity (optional)
			fetchSensorHTU21D(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, HTU21D_API_PIN, FPSTR(SENSORS_HTU21D), "HTU21D_");
			result = emptyString;
		}

		if (cfg::bmp_read && (!bmp_init_failed))
		{
			// getting temperature and pressure (optional)
			fetchSensorBMP(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, BMP_API_PIN, FPSTR(SENSORS_BMP180), "BMP_");
			result = emptyString;
		}

		if (cfg::bmx280_read && (!bmx280_init_failed))
		{
			// getting temperature, humidity and pressure (optional)
			fetchSensorBMX280(result);

			data += result;
			if (bmx280.sensorID() == BME280_SENSOR_ID)
			{
				sum_send_time += sendSensorCommunity(result, BME280_API_PIN, FPSTR(SENSORS_BME280), "BME280_");
			}
			else
			{
				sum_send_time += sendSensorCommunity(result, BMP280_API_PIN, FPSTR(SENSORS_BMP280), "BMP280_");
			}
			
			result = emptyString;
		}

		if (cfg::sht3x_read && (!sht3x_init_failed))
		{
			// getting temperature and humidity (optional)
			fetchSensorSHT3x(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, SHT3X_API_PIN, FPSTR(SENSORS_SHT3X), "SHT3X_");
			result = emptyString;
		}

		if (cfg::scd30_read && (!scd30_init_failed))
		{
			if( scd30.isConnected() )
			{// getting temperature and humidity (optional)
				fetchSensorSCD30(result);

				data += result;
				sum_send_time += sendSensorCommunity(result, SCD30_API_PIN, FPSTR(SENSORS_SCD30), "SCD30_");
				result = emptyString;
			}
		}

		if (cfg::ds18b20_read)
		{
			// getting temperature (optional)
			fetchSensorDS18B20(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, DS18B20_API_PIN, FPSTR(SENSORS_DS18B20), "DS18B20_");
			result = emptyString;
		}

		if (cfg::dnms_read && (!dnms_init_failed))
		{
			// getting noise measurement values from dnms (optional)
			fetchSensorDNMS(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, DNMS_API_PIN, FPSTR(SENSORS_DNMS), "DNMS_");
			result = emptyString;
		}

		if (cfg::gps_read)
		{
			data += result_GPS;
			sum_send_time += sendSensorCommunity(result_GPS, GPS_API_PIN, F("GPS"), "GPS_");
			result = emptyString;
		}

		add_Value2Json(data, F("samples"), String(sample_count));
		add_Value2Json(data, F("min_micro"), String(min_micro));
		add_Value2Json(data, F("max_micro"), String(max_micro));
		add_Value2Json(data, F("interval"), String(cfg::sending_intervall_ms));
		add_Value2Json(data, F("signal"), String(last_signal_strength));			// how strong wifi signal in %

		if ((unsigned)(data.lastIndexOf(',') + 1) == data.length())
		{
			data.remove(data.length() - 1);
		}

		data += "]}";						// set JSON end chars.

		debug_outln_info(FPSTR(DBG_TXT_SEP));
		Debug.println(data);				// print complete Json data string.

		yield();							// give waiting threads CPU time.

		// send to:
		sum_send_time += sendDataToOptionalApis(data);
		
		debug_outln_info(FPSTR(DBG_TXT_SEP));

#if defined(ESP8266)
		if (cfg::send2mqtt && mqtt_client.connected())
		{
			mqtt_client.loop();
		}
#endif

		// https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
		sending_time = (3 * sending_time + sum_send_time) / 4;
		if (sum_send_time > 0)
		{
			debug_outln_info(F("Time for Sending (ms): "), String(sending_time));
		}

		// reconnect to WiFi if disconnected
		if (WiFi.status() != WL_CONNECTED)
		{
			debug_outln_info(F("Connection lost, reconnecting "));

			WiFi_error_count++;

			if (cfg::has_fix_ip)
			{
				WiFi.reconnect();
				waitForWifiToConnect(20);
			}
			else
			{
				waitForMultiWiFiToConnect(20, WIFI_SCAN_TIMEOUT_MS);
			}
		}

		// only do a restart after finishing sending
		if (msSince(time_point_device_start_ms) > DURATION_BEFORE_FORCED_RESTART_MS)
		{
			sensor_restart();
		}

		// time for a OTA attempt?
		if (msSince(last_update_attempt) > PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS)
		{
			twoStageOTAUpdate();
			last_update_attempt = act_milli;
		}

		// Resetting for next sampling.
		last_data_string = std::move(data);
		lowpulseoccupancyP1 = 0;
		lowpulseoccupancyP2 = 0;
		sample_count = 0;
		last_micro = 0;
		min_micro = 1000000000;
		max_micro = 0;
		sum_send_time = 0;
		starttime = millis(); 					// store the start time.
		count_sends++;
	}

#if defined(ESP8266)
	MDNS.update();
	if (cfg::npm_read)
	{
		serialNPM.perform_work();
	}
	else
	{
		serialSDS.perform_work();
	}

	if (cfg::has_radarmotion && sntp_time_set > 0)
	{
		if( cfg::send2mqtt && !mqtt_client.connected())
		{// after x time MQTT connection will be lost wifi connection.... but why ????
			debug_outln_info(F("** RCWL0516 => MQTT Broker connection lost.\nRetry......"));
			//debug_outln_info(F("MQTT Broker connecting failed, state = "), String(mqtt_client.state()));

			setup_mqtt_broker( cfg::mqtt_server, cfg::mqtt_port);
		}

		RCWL0516.loop();
	}
#endif

	// Sleep if all of the tasks have an event in the future. The chip can then
	// enter a lower power mode.
	if (cfg::powersave)
	{
		delay(sleep);
	}

	if (sample_count % 500 == 0)
	{
		//		Serial.println(ESP.getFreeHeap(),DEC);
	}
} // end loop
