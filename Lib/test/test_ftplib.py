import socket
import threading
import ftplib
import time

from unittest import TestCase
from test import test_support

HOST = test_support.HOST

# This function sets the evt 3 times:
#  1) when the connection is ready to be accepted.
#  2) when it is safe for the caller to close the connection
#  3) when we have closed the socket
def server(evt, serv):
    serv.listen(5)
    # (1) Signal the caller that we are ready to accept the connection.
    evt.set()
    try:
        conn, addr = serv.accept()
    except socket.timeout:
        pass
    else:
        conn.send("1 Hola mundo\n")
        # (2) Signal the caller that it is safe to close the socket.
        evt.set()
        conn.close()
    finally:
        serv.close()
        # (3) Signal the caller that we are done.
        evt.set()

class GeneralTests(TestCase):

    def setUp(self):
        self.evt = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(3)
        self.port = test_support.bind_port(self.sock)
        threading.Thread(target=server, args=(self.evt,self.sock)).start()
        # Wait for the server to be ready.
        self.evt.wait()
        self.evt.clear()
        ftplib.FTP.port = self.port

    def tearDown(self):
        self.evt.wait()

    def testBasic(self):
        # do nothing
        ftplib.FTP()

        # connects
        ftp = ftplib.FTP(HOST)
        self.evt.wait()
        ftp.close()

    def testTimeoutDefault(self):
        # default -- use global socket timeout
        self.assert_(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            ftp = ftplib.FTP("localhost")
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()

    def testTimeoutNone(self):
        # no timeout -- do not use global socket timeout
        self.assert_(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            ftp = ftplib.FTP("localhost", timeout=None)
        finally:
            socket.setdefaulttimeout(None)
        self.assertTrue(ftp.sock.gettimeout() is None)
        self.evt.wait()
        ftp.close()

    def testTimeoutValue(self):
        # a value
        ftp = ftplib.FTP(HOST, timeout=30)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()

    def testTimeoutConnect(self):
        ftp = ftplib.FTP()
        ftp.connect(HOST, timeout=30)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()

    def testTimeoutDifferentOrder(self):
        ftp = ftplib.FTP(timeout=30)
        ftp.connect(HOST)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()

    def testTimeoutDirectAccess(self):
        ftp = ftplib.FTP()
        ftp.timeout = 30
        ftp.connect(HOST)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()


def test_main(verbose=None):
    test_support.run_unittest(GeneralTests)

if __name__ == '__main__':
    test_main()
