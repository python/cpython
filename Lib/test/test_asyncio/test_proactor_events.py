"""Tests for proactor_events.py"""

import socket
import unittest
import unittest.mock

import asyncio
from asyncio.proactor_events import BaseProactorEventLoop
from asyncio.proactor_events import _ProactorSocketTransport
from asyncio.proactor_events import _ProactorWritePipeTransport
from asyncio.proactor_events import _ProactorDuplexPipeTransport
from asyncio import test_utils


class ProactorSocketTransportTests(unittest.TestCase):

    def setUp(self):
        self.loop = test_utils.TestLoop()
        self.proactor = unittest.mock.Mock()
        self.loop._proactor = self.proactor
        self.protocol = test_utils.make_test_protocol(asyncio.Protocol)
        self.sock = unittest.mock.Mock(socket.socket)

    def test_ctor(self):
        fut = asyncio.Future(loop=self.loop)
        tr = _ProactorSocketTransport(
            self.loop, self.sock, self.protocol, fut)
        test_utils.run_briefly(self.loop)
        self.assertIsNone(fut.result())
        self.protocol.connection_made(tr)
        self.proactor.recv.assert_called_with(self.sock, 4096)

    def test_loop_reading(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._loop_reading()
        self.loop._proactor.recv.assert_called_with(self.sock, 4096)
        self.assertFalse(self.protocol.data_received.called)
        self.assertFalse(self.protocol.eof_received.called)

    def test_loop_reading_data(self):
        res = asyncio.Future(loop=self.loop)
        res.set_result(b'data')

        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)

        tr._read_fut = res
        tr._loop_reading(res)
        self.loop._proactor.recv.assert_called_with(self.sock, 4096)
        self.protocol.data_received.assert_called_with(b'data')

    def test_loop_reading_no_data(self):
        res = asyncio.Future(loop=self.loop)
        res.set_result(b'')

        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)

        self.assertRaises(AssertionError, tr._loop_reading, res)

        tr.close = unittest.mock.Mock()
        tr._read_fut = res
        tr._loop_reading(res)
        self.assertFalse(self.loop._proactor.recv.called)
        self.assertTrue(self.protocol.eof_received.called)
        self.assertTrue(tr.close.called)

    def test_loop_reading_aborted(self):
        err = self.loop._proactor.recv.side_effect = ConnectionAbortedError()

        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._fatal_error = unittest.mock.Mock()
        tr._loop_reading()
        tr._fatal_error.assert_called_with(
                            err,
                            'Fatal read error on pipe transport')

    def test_loop_reading_aborted_closing(self):
        self.loop._proactor.recv.side_effect = ConnectionAbortedError()

        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._closing = True
        tr._fatal_error = unittest.mock.Mock()
        tr._loop_reading()
        self.assertFalse(tr._fatal_error.called)

    def test_loop_reading_aborted_is_fatal(self):
        self.loop._proactor.recv.side_effect = ConnectionAbortedError()
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._closing = False
        tr._fatal_error = unittest.mock.Mock()
        tr._loop_reading()
        self.assertTrue(tr._fatal_error.called)

    def test_loop_reading_conn_reset_lost(self):
        err = self.loop._proactor.recv.side_effect = ConnectionResetError()

        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._closing = False
        tr._fatal_error = unittest.mock.Mock()
        tr._force_close = unittest.mock.Mock()
        tr._loop_reading()
        self.assertFalse(tr._fatal_error.called)
        tr._force_close.assert_called_with(err)

    def test_loop_reading_exception(self):
        err = self.loop._proactor.recv.side_effect = (OSError())

        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._fatal_error = unittest.mock.Mock()
        tr._loop_reading()
        tr._fatal_error.assert_called_with(
                            err,
                            'Fatal read error on pipe transport')

    def test_write(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._loop_writing = unittest.mock.Mock()
        tr.write(b'data')
        self.assertEqual(tr._buffer, None)
        tr._loop_writing.assert_called_with(data=b'data')

    def test_write_no_data(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr.write(b'')
        self.assertFalse(tr._buffer)

    def test_write_more(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._write_fut = unittest.mock.Mock()
        tr._loop_writing = unittest.mock.Mock()
        tr.write(b'data')
        self.assertEqual(tr._buffer, b'data')
        self.assertFalse(tr._loop_writing.called)

    def test_loop_writing(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._buffer = bytearray(b'data')
        tr._loop_writing()
        self.loop._proactor.send.assert_called_with(self.sock, b'data')
        self.loop._proactor.send.return_value.add_done_callback.\
            assert_called_with(tr._loop_writing)

    @unittest.mock.patch('asyncio.proactor_events.logger')
    def test_loop_writing_err(self, m_log):
        err = self.loop._proactor.send.side_effect = OSError()
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._fatal_error = unittest.mock.Mock()
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

        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._write_fut = fut
        tr._loop_writing(fut)
        self.assertIsNone(tr._write_fut)

    def test_loop_writing_closing(self):
        fut = asyncio.Future(loop=self.loop)
        fut.set_result(1)

        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._write_fut = fut
        tr.close()
        tr._loop_writing(fut)
        self.assertIsNone(tr._write_fut)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)

    def test_abort(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._force_close = unittest.mock.Mock()
        tr.abort()
        tr._force_close.assert_called_with(None)

    def test_close(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr.close()
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)
        self.assertTrue(tr._closing)
        self.assertEqual(tr._conn_lost, 1)

        self.protocol.connection_lost.reset_mock()
        tr.close()
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.connection_lost.called)

    def test_close_write_fut(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._write_fut = unittest.mock.Mock()
        tr.close()
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.connection_lost.called)

    def test_close_buffer(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._buffer = [b'data']
        tr.close()
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.connection_lost.called)

    @unittest.mock.patch('asyncio.base_events.logger')
    def test_fatal_error(self, m_logging):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._force_close = unittest.mock.Mock()
        tr._fatal_error(None)
        self.assertTrue(tr._force_close.called)
        self.assertTrue(m_logging.error.called)

    def test_force_close(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._buffer = [b'data']
        read_fut = tr._read_fut = unittest.mock.Mock()
        write_fut = tr._write_fut = unittest.mock.Mock()
        tr._force_close(None)

        read_fut.cancel.assert_called_with()
        write_fut.cancel.assert_called_with()
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)
        self.assertEqual(None, tr._buffer)
        self.assertEqual(tr._conn_lost, 1)

    def test_force_close_idempotent(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._closing = True
        tr._force_close(None)
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.connection_lost.called)

    def test_fatal_error_2(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._buffer = [b'data']
        tr._force_close(None)

        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)
        self.assertEqual(None, tr._buffer)

    def test_call_connection_lost(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
        tr._call_connection_lost(None)
        self.assertTrue(self.protocol.connection_lost.called)
        self.assertTrue(self.sock.close.called)

    def test_write_eof(self):
        tr = _ProactorSocketTransport(
            self.loop, self.sock, self.protocol)
        self.assertTrue(tr.can_write_eof())
        tr.write_eof()
        self.sock.shutdown.assert_called_with(socket.SHUT_WR)
        tr.write_eof()
        self.assertEqual(self.sock.shutdown.call_count, 1)
        tr.close()

    def test_write_eof_buffer(self):
        tr = _ProactorSocketTransport(self.loop, self.sock, self.protocol)
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
        self.assertTrue(tr._closing)
        self.loop._run_once()
        self.assertTrue(self.sock.close.called)
        tr.close()

    def test_write_eof_buffer_write_pipe(self):
        tr = _ProactorWritePipeTransport(self.loop, self.sock, self.protocol)
        f = asyncio.Future(loop=self.loop)
        tr._loop._proactor.send.return_value = f
        tr.write(b'data')
        tr.write_eof()
        self.assertTrue(tr._closing)
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
        tr.close()

    def test_pause_resume_reading(self):
        tr = _ProactorSocketTransport(
            self.loop, self.sock, self.protocol)
        futures = []
        for msg in [b'data1', b'data2', b'data3', b'data4', b'']:
            f = asyncio.Future(loop=self.loop)
            f.set_result(msg)
            futures.append(f)
        self.loop._proactor.recv.side_effect = futures
        self.loop._run_once()
        self.assertFalse(tr._paused)
        self.loop._run_once()
        self.protocol.data_received.assert_called_with(b'data1')
        self.loop._run_once()
        self.protocol.data_received.assert_called_with(b'data2')
        tr.pause_reading()
        self.assertTrue(tr._paused)
        for i in range(10):
            self.loop._run_once()
        self.protocol.data_received.assert_called_with(b'data2')
        tr.resume_reading()
        self.assertFalse(tr._paused)
        self.loop._run_once()
        self.protocol.data_received.assert_called_with(b'data3')
        self.loop._run_once()
        self.protocol.data_received.assert_called_with(b'data4')
        tr.close()


class BaseProactorEventLoopTests(unittest.TestCase):

    def setUp(self):
        self.sock = unittest.mock.Mock(socket.socket)
        self.proactor = unittest.mock.Mock()

        self.ssock, self.csock = unittest.mock.Mock(), unittest.mock.Mock()

        class EventLoop(BaseProactorEventLoop):
            def _socketpair(s):
                return (self.ssock, self.csock)

        self.loop = EventLoop(self.proactor)

    @unittest.mock.patch.object(BaseProactorEventLoop, 'call_soon')
    @unittest.mock.patch.object(BaseProactorEventLoop, '_socketpair')
    def test_ctor(self, socketpair, call_soon):
        ssock, csock = socketpair.return_value = (
            unittest.mock.Mock(), unittest.mock.Mock())
        loop = BaseProactorEventLoop(self.proactor)
        self.assertIs(loop._ssock, ssock)
        self.assertIs(loop._csock, csock)
        self.assertEqual(loop._internal_fds, 1)
        call_soon.assert_called_with(loop._loop_self_reading)

    def test_close_self_pipe(self):
        self.loop._close_self_pipe()
        self.assertEqual(self.loop._internal_fds, 0)
        self.assertTrue(self.ssock.close.called)
        self.assertTrue(self.csock.close.called)
        self.assertIsNone(self.loop._ssock)
        self.assertIsNone(self.loop._csock)

    def test_close(self):
        self.loop._close_self_pipe = unittest.mock.Mock()
        self.loop.close()
        self.assertTrue(self.loop._close_self_pipe.called)
        self.assertTrue(self.proactor.close.called)
        self.assertIsNone(self.loop._proactor)

        self.loop._close_self_pipe.reset_mock()
        self.loop.close()
        self.assertFalse(self.loop._close_self_pipe.called)

    def test_sock_recv(self):
        self.loop.sock_recv(self.sock, 1024)
        self.proactor.recv.assert_called_with(self.sock, 1024)

    def test_sock_sendall(self):
        self.loop.sock_sendall(self.sock, b'data')
        self.proactor.send.assert_called_with(self.sock, b'data')

    def test_sock_connect(self):
        self.loop.sock_connect(self.sock, 123)
        self.proactor.connect.assert_called_with(self.sock, 123)

    def test_sock_accept(self):
        self.loop.sock_accept(self.sock)
        self.proactor.accept.assert_called_with(self.sock)

    def test_socketpair(self):
        self.assertRaises(
            NotImplementedError, BaseProactorEventLoop, self.proactor)

    def test_make_socket_transport(self):
        tr = self.loop._make_socket_transport(self.sock, asyncio.Protocol())
        self.assertIsInstance(tr, _ProactorSocketTransport)

    def test_loop_self_reading(self):
        self.loop._loop_self_reading()
        self.proactor.recv.assert_called_with(self.ssock, 4096)
        self.proactor.recv.return_value.add_done_callback.assert_called_with(
            self.loop._loop_self_reading)

    def test_loop_self_reading_fut(self):
        fut = unittest.mock.Mock()
        self.loop._loop_self_reading(fut)
        self.assertTrue(fut.result.called)
        self.proactor.recv.assert_called_with(self.ssock, 4096)
        self.proactor.recv.return_value.add_done_callback.assert_called_with(
            self.loop._loop_self_reading)

    def test_loop_self_reading_exception(self):
        self.loop.close = unittest.mock.Mock()
        self.proactor.recv.side_effect = OSError()
        self.assertRaises(OSError, self.loop._loop_self_reading)
        self.assertTrue(self.loop.close.called)

    def test_write_to_self(self):
        self.loop._write_to_self()
        self.csock.send.assert_called_with(b'x')

    def test_process_events(self):
        self.loop._process_events([])

    @unittest.mock.patch('asyncio.base_events.logger')
    def test_create_server(self, m_log):
        pf = unittest.mock.Mock()
        call_soon = self.loop.call_soon = unittest.mock.Mock()

        self.loop._start_serving(pf, self.sock)
        self.assertTrue(call_soon.called)

        # callback
        loop = call_soon.call_args[0][0]
        loop()
        self.proactor.accept.assert_called_with(self.sock)

        # conn
        fut = unittest.mock.Mock()
        fut.result.return_value = (unittest.mock.Mock(), unittest.mock.Mock())

        make_tr = self.loop._make_socket_transport = unittest.mock.Mock()
        loop(fut)
        self.assertTrue(fut.result.called)
        self.assertTrue(make_tr.called)

        # exception
        fut.result.side_effect = OSError()
        loop(fut)
        self.assertTrue(self.sock.close.called)
        self.assertTrue(m_log.error.called)

    def test_create_server_cancel(self):
        pf = unittest.mock.Mock()
        call_soon = self.loop.call_soon = unittest.mock.Mock()

        self.loop._start_serving(pf, self.sock)
        loop = call_soon.call_args[0][0]

        # cancelled
        fut = asyncio.Future(loop=self.loop)
        fut.cancel()
        loop(fut)
        self.assertTrue(self.sock.close.called)

    def test_stop_serving(self):
        sock = unittest.mock.Mock()
        self.loop._stop_serving(sock)
        self.assertTrue(sock.close.called)
        self.proactor._stop_serving.assert_called_with(sock)


if __name__ == '__main__':
    unittest.main()
