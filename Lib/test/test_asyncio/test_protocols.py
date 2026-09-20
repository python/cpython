import unittest
from unittest import mock

import asyncio
from asyncio import protocols


def tearDownModule():
    # not needed for the test file but added for uniformness with all other
    # asyncio test files for the sake of unified cleanup
    asyncio.set_event_loop(None)


class ProtocolsAbsTests(unittest.TestCase):

    def test_base_protocol(self):
        f = mock.Mock()
        p = asyncio.BaseProtocol()
        self.assertIsNone(p.connection_made(f))
        self.assertIsNone(p.connection_lost(f))
        self.assertIsNone(p.pause_writing())
        self.assertIsNone(p.resume_writing())
        self.assertNotHasAttr(p, '__dict__')

    def test_protocol(self):
        f = mock.Mock()
        p = asyncio.Protocol()
        self.assertIsNone(p.connection_made(f))
        self.assertIsNone(p.connection_lost(f))
        self.assertIsNone(p.data_received(f))
        self.assertIsNone(p.eof_received())
        self.assertIsNone(p.pause_writing())
        self.assertIsNone(p.resume_writing())
        self.assertNotHasAttr(p, '__dict__')

    def test_buffered_protocol(self):
        f = mock.Mock()
        p = asyncio.BufferedProtocol()
        self.assertIsNone(p.connection_made(f))
        self.assertIsNone(p.connection_lost(f))
        self.assertIsNone(p.get_buffer(100))
        self.assertIsNone(p.buffer_updated(150))
        self.assertIsNone(p.pause_writing())
        self.assertIsNone(p.resume_writing())
        self.assertNotHasAttr(p, '__dict__')

    def test_datagram_protocol(self):
        f = mock.Mock()
        dp = asyncio.DatagramProtocol()
        self.assertIsNone(dp.connection_made(f))
        self.assertIsNone(dp.connection_lost(f))
        self.assertIsNone(dp.error_received(f))
        self.assertIsNone(dp.datagram_received(f, f))
        self.assertNotHasAttr(dp, '__dict__')

    def test_subprocess_protocol(self):
        f = mock.Mock()
        sp = asyncio.SubprocessProtocol()
        self.assertIsNone(sp.connection_made(f))
        self.assertIsNone(sp.connection_lost(f))
        self.assertIsNone(sp.pipe_data_received(1, f))
        self.assertIsNone(sp.pipe_connection_lost(1, f))
        self.assertIsNone(sp.process_exited())
        self.assertNotHasAttr(sp, '__dict__')


class FeedDataToBufferedProtoTests(unittest.TestCase):
    def _make_proto(self, bufsize):
        received = bytearray()
        buf = bytearray(bufsize)

        class P(asyncio.BufferedProtocol):
            def get_buffer(self, sizehint):
                return buf

            def buffer_updated(self, nbytes):
                received.extend(buf[:nbytes])

        return P(), received

    def test_large_multi_iteration(self):
        proto, received = self._make_proto(64)
        data = bytes(range(256)) * 16
        protocols._feed_data_to_buffered_proto(proto, data)
        self.assertEqual(bytes(received), data)

    def test_memoryview_input(self):
        proto, received = self._make_proto(64)
        payload = b"y" * 200
        protocols._feed_data_to_buffered_proto(proto, memoryview(payload))
        self.assertEqual(bytes(received), payload)


if __name__ == "__main__":
    unittest.main()
