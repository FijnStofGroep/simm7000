 /*
  *
  *  File:	OTARoot.h
  *
  *  Author: Roel Dieperink
  * 
  *	 Copyright: © 2024 ~ 2026	Rolenco Leusden
  *  Created on: 02 aug, 2026
  * 
  * 
  */


#ifndef _OTARBOOT_H_
#define _OTARBOOT_H_

#pragma region "OTA Definitions/Prototypes => Loading new firmware"

#include <Arduino.h>

// prototype:

#pragma region "OTA HTML page"
// ===== HTML page =====
constexpr const char *UPLOADPAGE = R"(
                                      <!DOCTYPE html>
                                      <html>
                                      <head>
                                          <meta name="viewport" content="width=device-width, initial-scale=1">
                                      </head>
                                      <body>
                                          <h2>{{SENSOR_NAME}}</h2>
                                          <form method='POST' action='/OTA_Update' enctype='multipart/form-data'>
                                              <input type='file' name='update'>
                                              <input type='submit' value='Upload'>
                                          </form>
                                          <br/><br/>
                                          <form method='POST' action='/'>
                                            <input type=submit value='Back to home page' {color:#fff;text-align:left;cursor:pointer;border-radius:5px;font-size:medium;background:#b33;padding:9px!important;width:100%;border-style:none} >
                                          </form>
                                      </body>
                                      </html>
                                      )";

#pragma endregion


/*
  ===== OTA Root SETUP =====
*/
void Set_OTA_UpdateHandlers(void);
void Send_OTA_RootPage(void);

void Print_ESP_OTA_Settings(void);

/*
  ===== LOOP =====
  Loop function runs over and over again forever.
*/
void OTA_UpdateLoop(void);

#pragma endregion

#endif