#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

Adafruit_SSD1306 display;
ESP8266WebServer webServer;

***REMOVED***
***REMOVED***

void drawStatus(const char *msg);

void handleConnection();
void handleImage();

void setup()
{
    SPIFFS.begin();
    Wire.begin(0, 2);
    display.begin();
    drawStatus("Connecting to wifi");
    WiFi.begin(WIFI_NAME, WIFI_PWD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }
    char str[255];
    snprintf(str, 254, "wifi connected! \n%s", WiFi.localIP().toString().c_str());
    drawStatus(str);

    webServer.on("/image", HTTP_POST, handleImage);
    webServer.on("/image", HTTP_OPTIONS, []() {
        webServer.sendHeader("Access-Control-Max-Age", "10000");
        webServer.sendHeader("Access-Control-Allow-Origin", "*");
        webServer.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
        webServer.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
        webServer.send(200, "text/plain", "");
    });
    webServer.onNotFound(handleConnection);
    webServer.begin();
}

void loop()
{
    webServer.handleClient();
}

void drawStatus(const char *msg)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(WHITE);
    display.println(msg);
    display.display();
}

void handleConnection()
{
    drawStatus(webServer.uri().c_str());
    fs::File index = SPIFFS.open("/index.html", "r");
    webServer.streamFile(index, "text/html");
}

void handleImage()
{
    webServer.sendHeader("Access-Control-Max-Age", "10000");
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    webServer.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    if (!webServer.hasArg("plain"))
    {
        webServer.send(403, "text/plain", "No body received!");
        return;
    }

    String body = webServer.arg("plain");
    if (body.length() > 1024)
    {
        webServer.send(403, "text/plain", "Body needs to be 1024 bytes long, was: " + String(body.length()));
        return;
    }

    webServer.send(200, "text/plain", "Image received!");
    display.clearDisplay();
    uint8_t bitmap[1024];
    memset(bitmap, 0, 1024);
    memcpy(bitmap, body.c_str(), body.length());
    display.drawBitmap(0, 0, bitmap, 128, 64, WHITE);
    display.display();
}