//For the local wifi gateway
#define ssid "Your-wifi-network"
#define pass "Its-password"

//For the remote TCP server gateway
#define host "192.168.1.99"
#define port 8165
const byte start_handshake[] = {0xAB, 0x80, 0xCD, 0x11, 0x47, 0x0C};
#define good_handshake 0x76
#define good_response 0xAB

//Connection behaviour
#define retry_delay_base 1 //Seconds
#define retry_delay_multiplier 2
#define retry_delay_max 20*60 //Seconds
#define tcp_timeout 10*1000 //ms

#define sample_interval 1000*5 //ms
#define report_interval 1000*10*60 //ms, 10 minutes
#define report_queue_size 1000 //Total samples