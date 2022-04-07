import unittest
from unittest import mock

import asyncio


def tearDownModule():
    # not needed for the test file but added for uniformness with all other
    # asyncio test files for the sake of unified cleanup
    asyncio.set_event_loop_policy(None)


class ProtocolsAbsTests(unittest.TestCase):

    def test_base_protocol(self):
        f = mock.Mock()
        p = asyncio.BaseProtocol()
        self.assertIsNone(p.connection_made(f))
        self.assertIsNone(p.connection_lost(f))
        self.assertIsNone(p.pause_writing())
        self.assertIsNone(p.resume_writing())
        self.assertFalse(hasattr(p, '__dict__'))

    def test_protocol(self):
        f = mock.Mock()
        p = asyncio.Protocol()
        self.assertIsNone(p.connection_made(f))
        self.assertIsNone(p.connection_lost(f))
        self.assertIsNone(p.data_received(f))
        self.assertIsNone(p.eof_received())
        self.assertIsNone(p.pause_writing())
        self.assertIsNone(p.resume_writing())
        self.assertFalse(hasattr(p, '__dict__'))

    def test_buffered_protocol(self):
        f = mock.Mock()
        p = asyncio.BufferedProtocol()
        self.assertIsNone(p.connection_made(f))
        self.assertIsNone(p.connection_lost(f))
        self.assertIsNone(p.get_buffer(100))
        self.assertIsNone(p.buffer_updated(150))
        self.assertIsNone(p.pause_writing())
        self.assertIsNone(p.resume_writing())
        self.assertFalse(hasattr(p, '__dict__'))

    def test_datagram_protocol(self):
        f = mock.Mock()
        dp = asyncio.DatagramProtocol()
        self.assertIsNone(dp.connection_made(f))
        self.assertIsNone(dp.connection_lost(f))
        self.assertIsNone(dp.error_received(f))
        self.assertIsNone(dp.datagram_received(f, f))
        self.assertFalse(hasattr(dp, '__dict__'))

    def test_subprocess_protocol(self):
        f = mock.Mock()
        sp = asyncio.SubprocessProtocol()
        self.assertIsNone(sp.connection_made(f))
        self.assertIsNone(sp.connection_lost(f))
        self.assertIsNone(sp.pipe_data_received(1, f))
        self.assertIsNone(sp.pipe_connection_lost(1, f))
        self.assertIsNone(sp.process_exited())
        self.assertFalse(hasattr(sp, '__dict__'))


if __name__ == '__main__':
    unittest.main()
