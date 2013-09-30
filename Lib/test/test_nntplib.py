import socket
import threading
import nntplib
import time

from unittest import TestCase
from test import test_support

HOST = test_support.HOST


def server(evt, serv, evil=False):
    serv.listen(5)
    try:
        conn, addr = serv.accept()
    except socket.timeout:
        pass
    else:
        if evil:
            conn.send("1 I'm too long response" * 3000 + "\n")
        else:
            conn.send("1 I'm OK response\n")
        conn.close()
    finally:
        serv.close()
        evt.set()


class BaseServerTest(TestCase):
    def setUp(self):
        self.evt = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(3)
        self.port = test_support.bind_port(self.sock)
        threading.Thread(
            target=server,
            args=(self.evt, self.sock, self.evil)).start()
        time.sleep(.1)

    def tearDown(self):
        self.evt.wait()


class ServerTests(BaseServerTest):
    evil = False

    def test_basic_connect(self):
        nntp = nntplib.NNTP('localhost', self.port)
        nntp.sock.close()


class EvilServerTests(BaseServerTest):
    evil = True

    def test_too_long_line(self):
        self.assertRaises(nntplib.NNTPDataError,
                          nntplib.NNTP, 'localhost', self.port)


def test_main(verbose=None):
    test_support.run_unittest(EvilServerTests)
    test_support.run_unittest(ServerTests)

if __name__ == '__main__':
    test_main()
