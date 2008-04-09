import socket
import threading
import poplib
import time

from unittest import TestCase
from test import test_support

HOST = test_support.HOST

def server(evt, serv):
    serv.listen(5)
    try:
        conn, addr = serv.accept()
    except socket.timeout:
        pass
    else:
        conn.send(b"+ Hola mundo\n")
        conn.close()
    finally:
        serv.close()
        evt.set()

class GeneralTests(TestCase):

    def setUp(self):
        self.evt = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(3)
        self.port = test_support.bind_port(self.sock)
        threading.Thread(target=server, args=(self.evt,self.sock)).start()
        time.sleep(.1)

    def tearDown(self):
        self.evt.wait()

    def testBasic(self):
        # connects
        pop = poplib.POP3(HOST, self.port)
        pop.sock.close()

    def testTimeoutDefault(self):
        # default
        pop = poplib.POP3(HOST, self.port)
        self.assertTrue(pop.sock.gettimeout() is None)
        pop.sock.close()

    def testTimeoutValue(self):
        # a value
        pop = poplib.POP3(HOST, self.port, timeout=30)
        self.assertEqual(pop.sock.gettimeout(), 30)
        pop.sock.close()

    def testTimeoutNone(self):
        # None, having other default
        previous = socket.getdefaulttimeout()
        socket.setdefaulttimeout(30)
        try:
            pop = poplib.POP3(HOST, self.port, timeout=None)
        finally:
            socket.setdefaulttimeout(previous)
        self.assertEqual(pop.sock.gettimeout(), 30)
        pop.sock.close()



def test_main(verbose=None):
    test_support.run_unittest(GeneralTests)

if __name__ == '__main__':
    test_main()
