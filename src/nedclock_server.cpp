#include <Arduino.h>
#include <WiFi.h>
#include "ESP32SSDP.h"
#include "FS.h"
#include "FFat.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "nedclock.h"

AsyncWebServer webserver(80);
AsyncWebSocket ws("/ws");

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
        client->ping();
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
    }
    else if (type == WS_EVT_ERROR)
    {
        Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
    }
    else if (type == WS_EVT_PONG)
    {
        Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        String msg = "";

        if (info->opcode == WS_TEXT)
        {
            for (size_t i = 0; i < info->len; i++)
            {
                msg += (char)data[i];
            }
            Serial.printf("%s\n", msg.c_str());

            String cmd = msg.c_str();
            if (cmd.startsWith("start"))
            {
                setClockState(RUNNING);
            }
            else if (cmd.startsWith("stop"))
            {
                setClockState(STOPPED);
            }
            else if (cmd.startsWith("add"))
            {
                int seconds = cmd.substring(cmd.indexOf(' ')).toInt();
                // Serial.printf("add - %d\n", seconds);
                setClockState(ADDING, seconds);
            }
            else if (cmd.startsWith("wait"))
            {
                int seconds = cmd.substring(cmd.indexOf(' ')).toInt();
                // Serial.printf("wait - %d\n", seconds);
                setClockState(WAITING, seconds);
            }
        }
    }
}

void updateClientTime(tm *time)
{
    if (ws.getClients().isEmpty() == false)
    {
        //Serial.println(time, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
        char buf[32];
        strftime(buf, sizeof(buf), "%FT%T%z", time); //YYYY-MM-DDThh:mm:ssTZD (eg 1997-07-16T19:20:30+01:00)
        StaticJsonDocument<128> doc;
        doc["time"] = buf;
        doc["state"] = getClockState();
        //serializeJsonPretty(doc, Serial);
        String output;
        serializeJson(doc, output);
        ws.textAll(output);
    }
}

//--------------- Server setup -------------------------------
void initWebserver()
{
    Serial.printf("Starting HTTP...\n");

    webserver.onNotFound([](AsyncWebServerRequest *request)
                         { request->send(FFat, "/index.htm", "text/html"); });

    webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(FFat, "/index.htm", "text/html"); });

    webserver.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(FFat, "/style.css", "text/css"); });

    ws.onEvent(onWsEvent);
    webserver.addHandler(&ws);

    webserver.begin();
}

void initSSDP()
{
    Serial.printf("Starting SSDP...\n");

    webserver.on("/description.xml", HTTP_GET, [&](AsyncWebServerRequest *request)
                 { request->send(200, "text/xml", SSDP.getSchema()); });

    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(80);
    SSDP.setName("Nedklok Masterclock");
    SSDP.setSerialNumber("1");
    SSDP.setURL("index.html");
    SSDP.setModelName("Nedklok MasterClock");
    SSDP.setModelDescription("Masterclock for nedklok");
    SSDP.setModelNumber("Model 1");
    SSDP.setModelURL("http://www.pa4wd.nl");
    SSDP.setManufacturer("PA4WD");
    SSDP.setManufacturerURL("http://www.pa4wd.nl");
    //"urn:schemas-upnp-org:device:Basic:1" if not set
    SSDP.setDeviceType("rootdevice"); // to appear as root device, other examples: MediaRenderer, MediaServer ...
    //"Arduino/1.0" if not set
    // SSDP.setServerName("SSDPServer/1.0");
    // set UUID, you can use https://www.uuidgenerator.net/
    // use 38323636-4558-4dda-9188-cda0e6 + 4 last bytes of mac address if not set
    // use SSDP.setUUID("daa26fa3-d2d4-4072-bc7a-a1b88ab4234a", false); for full UUID
    // SSDP.setUUID("daa26fa3-d2d4-4072-bc7a");
    // Set icons list, NB: optional, this is ignored under windows
    // SSDP.setIcons("<icon>"
    //               "<mimetype>image/png</mimetype>"
    //               "<height>48</height>"
    //               "<width>48</width>"
    //               "<depth>24</depth>"
    //               "<url>icon48.png</url>"
    //               "</icon>");
    // Set service list, NB: optional for simple device
    // SSDP.setServices("<service>"
    //                  "<serviceType>urn:schemas-upnp-org:service:SwitchPower:1</serviceType>"
    //                  "<serviceId>urn:upnp-org:serviceId:SwitchPower:1</serviceId>"
    //                  "<SCPDURL>/SwitchPower1.xml</SCPDURL>"
    //                  "<controlURL>/SwitchPower/Control</controlURL>"
    //                  "<eventSubURL>/SwitchPower/Event</eventSubURL>"
    //                  "</service>");

    SSDP.begin();
}

void cleanWebSockets()
{
    ws.cleanupClients();
}
