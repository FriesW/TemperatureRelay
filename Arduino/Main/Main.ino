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

#define start_wait 10 //Seconds

#define dht 2

void setup()
{
    Serial.begin(115200);
    pinMode(dht, INPUT);
    
    //Delay, so if someone is listening they capture the beginning
    Serial.println();
    for(byte i = start_wait; i > 0; i--)
    {
        delay(1000);
        Serial.print("Startup wait ");
        Serial.print(i);
        Serial.println("...");
    }
    Serial.println("Done");
    
    WiFi.begin(ssid, pass);
    check_connection();
}

void loop()
{
    //check_connection();
}

void check_connection()
{
    static boolean first = true;
    if(!first && WiFi.status() == WL_CONNECTED)
        return;
    ulong counter = 0;
    if(!first)
        Serial.println("WiFi connection lost.");
    Serial.print(first ? "Connecting to '" : "Reconnecting to '");
    Serial.print(ssid);
    Serial.print("' -> ");
    while(WiFi.status() != WL_CONNECTED)
    {
        counter++;
        if(counter % (2*10) == 0)
        {
            Serial.println();
            Serial.print("Attempting to ");
            Serial.print(first ? "connect to '" : "reconnect to '");
            Serial.print(ssid);
            Serial.print("' for ");
            Serial.print(counter / 2);
            Serial.print("seconds -> ");
        }
        Serial.print(".");
        delay(500);
    }
    Serial.println(" -> Connected.");
    Serial.print("Connection took ");
    Serial.print(counter / 2);
    Serial.println(" seconds.");
    Serial.print("Local IP address: ");
    Serial.println(WiFi.localIP());
    
    first = false;
}

#define timeout 1000 //in us
#define threshold 50 //in us
boolean get_dht(uint &temperature, uint &humidity) //TODO make sure DHT timeout is honored
{
    Serial.println("Reading DHT");
    //Request data
    pinMode(dht, OUTPUT);
    digitalWrite(dht, LOW);
    delay(1);
    cli();
    pinMode(dht, INPUT);
    delayMicroseconds(35); //Miss first super short pulse
    
    //Setup
    ulong last_t = micros();
    boolean last_s = false;
    boolean no_timeout = true;
    byte data[5];
    //uint timing[5*8+10]; //Debug
    uint bit_pos = 0;
    //Catch first bit
    while( !digitalRead(dht) && (no_timeout = micros() - last_t < timeout) );
    if(no_timeout) //Update time only if it didn't timeout
        last_t = micros();
    while( digitalRead(dht) && (no_timeout = micros() - last_t < timeout) );
    if(no_timeout) //Update time only if it didn't timeout
        last_t = micros();
    //Decode remaining 40 bits
    while( (bit_pos < 40) && (no_timeout = micros() - last_t < timeout) )
    {
        if(last_s != digitalRead(dht))
        {
            ulong c_t = micros();
            //If this pulse was high
            if(last_s)
            {
                byte i = bit_pos / 8;
                data[i] = data[i] << 1;
                data[i] += c_t - last_t > threshold;
                //timing[bit_pos] = c_t - last_t; //Debug
                bit_pos++;
            }
            last_t = c_t;
            last_s = !last_s;
        }
        //yield();
    }
    sei();
    
    if(!no_timeout)
    {
        Serial.println("Sensor timed out.");
        Serial.println("Reading DHT Done");
        return false;
    }
    
    /*for(uint i = 0; i < 5*8+10; i++) //Debug
        Serial.println(timing[i]);*/
    
    //Debug out
    char hex_out[2*5+2+1];
    Serial.print("Bits received: 0x:");
    sprintf(hex_out, "%02X%02X:%02X%02X:%02X", data[0], data[1], data[2], data[3], data[4]);
    hex_out[2*5+2] = 0; //Null at end of string
    Serial.println(hex_out);
    
    //Unpack
    byte check = data[4];
    byte t2 = data[3];
    byte t1 = data[2];
    byte h2 = data[1];
    byte h1 = data[0];
    
    //Checksum
    byte calc_check = t2 + t1 + h2 + h1;
    Serial.print("Checksum ");
    if(calc_check == check)
    {
        Serial.println("good.");
    }
    else
    {
        Serial.println("bad.");
        Serial.println("Reading DHT Done");
        return false;
    }
    
    //Assign
    temperature = t1;
    humidity = h1;
    temperature = (temperature << 8) + t2;
    humidity = (humidity << 8) + h2;
    
    Serial.print("Relative Humidity - ");
    Serial.print(humidity / 10);
    Serial.print(".");
    Serial.print(humidity % 10);
    Serial.print("%   Temperature - ");
    Serial.print(temperature / 10);
    Serial.print(".");
    Serial.print(temperature % 10);
    Serial.println("C");
    Serial.println("Reading DHT Done");
    
    return true;
}