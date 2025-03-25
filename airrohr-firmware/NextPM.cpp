/*
 * 	File:	NextPM.cpp
 *
 *	Copyright  : © 2024 ~ 2025	Rolenco Leusden
 *  Created on : 14 nov, 2024
 *      Author : Roel Dieperink
 */

#include <NextPM.h>


// To read NPM responses
enum NPM_WAITING_16
{
    NPM_REPLY_HEADER_16 = 16,
    NPM_REPLY_STATE_16 = 14,
    NPM_REPLY_BODY_16 = 13,
    NPM_REPLY_CHECKSUM_16 = 1
} NPM_waiting_for_16;           // for concentration

enum NPM_WAITING_4
{
    NPM_REPLY_HEADER_4 = 4,
    NPM_REPLY_STATE_4 = 2,
    NPM_REPLY_CHECKSUM_4 = 1
} NPM_waiting_for_4;            // for change

enum NPM_WAITING_5
{
    NPM_REPLY_HEADER_5 = 5,
    NPM_REPLY_STATE_5 = 3,
    NPM_REPLY_DATA_5 = 2,
    NPM_REPLY_CHECKSUM_5 = 1
} NPM_waiting_for_5;            // for fan speed

enum NPM_WAITING_6
{
    NPM_REPLY_HEADER_6 = 6,
    NPM_REPLY_STATE_6 = 4,
    NPM_REPLY_DATA_6 = 3,
    NPM_REPLY_CHECKSUM_6 = 1
} NPM_waiting_for_6;            // for version

enum NPM_WAITING_8
{
    NPM_REPLY_HEADER_8 = 8,
    NPM_REPLY_STATE_8 = 6,
    NPM_REPLY_BODY_8 = 5,
    NPM_REPLY_CHECKSUM_8 = 1
} NPM_waiting_for_8;            // for temperature/humidity

// NPM_WAITING_8 NPM_waiting_for_8;

/// @brief
/// @tparam T
/// @tparam N
/// @param
/// @return
template <typename T, std::size_t N>
constexpr std::size_t array_num_elements(const T (&)[N])
{
    return N;
}

/// @brief : consructor
/// @param : pointer to SoftwareSerial serial_instance.
NextPM::NextPM(SoftwareSerial &serial)
{
    this->hstream = &serial;
}

/// @brief : destructor
NextPM::~NextPM()
{
    // TODO Auto-generated destructor stub.
    this->hstream->end();
    this->hstream = NULL;
}

/*****************************************************************
 * read Next PM sensor serial and firmware date                  *
 *****************************************************************/
/// @brief : Init communication port with TERA NextPM sensor hardware.
void NextPM::begin()
{
    debug_outln_verbose(F("NPM configured...."));

#if defined(ESP8266)
    hstream->begin(NEXTPM_BAUD, SWSERIAL_8E1, PM_SERIAL_RX, PM_SERIAL_TX);
    hstream->enableIntTx(false);
#endif

#if defined(ESP32)
    hstream->begin(NEXTPM_BAUD, SERIAL_8E1, PM_SERIAL_RX, PM_SERIAL_TX);
#endif

    Debug.println(F("SetUp MCU Serial ComPort --> NextPM... 115200, 8E1"));
    hstream->setTimeout(400);
}

/// @brief : Stop serial process.
void NextPM::end()
{
    hstream->end();
}

/// @brief Run the internal processing and event engine.
///        Can be iteratively called from loop, or otherwise scheduled.
void NextPM::perform_work()
{
    hstream->perform_work();
}

/// @brief 
/// @return 
String NextPM::Get_Last_Device_State()
{
    return current_state_npm;
}

/*****************************************************************
 * read Tera Next PM sensor serial and firmware date             *
 *****************************************************************/
/// @brief
/// @param ptr to status memory.
/// @return
bool NextPM::Get_State(uint8_t *status)
{
    debug_outln_verbose(F("Get NPM State..."));

    uint8_t chrlen = 0;
    int reply = 5;

    Serial_Flush();

    Send_Cmd(PmSensorCmd2::State);

    while (!(chrlen = hstream->available()))
    { // wait till receive response from NextPM sensor.
        debug_outln("Wait for NPM State Response...", DEBUG_MAX_INFO);

        if (--reply == 0)
        {
            *status = 0b00100110;
            return false;
        }

        delay(NEXT_PM_COMMAND_DELAY);
    }

    debug_outln_verbose(F("NPM available chars: ") + String(chrlen, HEX));

    *status = 0b00000100;
    return Parser_StateValue(status);
}

/// @brief : Parse State Value
///     State code:
///         | Bit 7 | Bit 6  | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1    | Bit 0 |
///         | Laser | Memory | Fan   | T/RH  | Heat  |  Not  | Degraded | Sleep |
///         | Error | Error  | Error | Error | Error | Ready |  State   | State |
/// Remark:
///     The bit 0 is set to 1 when the sensor is set to sleep state: the laser, the fan and the heat are
///     switched off.
///
///     The bit 1 is set to 1 each time a minor error is detected, the sensor part (bit3 till bit7) in error
///     is set to 1 in the state code, the NextPM can still send data but with less accuracy.
///
/// @param *status =s NPM tate value.
/// @return : true = Okay, false = Not Okay.
bool NextPM::Parser_StateValue(uint8_t *status)
{
    bool result = false;
    uint8_t state[1];
    uint8_t checksum[1];
    uint8_t test[4];

    NPM_waiting_for_4 = NPM_REPLY_HEADER_4;
    const uint8_t constexpr header[2] = {0x81, 0x16};

    while (hstream->available() >= NPM_waiting_for_4) // get cnt how many bytes still in receive buffer.
    {
        switch (NPM_waiting_for_4)
        {
        case NPM_REPLY_HEADER_4:
            if (hstream->find(header, sizeof(header))) // Get header ID out receive buffer.
            {
                NPM_waiting_for_4 = NPM_REPLY_STATE_4;
            }

            break;

        case NPM_REPLY_STATE_4:
            hstream->readBytes(state, sizeof(state)); // read 1 byte (state) from receive stream.
            Display_State_Value(state[0]);
            *status = state[0];

            NPM_waiting_for_4 = NPM_REPLY_CHECKSUM_4;
            break;

        case NPM_REPLY_CHECKSUM_4:
            hstream->readBytes(checksum, sizeof(checksum)); // read 1 byte (CRC) from receive stream.

            memcpy(test, header, sizeof(header));
            memcpy(&test[sizeof(header)], state, sizeof(state));
            memcpy(&test[sizeof(header) + sizeof(state)], checksum, sizeof(checksum));

            if (Checksum_Valid(test, 4))
            {
                result = true;
            }
            else
            {
                debug_outln_verbose(F("NPM Checksum NOT OK..."));
            }

            Log_data_reader(test, 4);

            NPM_waiting_for_4 = NPM_REPLY_HEADER_4;
            break;
        }
    }

    return result;
}

/// @brief
/// Response:
///         Address | Cmd code | State | Checksum
///          0x81       0x15      0x00     0x38
/// @param status
/// @return
bool NextPM::Start_stop(uint8_t *status)
{
    debug_outln_info(F("Switch start/stop NPM..."));

    bool result = false;
    int reply = 5;

    Serial_Flush();

    Send_Cmd(PmSensorCmd2::Change);

    while (!hstream->available())
    { // wait till receive response from Tera sensor.
        debug_outln(F("Wait for NPM Start/Stop Response..."), DEBUG_MAX_INFO);

        if (--reply == 0)
        {
            return false;
        }

        delay(NEXT_PM_COMMAND_DELAY);
    }

    NPM_waiting_for_4 = NPM_REPLY_HEADER_4;
    const uint8_t constexpr header[2] = {0x81, 0x15};

    uint8_t state[1];
    uint8_t checksum[1];
    uint8_t test[4];
    bool stop = false;
    *status = 0b00000100;

    // Read: 0x81, 0x15, 0x26, 0x44
    while (!stop && hstream->available() >= NPM_waiting_for_4)
    {
        switch (NPM_waiting_for_4)
        {
        case NPM_REPLY_HEADER_4:
            if (hstream->find(header, sizeof(header)))
            {
                NPM_waiting_for_4 = NPM_REPLY_STATE_4;
            }
            break;

        case NPM_REPLY_STATE_4:
            hstream->readBytes(state, sizeof(state));
            Display_State_Value(state[0]);
            *status = state[0];

            if (bitRead(state[0], 1) == 1)
            {
                debug_outln_info(F("NPM stoped, there is a minor Error detected..."));
                Display_State_Error(state[0]);

                hstream->readBytes(checksum, sizeof(checksum));
                memcpy(test, header, sizeof(header));
                memcpy(&test[sizeof(header)], state, sizeof(state));
                memcpy(&test[sizeof(header) + sizeof(state)], checksum, sizeof(checksum));
                Log_data_reader(test, 4);

                result = false;
                stop = true;
            }
            else
            {
                if (bitRead(state[0], 0) == 0)
                { // NextPM will be switched on and will send the first PM datas after 15 seconds
                    debug_outln_info(F("NPM sensor will be switched on..."));
                    result = true;
                }
                else if (bitRead(state[0], 0) == 1)
                {
                    debug_outln_info(F("NPM sensor in sleep mode..."));
                    result = false;
                }
                else
                {
                    result = false;
                }
            }

            NPM_waiting_for_4 = NPM_REPLY_CHECKSUM_4;
            break;

        case NPM_REPLY_CHECKSUM_4:
            hstream->readBytes(checksum, sizeof(checksum));

            memcpy(test, header, sizeof(header));
            memcpy(&test[sizeof(header)], state, sizeof(state));
            memcpy(&test[sizeof(header) + sizeof(state)], checksum, sizeof(checksum));

            if (Checksum_Valid(test, sizeof(test)))
            {
                result = true;
            }
            else
            {
                debug_outln_verbose(F("NPM Checksum NOT OK..."));
            }

            Log_data_reader(test, 4);

            NPM_waiting_for_4 = NPM_REPLY_HEADER_4;
            break;
        }
    }

    return result; // ATTENTION
}

/// @brief
/// Response:
///         Address | Cmd code | State | Firmware version | Checksum
///          0x81       0x17      0x00         0x0134         0x38
/// @return
String NextPM::Firmware_version()
{
    // debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(DBG_TXT_NPM_VERSION_DATE));
    debug_outln_info(F("Get NPM Version..."));

    int reply = 5;

    Serial_Flush();

    NPM_waiting_for_6 = NPM_REPLY_HEADER_6;
    Send_Cmd(PmSensorCmd2::Version);

    while (!hstream->available())
    { // wait till receive response from Tera sensor.
        debug_outln(F("Wait for NPM Version Response..."), DEBUG_MAX_INFO);

        if (--reply == 0)
        {
            return F("NPM not connected..");
        }

        delay(NEXT_PM_COMMAND_DELAY);
    }

    String NPM_version;
    const uint8_t constexpr header[2] = {0x81, 0x17};
    uint8_t state[1];
    uint8_t data[2];
    uint8_t checksum[1];
    uint8_t test[6];

    while (hstream->available() >= NPM_waiting_for_6)
    {
        switch (NPM_waiting_for_6)
        {
        case NPM_REPLY_HEADER_6:
            if (hstream->find(header, sizeof(header)))
            {
                NPM_waiting_for_6 = NPM_REPLY_STATE_6;
            }
            break;

        case NPM_REPLY_STATE_6:
            hstream->readBytes(state, sizeof(state));
            Display_State_Value(state[0]);

            NPM_waiting_for_6 = NPM_REPLY_DATA_6;
            break;

        case NPM_REPLY_DATA_6:
            if (hstream->readBytes(data, sizeof(data)) == sizeof(data))
            {
#if defined(VS_DEBUG)
                Log_data_reader(data, 2);
#endif
                char tmp[6];
                snprintf_P(tmp, sizeof(tmp), PSTR("%01x.%01x%02x)"), (data[0] >> 4), (data[0] & 0x0f), data[1]);
                NPM_version = String(tmp);

                // uint16_t NPMversion = word(data[0], data[1]);
                // NPM_version = String(NPMversion);              // decimal notation.
                // NPM_version = String(NPMversion, HEX);           // hex notation.
                // debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(DBG_TXT_NPM_VERSION_DATE));
                // debug_outln_info(F("Next PM Firmware: "), last_value_NPM_version);
            }

            NPM_waiting_for_6 = NPM_REPLY_CHECKSUM_6;
            break;

        case NPM_REPLY_CHECKSUM_6:
            hstream->readBytes(checksum, sizeof(checksum));

            memcpy(test, header, sizeof(header));
            memcpy(&test[sizeof(header)], state, sizeof(state));
            memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
            memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));

            if (!Checksum_Valid(test, sizeof(test)))
            {
                NPM_version = F("x.xxx");
                debug_outln_verbose(F("NPM Checksum NOT OK..."));
            }

            Log_data_reader(test, 6);

            NPM_waiting_for_6 = NPM_REPLY_HEADER_6;
            break;
        }
    }

    return NPM_version;
}

// #pragma GCC diagnostic ignored "-Wunused-function"

/// @brief : Skip from V1.020
// void NextPM::Fan_speed()
// {
//     debug_outln_info(F("Set fan speed to 50 %..."));

// 	NPM_waiting_for_5 = NPM_REPLY_HEADER_5;
// 	NPM_sendCmd(PmSensorCmd2::Speed);

// 	while (!serialNPM.available())
// 	{// wait till receive response from Tera sensor.
//         debug_outln("Wait for NPM-Serial...", DEBUG_MAX_INFO);
//  }

//     const uint8_t constexpr header[2] = {0x81, 0x21};
//     uint8_t state[1];
//     uint8_t data[1];
//     uint8_t checksum[1];
//     uint8_t test[5];

//     while (serialNPM.available() >= NPM_waiting_for_5)
//     {
//         switch (NPM_waiting_for_5)
// 		{
// 		case NPM_REPLY_HEADER_5:
// 			if (serialNPM.find(header, sizeof(header)))
// 				NPM_waiting_for_5 = NPM_REPLY_STATE_5;
// 			break;

// 		case NPM_REPLY_STATE_5:
// 			serialNPM.readBytes(state, sizeof(state));
// 			get_NPM_State(state[0]);
// 			NPM_waiting_for_5 = NPM_REPLY_DATA_5;
// 			break;

// 		case NPM_REPLY_DATA_5:
// 			if (serialNPM.readBytes(data, sizeof(data)) == sizeof(data))
// 			{
// 				NPM_data_reader(data, 1);
// 			}
// 			NPM_waiting_for_5 = NPM_REPLY_CHECKSUM_5;
// 			break;

// 		case NPM_REPLY_CHECKSUM_5:
// 			serialNPM.readBytes(checksum, sizeof(checksum));
// 			memcpy(test, header, sizeof(header));
// 			memcpy(&test[sizeof(header)], state, sizeof(state));
// 			memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
// 			memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));
// 			NPM_data_reader(test, 5);

// 			NPM_waiting_for_5 = NPM_REPLY_HEADER_5;
// 			if (!NPM_checksum_valid(test,5))
// 			{
// 				debug_outln_info(F("Checksum NOT OK..."));
// 			}
// 			break;
// 		}
//     }
// }

// #pragma GCC diagnostic pop

/*****************************************************************
 * read Tera Next PM-Sensor get sensor values                    *
 *****************************************************************/
/// @brief
/// @param ptr to pm1       particulate matter concentration in µg/m3
/// @param pm25[]
/// @param pm10
/// @param ptr to pm1_pcs   particulate matter concentration in  pcs/L
/// @param pm25_pcs
/// @param pm10_pcs
/// @return
bool NextPM::ReadMeasuredPmValues(uint16_t *pm1, uint16_t *pm25, uint16_t *pm10,
                                  uint16_t *pm1_pcs, uint16_t *pm25_pcs, uint16_t *pm10_pcs)
{
    bool result = false;
    int reply = 5;

    uint8_t state[1];
    uint8_t data[12];
    uint8_t checksum[1];
    uint8_t test[16];

    uint16_t n1_serial = 0;
    uint16_t n25_serial = 0;
    uint16_t n10_serial = 0;
    uint16_t pm1_serial = 0;
    uint16_t pm25_serial = 0;
    uint16_t pm10_serial = 0;

    uint8_t test_state;

    /*
        The state code must always be read, it highlights the functional state of the NextPM and allows to
        know the validity of the sent PM values.
    */
    if (!Get_State(&test_state))
    {
        debug_outln_verbose(F("PM read ERROR, State => Time-Out."));
        return false;
    }
    else
    {
        if (test_state != 0x00)
        { // if bit2 set then NextPM not ready.
            // after 18hours a error
            debug_outln_verbose(F("PM read ERROR, Current State: "), String(test_state));
            return false;
        }
    }

    NPM_waiting_for_16 = NPM_REPLY_HEADER_16;
    const uint8_t constexpr header[2] = {0x81, 0x11};

    Send_Cmd(PmSensorCmd2::Concentration);

    while (!hstream->available())
    {
        debug_outln(F("Wait for NPM \"PM\" Response..."), DEBUG_MAX_INFO);

        if (--reply == 0)
        {
            return false;
        }

        delay(NEXT_PM_COMMAND_DELAY);
    }

    while (hstream->available() >= NPM_waiting_for_16)
    {
        switch (NPM_waiting_for_16)
        {
        case NPM_REPLY_HEADER_16:
            if (hstream->find(header, sizeof(header)))
            {
                NPM_waiting_for_16 = NPM_REPLY_STATE_16;
            }

            break;

        case NPM_REPLY_STATE_16:
            hstream->readBytes(state, sizeof(state)); // read state byte out receive buiffer
            current_state_npm = Display_State_Value(state[0]);
            NPM_waiting_for_16 = NPM_REPLY_BODY_16;
            break;

        case NPM_REPLY_BODY_16:
            if (hstream->readBytes(data, sizeof(data)) == sizeof(data))
            {
#if defined(VS_DEBUG)
                Log_data_reader(data, 12);
#endif
                // in µg/m3
                n1_serial = word(data[0], data[1]);
                n25_serial = word(data[2], data[3]);
                n10_serial = word(data[4], data[5]);

                // in pcs/L
                pm1_serial = word(data[6], data[7]);
                pm25_serial = word(data[8], data[9]);
                pm10_serial = word(data[10], data[11]);
            }

            NPM_waiting_for_16 = NPM_REPLY_CHECKSUM_16;
            break;

        case NPM_REPLY_CHECKSUM_16:
            hstream->readBytes(checksum, sizeof(checksum));

            memcpy(test, header, sizeof(header));
            memcpy(&test[sizeof(header)], state, sizeof(state));
            memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
            memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));

            if (Checksum_Valid(test, sizeof(test)))
            {
                *pm1 = pm1_serial;
                *pm25 = pm25_serial;
                *pm10 = pm10_serial;

                *pm1_pcs = n1_serial;
                *pm25_pcs = n25_serial;
                *pm10_pcs = n10_serial;

                result = true;
            }
            else
            {
                debug_outln_verbose(F("NPM Checksum NOT OK..."));
            }

            Log_data_reader(test, 16);

            NPM_waiting_for_16 = NPM_REPLY_HEADER_16;
            break;
        }
    }

    return result;
}

/// @brief raw temperature and relative humidity values
///   Sample:
///      Temperature = 0x0B40 which is 2880 in decimal.
///                    After dividing by 100, the physical value is 28.80 °C.
///                    Same calculation should be applied for calculating physical relative humidity.
/// @param ptr to *temp
/// @param ptr to *humi
/// @return
bool NextPM::ReadMeasuredTmp_HumValues(uint16_t *temp, uint16_t *humi)
{
    debug_outln_verbose(F("Read Temperature/Humidity values..."));

    bool result = false;
    uint8_t chrlen = 0;
    int reply = 5;

    uint16_t _temp = 0;
    uint16_t _humi = 0;

    Serial_Flush();

    Send_Cmd(PmSensorCmd2::Temphumi);

    while (!(chrlen = hstream->available()))
    { // wait till receive response from Tera sensor.
        debug_outln(F("Wait for NPM \"Temp-Hum\" Response..."), DEBUG_MAX_INFO);

        if (--reply == 0)
        {
            return F("");
        }

        delay(NEXT_PM_COMMAND_DELAY);
    }

    debug_outln_verbose(F("NPM available chars: ") + String(chrlen, HEX));

    NPM_waiting_for_8 = NPM_REPLY_HEADER_8;
    const uint8_t constexpr header[2] = {0x81, 0x14};

    uint8_t state[1];
    uint8_t data[4];
    uint8_t checksum[1];
    uint8_t test[8];

    if (chrlen == NPM_REPLY_HEADER_4)
    {
        Parser_StateValue(state);

        debug_outln_verbose(F("Tmp_Hum read ERROR, Current State: "), String(state[0]));

        *temp = 99; // test
        *humi = 9990;

        return false;
    }

    while (hstream->available() >= NPM_waiting_for_8)
    {
        switch (NPM_waiting_for_8)
        {
        case NPM_REPLY_HEADER_8:
            if (hstream->find(header, sizeof(header)))
            {
                NPM_waiting_for_8 = NPM_REPLY_STATE_8;
            }
            break;

        case NPM_REPLY_STATE_8:
            hstream->readBytes(state, sizeof(state));
            Display_State_Value(state[0]);
            NPM_waiting_for_8 = NPM_REPLY_BODY_8;
            break;

        case NPM_REPLY_BODY_8:
            if (hstream->readBytes(data, sizeof(data)) == sizeof(data))
            {
                Log_data_reader(data, 4);

                _temp = word(data[0], data[1]);
                _humi = word(data[2], data[3]);
            }

            NPM_waiting_for_8 = NPM_REPLY_CHECKSUM_8;
            break;

        case NPM_REPLY_CHECKSUM_16:
            hstream->readBytes(checksum, sizeof(checksum));

            memcpy(test, header, sizeof(header));
            memcpy(&test[sizeof(header)], state, sizeof(state));
            memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
            memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));

            if (Checksum_Valid(&test[0], 8))
            {
                result = true;
            }
            else
            {
                debug_outln_verbose(F("NPM Checksum NOT OK..."));
            }

            Log_data_reader(test, 8);

            NPM_waiting_for_8 = NPM_REPLY_HEADER_8;
            break;
        }
    }

    *temp = _temp;
    *humi = _humi;

    return result; // String(NPM_temp / 100.0f) + " / " + String(NPM_humi / 100.0f);
}

/*
    The NextPM has the ability to automatically trigger and regulate its internal heater in case of high
    relative humidity.
    This provides a better measurement accuracy in those specific environmental conditions by drying the
    input air and the particles.
    The heater is enabled from 60 %RH threshold and the heat generated is dependent on the measured
    relative humidity and so, the NextPM current consumption also (the additional current due to the
    heater can reach 140mA).

        Cmd code | Description
        --------------------------------------
          0x41     Heater OFF (0%)
          0x42     Heater ON  (100%)
          0x43     Automatic heater regulation (default)
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch" // ignored:  warning: enumeration value 'NPM_HEAT_MODE::none' not handled in switch

void NextPM::Set_Heater_Mode(NPM_HEAT_MODE mode)
{
    static uint8_t heater_off_cmd[] PROGMEM = {
        0x81, 0x41, 0x3E};

    static uint8_t heater_on_cmd[] PROGMEM = {
        0x81, 0x42, 0x3D};

    static uint8_t heater_auto_cmd[] PROGMEM = {
        0x81, 0x43, 0x3C};

    uint8_t cmd_len = sizeof(heater_off_cmd);
    uint8_t sndbuf[cmd_len];

    switch (mode)
    {
    case NPM_HEAT_MODE::none:
        // do nothink
        return;

    case NPM_HEAT_MODE::stopped:
        memcpy_P(sndbuf, heater_off_cmd, cmd_len);
        break;

    case NPM_HEAT_MODE::full:
        memcpy_P(sndbuf, heater_on_cmd, cmd_len);
        break;

    case NPM_HEAT_MODE::auto_regulated:
        memcpy_P(sndbuf, heater_auto_cmd, cmd_len);
        break;
    }

    hstream->write(sndbuf, cmd_len);

    Log_data_reader(sndbuf, cmd_len, false);

    int reply = 5;
    int len = 0;

    while (!(len = hstream->available() >= 3))
    { // wait till receive response from Tera sensor.
        debug_outln(F("Wait for NPM Heater_Mode Response..."), DEBUG_MAX_INFO);

        if (--reply == 0)
        {
            return;
        }

        delay(NEXT_PM_COMMAND_DELAY);
    }

    uint8_t response[len];
    hstream->readBytes(response, len);

    debug_outln(F("NPM_Heater_Mode response: "), DEBUG_MAX_INFO);
    Log_data_reader(response, len);
}

#pragma GCC diagnostic pop

/// @brief Display current state NextPM sensor.
/// @param test_state
void NextPM::Display_State_Error(uint8_t test_state)
{
    if (bitRead(test_state, 0) == 0)
    {
        debug_outln_info(F("\tNPM still wake..."));
    }
    else
    {
        debug_outln_info(F("\tNPM in Sleep mode..."));
    }

    if (bitRead(test_state, 1) == 1)
    {
        debug_out(F("\tDegraded state: "), DEBUG_MIN_INFO);
    }
    else
    {
        debug_out(F("\tCurrent state: "), DEBUG_MIN_INFO);
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
    }

    if (bitRead(test_state, 6) == 1)
    {
        debug_outln_info(F("Memory error"));
    }

    if (bitRead(test_state, 7) == 1)
    {
        debug_outln_info(F("Laser error"));
    }
}

/*********************************************************************************
 * send Tera Sensor Next PM sensor command state, change, concentration, version *
 *********************************************************************************/
/*****************************************************************
 * Helpers : Display Tera NextPM data on USB port.               *
 *****************************************************************/
void NextPM::Log_data_reader(const uint8_t data[], size_t size, bool RxdMode)
{
    String reader = RxdMode ? F("Response: ") : F("Send: ");

    for (size_t i = 0; i < size; i++)
    {
        reader += "0x";
        if (data[i] < 0x10)
        {
            reader += "0";
        }

        reader += String(data[i], HEX);

        if (i != (size - 1))
        {
            reader += ", ";
        }
    }

    debug_outln(reader, DEBUG_MAX_INFO);
}

/// @brief Display Tera NextPM State value on USB port.
/// @param bytedata
/// @return
String NextPM::Display_State_Value(uint8_t bytedata)
{
    String state = F("State: ");

    for (int b = 7; b >= 0; b--)
    {
        state += String(bitRead(bytedata, b));
    }

    debug_outln(state, DEBUG_MAX_INFO);

    return state;
}

/// @brief Empty Serial receive buffer.
void NextPM::Serial_Flush(void)
{
    while (hstream->available())
    {
        hstream->read();
        //delay(5);
    }
}

/// @brief NPM checksum valid
/// @param ptr to data[]
/// @param lenght of data[]
/// @return true OK, false CDV faild
bool NextPM::Checksum_Valid(const uint8_t *data, uint8_t len)
{
    uint8_t sum = 0;
    uint8_t checksum = 0;

    for (uint8_t idx = 0; idx < len; idx++)
    {
        sum += *(data + idx);
    }

    checksum = sum % 0x100;
    return (checksum == 0);
}

/// @brief : The checksum is calculated in order that the sum of all the frame bytes is
///          equal to a multiple of 256 (0x100).
///
/// @param data
/// @param len
/// @return check sum
uint8_t NextPM::Calculate_Checksum(const uint8_t *data, uint8_t len)
{
    uint8_t sum = 0;

    for (uint8_t idx = 0; idx < len; idx++)
    {
        sum += *(data + idx);
    }

    return 0x100 - sum;
}

/// @brief
/// @param cmd
void NextPM::Send_Cmd(PmSensorCmd2 cmd)
{
    static constexpr uint8_t state_cmd[] PROGMEM = {// read the current state
                                                    0x81, 0x16, 0x69};

    static constexpr uint8_t change_cmd[] PROGMEM = {// change the sate alternatively start/stop
                                                     0x81, 0x15, 0x6A};

    static constexpr uint8_t concentration_cmd[] PROGMEM = {
        // No continous mode => repeat call
                                        0x81, 0x11, 0x6E // Concentrations reading’s averaged over 10 seconds and updated every 1 second
                                        };

    static constexpr uint8_t version_cmd[] PROGMEM = {
                                                        0x81, 0x17, 0x68};

    static constexpr uint8_t speed_cmd[] PROGMEM = {
        // 0x81, 0x21, 0x00, 0x5E    //0% to get current value
                                                        0x81, 0x21, 0x32, 0x2C // 50%
                                                    };

    static constexpr uint8_t temphumi_cmd[] PROGMEM = {
                                                        0x81, 0x14, 0x6B};

    // CRC: 0x81 + 0x21 + 0x55 + 0x09 = 0x100

    /*constexpr*/ uint8_t cmd_len = array_num_elements(speed_cmd); // the larges command.
    uint8_t buf[cmd_len];

    switch (cmd)
    {
    case PmSensorCmd2::State:
        cmd_len = array_num_elements(state_cmd);
        memcpy_P(buf, state_cmd, cmd_len);
        break;
    case PmSensorCmd2::Change:
        cmd_len = array_num_elements(change_cmd);
        memcpy_P(buf, change_cmd, cmd_len);
        break;
    case PmSensorCmd2::Concentration:
        cmd_len = array_num_elements(concentration_cmd);
        memcpy_P(buf, concentration_cmd, cmd_len);
        break;
    case PmSensorCmd2::Version:
        cmd_len = array_num_elements(version_cmd);
        memcpy_P(buf, version_cmd, cmd_len);
        break;
    case PmSensorCmd2::Speed:
        memcpy_P(buf, speed_cmd, cmd_len);
        break;
    case PmSensorCmd2::Temphumi:
        cmd_len = array_num_elements(temphumi_cmd);
        memcpy_P(buf, temphumi_cmd, cmd_len);
        break;
    }

    // send Command string to Tera NextPM sensor firmware.
    hstream->write(buf, cmd_len);
}


