import socket
import threading
import smtplib
import time

from unittest import TestCase
from test import test_support


def server(evt):
    serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv.settimeout(3)
    serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    serv.bind(("", 9091))
    serv.listen(5)
    try:
        conn, addr = serv.accept()
    except socket.timeout:
        pass
    else:
        conn.send("220 Hola mundo\n")
        conn.close()
    finally:
        serv.close()
        evt.set()

class GeneralTests(TestCase):
    
    def setUp(self):
        self.evt = threading.Event()
        threading.Thread(target=server, args=(self.evt,)).start()
        time.sleep(.1)

    def tearDown(self):
        self.evt.wait()

    def testBasic(self):
        # connects
        smtp = smtplib.SMTP("localhost", 9091)
        smtp.sock.close()
        
    def testTimeoutDefault(self):
        # default
        smtp = smtplib.SMTP("localhost", 9091)
        self.assertTrue(smtp.sock.gettimeout() is None)
        smtp.sock.close()
    
    def testTimeoutValue(self):
        # a value
        smtp = smtplib.SMTP("localhost", 9091, timeout=30)
        self.assertEqual(smtp.sock.gettimeout(), 30)
        smtp.sock.close()

    def testTimeoutNone(self):
        # None, having other default
        previous = socket.getdefaulttimeout()
        socket.setdefaulttimeout(30)
        try:
            smtp = smtplib.SMTP("localhost", 9091, timeout=None)
        finally:
            socket.setdefaulttimeout(previous)
        self.assertEqual(smtp.sock.gettimeout(), 30)
        smtp.sock.close()



def test_main(verbose=None):
    test_support.run_unittest(GeneralTests)

if __name__ == '__main__':
    test_main()
