"""Tests for asyncio/sslproto.py."""

import os
import logging
import unittest
from unittest import mock
try:
    import ssl
except ImportError:
    ssl = None

import asyncio
from asyncio import log
from asyncio import sslproto
from asyncio import tasks
from test.test_asyncio import utils as test_utils
from test.test_asyncio import functional as func_tests


@unittest.skipIf(ssl is None, 'No ssl module')
class SslProtoHandshakeTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = asyncio.new_event_loop()
        self.set_event_loop(self.loop)

    def ssl_protocol(self, waiter=None):
        sslcontext = test_utils.dummy_ssl_context()
        app_proto = asyncio.Protocol()
        proto = sslproto.SSLProtocol(self.loop, app_proto, sslcontext, waiter,
                                     ssl_handshake_timeout=0.1)
        self.assertIs(proto._app_transport.get_protocol(), app_proto)
        self.addCleanup(proto._app_transport.close)
        return proto

    def connection_made(self, ssl_proto, do_handshake=None):
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

    def test_cancel_handshake(self):
        # Python issue #23197: cancelling a handshake must not raise an
        # exception or log an error, even if the handshake failed
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = self.ssl_protocol(waiter)
        handshake_fut = asyncio.Future(loop=self.loop)

        def do_handshake(callback):
            exc = Exception()
            callback(exc)
            handshake_fut.set_result(None)
            return []

        waiter.cancel()
        self.connection_made(ssl_proto, do_handshake)

        with test_utils.disable_logger():
            self.loop.run_until_complete(handshake_fut)

    def test_handshake_timeout(self):
        # bpo-29970: Check that a connection is aborted if handshake is not
        # completed in timeout period, instead of remaining open indefinitely
        ssl_proto = self.ssl_protocol()
        transport = self.connection_made(ssl_proto)

        with test_utils.disable_logger():
            self.loop.run_until_complete(tasks.sleep(0.2, loop=self.loop))
        self.assertTrue(transport.abort.called)

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
        ssl_proto = self.ssl_protocol(waiter)
        self.connection_made(ssl_proto)
        ssl_proto.eof_received()
        test_utils.run_briefly(self.loop)
        self.assertIsInstance(waiter.exception(), ConnectionResetError)

    def test_fatal_error_no_name_error(self):
        # From issue #363.
        # _fatal_error() generates a NameError if sslproto.py
        # does not import base_events.
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = self.ssl_protocol(waiter)
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
        ssl_proto = self.ssl_protocol(waiter)
        self.connection_made(ssl_proto)
        ssl_proto.connection_lost(ConnectionAbortedError)
        test_utils.run_briefly(self.loop)
        self.assertIsInstance(waiter.exception(), ConnectionAbortedError)

    def test_close_during_handshake(self):
        # bpo-29743 Closing transport during handshake process leaks socket
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = self.ssl_protocol(waiter)

        def do_handshake(callback):
            return []

        transport = self.connection_made(ssl_proto)
        test_utils.run_briefly(self.loop)

        ssl_proto._app_transport.close()
        self.assertTrue(transport.abort.called)

    def test_get_extra_info_on_closed_connection(self):
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = self.ssl_protocol(waiter)
        self.assertIsNone(ssl_proto._get_extra_info('socket'))
        default = object()
        self.assertIs(ssl_proto._get_extra_info('socket', default), default)
        self.connection_made(ssl_proto)
        self.assertIsNotNone(ssl_proto._get_extra_info('socket'))
        ssl_proto.connection_lost(None)
        self.assertIsNone(ssl_proto._get_extra_info('socket'))

    def test_set_new_app_protocol(self):
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = self.ssl_protocol(waiter)
        new_app_proto = asyncio.Protocol()
        ssl_proto._app_transport.set_protocol(new_app_proto)
        self.assertIs(ssl_proto._app_transport.get_protocol(), new_app_proto)
        self.assertIs(ssl_proto._app_protocol, new_app_proto)


##############################################################################
# Start TLS Tests
##############################################################################


class BaseStartTLS(func_tests.FunctionalTestCaseMixin):

    def new_loop(self):
        raise NotImplementedError

    def test_start_tls_client_1(self):
        HELLO_MSG = b'1' * 1024 * 1024 * 5

        server_context = test_utils.simple_server_sslcontext()
        client_context = test_utils.simple_client_sslcontext()

        def serve(sock):
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.start_tls(server_context, server_side=True)

            sock.sendall(b'O')
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))
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

        with self.tcp_server(serve) as srv:
            self.loop.run_until_complete(
                asyncio.wait_for(client(srv.addr), loop=self.loop, timeout=10))

    def test_start_tls_server_1(self):
        HELLO_MSG = b'1' * 1024 * 1024 * 5

        server_context = test_utils.simple_server_sslcontext()
        client_context = test_utils.simple_client_sslcontext()

        def client(sock, addr):
            sock.connect(addr)
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.start_tls(client_context)
            sock.sendall(HELLO_MSG)
            sock.close()

        class ServerProto(asyncio.Protocol):
            def __init__(self, on_con, on_eof):
                self.on_con = on_con
                self.on_eof = on_eof
                self.data = b''

            def connection_made(self, tr):
                self.on_con.set_result(tr)

            def data_received(self, data):
                self.data += data

            def eof_received(self):
                self.on_eof.set_result(1)

        async def main():
            tr = await on_con
            tr.write(HELLO_MSG)

            self.assertEqual(proto.data, b'')

            new_tr = await self.loop.start_tls(
                tr, proto, server_context,
                server_side=True)

            await on_eof
            self.assertEqual(proto.data, HELLO_MSG)
            new_tr.close()

            server.close()
            await server.wait_closed()

        on_con = self.loop.create_future()
        on_eof = self.loop.create_future()
        proto = ServerProto(on_con, on_eof)

        server = self.loop.run_until_complete(
            self.loop.create_server(
                lambda: proto, '127.0.0.1', 0))
        addr = server.sockets[0].getsockname()

        with self.tcp_client(lambda sock: client(sock, addr)):
            self.loop.run_until_complete(
                asyncio.wait_for(main(), loop=self.loop, timeout=10))

    def test_start_tls_wrong_args(self):
        async def main():
            with self.assertRaisesRegex(TypeError, 'SSLContext, got'):
                await self.loop.start_tls(None, None, None)

            sslctx = test_utils.simple_server_sslcontext()
            with self.assertRaisesRegex(TypeError, 'is not supported'):
                await self.loop.start_tls(None, None, sslctx)

        self.loop.run_until_complete(main())


@unittest.skipIf(ssl is None, 'No ssl module')
class SelectorStartTLSTests(BaseStartTLS, unittest.TestCase):

    def new_loop(self):
        return asyncio.SelectorEventLoop()


@unittest.skipIf(ssl is None, 'No ssl module')
@unittest.skipUnless(hasattr(asyncio, 'ProactorEventLoop'), 'Windows only')
@unittest.skipIf(os.environ.get('APPVEYOR'), 'XXX: issue 32458')
class ProactorStartTLSTests(BaseStartTLS, unittest.TestCase):

    def new_loop(self):
        return asyncio.ProactorEventLoop()


if __name__ == '__main__':
    unittest.main()
