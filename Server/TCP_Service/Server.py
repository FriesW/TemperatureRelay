#Python2.7
import socket
import threading
#import time
import traceback
import thread

#TIMEOUT = 60*30 #Half an hour
TIMEOUT = 10
HOST = '0.0.0.0'
PORT = 8165

def s_hex(s):
	return "0x " + ' '.join([hex(ord(a))[2:].zfill(2) for a in s])

__print_lock = threading.Lock()
def sp(*args): #Thread safe print
    m = ''.join([str(e) for e in args])
    with __print_lock:
        print m

def handler(client, addr):
    n = str( threading.current_thread().name ) + " :: " #name
    sp( n, "Connection initiated from ", addr, "." )
    try:
        active = True
        while active:
            m = client.recv(1024)
            active = m != ''
            if active:
                sp( n, "Data recv: ", s_hex(m) )
            client.sendall(0xAB)
        
    except socket.timeout as e:
        sp( n, "Timeout occurred." )
    except socket.error as e:
        tb = traceback.format_exc()
        sp( n, "Unspecified networking error occurred.\n", tb )
    except:
        tb = traceback.format_exc()
        sp( n, "Unspecified error occurred.\n", tb )
    finally:
        client.shutdown(socket.SHUT_RDWR)
        client.close()
        sp( n, "Connection closed." )

def run():
    sp( "Starting server..." )
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    addr = (HOST,PORT)
    server.bind(addr)
    server.listen(5)
    sp( "Running. Listening on " + str(addr) + "." )
    try:
        while True:
            client, addr = server.accept()
            client.settimeout(TIMEOUT)
            thread.start_new_thread(handler, (client, addr))
    finally:
        sp( "Cleaning up server." )
        server.shutdown(socket.SHUT_RDWR)
        server.close()