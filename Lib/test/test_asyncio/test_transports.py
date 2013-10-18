"""Tests for transports.py."""

import unittest
import unittest.mock

from asyncio import transports


class TransportTests(unittest.TestCase):

    def test_ctor_extra_is_none(self):
        transport = transports.Transport()
        self.assertEqual(transport._extra, {})

    def test_get_extra_info(self):
        transport = transports.Transport({'extra': 'info'})
        self.assertEqual('info', transport.get_extra_info('extra'))
        self.assertIsNone(transport.get_extra_info('unknown'))

        default = object()
        self.assertIs(default, transport.get_extra_info('unknown', default))

    def test_writelines(self):
        transport = transports.Transport()
        transport.write = unittest.mock.Mock()

        transport.writelines(['line1', 'line2', 'line3'])
        self.assertEqual(3, transport.write.call_count)

    def test_not_implemented(self):
        transport = transports.Transport()

        self.assertRaises(NotImplementedError, transport.write, 'data')
        self.assertRaises(NotImplementedError, transport.write_eof)
        self.assertRaises(NotImplementedError, transport.can_write_eof)
        self.assertRaises(NotImplementedError, transport.pause_reading)
        self.assertRaises(NotImplementedError, transport.resume_reading)
        self.assertRaises(NotImplementedError, transport.close)
        self.assertRaises(NotImplementedError, transport.abort)

    def test_dgram_not_implemented(self):
        transport = transports.DatagramTransport()

        self.assertRaises(NotImplementedError, transport.sendto, 'data')
        self.assertRaises(NotImplementedError, transport.abort)

    def test_subprocess_transport_not_implemented(self):
        transport = transports.SubprocessTransport()

        self.assertRaises(NotImplementedError, transport.get_pid)
        self.assertRaises(NotImplementedError, transport.get_returncode)
        self.assertRaises(NotImplementedError, transport.get_pipe_transport, 1)
        self.assertRaises(NotImplementedError, transport.send_signal, 1)
        self.assertRaises(NotImplementedError, transport.terminate)
        self.assertRaises(NotImplementedError, transport.kill)
