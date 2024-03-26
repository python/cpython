"""Tests for selector_events.py"""

import collections
import selectors
import socket
import sys
import unittest
from asyncio import selector_events
from unittest import mock

try:
    import ssl
except ImportError:
    ssl = None

import asyncio
from asyncio.selector_events import (BaseSelectorEventLoop,
                                     _SelectorDatagramTransport,
                                     _SelectorSocketTransport,
                                     _SelectorTransport)
from test.test_asyncio import utils as test_utils

MOCK_ANY = mock.ANY


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class TestBaseSelectorEventLoop(BaseSelectorEventLoop):

    def _make_self_pipe(self):
        self._ssock = mock.Mock()
        self._csock = mock.Mock()
        self._internal_fds += 1

    def _close_self_pipe(self):
        pass


def list_to_buffer(l=()):
    buffer = collections.deque()
    buffer.extend((memoryview(i) for i in l))
    return buffer



def close_transport(transport):
    # Don't call transport.close() because the event loop and the selector
    # are mocked
    if transport._sock is None:
        return
    transport._sock.close()
    transport._sock = None


class BaseSelectorEventLoopTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.selector = mock.Mock()
        self.selector.select.return_value = []
        self.loop = TestBaseSelectorEventLoop(self.selector)
        self.set_event_loop(self.loop)

    def test_make_socket_transport(self):
        m = mock.Mock()
        self.loop.add_reader = mock.Mock()
        self.loop._ensure_fd_no_transport = mock.Mock()
        transport = self.loop._make_socket_transport(m, asyncio.Protocol())
        self.assertIsInstance(transport, _SelectorSocketTransport)
        self.assertEqual(self.loop._ensure_fd_no_transport.call_count, 1)

        # Calling repr() must not fail when the event loop is closed
        self.loop.close()
        repr(transport)

        close_transport(transport)

    @mock.patch('asyncio.selector_events.ssl', None)
    @mock.patch('asyncio.sslproto.ssl', None)
    def test_make_ssl_transport_without_ssl_error(self):
        m = mock.Mock()
        self.loop.add_reader = mock.Mock()
        self.loop.add_writer = mock.Mock()
        self.loop.remove_reader = mock.Mock()
        self.loop.remove_writer = mock.Mock()
        self.loop._ensure_fd_no_transport = mock.Mock()
        with self.assertRaises(RuntimeError):
            self.loop._make_ssl_transport(m, m, m, m)
        self.assertEqual(self.loop._ensure_fd_no_transport.call_count, 1)

    def test_close(self):
        class EventLoop(BaseSelectorEventLoop):
            def _make_self_pipe(self):
                self._ssock = mock.Mock()
                self._csock = mock.Mock()
                self._internal_fds += 1

        self.loop = EventLoop(self.selector)
        self.set_event_loop(self.loop)

        ssock = self.loop._ssock
        ssock.fileno.return_value = 7
        csock = self.loop._csock
        csock.fileno.return_value = 1
        remove_reader = self.loop._remove_reader = mock.Mock()

        self.loop._selector.close()
        self.loop._selector = selector = mock.Mock()
        self.assertFalse(self.loop.is_closed())

        self.loop.close()
        self.assertTrue(self.loop.is_closed())
        self.assertIsNone(self.loop._selector)
        self.assertIsNone(self.loop._csock)
        self.assertIsNone(self.loop._ssock)
        selector.close.assert_called_with()
        ssock.close.assert_called_with()
        csock.close.assert_called_with()
        remove_reader.assert_called_with(7)

        # it should be possible to call close() more than once
        self.loop.close()
        self.loop.close()

        # operation blocked when the loop is closed
        f = self.loop.create_future()
        self.assertRaises(RuntimeError, self.loop.run_forever)
        self.assertRaises(RuntimeError, self.loop.run_until_complete, f)
        fd = 0
        def callback():
            pass
        self.assertRaises(RuntimeError, self.loop.add_reader, fd, callback)
        self.assertRaises(RuntimeError, self.loop.add_writer, fd, callback)

    def test_close_no_selector(self):
        self.loop.remove_reader = mock.Mock()
        self.loop._selector.close()
        self.loop._selector = None
        self.loop.close()
        self.assertIsNone(self.loop._selector)

    def test_read_from_self_tryagain(self):
        self.loop._ssock.recv.side_effect = BlockingIOError
        self.assertIsNone(self.loop._read_from_self())

    def test_read_from_self_exception(self):
        self.loop._ssock.recv.side_effect = OSError
        self.assertRaises(OSError, self.loop._read_from_self)

    def test_write_to_self_tryagain(self):
        self.loop._csock.send.side_effect = BlockingIOError
        with test_utils.disable_logger():
            self.assertIsNone(self.loop._write_to_self())

    def test_write_to_self_exception(self):
        # _write_to_self() swallows OSError
        self.loop._csock.send.side_effect = RuntimeError()
        self.assertRaises(RuntimeError, self.loop._write_to_self)

    @mock.patch('socket.getaddrinfo')
    def test_sock_connect_resolve_using_socket_params(self, m_gai):
        addr = ('need-resolution.com', 8080)
        for sock_type in [socket.SOCK_STREAM, socket.SOCK_DGRAM]:
            with self.subTest(sock_type):
                sock = test_utils.mock_nonblocking_socket(type=sock_type)

                m_gai.side_effect = \
                    lambda *args: [(None, None, None, None, ('127.0.0.1', 0))]

                con = self.loop.create_task(self.loop.sock_connect(sock, addr))
                self.loop.run_until_complete(con)
                m_gai.assert_called_with(
                    addr[0], addr[1], sock.family, sock.type, sock.proto, 0)

                self.loop.run_until_complete(con)
                sock.connect.assert_called_with(('127.0.0.1', 0))

    def test_add_reader(self):
        self.loop._selector.get_map.return_value = {}
        cb = lambda: True
        self.loop.add_reader(1, cb)

        self.assertTrue(self.loop._selector.register.called)
        fd, mask, (r, w) = self.loop._selector.register.call_args[0]
        self.assertEqual(1, fd)
        self.assertEqual(selectors.EVENT_READ, mask)
        self.assertEqual(cb, r._callback)
        self.assertIsNone(w)

    def test_add_reader_existing(self):
        reader = mock.Mock()
        writer = mock.Mock()
        self.loop._selector.get_map.return_value = {1: selectors.SelectorKey(
            1, 1, selectors.EVENT_WRITE, (reader, writer))}
        cb = lambda: True
        self.loop.add_reader(1, cb)

        self.assertTrue(reader.cancel.called)
        self.assertFalse(self.loop._selector.register.called)
        self.assertTrue(self.loop._selector.modify.called)
        fd, mask, (r, w) = self.loop._selector.modify.call_args[0]
        self.assertEqual(1, fd)
        self.assertEqual(selectors.EVENT_WRITE | selectors.EVENT_READ, mask)
        self.assertEqual(cb, r._callback)
        self.assertEqual(writer, w)

    def test_add_reader_existing_writer(self):
        writer = mock.Mock()
        self.loop._selector.get_map.return_value = {1: selectors.SelectorKey(
            1, 1, selectors.EVENT_WRITE, (None, writer))}
        cb = lambda: True
        self.loop.add_reader(1, cb)

        self.assertFalse(self.loop._selector.register.called)
        self.assertTrue(self.loop._selector.modify.called)
        fd, mask, (r, w) = self.loop._selector.modify.call_args[0]
        self.assertEqual(1, fd)
        self.assertEqual(selectors.EVENT_WRITE | selectors.EVENT_READ, mask)
        self.assertEqual(cb, r._callback)
        self.assertEqual(writer, w)

    def test_remove_reader(self):
        self.loop._selector.get_map.return_value = {1: selectors.SelectorKey(
            1, 1, selectors.EVENT_READ, (None, None))}
        self.assertFalse(self.loop.remove_reader(1))

        self.assertTrue(self.loop._selector.unregister.called)

    def test_remove_reader_read_write(self):
        reader = mock.Mock()
        writer = mock.Mock()
        self.loop._selector.get_map.return_value = {1: selectors.SelectorKey(
            1, 1, selectors.EVENT_READ | selectors.EVENT_WRITE,
            (reader, writer))}
        self.assertTrue(
            self.loop.remove_reader(1))

        self.assertFalse(self.loop._selector.unregister.called)
        self.assertEqual(
            (1, selectors.EVENT_WRITE, (None, writer)),
            self.loop._selector.modify.call_args[0])

    def test_remove_reader_unknown(self):
        self.loop._selector.get_map.return_value = {}
        self.assertFalse(
            self.loop.remove_reader(1))

    def test_add_writer(self):
        self.loop._selector.get_map.return_value = {}
        cb = lambda: True
        self.loop.add_writer(1, cb)

        self.assertTrue(self.loop._selector.register.called)
        fd, mask, (r, w) = self.loop._selector.register.call_args[0]
        self.assertEqual(1, fd)
        self.assertEqual(selectors.EVENT_WRITE, mask)
        self.assertIsNone(r)
        self.assertEqual(cb, w._callback)

    def test_add_writer_existing(self):
        reader = mock.Mock()
        writer = mock.Mock()
        self.loop._selector.get_map.return_value = {1: selectors.SelectorKey(
            1, 1, selectors.EVENT_READ, (reader, writer))}
        cb = lambda: True
        self.loop.add_writer(1, cb)

        self.assertTrue(writer.cancel.called)
        self.assertFalse(self.loop._selector.register.called)
        self.assertTrue(self.loop._selector.modify.called)
        fd, mask, (r, w) = self.loop._selector.modify.call_args[0]
        self.assertEqual(1, fd)
        self.assertEqual(selectors.EVENT_WRITE | selectors.EVENT_READ, mask)
        self.assertEqual(reader, r)
        self.assertEqual(cb, w._callback)

    def test_remove_writer(self):
        self.loop._selector.get_map.return_value = {1: selectors.SelectorKey(
            1, 1, selectors.EVENT_WRITE, (None, None))}
        self.assertFalse(self.loop.remove_writer(1))

        self.assertTrue(self.loop._selector.unregister.called)

    def test_remove_writer_read_write(self):
        reader = mock.Mock()
        writer = mock.Mock()
        self.loop._selector.get_map.return_value = {1: selectors.SelectorKey(
            1, 1, selectors.EVENT_READ | selectors.EVENT_WRITE,
            (reader, writer))}
        self.assertTrue(
            self.loop.remove_writer(1))

        self.assertFalse(self.loop._selector.unregister.called)
        self.assertEqual(
            (1, selectors.EVENT_READ, (reader, None)),
            self.loop._selector.modify.call_args[0])

    def test_remove_writer_unknown(self):
        self.loop._selector.get_map.return_value = {}
        self.assertFalse(
            self.loop.remove_writer(1))

    def test_process_events_read(self):
        reader = mock.Mock()
        reader._cancelled = False

        self.loop._add_callback = mock.Mock()
        self.loop._process_events(
            [(selectors.SelectorKey(
                1, 1, selectors.EVENT_READ, (reader, None)),
              selectors.EVENT_READ)])
        self.assertTrue(self.loop._add_callback.called)
        self.loop._add_callback.assert_called_with(reader)

    def test_process_events_read_cancelled(self):
        reader = mock.Mock()
        reader.cancelled = True

        self.loop._remove_reader = mock.Mock()
        self.loop._process_events(
            [(selectors.SelectorKey(
                1, 1, selectors.EVENT_READ, (reader, None)),
             selectors.EVENT_READ)])
        self.loop._remove_reader.assert_called_with(1)

    def test_process_events_write(self):
        writer = mock.Mock()
        writer._cancelled = False

        self.loop._add_callback = mock.Mock()
        self.loop._process_events(
            [(selectors.SelectorKey(1, 1, selectors.EVENT_WRITE,
                                    (None, writer)),
              selectors.EVENT_WRITE)])
        self.loop._add_callback.assert_called_with(writer)

    def test_process_events_write_cancelled(self):
        writer = mock.Mock()
        writer.cancelled = True
        self.loop._remove_writer = mock.Mock()

        self.loop._process_events(
            [(selectors.SelectorKey(1, 1, selectors.EVENT_WRITE,
                                    (None, writer)),
              selectors.EVENT_WRITE)])
        self.loop._remove_writer.assert_called_with(1)

    def test_accept_connection_multiple(self):
        sock = mock.Mock()
        sock.accept.return_value = (mock.Mock(), mock.Mock())
        backlog = 100
        # Mock the coroutine generation for a connection to prevent
        # warnings related to un-awaited coroutines. _accept_connection2
        # is an async function that is patched with AsyncMock. create_task
        # creates a task out of coroutine returned by AsyncMock, so use
        # asyncio.sleep(0) to ensure created tasks are complete to avoid
        # task pending warnings.
        mock_obj = mock.patch.object
        with mock_obj(self.loop, '_accept_connection2') as accept2_mock:
            self.loop._accept_connection(
                mock.Mock(), sock, backlog=backlog)
        self.loop.run_until_complete(asyncio.sleep(0))
        self.assertEqual(sock.accept.call_count, backlog)


class SelectorTransportTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = self.new_test_loop()
        self.protocol = test_utils.make_test_protocol(asyncio.Protocol)
        self.sock = mock.Mock(socket.socket)
        self.sock.fileno.return_value = 7

    def create_transport(self):
        transport = _SelectorTransport(self.loop, self.sock, self.protocol,
                                       None)
        self.addCleanup(close_transport, transport)
        return transport

    def test_ctor(self):
        tr = self.create_transport()
        self.assertIs(tr._loop, self.loop)
        self.assertIs(tr._sock, self.sock)
        self.assertIs(tr._sock_fd, 7)

    def test_abort(self):
        tr = self.create_transport()
        tr._force_close = mock.Mock()

        tr.abort()
        tr._force_close.assert_called_with(None)

    def test_close(self):
        tr = self.create_transport()
        tr.close()

        self.assertTrue(tr.is_closing())
        self.assertEqual(1, self.loop.remove_reader_count[7])
        self.protocol.connection_lost(None)
        self.assertEqual(tr._conn_lost, 1)

        tr.close()
        self.assertEqual(tr._conn_lost, 1)
        self.assertEqual(1, self.loop.remove_reader_count[7])

    def test_close_write_buffer(self):
        tr = self.create_transport()
        tr._buffer.extend(b'data')
        tr.close()

        self.assertFalse(self.loop.readers)
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.connection_lost.called)

    def test_force_close(self):
        tr = self.create_transport()
        tr._buffer.extend(b'1')
        self.loop._add_reader(7, mock.sentinel)
        self.loop._add_writer(7, mock.sentinel)
        tr._force_close(None)

        self.assertTrue(tr.is_closing())
        self.assertEqual(tr._buffer, list_to_buffer())
        self.assertFalse(self.loop.readers)
        self.assertFalse(self.loop.writers)

        # second close should not remove reader
        tr._force_close(None)
        self.assertFalse(self.loop.readers)
        self.assertEqual(1, self.loop.remove_reader_count[7])

    @mock.patch('asyncio.log.logger.error')
    def test_fatal_error(self, m_exc):
        exc = OSError()
        tr = self.create_transport()
        tr._force_close = mock.Mock()
        tr._fatal_error(exc)

        m_exc.assert_not_called()

        tr._force_close.assert_called_with(exc)

    @mock.patch('asyncio.log.logger.error')
    def test_fatal_error_custom_exception(self, m_exc):
        class MyError(Exception):
            pass
        exc = MyError()
        tr = self.create_transport()
        tr._force_close = mock.Mock()
        tr._fatal_error(exc)

        m_exc.assert_called_with(
            test_utils.MockPattern(
                'Fatal error on transport\nprotocol:.*\ntransport:.*'),
            exc_info=(MyError, MOCK_ANY, MOCK_ANY))

        tr._force_close.assert_called_with(exc)

    def test_connection_lost(self):
        exc = OSError()
        tr = self.create_transport()
        self.assertIsNotNone(tr._protocol)
        self.assertIsNotNone(tr._loop)
        tr._call_connection_lost(exc)

        self.protocol.connection_lost.assert_called_with(exc)
        self.sock.close.assert_called_with()
        self.assertIsNone(tr._sock)

        self.assertIsNone(tr._protocol)
        self.assertIsNone(tr._loop)

    def test__add_reader(self):
        tr = self.create_transport()
        tr._buffer.extend(b'1')
        tr._add_reader(7, mock.sentinel)
        self.assertTrue(self.loop.readers)

        tr._force_close(None)

        self.assertTrue(tr.is_closing())
        self.assertFalse(self.loop.readers)

        # can not add readers after closing
        tr._add_reader(7, mock.sentinel)
        self.assertFalse(self.loop.readers)


class SelectorSocketTransportTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = self.new_test_loop()
        self.protocol = test_utils.make_test_protocol(asyncio.Protocol)
        self.sock = mock.Mock(socket.socket)
        self.sock_fd = self.sock.fileno.return_value = 7

    def socket_transport(self, waiter=None, sendmsg=False):
        transport = _SelectorSocketTransport(self.loop, self.sock,
                                             self.protocol, waiter=waiter)
        if sendmsg:
            transport._write_ready = transport._write_sendmsg
        else:
            transport._write_ready = transport._write_send
        self.addCleanup(close_transport, transport)
        return transport

    def test_ctor(self):
        waiter = self.loop.create_future()
        tr = self.socket_transport(waiter=waiter)
        self.loop.run_until_complete(waiter)

        self.loop.assert_reader(7, tr._read_ready)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_made.assert_called_with(tr)

    def test_ctor_with_waiter(self):
        waiter = self.loop.create_future()
        self.socket_transport(waiter=waiter)
        self.loop.run_until_complete(waiter)

        self.assertIsNone(waiter.result())

    def test_pause_resume_reading(self):
        tr = self.socket_transport()
        test_utils.run_briefly(self.loop)
        self.assertFalse(tr._paused)
        self.assertTrue(tr.is_reading())
        self.loop.assert_reader(7, tr._read_ready)

        tr.pause_reading()
        tr.pause_reading()
        self.assertTrue(tr._paused)
        self.assertFalse(tr.is_reading())
        self.loop.assert_no_reader(7)

        tr.resume_reading()
        tr.resume_reading()
        self.assertFalse(tr._paused)
        self.assertTrue(tr.is_reading())
        self.loop.assert_reader(7, tr._read_ready)

        tr.close()
        self.assertFalse(tr.is_reading())
        self.loop.assert_no_reader(7)

    def test_pause_reading_connection_made(self):
        tr = self.socket_transport()
        self.protocol.connection_made.side_effect = lambda _: tr.pause_reading()
        test_utils.run_briefly(self.loop)
        self.assertFalse(tr.is_reading())
        self.loop.assert_no_reader(7)

        tr.resume_reading()
        self.assertTrue(tr.is_reading())
        self.loop.assert_reader(7, tr._read_ready)

        tr.close()
        self.assertFalse(tr.is_reading())
        self.loop.assert_no_reader(7)


    def test_read_eof_received_error(self):
        transport = self.socket_transport()
        transport.close = mock.Mock()
        transport._fatal_error = mock.Mock()

        self.loop.call_exception_handler = mock.Mock()

        self.protocol.eof_received.side_effect = LookupError()

        self.sock.recv.return_value = b''
        transport._read_ready()

        self.protocol.eof_received.assert_called_with()
        self.assertTrue(transport._fatal_error.called)

    def test_data_received_error(self):
        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()

        self.loop.call_exception_handler = mock.Mock()
        self.protocol.data_received.side_effect = LookupError()

        self.sock.recv.return_value = b'data'
        transport._read_ready()

        self.assertTrue(transport._fatal_error.called)
        self.assertTrue(self.protocol.data_received.called)

    def test_read_ready(self):
        transport = self.socket_transport()

        self.sock.recv.return_value = b'data'
        transport._read_ready()

        self.protocol.data_received.assert_called_with(b'data')

    def test_read_ready_eof(self):
        transport = self.socket_transport()
        transport.close = mock.Mock()

        self.sock.recv.return_value = b''
        transport._read_ready()

        self.protocol.eof_received.assert_called_with()
        transport.close.assert_called_with()

    def test_read_ready_eof_keep_open(self):
        transport = self.socket_transport()
        transport.close = mock.Mock()

        self.sock.recv.return_value = b''
        self.protocol.eof_received.return_value = True
        transport._read_ready()

        self.protocol.eof_received.assert_called_with()
        self.assertFalse(transport.close.called)

    @mock.patch('logging.exception')
    def test_read_ready_tryagain(self, m_exc):
        self.sock.recv.side_effect = BlockingIOError

        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()
        transport._read_ready()

        self.assertFalse(transport._fatal_error.called)

    @mock.patch('logging.exception')
    def test_read_ready_tryagain_interrupted(self, m_exc):
        self.sock.recv.side_effect = InterruptedError

        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()
        transport._read_ready()

        self.assertFalse(transport._fatal_error.called)

    @mock.patch('logging.exception')
    def test_read_ready_conn_reset(self, m_exc):
        err = self.sock.recv.side_effect = ConnectionResetError()

        transport = self.socket_transport()
        transport._force_close = mock.Mock()
        with test_utils.disable_logger():
            transport._read_ready()
        transport._force_close.assert_called_with(err)

    @mock.patch('logging.exception')
    def test_read_ready_err(self, m_exc):
        err = self.sock.recv.side_effect = OSError()

        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()
        transport._read_ready()

        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal read error on socket transport')

    def test_write(self):
        data = b'data'
        self.sock.send.return_value = len(data)

        transport = self.socket_transport()
        transport.write(data)
        self.sock.send.assert_called_with(data)

    def test_write_bytearray(self):
        data = bytearray(b'data')
        self.sock.send.return_value = len(data)

        transport = self.socket_transport()
        transport.write(data)
        self.sock.send.assert_called_with(data)
        self.assertEqual(data, bytearray(b'data'))  # Hasn't been mutated.

    def test_write_memoryview(self):
        data = memoryview(b'data')
        self.sock.send.return_value = len(data)

        transport = self.socket_transport()
        transport.write(data)
        self.sock.send.assert_called_with(data)

    def test_write_no_data(self):
        transport = self.socket_transport()
        transport._buffer.append(memoryview(b'data'))
        transport.write(b'')
        self.assertFalse(self.sock.send.called)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_buffer(self):
        transport = self.socket_transport()
        transport._buffer.append(b'data1')
        transport.write(b'data2')
        self.assertFalse(self.sock.send.called)
        self.assertEqual(list_to_buffer([b'data1', b'data2']),
                         transport._buffer)

    def test_write_partial(self):
        data = b'data'
        self.sock.send.return_value = 2

        transport = self.socket_transport()
        transport.write(data)

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'ta']), transport._buffer)

    def test_write_partial_bytearray(self):
        data = bytearray(b'data')
        self.sock.send.return_value = 2

        transport = self.socket_transport()
        transport.write(data)

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'ta']), transport._buffer)
        self.assertEqual(data, bytearray(b'data'))  # Hasn't been mutated.

    def test_write_partial_memoryview(self):
        data = memoryview(b'data')
        self.sock.send.return_value = 2

        transport = self.socket_transport()
        transport.write(data)

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'ta']), transport._buffer)

    def test_write_partial_none(self):
        data = b'data'
        self.sock.send.return_value = 0
        self.sock.fileno.return_value = 7

        transport = self.socket_transport()
        transport.write(data)

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_tryagain(self):
        self.sock.send.side_effect = BlockingIOError

        data = b'data'
        transport = self.socket_transport()
        transport.write(data)

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_sendmsg_no_data(self):
        self.sock.sendmsg = mock.Mock()
        self.sock.sendmsg.return_value = 0
        transport = self.socket_transport(sendmsg=True)
        transport._buffer.append(memoryview(b'data'))
        transport.write(b'')
        self.assertFalse(self.sock.sendmsg.called)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    @unittest.skipUnless(selector_events._HAS_SENDMSG, 'no sendmsg')
    def test_writelines_sendmsg_full(self):
        data = memoryview(b'data')
        self.sock.sendmsg = mock.Mock()
        self.sock.sendmsg.return_value = len(data)

        transport = self.socket_transport(sendmsg=True)
        transport.writelines([data])
        self.assertTrue(self.sock.sendmsg.called)
        self.assertFalse(self.loop.writers)

    @unittest.skipUnless(selector_events._HAS_SENDMSG, 'no sendmsg')
    def test_writelines_sendmsg_partial(self):
        data = memoryview(b'data')
        self.sock.sendmsg = mock.Mock()
        self.sock.sendmsg.return_value = 2

        transport = self.socket_transport(sendmsg=True)
        transport.writelines([data])
        self.assertTrue(self.sock.sendmsg.called)
        self.assertTrue(self.loop.writers)

    def test_writelines_send_full(self):
        data = memoryview(b'data')
        self.sock.send.return_value = len(data)
        self.sock.send.fileno.return_value = 7

        transport = self.socket_transport()
        transport.writelines([data])
        self.assertTrue(self.sock.send.called)
        self.assertFalse(self.loop.writers)

    def test_writelines_send_partial(self):
        data = memoryview(b'data')
        self.sock.send.return_value = 2
        self.sock.send.fileno.return_value = 7

        transport = self.socket_transport()
        transport.writelines([data])
        self.assertTrue(self.sock.send.called)
        self.assertTrue(self.loop.writers)

    @unittest.skipUnless(selector_events._HAS_SENDMSG, 'no sendmsg')
    def test_write_sendmsg_full(self):
        data = memoryview(b'data')
        self.sock.sendmsg = mock.Mock()
        self.sock.sendmsg.return_value = len(data)

        transport = self.socket_transport(sendmsg=True)
        transport._buffer.append(data)
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()
        self.assertTrue(self.sock.sendmsg.called)
        self.assertFalse(self.loop.writers)

    @unittest.skipUnless(selector_events._HAS_SENDMSG, 'no sendmsg')
    def test_write_sendmsg_partial(self):

        data = memoryview(b'data')
        self.sock.sendmsg = mock.Mock()
        # Sent partial data
        self.sock.sendmsg.return_value = 2

        transport = self.socket_transport(sendmsg=True)
        transport._buffer.append(data)
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()
        self.assertTrue(self.sock.sendmsg.called)
        self.assertTrue(self.loop.writers)
        self.assertEqual(list_to_buffer([b'ta']), transport._buffer)

    @unittest.skipUnless(selector_events._HAS_SENDMSG, 'no sendmsg')
    def test_write_sendmsg_half_buffer(self):
        data = [memoryview(b'data1'), memoryview(b'data2')]
        self.sock.sendmsg = mock.Mock()
        # Sent partial data
        self.sock.sendmsg.return_value = 2

        transport = self.socket_transport(sendmsg=True)
        transport._buffer.extend(data)
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()
        self.assertTrue(self.sock.sendmsg.called)
        self.assertTrue(self.loop.writers)
        self.assertEqual(list_to_buffer([b'ta1', b'data2']), transport._buffer)

    @unittest.skipUnless(selector_events._HAS_SENDMSG, 'no sendmsg')
    def test_write_sendmsg_OSError(self):
        data = memoryview(b'data')
        self.sock.sendmsg = mock.Mock()
        err = self.sock.sendmsg.side_effect = OSError()

        transport = self.socket_transport(sendmsg=True)
        transport._fatal_error = mock.Mock()
        transport._buffer.extend(data)
        # Calls _fatal_error and clears the buffer
        transport._write_ready()
        self.assertTrue(self.sock.sendmsg.called)
        self.assertFalse(self.loop.writers)
        self.assertEqual(list_to_buffer([]), transport._buffer)
        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal write error on socket transport')

    @mock.patch('asyncio.selector_events.logger')
    def test_write_exception(self, m_log):
        err = self.sock.send.side_effect = OSError()

        data = b'data'
        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()
        transport.write(data)
        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal write error on socket transport')
        transport._conn_lost = 1

        self.sock.reset_mock()
        transport.write(data)
        self.assertFalse(self.sock.send.called)
        self.assertEqual(transport._conn_lost, 2)
        transport.write(data)
        transport.write(data)
        transport.write(data)
        transport.write(data)
        m_log.warning.assert_called_with('socket.send() raised exception.')

    def test_write_str(self):
        transport = self.socket_transport()
        self.assertRaises(TypeError, transport.write, 'str')

    def test_write_closing(self):
        transport = self.socket_transport()
        transport.close()
        self.assertEqual(transport._conn_lost, 1)
        transport.write(b'data')
        self.assertEqual(transport._conn_lost, 2)

    def test_write_ready(self):
        data = b'data'
        self.sock.send.return_value = len(data)

        transport = self.socket_transport()
        transport._buffer.append(data)
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()
        self.assertTrue(self.sock.send.called)
        self.assertFalse(self.loop.writers)

    def test_write_ready_closing(self):
        data = memoryview(b'data')
        self.sock.send.return_value = len(data)

        transport = self.socket_transport()
        transport._closing = True
        transport._buffer.append(data)
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()
        self.assertTrue(self.sock.send.called)
        self.assertFalse(self.loop.writers)
        self.sock.close.assert_called_with()
        self.protocol.connection_lost.assert_called_with(None)

    @unittest.skipIf(sys.flags.optimize, "Assertions are disabled in optimized mode")
    def test_write_ready_no_data(self):
        transport = self.socket_transport()
        # This is an internal error.
        self.assertRaises(AssertionError, transport._write_ready)

    def test_write_ready_partial(self):
        data = memoryview(b'data')
        self.sock.send.return_value = 2

        transport = self.socket_transport()
        transport._buffer.append(data)
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()
        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'ta']), transport._buffer)

    def test_write_ready_partial_none(self):
        data = b'data'
        self.sock.send.return_value = 0

        transport = self.socket_transport()
        transport._buffer.append(data)
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()
        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_ready_tryagain(self):
        self.sock.send.side_effect = BlockingIOError

        transport = self.socket_transport()
        buffer = list_to_buffer([b'data1', b'data2'])
        transport._buffer = buffer
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(buffer, transport._buffer)

    def test_write_ready_exception(self):
        err = self.sock.send.side_effect = OSError()

        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()
        transport._buffer.extend(b'data')
        transport._write_ready()
        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal write error on socket transport')

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
        self.sock.send.side_effect = BlockingIOError
        tr.write(b'data')
        tr.write_eof()
        self.assertEqual(tr._buffer, list_to_buffer([b'data']))
        self.assertTrue(tr._eof)
        self.assertFalse(self.sock.shutdown.called)
        self.sock.send.side_effect = lambda _: 4
        tr._write_ready()
        self.assertTrue(self.sock.send.called)
        self.sock.shutdown.assert_called_with(socket.SHUT_WR)
        tr.close()

    def test_write_eof_after_close(self):
        tr = self.socket_transport()
        tr.close()
        self.loop.run_until_complete(asyncio.sleep(0))
        tr.write_eof()

    @mock.patch('asyncio.base_events.logger')
    def test_transport_close_remove_writer(self, m_log):
        remove_writer = self.loop._remove_writer = mock.Mock()

        transport = self.socket_transport()
        transport.close()
        remove_writer.assert_called_with(self.sock_fd)


class SelectorSocketTransportBufferedProtocolTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = self.new_test_loop()

        self.protocol = test_utils.make_test_protocol(asyncio.BufferedProtocol)
        self.buf = bytearray(1)
        self.protocol.get_buffer.side_effect = lambda hint: self.buf

        self.sock = mock.Mock(socket.socket)
        self.sock_fd = self.sock.fileno.return_value = 7

    def socket_transport(self, waiter=None):
        transport = _SelectorSocketTransport(self.loop, self.sock,
                                             self.protocol, waiter=waiter)
        self.addCleanup(close_transport, transport)
        return transport

    def test_ctor(self):
        waiter = self.loop.create_future()
        tr = self.socket_transport(waiter=waiter)
        self.loop.run_until_complete(waiter)

        self.loop.assert_reader(7, tr._read_ready)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_made.assert_called_with(tr)

    def test_get_buffer_error(self):
        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()

        self.loop.call_exception_handler = mock.Mock()
        self.protocol.get_buffer.side_effect = LookupError()

        transport._read_ready()

        self.assertTrue(transport._fatal_error.called)
        self.assertTrue(self.protocol.get_buffer.called)
        self.assertFalse(self.protocol.buffer_updated.called)

    def test_get_buffer_zerosized(self):
        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()

        self.loop.call_exception_handler = mock.Mock()
        self.protocol.get_buffer.side_effect = lambda hint: bytearray(0)

        transport._read_ready()

        self.assertTrue(transport._fatal_error.called)
        self.assertTrue(self.protocol.get_buffer.called)
        self.assertFalse(self.protocol.buffer_updated.called)

    def test_proto_type_switch(self):
        self.protocol = test_utils.make_test_protocol(asyncio.Protocol)
        transport = self.socket_transport()

        self.sock.recv.return_value = b'data'
        transport._read_ready()

        self.protocol.data_received.assert_called_with(b'data')

        # switch protocol to a BufferedProtocol

        buf_proto = test_utils.make_test_protocol(asyncio.BufferedProtocol)
        buf = bytearray(4)
        buf_proto.get_buffer.side_effect = lambda hint: buf

        transport.set_protocol(buf_proto)

        self.sock.recv_into.return_value = 10
        transport._read_ready()

        buf_proto.get_buffer.assert_called_with(-1)
        buf_proto.buffer_updated.assert_called_with(10)

    def test_buffer_updated_error(self):
        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()

        self.loop.call_exception_handler = mock.Mock()
        self.protocol.buffer_updated.side_effect = LookupError()

        self.sock.recv_into.return_value = 10
        transport._read_ready()

        self.assertTrue(transport._fatal_error.called)
        self.assertTrue(self.protocol.get_buffer.called)
        self.assertTrue(self.protocol.buffer_updated.called)

    def test_read_eof_received_error(self):
        transport = self.socket_transport()
        transport.close = mock.Mock()
        transport._fatal_error = mock.Mock()

        self.loop.call_exception_handler = mock.Mock()

        self.protocol.eof_received.side_effect = LookupError()

        self.sock.recv_into.return_value = 0
        transport._read_ready()

        self.protocol.eof_received.assert_called_with()
        self.assertTrue(transport._fatal_error.called)

    def test_read_ready(self):
        transport = self.socket_transport()

        self.sock.recv_into.return_value = 10
        transport._read_ready()

        self.protocol.get_buffer.assert_called_with(-1)
        self.protocol.buffer_updated.assert_called_with(10)

    def test_read_ready_eof(self):
        transport = self.socket_transport()
        transport.close = mock.Mock()

        self.sock.recv_into.return_value = 0
        transport._read_ready()

        self.protocol.eof_received.assert_called_with()
        transport.close.assert_called_with()

    def test_read_ready_eof_keep_open(self):
        transport = self.socket_transport()
        transport.close = mock.Mock()

        self.sock.recv_into.return_value = 0
        self.protocol.eof_received.return_value = True
        transport._read_ready()

        self.protocol.eof_received.assert_called_with()
        self.assertFalse(transport.close.called)

    @mock.patch('logging.exception')
    def test_read_ready_tryagain(self, m_exc):
        self.sock.recv_into.side_effect = BlockingIOError

        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()
        transport._read_ready()

        self.assertFalse(transport._fatal_error.called)

    @mock.patch('logging.exception')
    def test_read_ready_tryagain_interrupted(self, m_exc):
        self.sock.recv_into.side_effect = InterruptedError

        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()
        transport._read_ready()

        self.assertFalse(transport._fatal_error.called)

    @mock.patch('logging.exception')
    def test_read_ready_conn_reset(self, m_exc):
        err = self.sock.recv_into.side_effect = ConnectionResetError()

        transport = self.socket_transport()
        transport._force_close = mock.Mock()
        with test_utils.disable_logger():
            transport._read_ready()
        transport._force_close.assert_called_with(err)

    @mock.patch('logging.exception')
    def test_read_ready_err(self, m_exc):
        err = self.sock.recv_into.side_effect = OSError()

        transport = self.socket_transport()
        transport._fatal_error = mock.Mock()
        transport._read_ready()

        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal read error on socket transport')


class SelectorDatagramTransportTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = self.new_test_loop()
        self.protocol = test_utils.make_test_protocol(asyncio.DatagramProtocol)
        self.sock = mock.Mock(spec_set=socket.socket)
        self.sock.fileno.return_value = 7

    def datagram_transport(self, address=None):
        self.sock.getpeername.side_effect = None if address else OSError
        transport = _SelectorDatagramTransport(self.loop, self.sock,
                                               self.protocol,
                                               address=address)
        self.addCleanup(close_transport, transport)
        return transport

    def test_read_ready(self):
        transport = self.datagram_transport()

        self.sock.recvfrom.return_value = (b'data', ('0.0.0.0', 1234))
        transport._read_ready()

        self.protocol.datagram_received.assert_called_with(
            b'data', ('0.0.0.0', 1234))

    def test_transport_inheritance(self):
        transport = self.datagram_transport()
        self.assertIsInstance(transport, asyncio.DatagramTransport)

    def test_read_ready_tryagain(self):
        transport = self.datagram_transport()

        self.sock.recvfrom.side_effect = BlockingIOError
        transport._fatal_error = mock.Mock()
        transport._read_ready()

        self.assertFalse(transport._fatal_error.called)

    def test_read_ready_err(self):
        transport = self.datagram_transport()

        err = self.sock.recvfrom.side_effect = RuntimeError()
        transport._fatal_error = mock.Mock()
        transport._read_ready()

        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal read error on datagram transport')

    def test_read_ready_oserr(self):
        transport = self.datagram_transport()

        err = self.sock.recvfrom.side_effect = OSError()
        transport._fatal_error = mock.Mock()
        transport._read_ready()

        self.assertFalse(transport._fatal_error.called)
        self.protocol.error_received.assert_called_with(err)

    def test_sendto(self):
        data = b'data'
        transport = self.datagram_transport()
        transport.sendto(data, ('0.0.0.0', 1234))
        self.assertTrue(self.sock.sendto.called)
        self.assertEqual(
            self.sock.sendto.call_args[0], (data, ('0.0.0.0', 1234)))

    def test_sendto_bytearray(self):
        data = bytearray(b'data')
        transport = self.datagram_transport()
        transport.sendto(data, ('0.0.0.0', 1234))
        self.assertTrue(self.sock.sendto.called)
        self.assertEqual(
            self.sock.sendto.call_args[0], (data, ('0.0.0.0', 1234)))

    def test_sendto_memoryview(self):
        data = memoryview(b'data')
        transport = self.datagram_transport()
        transport.sendto(data, ('0.0.0.0', 1234))
        self.assertTrue(self.sock.sendto.called)
        self.assertEqual(
            self.sock.sendto.call_args[0], (data, ('0.0.0.0', 1234)))

    def test_sendto_no_data(self):
        transport = self.datagram_transport()
        transport.sendto(b'', ('0.0.0.0', 1234))
        self.assertTrue(self.sock.sendto.called)
        self.assertEqual(
            self.sock.sendto.call_args[0], (b'', ('0.0.0.0', 1234)))

    def test_sendto_buffer(self):
        transport = self.datagram_transport()
        transport._buffer.append((b'data1', ('0.0.0.0', 12345)))
        transport.sendto(b'data2', ('0.0.0.0', 12345))
        self.assertFalse(self.sock.sendto.called)
        self.assertEqual(
            [(b'data1', ('0.0.0.0', 12345)),
             (b'data2', ('0.0.0.0', 12345))],
            list(transport._buffer))

    def test_sendto_buffer_bytearray(self):
        data2 = bytearray(b'data2')
        transport = self.datagram_transport()
        transport._buffer.append((b'data1', ('0.0.0.0', 12345)))
        transport.sendto(data2, ('0.0.0.0', 12345))
        self.assertFalse(self.sock.sendto.called)
        self.assertEqual(
            [(b'data1', ('0.0.0.0', 12345)),
             (b'data2', ('0.0.0.0', 12345))],
            list(transport._buffer))
        self.assertIsInstance(transport._buffer[1][0], bytes)

    def test_sendto_buffer_memoryview(self):
        data2 = memoryview(b'data2')
        transport = self.datagram_transport()
        transport._buffer.append((b'data1', ('0.0.0.0', 12345)))
        transport.sendto(data2, ('0.0.0.0', 12345))
        self.assertFalse(self.sock.sendto.called)
        self.assertEqual(
            [(b'data1', ('0.0.0.0', 12345)),
             (b'data2', ('0.0.0.0', 12345))],
            list(transport._buffer))
        self.assertIsInstance(transport._buffer[1][0], bytes)

    def test_sendto_buffer_nodata(self):
        data2 = b''
        transport = self.datagram_transport()
        transport._buffer.append((b'data1', ('0.0.0.0', 12345)))
        transport.sendto(data2, ('0.0.0.0', 12345))
        self.assertFalse(self.sock.sendto.called)
        self.assertEqual(
            [(b'data1', ('0.0.0.0', 12345)),
             (b'', ('0.0.0.0', 12345))],
            list(transport._buffer))
        self.assertIsInstance(transport._buffer[1][0], bytes)

    def test_sendto_tryagain(self):
        data = b'data'

        self.sock.sendto.side_effect = BlockingIOError

        transport = self.datagram_transport()
        transport.sendto(data, ('0.0.0.0', 12345))

        self.loop.assert_writer(7, transport._sendto_ready)
        self.assertEqual(
            [(b'data', ('0.0.0.0', 12345))], list(transport._buffer))

    @mock.patch('asyncio.selector_events.logger')
    def test_sendto_exception(self, m_log):
        data = b'data'
        err = self.sock.sendto.side_effect = RuntimeError()

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
        m_log.warning.assert_called_with('socket.send() raised exception.')

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

        self.sock.send.side_effect = ConnectionRefusedError

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

    def test_sendto_ready(self):
        data = b'data'
        self.sock.sendto.return_value = len(data)

        transport = self.datagram_transport()
        transport._buffer.append((data, ('0.0.0.0', 12345)))
        self.loop._add_writer(7, transport._sendto_ready)
        transport._sendto_ready()
        self.assertTrue(self.sock.sendto.called)
        self.assertEqual(
            self.sock.sendto.call_args[0], (data, ('0.0.0.0', 12345)))
        self.assertFalse(self.loop.writers)

    def test_sendto_ready_closing(self):
        data = b'data'
        self.sock.send.return_value = len(data)

        transport = self.datagram_transport()
        transport._closing = True
        transport._buffer.append((data, ()))
        self.loop._add_writer(7, transport._sendto_ready)
        transport._sendto_ready()
        self.sock.sendto.assert_called_with(data, ())
        self.assertFalse(self.loop.writers)
        self.sock.close.assert_called_with()
        self.protocol.connection_lost.assert_called_with(None)

    def test_sendto_ready_no_data(self):
        transport = self.datagram_transport()
        self.loop._add_writer(7, transport._sendto_ready)
        transport._sendto_ready()
        self.assertFalse(self.sock.sendto.called)
        self.assertFalse(self.loop.writers)

    def test_sendto_ready_tryagain(self):
        self.sock.sendto.side_effect = BlockingIOError

        transport = self.datagram_transport()
        transport._buffer.extend([(b'data1', ()), (b'data2', ())])
        self.loop._add_writer(7, transport._sendto_ready)
        transport._sendto_ready()

        self.loop.assert_writer(7, transport._sendto_ready)
        self.assertEqual(
            [(b'data1', ()), (b'data2', ())],
            list(transport._buffer))

    def test_sendto_ready_exception(self):
        err = self.sock.sendto.side_effect = RuntimeError()

        transport = self.datagram_transport()
        transport._fatal_error = mock.Mock()
        transport._buffer.append((b'data', ()))
        transport._sendto_ready()

        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal write error on datagram transport')

    def test_sendto_ready_error_received(self):
        self.sock.sendto.side_effect = ConnectionRefusedError

        transport = self.datagram_transport()
        transport._fatal_error = mock.Mock()
        transport._buffer.append((b'data', ()))
        transport._sendto_ready()

        self.assertFalse(transport._fatal_error.called)

    def test_sendto_ready_error_received_connection(self):
        self.sock.send.side_effect = ConnectionRefusedError

        transport = self.datagram_transport(address=('0.0.0.0', 1))
        transport._fatal_error = mock.Mock()
        transport._buffer.append((b'data', ()))
        transport._sendto_ready()

        self.assertFalse(transport._fatal_error.called)
        self.assertTrue(self.protocol.error_received.called)

    @mock.patch('asyncio.base_events.logger.error')
    def test_fatal_error_connected(self, m_exc):
        transport = self.datagram_transport(address=('0.0.0.0', 1))
        err = ConnectionRefusedError()
        transport._fatal_error(err)
        self.assertFalse(self.protocol.error_received.called)
        m_exc.assert_not_called()

    @mock.patch('asyncio.base_events.logger.error')
    def test_fatal_error_connected_custom_error(self, m_exc):
        class MyException(Exception):
            pass
        transport = self.datagram_transport(address=('0.0.0.0', 1))
        err = MyException()
        transport._fatal_error(err)
        self.assertFalse(self.protocol.error_received.called)
        m_exc.assert_called_with(
            test_utils.MockPattern(
                'Fatal error on transport\nprotocol:.*\ntransport:.*'),
            exc_info=(MyException, MOCK_ANY, MOCK_ANY))


if __name__ == '__main__':
    unittest.main()
