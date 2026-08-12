/*
 *
 *  File:	OTALocal.cpp
 *
 *
 * ESP8266 OTA Firmware Update via AP Mode + Browser
 * ===================================================
 * De ESP8266 start als Access Point.
 * Verbind met het WiFi netwerk "ESP8266-OTA" (wachtwoord: "12345678")
 * Ga naar http://192.168.4.1 in de browser om de firmware te uploaden.
 *
 *  Author: Roel Dieperink
 *
 *	 Copyright: © 2024 ~ 2026	Rolenco Leusden
 *  Created on: 02 aug, 2026
 *
 *
 * 2026-08-02
 *
 *  - rboot_get_current_rom();
 *     Get the currently selected boot ROM (this will be the currently running ROM, as long as you haven't changed
 *     it since boot or rBoot booted the rom in temporary boot mode.
 *
 *  - rboot_config:
 *     ROM addresses must be multiples of 0x1000 (flash sector aligned).
 *     Without BOOT_BIG_FLASH only the first 8Mbit (1MB) of the chip will be memory mapped so ROM slots containing.
 *		irom0.text sections must remain below 0x100000.
 *     Slots beyond this will only be accessible via spi read calls, so use these for stored resources, not code.
 *     With BOOT_BIG_FLASH the flash will be mapped in chunks of 8MBit (1MB), so ROMs can be anywhere, but must not
 *     straddle two 8MBit (1MB) blocks.
 *
 */

#include "OTALocal.h"

#include <ESP8266WebServer.h>
#include <DNSServer.h>

// #include <Updater.h>      // arduino../cores/esp8266/

// #include <rboot-api.h>
// #include <rboot-ota.h>

#include "defines.h"
#include "ext_def.h"
#include "utils.h"

#pragma region "OTA Definitions/Prototypes"

// ===== PROTOTYPES =====
const char TXT_CONTENT_TYPE_TEXT_HTML[] PROGMEM = "text/html; charset=utf-8";

// ===== CONFIG =====
#define UPDATE_SIZE_UNKNOWN -1

// const byte DNS_PORT = 53; // moved to ext_def.h

// ===== extern GLOBALS =====
extern volatile int flg_OTAStartbyWebCall;
extern ESP8266WebServer server;
extern DNSServer dnsServer;

/// @brief
namespace cfg
{
  extern char fs_ssid[LEN_FS_SSID];
}

#pragma endregion

#pragma region "Server call handlings"
/*
  ===== WEB ROUTES =====
*/
void Set_OTA_UpdateHandlers(void)
{
  debug_outln_verbose("*** Set_OTA_UpdateHandlers( START ). ***\r\n");

  // OTA update endpoint
  server.on(F("/OTA_Update"), HTTP_POST,
            []() { // finisch
              debug_outln_verbose(F("Finisch => Browser State: "), Update.hasError() ? "ERROR FAIL" : "OK");

              if (Update.hasError())
              {
                int8_t error = Update.getError();

                debug_outln_verbose(F("[_OTA_] Update error code: "), String(error));
                Update.printError(Debug);

                // A single HTTP response can only have one status code. You cannot send: 500 and later 303. 
                // The first status code sent will be the one that the client sees. The second status code will be ignored.
                //server.send(500, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), "<h2>*** OTA Update FAILED. ***</h2>");

                // if( error == 13)
                // { // No firmware data. (no file selected)
                //     flg_OTAStartbyWebCall = 0; // Reset OTA flag after update is complete
                // }

                //delay(2000);

                //server.send(200, "text/html", "<script>window.location.href = '/';</script>");
                //server.send(200, "text/html",
                //            "<html><body>Error. Redirecting...<script>setTimeout(()=>location.href='/', 2000);</script></body></html>");

                // Redirect back to the main page, works
                server.sendHeader("Location", "/");
                server.send(303, "text/plain", "Redirecting...");
              }
              else
              {
                // server.send(200, "text/plain", Update.hasError() ? "FAIL" : "OK");
                server.send(200, "text/plain", "Update OK. Rebooting...");
                delay(1000);

                debug_outln_verbose(F("ESP.restart()"), F(" after OTA update..."));
                // delay(3000);

                ESP.restart();
              }
            },
            []() { // File downloading process.
              HTTPUpload &httpUpload = server.upload();

              if (httpUpload.status == UPLOAD_FILE_START)
              {
                debug_outln_verbose(F("Start Upload Filename: "), String(httpUpload.filename.c_str()));

                if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                {
                  Update.printError(Debug);
                }
              }
              else if (httpUpload.status == UPLOAD_FILE_WRITE)
              {
                debug_outln_verbose(F("httpUpload.status: write: "), String(httpUpload.currentSize) + F("bytes."));

                if (Update.write(httpUpload.buf, httpUpload.currentSize) != httpUpload.currentSize)
                {
                  Update.printError(Debug);
                }
              }
              else if (httpUpload.status == UPLOAD_FILE_END)
              {
                if (Update.end(true))
                {
                  debug_outln_verbose(F("Success: "), String(httpUpload.totalSize) + F("bytes."));
                }
                else
                {
                  Update.printError(Debug); // Print (LoggingSerial instance) error if update failed
                }
              }
            });

  debug_outln_verbose(F("*** Set_OTA_UpdateHandlers( ENDED ). ***"));
}

/*
    server.on("/save", HTTP_POST, handleSave);


    return 303 redirect to /error
          create page /error shows the message.
          server.on("/error", HTTP_POST, handleError);
*/
void handleSave( void )
{
  bool badInput = false;

    String name = server.arg("name");   // read submitted form data
    debug_outln_verbose("Received: " + name);

  if (badInput) 
  {
    server.sendHeader("Location", "/error");
    server.send(303, "text/plain", "See error page");
    return;
  }

  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "Redirecting...");
}


/// @brief
/// @param
void Send_OTA_RootPage(void)
{
  RESERVE_STRING(wrkuploadPage, LARGE_STR);
  wrkuploadPage = FPSTR(UPLOADPAGE);
  wrkuploadPage.replace(F("{{SENSOR_NAME}}"), String(cfg::fs_ssid));

  // Send the HTML page for OTA update
  server.send(200, "text/html", wrkuploadPage);
}

/*
  ===== OTA Root SETTINGS =====
*/
void Print_ESP_OTA_Settings(void)
{
  // Get flash chip ID.
  uint32_t chipId = ESP.getFlashChipId();
  debug_outln_verbose(F("Flash Chip ID: ") + String(chipId, HEX) + F(" => ") + String(chipId) + F("\n"));

  // Get flash chip size (in bytes)
  uint32_t flashSize = ESP.getFlashChipRealSize();
  debug_outln_verbose( StringFormat( F("Flash Chip Size: %u bytes (%.2f MB)\r\n"), flashSize, ((flashSize / 1024.0) / 1024.0)));
  //Debug.printf( "Flash Chip Size: %u bytes (%.2f MB)\r\n", flashSize, ((flashSize / 1024.0) / 1024.0));

  // Get flash chip speed
  uint32_t flashSpeed = ESP.getFlashChipSpeed();
  debug_outln_verbose( StringFormat( F("Flash Chip Speed: %u Hz (%.2f MHz)\n"), flashSpeed, flashSpeed / 1000000.0));

  // Sketch size and free space
  debug_outln_verbose( StringFormat( F("Sketch Size: %u bytes.\nFree Sketch Space: %u bytes\n"), ESP.getSketchSize(), ESP.getFreeSketchSpace()));

  // Flash mode
  FlashMode_t mode = ESP.getFlashChipMode();
  debug_outln_verbose( StringFormat( F("Flash Mode: %s\n"), (mode == FM_QIO)  ? "QIO" :
                                                            (mode == FM_QOUT) ? "QOUT" :
                                                            (mode == FM_DIO)  ? "DIO" :
                                                            (mode == FM_DOUT) ? "DOUT" : "Unknown"));

  debug_outln_verbose(F("Note: The 'flash address' for firmware upload usually starts at 0x00000."));

  // rboot_config conf;
  // conf = rboot_get_config();

  // debug_outln_verbose( StringFormat(F("*** Selected Boot ROM = %d ***\r\n"), conf.current_rom));
  // debug_outln_verbose( StringFormat(F("*** current version = %d ***\r\n"), conf.version));
  // debug_outln_verbose( StringFormat(F("*** Flash addresses of each ROM 1 = %x ***\r\n"), conf.roms[0]));
  // debug_outln_verbose( StringFormat(F("*** Flash addresses of each ROM 2 = %x ***\r\n"), conf.roms[1]));
  // debug_outln_verbose( StringFormat(F("*** Flash addresses of each ROM 3 = %x ***\r\n"), conf.roms[2]));
  // debug_outln_verbose( StringFormat(F("*** Flash addresses of each ROM 4 = %x ***\r\n"), conf.roms[3]));
  // debug_outln_verbose( StringFormat(F("*** current Flash mode = %d ***\r\n"), conf.mode));
  // debug_outln_verbose( F("***  ***\r\n"));
}

/*
  ===== LOOP =====
  Loop function runs over and over again forever.
*/
void OTA_UpdateLoop(void)
{
  dnsServer.processNextRequest();
  server.handleClient();

  debug_outln_info(F("ws: Firmware OTA_UpdateLoop running..."));

  delay(1000); // Small delay to avoid overwhelming the CPU and wait a bit to let the OTA process do his job.
}

#pragma endregion
