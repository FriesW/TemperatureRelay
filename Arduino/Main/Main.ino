//Target: generic ESP8266
//Arduino board manager path: http://arduino.esp8266.com/stable/package_esp8266com_index.json
//Board: Generic ESP8266 Module
//Flash Mode: DIO
//Flash Frequency: 40MHz
//Upload Using: Serial
//CPU Frequency: 80 MHz
//Flash Size: 512K (64K SPIFFS)
//Upload Speed: 115200

#include <ESP8266WiFi.h>

#define ssid ""
#define pass ""

void setup()
{
    Serial.begin(115200);
    delay(10000);
    Serial.println();
    Serial.print("Connecting");
    WiFi.begin(ssid, pass);
    while(WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println("Connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop()
{
    
}