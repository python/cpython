"""Tests for proactor_events.py"""

import io
import socket
import unittest
import sys
from unittest import mock

import asyncio
from asyncio.proactor_events import BaseProactorEventLoop
from asyncio.proactor_events import _ProactorSocketTransport
from asyncio.proactor_events import _ProactorWritePipeTransport
from asyncio.proactor_events import _ProactorDuplexPipeTransport
from asyncio.proactor_events import _ProactorDatagramTransport
from test.support import os_helper
from test.support import socket_helper
from test.test_asyncio import utils as test_utils


def tearDownModule():
    asyncio.set_event_loop_policy(None)


def close_transport(transport):
    # Don't call transport.close() because the event loop and the IOCP proactor
    # are mocked
    if transport._sock is None:
        return
    transport._sock.close()
    transport._sock = None


class ProactorSocketTransportTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = self.new_test_loop()
        self.addCleanup(self.loop.close)
        self.proactor = mock.Mock()
        self.loop._proactor = self.proactor
        self.protocol = test_utils.make_test_protocol(asyncio.Protocol)
        self.sock = mock.Mock(socket.socket)
        self.buffer_size = 65536

    def socket_transport(self, waiter=None):
        transport = _ProactorSocketTransport(self.loop, self.sock,
                                             self.protocol, waiter=waiter)
        self.addCleanup(close_transport, transport)
        return transport

    def test_ctor(self):
        fut = self.loop.create_future()
        tr = self.socket_transport(waiter=fut)
        test_utils.run_briefly(self.loop)
        self.assertIsNone(fut.result())
        self.protocol.connection_made(tr)
        self.proactor.recv_into.assert_called_with(self.sock, bytearray(self.buffer_size))

    def test_loop_reading(self):
        tr = self.socket_transport()
        tr._loop_reading()
        self.loop._proactor.recv_into.assert_called_with(self.sock, bytearray(self.buffer_size))
        self.assertFalse(self.protocol.data_received.called)
        self.assertFalse(self.protocol.eof_received.called)

    def test_loop_reading_data(self):
        buf = b'data'
        res = self.loop.create_future()
        res.set_result(len(buf))

        tr = self.socket_transport()
        tr._read_fut = res
        tr._data[:len(buf)] = buf
        tr._loop_reading(res)
        called_buf = bytearray(self.buffer_size)
        called_buf[:len(buf)] = buf
        self.loop._proactor.recv_into.assert_called_with(self.sock, called_buf)
        self.protocol.data_received.assert_called_with(bytearray(buf))

    @unittest.skipIf(sys.flags.optimize, "Assertions are disabled in optimized mode")
    def test_loop_reading_no_data(self):
        res = self.loop.create_future()
        res.set_result(0)

        tr = self.socket_transport()
        self.assertRaises(AssertionError, tr._loop_reading, res)

        tr.close = mock.Mock()
        tr._read_fut = res
        tr._loop_reading(res)
        self.assertFalse(self.loop._proactor.recv_into.called)
        self.assertTrue(self.protocol.eof_received.called)
        self.assertTrue(tr.close.called)

    def test_loop_reading_aborted(self):
        err = self.loop._proactor.recv_into.side_effect = ConnectionAbortedError()

        tr = self.socket_transport()
        tr._fatal_error = mock.Mock()
        tr._loop_reading()
        tr._fatal_error.assert_called_with(
                            err,
                            'Fatal read error on pipe transport')

    def test_loop_reading_aborted_closing(self):
        self.loop._proactor.recv_into.side_effect = ConnectionAbortedError()

        tr = self.socket_transport()
        tr._closing = True
        tr._fatal_error = mock.Mock()
        tr._loop_reading()
        self.assertFalse(tr._fatal_error.called)

    def test_loop_reading_aborted_is_fatal(self):
        self.loop._proactor.recv_into.side_effect = ConnectionAbortedError()
        tr = self.socket_transport()
        tr._closing = False
        tr._fatal_error = mock.Mock()
        tr._loop_reading()
        self.assertTrue(tr._fatal_error.called)

    def test_loop_reading_conn_reset_lost(self):
        err = self.loop._proactor.recv_into.side_effect = ConnectionResetError()

        tr = self.socket_transport()
        tr._closing = False
        tr._fatal_error = mock.Mock()
        tr._force_close = mock.Mock()
        tr._loop_reading()
        self.assertFalse(tr._fatal_error.called)
        tr._force_close.assert_called_with(err)

    def test_loop_reading_exception(self):
        err = self.loop._proactor.recv_into.side_effect = (OSError())

        tr = self.socket_transport()
        tr._fatal_error = mock.Mock()
        tr._loop_reading()
        tr._fatal_error.assert_called_with(
                            err,
                            'Fatal read error on pipe transport')

    def test_write(self):
        tr = self.socket_transport()
        tr._loop_writing = mock.Mock()
        tr.write(b'data')
        self.assertEqual(tr._buffer, None)
        tr._loop_writing.assert_called_with(data=b'data')

    def test_write_no_data(self):
        tr = self.socket_transport()
        tr.write(b'')
        self.assertFalse(tr._buffer)

    def test_write_more(self):
        tr = self.socket_transport()
        tr._write_fut = mock.Mock()
        tr._loop_writing = mock.Mock()
        tr.write(b'data')
        self.assertEqual(tr._buffer, b'data')
        self.assertFalse(tr._loop_writing.called)

    def test_loop_writing(self):
        tr = self.socket_transport()
        tr._buffer = bytearray(b'data')
        tr._loop_writing()
        self.loop._proactor.send.assert_called_with(self.sock, b'data')
        self.loop._proactor.send.return_value.add_done_callback.\
            assert_called_with(tr._loop_writing)

    @mock.patch('asyncio.proactor_events.logger')
    def test_loop_writing_err(self, m_log):
        err = self.loop._proactor.send.side_effect = OSError()
        tr = self.socket_transport()
        tr._fatal_error = mock.Mock()
        tr._buffer = [b'da', b'ta']
        tr._loop_writing()
        tr._fatal_error.assert_called_with(
                            err,
                            'Fatal write error on pipe transport')
        tr._conn_lost = 1

        tr.write(b'data')
        tr.write(b'data')
        tr.write(b'data')
        tr.write(b'data')
        tr.write(b'data')
        self.assertEqual(tr._buffer, None)
        m_log.warning.assert_called_with('socket.send() raised exception.')

    def test_loop_writing_stop(self):
        fut = self.loop.create_future()
        fut.set_result(b'data')

        tr = self.socket_transport()
        tr._write_fut = fut
        tr._loop_writing(fut)
        self.assertIsNone(tr._write_fut)

    def test_loop_writing_closing(self):
        fut = self.loop.create_future()
        fut.set_result(1)

        tr = self.socket_transport()
        tr._write_fut = fut
        tr.close()
        tr._loop_writing(fut)
        self.assertIsNone(tr._write_fut)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)

    def test_abort(self):
        tr = self.socket_transport()
        tr._force_close = mock.Mock()
        tr.abort()
        tr._force_close.assert_called_with(None)

    def test_close(self):
        tr = self.socket_transport()
        tr.close()
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)
        self.assertTrue(tr.is_closing())
        self.assertEqual(tr._conn_lost, 1)

        self.protocol.connection_lost.reset_mock()
        tr.close()
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.connection_lost.called)

    def test_close_write_fut(self):
        tr = self.socket_transport()
        tr._write_fut = mock.Mock()
        tr.close()
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.connection_lost.called)

    def test_close_buffer(self):
        tr = self.socket_transport()
        tr._buffer = [b'data']
        tr.close()
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.connection_lost.called)

    def test_close_invalid_sockobj(self):
        tr = self.socket_transport()
        self.sock.fileno.return_value = -1
        tr.close()
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)
        self.assertFalse(self.sock.shutdown.called)

    @mock.patch('asyncio.base_events.logger')
    def test_fatal_error(self, m_logging):
        tr = self.socket_transport()
        tr._force_close = mock.Mock()
        tr._fatal_error(None)
        self.assertTrue(tr._force_close.called)
        self.assertTrue(m_logging.error.called)

    def test_force_close(self):
        tr = self.socket_transport()
        tr._buffer = [b'data']
        read_fut = tr._read_fut = mock.Mock()
        write_fut = tr._write_fut = mock.Mock()
        tr._force_close(None)

        read_fut.cancel.assert_called_with()
        write_fut.cancel.assert_called_with()
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)
        self.assertEqual(None, tr._buffer)
        self.assertEqual(tr._conn_lost, 1)

    def test_loop_writing_force_close(self):
        exc_handler = mock.Mock()
        self.loop.set_exception_handler(exc_handler)
        fut = self.loop.create_future()
        fut.set_result(1)
        self.proactor.send.return_value = fut

        tr = self.socket_transport()
        tr.write(b'data')
        tr._force_close(None)
        test_utils.run_briefly(self.loop)
        exc_handler.assert_not_called()

    def test_force_close_idempotent(self):
        tr = self.socket_transport()
        tr._closing = True
        tr._force_close(None)
        test_utils.run_briefly(self.loop)
        # See https://github.com/python/cpython/issues/89237
        # `protocol.connection_lost` should be called even if
        # the transport was closed forcefully otherwise
        # the resources held by protocol will never be freed
        # and waiters will never be notified leading to hang.
        self.assertTrue(self.protocol.connection_lost.called)

    def test_force_close_protocol_connection_lost_once(self):
        tr = self.socket_transport()
        self.assertFalse(self.protocol.connection_lost.called)
        tr._closing = True
        # Calling _force_close twice should not call
        # protocol.connection_lost twice
        tr._force_close(None)
        tr._force_close(None)
        test_utils.run_briefly(self.loop)
        self.assertEqual(1, self.protocol.connection_lost.call_count)

    def test_close_protocol_connection_lost_once(self):
        tr = self.socket_transport()
        self.assertFalse(self.protocol.connection_lost.called)
        # Calling close twice should not call
        # protocol.connection_lost twice
        tr.close()
        tr.close()
        test_utils.run_briefly(self.loop)
        self.assertEqual(1, self.protocol.connection_lost.call_count)

    def test_fatal_error_2(self):
        tr = self.socket_transport()
        tr._buffer = [b'data']
        tr._force_close(None)

        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)
        self.assertEqual(None, tr._buffer)

    def test_call_connection_lost(self):
        tr = self.socket_transport()
        tr._call_connection_lost(None)
        self.assertTrue(self.protocol.connection_lost.called)
        self.assertTrue(self.sock.close.called)

    def test_write_eof(self):
        tr = self.socket_transport()
        self.assertTrue(tr.can_write_eof())
        tr.write_eof()
        self.sock.shutdown.assert_called_with(socket.SHUT_WR)
        tr.write_eof()
        self.assertEqual(self.sock.shutdown.call_count, 1)
        tr.close()

    def test_write_eof_buffer(self):
        tr = self.socket_transport()
        f = self.loop.create_future()
        tr._loop._proactor.send.return_value = f
        tr.write(b'data')
        tr.write_eof()
        self.assertTrue(tr._eof_written)
        self.assertFalse(self.sock.shutdown.called)
        tr._loop._proactor.send.assert_called_with(self.sock, b'data')
        f.set_result(4)
        self.loop._run_once()
        self.sock.shutdown.assert_called_with(socket.SHUT_WR)
        tr.close()

    def test_write_eof_write_pipe(self):
        tr = _ProactorWritePipeTransport(
            self.loop, self.sock, self.protocol)
        self.assertTrue(tr.can_write_eof())
        tr.write_eof()
        self.assertTrue(tr.is_closing())
        self.loop._run_once()
        self.assertTrue(self.sock.close.called)
        tr.close()

    def test_write_eof_buffer_write_pipe(self):
        tr = _ProactorWritePipeTransport(self.loop, self.sock, self.protocol)
        f = self.loop.create_future()
        tr._loop._proactor.send.return_value = f
        tr.write(b'data')
        tr.write_eof()
        self.assertTrue(tr.is_closing())
        self.assertFalse(self.sock.shutdown.called)
        tr._loop._proactor.send.assert_called_with(self.sock, b'data')
        f.set_result(4)
        self.loop._run_once()
        self.loop._run_once()
        self.assertTrue(self.sock.close.called)
        tr.close()

    def test_write_eof_duplex_pipe(self):
        tr = _ProactorDuplexPipeTransport(
            self.loop, self.sock, self.protocol)
        self.assertFalse(tr.can_write_eof())
        with self.assertRaises(NotImplementedError):
            tr.write_eof()
        close_transport(tr)

    def test_pause_resume_reading(self):
        tr = self.socket_transport()
        index = 0
        msgs = [b'data1', b'data2', b'data3', b'data4', b'data5', b'']
        reversed_msgs = list(reversed(msgs))

        def recv_into(sock, data):
            f = self.loop.create_future()
            msg = reversed_msgs.pop()

            result = f.result
            def monkey():
                data[:len(msg)] = msg
                return result()
            f.result = monkey

            f.set_result(len(msg))
            return f

        self.loop._proactor.recv_into.side_effect = recv_into
        self.loop._run_once()
        self.assertFalse(tr._paused)
        self.assertTrue(tr.is_reading())

        for msg in msgs[:2]:
            self.loop._run_once()
            self.protocol.data_received.assert_called_with(bytearray(msg))

        tr.pause_reading()
        tr.pause_reading()
        self.assertTrue(tr._paused)
        self.assertFalse(tr.is_reading())
        for i in range(10):
            self.loop._run_once()
        self.protocol.data_received.assert_called_with(bytearray(msgs[1]))

        tr.resume_reading()
        tr.resume_reading()
        self.assertFalse(tr._paused)
        self.assertTrue(tr.is_reading())

        for msg in msgs[2:4]:
            self.loop._run_once()
            self.protocol.data_received.assert_called_with(bytearray(msg))

        tr.pause_reading()
        tr.resume_reading()
        self.loop.call_exception_handler = mock.Mock()
        self.loop._run_once()
        self.loop.call_exception_handler.assert_not_called()
        self.protocol.data_received.assert_called_with(bytearray(msgs[4]))
        tr.close()

        self.assertFalse(tr.is_reading())

    def test_pause_reading_connection_made(self):
        tr = self.socket_transport()
        self.protocol.connection_made.side_effect = lambda _: tr.pause_reading()
        test_utils.run_briefly(self.loop)
        self.assertFalse(tr.is_reading())
        self.loop.assert_no_reader(7)

        tr.resume_reading()
        self.assertTrue(tr.is_reading())

        tr.close()
        self.assertFalse(tr.is_reading())


    def pause_writing_transport(self, high):
        tr = self.socket_transport()
        tr.set_write_buffer_limits(high=high)

        self.assertEqual(tr.get_write_buffer_size(), 0)
        self.assertFalse(self.protocol.pause_writing.called)
        self.assertFalse(self.protocol.resume_writing.called)
        return tr

    def test_pause_resume_writing(self):
        tr = self.pause_writing_transport(high=4)

        # write a large chunk, must pause writing
        fut = self.loop.create_future()
        self.loop._proactor.send.return_value = fut
        tr.write(b'large data')
        self.loop._run_once()
        self.assertTrue(self.protocol.pause_writing.called)

        # flush the buffer
        fut.set_result(None)
        self.loop._run_once()
        self.assertEqual(tr.get_write_buffer_size(), 0)
        self.assertTrue(self.protocol.resume_writing.called)

    def test_pause_writing_2write(self):
        tr = self.pause_writing_transport(high=4)

        # first short write, the buffer is not full (3 <= 4)
        fut1 = self.loop.create_future()
        self.loop._proactor.send.return_value = fut1
        tr.write(b'123')
        self.loop._run_once()
        self.assertEqual(tr.get_write_buffer_size(), 3)
        self.assertFalse(self.protocol.pause_writing.called)

        # fill the buffer, must pause writing (6 > 4)
        tr.write(b'abc')
        self.loop._run_once()
        self.assertEqual(tr.get_write_buffer_size(), 6)
        self.assertTrue(self.protocol.pause_writing.called)

    def test_pause_writing_3write(self):
        tr = self.pause_writing_transport(high=4)

        # first short write, the buffer is not full (1 <= 4)
        fut = self.loop.create_future()
        self.loop._proactor.send.return_value = fut
        tr.write(b'1')
        self.loop._run_once()
        self.assertEqual(tr.get_write_buffer_size(), 1)
        self.assertFalse(self.protocol.pause_writing.called)

        # second short write, the buffer is not full (3 <= 4)
        tr.write(b'23')
        self.loop._run_once()
        self.assertEqual(tr.get_write_buffer_size(), 3)
        self.assertFalse(self.protocol.pause_writing.called)

        # fill the buffer, must pause writing (6 > 4)
        tr.write(b'abc')
        self.loop._run_once()
        self.assertEqual(tr.get_write_buffer_size(), 6)
        self.assertTrue(self.protocol.pause_writing.called)

    def test_dont_pause_writing(self):
        tr = self.pause_writing_transport(high=4)

        # write a large chunk which completes immediately,
        # it should not pause writing
        fut = self.loop.create_future()
        fut.set_result(None)
        self.loop._proactor.send.return_value = fut
        tr.write(b'very large data')
        self.loop._run_once()
        self.assertEqual(tr.get_write_buffer_size(), 0)
        self.assertFalse(self.protocol.pause_writing.called)


class ProactorDatagramTransportTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = self.new_test_loop()
        self.proactor = mock.Mock()
        self.loop._proactor = self.proactor
        self.protocol = test_utils.make_test_protocol(asyncio.DatagramProtocol)
        self.sock = mock.Mock(spec_set=socket.socket)
        self.sock.fileno.return_value = 7

    def datagram_transport(self, address=None):
        self.sock.getpeername.side_effect = None if address else OSError
        transport = _ProactorDatagramTransport(self.loop, self.sock,
                                               self.protocol,
                                               address=address)
        self.addCleanup(close_transport, transport)
        return transport

    def test_sendto(self):
        data = b'data'
        transport = self.datagram_transport()
        transport.sendto(data, ('0.0.0.0', 1234))
        self.assertTrue(self.proactor.sendto.called)
        self.proactor.sendto.assert_called_with(
            self.sock, data, addr=('0.0.0.0', 1234))

    def test_sendto_bytearray(self):
        data = bytearray(b'data')
        transport = self.datagram_transport()
        transport.sendto(data, ('0.0.0.0', 1234))
        self.assertTrue(self.proactor.sendto.called)
        self.proactor.sendto.assert_called_with(
            self.sock, b'data', addr=('0.0.0.0', 1234))

    def test_sendto_memoryview(self):
        data = memoryview(b'data')
        transport = self.datagram_transport()
        transport.sendto(data, ('0.0.0.0', 1234))
        self.assertTrue(self.proactor.sendto.called)
        self.proactor.sendto.assert_called_with(
            self.sock, b'data', addr=('0.0.0.0', 1234))

    def test_sendto_no_data(self):
        transport = self.datagram_transport()
        transport._buffer.append((b'data', ('0.0.0.0', 12345)))
        transport.sendto(b'', ())
        self.assertFalse(self.sock.sendto.called)
        self.assertEqual(
            [(b'data', ('0.0.0.0', 12345))], list(transport._buffer))

    def test_sendto_buffer(self):
        transport = self.datagram_transport()
        transport._buffer.append((b'data1', ('0.0.0.0', 12345)))
        transport._write_fut = object()
        transport.sendto(b'data2', ('0.0.0.0', 12345))
        self.assertFalse(self.proactor.sendto.called)
        self.assertEqual(
            [(b'data1', ('0.0.0.0', 12345)),
             (b'data2', ('0.0.0.0', 12345))],
            list(transport._buffer))

    def test_sendto_buffer_bytearray(self):
        data2 = bytearray(b'data2')
        transport = self.datagram_transport()
        transport._buffer.append((b'data1', ('0.0.0.0', 12345)))
        transport._write_fut = object()
        transport.sendto(data2, ('0.0.0.0', 12345))
        self.assertFalse(self.proactor.sendto.called)
        self.assertEqual(
            [(b'data1', ('0.0.0.0', 12345)),
             (b'data2', ('0.0.0.0', 12345))],
            list(transport._buffer))
        self.assertIsInstance(transport._buffer[1][0], bytes)

    def test_sendto_buffer_memoryview(self):
        data2 = memoryview(b'data2')
        transport = self.datagram_transport()
        transport._buffer.append((b'data1', ('0.0.0.0', 12345)))
        transport._write_fut = object()
        transport.sendto(data2, ('0.0.0.0', 12345))
        self.assertFalse(self.proactor.sendto.called)
        self.assertEqual(
            [(b'data1', ('0.0.0.0', 12345)),
             (b'data2', ('0.0.0.0', 12345))],
            list(transport._buffer))
        self.assertIsInstance(transport._buffer[1][0], bytes)

    @mock.patch('asyncio.proactor_events.logger')
    def test_sendto_exception(self, m_log):
        data = b'data'
        err = self.proactor.sendto.side_effect = RuntimeError()

        transport = self.datagram_transport()
        transport._fatal_error = mock.Mock()
        transport.sendto(data, ())

        self.assertTrue(transport._fatal_error.called)
        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal write error on datagram transport')
        transport._conn_lost = 1

        transport._address = ('123',)
        transport.sendto(data)
        transport.sendto(data)
        transport.sendto(data)
        transport.sendto(data)
        transport.sendto(data)
        m_log.warning.assert_called_with('socket.sendto() raised exception.')

    def test_sendto_error_received(self):
        data = b'data'

        self.sock.sendto.side_effect = ConnectionRefusedError

        transport = self.datagram_transport()
        transport._fatal_error = mock.Mock()
        transport.sendto(data, ())

        self.assertEqual(transport._conn_lost, 0)
        self.assertFalse(transport._fatal_error.called)

    def test_sendto_error_received_connected(self):
        data = b'data'

        self.proactor.send.side_effect = ConnectionRefusedError

        transport = self.datagram_transport(address=('0.0.0.0', 1))
        transport._fatal_error = mock.Mock()
        transport.sendto(data)

        self.assertFalse(transport._fatal_error.called)
        self.assertTrue(self.protocol.error_received.called)

    def test_sendto_str(self):
        transport = self.datagram_transport()
        self.assertRaises(TypeError, transport.sendto, 'str', ())

    def test_sendto_connected_addr(self):
        transport = self.datagram_transport(address=('0.0.0.0', 1))
        self.assertRaises(
            ValueError, transport.sendto, b'str', ('0.0.0.0', 2))

    def test_sendto_closing(self):
        transport = self.datagram_transport(address=(1,))
        transport.close()
        self.assertEqual(transport._conn_lost, 1)
        transport.sendto(b'data', (1,))
        self.assertEqual(transport._conn_lost, 2)

    def test__loop_writing_closing(self):
        transport = self.datagram_transport()
        transport._closing = True
        transport._loop_writing()
        self.assertIsNone(transport._write_fut)
        test_utils.run_briefly(self.loop)
        self.sock.close.assert_called_with()
        self.protocol.connection_lost.assert_called_with(None)

    def test__loop_writing_exception(self):
        err = self.proactor.sendto.side_effect = RuntimeError()

        transport = self.datagram_transport()
        transport._fatal_error = mock.Mock()
        transport._buffer.append((b'data', ()))
        transport._loop_writing()

        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal write error on datagram transport')

    def test__loop_writing_error_received(self):
        self.proactor.sendto.side_effect = ConnectionRefusedError

        transport = self.datagram_transport()
        transport._fatal_error = mock.Mock()
        transport._buffer.append((b'data', ()))
        transport._loop_writing()

        self.assertFalse(transport._fatal_error.called)

    def test__loop_writing_error_received_connection(self):
        self.proactor.send.side_effect = ConnectionRefusedError

        transport = self.datagram_transport(address=('0.0.0.0', 1))
        transport._fatal_error = mock.Mock()
        transport._buffer.append((b'data', ()))
        transport._loop_writing()

        self.assertFalse(transport._fatal_error.called)
        self.assertTrue(self.protocol.error_received.called)

    @mock.patch('asyncio.base_events.logger.error')
    def test_fatal_error_connected(self, m_exc):
        transport = self.datagram_transport(address=('0.0.0.0', 1))
        err = ConnectionRefusedError()
        transport._fatal_error(err)
        self.assertFalse(self.protocol.error_received.called)
        m_exc.assert_not_called()


class BaseProactorEventLoopTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()

        self.sock = test_utils.mock_nonblocking_socket()
        self.proactor = mock.Mock()

        self.ssock, self.csock = mock.Mock(), mock.Mock()

        with mock.patch('asyncio.proactor_events.socket.socketpair',
                        return_value=(self.ssock, self.csock)):
            with mock.patch('signal.set_wakeup_fd'):
                self.loop = BaseProactorEventLoop(self.proactor)
        self.set_event_loop(self.loop)

    @mock.patch('asyncio.proactor_events.socket.socketpair')
    def test_ctor(self, socketpair):
        ssock, csock = socketpair.return_value = (
            mock.Mock(), mock.Mock())
        with mock.patch('signal.set_wakeup_fd'):
            loop = BaseProactorEventLoop(self.proactor)
        self.assertIs(loop._ssock, ssock)
        self.assertIs(loop._csock, csock)
        self.assertEqual(loop._internal_fds, 1)
        loop.close()

    def test_close_self_pipe(self):
        self.loop._close_self_pipe()
        self.assertEqual(self.loop._internal_fds, 0)
        self.assertTrue(self.ssock.close.called)
        self.assertTrue(self.csock.close.called)
        self.assertIsNone(self.loop._ssock)
        self.assertIsNone(self.loop._csock)

        # Don't call close(): _close_self_pipe() cannot be called twice
        self.loop._closed = True

    def test_close(self):
        self.loop._close_self_pipe = mock.Mock()
        self.loop.close()
        self.assertTrue(self.loop._close_self_pipe.called)
        self.assertTrue(self.proactor.close.called)
        self.assertIsNone(self.loop._proactor)

        self.loop._close_self_pipe.reset_mock()
        self.loop.close()
        self.assertFalse(self.loop._close_self_pipe.called)

    def test_make_socket_transport(self):
        tr = self.loop._make_socket_transport(self.sock, asyncio.Protocol())
        self.assertIsInstance(tr, _ProactorSocketTransport)
        close_transport(tr)

    def test_loop_self_reading(self):
        self.loop._loop_self_reading()
        self.proactor.recv.assert_called_with(self.ssock, 4096)
        self.proactor.recv.return_value.add_done_callback.assert_called_with(
            self.loop._loop_self_reading)

    def test_loop_self_reading_fut(self):
        fut = mock.Mock()
        self.loop._self_reading_future = fut
        self.loop._loop_self_reading(fut)
        self.assertTrue(fut.result.called)
        self.proactor.recv.assert_called_with(self.ssock, 4096)
        self.proactor.recv.return_value.add_done_callback.assert_called_with(
            self.loop._loop_self_reading)

    def test_loop_self_reading_exception(self):
        self.loop.call_exception_handler = mock.Mock()
        self.proactor.recv.side_effect = OSError()
        self.loop._loop_self_reading()
        self.assertTrue(self.loop.call_exception_handler.called)

    def test_write_to_self(self):
        self.loop._write_to_self()
        self.csock.send.assert_called_with(b'\0')

    def test_process_events(self):
        self.loop._process_events([])

    @mock.patch('asyncio.base_events.logger')
    def test_create_server(self, m_log):
        pf = mock.Mock()
        call_soon = self.loop.call_soon = mock.Mock()

        self.loop._start_serving(pf, self.sock)
        self.assertTrue(call_soon.called)

        # callback
        loop = call_soon.call_args[0][0]
        loop()
        self.proactor.accept.assert_called_with(self.sock)

        # conn
        fut = mock.Mock()
        fut.result.return_value = (mock.Mock(), mock.Mock())

        make_tr = self.loop._make_socket_transport = mock.Mock()
        loop(fut)
        self.assertTrue(fut.result.called)
        self.assertTrue(make_tr.called)

        # exception
        fut.result.side_effect = OSError()
        loop(fut)
        self.assertTrue(self.sock.close.called)
        self.assertTrue(m_log.error.called)

    def test_create_server_cancel(self):
        pf = mock.Mock()
        call_soon = self.loop.call_soon = mock.Mock()

        self.loop._start_serving(pf, self.sock)
        loop = call_soon.call_args[0][0]

        # cancelled
        fut = self.loop.create_future()
        fut.cancel()
        loop(fut)
        self.assertTrue(self.sock.close.called)

    def test_stop_serving(self):
        sock1 = mock.Mock()
        future1 = mock.Mock()
        sock2 = mock.Mock()
        future2 = mock.Mock()
        self.loop._accept_futures = {
            sock1.fileno(): future1,
            sock2.fileno(): future2
        }

        self.loop._stop_serving(sock1)
        self.assertTrue(sock1.close.called)
        self.assertTrue(future1.cancel.called)
        self.proactor._stop_serving.assert_called_with(sock1)
        self.assertFalse(sock2.close.called)
        self.assertFalse(future2.cancel.called)

    def datagram_transport(self):
        self.protocol = test_utils.make_test_protocol(asyncio.DatagramProtocol)
        return self.loop._make_datagram_transport(self.sock, self.protocol)

    def test_make_datagram_transport(self):
        tr = self.datagram_transport()
        self.assertIsInstance(tr, _ProactorDatagramTransport)
        self.assertIsInstance(tr, asyncio.DatagramTransport)
        close_transport(tr)

    def test_datagram_loop_writing(self):
        tr = self.datagram_transport()
        tr._buffer.appendleft((b'data', ('127.0.0.1', 12068)))
        tr._loop_writing()
        self.loop._proactor.sendto.assert_called_with(self.sock, b'data', addr=('127.0.0.1', 12068))
        self.loop._proactor.sendto.return_value.add_done_callback.\
            assert_called_with(tr._loop_writing)

        close_transport(tr)

    def test_datagram_loop_reading(self):
        tr = self.datagram_transport()
        tr._loop_reading()
        self.loop._proactor.recvfrom.assert_called_with(self.sock, 256 * 1024)
        self.assertFalse(self.protocol.datagram_received.called)
        self.assertFalse(self.protocol.error_received.called)
        close_transport(tr)

    def test_datagram_loop_reading_data(self):
        res = self.loop.create_future()
        res.set_result((b'data', ('127.0.0.1', 12068)))

        tr = self.datagram_transport()
        tr._read_fut = res
        tr._loop_reading(res)
        self.loop._proactor.recvfrom.assert_called_with(self.sock, 256 * 1024)
        self.protocol.datagram_received.assert_called_with(b'data', ('127.0.0.1', 12068))
        close_transport(tr)

    @unittest.skipIf(sys.flags.optimize, "Assertions are disabled in optimized mode")
    def test_datagram_loop_reading_no_data(self):
        res = self.loop.create_future()
        res.set_result((b'', ('127.0.0.1', 12068)))

        tr = self.datagram_transport()
        self.assertRaises(AssertionError, tr._loop_reading, res)

        tr.close = mock.Mock()
        tr._read_fut = res
        tr._loop_reading(res)
        self.assertTrue(self.loop._proactor.recvfrom.called)
        self.assertFalse(self.protocol.error_received.called)
        self.assertFalse(tr.close.called)
        close_transport(tr)

    def test_datagram_loop_reading_aborted(self):
        err = self.loop._proactor.recvfrom.side_effect = ConnectionAbortedError()

        tr = self.datagram_transport()
        tr._fatal_error = mock.Mock()
        tr._protocol.error_received = mock.Mock()
        tr._loop_reading()
        tr._protocol.error_received.assert_called_with(err)
        close_transport(tr)

    def test_datagram_loop_writing_aborted(self):
        err = self.loop._proactor.sendto.side_effect = ConnectionAbortedError()

        tr = self.datagram_transport()
        tr._fatal_error = mock.Mock()
        tr._protocol.error_received = mock.Mock()
        tr._buffer.appendleft((b'Hello', ('127.0.0.1', 12068)))
        tr._loop_writing()
        tr._protocol.error_received.assert_called_with(err)
        close_transport(tr)


@unittest.skipIf(sys.platform != 'win32',
                 'Proactor is supported on Windows only')
class ProactorEventLoopUnixSockSendfileTests(test_utils.TestCase):
    DATA = b"12345abcde" * 16 * 1024  # 160 KiB

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
        self.loop = asyncio.ProactorEventLoop()
        self.set_event_loop(self.loop)
        self.addCleanup(self.loop.close)
        self.file = open(os_helper.TESTFN, 'rb')
        self.addCleanup(self.file.close)
        super().setUp()

    def make_socket(self, cleanup=True):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setblocking(False)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1024)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)
        if cleanup:
            self.addCleanup(sock.close)
        return sock

    def run_loop(self, coro):
        return self.loop.run_until_complete(coro)

    def prepare(self):
        sock = self.make_socket()
        proto = self.MyProto(self.loop)
        port = socket_helper.find_unused_port()
        srv_sock = self.make_socket(cleanup=False)
        srv_sock.bind(('127.0.0.1', port))
        server = self.run_loop(self.loop.create_server(
            lambda: proto, sock=srv_sock))
        self.run_loop(self.loop.sock_connect(sock, srv_sock.getsockname()))

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

    def test_sock_sendfile_not_a_file(self):
        sock, proto = self.prepare()
        f = object()
        with self.assertRaisesRegex(asyncio.SendfileNotAvailableError,
                                    "not a regular file"):
            self.run_loop(self.loop._sock_sendfile_native(sock, f,
                                                          0, None))
        self.assertEqual(self.file.tell(), 0)

    def test_sock_sendfile_iobuffer(self):
        sock, proto = self.prepare()
        f = io.BytesIO()
        with self.assertRaisesRegex(asyncio.SendfileNotAvailableError,
                                    "not a regular file"):
            self.run_loop(self.loop._sock_sendfile_native(sock, f,
                                                          0, None))
        self.assertEqual(self.file.tell(), 0)

    def test_sock_sendfile_not_regular_file(self):
        sock, proto = self.prepare()
        f = mock.Mock()
        f.fileno.return_value = -1
        with self.assertRaisesRegex(asyncio.SendfileNotAvailableError,
                                    "not a regular file"):
            self.run_loop(self.loop._sock_sendfile_native(sock, f,
                                                          0, None))
        self.assertEqual(self.file.tell(), 0)


if __name__ == '__main__':
    unittest.main()
