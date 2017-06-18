"""Tests for asyncio/sslproto.py."""
import contextlib
import logging
import os
import socket
import threading
import unittest
from unittest import mock


try:
    import ssl
except ImportError:
    ssl = None

import asyncio
from asyncio import log
from asyncio import sslproto
from asyncio import test_utils

HOST = '127.0.0.1'


def data_file(name):
    return os.path.join(os.path.dirname(__file__), name)


class DummySSLServer(threading.Thread):
    class Protocol(asyncio.Protocol):
        transport = None

        def connection_lost(self, exc):
            self.transport.close()

        def connection_made(self, transport):
            self.transport = transport

    def __init__(self):
        super().__init__()

        self.loop = asyncio.new_event_loop()
        context = ssl.SSLContext()
        context.load_cert_chain(data_file('keycert3.pem'))
        server_future = self.loop.create_server(
            self.Protocol, *(HOST, 0),
            ssl=context)
        self.server = self.loop.run_until_complete(server_future)

    def run(self):
        self.loop.run_forever()
        self.server.close()
        self.loop.run_until_complete(self.server.wait_closed())
        self.loop.close()

    def stop(self):
        self.loop.call_soon_threadsafe(self.loop.stop)


@contextlib.contextmanager
def run_test_ssl_server():
    th = DummySSLServer()
    th.start()
    try:
        yield th.server
    finally:
        th.stop()
        th.join()



@unittest.skipIf(ssl is None, 'No ssl module')
class SslProtoHandshakeTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = asyncio.new_event_loop()
        self.set_event_loop(self.loop)

    def ssl_protocol(self, waiter=None):
        sslcontext = test_utils.dummy_ssl_context()
        app_proto = asyncio.Protocol()
        proto = sslproto.SSLProtocol(self.loop, app_proto, sslcontext, waiter)
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

    def test_ssl_shutdown(self):
        # bpo-30698 Shutdown the ssl layer cleanly
        with run_test_ssl_server() as server:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(server.sockets[0].getsockname())
            ssl_socket = test_utils.dummy_ssl_context().wrap_socket(sock)
            with contextlib.closing(ssl_socket):
                ssl_socket.unwrap()


if __name__ == '__main__':
    unittest.main()
