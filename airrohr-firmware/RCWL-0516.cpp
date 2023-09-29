/*********

  RCWL-0516 Microwave Radar Motion Sensor.

*********/

#include "RCWL-0516.h"
#include "./utils.h"

RCWL_0516 RCWL0516;

/*
 *
 * DetectsCHANGE() => ISRs should be as short and fast as possible as they block normal program execution.
 *
 */
ICACHE_RAM_ATTR void DetectsCHANGE()
{
  // Serial.println("MOTION CHANGED DETECTED!!!");

  RCWL0516.motionValue = digitalRead(RCWL0516.MotionSensorID); // read sensor value

  debug_outln_info(F("Radar Motion/Sensor value: "), String(RCWL0516.motionValue));
  // Serial.println( printf("sensor value: %d", RCWL0516.motionValue));

  RCWL0516.flgSendToServer = true;
}

//***********************************************************************************************************
/*
 *  Send motion value To a Server.
 *
 */
void RCWL_0516::SendToServer(int val)
{
  // printf("SendToServer(): Sensor value: %d\n", val);
  debug_outln_info(F("SendToServer(): Radar Motion value: "), String(val));

  WiFiClient client;

  Serial.print("connecting to ");
  Serial.print(m_serverHost);
  Serial.print(':');
  Serial.println(m_port);
  // Serial.printf("\n[Connecting to %s:%d ...]\n", host, port);

  if (!client.connect(m_serverHost, m_port))
  {
    // Serial.println("connection failed");
    Serial.println(String("Connection failed to Host = ") + String(m_serverHost));
    delay(1000);
    return;
  }

  if (client.connected())
  {
    // Serial.println("** [Connected] **");

    debug_outln_info(F("[Sending a request] => Radar Motion Value: "), String(val));

    client.print(String("Radar Value: ") + String(val));
    // client.println( String( "Radar Value: ") + String(val));   // send include "\r\n" char.

    // delay(500);
    client.stop();

    // Serial.println("** [Disconnect] **");
  }
  else
  {
    debug_outln_info(F("Could Not connect to Host = "), String(m_serverHost));
  }
}

//***********************************************************************************************************
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/*
 *
 */
void RCWL_0516::begin(char *serverHost, uint port)
{
  strcpy(m_serverHost, serverHost); // m_serverHost = serverHost;
  m_port = port;

  // Set GPIO mode (LED) to output
  //pinMode(MotionLedID, OUTPUT);

  // PIR Motion Sensor mode INPUT_PULLUP => is more stabale signal.
  pinMode(MotionSensorID, INPUT_PULLUP);
  // pinMode(MotionSensorID, INPUT);

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

  attachInterrupt( digitalPinToInterrupt(MotionSensorID), DetectsCHANGE, CHANGE);

  // digitalWrite(led, LEDLOW);
}

#pragma GCC diagnostic pop

/*
 * Message loop
 */
void RCWL_0516::loop()
{
  if (flgSendToServer)
  {
    flgSendToServer = false;

    // Send Radar value to Server.
    SendToServer(motionValue);

    // if (motionValue == HIGH)
    // {                                   // check if the sensor is HIGH
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
