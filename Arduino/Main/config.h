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
#define max_retries 3
#define retry_delay_base 1 //Seconds
#define retry_delay_multiplier 3
#define tcp_timeout 5000 //ms