# test asynchat -- requires threading

import thread # If this fails, we can't test this module
import asyncore, asynchat, socket, threading, time

HOST = "127.0.0.1"
PORT = 54321

class echo_server(threading.Thread):

    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((HOST, PORT))
        sock.listen(1)
        conn, client = sock.accept()
        buffer = ""
        while "\n" not in buffer:
            data = conn.recv(10)
            if not data:
                break
            buffer = buffer + data
        while buffer:
            n = conn.send(buffer)
            buffer = buffer[n:]
        conn.close()
        sock.close()

class echo_client(asynchat.async_chat):

    def __init__(self):
        asynchat.async_chat.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect((HOST, PORT))
        self.set_terminator("\n")
        self.buffer = ""

    def handle_connect(self):
        print "Connected"

    def collect_incoming_data(self, data):
        self.buffer = self.buffer + data

    def found_terminator(self):
        print "Received:", `self.buffer`
        self.buffer = ""
        self.close()

def main():
    s = echo_server()
    s.start()
    time.sleep(1) # Give server time to initialize
    c = echo_client()
    c.push("hello ")
    c.push("world\n")
    asyncore.loop()

main()
