import socket
import threading
import poplib
import time

from unittest import TestCase
from test import test_support


def server(ready, evt):
    serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv.settimeout(3)
    serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    serv.bind(("", 9091))
    serv.listen(5)
    ready.set()
    try:
        conn, addr = serv.accept()
    except socket.timeout:
        pass
    else:
        conn.send("+ Hola mundo\n")
        conn.close()
    finally:
        serv.close()
        evt.set()

class GeneralTests(TestCase):

    def setUp(self):
        self.evt = threading.Event()
        self.ready = threading.Event()
        threading.Thread(target=server, args=(self.ready, self.evt,)).start()
        self.ready.wait()

    def tearDown(self):
        self.evt.wait()

    def testBasic(self):
        # connects
        pop = poplib.POP3("localhost", 9091)
        pop.sock.close()

    def testTimeoutDefault(self):
        # default
        pop = poplib.POP3("localhost", 9091)
        self.assertTrue(pop.sock.gettimeout() is None)
        pop.sock.close()

    def testTimeoutValue(self):
        # a value
        pop = poplib.POP3("localhost", 9091, timeout=30)
        self.assertEqual(pop.sock.gettimeout(), 30)
        pop.sock.close()

    def testTimeoutNone(self):
        # None, having other default
        previous = socket.getdefaulttimeout()
        socket.setdefaulttimeout(30)
        try:
            pop = poplib.POP3("localhost", 9091, timeout=None)
        finally:
            socket.setdefaulttimeout(previous)
        self.assertEqual(pop.sock.gettimeout(), 30)
        pop.sock.close()



def test_main(verbose=None):
    test_support.run_unittest(GeneralTests)

if __name__ == '__main__':
    test_main()
