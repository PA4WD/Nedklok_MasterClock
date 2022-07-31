#include <Arduino.h>
#include <WiFi.h>
#include <NetBIOS.h>
#include "ESPAsyncWebServer.h"
#include "ESP32SSDP.h"
#include "time.h"
#include "sntp.h"
#include "credentials.h"

const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWD;

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

const char *time_zone = "CET-1CEST,M3.5.0,M10.5.0/3"; // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

AsyncWebServer webserver(80);

#define L9110S_1B 25
#define L9110S_1A 26

String outputState(int output)
{
  if (digitalRead(output))
  {
    return "checked";
  }
  else
  {
    return "";
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Nedklok Masterclock</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
</head>
<body>
  <h2>Nedklok Masterclock</h2>
  %CURRENTTIME%
  %BUTTONPLACEHOLDER%
  <script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
  }
</script>
</body>
</html>
)rawliteral";

String processor(const String &var)
{
  if (var == "CURRENTTIME")
  {
    String time = "";
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
      char s[100];
      strftime(s, sizeof(s), "<h3>Datum: %d-%m-%y</h3><h3>Tijd: %T</h3>", &timeinfo);
      time = s;
    }
    else
    {
      time = "No time available (yet)";
    }
    return time;
  }
  if (var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    buttons += " <button type=\"button\" onchange=>Seconden</button>";
    buttons += "<h4>Add Second</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

// Replaces placeholder with button section in your web page
// String processor(const String &var)
// {
//   // Serial.println(var);
//   if (var == "BUTTONPLACEHOLDER")
//   {
//     String buttons = "";
//     buttons += "<h4>Output - GPIO 2</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider\"></span></label>";
//     buttons += "<h4>Output - GPIO 4</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + outputState(4) + "><span class=\"slider\"></span></label>";
//     buttons += "<h4>Output - GPIO 33</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"33\" " + outputState(33) + "><span class=\"slider\"></span></label>";
//     return buttons;
//   }
//   return String();
//}

bool positive;

void tickSecond()
{
  Serial.println("Second tick");
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
}

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
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setupSSDP()
{
  Serial.printf("Starting HTTP...\n");
  // Route for root / web page
  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               { request->send_P(200, "text/html", index_html, processor); });
  webserver.on("/index.html", HTTP_GET, [&](AsyncWebServerRequest *request)
               { request->send_P(200, "text/html", index_html, processor); });
  // webserver.on("/description.xml", HTTP_GET, [&](AsyncWebServerRequest *request)
  //{ request->send(200, "text/xml", SSDP.schema(false)); });
  webserver.on("/description.xml", HTTP_GET, [&](AsyncWebServerRequest *request)
               { request->send(200, "text/xml", SSDP.getSchema()); });

  webserver.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    Serial.println(request->params());

    // String inputMessage1;
    // String inputMessage2;
    // // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    // if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
    //   inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
    //   inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
    //   digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    // }
    // else {
    //   inputMessage1 = "No message sent";
    //   inputMessage2 = "No message sent";
    // }
    // Serial.print("GPIO: ");
    // Serial.print(inputMessage1);
    // Serial.print(" - Set to: ");
    // Serial.println(inputMessage2);
    // request->send(200, "text/plain", "OK"); 
    //
     });

  webserver.begin();

  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName("Nedklok Masterclock");
  // SSDP.setSerialNumber("001788102201");
  SSDP.setURL("index.html");
  SSDP.setModelName("Nedklok MasterClock");
  SSDP.setModelDescription("Masterclock for nedklok");
  // SSDP.setModelNumber("929000226503");
  // SSDP.setModelURL("http://www.meethue.com");
  // SSDP.setManufacturer("Royal Philips Electronics");
  // SSDP.setManufacturerURL("http://www.philips.com");
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

  Serial.printf("Starting SSDP...\n");
  SSDP.begin();
}

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
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
}

void setup()
{
  pinMode(L9110S_1A, OUTPUT);
  pinMode(L9110S_1B, OUTPUT);

  Serial.begin(9600);

  // set notification call-back function
  sntp_set_time_sync_notification_cb(timeavailable);

  /**
   * This will set configured ntp servers and constant TimeZone/daylightOffset
   * should be OK if your time zone does not need to adjust daylightOffset twice a year,
   * in such a case time adjustment won't be handled automagicaly.
   */
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  /**
   * A more convenient approach to handle TimeZones with daylightOffset
   * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
   * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
   */
  // configTzTime(time_zone, ntpServer1, ntpServer2);

  Serial.println();
  connectWifi();
  Serial.println();

  NBNS.begin("Nedklok-MasterClock");
  setupSSDP();
}

int lastSecond;

void loop()
{
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    if (timeinfo.tm_sec != lastSecond)
    {
      lastSecond = timeinfo.tm_sec;
      tickSecond();
      Serial.printf("second past - %d\r\n", lastSecond);
    }
  }
  else
  {
    Serial.println("No time available (yet)");
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    connectWifi();
  }
}