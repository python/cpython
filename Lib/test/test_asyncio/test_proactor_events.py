"""Tests for proactor_events.py"""

import socket
import unittest
from unittest import mock

import asyncio
from asyncio.proactor_events import BaseProactorEventLoop
from asyncio.proactor_events import _ProactorSocketTransport
from asyncio.proactor_events import _ProactorWritePipeTransport
from asyncio.proactor_events import _ProactorDuplexPipeTransport
from test.test_asyncio import utils as test_utils


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

    def socket_transport(self, waiter=None):
        transport = _ProactorSocketTransport(self.loop, self.sock,
                                             self.protocol, waiter=waiter)
        self.addCleanup(close_transport, transport)
        return transport

    def test_ctor(self):
        fut = asyncio.Future(loop=self.loop)
        tr = self.socket_transport(waiter=fut)
        test_utils.run_briefly(self.loop)
        self.assertIsNone(fut.result())
        self.protocol.connection_made(tr)
        self.proactor.recv.assert_called_with(self.sock, 4096)

    def test_loop_reading(self):
        tr = self.socket_transport()
        tr._loop_reading()
        self.loop._proactor.recv.assert_called_with(self.sock, 4096)
        self.assertFalse(self.protocol.data_received.called)
        self.assertFalse(self.protocol.eof_received.called)

    def test_loop_reading_data(self):
        res = asyncio.Future(loop=self.loop)
        res.set_result(b'data')

        tr = self.socket_transport()
        tr._read_fut = res
        tr._loop_reading(res)
        self.loop._proactor.recv.assert_called_with(self.sock, 4096)
        self.protocol.data_received.assert_called_with(b'data')

    def test_loop_reading_no_data(self):
        res = asyncio.Future(loop=self.loop)
        res.set_result(b'')

        tr = self.socket_transport()
        self.assertRaises(AssertionError, tr._loop_reading, res)

        tr.close = mock.Mock()
        tr._read_fut = res
        tr._loop_reading(res)
        self.assertFalse(self.loop._proactor.recv.called)
        self.assertTrue(self.protocol.eof_received.called)
        self.assertTrue(tr.close.called)

    def test_loop_reading_aborted(self):
        err = self.loop._proactor.recv.side_effect = ConnectionAbortedError()

        tr = self.socket_transport()
        tr._fatal_error = mock.Mock()
        tr._loop_reading()
        tr._fatal_error.assert_called_with(
                            err,
                            'Fatal read error on pipe transport')

    def test_loop_reading_aborted_closing(self):
        self.loop._proactor.recv.side_effect = ConnectionAbortedError()

        tr = self.socket_transport()
        tr._closing = True
        tr._fatal_error = mock.Mock()
        tr._loop_reading()
        self.assertFalse(tr._fatal_error.called)

    def test_loop_reading_aborted_is_fatal(self):
        self.loop._proactor.recv.side_effect = ConnectionAbortedError()
        tr = self.socket_transport()
        tr._closing = False
        tr._fatal_error = mock.Mock()
        tr._loop_reading()
        self.assertTrue(tr._fatal_error.called)

    def test_loop_reading_conn_reset_lost(self):
        err = self.loop._proactor.recv.side_effect = ConnectionResetError()

        tr = self.socket_transport()
        tr._closing = False
        tr._fatal_error = mock.Mock()
        tr._force_close = mock.Mock()
        tr._loop_reading()
        self.assertFalse(tr._fatal_error.called)
        tr._force_close.assert_called_with(err)

    def test_loop_reading_exception(self):
        err = self.loop._proactor.recv.side_effect = (OSError())

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
        fut = asyncio.Future(loop=self.loop)
        fut.set_result(b'data')

        tr = self.socket_transport()
        tr._write_fut = fut
        tr._loop_writing(fut)
        self.assertIsNone(tr._write_fut)

    def test_loop_writing_closing(self):
        fut = asyncio.Future(loop=self.loop)
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

    def test_force_close_idempotent(self):
        tr = self.socket_transport()
        tr._closing = True
        tr._force_close(None)
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.connection_lost.called)

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
        f = asyncio.Future(loop=self.loop)
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
        f = asyncio.Future(loop=self.loop)
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
        futures = []
        for msg in [b'data1', b'data2', b'data3', b'data4', b'']:
            f = asyncio.Future(loop=self.loop)
            f.set_result(msg)
            futures.append(f)

        self.loop._proactor.recv.side_effect = futures
        self.loop._run_once()
        self.assertFalse(tr._paused)
        self.assertTrue(tr.is_reading())
        self.loop._run_once()
        self.protocol.data_received.assert_called_with(b'data1')
        self.loop._run_once()
        self.protocol.data_received.assert_called_with(b'data2')

        tr.pause_reading()
        tr.pause_reading()
        self.assertTrue(tr._paused)
        self.assertFalse(tr.is_reading())
        for i in range(10):
            self.loop._run_once()
        self.protocol.data_received.assert_called_with(b'data2')

        tr.resume_reading()
        tr.resume_reading()
        self.assertFalse(tr._paused)
        self.assertTrue(tr.is_reading())
        self.loop._run_once()
        self.protocol.data_received.assert_called_with(b'data3')
        self.loop._run_once()
        self.protocol.data_received.assert_called_with(b'data4')
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
        fut = asyncio.Future(loop=self.loop)
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
        fut1 = asyncio.Future(loop=self.loop)
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
        fut = asyncio.Future(loop=self.loop)
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
        fut = asyncio.Future(loop=self.loop)
        fut.set_result(None)
        self.loop._proactor.send.return_value = fut
        tr.write(b'very large data')
        self.loop._run_once()
        self.assertEqual(tr.get_write_buffer_size(), 0)
        self.assertFalse(self.protocol.pause_writing.called)


class BaseProactorEventLoopTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()

        self.sock = test_utils.mock_nonblocking_socket()
        self.proactor = mock.Mock()

        self.ssock, self.csock = mock.Mock(), mock.Mock()

        with mock.patch('asyncio.proactor_events.socket.socketpair',
                        return_value=(self.ssock, self.csock)):
            self.loop = BaseProactorEventLoop(self.proactor)
        self.set_event_loop(self.loop)

    @mock.patch.object(BaseProactorEventLoop, 'call_soon')
    @mock.patch('asyncio.proactor_events.socket.socketpair')
    def test_ctor(self, socketpair, call_soon):
        ssock, csock = socketpair.return_value = (
            mock.Mock(), mock.Mock())
        loop = BaseProactorEventLoop(self.proactor)
        self.assertIs(loop._ssock, ssock)
        self.assertIs(loop._csock, csock)
        self.assertEqual(loop._internal_fds, 1)
        call_soon.assert_called_with(loop._loop_self_reading)
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
        fut = asyncio.Future(loop=self.loop)
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


if __name__ == '__main__':
    unittest.main()
