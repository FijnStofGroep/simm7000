/*
 * @file RCWL-0516.cpp
 *
 * Written by Roel, Rolenco Leusden
 *
 * RCWL-0516 Microwave Radar Motion Sensor.
 *
*/

#include "RCWL-0516.h"
#include "./utils.h"
#include "./Queue.h"

RCWL_0516 RCWL0516; // Create RCWL_0516 instance on Stack.

/*
 *
 * MotionSensorChangeEvent() => ISRs should be as short and fast as possible as they block normal program execution.
 *
 */
ICACHE_RAM_ATTR void MotionSensorChangeEvent()
{
  int motionValue = digitalRead(RCWL0516.MotionSensorID); // read Radar sensor value.

  RCWL0516.m_queue->Enqueue(motionValue);

  //debug_outln_verbose(F("MotionSensorChangeEvent()::Radar Motion/Sensor value: "), String(motionValue));
}

//**********************************************************************************************************************************
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/*
    Init RCWL_0516 Instance.

    Wait_max_time in sec.
*/
bool RCWL_0516::init(unsigned long Wait_max_time, int motionSensorID)
{
  MotionSensorID = motionSensorID;

  // send motion value only if this time has pased.
  // start on high signal and end on low signal.
  m_Wait_until_max_time_provided = Wait_max_time * 1000;

   pinMode(MotionSensorID, INPUT);
  // Radar Motion Sensor signal mode INPUT_PULLUP => is more stabale signal.
  //pinMode(MotionSensorID, INPUT_PULLUP);

  // Create a Queue[] array of 10 radar events could be stored on the heap.
  m_queue = new Queue(10);

  return true;
}

/******************************************************************
 *                                                                *
 ******************************************************************/
bool RCWL_0516::begin(const char *serverHost, uint port)
{
  strcpy(m_serverHost, serverHost); // m_serverHost = serverHost;
  m_port = port;
  m_Active = true;

  // Check connection to Server.
  //WiFiClient client;

  // if (!client.connect(m_serverHost, m_port))
  // {
  //   lastTrigger = millis() - m_timeSeconds;
  //   m_Active = false;
  // }

  // client.stop();

  /*
      Set MotionSensor pin as interrupt, assign interrupt function and set "CHANGE" mode
      only one interrupt event could be set.
      first => detachInterrupt(digitalPinToInterrupt(MotionSensorID));
      then attachInterrupt(digitalPinToInterrupt, Mode)

   * Mode – defines when the interrupt should be triggered. Five constants are predefined as valid values:
      LOW     Triggers the interrupt whenever the pin is LOW
      HIGH    Triggers the interrupt whenever the pin is HIGH
      CHANGE  Triggers the interrupt whenever the pin changes value, from HIGH to LOW or LOW to HIGH
      FALLING Triggers the interrupt when the pin goes from HIGH to LOW
      RISING  Triggers the interrupt when the pin goes from LOW to HIGH
   *
   */

  attachInterrupt(digitalPinToInterrupt(MotionSensorID), MotionSensorChangeEvent, CHANGE);

  // digitalWrite(led, LEDLOW);

  return m_Active;
}

/*
 *
 *
*/
bool RCWL_0516::setMQTTClient(PubSubClient& mqttclient, String _header)
{
  this->mqtt_client = &mqttclient;

		// ++ Set-Up Topic header for MQTT Broker
		if( _header.length() <= LEN_MQTT_LARGE_HEADER)
		{
			strcpy(mqtt_header, _header.c_str());
		}

  return false;
}


#pragma GCC diagnostic pop

/*
 * Motion -Sensor Message loop()
 */
void RCWL_0516::loop()
{
  if (RCWL0516.m_queue != NULL && !RCWL0516.m_queue->IsEmpty())
  {
    int motionValue = RCWL0516.m_queue->Dequeue();

    // debug_outln_verbose(F("loop()::Radar Motion-Sensor value: "), String(motionval));

    if (m_Wait_until_max_time_provided == 0)
    {// send all motion value to a external device.
      sendMotionValue( motionValue );
    }
    else
    {
      if (motionValue == HIGH)
      { // sensor is HIGH => radar motion detected it.
        if (m_motionState == LOW)
        {
          startTriggerEvent = millis();         // set start time value.
          m_motionState = HIGH;                 // update variable state to HIGH
        }
      }
      else
      {// sensor is LOW => radar motion detection ended.
        if (m_motionState == HIGH)
        {
          if ((millis() - startTriggerEvent) >= m_Wait_until_max_time_provided)
          { // Inform external application there is active motion detected.
            sendMotionValue( 2 );
            m_motionState = LOW; // update variable state to LOW
            startTriggerEvent = 0;
          }
          else
          {// to short time window => restart.
            m_motionState = LOW; // update variable state to LOW
            startTriggerEvent = 0;
          }
        }
      }
    }
  }
}

/*
    Stop Radar motion detection Event process.
*/
void RCWL_0516::end(void)
{
  detachInterrupt(digitalPinToInterrupt(MotionSensorID));
}

/*
  Get MotionCount
*/
unsigned long RCWL_0516::GetMotionCount()
{
  return count_RadarMotion;
}

//*************************************************************************************************************************************
/*
 * Send Radar motion to outside world.
 *  Input:
 *         motionValue: 0 => signal down.(end detect motion)
 *                      1 => signal up.  (begin detect motion)
 *                      2 => one motion cycle.
 *
*/
void RCWL_0516::sendMotionValue(int motionValue)
{
      time_t now = time(nullptr);       // Get time stamp.

      // Send Radar value to Server.
      SendToServer(motionValue, now);

      // Send Radar value to MQTT broker.
      sendMQTT(motionValue, now);

      count_RadarMotion++;
}

/*
 *  Send Radar motion value To a web-Server.
 *
 */
void RCWL_0516::SendToServer(int val, time_t now)
{
  char time_buffer[10];
  struct tm *timeinfo;
  timeinfo = localtime(&now);
  // if (timeinfo->tm_isdst == 1)                   // Daylight Saving Time flag
  // {
  //   timeinfo->tm_hour = timeinfo->tm_hour + 2;   // summer time
  // }
  // else
  // {
  //   timeinfo->tm_hour = timeinfo->tm_hour + 1;
  // }

  strftime(time_buffer, 10, "%H-%M-%S", timeinfo);
  String dspmessage = String(F("Date: ")) + String(time_buffer) + String(F(" Radar Motion value: ")) + String(val);
  debug_outln_verbose(F("SendToServer(), "), dspmessage);

  if (!m_Active)
  {
    // Current time
    currentTrigger = millis();

    if ((currentTrigger - lastTriggerEvent) < m_timeSeconds)
    {
      debug_outln_verbose(F("Wait for retry connect to Server = "), String((currentTrigger - lastTriggerEvent) / 1000) + F(" sec."));
      return;
    }

    m_Active = true;
  }

  WiFiClient client;

  debug_outln_verbose(F("connecting to "), String(m_serverHost) + F(":") + String(m_port));

  if (!client.connect(m_serverHost, m_port))
  {
    // Serial.println("connection failed");
    debug_outln_verbose(F("Connection failed to Server = "), String(m_serverHost));

    lastTriggerEvent = millis();
    m_Active = false;
    return;
  }

  if (client.connected())
  {
    // debug_outln_verbose(F("[Sending a request] => Radar Motion Value: "), String(val));
 
    // Send rader value to external Server.
    //client.print(String("Radar Value: ") + String(val));

    String message = String(F("Date:")) + String(time_buffer) + String(F("Radar Value:")) + String(val);
    client.print( message);
    client.stop();                      // clean-up client resouces.

    // debug_outln_verbose(F("[Sending a request] => ENDED: "));
  }
  else
  {
    debug_outln_verbose(F("Could Not connect to Server = "), String(m_serverHost));
  }
}

/*****************************************************************
* send radar motion value to mqtt api                            *
*                                                                *
*****************************************************************/
void RCWL_0516::sendMQTT(int val, time_t now)
{
	if ( mqtt_client != nullptr && mqtt_client->connected() )
	{
			debug_outln_verbose(F("Radar topic = "), mqtt_header);

      char time_buffer[10];
      struct tm *timeinfo;
      timeinfo = localtime(&now);
      strftime(time_buffer, 10, "%H-%M-%S", timeinfo);

      String status_header, payload_status;
      status_header = mqtt_header;
			status_header += "/radar";

			payload_status = "{\"";
      payload_status += F("Date");
			payload_status += "\":\"";
      payload_status += String(time_buffer);
			payload_status += "\":\"";
			payload_status += F("Value");
			payload_status += "\":\"";
			payload_status += String(val);
      payload_status += "\"}";

			if( mqtt_client->publish(status_header.c_str(), payload_status.c_str()))
			{
        mqtt_client->loop();

        //mqtt_client->flush();

				debug_outln_verbose(F("Radar send ok..."));
			}
			else
			{
        debug_outln_verbose(F("Radar send failed, rc= "), String(mqtt_client->state()));
			}
	}
  else
  {
			debug_outln_verbose(F("** MQTT Broker NOT connected **"));
  }

	return;
}
