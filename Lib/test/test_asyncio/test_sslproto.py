"""Tests for asyncio/sslproto.py."""

import unittest
from unittest import mock

import asyncio
from asyncio import sslproto
from asyncio import test_utils


class SslProtoHandshakeTests(test_utils.TestCase):

    def setUp(self):
        self.loop = asyncio.new_event_loop()
        self.set_event_loop(self.loop)

    def test_cancel_handshake(self):
        # Python issue #23197: cancelling an handshake must not raise an
        # exception or log an error, even if the handshake failed
        sslcontext = test_utils.dummy_ssl_context()
        app_proto = asyncio.Protocol()
        waiter = asyncio.Future(loop=self.loop)
        ssl_proto = sslproto.SSLProtocol(self.loop, app_proto, sslcontext,
                                         waiter)
        handshake_fut = asyncio.Future(loop=self.loop)

        def do_handshake(callback):
            exc = Exception()
            callback(exc)
            handshake_fut.set_result(None)
            return []

        waiter.cancel()
        transport = mock.Mock()
        sslpipe = mock.Mock()
        sslpipe.shutdown.return_value = b''
        sslpipe.do_handshake.side_effect = do_handshake
        with mock.patch('asyncio.sslproto._SSLPipe', return_value=sslpipe):
            ssl_proto.connection_made(transport)

        with test_utils.disable_logger():
            self.loop.run_until_complete(handshake_fut)

        # Close the transport
        ssl_proto._app_transport.close()


if __name__ == '__main__':
    unittest.main()
