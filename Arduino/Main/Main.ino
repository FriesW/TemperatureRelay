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

#define ulong unsigned long
#define uint unsigned int

#define ssid ""
#define pass ""

#define dht 2

void setup()
{
    Serial.begin(115200);
    pinMode(dht, INPUT);
    
    Serial.println("'Booting'");
    delay(10000);
    Serial.println("Done");
    
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
    
    get_dht();
    Serial.println("Done");
}

void loop()
{
    
}

#define timeout 1000 //in us
//#define 
boolean get_dht()
{
    Serial.print("Reading DHT...");
    //Request data
    pinMode(dht, OUTPUT);
    digitalWrite(dht, LOW);
    delay(1);
    pinMode(dht, INPUT);
    
    //Decode
    ulong last_t = micros();
    ulong c_t = last_t;
    boolean last_s = false;
    uint timing[5*8*2+10];
    uint pos = 0;
    cli();
    while(micros() - c_t < 10000)
    {
        if(last_s != digitalRead(dht))
        {
            c_t = micros();
            //Serial.print(last_s);
            //Serial.print(", ");
            //Serial.println(c_t - last_t);
            timing[pos] = c_t - last_t;
            pos++;
            last_s = !last_s;
            last_t = c_t;
        }
        yield();
    }
    sei();
    
    for(uint i = 0; i < 5*8*2+10; i++)
    {
        Serial.println(timing[i]);
    }
    
    Serial.println("Done");
    return true;
}