/*
 *  BK_Common.h -- platform definitions.
 */

#ifndef BK_COMMON_H
#define BK_COMMON_H

#if ESP8266 && (ARDUINO >= 100)
    // VS: Convert Arduino file to C++ manually.
    #include <Arduino.h>
#else
  #include "WProgram.h"
  #include <NewSoftSerial.h>
#endif

#if (defined(__AVR__))
  #include <avr/pgmspace.h>
#elif (defined(ESP8266))
	#include <pgmspace.h>
#endif

#ifdef BK_MODEM_DEBUG

// #define DEBUG_ESP_DNS
//#define BK_MODEM_HEXDEBUG                                               // print hex value

// need to do some debugging...
  #define BK_DEBUG_PRINT(...)		DebugStream->print(__VA_ARGS__)
  #define BK_DEBUG_PRINTLN(...)	    DebugStream->println(__VA_ARGS__)
#else
// debug is disabled
  #define BK_DEBUG_PRINT(...)
  #define BK_DEBUG_PRINTLN(...)
#endif

// a few typedefs to keep things portable
typedef	Stream 	                                BK_SIM7000_StreamType;
typedef const   __FlashStringHelper             *FStringPtr;

#define prog_char  	                            char PROGMEM

#define prog_char_strcmp(a, b)					strcmp_P((a), (b))
#define prog_char_strncmp(a, b, c)		        strncmp_P((a), (b), (c))
#define prog_char_strstr(a, b)					strstr_P((a), (b))
#define prog_char_strlen(a)						strlen_P((a))
#define prog_char_strcpy(to, fromprogmem)		strcpy_P((to), (fromprogmem))
//define prog_char_strncpy(to, from, len)		strncpy_P((to), (fromprogmem), (len))
#define prog_char_strcat(to, fromprogmem)       strcat_P((to), (fromprogmem))

#endif /* BK_COMMON_H */

