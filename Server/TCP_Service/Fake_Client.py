#Python2.7

import socket

def client():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect(('localhost',8165))
        m = raw_input("Data to send, or q to exit:").strip()
        while(m != "q"):
            sock.sendall(m)
            m = raw_input("Data to send, or q to exit:").strip()
    finally:
        sock.close()