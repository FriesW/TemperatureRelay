#Python2.7
import SocketServer
import socket
import threading
import time
import traceback

TIMEOUT = 60*30 #Half an hour

class ThreadedHandler(SocketServer.BaseRequestHandler):
    def __init__(self, *args):
        self.tn = str( threading.current_thread().name ) #thread name
        #Issues with calling super: https://mail.python.org/pipermail/python-list/2012-May/624418.html
        #super(ThreadedHandler, self).__init__(*args)
        SocketServer.BaseRequestHandler.__init__(self, *args)
        print self.tn + " started."
    
    def pp(self, *text):
        print self.tn + " :: " + " ".join(text)

    def handle(self):
        self.pp("Connection started.")
        self.request.settimeout(TIMEOUT)
        try:
            while True:
                m = self.request.recv(2)
                if( m != ''):
                    self.pp("Data recv:", m)
        except socket.timeout as e:
            self.pp("Timeout occurred.")
        except socket.error as e:
            tb = ''.join( traceback.format_exception() )
            self.pp("Unspecified error occurred.\n" + tb) #'Atomic' print, don't know if it actually is
        finally:
            self.pp("Connection closed.")
            self.request.close()
            
        
        #i = 0
        #while(True):
        #    self.request.sendall(str(i))
        #    i += 1
        #    sleep(10)

class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass

def run():
    server = ThreadedTCPServer(('0.0.0.0',8165), ThreadedHandler)
    try:
        server.serve_forever()
    except:
        print "Cleaning up server"
        server.server_close()