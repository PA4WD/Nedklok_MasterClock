#include <Arduino.h>
#include <WiFi.h>
#include "credentials.h"

const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWD;

// connect to wifi network
void connectWifi()
{
    // attempt to connect to Wifi network:
    WiFi.mode(WIFI_STA);
    Serial.print(F("Connecting to "));
    Serial.println(ssid);

    // Connect to WEP/WPA/WPA2 network:
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }

    Serial.println();
    Serial.println(F("WiFi connected"));
    Serial.print(F("IPv4: "));
    Serial.println(WiFi.localIP());
    Serial.print(F("IPv6: "));
    Serial.println(WiFi.localIPv6());
}

void checkWifi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectWifi();
    }
}