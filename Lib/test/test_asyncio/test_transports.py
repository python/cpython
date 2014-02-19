"""Tests for transports.py."""

import unittest
import unittest.mock

import asyncio
from asyncio import transports


class TransportTests(unittest.TestCase):

    def test_ctor_extra_is_none(self):
        transport = asyncio.Transport()
        self.assertEqual(transport._extra, {})

    def test_get_extra_info(self):
        transport = asyncio.Transport({'extra': 'info'})
        self.assertEqual('info', transport.get_extra_info('extra'))
        self.assertIsNone(transport.get_extra_info('unknown'))

        default = object()
        self.assertIs(default, transport.get_extra_info('unknown', default))

    def test_writelines(self):
        transport = asyncio.Transport()
        transport.write = unittest.mock.Mock()

        transport.writelines([b'line1',
                              bytearray(b'line2'),
                              memoryview(b'line3')])
        self.assertEqual(1, transport.write.call_count)
        transport.write.assert_called_with(b'line1line2line3')

    def test_not_implemented(self):
        transport = asyncio.Transport()

        self.assertRaises(NotImplementedError,
                          transport.set_write_buffer_limits)
        self.assertRaises(NotImplementedError, transport.get_write_buffer_size)
        self.assertRaises(NotImplementedError, transport.write, 'data')
        self.assertRaises(NotImplementedError, transport.write_eof)
        self.assertRaises(NotImplementedError, transport.can_write_eof)
        self.assertRaises(NotImplementedError, transport.pause_reading)
        self.assertRaises(NotImplementedError, transport.resume_reading)
        self.assertRaises(NotImplementedError, transport.close)
        self.assertRaises(NotImplementedError, transport.abort)

    def test_dgram_not_implemented(self):
        transport = asyncio.DatagramTransport()

        self.assertRaises(NotImplementedError, transport.sendto, 'data')
        self.assertRaises(NotImplementedError, transport.abort)

    def test_subprocess_transport_not_implemented(self):
        transport = asyncio.SubprocessTransport()

        self.assertRaises(NotImplementedError, transport.get_pid)
        self.assertRaises(NotImplementedError, transport.get_returncode)
        self.assertRaises(NotImplementedError, transport.get_pipe_transport, 1)
        self.assertRaises(NotImplementedError, transport.send_signal, 1)
        self.assertRaises(NotImplementedError, transport.terminate)
        self.assertRaises(NotImplementedError, transport.kill)

    def test_flowcontrol_mixin_set_write_limits(self):

        class MyTransport(transports._FlowControlMixin,
                          transports.Transport):

            def get_write_buffer_size(self):
                return 512

        transport = MyTransport()
        transport._protocol = unittest.mock.Mock()

        self.assertFalse(transport._protocol_paused)

        with self.assertRaisesRegex(ValueError, 'high.*must be >= low'):
            transport.set_write_buffer_limits(high=0, low=1)

        transport.set_write_buffer_limits(high=1024, low=128)
        self.assertFalse(transport._protocol_paused)

        transport.set_write_buffer_limits(high=256, low=128)
        self.assertTrue(transport._protocol_paused)


if __name__ == '__main__':
    unittest.main()
