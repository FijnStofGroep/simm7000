
#ifndef __RCWL_0516_H__
#define __RCWL_0516_H__

#include "Arduino.h"
#include <ESP8266WiFi.h>

class RCWL_0516
{

public:
  // Set GPIO no. Radar Motion Sensor.
  int MotionSensorID = 12; // Radar data out.

  // Timer: Auxiliary variables
  int motionState; // by default, no motion detected
  int motionValue; // variable to store the sensor status (value)
  bool flgSendToServer;

  // constructor
  RCWL_0516()
  {
    flgSendToServer = false;
    motionState = LOW;
    motionValue = 0;
  };

  // destructor => Clean-up all resource.
  ~RCWL_0516(){};

  void begin(char *serverHost, uint port);
  void loop(void);

private:
  const int LEDHIGH = LOW;
  const int LEDLOW = HIGH;

  // Set GPIO no. for LED indicator.
  int MotionLedID = 2;     // blue led on board

  char m_serverHost[25] = "192.168.2.105"; // server has static IPAdres.
  uint m_port = 8080;                      // 8080 default port nr.

  void SendToServer(int val);
};

// RCWL0516 declared in cpp file
extern RCWL_0516 RCWL0516;

#endif
