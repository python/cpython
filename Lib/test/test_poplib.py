import os
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
        conn.send("+ Hola mundo\n")
        conn.close()
    finally:
        serv.close()
        evt.set()


def evil_server(evt, serv, use_ssl=False):
    serv.listen(5)
    try:
        conn, addr = serv.accept()
        if use_ssl:
            conn = ssl.wrap_socket(
                conn,
                server_side=True,
                certfile=CERTFILE,
            )
    except socket.timeout:
        pass
    else:
        if use_ssl:
            try:
                conn.do_handshake()
            except ssl.SSLError, err:
                if err.args[0] not in (ssl.SSL_ERROR_WANT_READ,
                                       ssl.SSL_ERROR_WANT_WRITE):
                    raise
        conn.send("+ Hola mundo" * 1000 + "\n")
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


class EvilServerTests(TestCase):
    use_ssl = False

    def setUp(self):
        self.evt = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(3)
        self.port = test_support.bind_port(self.sock)
        threading.Thread(
            target=evil_server,
            args=(self.evt, self.sock, self.use_ssl)).start()
        time.sleep(.1)

    def tearDown(self):
        self.evt.wait()

    def testTooLongLines(self):
        self.assertRaises(poplib.error_proto, poplib.POP3,
                          'localhost', self.port, timeout=30)


SUPPORTS_SSL = False

if hasattr(poplib, 'POP3_SSL'):
    import ssl

    SUPPORTS_SSL = True
    CERTFILE = os.path.join(os.path.dirname(__file__) or os.curdir,
                            "keycert.pem")

    class EvilSSLServerTests(EvilServerTests):
        use_ssl = True

        def testTooLongLines(self):
            self.assertRaises(poplib.error_proto, poplib.POP3_SSL,
                              'localhost', self.port)


def test_main(verbose=None):
    test_support.run_unittest(GeneralTests)
    test_support.run_unittest(EvilServerTests)

    if SUPPORTS_SSL:
        test_support.run_unittest(EvilSSLServerTests)

if __name__ == '__main__':
    test_main()
