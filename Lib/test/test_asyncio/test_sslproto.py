"""Tests for asyncio/sslproto.py."""

import logging
import socket
import sys
import unittest
import weakref
from unittest import mock
try:
    import ssl
except ImportError:
    ssl = None

import asyncio
from asyncio import log
from asyncio import protocols
from asyncio import sslproto
from asyncio import tasks
from test.test_asyncio import utils as test_utils
from test.test_asyncio import functional as func_tests


def tearDownModule():
    asyncio.set_event_loop_policy(None)


@unittest.skipIf(ssl is None, 'No ssl module')
class SslProtoHandshakeTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = asyncio.new_event_loop()
        self.set_event_loop(self.loop)

    def ssl_protocol(self, *, waiter=None, proto=None):
        sslcontext = test_utils.dummy_ssl_context()
        if proto is None:  # app protocol
            proto = asyncio.Protocol()
        ssl_proto = sslproto.SSLProtocol(self.loop, proto, sslcontext, waiter,
                                         ssl_handshake_timeout=0.1)
        self.assertIs(ssl_proto._app_transport.get_protocol(), proto)
        self.addCleanup(ssl_proto._app_transport.close)
        return ssl_proto

    def connection_made(self, ssl_proto, *, do_handshake=None):
        transport = mock.Mock()
        sslpipe = mock.Mock()
        sslpipe.shutdown.return_value = b''
        if do_handshake:
            sslpipe.do_handshake.side_effect = do_handshake
        else:
            def mock_handshake(callback):
                return []
            sslpipe.do_handshake.side_effect = mock_handshake
        with mock.patch('asyncio.sslproto._SSLPipe', return_value=sslpipe):
            ssl_proto.connection_made(transport)
        return transport

    def test_handshake_timeout_zero(self):
        sslcontext = test_utils.dummy_ssl_context()
        app_proto = mock.Mock()
        waiter = mock.Mock()
        with self.assertRaisesRegex(ValueError, 'a positive number'):
            sslproto.SSLProtocol(self.loop, app_proto, sslcontext, waiter,
                                 ssl_handshake_timeout=0)

    def test_handshake_timeout_negative(self):
        sslcontext = test_utils.dummy_ssl_context()
        app_proto = mock.Mock()
        waiter = mock.Mock()
        with self.assertRaisesRegex(ValueError, 'a positive number'):
            sslproto.SSLProtocol(self.loop, app_proto, sslcontext, waiter,
                                 ssl_handshake_timeout=-10)

    def test_eof_received_waiter(self):
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = self.ssl_protocol(waiter=waiter)
        self.connection_made(ssl_proto)
        ssl_proto.eof_received()
        test_utils.run_briefly(self.loop)
        self.assertIsInstance(waiter.exception(), ConnectionResetError)

    def test_fatal_error_no_name_error(self):
        # From issue #363.
        # _fatal_error() generates a NameError if sslproto.py
        # does not import base_events.
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = self.ssl_protocol(waiter=waiter)
        # Temporarily turn off error logging so as not to spoil test output.
        log_level = log.logger.getEffectiveLevel()
        log.logger.setLevel(logging.FATAL)
        try:
            ssl_proto._fatal_error(None)
        finally:
            # Restore error logging.
            log.logger.setLevel(log_level)

    def test_connection_lost(self):
        # From issue #472.
        # yield from waiter hang if lost_connection was called.
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = self.ssl_protocol(waiter=waiter)
        self.connection_made(ssl_proto)
        ssl_proto.connection_lost(ConnectionAbortedError)
        test_utils.run_briefly(self.loop)
        self.assertIsInstance(waiter.exception(), ConnectionAbortedError)

    def test_close_during_handshake(self):
        # bpo-29743 Closing transport during handshake process leaks socket
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = self.ssl_protocol(waiter=waiter)

        transport = self.connection_made(ssl_proto)
        test_utils.run_briefly(self.loop)

        ssl_proto._app_transport.close()
        self.assertTrue(transport.abort.called)

    def test_get_extra_info_on_closed_connection(self):
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = self.ssl_protocol(waiter=waiter)
        self.assertIsNone(ssl_proto._get_extra_info('socket'))
        default = object()
        self.assertIs(ssl_proto._get_extra_info('socket', default), default)
        self.connection_made(ssl_proto)
        self.assertIsNotNone(ssl_proto._get_extra_info('socket'))
        ssl_proto.connection_lost(None)
        self.assertIsNone(ssl_proto._get_extra_info('socket'))

    def test_set_new_app_protocol(self):
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = self.ssl_protocol(waiter=waiter)
        new_app_proto = asyncio.Protocol()
        ssl_proto._app_transport.set_protocol(new_app_proto)
        self.assertIs(ssl_proto._app_transport.get_protocol(), new_app_proto)
        self.assertIs(ssl_proto._app_protocol, new_app_proto)

    def test_data_received_after_closing(self):
        ssl_proto = self.ssl_protocol()
        self.connection_made(ssl_proto)
        transp = ssl_proto._app_transport

        transp.close()

        # should not raise
        self.assertIsNone(ssl_proto.data_received(b'data'))

    def test_write_after_closing(self):
        ssl_proto = self.ssl_protocol()
        self.connection_made(ssl_proto)
        transp = ssl_proto._app_transport
        transp.close()

        # should not raise
        self.assertIsNone(transp.write(b'data'))


##############################################################################
# Start TLS Tests
##############################################################################


class BaseStartTLS(func_tests.FunctionalTestCaseMixin):

    PAYLOAD_SIZE = 1024 * 100
    TIMEOUT = 60

    def new_loop(self):
        raise NotImplementedError

    def test_buf_feed_data(self):

        class Proto(asyncio.BufferedProtocol):

            def __init__(self, bufsize, usemv):
                self.buf = bytearray(bufsize)
                self.mv = memoryview(self.buf)
                self.data = b''
                self.usemv = usemv

            def get_buffer(self, sizehint):
                if self.usemv:
                    return self.mv
                else:
                    return self.buf

            def buffer_updated(self, nsize):
                if self.usemv:
                    self.data += self.mv[:nsize]
                else:
                    self.data += self.buf[:nsize]

        for usemv in [False, True]:
            proto = Proto(1, usemv)
            protocols._feed_data_to_buffered_proto(proto, b'12345')
            self.assertEqual(proto.data, b'12345')

            proto = Proto(2, usemv)
            protocols._feed_data_to_buffered_proto(proto, b'12345')
            self.assertEqual(proto.data, b'12345')

            proto = Proto(2, usemv)
            protocols._feed_data_to_buffered_proto(proto, b'1234')
            self.assertEqual(proto.data, b'1234')

            proto = Proto(4, usemv)
            protocols._feed_data_to_buffered_proto(proto, b'1234')
            self.assertEqual(proto.data, b'1234')

            proto = Proto(100, usemv)
            protocols._feed_data_to_buffered_proto(proto, b'12345')
            self.assertEqual(proto.data, b'12345')

            proto = Proto(0, usemv)
            with self.assertRaisesRegex(RuntimeError, 'empty buffer'):
                protocols._feed_data_to_buffered_proto(proto, b'12345')

    def test_start_tls_client_reg_proto_1(self):
        HELLO_MSG = b'1' * self.PAYLOAD_SIZE

        server_context = test_utils.simple_server_sslcontext()
        client_context = test_utils.simple_client_sslcontext()

        def serve(sock):
            sock.settimeout(self.TIMEOUT)

            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.start_tls(server_context, server_side=True)

            sock.sendall(b'O')
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.shutdown(socket.SHUT_RDWR)
            sock.close()

        class ClientProto(asyncio.Protocol):
            def __init__(self, on_data, on_eof):
                self.on_data = on_data
                self.on_eof = on_eof
                self.con_made_cnt = 0

            def connection_made(proto, tr):
                proto.con_made_cnt += 1
                # Ensure connection_made gets called only once.
                self.assertEqual(proto.con_made_cnt, 1)

            def data_received(self, data):
                self.on_data.set_result(data)

            def eof_received(self):
                self.on_eof.set_result(True)

        async def client(addr):
            await asyncio.sleep(0.5)

            on_data = self.loop.create_future()
            on_eof = self.loop.create_future()

            tr, proto = await self.loop.create_connection(
                lambda: ClientProto(on_data, on_eof), *addr)

            tr.write(HELLO_MSG)
            new_tr = await self.loop.start_tls(tr, proto, client_context)

            self.assertEqual(await on_data, b'O')
            new_tr.write(HELLO_MSG)
            await on_eof

            new_tr.close()

        with self.tcp_server(serve, timeout=self.TIMEOUT) as srv:
            self.loop.run_until_complete(
                asyncio.wait_for(client(srv.addr), timeout=10))

        # No garbage is left if SSL is closed uncleanly
        client_context = weakref.ref(client_context)
        self.assertIsNone(client_context())

    def test_create_connection_memory_leak(self):
        HELLO_MSG = b'1' * self.PAYLOAD_SIZE

        server_context = test_utils.simple_server_sslcontext()
        client_context = test_utils.simple_client_sslcontext()

        def serve(sock):
            sock.settimeout(self.TIMEOUT)

            sock.start_tls(server_context, server_side=True)

            sock.sendall(b'O')
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.shutdown(socket.SHUT_RDWR)
            sock.close()

        class ClientProto(asyncio.Protocol):
            def __init__(self, on_data, on_eof):
                self.on_data = on_data
                self.on_eof = on_eof
                self.con_made_cnt = 0

            def connection_made(proto, tr):
                # XXX: We assume user stores the transport in protocol
                proto.tr = tr
                proto.con_made_cnt += 1
                # Ensure connection_made gets called only once.
                self.assertEqual(proto.con_made_cnt, 1)

            def data_received(self, data):
                self.on_data.set_result(data)

            def eof_received(self):
                self.on_eof.set_result(True)

        async def client(addr):
            await asyncio.sleep(0.5)

            on_data = self.loop.create_future()
            on_eof = self.loop.create_future()

            tr, proto = await self.loop.create_connection(
                lambda: ClientProto(on_data, on_eof), *addr,
                ssl=client_context)

            self.assertEqual(await on_data, b'O')
            tr.write(HELLO_MSG)
            await on_eof

            tr.close()

        with self.tcp_server(serve, timeout=self.TIMEOUT) as srv:
            self.loop.run_until_complete(
                asyncio.wait_for(client(srv.addr), timeout=10))

        # No garbage is left for SSL client from loop.create_connection, even
        # if user stores the SSLTransport in corresponding protocol instance
        client_context = weakref.ref(client_context)
        self.assertIsNone(client_context())

    def test_start_tls_client_buf_proto_1(self):
        HELLO_MSG = b'1' * self.PAYLOAD_SIZE

        server_context = test_utils.simple_server_sslcontext()
        client_context = test_utils.simple_client_sslcontext()
        client_con_made_calls = 0

        def serve(sock):
            sock.settimeout(self.TIMEOUT)

            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.start_tls(server_context, server_side=True)

            sock.sendall(b'O')
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.sendall(b'2')
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.shutdown(socket.SHUT_RDWR)
            sock.close()

        class ClientProtoFirst(asyncio.BufferedProtocol):
            def __init__(self, on_data):
                self.on_data = on_data
                self.buf = bytearray(1)

            def connection_made(self, tr):
                nonlocal client_con_made_calls
                client_con_made_calls += 1

            def get_buffer(self, sizehint):
                return self.buf

            def buffer_updated(self, nsize):
                assert nsize == 1
                self.on_data.set_result(bytes(self.buf[:nsize]))

        class ClientProtoSecond(asyncio.Protocol):
            def __init__(self, on_data, on_eof):
                self.on_data = on_data
                self.on_eof = on_eof
                self.con_made_cnt = 0

            def connection_made(self, tr):
                nonlocal client_con_made_calls
                client_con_made_calls += 1

            def data_received(self, data):
                self.on_data.set_result(data)

            def eof_received(self):
                self.on_eof.set_result(True)

        async def client(addr):
            await asyncio.sleep(0.5)

            on_data1 = self.loop.create_future()
            on_data2 = self.loop.create_future()
            on_eof = self.loop.create_future()

            tr, proto = await self.loop.create_connection(
                lambda: ClientProtoFirst(on_data1), *addr)

            tr.write(HELLO_MSG)
            new_tr = await self.loop.start_tls(tr, proto, client_context)

            self.assertEqual(await on_data1, b'O')
            new_tr.write(HELLO_MSG)

            new_tr.set_protocol(ClientProtoSecond(on_data2, on_eof))
            self.assertEqual(await on_data2, b'2')
            new_tr.write(HELLO_MSG)
            await on_eof

            new_tr.close()

            # connection_made() should be called only once -- when
            # we establish connection for the first time. Start TLS
            # doesn't call connection_made() on application protocols.
            self.assertEqual(client_con_made_calls, 1)

        with self.tcp_server(serve, timeout=self.TIMEOUT) as srv:
            self.loop.run_until_complete(
                asyncio.wait_for(client(srv.addr),
                                 timeout=self.TIMEOUT))

    def test_start_tls_slow_client_cancel(self):
        HELLO_MSG = b'1' * self.PAYLOAD_SIZE

        client_context = test_utils.simple_client_sslcontext()
        server_waits_on_handshake = self.loop.create_future()

        def serve(sock):
            sock.settimeout(self.TIMEOUT)

            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            try:
                self.loop.call_soon_threadsafe(
                    server_waits_on_handshake.set_result, None)
                data = sock.recv_all(1024 * 1024)
            except ConnectionAbortedError:
                pass
            finally:
                sock.close()

        class ClientProto(asyncio.Protocol):
            def __init__(self, on_data, on_eof):
                self.on_data = on_data
                self.on_eof = on_eof
                self.con_made_cnt = 0

            def connection_made(proto, tr):
                proto.con_made_cnt += 1
                # Ensure connection_made gets called only once.
                self.assertEqual(proto.con_made_cnt, 1)

            def data_received(self, data):
                self.on_data.set_result(data)

            def eof_received(self):
                self.on_eof.set_result(True)

        async def client(addr):
            await asyncio.sleep(0.5)

            on_data = self.loop.create_future()
            on_eof = self.loop.create_future()

            tr, proto = await self.loop.create_connection(
                lambda: ClientProto(on_data, on_eof), *addr)

            tr.write(HELLO_MSG)

            await server_waits_on_handshake

            with self.assertRaises(asyncio.TimeoutError):
                await asyncio.wait_for(
                    self.loop.start_tls(tr, proto, client_context),
                    0.5)

        with self.tcp_server(serve, timeout=self.TIMEOUT) as srv:
            self.loop.run_until_complete(
                asyncio.wait_for(client(srv.addr), timeout=10))

    def test_start_tls_server_1(self):
        HELLO_MSG = b'1' * self.PAYLOAD_SIZE
        ANSWER = b'answer'

        server_context = test_utils.simple_server_sslcontext()
        client_context = test_utils.simple_client_sslcontext()
        if sys.platform.startswith('freebsd') or sys.platform.startswith('win'):
            # bpo-35031: Some FreeBSD and Windows buildbots fail to run this test
            # as the eof was not being received by the server if the payload
            # size is not big enough. This behaviour only appears if the
            # client is using TLS1.3.
            client_context.options |= ssl.OP_NO_TLSv1_3
        answer = None

        def client(sock, addr):
            nonlocal answer
            sock.settimeout(self.TIMEOUT)

            sock.connect(addr)
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.start_tls(client_context)
            sock.sendall(HELLO_MSG)
            answer = sock.recv_all(len(ANSWER))
            sock.close()

        class ServerProto(asyncio.Protocol):
            def __init__(self, on_con, on_con_lost):
                self.on_con = on_con
                self.on_con_lost = on_con_lost
                self.data = b''
                self.transport = None

            def connection_made(self, tr):
                self.transport = tr
                self.on_con.set_result(tr)

            def replace_transport(self, tr):
                self.transport = tr

            def data_received(self, data):
                self.data += data
                if len(self.data) >= len(HELLO_MSG):
                    self.transport.write(ANSWER)

            def connection_lost(self, exc):
                self.transport = None
                if exc is None:
                    self.on_con_lost.set_result(None)
                else:
                    self.on_con_lost.set_exception(exc)

        async def main(proto, on_con, on_con_lost):
            tr = await on_con
            tr.write(HELLO_MSG)

            self.assertEqual(proto.data, b'')

            new_tr = await self.loop.start_tls(
                tr, proto, server_context,
                server_side=True,
                ssl_handshake_timeout=self.TIMEOUT)

            proto.replace_transport(new_tr)

            await on_con_lost
            self.assertEqual(proto.data, HELLO_MSG)
            new_tr.close()

        async def run_main():
            on_con = self.loop.create_future()
            on_con_lost = self.loop.create_future()
            proto = ServerProto(on_con, on_con_lost)

            server = await self.loop.create_server(
                lambda: proto, '127.0.0.1', 0)
            addr = server.sockets[0].getsockname()

            with self.tcp_client(lambda sock: client(sock, addr),
                                 timeout=self.TIMEOUT):
                await asyncio.wait_for(
                    main(proto, on_con, on_con_lost),
                    timeout=self.TIMEOUT)

            server.close()
            await server.wait_closed()
            self.assertEqual(answer, ANSWER)

        self.loop.run_until_complete(run_main())

    def test_start_tls_wrong_args(self):
        async def main():
            with self.assertRaisesRegex(TypeError, 'SSLContext, got'):
                await self.loop.start_tls(None, None, None)

            sslctx = test_utils.simple_server_sslcontext()
            with self.assertRaisesRegex(TypeError, 'is not supported'):
                await self.loop.start_tls(None, None, sslctx)

        self.loop.run_until_complete(main())

    def test_handshake_timeout(self):
        # bpo-29970: Check that a connection is aborted if handshake is not
        # completed in timeout period, instead of remaining open indefinitely
        client_sslctx = test_utils.simple_client_sslcontext()

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        server_side_aborted = False

        def server(sock):
            nonlocal server_side_aborted
            try:
                sock.recv_all(1024 * 1024)
            except ConnectionAbortedError:
                server_side_aborted = True
            finally:
                sock.close()

        async def client(addr):
            await asyncio.wait_for(
                self.loop.create_connection(
                    asyncio.Protocol,
                    *addr,
                    ssl=client_sslctx,
                    server_hostname='',
                    ssl_handshake_timeout=10.0),
                0.5)

        with self.tcp_server(server,
                             max_clients=1,
                             backlog=1) as srv:

            with self.assertRaises(asyncio.TimeoutError):
                self.loop.run_until_complete(client(srv.addr))

        self.assertTrue(server_side_aborted)

        # Python issue #23197: cancelling a handshake must not raise an
        # exception or log an error, even if the handshake failed
        self.assertEqual(messages, [])

        # The 10s handshake timeout should be cancelled to free related
        # objects without really waiting for 10s
        client_sslctx = weakref.ref(client_sslctx)
        self.assertIsNone(client_sslctx())

    def test_create_connection_ssl_slow_handshake(self):
        client_sslctx = test_utils.simple_client_sslcontext()

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        def server(sock):
            try:
                sock.recv_all(1024 * 1024)
            except ConnectionAbortedError:
                pass
            finally:
                sock.close()

        async def client(addr):
            with self.assertWarns(DeprecationWarning):
                reader, writer = await asyncio.open_connection(
                    *addr,
                    ssl=client_sslctx,
                    server_hostname='',
                    loop=self.loop,
                    ssl_handshake_timeout=1.0)

        with self.tcp_server(server,
                             max_clients=1,
                             backlog=1) as srv:

            with self.assertRaisesRegex(
                    ConnectionAbortedError,
                    r'SSL handshake.*is taking longer'):

                self.loop.run_until_complete(client(srv.addr))

        self.assertEqual(messages, [])

    def test_create_connection_ssl_failed_certificate(self):
        self.loop.set_exception_handler(lambda loop, ctx: None)

        sslctx = test_utils.simple_server_sslcontext()
        client_sslctx = test_utils.simple_client_sslcontext(
            disable_verify=False)

        def server(sock):
            try:
                sock.start_tls(
                    sslctx,
                    server_side=True)
            except ssl.SSLError:
                pass
            except OSError:
                pass
            finally:
                sock.close()

        async def client(addr):
            with self.assertWarns(DeprecationWarning):
                reader, writer = await asyncio.open_connection(
                    *addr,
                    ssl=client_sslctx,
                    server_hostname='',
                    loop=self.loop,
                    ssl_handshake_timeout=1.0)

        with self.tcp_server(server,
                             max_clients=1,
                             backlog=1) as srv:

            with self.assertRaises(ssl.SSLCertVerificationError):
                self.loop.run_until_complete(client(srv.addr))

    def test_start_tls_client_corrupted_ssl(self):
        self.loop.set_exception_handler(lambda loop, ctx: None)

        sslctx = test_utils.simple_server_sslcontext()
        client_sslctx = test_utils.simple_client_sslcontext()

        def server(sock):
            orig_sock = sock.dup()
            try:
                sock.start_tls(
                    sslctx,
                    server_side=True)
                sock.sendall(b'A\n')
                sock.recv_all(1)
                orig_sock.send(b'please corrupt the SSL connection')
            except ssl.SSLError:
                pass
            finally:
                orig_sock.close()
                sock.close()

        async def client(addr):
            with self.assertWarns(DeprecationWarning):
                reader, writer = await asyncio.open_connection(
                    *addr,
                    ssl=client_sslctx,
                    server_hostname='',
                    loop=self.loop)

            self.assertEqual(await reader.readline(), b'A\n')
            writer.write(b'B')
            with self.assertRaises(ssl.SSLError):
                await reader.readline()

            writer.close()
            return 'OK'

        with self.tcp_server(server,
                             max_clients=1,
                             backlog=1) as srv:

            res = self.loop.run_until_complete(client(srv.addr))

        self.assertEqual(res, 'OK')


@unittest.skipIf(ssl is None, 'No ssl module')
class SelectorStartTLSTests(BaseStartTLS, unittest.TestCase):

    def new_loop(self):
        return asyncio.SelectorEventLoop()


@unittest.skipIf(ssl is None, 'No ssl module')
@unittest.skipUnless(hasattr(asyncio, 'ProactorEventLoop'), 'Windows only')
class ProactorStartTLSTests(BaseStartTLS, unittest.TestCase):

    def new_loop(self):
        return asyncio.ProactorEventLoop()


if __name__ == '__main__':
    unittest.main()
