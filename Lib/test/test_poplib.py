"""Test script for poplib module."""

# Modified by Giampaolo Rodola' to give poplib.POP3 and poplib.POP3_SSL
# a real test suite

import asyncio
import os
import poplib
import socket
from asyncio import sslproto
from functools import partial
from test import support as test_support
from unittest import TestCase, skipUnless

threading = test_support.import_module('threading')

HOST = test_support.HOST
PORT = 0

SUPPORTS_SSL = False
if hasattr(poplib, 'POP3_SSL'):
    import ssl

    SUPPORTS_SSL = True
    CERTFILE = os.path.join(os.path.dirname(__file__) or os.curdir, "keycert3.pem")
    CAFILE = os.path.join(os.path.dirname(__file__) or os.curdir, "pycacert.pem")


    def get_ssl_context():
        context = ssl.SSLContext()
        context.load_cert_chain(CERTFILE)
        return context

requires_ssl = skipUnless(SUPPORTS_SSL, 'SSL not supported')

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


class DummyPOP3Handler(asyncio.StreamReaderProtocol):
    TERMINATOR = b"\r\n"
    CAPAS = {'UIDL': [], 'IMPLEMENTATION': ['python-testlib-pop-server']}
    enable_UTF8 = False

    _over_ssl = False
    _transport = None
    _handle_task = None
    _tls_protocol = None

    def __init__(self, loop, ssl_started):
        self.loop = loop
        self._tls_started = ssl_started

        super().__init__(
            asyncio.StreamReader(loop=self.loop),
            client_connected_cb=self._client_connected_cb,
            loop=self.loop)

    def connection_made(self, transport):
        self._transport = transport
        if transport.get_extra_info('sslcontext') and not self._tls_started:
            self._stream_reader._transport = self._transport
            self._stream_writer._transport = self._transport
        else:
            super(DummyPOP3Handler, self).connection_made(transport)

    def connection_lost(self, exc):
        super(DummyPOP3Handler, self).connection_lost(exc)
        self._transport.close()

    def _client_connected_cb(self, *_):
        self.push('+OK dummy pop3 server ready. <timestamp>')
        self._handle_task = self.loop.create_task(
            self._handle_client())

    def _handle_client(self):
        while self._transport:
            try:
                line = yield from self._stream_reader.readuntil(separator=self.TERMINATOR)
            except asyncio.IncompleteReadError:
                break

            self._process_line(line.rstrip(self.TERMINATOR))

    def _process_line(self, line):
        line = str(line, 'ISO-8859-1')
        cmd, arg = line.partition(" ")[::2]
        method_name = "cmd_{0}".format(cmd.lower())
        method = getattr(self, method_name, partial(self._unknown_cmd, cmd))
        method(arg)

    def push(self, data):
        data = data.encode("ISO-8859-1") + b'\r\n'
        self._stream_writer.write(data)

    def _unknown_cmd(self, cmd, _):
        self.push('-ERR unrecognized POP3 command "%s".' % cmd)

    def cmd_echo(self, arg):
        # sends back the received string (used by the test suite)
        self.push(arg)

    def cmd_user(self, arg):
        if arg != "guido":
            self.push("-ERR no such user")
        else:
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
            self._stream_writer.write(LIST_RESP)

    cmd_uidl = cmd_list

    def cmd_retr(self, arg):
        self.push('+OK %s bytes' % len(RETR_RESP))
        self._stream_writer.write(RETR_RESP)

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
        self._transport.close()

    def _get_capas(self):
        _capas = dict(self.CAPAS)
        if not self._over_ssl and SUPPORTS_SSL:
            _capas['STLS'] = []
        return _capas

    def cmd_capa(self, arg):
        self.push('+OK Capability list follows')
        if self._get_capas():
            for cap, params in self._get_capas().items():
                _ln = [cap]
                if params:
                    _ln.extend(params)
                self.push(' '.join(_ln))
        self.push('.')

    def cmd_utf8(self, arg):
        self.push('+OK I know RFC6856'
                  if self.enable_UTF8
                  else '-ERR What is UTF8?!')

    if SUPPORTS_SSL:
        def cmd_stls(self, _):
            if self._over_ssl is False:
                self.push('+OK Begin TLS negotiation')
                self._tls_protocol = sslproto.SSLProtocol(
                    self.loop,
                    self,
                    get_ssl_context(),
                    None,
                    server_side=True)

                self._transport._protocol = self._tls_protocol
                self._tls_protocol.connection_made(self._transport)

                self._over_ssl = True
            else:
                self.push('-ERR Command not permitted when TLS active')


class DummyPOP3Server(threading.Thread):
    SSL = False
    handler = DummyPOP3Handler

    def __init__(self, address, af=socket.AF_INET):
        super(DummyPOP3Server, self).__init__()
        self._stop = threading.Event()
        self._loop = asyncio.new_event_loop()
        self._af = af

        server_future = self._loop.create_server(lambda: self.handler(self._loop, self.SSL),
                                                 *address,
                                                 ssl=get_ssl_context() if self.SSL else None,
                                                 family=self._af, flags=socket.SOCK_STREAM)
        self._server = self._loop.run_until_complete(server_future)
        self.host, self.port = self._server.sockets[0].getsockname()

    def run(self):
        self._loop.run_forever()
        self._server.close()
        self._loop.run_until_complete(self._server.wait_closed())
        self._cancel_tasks()
        self._loop.close()
        self._stop.set()

    def stop(self):
        self._loop.call_soon_threadsafe(self._loop.stop)
        self._stop.wait()

    def _cancel_tasks(self):
        pending = asyncio.Task.all_tasks(loop=self._loop)
        if pending:
            self._loop.run_until_complete(asyncio.gather(*pending))


class DummyPOP3ServerSSL(DummyPOP3Server):
    SSL = True


class TestPOP3Class(TestCase):
    def assertOK(self, resp):
        self.assertTrue(resp.startswith(b"+OK"))

    def setUp(self):
        self.server = DummyPOP3Server((HOST, PORT))
        self.server.start()
        self.client = poplib.POP3(self.server.host, self.server.port, timeout=3)

    def tearDown(self):
        if self.client.file is not None and self.client.sock is not None:
            self.client.quit()
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

        # read remaining data, otherwise it will be treated as response
        # to QUIT and client close connection to quickly
        self.client.file.readline()

    def test_dele(self):
        self.assertOK(self.client.dele('foo'))

    def test_noop(self):
        self.assertOK(self.client.noop())

    def test_rpop(self):
        self.assertOK(self.client.rpop('foo'))

    def test_apop(self):
        self.assertOK(self.client.apop('foo', 'dummypassword'))

    def test_top(self):
        expected = (b'+OK 116 bytes',
                    [b'From: postmaster@python.org', b'Content-Type: text/plain',
                     b'MIME-Version: 1.0', b'Subject: Dummy', b'',
                     b'line1', b'line2', b'line3'],
                    113)
        self.assertEqual(self.client.top(1, 1), expected)

    def test_uidl(self):
        self.client.uidl()
        self.client.uidl('foo')

    def test_utf8_raises_if_unsupported(self):
        self.server.handler.enable_UTF8 = False
        self.assertRaises(poplib.error_proto, self.client.utf8)

    def test_utf8(self):
        self.server.handler.enable_UTF8 = True
        expected = b'+OK I know RFC6856'
        result = self.client.utf8()
        self.assertEqual(result, expected)

    def test_capa(self):
        capa = self.client.capa()
        self.assertTrue('IMPLEMENTATION' in capa.keys())

    def test_quit(self):
        resp = self.client.quit()
        self.assertTrue(resp)
        self.assertIsNone(self.client.sock)
        self.assertIsNone(self.client.file)

    @requires_ssl
    def test_stls_capa(self):
        capa = self.client.capa()
        self.assertTrue('STLS' in capa.keys())

    @requires_ssl
    def test_stls(self):
        expected = b'+OK Begin TLS negotiation'
        resp = self.client.stls()
        self.assertEqual(resp, expected)

    @requires_ssl
    def test_stls_context(self):
        expected = b'+OK Begin TLS negotiation'
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.load_verify_locations(CAFILE)
        ctx.verify_mode = ssl.CERT_REQUIRED
        ctx.check_hostname = True
        with self.assertRaises(ssl.CertificateError):
            resp = self.client.stls(context=ctx)
        self.client = poplib.POP3("localhost", self.server.port, timeout=3)
        resp = self.client.stls(context=ctx)
        self.assertEqual(resp, expected)


@requires_ssl
class TestPOP3_SSLClass(TestPOP3Class):
    # repeat previous tests by using poplib.POP3_SSL

    def setUp(self):
        self.server = DummyPOP3ServerSSL((HOST, PORT))
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

    def test_stls(self):
        self.assertRaises(poplib.error_proto, self.client.stls)

    test_stls_context = test_stls

    def test_stls_capa(self):
        capa = self.client.capa()
        self.assertFalse('STLS' in capa.keys())


@requires_ssl
class TestPOP3_TLSClass(TestPOP3Class):
    # repeat previous tests by using poplib.POP3.stls()

    def setUp(self):
        self.server = DummyPOP3Server((HOST, PORT))
        self.server.start()
        self.client = poplib.POP3(self.server.host, self.server.port, timeout=3)
        self.client.stls()

    def test_stls(self):
        self.assertRaises(poplib.error_proto, self.client.stls)

    test_stls_context = test_stls

    def test_stls_capa(self):
        capa = self.client.capa()
        self.assertFalse(b'STLS' in capa.keys())


class TestTimeouts(TestCase):
    def setUp(self):
        self.evt = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(60)  # Safety net. Look issue 11812
        self.port = test_support.bind_port(self.sock)
        self.thread = threading.Thread(target=self.server, args=(self.evt, self.sock))
        self.thread.setDaemon(True)
        self.thread.start()
        self.evt.wait()

    def tearDown(self):
        self.thread.join()
        del self.thread  # Clear out any dangling Thread objects.

    def server(self, evt, serv):
        serv.listen()
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
             TestPOP3_SSLClass, TestPOP3_TLSClass]
    thread_info = test_support.threading_setup()
    try:
        test_support.run_unittest(*tests)
    finally:
        test_support.threading_cleanup(*thread_info)


if __name__ == '__main__':
    test_main()
