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

#define start_wait 10 //Seconds

#define dht 2



ulong last_sample;
ulong last_report;
byte report_queue[report_queue_size * 2];
uint report_queue_pos = 0; //Points to first empty spot
long sample_sum = 0;
long total_samples = 0;


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
            sample_sum += temp;
            total_samples ++;
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
        int avg = sample_sum / total_samples;
        report_queue[report_queue_pos++] = avg >> 8;
        report_queue[report_queue_pos++] = avg; //& 0xFF <- implicit
        //Reset sample
        sample_sum = 0;
        total_samples = 0;
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

#define print_space 5000 //ms between printing
void check_wifi()
{
    static boolean first = true;
    static boolean last_status = false; //false if not connected, true if connected
    static long conn_start = millis() / 1000; //in seconds
    static ulong last_print = millis();
    
    boolean this_status = WiFi.status() == WL_CONNECTED;
    
    //If not first and ( wifi is connected or not time for print )
    if( !first &&
        ((last_status && this_status) || millis() - last_print < print_space) )
        return;
    
    //Previously connected, but isn't any-more
    if(last_status && !this_status)
        conn_start = millis() / 1000;
    
    Serial.println();
    if(!first)
        Serial.print("WiFi connection lost. ");
    Serial.print(first ? "Connecting to '" : "Reconnecting to '");
    Serial.print(ssid);
    Serial.print("'....");
    
    //Previously disconnected, but now is
    if(!last_status && this_status)
    {
        Serial.println(" -> Connected.");
        Serial.print("Connection took ");
        Serial.print( millis() / 1000 - conn_start ); //Not perfect, prints in multiples of print_space
        Serial.println(" seconds.");
        Serial.print("Local IP address: ");
        Serial.print(WiFi.localIP());
    }
    
    Serial.println();
    last_print = millis();
    last_status = this_status;
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








WiFiClient _client;
boolean _first = true;
boolean _this_status = false; //really last_
ulong _this_attempt; //really last_
uint _connection_attempts = 0;

static boolean _wait_for_data()
{
    ulong start = millis();
    boolean timeout = false;
    while( !_client.available() && !(timeout = millis() - start > tcp_timeout)) yield();
    if(timeout)
        Serial.print("Timeout occurred waiting for data. ");
    return timeout;
}

static ulong _get_delay_time()
{
    ulong delay_time = 0;
    if(_connection_attempts)
    {
        delay_time = retry_delay_base * _longpow(retry_delay_multiplier, _connection_attempts - 1);
            if(delay_time > retry_delay_max)
                delay_time = retry_delay_max;
    }
    return delay_time;
}

static boolean _failure()
{
    //Stop client
    _client.stop();
    //Record failure
    if(_get_delay_time() != retry_delay_max)
        _connection_attempts++;
    //Print debug
    Serial.println("Failure.");
    Serial.print("Will try again after ");
    Serial.print(_get_delay_time());
    Serial.println(" seconds have passed.");
    _this_attempt = millis();
    return false;
}

static byte _print_byte(byte b)
{
    char hex_out[3];
    hex_out[2] = 0;
    sprintf(hex_out, "%02X", b);
    Serial.print(hex_out);
    return b;
}

static ulong _longpow(ulong x, ulong y)
{
    ulong o = 1;
    for(ulong i = 0; i < y; i++)
        o *= x;
    return o;
}

boolean tcp_send(const byte data[], uint length)
{
    //Shift status
    boolean last_status = _this_status;
    ulong last_attempt = _this_attempt;
    
    //Check for timeout
    if(_connection_attempts) //Not necessary, but nice to have
    {
        //Get delay time
        ulong delay_time = _get_delay_time();
        //Has enough time elapsed?
        if( millis() - last_attempt < delay_time*1000 )
            return false;
    }
    
    //Here forward, something WILL print, so print block
    Serial.println();
    
    //Update status
    _this_status = _client.status() == ESTABLISHED;
    _this_attempt = millis();
    
    //If not connected...
    if(!_this_status)
    {
        
        //Attempt connection
        Serial.print(_first ? "Establishing connection to " : "Connection failed. Reconnecting to ");
        Serial.print(host);
        Serial.print(":");
        Serial.print(port);
        Serial.print("...");
        _client.setTimeout(tcp_timeout);
        if( !_client.connect(host, port) )
            return _failure();
        else
        {
            Serial.println("Success.");
            _first = false;
        }
        
        //Handshake
        Serial.print("Handshake...");
        if( !_client.write(start_handshake, sizeof(start_handshake)) )
            return _failure();
        if(_wait_for_data())
            return _failure();
        //Get response
        Serial.print("Received 0x");
        byte resp = _print_byte(_client.read());
        Serial.print(" -> ");
        if(resp != good_handshake)
            return _failure();
        Serial.println("Success.");
        
    }
    
    //Display data
    Serial.print("Sending bits: 0x");
    for(uint i = 0; i < length; i++)
        _print_byte(data[i]);
    Serial.print(" ");
    
    //Send data
    if( !_client.write(data, length) )
        return _failure();
    Serial.println();
    
    //Wait for response
    if(_wait_for_data())
        return _failure();
    
    //Echo response
    byte last;
    Serial.print("Received: 0x");
    while(_client.available())
        last = _print_byte(_client.read());
    
    //Check response
    Serial.print(" -> Last bit signals ");
    Serial.print( last == good_response ? "good" : "bad");
    Serial.print(" response. ");
    if(last != good_response)
        return _failure();
    Serial.println();
    
    //Total success! Reset...
    _connection_attempts = 0;
    return true;
}