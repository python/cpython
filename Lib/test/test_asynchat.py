# test asynchat -- requires threading

import thread # If this fails, we can't test this module
import asyncore, asynchat, socket, threading, time
import unittest
from test import test_support

HOST = "127.0.0.1"
PORT = 54322

class echo_server(threading.Thread):

    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        global PORT
        PORT = test_support.bind_port(sock, HOST, PORT)
        sock.listen(1)
        conn, client = sock.accept()
        buffer = ""
        while "\n" not in buffer:
            data = conn.recv(1)
            if not data:
                break
            buffer = buffer + data
        while buffer:
            n = conn.send(buffer)
            buffer = buffer[n:]
        conn.close()
        sock.close()

class echo_client(asynchat.async_chat):

    def __init__(self, terminator):
        asynchat.async_chat.__init__(self)
        self.contents = None
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect((HOST, PORT))
        self.set_terminator(terminator)
        self.buffer = ""

    def handle_connect(self):
        pass
        ##print "Connected"

    def collect_incoming_data(self, data):
        self.buffer = self.buffer + data

    def found_terminator(self):
        #print "Received:", repr(self.buffer)
        self.contents = self.buffer
        self.buffer = ""
        self.close()


class TestAsynchat(unittest.TestCase):
    def setUp (self):
        pass

    def tearDown (self):
        pass

    def test_line_terminator(self):
        s = echo_server()
        s.start()
        time.sleep(1) # Give server time to initialize
        c = echo_client('\n')
        c.push("hello ")
        c.push("world\n")
        asyncore.loop()
        s.join()

        self.assertEqual(c.contents, 'hello world')

    def test_numeric_terminator(self):
        # Try reading a fixed number of bytes
        s = echo_server()
        s.start()
        time.sleep(1) # Give server time to initialize
        c = echo_client(6L)
        c.push("hello ")
        c.push("world\n")
        asyncore.loop()
        s.join()

        self.assertEqual(c.contents, 'hello ')


def test_main(verbose=None):
    test_support.run_unittest(TestAsynchat)

if __name__ == "__main__":
    test_main(verbose=True)
