#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "sntp.h"
#include "nedclock.h"
#include "nedclock_server.h"
#include <Wire.h>
#include <RtcDS3231.h> //https://github.com/Makuna/Rtc/


const char *ntpServer1 = "nl.pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *time_zone = "CET-1CEST,M3.5.0,M10.5.0/3"; // TimeZone rule for Europe/Amsterdam including daylight adjustment
clockState_t currentState;
int currentMinutes;

RtcDS3231<TwoWire> Rtc(Wire);


// Callback function (get's called when time adjusts via NTP)
void timeAvailable(struct timeval *t)
{
    Serial.println("Got time adjustment from NTP!");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("No time available (yet)");
        return;
    }
    Serial.println(&timeinfo, "NTP time = %A, %B %d %Y %H:%M:%S zone %Z %z ");

    //time_t now;
    //time(&now);
    //RtcDateTime utc;
    //utc.InitWithEpoch64Time(now);
    RtcDateTime utc(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    //RtcDateTime ntpdt = RtcDateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    //RTC.SetDateTime(ntpdt);

    Rtc.SetDateTime(utc);
}

#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u-%02u-%04u %02u:%02u:%02u"),
            dt.Day(),
            dt.Month(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.println(datestring);
}

String getISO8601(const RtcDateTime& dt)
{
    //YYYY-MM-DDThh:mm:ssTZD (eg 1997-07-16T19:20:30+01:00)

    time_t epoch = dt.Epoch32Time();
    struct tm *ptm;
    ptm = gmtime(&epoch);
    char buf[32];
    strftime(buf, sizeof(buf), "%FT%T%z", ptm); //YYYY-MM-DDThh:mm:ssTZD (eg 1997-07-16T19:20:30+01:00)
    //Serial.println(buf);
    return buf;
} 

// handy routine to return true if there was an error
// but it will also print out an error message with the given topic
bool wasError(const char* errorTopic = "")
{
    uint8_t error = Rtc.LastError();
    if (error != 0)
    {
        // we have a communications error
        // see https://www.arduino.cc/reference/en/language/functions/communication/wire/endtransmission/
        // for what the number means
        Serial.print("[");
        Serial.print(errorTopic);
        Serial.print("] WIRE communications error (");
        Serial.print(error);
        Serial.print(") : ");

        switch (error)
        {
        case Rtc_Wire_Error_None:
            Serial.println("(none?!)");
            break;
        case Rtc_Wire_Error_TxBufferOverflow:
            Serial.println("transmit buffer overflow");
            break;
        case Rtc_Wire_Error_NoAddressableDevice:
            Serial.println("no device responded");
            break;
        case Rtc_Wire_Error_UnsupportedRequest:
            Serial.println("device doesn't support request");
            break;
        case Rtc_Wire_Error_Unspecific:
            Serial.println("unspecified error");
            break;
        case Rtc_Wire_Error_CommunicationTimeout:
            Serial.println("communications timed out");
            break;
        }
        return true;
    }
    return false;
}

void initClock()
{
    Serial.println("Init Clock");

    pinMode(L9110S_1A, OUTPUT);
    pinMode(L9110S_1B, OUTPUT);

    Wire.begin(DS3231_SDA, DS3231_SCL);
    Rtc.Begin();
    
    RtcDateTime now = Rtc.GetDateTime();
    if (!wasError("initclock"))
    {
         printDateTime(now);
         Serial.println();
    }

    currentState = RUNNING;

    // set ntp notification call-back function
    sntp_set_time_sync_notification_cb(timeAvailable);

    /**
     * This will set configured ntp servers and constant TimeZone/daylightOffset
     * should be OK if your time zone does not need to adjust daylightOffset twice a year,
     * in such a case time adjustment won't be handled automagicaly.
     */
    //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

    /**
     * A more convenient approach to handle TimeZones with daylightOffset
     * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
     * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
     */
    configTzTime(time_zone, ntpServer1, ntpServer2);
}

bool positive;
void tickClock(int ticks)
{
    for (int i = ticks; i > 0; i--)
    {
        //Serial.println("Clock tick");

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
int lastMinute;
void tickSecond()
{
    RtcDateTime now = Rtc.GetDateTime();
    if (!wasError("tickSecond"))
    {    
        if (now.Second() != lastSecond)
        {
            lastSecond = now.Second();
            //printDateTime(now);

            switch (currentState)
            {
            case RUNNING:
                if (now.Minute() != lastMinute)
                {
                    lastMinute = now.Minute();
                    tickClock(1);
                }
                break;
            case STOPPED:
                break;
            case ADDING:
                tickClock(currentMinutes);
                currentState = RUNNING;
                break;
            case WAITING:
                if (now.Minute() != lastMinute)
                {
                    lastMinute = now.Minute();
                    currentMinutes--;
                    if (currentMinutes == 0)
                    {
                        currentState = RUNNING;
                    }
                }
                break;
            default:
                break;
            }

            updateClientTime(getISO8601(now));
        }        
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

void setClockState(clockState_t state, int minutes)
{
    currentState = state;
    currentMinutes = minutes;
}

float getTemperature()
{
    RtcTemperature temp = Rtc.GetTemperature();
    if (!wasError("loop GetTemperature"))
    {
        //Serial.print(temp.AsFloatDegC());
        //Serial.println("C");
        return temp.AsFloatDegC();
    }
    return 0.0;
}
