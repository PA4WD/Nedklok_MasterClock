#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "sntp.h"
#include "nedclock.h"
#include "nedclock_server.h"

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *time_zone = "CET-1CEST,M3.5.0,M10.5.0/3"; // TimeZone rule for Europe/Amsterdam including daylight adjustment
clockState_t currentState;
int currentSeconds;

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
    Serial.println("Got time adjustment from NTP!");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("No time available (yet)");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

void initClock()
{
    pinMode(L9110S_1A, OUTPUT);
    pinMode(L9110S_1B, OUTPUT);

    currentState = RUNNING;

    // set notification call-back function
    sntp_set_time_sync_notification_cb(timeavailable);

    /**
     * This will set configured ntp servers and constant TimeZone/daylightOffset
     * should be OK if your time zone does not need to adjust daylightOffset twice a year,
     * in such a case time adjustment won't be handled automagicaly.
     */
    // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

    /**
     * A more convenient approach to handle TimeZones with daylightOffset
     * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
     * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
     */
    configTzTime(time_zone, ntpServer1, ntpServer2);
}

bool positive;
void tickSecond(int ticks)
{
    for (int i = ticks; i > 0; i--)
    {
        // Serial.println("Second tick");

        if (positive)
        {
            digitalWrite(L9110S_1A, 1);
            positive = false;
        }
        else
        {
            digitalWrite(L9110S_1B, 1);
            positive = true;
        }
        delay(200);

        digitalWrite(L9110S_1A, 0);
        digitalWrite(L9110S_1B, 0);

        // LED.Toggle();

        if (i > 1)
        {
            delay(200);
        }
    }
}

int lastSecond;
void tickClock()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        if (timeinfo.tm_sec != lastSecond)
        {
            // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
            lastSecond = timeinfo.tm_sec;
            // Serial.printf("second past - %d\r\n", lastSecond);

            switch (currentState)
            {
            case RUNNING:
                tickSecond(1);
                break;
            case STOPPED:
                break;
            case ADDING:
                tickSecond(currentSeconds);
                currentState = RUNNING;
                break;
            case WAITING:
                currentSeconds--;
                if (currentSeconds == 0)
                {
                    currentState = RUNNING;
                }

                break;
            default:
                break;
            }

            updateClientTime(&timeinfo);
        }
    }
    else
    {
        Serial.println("No time available (yet)");
    }
}

const String clockStatesStr[] = {"Running", "Stopped", "Adding", "waiting"};
String getClockState()
{
    return clockStatesStr[currentState];
}

void setClockState(clockState_t state)
{
    currentState = state;
}

void setClockState(clockState_t state, int seconds)
{
    currentState = state;
    currentSeconds = seconds;
}
