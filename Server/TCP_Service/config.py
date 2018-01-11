#Server Settings
HOST = '0.0.0.0'
PORT = 8165
START_HANDSHAKE = 0xAB80CD11470C
GOOD_HANDSHAKE = 0x76
GOOD_RESPONSE = 0xAB
BAD_RESPONSE = 0xAA

#Connection behaviour
TIMEOUT = 60*30 #Seconds, half an hour
MAX_THREADS = 10
MAX_TCP_QUEUE = 2

#Remote post
URL = 'posttestserver.com'
PATH = '/post.php'
LOCAL_PASSWORD = 'testpass'