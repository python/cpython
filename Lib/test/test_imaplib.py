from test import support
# If we end up with a significant number of tests that don't require
# threading, this test module should be split.  Right now we skip
# them all if we don't have threading.
threading = support.import_module('threading')

from contextlib import contextmanager
import imaplib
import os.path
import socketserver
import time

from test.support import reap_threads, verbose, transient_internet
import unittest

try:
    import ssl
except ImportError:
    ssl = None

CERTFILE = None


class TestImaplib(unittest.TestCase):

    def test_Internaldate2tuple(self):
        tt = imaplib.Internaldate2tuple(
            b'25 (INTERNALDATE "01-Jan-1970 00:00:00 +0000")')
        self.assertEqual(time.mktime(tt), 0)
        tt = imaplib.Internaldate2tuple(
            b'25 (INTERNALDATE "01-Jan-1970 11:30:00 +1130")')
        self.assertEqual(time.mktime(tt), 0)
        tt = imaplib.Internaldate2tuple(
            b'25 (INTERNALDATE "31-Dec-1969 12:30:00 -1130")')
        self.assertEqual(time.mktime(tt), 0)

    def test_that_Time2Internaldate_returns_a_result(self):
        # We can check only that it successfully produces a result,
        # not the correctness of the result itself, since the result
        # depends on the timezone the machine is in.
        timevalues = [2000000000, 2000000000.0, time.localtime(2000000000),
                      '"18-May-2033 05:33:20 +0200"']

        for t in timevalues:
            imaplib.Time2Internaldate(t)


if ssl:

    class SecureTCPServer(socketserver.TCPServer):

        def get_request(self):
            newsocket, fromaddr = self.socket.accept()
            connstream = ssl.wrap_socket(newsocket,
                                         server_side=True,
                                         certfile=CERTFILE)
            return connstream, fromaddr

    IMAP4_SSL = imaplib.IMAP4_SSL

else:

    class SecureTCPServer:
        pass

    IMAP4_SSL = None


class SimpleIMAPHandler(socketserver.StreamRequestHandler):

    timeout = 1

    def _send(self, message):
        if verbose: print("SENT: %r" % message.strip())
        self.wfile.write(message)

    def handle(self):
        # Send a welcome message.
        self._send(b'* OK IMAP4rev1\r\n')
        while 1:
            # Gather up input until we receive a line terminator or we timeout.
            # Accumulate read(1) because it's simpler to handle the differences
            # between naked sockets and SSL sockets.
            line = b''
            while 1:
                try:
                    part = self.rfile.read(1)
                    if part == b'':
                        # Naked sockets return empty strings..
                        return
                    line += part
                except IOError:
                    # ..but SSLSockets throw exceptions.
                    return
                if line.endswith(b'\r\n'):
                    break

            if verbose: print('GOT: %r' % line.strip())
            splitline = line.split()
            tag = splitline[0].decode('ASCII')
            cmd = splitline[1].decode('ASCII')
            args = splitline[2:]

            if hasattr(self, 'cmd_'+cmd):
                getattr(self, 'cmd_'+cmd)(tag, args)
            else:
                self._send('{} BAD {} unknown\r\n'.format(tag, cmd).encode('ASCII'))

    def cmd_CAPABILITY(self, tag, args):
        self._send(b'* CAPABILITY IMAP4rev1\r\n')
        self._send('{} OK CAPABILITY completed\r\n'.format(tag).encode('ASCII'))


class BaseThreadedNetworkedTests(unittest.TestCase):

    def make_server(self, addr, hdlr):

        class MyServer(self.server_class):
            def handle_error(self, request, client_address):
                self.close_request(request)
                self.server_close()
                raise

        if verbose: print("creating server")
        server = MyServer(addr, hdlr)
        self.assertEqual(server.server_address, server.socket.getsockname())

        if verbose:
            print("server created")
            print("ADDR =", addr)
            print("CLASS =", self.server_class)
            print("HDLR =", server.RequestHandlerClass)

        t = threading.Thread(
            name='%s serving' % self.server_class,
            target=server.serve_forever,
            # Short poll interval to make the test finish quickly.
            # Time between requests is short enough that we won't wake
            # up spuriously too many times.
            kwargs={'poll_interval':0.01})
        t.daemon = True  # In case this function raises.
        t.start()
        if verbose: print("server running")
        return server, t

    def reap_server(self, server, thread):
        if verbose: print("waiting for server")
        server.shutdown()
        server.server_close()
        thread.join()
        if verbose: print("done")

    @contextmanager
    def reaped_server(self, hdlr):
        server, thread = self.make_server((support.HOST, 0), hdlr)
        try:
            yield server
        finally:
            self.reap_server(server, thread)

    @reap_threads
    def test_connect(self):
        with self.reaped_server(SimpleIMAPHandler) as server:
            client = self.imap_class(*server.server_address)
            client.shutdown()

    @reap_threads
    def test_issue5949(self):

        class EOFHandler(socketserver.StreamRequestHandler):
            def handle(self):
                # EOF without sending a complete welcome message.
                self.wfile.write(b'* OK')

        with self.reaped_server(EOFHandler) as server:
            self.assertRaises(imaplib.IMAP4.abort,
                              self.imap_class, *server.server_address)

    @reap_threads
    def test_line_termination(self):

        class BadNewlineHandler(SimpleIMAPHandler):

            def cmd_CAPABILITY(self, tag, args):
                self._send(b'* CAPABILITY IMAP4rev1 AUTH\n')
                self._send('{} OK CAPABILITY completed\r\n'.format(tag).encode('ASCII'))

        with self.reaped_server(BadNewlineHandler) as server:
            self.assertRaises(imaplib.IMAP4.abort,
                              self.imap_class, *server.server_address)



class ThreadedNetworkedTests(BaseThreadedNetworkedTests):

    server_class = socketserver.TCPServer
    imap_class = imaplib.IMAP4


@unittest.skipUnless(ssl, "SSL not available")
class ThreadedNetworkedTestsSSL(BaseThreadedNetworkedTests):

    server_class = SecureTCPServer
    imap_class = IMAP4_SSL


class RemoteIMAPTest(unittest.TestCase):
    host = 'cyrus.andrew.cmu.edu'
    port = 143
    username = 'anonymous'
    password = 'pass'
    imap_class = imaplib.IMAP4

    def setUp(self):
        with transient_internet(self.host):
            self.server = self.imap_class(self.host, self.port)

    def tearDown(self):
        if self.server is not None:
            self.server.logout()

    def test_logincapa(self):
        for cap in self.server.capabilities:
            self.assertIsInstance(cap, str)
        self.assertTrue('LOGINDISABLED' in self.server.capabilities)
        self.assertTrue('AUTH=ANONYMOUS' in self.server.capabilities)
        rs = self.server.login(self.username, self.password)
        self.assertEqual(rs[0], 'OK')

    def test_logout(self):
        rs = self.server.logout()
        self.server = None
        self.assertEqual(rs[0], 'BYE')


@unittest.skipUnless(ssl, "SSL not available")
class RemoteIMAP_STARTTLSTest(RemoteIMAPTest):

    def setUp(self):
        super().setUp()
        rs = self.server.starttls()
        self.assertEqual(rs[0], 'OK')

    def test_logincapa(self):
        for cap in self.server.capabilities:
            self.assertIsInstance(cap, str)
        self.assertFalse('LOGINDISABLED' in self.server.capabilities)


@unittest.skipUnless(ssl, "SSL not available")
class RemoteIMAP_SSLTest(RemoteIMAPTest):
    port = 993
    imap_class = IMAP4_SSL

    def test_logincapa(self):
        for cap in self.server.capabilities:
            self.assertIsInstance(cap, str)
        self.assertFalse('LOGINDISABLED' in self.server.capabilities)
        self.assertTrue('AUTH=PLAIN' in self.server.capabilities)


def test_main():
    tests = [TestImaplib]

    if support.is_resource_enabled('network'):
        if ssl:
            global CERTFILE
            CERTFILE = os.path.join(os.path.dirname(__file__) or os.curdir,
                                    "keycert.pem")
            if not os.path.exists(CERTFILE):
                raise support.TestFailed("Can't read certificate files!")
        tests.extend([
            ThreadedNetworkedTests, ThreadedNetworkedTestsSSL,
            RemoteIMAPTest, RemoteIMAP_SSLTest, RemoteIMAP_STARTTLSTest,
        ])

    support.run_unittest(*tests)


if __name__ == "__main__":
    support.use_resources = ['network']
    test_main()
