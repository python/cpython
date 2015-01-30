"""Tests for asyncio/sslproto.py."""

import unittest
from unittest import mock
try:
    import ssl
except ImportError:
    ssl = None

import asyncio
from asyncio import sslproto
from asyncio import test_utils


@unittest.skipIf(ssl is None, 'No ssl module')
class SslProtoHandshakeTests(test_utils.TestCase):

    def setUp(self):
        self.loop = asyncio.new_event_loop()
        self.set_event_loop(self.loop)

    def ssl_protocol(self, waiter=None):
        sslcontext = test_utils.dummy_ssl_context()
        app_proto = asyncio.Protocol()
        proto = sslproto.SSLProtocol(self.loop, app_proto, sslcontext, waiter)
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

    def test_cancel_handshake(self):
        # Python issue #23197: cancelling an handshake must not raise an
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


if __name__ == '__main__':
    unittest.main()
