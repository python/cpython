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

from unittest import TestCase
from test import support as test_support
threading = test_support.import_module('threading')

HOST = test_support.HOST
PORT = 0

# the dummy data returned by server when LIST and RETR commands are issued
LIST_RESP = b'1 1\r\n2 2\r\n3 3\r\n4 4\r\n5 5\r\n.\r\n'
RETR_RESP = b"""From: postmaster@python.org\
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
        self.set_terminator(b"\r\n")
        self.in_buffer = []
        self.push('+OK dummy pop3 server ready. <timestamp>')

    def collect_incoming_data(self, data):
        self.in_buffer.append(data)

    def found_terminator(self):
        line = b''.join(self.in_buffer)
        line = str(line, 'ISO-8859-1')
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
        asynchat.async_chat.push(self, data.encode("ISO-8859-1") + b'\r\n')

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
            self.push('+OK %s %s' % (arg, arg))
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

    def cmd_apop(self, arg):
        self.push('+OK done nothing.')

    def cmd_quit(self, arg):
        self.push('+OK closing.')
        self.close_when_done()


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
        self.handler_instance = None

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

    def handle_accepted(self, conn, addr):
        self.handler_instance = self.handler(conn)

    def handle_connect(self):
        self.close()
    handle_read = handle_connect

    def writable(self):
        return 0

    def handle_error(self):
        raise


class TestPOP3Class(TestCase):
    def assertOK(self, resp):
        self.assertTrue(resp.startswith(b"+OK"))

    def setUp(self):
        self.server = DummyPOP3Server((HOST, PORT))
        self.server.start()
        self.client = poplib.POP3(self.server.host, self.server.port, timeout=3)

    def tearDown(self):
        self.client.close()
        self.server.stop()

    def test_getwelcome(self):
        self.assertEqual(self.client.getwelcome(),
                         b'+OK dummy pop3 server ready. <timestamp>')

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
                         ([b'1 1', b'2 2', b'3 3', b'4 4', b'5 5'],
                          25))
        self.assertTrue(self.client.list('1').endswith(b"OK 1 1"))

    def test_retr(self):
        expected = (b'+OK 116 bytes',
                    [b'From: postmaster@python.org', b'Content-Type: text/plain',
                     b'MIME-Version: 1.0', b'Subject: Dummy',
                     b'', b'line1', b'line2', b'line3'],
                    113)
        foo = self.client.retr('foo')
        self.assertEqual(foo, expected)

    def test_too_long_lines(self):
        self.assertRaises(poplib.error_proto, self.client._shortcmd,
                          'echo +%s' % ((poplib._MAXLINE + 10) * 'a'))

    def test_dele(self):
        self.assertOK(self.client.dele('foo'))

    def test_noop(self):
        self.assertOK(self.client.noop())

    def test_rpop(self):
        self.assertOK(self.client.rpop('foo'))

    def test_apop(self):
        self.assertOK(self.client.apop('foo', 'dummypassword'))

    def test_top(self):
        expected =  (b'+OK 116 bytes',
                     [b'From: postmaster@python.org', b'Content-Type: text/plain',
                      b'MIME-Version: 1.0', b'Subject: Dummy', b'',
                      b'line1', b'line2', b'line3'],
                     113)
        self.assertEqual(self.client.top(1, 1), expected)

    def test_uidl(self):
        self.client.uidl()
        self.client.uidl('foo')

    def test_quit(self):
        resp = self.client.quit()
        self.assertTrue(resp)
        self.assertIsNone(self.client.sock)
        self.assertIsNone(self.client.file)


SUPPORTS_SSL = False
if hasattr(poplib, 'POP3_SSL'):
    import ssl

    SUPPORTS_SSL = True
    CERTFILE = os.path.join(os.path.dirname(__file__) or os.curdir, "keycert.pem")

    class DummyPOP3_SSLHandler(DummyPOP3Handler):

        def __init__(self, conn):
            asynchat.async_chat.__init__(self, conn)
            ssl_socket = ssl.wrap_socket(self.socket, certfile=CERTFILE,
                                          server_side=True,
                                          do_handshake_on_connect=False)
            self.del_channel()
            self.set_socket(ssl_socket)
            # Must try handshake before calling push()
            self._ssl_accepting = True
            self._do_ssl_handshake()
            self.set_terminator(b"\r\n")
            self.in_buffer = []
            self.push('+OK dummy pop3 server ready. <timestamp>')

        def _do_ssl_handshake(self):
            try:
                self.socket.do_handshake()
            except ssl.SSLError as err:
                if err.args[0] in (ssl.SSL_ERROR_WANT_READ,
                                   ssl.SSL_ERROR_WANT_WRITE):
                    return
                elif err.args[0] == ssl.SSL_ERROR_EOF:
                    return self.handle_close()
                raise
            except socket.error as err:
                if err.args[0] == errno.ECONNABORTED:
                    return self.handle_close()
            else:
                self._ssl_accepting = False

        def handle_read(self):
            if self._ssl_accepting:
                self._do_ssl_handshake()
            else:
                DummyPOP3Handler.handle_read(self)


    class TestPOP3_SSLClass(TestPOP3Class):
        # repeat previous tests by using poplib.POP3_SSL

        def setUp(self):
            self.server = DummyPOP3Server((HOST, PORT))
            self.server.handler = DummyPOP3_SSLHandler
            self.server.start()
            self.client = poplib.POP3_SSL(self.server.host, self.server.port)

        def test__all__(self):
            self.assertIn('POP3_SSL', poplib.__all__)

        def test_context(self):
            ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            self.assertRaises(ValueError, poplib.POP3_SSL, self.server.host,
                              self.server.port, keyfile=CERTFILE, context=ctx)
            self.assertRaises(ValueError, poplib.POP3_SSL, self.server.host,
                              self.server.port, certfile=CERTFILE, context=ctx)
            self.assertRaises(ValueError, poplib.POP3_SSL, self.server.host,
                              self.server.port, keyfile=CERTFILE,
                              certfile=CERTFILE, context=ctx)

            self.client.quit()
            self.client = poplib.POP3_SSL(self.server.host, self.server.port,
                                          context=ctx)
            self.assertIsInstance(self.client.sock, ssl.SSLSocket)
            self.assertIs(self.client.sock.context, ctx)
            self.assertTrue(self.client.noop().startswith(b'+OK'))


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
            conn.send(b"+ Hola mundo\n")
            conn.close()
        except socket.timeout:
            pass
        finally:
            serv.close()

    def testTimeoutDefault(self):
        self.assertTrue(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            pop = poplib.POP3(HOST, self.port)
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
        pop = poplib.POP3(HOST, self.port, timeout=30)
        self.assertEqual(pop.sock.gettimeout(), 30)
        pop.sock.close()


def test_main():
    tests = [TestPOP3Class, TestTimeouts]
    if SUPPORTS_SSL:
        tests.append(TestPOP3_SSLClass)
    thread_info = test_support.threading_setup()
    try:
        test_support.run_unittest(*tests)
    finally:
        test_support.threading_cleanup(*thread_info)


if __name__ == '__main__':
    test_main()
