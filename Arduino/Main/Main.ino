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

#include "config.h"

#define ulong unsigned long
#define uint unsigned int
#define MAX_INT 32767

#define start_wait 10 //Seconds

#define dht 2



ulong last_sample;
ulong last_report;
byte report_queue[report_queue_size * 2];
uint report_queue_pos = 0; //Points to first empty spot
int sample = MAX_INT;


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
    check_wifi();
    
    last_sample = millis();
    last_report = millis();
}

void loop()
{
    check_wifi();
    ulong t = millis();
    
    //Check if time for a new sample
    if(t - last_sample > sample_interval)
    {
        int temp;
        int humid;
        if( get_dht(temp, humid) )
        {
            sample = sample < temp ? sample : temp; //Minimum
            //Increment timer
            uint c = 0;
            while(t - last_sample > sample_interval)
            {
                last_sample += sample_interval;
                c++;
            }
            if(c > 1)
                Serial.println("\nWarning: Sample missed! Something is taking too long, or the interval is too small.");
        }
    }
    
    //Check if time to report
    if(t - last_report > report_interval)
    {
        //If at end of queue
        if(report_queue_pos == sizeof(report_queue))
        {
            Serial.println("\nWarning: report queue has been overrun! Old samples will be lost.");
            //Shift array
            for(uint i = 0; i < sizeof(report_queue) - 2; i++)
                report_queue[i] = report_queue[i+2];
            report_queue_pos -= 2;
        }
        //Push sample
        report_queue[report_queue_pos++] = sample >> 8;
        report_queue[report_queue_pos++] = sample; //& 0xFF <- implicit
        //Reset sample
        sample = MAX_INT;
        //Increment timer
        uint c = 0;
        while(t - last_report > report_interval)
        {
            last_report += report_interval;
            c++;
        }
        if(c > 1)
            Serial.println("\nWarning: Report missed!  Something is taking too long, or the interval is too small.");
    }
    
    //Check if there is anything to report
    if( report_queue_pos != 0 )
    {
        if( tcp_send(report_queue, report_queue_pos) )
            report_queue_pos = 0;
    }
    
}

boolean tcp_send(const byte data[], uint length)
{
    static WiFiClient client;
    static boolean first = true;
    static boolean status = false; //true -> full, correct transaction
    static ulong last_attempt;
    static uint connection_attempts = 0; //Reset to 0 on success
    
    status = status && client.status() == ESTABLISHED;
    
    //Establish connection if it doesn't exist
    if(!status)
    {
        uint delay_time;
        //Special case: first failure
        if(connection_attempts == 0)
        {
            last_attempt = millis();
            delay_time = 0;
        }
        //After the first failure
        else
        {
            //Get delay time
            delay_time = retry_delay_base * intpow(retry_delay_multiplier, connection_attempts - 1);
            if(delay_time > retry_delay_max)
                delay_time = retry_delay_max;
        }
        //If not enough time has elapsed, then don't try connecting
        if( millis() - last_attempt < delay_time*1000 ) //Should always eval False when connection_attempts == 0
        {
            return false; //Need to wait longer until next attempt
        }
        else
        {
            //Delay has been waited, so reset vars and continue
            if(delay_time != retry_delay_max) //Prevent rollover, although unlikely
                connection_attempts++;
            last_attempt = millis();
            
            //Reconnect if status is false due to lost connection
            if(client.status() != ESTABLISHED)
            {
                Serial.println(); //Print block separator
                Serial.print(first ? "Establishing connection to " : "Connection failed. Reconnecting to ");
                Serial.print(host);
                Serial.print(":");
                Serial.print(port);
                Serial.print("...");
                
                if( client.connect(host, port) )
                {
                    Serial.println("Success.");
                    first = false;
                    connection_attempts = 0;
                }
                else
                {
                    Serial.println("Failure.");
                    Serial.print("Try again after ");
                    uint delay_time = retry_delay_base * intpow(retry_delay_multiplier, connection_attempts - 1); //New delay time
                    Serial.print(delay_time > retry_delay_max ? retry_delay_max : delay_time);
                    Serial.println(" seconds have passed.");
                    return false;
                }
                
                //Handshake
                Serial.print("Handshake...");
                if( !client.write(start_handshake, sizeof(start_handshake)) )
                {
                    Serial.println("Failure sending handshake.");
                    client.stop();
                    return false;
                }
                //Wait for response
                ulong start = millis();
                boolean timeout = false;
                while( !client.available() && !(timeout = millis() - start > tcp_timeout) ) yield();
                if(timeout)
                {
                    Serial.println("Response timed out.");
                    client.stop();
                    return false;
                }
                //Get response
                byte resp = client.read();
                Serial.print("Recieved 0x");
                char hex_out[3];
                hex_out[2] = 0;
                sprintf(hex_out, "%02X", resp);
                Serial.println(hex_out);
                if(resp != good_handshake)
                {
                    Serial.println("Bad response.");
                    client.stop();
                    return false;
                }
                Serial.println("Good response.");
        
            }
        }
        
    }
    else
        Serial.println(); //Print block separator
    
    //Send data
    Serial.print("Sending bits: 0x");
    char hex_out[3];
    hex_out[2] = 0;
    for(uint i = 0; i < length; i++)
    {
        sprintf(hex_out, "%02X", data[i]);
        Serial.print(hex_out);
    }
    Serial.println();
    if( !client.write(data, length) )
    {
        Serial.println("Failure sending data.");
        client.stop();
        //return tcp_send(data, length); //Could this recursively run away?
        return false;
    }
    
    //Wait for response
    ulong start = millis();
    boolean timeout = false;
    while( !client.available() && !(timeout = millis() - start > tcp_timeout) ) yield();
    if(timeout)
    {
        Serial.println("Response timeout. Data was probably not delivered.");
        client.stop();
        return false;
    }
    //Echo response
    //char hex_out[3]; //Reuse
    //hex_out[2] = 0;
    byte last;
    Serial.print("Received: 0x");
    while(client.available())
    {
        last = client.read();
        sprintf(hex_out, "%02X", last);
        Serial.print(hex_out);
    }
    Serial.println();
    Serial.print("Last bit signals ");
    Serial.print(last == good_response ? "good" : "bad");
    Serial.println(" response.");
    
    //Update status, return
    if(last == good_response && status == false)
        connection_attempts = 0;
    status = last == good_response;
    if(!status)
    {
        Serial.print("Try again after ");
        uint delay_time = retry_delay_base * intpow(retry_delay_multiplier, connection_attempts - 1);
        Serial.print(delay_time > retry_delay_max ? retry_delay_max : delay_time);
        Serial.println(" seconds have passed.");
    }
    return status;
}

#define settle_time 10000 //ms
void check_wifi()
{
    static boolean first = true;
    if(!first && WiFi.status() == WL_CONNECTED)
        return;
    Serial.println();
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
    
    Serial.print("Letting connection settle...");
    delay(settle_time); //effectively a yield, let the initial connection furry finish
    Serial.println("Done");
    
    first = false;
}

#define sig_timeout 1000 //in us
#define sense_timeout 2000 //in ms, via the docsheet
#define threshold 50 //in us
boolean get_dht(int &temperature, int &humidity)
{
    static boolean first = true;
    static ulong last_invoke = 0;
    
    Serial.println();
    Serial.println("Reading DHT");
    
    //Must have a minimum time between readings
    if(!first && millis() - last_invoke < sense_timeout)
    {
        Serial.println("Need more time between DHT readings!");
        return false;
    }
    first = false;
    last_invoke = millis();
    
    optimistic_yield(1000); //Next operation takes a long time, and could reset wdt without this
    
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
    while( !digitalRead(dht) && (no_timeout = micros() - last_t < sig_timeout) );
    if(no_timeout) //Update time only if it didn't timeout
        last_t = micros();
    while( digitalRead(dht) && (no_timeout = micros() - last_t < sig_timeout) );
    if(no_timeout) //Update time only if it didn't timeout
        last_t = micros();
    //Decode remaining 40 bits
    while( (bit_pos < 40) && (no_timeout = micros() - last_t < sig_timeout) )
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
    
    optimistic_yield(1000);
    
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
    temperature = t1 & 0x7F; //Remove MSB
    humidity = h1;
    temperature = (temperature << 8) + t2;
    humidity = (humidity << 8) + h2;
    if(t1 & 0x80) temperature *= -1; //If MSB is set, then negative
    
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

uint intpow(uint x, uint y)
{
    uint o = 1;
    for(uint i = 0; i < y; i++)
        o *= x;
    return o;
}