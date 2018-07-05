#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <Base64.h>

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

Adafruit_SSD1306 display;
ESP8266WebServer webServer;

unsigned long lastImage;
bool imageSet = false;

***REMOVED***
***REMOVED***

#define MAX_DISPLAY_BYTES (SSD1306_LCDWIDTH / 8 * SSD1306_LCDHEIGHT)

#define DISPLAY_TIME (2000)

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
    if (millis() > lastImage + DISPLAY_TIME)
    {
        imageSet = false;
        char str[255];
        snprintf(str, 254, "wifi connected! \n%s", WiFi.localIP().toString().c_str());
        drawStatus(str);
    }
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
    fs::File index = SPIFFS.open("/index.html", "r");
    webServer.streamFile(index, "text/html");
}

void handleImage()
{
    webServer.sendHeader("Access-Control-Max-Age", "10000");
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    webServer.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");

    if (imageSet)
    {
        webServer.send(409,
                       "text/plain",
                       "Image is already being displayed, try again in " +
                           String(static_cast<float>(lastImage + DISPLAY_TIME - millis()) / 1000.f) +
                           " seconds!");
        return;
    }

    if (!webServer.hasArg("width"))
    {
        webServer.send(403, "text/plain", "Width parameter expected!");
        return;
    }

    if (!webServer.hasArg("height"))
    {
        webServer.send(403, "text/plain", "Height parameter expected!");
        return;
    }

    if (!webServer.hasArg("data"))
    {
        webServer.send(403, "text/plain", "Data parameter expected!");
        return;
    }

    size_t width = webServer.arg("width").toInt();
    size_t height = webServer.arg("height").toInt();

    if (width > SSD1306_LCDWIDTH || height > SSD1306_LCDHEIGHT)
    {
        webServer.send(403, "text/plain", "Image can be at most 128x64 pixels in size!");
        return;
    }

    size_t expectedDataLength = ceil(static_cast<float>(width) / 8.f) * height;

    String encodedData = webServer.arg("data");

    char encodedBuffer[encodedData.length()];
    strncpy(encodedBuffer, encodedData.c_str(), encodedData.length());

    if (Base64.decodedLength(encodedBuffer, encodedData.length()) == -1)
    {
        webServer.send(403, "text/plain", "Could not decode image data!");
        return;
    }

    if (Base64.decodedLength(encodedBuffer, encodedData.length()) != static_cast<int>(expectedDataLength))
    {
        webServer.send(403, "text/plain", "Image data length does not match image size!");
        return;
    }

    char decodedData[expectedDataLength];

    if (Base64.decode(decodedData, encodedBuffer, encodedData.length()) == -1)
    {
        webServer.send(403, "text/plain", "Could not decode image data!");
        return;
    }

    webServer.send(200, "text/plain", "Image received!");
    imageSet = true;
    lastImage = millis();
    display.clearDisplay();
    display.drawBitmap(0, 0, reinterpret_cast<uint8_t *>(decodedData), width, height, WHITE);
    display.display();
}