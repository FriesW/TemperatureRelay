#Python2.7
import socket
import threading
#import time
import traceback
import thread

from config import *

def s_to_hex(s): #string to hex string
	return "0x " + ' '.join([hex(ord(a))[2:].zfill(2) for a in s])
def int_to_s(n): #int (hex) to string
    o = ''
    while n != 0:
        o += chr( n & 0xFF )
        n = n >> 8
    return o[::-1] #reverse
def s_to_int(s): #string to int (hex)
    o = 0
    for c in s:
        o = o << 8
        o += ord(c)
    return o
def int_to_hex(n): #int to hex string, not that efficient...
    return s_to_hex( int_to_s(n) )

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
        #Handshake
        m = ''
        sh = int_to_s(START_HANDSHAKE)
        while len(m) != len(sh):
            m += client.recv(1024)
        if m != sh:
            active = False
            sp(n, "Improper handshake.")
        else:
            sp(n, "Good handshake.")
            client.sendall( int_to_s(GOOD_HANDSHAKE) )
        
        #Receive data loop
        while active:
            m = ''
            while len(m) < 2:
                m += client.recv(1024)
            sp( n, "Data recv: ", s_to_hex(m) )
            #Unpack data
            temps = []
            for i in range(0, len(m)-1, 2):
                ub = ord(m[i])
                lb = ord(m[i+1])
                t = (ub & 0x7F) + lb
                if ub >> 7 == 1: t *= -1
                temps.append(str(t))
            sp(n, "Temps:", 'deciC '.join(temps) + 'deciC' )
            #Submit data for POST
            
            
            #On success of all, send good response
            client.sendall( int_to_s(GOOD_RESPONSE) )
        
    except socket.timeout as e:
        sp( n, "Timeout occurred." )
    except socket.error as e:
        tb = traceback.format_exc()
        sp( n, "Unspecified networking error occurred.\n", tb )
    except:
        tb = traceback.format_exc()
        sp( n, "Unspecified error occurred.\n", tb )
    finally:
        try:
            client.shutdown(socket.SHUT_RDWR)
        except:
            pass
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