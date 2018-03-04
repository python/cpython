"""Test script for poplib module."""

# Modified by Giampaolo Rodola' to give poplib.POP3 and poplib.POP3_SSL
# a real test suite

import poplib
import asyncore
import asynchat
import socket
import os
import time
import errno

from unittest import TestCase, skipUnless
from test import test_support
from test.test_support import HOST
threading = test_support.import_module('threading')


# the dummy data returned by server when LIST and RETR commands are issued
LIST_RESP = '1 1\r\n2 2\r\n3 3\r\n4 4\r\n5 5\r\n.\r\n'
RETR_RESP = """From: postmaster@python.org\
\r\nContent-Type: text/plain\r\n\
MIME-Version: 1.0\r\n\
Subject: Dummy\r\n\
\r\n\
line1\r\n\
line2\r\n\
line3\r\n\
.\r\n"""


class DummyPOP3Handler(asynchat.async_chat):

    def __init__(self, conn):
        asynchat.async_chat.__init__(self, conn)
        self.set_terminator("\r\n")
        self.in_buffer = []
        self.push('+OK dummy pop3 server ready.')

    def collect_incoming_data(self, data):
        self.in_buffer.append(data)

    def found_terminator(self):
        line = ''.join(self.in_buffer)
        self.in_buffer = []
        cmd = line.split(' ')[0].lower()
        space = line.find(' ')
        if space != -1:
            arg = line[space + 1:]
        else:
            arg = ""
        if hasattr(self, 'cmd_' + cmd):
            method = getattr(self, 'cmd_' + cmd)
            method(arg)
        else:
            self.push('-ERR unrecognized POP3 command "%s".' %cmd)

    def handle_error(self):
        raise

    def push(self, data):
        asynchat.async_chat.push(self, data + '\r\n')

    def cmd_echo(self, arg):
        # sends back the received string (used by the test suite)
        self.push(arg)

    def cmd_user(self, arg):
        if arg != "guido":
            self.push("-ERR no such user")
        self.push('+OK password required')

    def cmd_pass(self, arg):
        if arg != "python":
            self.push("-ERR wrong password")
        self.push('+OK 10 messages')

    def cmd_stat(self, arg):
        self.push('+OK 10 100')

    def cmd_list(self, arg):
        if arg:
            self.push('+OK %s %s' %(arg, arg))
        else:
            self.push('+OK')
            asynchat.async_chat.push(self, LIST_RESP)

    cmd_uidl = cmd_list

    def cmd_retr(self, arg):
        self.push('+OK %s bytes' %len(RETR_RESP))
        asynchat.async_chat.push(self, RETR_RESP)

    cmd_top = cmd_retr

    def cmd_dele(self, arg):
        self.push('+OK message marked for deletion.')

    def cmd_noop(self, arg):
        self.push('+OK done nothing.')

    def cmd_rpop(self, arg):
        self.push('+OK done nothing.')


class DummyPOP3Server(asyncore.dispatcher, threading.Thread):

    handler = DummyPOP3Handler

    def __init__(self, address, af=socket.AF_INET):
        threading.Thread.__init__(self)
        asyncore.dispatcher.__init__(self)
        self.create_socket(af, socket.SOCK_STREAM)
        self.bind(address)
        self.listen(5)
        self.active = False
        self.active_lock = threading.Lock()
        self.host, self.port = self.socket.getsockname()[:2]

    def start(self):
        assert not self.active
        self.__flag = threading.Event()
        threading.Thread.start(self)
        self.__flag.wait()

    def run(self):
        self.active = True
        self.__flag.set()
        while self.active and asyncore.socket_map:
            self.active_lock.acquire()
            asyncore.loop(timeout=0.1, count=1)
            self.active_lock.release()
        asyncore.close_all(ignore_all=True)

    def stop(self):
        assert self.active
        self.active = False
        self.join()

    def handle_accept(self):
        conn, addr = self.accept()
        self.handler = self.handler(conn)
        self.close()

    def handle_connect(self):
        self.close()
    handle_read = handle_connect

    def writable(self):
        return 0

    def handle_error(self):
        raise


class TestPOP3Class(TestCase):

    def assertOK(self, resp):
        self.assertTrue(resp.startswith("+OK"))

    def setUp(self):
        self.server = DummyPOP3Server((HOST, 0))
        self.server.start()
        self.client = poplib.POP3(self.server.host, self.server.port)

    def tearDown(self):
        self.client.quit()
        self.server.stop()

    def test_getwelcome(self):
        self.assertEqual(self.client.getwelcome(), '+OK dummy pop3 server ready.')

    def test_exceptions(self):
        self.assertRaises(poplib.error_proto, self.client._shortcmd, 'echo -err')

    def test_user(self):
        self.assertOK(self.client.user('guido'))
        self.assertRaises(poplib.error_proto, self.client.user, 'invalid')

    def test_pass_(self):
        self.assertOK(self.client.pass_('python'))
        self.assertRaises(poplib.error_proto, self.client.user, 'invalid')

    def test_stat(self):
        self.assertEqual(self.client.stat(), (10, 100))

    def test_list(self):
        self.assertEqual(self.client.list()[1:],
                         (['1 1', '2 2', '3 3', '4 4', '5 5'], 25))
        self.assertTrue(self.client.list('1').endswith("OK 1 1"))

    def test_retr(self):
        expected = ('+OK 116 bytes',
                    ['From: postmaster@python.org', 'Content-Type: text/plain',
                     'MIME-Version: 1.0', 'Subject: Dummy',
                     '', 'line1', 'line2', 'line3'],
                    113)
        self.assertEqual(self.client.retr('foo'), expected)

    def test_too_long_lines(self):
        self.assertRaises(poplib.error_proto, self.client._shortcmd,
                          'echo +%s' % ((poplib._MAXLINE + 10) * 'a'))

    def test_dele(self):
        self.assertOK(self.client.dele('foo'))

    def test_noop(self):
        self.assertOK(self.client.noop())

    def test_rpop(self):
        self.assertOK(self.client.rpop('foo'))

    def test_apop_REDOS(self):
        # Replace welcome with very long evil welcome.
        # NB The upper bound on welcome length is currently 2048.
        # At this length, evil input makes each apop call take
        # on the order of milliseconds instead of microseconds.
        evil_welcome = b'+OK' + (b'<' * 1000000)
        with test_support.swap_attr(self.client, 'welcome', evil_welcome):
            # The evil welcome is invalid, so apop should throw.
            self.assertRaises(poplib.error_proto, self.client.apop, 'a', 'kb')

    def test_top(self):
        expected =  ('+OK 116 bytes',
                     ['From: postmaster@python.org', 'Content-Type: text/plain',
                      'MIME-Version: 1.0', 'Subject: Dummy', '',
                      'line1', 'line2', 'line3'],
                     113)
        self.assertEqual(self.client.top(1, 1), expected)

    def test_uidl(self):
        self.client.uidl()
        self.client.uidl('foo')


SUPPORTS_SSL = False
if hasattr(poplib, 'POP3_SSL'):
    import ssl

    SUPPORTS_SSL = True
    CERTFILE = os.path.join(os.path.dirname(__file__) or os.curdir, "keycert.pem")

    class DummyPOP3_SSLHandler(DummyPOP3Handler):

        def __init__(self, conn):
            asynchat.async_chat.__init__(self, conn)
            self.socket = ssl.wrap_socket(self.socket, certfile=CERTFILE,
                                          server_side=True,
                                          do_handshake_on_connect=False)
            # Must try handshake before calling push()
            self._ssl_accepting = True
            self._do_ssl_handshake()
            self.set_terminator("\r\n")
            self.in_buffer = []
            self.push('+OK dummy pop3 server ready.')

        def _do_ssl_handshake(self):
            try:
                self.socket.do_handshake()
            except ssl.SSLError, err:
                if err.args[0] in (ssl.SSL_ERROR_WANT_READ,
                                   ssl.SSL_ERROR_WANT_WRITE):
                    return
                elif err.args[0] == ssl.SSL_ERROR_EOF:
                    return self.handle_close()
                raise
            except socket.error, err:
                if err.args[0] == errno.ECONNABORTED:
                    return self.handle_close()
            else:
                self._ssl_accepting = False

        def handle_read(self):
            if self._ssl_accepting:
                self._do_ssl_handshake()
            else:
                DummyPOP3Handler.handle_read(self)

requires_ssl = skipUnless(SUPPORTS_SSL, 'SSL not supported')

@requires_ssl
class TestPOP3_SSLClass(TestPOP3Class):
    # repeat previous tests by using poplib.POP3_SSL

    def setUp(self):
        self.server = DummyPOP3Server((HOST, 0))
        self.server.handler = DummyPOP3_SSLHandler
        self.server.start()
        self.client = poplib.POP3_SSL(self.server.host, self.server.port)

    def test__all__(self):
        self.assertIn('POP3_SSL', poplib.__all__)


class TestTimeouts(TestCase):

    def setUp(self):
        self.evt = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(60)  # Safety net. Look issue 11812
        self.port = test_support.bind_port(self.sock)
        self.thread = threading.Thread(target=self.server, args=(self.evt,self.sock))
        self.thread.setDaemon(True)
        self.thread.start()
        self.evt.wait()

    def tearDown(self):
        self.thread.join()
        del self.thread  # Clear out any dangling Thread objects.

    def server(self, evt, serv):
        serv.listen(5)
        evt.set()
        try:
            conn, addr = serv.accept()
            conn.send("+ Hola mundo\n")
            conn.close()
        except socket.timeout:
            pass
        finally:
            serv.close()

    def testTimeoutDefault(self):
        self.assertIsNone(socket.getdefaulttimeout())
        socket.setdefaulttimeout(30)
        try:
            pop = poplib.POP3(HOST, self.port)
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(pop.sock.gettimeout(), 30)
        pop.sock.close()

    def testTimeoutNone(self):
        self.assertIsNone(socket.getdefaulttimeout())
        socket.setdefaulttimeout(30)
        try:
            pop = poplib.POP3(HOST, self.port, timeout=None)
        finally:
            socket.setdefaulttimeout(None)
        self.assertIsNone(pop.sock.gettimeout())
        pop.sock.close()

    def testTimeoutValue(self):
        pop = poplib.POP3(HOST, self.port, timeout=30)
        self.assertEqual(pop.sock.gettimeout(), 30)
        pop.sock.close()


def test_main():
    tests = [TestPOP3Class, TestTimeouts,
             TestPOP3_SSLClass]
    thread_info = test_support.threading_setup()
    try:
        test_support.run_unittest(*tests)
    finally:
        test_support.threading_cleanup(*thread_info)


if __name__ == '__main__':
    test_main()
