"""Tests for sendfile functionality."""

import asyncio
import os
import socket
import sys
import tempfile
import unittest
from asyncio import base_events
from asyncio import constants
from unittest import mock
from test import support
from test.support import os_helper
from test.support import socket_helper
from test.test_asyncio import utils as test_utils

try:
    import ssl
except ImportError:
    ssl = None


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class MySendfileProto(asyncio.Protocol):

    def __init__(self, loop=None, close_after=0):
        self.transport = None
        self.state = 'INITIAL'
        self.nbytes = 0
        if loop is not None:
            self.connected = loop.create_future()
            self.done = loop.create_future()
        self.data = bytearray()
        self.close_after = close_after

    def _assert_state(self, *expected):
        if self.state not in expected:
            raise AssertionError(f'state: {self.state!r}, expected: {expected!r}')

    def connection_made(self, transport):
        self.transport = transport
        self._assert_state('INITIAL')
        self.state = 'CONNECTED'
        if self.connected:
            self.connected.set_result(None)

    def eof_received(self):
        self._assert_state('CONNECTED')
        self.state = 'EOF'

    def connection_lost(self, exc):
        self._assert_state('CONNECTED', 'EOF')
        self.state = 'CLOSED'
        if self.done:
            self.done.set_result(None)

    def data_received(self, data):
        self._assert_state('CONNECTED')
        self.nbytes += len(data)
        self.data.extend(data)
        super().data_received(data)
        if self.close_after and self.nbytes >= self.close_after:
            self.transport.close()


class MyProto(asyncio.Protocol):

    def __init__(self, loop):
        self.started = False
        self.closed = False
        self.data = bytearray()
        self.fut = loop.create_future()
        self.transport = None

    def connection_made(self, transport):
        self.started = True
        self.transport = transport

    def data_received(self, data):
        self.data.extend(data)

    def connection_lost(self, exc):
        self.closed = True
        self.fut.set_result(None)

    async def wait_closed(self):
        await self.fut


class SendfileBase:

    # 256 KiB plus small unaligned to buffer chunk
    # Newer versions of Windows seems to have increased its internal
    # buffer and tries to send as much of the data as it can as it
    # has some form of buffering for this which is less than 256KiB
    # on newer server versions and Windows 11.
    # So DATA should be larger than 256 KiB to make this test reliable.
    DATA = b"x" * (1024 * 256 + 1)
    # Reduce socket buffer size to test on relative small data sets.
    BUF_SIZE = 4 * 1024   # 4 KiB

    def create_event_loop(self):
        raise NotImplementedError

    @classmethod
    def setUpClass(cls):
        with open(os_helper.TESTFN, 'wb') as fp:
            fp.write(cls.DATA)
        super().setUpClass()

    @classmethod
    def tearDownClass(cls):
        os_helper.unlink(os_helper.TESTFN)
        super().tearDownClass()

    def setUp(self):
        self.file = open(os_helper.TESTFN, 'rb')
        self.addCleanup(self.file.close)
        self.loop = self.create_event_loop()
        self.set_event_loop(self.loop)
        super().setUp()

    def tearDown(self):
        # just in case if we have transport close callbacks
        if not self.loop.is_closed():
            test_utils.run_briefly(self.loop)

        self.doCleanups()
        support.gc_collect()
        super().tearDown()

    def run_loop(self, coro):
        return self.loop.run_until_complete(coro)


class SockSendfileMixin(SendfileBase):

    @classmethod
    def setUpClass(cls):
        cls.__old_bufsize = constants.SENDFILE_FALLBACK_READBUFFER_SIZE
        constants.SENDFILE_FALLBACK_READBUFFER_SIZE = 1024 * 16
        super().setUpClass()

    @classmethod
    def tearDownClass(cls):
        constants.SENDFILE_FALLBACK_READBUFFER_SIZE = cls.__old_bufsize
        super().tearDownClass()

    def make_socket(self, cleanup=True):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setblocking(False)
        if cleanup:
            self.addCleanup(sock.close)
        return sock

    def reduce_receive_buffer_size(self, sock):
        # Reduce receive socket buffer size to test on relative
        # small data sets.
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, self.BUF_SIZE)

    def reduce_send_buffer_size(self, sock, transport=None):
        # Reduce send socket buffer size to test on relative small data sets.

        # On macOS, SO_SNDBUF is reset by connect(). So this method
        # should be called after the socket is connected.
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, self.BUF_SIZE)

        if transport is not None:
            transport.set_write_buffer_limits(high=self.BUF_SIZE)

    def prepare_socksendfile(self):
        proto = MyProto(self.loop)
        port = socket_helper.find_unused_port()
        srv_sock = self.make_socket(cleanup=False)
        srv_sock.bind((socket_helper.HOST, port))
        server = self.run_loop(self.loop.create_server(
            lambda: proto, sock=srv_sock))
        self.reduce_receive_buffer_size(srv_sock)

        sock = self.make_socket()
        self.run_loop(self.loop.sock_connect(sock, ('127.0.0.1', port)))
        self.reduce_send_buffer_size(sock)

        def cleanup():
            if proto.transport is not None:
                # can be None if the task was cancelled before
                # connection_made callback
                proto.transport.close()
                self.run_loop(proto.wait_closed())

            server.close()
            self.run_loop(server.wait_closed())

        self.addCleanup(cleanup)

        return sock, proto

    def test_sock_sendfile_success(self):
        sock, proto = self.prepare_socksendfile()
        ret = self.run_loop(self.loop.sock_sendfile(sock, self.file))
        sock.close()
        self.run_loop(proto.wait_closed())

        self.assertEqual(ret, len(self.DATA))
        self.assertEqual(proto.data, self.DATA)
        self.assertEqual(self.file.tell(), len(self.DATA))

    def test_sock_sendfile_with_offset_and_count(self):
        sock, proto = self.prepare_socksendfile()
        ret = self.run_loop(self.loop.sock_sendfile(sock, self.file,
                                                    1000, 2000))
        sock.close()
        self.run_loop(proto.wait_closed())

        self.assertEqual(proto.data, self.DATA[1000:3000])
        self.assertEqual(self.file.tell(), 3000)
        self.assertEqual(ret, 2000)

    def test_sock_sendfile_zero_size(self):
        sock, proto = self.prepare_socksendfile()
        with tempfile.TemporaryFile() as f:
            ret = self.run_loop(self.loop.sock_sendfile(sock, f,
                                                        0, None))
        sock.close()
        self.run_loop(proto.wait_closed())

        self.assertEqual(ret, 0)
        self.assertEqual(self.file.tell(), 0)

    def test_sock_sendfile_mix_with_regular_send(self):
        buf = b"mix_regular_send" * (4 * 1024)  # 64 KiB
        sock, proto = self.prepare_socksendfile()
        self.run_loop(self.loop.sock_sendall(sock, buf))
        ret = self.run_loop(self.loop.sock_sendfile(sock, self.file))
        self.run_loop(self.loop.sock_sendall(sock, buf))
        sock.close()
        self.run_loop(proto.wait_closed())

        self.assertEqual(ret, len(self.DATA))
        expected = buf + self.DATA + buf
        self.assertEqual(proto.data, expected)
        self.assertEqual(self.file.tell(), len(self.DATA))


class SendfileMixin(SendfileBase):

    # Note: sendfile via SSL transport is equal to sendfile fallback

    def prepare_sendfile(self, *, is_ssl=False, close_after=0):
        port = socket_helper.find_unused_port()
        srv_proto = MySendfileProto(loop=self.loop,
                                    close_after=close_after)
        if is_ssl:
            if not ssl:
                self.skipTest("No ssl module")
            srv_ctx = test_utils.simple_server_sslcontext()
            cli_ctx = test_utils.simple_client_sslcontext()
        else:
            srv_ctx = None
            cli_ctx = None
        srv_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv_sock.bind((socket_helper.HOST, port))
        server = self.run_loop(self.loop.create_server(
            lambda: srv_proto, sock=srv_sock, ssl=srv_ctx))
        self.reduce_receive_buffer_size(srv_sock)

        if is_ssl:
            server_hostname = socket_helper.HOST
        else:
            server_hostname = None
        cli_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        cli_sock.connect((socket_helper.HOST, port))

        cli_proto = MySendfileProto(loop=self.loop)
        tr, pr = self.run_loop(self.loop.create_connection(
            lambda: cli_proto, sock=cli_sock,
            ssl=cli_ctx, server_hostname=server_hostname))
        self.reduce_send_buffer_size(cli_sock, transport=tr)

        def cleanup():
            srv_proto.transport.close()
            cli_proto.transport.close()
            self.run_loop(srv_proto.done)
            self.run_loop(cli_proto.done)

            server.close()
            self.run_loop(server.wait_closed())

        self.addCleanup(cleanup)
        return srv_proto, cli_proto

    @unittest.skipIf(sys.platform == 'win32', "UDP sockets are not supported")
    def test_sendfile_not_supported(self):
        tr, pr = self.run_loop(
            self.loop.create_datagram_endpoint(
                asyncio.DatagramProtocol,
                family=socket.AF_INET))
        try:
            with self.assertRaisesRegex(RuntimeError, "not supported"):
                self.run_loop(
                    self.loop.sendfile(tr, self.file))
            self.assertEqual(0, self.file.tell())
        finally:
            # don't use self.addCleanup because it produces resource warning
            tr.close()

    def test_sendfile(self):
        srv_proto, cli_proto = self.prepare_sendfile()
        ret = self.run_loop(
            self.loop.sendfile(cli_proto.transport, self.file))
        cli_proto.transport.close()
        self.run_loop(srv_proto.done)
        self.assertEqual(ret, len(self.DATA))
        self.assertEqual(srv_proto.nbytes, len(self.DATA))
        self.assertEqual(srv_proto.data, self.DATA)
        self.assertEqual(self.file.tell(), len(self.DATA))

    def test_sendfile_force_fallback(self):
        srv_proto, cli_proto = self.prepare_sendfile()

        def sendfile_native(transp, file, offset, count):
            # to raise SendfileNotAvailableError
            return base_events.BaseEventLoop._sendfile_native(
                self.loop, transp, file, offset, count)

        self.loop._sendfile_native = sendfile_native

        ret = self.run_loop(
            self.loop.sendfile(cli_proto.transport, self.file))
        cli_proto.transport.close()
        self.run_loop(srv_proto.done)
        self.assertEqual(ret, len(self.DATA))
        self.assertEqual(srv_proto.nbytes, len(self.DATA))
        self.assertEqual(srv_proto.data, self.DATA)
        self.assertEqual(self.file.tell(), len(self.DATA))

    def test_sendfile_force_unsupported_native(self):
        if sys.platform == 'win32':
            if isinstance(self.loop, asyncio.ProactorEventLoop):
                self.skipTest("Fails on proactor event loop")
        srv_proto, cli_proto = self.prepare_sendfile()

        def sendfile_native(transp, file, offset, count):
            # to raise SendfileNotAvailableError
            return base_events.BaseEventLoop._sendfile_native(
                self.loop, transp, file, offset, count)

        self.loop._sendfile_native = sendfile_native

        with self.assertRaisesRegex(asyncio.SendfileNotAvailableError,
                                    "not supported"):
            self.run_loop(
                self.loop.sendfile(cli_proto.transport, self.file,
                                   fallback=False))

        cli_proto.transport.close()
        self.run_loop(srv_proto.done)
        self.assertEqual(srv_proto.nbytes, 0)
        self.assertEqual(self.file.tell(), 0)

    def test_sendfile_ssl(self):
        srv_proto, cli_proto = self.prepare_sendfile(is_ssl=True)
        ret = self.run_loop(
            self.loop.sendfile(cli_proto.transport, self.file))
        cli_proto.transport.close()
        self.run_loop(srv_proto.done)
        self.assertEqual(ret, len(self.DATA))
        self.assertEqual(srv_proto.nbytes, len(self.DATA))
        self.assertEqual(srv_proto.data, self.DATA)
        self.assertEqual(self.file.tell(), len(self.DATA))

    def test_sendfile_for_closing_transp(self):
        srv_proto, cli_proto = self.prepare_sendfile()
        cli_proto.transport.close()
        with self.assertRaisesRegex(RuntimeError, "is closing"):
            self.run_loop(self.loop.sendfile(cli_proto.transport, self.file))
        self.run_loop(srv_proto.done)
        self.assertEqual(srv_proto.nbytes, 0)
        self.assertEqual(self.file.tell(), 0)

    def test_sendfile_pre_and_post_data(self):
        srv_proto, cli_proto = self.prepare_sendfile()
        PREFIX = b'PREFIX__' * 1024  # 8 KiB
        SUFFIX = b'--SUFFIX' * 1024  # 8 KiB
        cli_proto.transport.write(PREFIX)
        ret = self.run_loop(
            self.loop.sendfile(cli_proto.transport, self.file))
        cli_proto.transport.write(SUFFIX)
        cli_proto.transport.close()
        self.run_loop(srv_proto.done)
        self.assertEqual(ret, len(self.DATA))
        self.assertEqual(srv_proto.data, PREFIX + self.DATA + SUFFIX)
        self.assertEqual(self.file.tell(), len(self.DATA))

    def test_sendfile_ssl_pre_and_post_data(self):
        srv_proto, cli_proto = self.prepare_sendfile(is_ssl=True)
        PREFIX = b'zxcvbnm' * 1024
        SUFFIX = b'0987654321' * 1024
        cli_proto.transport.write(PREFIX)
        ret = self.run_loop(
            self.loop.sendfile(cli_proto.transport, self.file))
        cli_proto.transport.write(SUFFIX)
        cli_proto.transport.close()
        self.run_loop(srv_proto.done)
        self.assertEqual(ret, len(self.DATA))
        self.assertEqual(srv_proto.data, PREFIX + self.DATA + SUFFIX)
        self.assertEqual(self.file.tell(), len(self.DATA))

    def test_sendfile_partial(self):
        srv_proto, cli_proto = self.prepare_sendfile()
        ret = self.run_loop(
            self.loop.sendfile(cli_proto.transport, self.file, 1000, 100))
        cli_proto.transport.close()
        self.run_loop(srv_proto.done)
        self.assertEqual(ret, 100)
        self.assertEqual(srv_proto.nbytes, 100)
        self.assertEqual(srv_proto.data, self.DATA[1000:1100])
        self.assertEqual(self.file.tell(), 1100)

    def test_sendfile_ssl_partial(self):
        srv_proto, cli_proto = self.prepare_sendfile(is_ssl=True)
        ret = self.run_loop(
            self.loop.sendfile(cli_proto.transport, self.file, 1000, 100))
        cli_proto.transport.close()
        self.run_loop(srv_proto.done)
        self.assertEqual(ret, 100)
        self.assertEqual(srv_proto.nbytes, 100)
        self.assertEqual(srv_proto.data, self.DATA[1000:1100])
        self.assertEqual(self.file.tell(), 1100)

    def test_sendfile_close_peer_after_receiving(self):
        srv_proto, cli_proto = self.prepare_sendfile(
            close_after=len(self.DATA))
        ret = self.run_loop(
            self.loop.sendfile(cli_proto.transport, self.file))
        cli_proto.transport.close()
        self.run_loop(srv_proto.done)
        self.assertEqual(ret, len(self.DATA))
        self.assertEqual(srv_proto.nbytes, len(self.DATA))
        self.assertEqual(srv_proto.data, self.DATA)
        self.assertEqual(self.file.tell(), len(self.DATA))

    def test_sendfile_ssl_close_peer_after_receiving(self):
        srv_proto, cli_proto = self.prepare_sendfile(
            is_ssl=True, close_after=len(self.DATA))
        ret = self.run_loop(
            self.loop.sendfile(cli_proto.transport, self.file))
        self.run_loop(srv_proto.done)
        self.assertEqual(ret, len(self.DATA))
        self.assertEqual(srv_proto.nbytes, len(self.DATA))
        self.assertEqual(srv_proto.data, self.DATA)
        self.assertEqual(self.file.tell(), len(self.DATA))

    # On Solaris, lowering SO_RCVBUF on a TCP connection after it has been
    # established has no effect. Due to its age, this bug affects both Oracle
    # Solaris as well as all other OpenSolaris forks (unless they fixed it
    # themselves).
    @unittest.skipIf(sys.platform.startswith('sunos'),
                     "Doesn't work on Solaris")
    def test_sendfile_close_peer_in_the_middle_of_receiving(self):
        srv_proto, cli_proto = self.prepare_sendfile(close_after=1024)
        with self.assertRaises(ConnectionError):
            self.run_loop(
                self.loop.sendfile(cli_proto.transport, self.file))
        self.run_loop(srv_proto.done)

        self.assertTrue(1024 <= srv_proto.nbytes < len(self.DATA),
                        srv_proto.nbytes)
        self.assertTrue(1024 <= self.file.tell() < len(self.DATA),
                        self.file.tell())
        self.assertTrue(cli_proto.transport.is_closing())

    def test_sendfile_fallback_close_peer_in_the_middle_of_receiving(self):

        def sendfile_native(transp, file, offset, count):
            # to raise SendfileNotAvailableError
            return base_events.BaseEventLoop._sendfile_native(
                self.loop, transp, file, offset, count)

        self.loop._sendfile_native = sendfile_native

        srv_proto, cli_proto = self.prepare_sendfile(close_after=1024)
        with self.assertRaises(ConnectionError):
            self.run_loop(
                self.loop.sendfile(cli_proto.transport, self.file))
        self.run_loop(srv_proto.done)

        self.assertTrue(1024 <= srv_proto.nbytes < len(self.DATA),
                        srv_proto.nbytes)
        self.assertTrue(1024 <= self.file.tell() < len(self.DATA),
                        self.file.tell())

    @unittest.skipIf(not hasattr(os, 'sendfile'),
                     "Don't have native sendfile support")
    def test_sendfile_prevents_bare_write(self):
        srv_proto, cli_proto = self.prepare_sendfile()
        fut = self.loop.create_future()

        async def coro():
            fut.set_result(None)
            return await self.loop.sendfile(cli_proto.transport, self.file)

        t = self.loop.create_task(coro())
        self.run_loop(fut)
        with self.assertRaisesRegex(RuntimeError,
                                    "sendfile is in progress"):
            cli_proto.transport.write(b'data')
        ret = self.run_loop(t)
        self.assertEqual(ret, len(self.DATA))

    def test_sendfile_no_fallback_for_fallback_transport(self):
        transport = mock.Mock()
        transport.is_closing.side_effect = lambda: False
        transport._sendfile_compatible = constants._SendfileMode.FALLBACK
        with self.assertRaisesRegex(RuntimeError, 'fallback is disabled'):
            self.loop.run_until_complete(
                self.loop.sendfile(transport, None, fallback=False))


class SendfileTestsBase(SendfileMixin, SockSendfileMixin):
    pass


if sys.platform == 'win32':

    class SelectEventLoopTests(SendfileTestsBase,
                               test_utils.TestCase):

        def create_event_loop(self):
            return asyncio.SelectorEventLoop()

    class ProactorEventLoopTests(SendfileTestsBase,
                                 test_utils.TestCase):

        def create_event_loop(self):
            return asyncio.ProactorEventLoop()

else:
    import selectors

    if hasattr(selectors, 'KqueueSelector'):
        class KqueueEventLoopTests(SendfileTestsBase,
                                   test_utils.TestCase):

            def create_event_loop(self):
                return asyncio.SelectorEventLoop(
                    selectors.KqueueSelector())

    if hasattr(selectors, 'EpollSelector'):
        class EPollEventLoopTests(SendfileTestsBase,
                                  test_utils.TestCase):

            def create_event_loop(self):
                return asyncio.SelectorEventLoop(selectors.EpollSelector())

    if hasattr(selectors, 'PollSelector'):
        class PollEventLoopTests(SendfileTestsBase,
                                 test_utils.TestCase):

            def create_event_loop(self):
                return asyncio.SelectorEventLoop(selectors.PollSelector())

    # Should always exist.
    class SelectEventLoopTests(SendfileTestsBase,
                               test_utils.TestCase):

        def create_event_loop(self):
            return asyncio.SelectorEventLoop(selectors.SelectSelector())


if __name__ == '__main__':
    unittest.main()
