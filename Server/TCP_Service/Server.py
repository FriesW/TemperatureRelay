#Python2.7
import socket
import threading
#import time
import traceback
import thread

from config import *

class BadHandshake(Exception):
    def __init__(self,*args,**kwargs):
        Exception.__init__(self,*args,**kwargs)
        
class RecvClosed(Exception):
    def __init__(self,*args,**kwargs):
        Exception.__init__(self,*args,**kwargs)

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



max_threads = threading.Semaphore(MAX_THREADS)


def handler(client, addr):
    
    def get(length):
        out = ''
        while len(out) != length:
            new = client.recv(1024)
            if len(new) == 0:
                raise RecvClosed()
            out += new
        return out
        
    n = str( threading.current_thread().name ) + " :: " #name
    sp( n, "Connection initiated from ", addr, "." )
    try:
        #Handshake
        sh = int_to_s(START_HANDSHAKE)
        m = get(len(sh))
        if m != sh:
            raise BadHandshake()
        sp(n, "Good handshake.")
        client.sendall( int_to_s(GOOD_HANDSHAKE) )
        
        #Receive data loop
        while True:
            m = get(2)
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
    except BadHandshake as e:
        sp(n, "Improper handshake.")
    except RecvClosed as e:
        sp(n, "Remote client closed its output.")
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
        max_threads.release()
        sp( n, "Connection closed." )

def run():
    n = "MAIN :: "
    sp( n, "Starting server..." )
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    addr = (HOST,PORT)
    server.bind(addr)
    server.listen(MAX_TCP_QUEUE)
    sp( n, "Running. Listening on " + str(addr) + "." )
    try:
        while True:
            max_threads.acquire() #If max is reached, then wait
            client, addr = server.accept()
            sp( n, "Dispatching thread. Approximately ", max_threads._Semaphore__value, " threads left.")
            client.settimeout(TIMEOUT)
            thread.start_new_thread(handler, (client, addr))
    finally:
        sp( n, "Cleaning up server." )
        server.shutdown(socket.SHUT_RDWR)
        server.close()

run()