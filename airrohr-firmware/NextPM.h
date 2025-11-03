/*
 * 	File:	NextPM.h
 *
 *	Copyright  : © 2024 ~ 2025,	Rolenco Leusden
 *  Created on : 14 nov, 2024
 *      Author : Roel Dieperink
 */

#ifndef NEXTPM_H_
#define NEXTPM_H_

#include <WString.h>
#include <SoftwareSerial.h>

#include "./intl.h" 			// define languages
#include "./utils.h"
#include "./defines.h"
#include "./ext_def.h"

#define NEXTPM_BAUD 115200
#define NEXT_PM_COMMAND_DELAY 500


/// @brief : define extern enum type.
enum class PmSensorCmd2;

/// @brief 
enum NPM_HEATER_MODE
{
	NONE = 0,
	OFF = 1,
	ON = 2,
	AUTO_REGULATED = 3,
	HEATING_CONTROL = 4,		// if hum > 65%RH then Heater ON, hum < 60%RH Heater OFF.
	
	Count 						// always last => constant such as `Count` the number of enum entries. 
};

//--------------------------------------------------------------------------------------------------------
//extern definitions.


//--------------------------------------------------------------------------------------------------------

/// @brief 
/// @author: R.Dieperink

class NextPM
{

public:
	NextPM(SoftwareSerial &serial);
	virtual ~NextPM();

	void begin( float humidity_threshold = 65.0);
	void end();
	void perform_work();

	bool Get_State(uint8_t *status);
	bool Start_stop(uint8_t *status);
	String Firmware_version();
	// void 	Fan_speed();

	uint Set_Heater_Mode(NPM_HEATER_MODE heaterMode);
	void Heating_Control(float hum_value);
	void Display_State_Error(uint8_t test_state);
	String Get_Last_Device_State();

	bool ReadMeasuredPmValues(uint16_t *pm1, uint16_t *pm25, uint16_t *pm10,
							  uint16_t *pm1_pcs, uint16_t *pm25_pcs, uint16_t *pm10_pcs);

	bool ReadMeasuredTmp_HumValues(uint16_t *temp, uint16_t *humi);


private:
	SoftwareSerial *hstream = NULL; // communication port to Tera NextPM sensor hardware.

	String current_state_npm;

	bool Parser_StateValue(uint8_t *status);
	void Serial_Flush(void);
	void Send_Cmd(PmSensorCmd2 cmd);
	bool Checksum_Valid(const uint8_t *data, uint8_t len);
	uint8_t Calculate_Checksum(const uint8_t *data, uint8_t len);
	void Log_data_reader(const uint8_t data[], size_t size, bool receiveMode = true);
	String Display_State_Value(uint8_t bytedata);

};	// end class NextPM

#endif /* NEXTPM_H_ */
