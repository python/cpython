import socket
import threading
import ftplib
import time

from unittest import TestCase
from test import test_support

server_port = None

# This function sets the evt 3 times:
#  1) when the connection is ready to be accepted.
#  2) when it is safe for the caller to close the connection
#  3) when we have closed the socket
def server(evt):
    global server_port
    serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv.settimeout(3)
    serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_port = test_support.bind_port(serv, "", 9091)
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
        threading.Thread(target=server, args=(self.evt,)).start()
        # Wait for the server to be ready.
        self.evt.wait()
        self.evt.clear()
        ftplib.FTP.port = server_port

    def tearDown(self):
        # Wait on the closing of the socket (this shouldn't be necessary).
        self.evt.wait()

    def testBasic(self):
        # do nothing
        ftplib.FTP()

        # connects
        ftp = ftplib.FTP("localhost")
        self.evt.wait()
        ftp.sock.close()

    def testTimeoutDefault(self):
        # default
        ftp = ftplib.FTP("localhost")
        self.assertTrue(ftp.sock.gettimeout() is None)
        self.evt.wait()
        ftp.sock.close()

    def testTimeoutValue(self):
        # a value
        ftp = ftplib.FTP("localhost", timeout=30)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.sock.close()

    def testTimeoutConnect(self):
        ftp = ftplib.FTP()
        ftp.connect("localhost", timeout=30)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.sock.close()

    def testTimeoutDifferentOrder(self):
        ftp = ftplib.FTP(timeout=30)
        ftp.connect("localhost")
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.sock.close()

    def testTimeoutDirectAccess(self):
        ftp = ftplib.FTP()
        ftp.timeout = 30
        ftp.connect("localhost")
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.sock.close()

    def testTimeoutNone(self):
        # None, having other default
        previous = socket.getdefaulttimeout()
        socket.setdefaulttimeout(30)
        try:
            ftp = ftplib.FTP("localhost", timeout=None)
        finally:
            socket.setdefaulttimeout(previous)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()


def test_main(verbose=None):
    test_support.run_unittest(GeneralTests)

if __name__ == '__main__':
    test_main()
