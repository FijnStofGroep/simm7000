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
  int motionValue = digitalRead(RCWL0516.MotionSensorID); // read sensor value

  RCWL0516.m_queue->Enqueue(motionValue);

  debug_outln_info(F("MotionSensorChangeEvent()::Radar Motion/Sensor value: "), String(motionValue));
}

//**********************************************************************************************************************************
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/*
    Init RCWL_0516 Instance.
*/
bool RCWL_0516::init(int motionSensorID)
{
  // Set GPIO mode (LED) to output
  // pinMode(MotionLedID, OUTPUT);

  MotionSensorID = motionSensorID;

  // pinMode(MotionSensorID, INPUT);
  // PIR Motion Sensor mode INPUT_PULLUP => is more stabale signal.
  pinMode(MotionSensorID, INPUT_PULLUP);

  // Create a Queue[] array of capacity 10 on the heap.
  m_queue = new Queue(10);

  return true;
}

/*
 *
 */
bool RCWL_0516::begin(const char *serverHost, uint port)
{
  strcpy(m_serverHost, serverHost); // m_serverHost = serverHost;
  m_port = port;
  m_Active = true;
  WiFiClient client;

  if (!client.connect(m_serverHost, m_port))
  {
    lastTrigger = millis() - m_timeSeconds;
    m_Active = false;
  }

  client.stop();

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

#pragma GCC diagnostic pop

/*
 * Motion -Sensor Message loop()
 */
void RCWL_0516::loop()
{
  if (RCWL0516.m_queue != NULL && !RCWL0516.m_queue->IsEmpty())
  {
    int motionval = RCWL0516.m_queue->Dequeue();

    // debug_outln_info(F("loop()::Radar Motion-Sensor value: "), String(motionval));

    // Send Radar value to Server.
    SendToServer(motionval);

    // if (motionValue == HIGH)
    // { // check if the sensor is HIGH
    //   digitalWrite(MotionLed, LEDHIGH); // turn LED ON

    //   if (motionState == LOW)
    //   {
    //     Serial.println("Motion detected!");
    //     RWCL0516.motionState = HIGH; // update variable state to HIGH
    //   }
    // }
    // else
    // {
    //   digitalWrite(MotionLed, LEDLOW); // turn LED OFF

    //   if (motionState == HIGH)
    //   {
    //     Serial.println("***** Motion stopped *******!");
    //     motionState = LOW; // update variable state to LOW
    //   }
    // }
  }
}

/*
    Stop Radar motion detection Event process.
*/
void RCWL_0516::end(void)
{
  detachInterrupt(digitalPinToInterrupt(MotionSensorID));
}

//*************************************************************************************************************************************
/*
 *  Send Radar motion value To a Server.
 *
 */
void RCWL_0516::SendToServer(int val)
{
  debug_outln_info(F("SendToServer(): Radar Motion value: "), String(val));

  if (!m_Active)
  {
    // Current time
    now = millis();

    if ((now - lastTrigger < m_timeSeconds))
    {
      debug_outln_info(F("Wait for retry connect to Server = "), String((now - lastTrigger) / 1000) + F(" sec."));
      return;
    }

    m_Active = true;
  }

  WiFiClient client;

  debug_outln_info(F("connecting to "), String(m_serverHost) + F(":") + String(m_port));

  if (!client.connect(m_serverHost, m_port))
  {
    // Serial.println("connection failed");
    debug_outln_info(F("Connection failed to Server = "), String(m_serverHost));

    lastTrigger = millis();
    m_Active = false;
    return;
  }

  if (client.connected())
  {
    // debug_outln_info(F("[Sending a request] => Radar Motion Value: "), String(val));

    client.print(String("Radar Value: ") + String(val));

    client.stop();

    // debug_outln_info(F("[Sending a request] => ENDED: "));
  }
  else
  {
    debug_outln_info(F("Could Not connect to Server = "), String(m_serverHost));
  }
}
