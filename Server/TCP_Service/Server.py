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

__print_lock = threading.Lock()
def sp(*args): #Thread safe print
    with __print_lock:
        print ' '.join([str(e) for e in args])

def handler(client, addr):
    n = str( threading.current_thread().name ) + " ::" #name
    sp( n, "Connection initiated from " + str(addr) + "." )
    try:
        active = True
        while active:
            m = client.recv(1024)
            active = m != ''
            if active:
                sp( n, "Data recv:", m )
        
    except socket.timeout as e:
        sp( n, "Timeout occurred." )
    except socket.error as e:
        tb = ''.join( traceback.format_exception() )
        sp( n, "Unspecified error occurred.\n" + tb )
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