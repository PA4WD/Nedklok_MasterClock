#include <Arduino.h>
#include "FS.h"
#include "FFat.h"
#include "nedclock.h"
#include "nedclock_server.h"
#include "nedclock_wifi.h"

void setup()
{
  Serial.begin(9600);

  if (!FFat.begin(true))
  {
    Serial.println("FFat Mount Failed");
    return;
  }

  initClock();
  connectWifi();
  initWebserver();
  initSSDP();
}

void loop()
{
  tickClock();
  checkWifi();
  cleanWebSockets();
}