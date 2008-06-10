import socket
import threading
import poplib
import time

from unittest import TestCase
from test import support

HOST = support.HOST

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
        self.port = support.bind_port(self.sock)
        threading.Thread(target=server, args=(self.evt,self.sock)).start()
        time.sleep(.1)

    def tearDown(self):
        self.evt.wait()

    def testBasic(self):
        # connects
        pop = poplib.POP3(HOST, self.port)
        pop.sock.close()

    def testTimeoutDefault(self):
        self.assertTrue(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            pop = poplib.POP3("localhost", self.port)
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(pop.sock.gettimeout(), 30)
        pop.sock.close()

    def testTimeoutNone(self):
        self.assertTrue(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            pop = poplib.POP3(HOST, self.port, timeout=None)
        finally:
            socket.setdefaulttimeout(None)
        self.assertTrue(pop.sock.gettimeout() is None)
        pop.sock.close()

    def testTimeoutValue(self):
        pop = poplib.POP3("localhost", self.port, timeout=30)
        self.assertEqual(pop.sock.gettimeout(), 30)
        pop.sock.close()


def test_main(verbose=None):
    support.run_unittest(GeneralTests)

if __name__ == '__main__':
    test_main()
