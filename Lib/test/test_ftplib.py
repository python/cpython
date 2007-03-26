import socket
import threading
import ftplib
import time

from unittest import TestCase
from test import test_support

def server(evt):
    serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    serv.bind(("", 9091))
    serv.listen(5)
    conn, addr = serv.accept()
    conn.send("1 Hola mundo\n")
    conn.close()
    serv.close()
    evt.set()

class GeneralTests(TestCase):
    
    def setUp(self):
        ftplib.FTP.port = 9091
        self.evt = threading.Event()
        threading.Thread(target=server, args=(self.evt,)).start()
        time.sleep(.1)

    def tearDown(self):
        self.evt.wait()

    def testBasic(self):
        # do nothing
        ftplib.FTP()

        # connects
        ftp = ftplib.FTP("localhost")
        ftp.sock.close()
        
    def testTimeoutDefault(self):
        # default
        ftp = ftplib.FTP("localhost")
        self.assertTrue(ftp.sock.gettimeout() is None)
        ftp.sock.close()
    
    def testTimeoutValue(self):
        # a value
        ftp = ftplib.FTP("localhost", timeout=30)
        self.assertEqual(ftp.sock.gettimeout(), 30)
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
        ftp.close()



def test_main(verbose=None):
    test_support.run_unittest(GeneralTests)

if __name__ == '__main__':
    test_main()
