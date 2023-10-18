/*!
 * @file RCWL-0516.h
 *
 * Written by Roel, Rolenco Leusden
 *
 * BSD license, all text here must be included in any redistribution.
 * See the LICENSE file for details.
 *
 */


#ifndef __RCWL_0516_H__
#define __RCWL_0516_H__

// VS: Convert Arduino file to C++ manually.
#include <Arduino.h>
#include <ESP8266WiFi.h>

class RCWL_0516
{

public:
  // Set GPIO no. Radar Motion Sensor.
  //                        // comment: pinNo D8 => MCU DON'T start-UP.
  int MotionSensorID = D7;  // Radar data out. => the pin D7 = 13 that the sensor is attached to.

  // Auxiliary variables
  int motionState;          // by default, no motion detected

  Queue* m_queue = NULL;

  /*
    constructor
  */
  RCWL_0516()
  {
    motionState = LOW;
    lastTrigger = millis();
  };

  /*
    destructor => Clean-up all resource.
  */
  ~RCWL_0516()
  {
    if( m_queue != NULL)
    {
      delete m_queue;         // destructor will get called here, after which it's memory is freed => remove m_queue from heap memory.
    }
  };

  //public function methods
  bool init(int motionSensorID = D7);             // default: pin D7(13) that the radar sensor is attached to.
  bool begin(const char *serverHost, uint port);  
  void loop(void);
  void end(void);                                 // end/stop motion Event process.
  unsigned long GetMotionCount();

private:
  const int LEDHIGH = LOW;
  const int LEDLOW = HIGH;
  unsigned long m_timeSeconds = 30 * 1000;        // WaitTime in seconds. Every 30 sec. retry to connect to Server.

 // Set GPIO no. for LED indicator.
  int MotionLedID = LED_BUILTIN;                  // blue led on board

// Timer: Auxiliary variables
  unsigned long now = 0;
  unsigned long lastTrigger = 0;
  unsigned long count_RadarMotion = 0;

  bool m_Active = false;
  char m_serverHost[25] = "192.168.2.105";        // server has static IPAdres.
  uint m_port = 8080;                             // 8080 default port nr.

  // private function methods
  void SendToServer(int val);
 
};

// external declaration of RCWL0516 instances.
extern RCWL_0516 RCWL0516;

#endif
