"""Tests for selector_events.py"""

import collections
import errno
import gc
import pprint
import socket
import sys
import unittest
import unittest.mock
try:
    import ssl
except ImportError:
    ssl = None

import asyncio
from asyncio import selectors
from asyncio import test_utils
from asyncio.selector_events import BaseSelectorEventLoop
from asyncio.selector_events import _SelectorTransport
from asyncio.selector_events import _SelectorSslTransport
from asyncio.selector_events import _SelectorSocketTransport
from asyncio.selector_events import _SelectorDatagramTransport


MOCK_ANY = unittest.mock.ANY


class TestBaseSelectorEventLoop(BaseSelectorEventLoop):

    def _make_self_pipe(self):
        self._ssock = unittest.mock.Mock()
        self._csock = unittest.mock.Mock()
        self._internal_fds += 1


def list_to_buffer(l=()):
    return bytearray().join(l)


class BaseSelectorEventLoopTests(unittest.TestCase):

    def setUp(self):
        selector = unittest.mock.Mock()
        self.loop = TestBaseSelectorEventLoop(selector)

    def test_make_socket_transport(self):
        m = unittest.mock.Mock()
        self.loop.add_reader = unittest.mock.Mock()
        transport = self.loop._make_socket_transport(m, asyncio.Protocol())
        self.assertIsInstance(transport, _SelectorSocketTransport)

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_make_ssl_transport(self):
        m = unittest.mock.Mock()
        self.loop.add_reader = unittest.mock.Mock()
        self.loop.add_writer = unittest.mock.Mock()
        self.loop.remove_reader = unittest.mock.Mock()
        self.loop.remove_writer = unittest.mock.Mock()
        waiter = asyncio.Future(loop=self.loop)
        transport = self.loop._make_ssl_transport(
            m, asyncio.Protocol(), m, waiter)
        self.assertIsInstance(transport, _SelectorSslTransport)

    @unittest.mock.patch('asyncio.selector_events.ssl', None)
    def test_make_ssl_transport_without_ssl_error(self):
        m = unittest.mock.Mock()
        self.loop.add_reader = unittest.mock.Mock()
        self.loop.add_writer = unittest.mock.Mock()
        self.loop.remove_reader = unittest.mock.Mock()
        self.loop.remove_writer = unittest.mock.Mock()
        with self.assertRaises(RuntimeError):
            self.loop._make_ssl_transport(m, m, m, m)

    def test_close(self):
        ssock = self.loop._ssock
        ssock.fileno.return_value = 7
        csock = self.loop._csock
        csock.fileno.return_value = 1
        remove_reader = self.loop.remove_reader = unittest.mock.Mock()

        self.loop._selector.close()
        self.loop._selector = selector = unittest.mock.Mock()
        self.loop.close()
        self.assertIsNone(self.loop._selector)
        self.assertIsNone(self.loop._csock)
        self.assertIsNone(self.loop._ssock)
        selector.close.assert_called_with()
        ssock.close.assert_called_with()
        csock.close.assert_called_with()
        remove_reader.assert_called_with(7)

        self.loop.close()
        self.loop.close()

    def test_close_no_selector(self):
        ssock = self.loop._ssock
        csock = self.loop._csock
        remove_reader = self.loop.remove_reader = unittest.mock.Mock()

        self.loop._selector.close()
        self.loop._selector = None
        self.loop.close()
        self.assertIsNone(self.loop._selector)
        self.assertFalse(ssock.close.called)
        self.assertFalse(csock.close.called)
        self.assertFalse(remove_reader.called)

    def test_socketpair(self):
        self.assertRaises(NotImplementedError, self.loop._socketpair)

    def test_read_from_self_tryagain(self):
        self.loop._ssock.recv.side_effect = BlockingIOError
        self.assertIsNone(self.loop._read_from_self())

    def test_read_from_self_exception(self):
        self.loop._ssock.recv.side_effect = OSError
        self.assertRaises(OSError, self.loop._read_from_self)

    def test_write_to_self_tryagain(self):
        self.loop._csock.send.side_effect = BlockingIOError
        self.assertIsNone(self.loop._write_to_self())

    def test_write_to_self_exception(self):
        self.loop._csock.send.side_effect = OSError()
        self.assertRaises(OSError, self.loop._write_to_self)

    def test_sock_recv(self):
        sock = unittest.mock.Mock()
        self.loop._sock_recv = unittest.mock.Mock()

        f = self.loop.sock_recv(sock, 1024)
        self.assertIsInstance(f, asyncio.Future)
        self.loop._sock_recv.assert_called_with(f, False, sock, 1024)

    def test__sock_recv_canceled_fut(self):
        sock = unittest.mock.Mock()

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop._sock_recv(f, False, sock, 1024)
        self.assertFalse(sock.recv.called)

    def test__sock_recv_unregister(self):
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop.remove_reader = unittest.mock.Mock()
        self.loop._sock_recv(f, True, sock, 1024)
        self.assertEqual((10,), self.loop.remove_reader.call_args[0])

    def test__sock_recv_tryagain(self):
        f = asyncio.Future(loop=self.loop)
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10
        sock.recv.side_effect = BlockingIOError

        self.loop.add_reader = unittest.mock.Mock()
        self.loop._sock_recv(f, False, sock, 1024)
        self.assertEqual((10, self.loop._sock_recv, f, True, sock, 1024),
                         self.loop.add_reader.call_args[0])

    def test__sock_recv_exception(self):
        f = asyncio.Future(loop=self.loop)
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10
        err = sock.recv.side_effect = OSError()

        self.loop._sock_recv(f, False, sock, 1024)
        self.assertIs(err, f.exception())

    def test_sock_sendall(self):
        sock = unittest.mock.Mock()
        self.loop._sock_sendall = unittest.mock.Mock()

        f = self.loop.sock_sendall(sock, b'data')
        self.assertIsInstance(f, asyncio.Future)
        self.assertEqual(
            (f, False, sock, b'data'),
            self.loop._sock_sendall.call_args[0])

    def test_sock_sendall_nodata(self):
        sock = unittest.mock.Mock()
        self.loop._sock_sendall = unittest.mock.Mock()

        f = self.loop.sock_sendall(sock, b'')
        self.assertIsInstance(f, asyncio.Future)
        self.assertTrue(f.done())
        self.assertIsNone(f.result())
        self.assertFalse(self.loop._sock_sendall.called)

    def test__sock_sendall_canceled_fut(self):
        sock = unittest.mock.Mock()

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop._sock_sendall(f, False, sock, b'data')
        self.assertFalse(sock.send.called)

    def test__sock_sendall_unregister(self):
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop.remove_writer = unittest.mock.Mock()
        self.loop._sock_sendall(f, True, sock, b'data')
        self.assertEqual((10,), self.loop.remove_writer.call_args[0])

    def test__sock_sendall_tryagain(self):
        f = asyncio.Future(loop=self.loop)
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10
        sock.send.side_effect = BlockingIOError

        self.loop.add_writer = unittest.mock.Mock()
        self.loop._sock_sendall(f, False, sock, b'data')
        self.assertEqual(
            (10, self.loop._sock_sendall, f, True, sock, b'data'),
            self.loop.add_writer.call_args[0])

    def test__sock_sendall_interrupted(self):
        f = asyncio.Future(loop=self.loop)
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10
        sock.send.side_effect = InterruptedError

        self.loop.add_writer = unittest.mock.Mock()
        self.loop._sock_sendall(f, False, sock, b'data')
        self.assertEqual(
            (10, self.loop._sock_sendall, f, True, sock, b'data'),
            self.loop.add_writer.call_args[0])

    def test__sock_sendall_exception(self):
        f = asyncio.Future(loop=self.loop)
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10
        err = sock.send.side_effect = OSError()

        self.loop._sock_sendall(f, False, sock, b'data')
        self.assertIs(f.exception(), err)

    def test__sock_sendall(self):
        sock = unittest.mock.Mock()

        f = asyncio.Future(loop=self.loop)
        sock.fileno.return_value = 10
        sock.send.return_value = 4

        self.loop._sock_sendall(f, False, sock, b'data')
        self.assertTrue(f.done())
        self.assertIsNone(f.result())

    def test__sock_sendall_partial(self):
        sock = unittest.mock.Mock()

        f = asyncio.Future(loop=self.loop)
        sock.fileno.return_value = 10
        sock.send.return_value = 2

        self.loop.add_writer = unittest.mock.Mock()
        self.loop._sock_sendall(f, False, sock, b'data')
        self.assertFalse(f.done())
        self.assertEqual(
            (10, self.loop._sock_sendall, f, True, sock, b'ta'),
            self.loop.add_writer.call_args[0])

    def test__sock_sendall_none(self):
        sock = unittest.mock.Mock()

        f = asyncio.Future(loop=self.loop)
        sock.fileno.return_value = 10
        sock.send.return_value = 0

        self.loop.add_writer = unittest.mock.Mock()
        self.loop._sock_sendall(f, False, sock, b'data')
        self.assertFalse(f.done())
        self.assertEqual(
            (10, self.loop._sock_sendall, f, True, sock, b'data'),
            self.loop.add_writer.call_args[0])

    def test_sock_connect(self):
        sock = unittest.mock.Mock()
        self.loop._sock_connect = unittest.mock.Mock()

        f = self.loop.sock_connect(sock, ('127.0.0.1', 8080))
        self.assertIsInstance(f, asyncio.Future)
        self.assertEqual(
            (f, False, sock, ('127.0.0.1', 8080)),
            self.loop._sock_connect.call_args[0])

    def test__sock_connect(self):
        f = asyncio.Future(loop=self.loop)

        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10

        self.loop._sock_connect(f, False, sock, ('127.0.0.1', 8080))
        self.assertTrue(f.done())
        self.assertIsNone(f.result())
        self.assertTrue(sock.connect.called)

    def test__sock_connect_canceled_fut(self):
        sock = unittest.mock.Mock()

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop._sock_connect(f, False, sock, ('127.0.0.1', 8080))
        self.assertFalse(sock.connect.called)

    def test__sock_connect_unregister(self):
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop.remove_writer = unittest.mock.Mock()
        self.loop._sock_connect(f, True, sock, ('127.0.0.1', 8080))
        self.assertEqual((10,), self.loop.remove_writer.call_args[0])

    def test__sock_connect_tryagain(self):
        f = asyncio.Future(loop=self.loop)
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10
        sock.getsockopt.return_value = errno.EAGAIN

        self.loop.add_writer = unittest.mock.Mock()
        self.loop.remove_writer = unittest.mock.Mock()

        self.loop._sock_connect(f, True, sock, ('127.0.0.1', 8080))
        self.assertEqual(
            (10, self.loop._sock_connect, f,
             True, sock, ('127.0.0.1', 8080)),
            self.loop.add_writer.call_args[0])

    def test__sock_connect_exception(self):
        f = asyncio.Future(loop=self.loop)
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10
        sock.getsockopt.return_value = errno.ENOTCONN

        self.loop.remove_writer = unittest.mock.Mock()
        self.loop._sock_connect(f, True, sock, ('127.0.0.1', 8080))
        self.assertIsInstance(f.exception(), OSError)

    def test_sock_accept(self):
        sock = unittest.mock.Mock()
        self.loop._sock_accept = unittest.mock.Mock()

        f = self.loop.sock_accept(sock)
        self.assertIsInstance(f, asyncio.Future)
        self.assertEqual(
            (f, False, sock), self.loop._sock_accept.call_args[0])

    def test__sock_accept(self):
        f = asyncio.Future(loop=self.loop)

        conn = unittest.mock.Mock()

        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10
        sock.accept.return_value = conn, ('127.0.0.1', 1000)

        self.loop._sock_accept(f, False, sock)
        self.assertTrue(f.done())
        self.assertEqual((conn, ('127.0.0.1', 1000)), f.result())
        self.assertEqual((False,), conn.setblocking.call_args[0])

    def test__sock_accept_canceled_fut(self):
        sock = unittest.mock.Mock()

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop._sock_accept(f, False, sock)
        self.assertFalse(sock.accept.called)

    def test__sock_accept_unregister(self):
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop.remove_reader = unittest.mock.Mock()
        self.loop._sock_accept(f, True, sock)
        self.assertEqual((10,), self.loop.remove_reader.call_args[0])

    def test__sock_accept_tryagain(self):
        f = asyncio.Future(loop=self.loop)
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10
        sock.accept.side_effect = BlockingIOError

        self.loop.add_reader = unittest.mock.Mock()
        self.loop._sock_accept(f, False, sock)
        self.assertEqual(
            (10, self.loop._sock_accept, f, True, sock),
            self.loop.add_reader.call_args[0])

    def test__sock_accept_exception(self):
        f = asyncio.Future(loop=self.loop)
        sock = unittest.mock.Mock()
        sock.fileno.return_value = 10
        err = sock.accept.side_effect = OSError()

        self.loop._sock_accept(f, False, sock)
        self.assertIs(err, f.exception())

    def test_add_reader(self):
        self.loop._selector.get_key.side_effect = KeyError
        cb = lambda: True
        self.loop.add_reader(1, cb)

        self.assertTrue(self.loop._selector.register.called)
        fd, mask, (r, w) = self.loop._selector.register.call_args[0]
        self.assertEqual(1, fd)
        self.assertEqual(selectors.EVENT_READ, mask)
        self.assertEqual(cb, r._callback)
        self.assertIsNone(w)

    def test_add_reader_existing(self):
        reader = unittest.mock.Mock()
        writer = unittest.mock.Mock()
        self.loop._selector.get_key.return_value = selectors.SelectorKey(
            1, 1, selectors.EVENT_WRITE, (reader, writer))
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
        writer = unittest.mock.Mock()
        self.loop._selector.get_key.return_value = selectors.SelectorKey(
            1, 1, selectors.EVENT_WRITE, (None, writer))
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
        self.loop._selector.get_key.return_value = selectors.SelectorKey(
            1, 1, selectors.EVENT_READ, (None, None))
        self.assertFalse(self.loop.remove_reader(1))

        self.assertTrue(self.loop._selector.unregister.called)

    def test_remove_reader_read_write(self):
        reader = unittest.mock.Mock()
        writer = unittest.mock.Mock()
        self.loop._selector.get_key.return_value = selectors.SelectorKey(
            1, 1, selectors.EVENT_READ | selectors.EVENT_WRITE,
            (reader, writer))
        self.assertTrue(
            self.loop.remove_reader(1))

        self.assertFalse(self.loop._selector.unregister.called)
        self.assertEqual(
            (1, selectors.EVENT_WRITE, (None, writer)),
            self.loop._selector.modify.call_args[0])

    def test_remove_reader_unknown(self):
        self.loop._selector.get_key.side_effect = KeyError
        self.assertFalse(
            self.loop.remove_reader(1))

    def test_add_writer(self):
        self.loop._selector.get_key.side_effect = KeyError
        cb = lambda: True
        self.loop.add_writer(1, cb)

        self.assertTrue(self.loop._selector.register.called)
        fd, mask, (r, w) = self.loop._selector.register.call_args[0]
        self.assertEqual(1, fd)
        self.assertEqual(selectors.EVENT_WRITE, mask)
        self.assertIsNone(r)
        self.assertEqual(cb, w._callback)

    def test_add_writer_existing(self):
        reader = unittest.mock.Mock()
        writer = unittest.mock.Mock()
        self.loop._selector.get_key.return_value = selectors.SelectorKey(
            1, 1, selectors.EVENT_READ, (reader, writer))
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
        self.loop._selector.get_key.return_value = selectors.SelectorKey(
            1, 1, selectors.EVENT_WRITE, (None, None))
        self.assertFalse(self.loop.remove_writer(1))

        self.assertTrue(self.loop._selector.unregister.called)

    def test_remove_writer_read_write(self):
        reader = unittest.mock.Mock()
        writer = unittest.mock.Mock()
        self.loop._selector.get_key.return_value = selectors.SelectorKey(
            1, 1, selectors.EVENT_READ | selectors.EVENT_WRITE,
            (reader, writer))
        self.assertTrue(
            self.loop.remove_writer(1))

        self.assertFalse(self.loop._selector.unregister.called)
        self.assertEqual(
            (1, selectors.EVENT_READ, (reader, None)),
            self.loop._selector.modify.call_args[0])

    def test_remove_writer_unknown(self):
        self.loop._selector.get_key.side_effect = KeyError
        self.assertFalse(
            self.loop.remove_writer(1))

    def test_process_events_read(self):
        reader = unittest.mock.Mock()
        reader._cancelled = False

        self.loop._add_callback = unittest.mock.Mock()
        self.loop._process_events(
            [(selectors.SelectorKey(
                1, 1, selectors.EVENT_READ, (reader, None)),
              selectors.EVENT_READ)])
        self.assertTrue(self.loop._add_callback.called)
        self.loop._add_callback.assert_called_with(reader)

    def test_process_events_read_cancelled(self):
        reader = unittest.mock.Mock()
        reader.cancelled = True

        self.loop.remove_reader = unittest.mock.Mock()
        self.loop._process_events(
            [(selectors.SelectorKey(
                1, 1, selectors.EVENT_READ, (reader, None)),
             selectors.EVENT_READ)])
        self.loop.remove_reader.assert_called_with(1)

    def test_process_events_write(self):
        writer = unittest.mock.Mock()
        writer._cancelled = False

        self.loop._add_callback = unittest.mock.Mock()
        self.loop._process_events(
            [(selectors.SelectorKey(1, 1, selectors.EVENT_WRITE,
                                    (None, writer)),
              selectors.EVENT_WRITE)])
        self.loop._add_callback.assert_called_with(writer)

    def test_process_events_write_cancelled(self):
        writer = unittest.mock.Mock()
        writer.cancelled = True
        self.loop.remove_writer = unittest.mock.Mock()

        self.loop._process_events(
            [(selectors.SelectorKey(1, 1, selectors.EVENT_WRITE,
                                    (None, writer)),
              selectors.EVENT_WRITE)])
        self.loop.remove_writer.assert_called_with(1)


class SelectorTransportTests(unittest.TestCase):

    def setUp(self):
        self.loop = test_utils.TestLoop()
        self.protocol = test_utils.make_test_protocol(asyncio.Protocol)
        self.sock = unittest.mock.Mock(socket.socket)
        self.sock.fileno.return_value = 7

    def test_ctor(self):
        tr = _SelectorTransport(self.loop, self.sock, self.protocol, None)
        self.assertIs(tr._loop, self.loop)
        self.assertIs(tr._sock, self.sock)
        self.assertIs(tr._sock_fd, 7)

    def test_abort(self):
        tr = _SelectorTransport(self.loop, self.sock, self.protocol, None)
        tr._force_close = unittest.mock.Mock()

        tr.abort()
        tr._force_close.assert_called_with(None)

    def test_close(self):
        tr = _SelectorTransport(self.loop, self.sock, self.protocol, None)
        tr.close()

        self.assertTrue(tr._closing)
        self.assertEqual(1, self.loop.remove_reader_count[7])
        self.protocol.connection_lost(None)
        self.assertEqual(tr._conn_lost, 1)

        tr.close()
        self.assertEqual(tr._conn_lost, 1)
        self.assertEqual(1, self.loop.remove_reader_count[7])

    def test_close_write_buffer(self):
        tr = _SelectorTransport(self.loop, self.sock, self.protocol, None)
        tr._buffer.extend(b'data')
        tr.close()

        self.assertFalse(self.loop.readers)
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.connection_lost.called)

    def test_force_close(self):
        tr = _SelectorTransport(self.loop, self.sock, self.protocol, None)
        tr._buffer.extend(b'1')
        self.loop.add_reader(7, unittest.mock.sentinel)
        self.loop.add_writer(7, unittest.mock.sentinel)
        tr._force_close(None)

        self.assertTrue(tr._closing)
        self.assertEqual(tr._buffer, list_to_buffer())
        self.assertFalse(self.loop.readers)
        self.assertFalse(self.loop.writers)

        # second close should not remove reader
        tr._force_close(None)
        self.assertFalse(self.loop.readers)
        self.assertEqual(1, self.loop.remove_reader_count[7])

    @unittest.mock.patch('asyncio.log.logger.error')
    def test_fatal_error(self, m_exc):
        exc = OSError()
        tr = _SelectorTransport(self.loop, self.sock, self.protocol, None)
        tr._force_close = unittest.mock.Mock()
        tr._fatal_error(exc)

        m_exc.assert_called_with(
            test_utils.MockPattern(
                'Fatal error on transport\nprotocol:.*\ntransport:.*'),
            exc_info=(OSError, MOCK_ANY, MOCK_ANY))

        tr._force_close.assert_called_with(exc)

    def test_connection_lost(self):
        exc = OSError()
        tr = _SelectorTransport(self.loop, self.sock, self.protocol, None)
        tr._call_connection_lost(exc)

        self.protocol.connection_lost.assert_called_with(exc)
        self.sock.close.assert_called_with()
        self.assertIsNone(tr._sock)

        self.assertIsNone(tr._protocol)
        self.assertEqual(2, sys.getrefcount(self.protocol),
                         pprint.pformat(gc.get_referrers(self.protocol)))
        self.assertIsNone(tr._loop)
        self.assertEqual(2, sys.getrefcount(self.loop),
                         pprint.pformat(gc.get_referrers(self.loop)))


class SelectorSocketTransportTests(unittest.TestCase):

    def setUp(self):
        self.loop = test_utils.TestLoop()
        self.protocol = test_utils.make_test_protocol(asyncio.Protocol)
        self.sock = unittest.mock.Mock(socket.socket)
        self.sock_fd = self.sock.fileno.return_value = 7

    def test_ctor(self):
        tr = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        self.loop.assert_reader(7, tr._read_ready)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_made.assert_called_with(tr)

    def test_ctor_with_waiter(self):
        fut = asyncio.Future(loop=self.loop)

        _SelectorSocketTransport(
            self.loop, self.sock, self.protocol, fut)
        test_utils.run_briefly(self.loop)
        self.assertIsNone(fut.result())

    def test_pause_resume_reading(self):
        tr = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        self.assertFalse(tr._paused)
        self.loop.assert_reader(7, tr._read_ready)
        tr.pause_reading()
        self.assertTrue(tr._paused)
        self.assertFalse(7 in self.loop.readers)
        tr.resume_reading()
        self.assertFalse(tr._paused)
        self.loop.assert_reader(7, tr._read_ready)

    def test_read_ready(self):
        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)

        self.sock.recv.return_value = b'data'
        transport._read_ready()

        self.protocol.data_received.assert_called_with(b'data')

    def test_read_ready_eof(self):
        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.close = unittest.mock.Mock()

        self.sock.recv.return_value = b''
        transport._read_ready()

        self.protocol.eof_received.assert_called_with()
        transport.close.assert_called_with()

    def test_read_ready_eof_keep_open(self):
        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.close = unittest.mock.Mock()

        self.sock.recv.return_value = b''
        self.protocol.eof_received.return_value = True
        transport._read_ready()

        self.protocol.eof_received.assert_called_with()
        self.assertFalse(transport.close.called)

    @unittest.mock.patch('logging.exception')
    def test_read_ready_tryagain(self, m_exc):
        self.sock.recv.side_effect = BlockingIOError

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._fatal_error = unittest.mock.Mock()
        transport._read_ready()

        self.assertFalse(transport._fatal_error.called)

    @unittest.mock.patch('logging.exception')
    def test_read_ready_tryagain_interrupted(self, m_exc):
        self.sock.recv.side_effect = InterruptedError

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._fatal_error = unittest.mock.Mock()
        transport._read_ready()

        self.assertFalse(transport._fatal_error.called)

    @unittest.mock.patch('logging.exception')
    def test_read_ready_conn_reset(self, m_exc):
        err = self.sock.recv.side_effect = ConnectionResetError()

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._force_close = unittest.mock.Mock()
        transport._read_ready()
        transport._force_close.assert_called_with(err)

    @unittest.mock.patch('logging.exception')
    def test_read_ready_err(self, m_exc):
        err = self.sock.recv.side_effect = OSError()

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._fatal_error = unittest.mock.Mock()
        transport._read_ready()

        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal read error on socket transport')

    def test_write(self):
        data = b'data'
        self.sock.send.return_value = len(data)

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.write(data)
        self.sock.send.assert_called_with(data)

    def test_write_bytearray(self):
        data = bytearray(b'data')
        self.sock.send.return_value = len(data)

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.write(data)
        self.sock.send.assert_called_with(data)
        self.assertEqual(data, bytearray(b'data'))  # Hasn't been mutated.

    def test_write_memoryview(self):
        data = memoryview(b'data')
        self.sock.send.return_value = len(data)

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.write(data)
        self.sock.send.assert_called_with(data)

    def test_write_no_data(self):
        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._buffer.extend(b'data')
        transport.write(b'')
        self.assertFalse(self.sock.send.called)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_buffer(self):
        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._buffer.extend(b'data1')
        transport.write(b'data2')
        self.assertFalse(self.sock.send.called)
        self.assertEqual(list_to_buffer([b'data1', b'data2']),
                         transport._buffer)

    def test_write_partial(self):
        data = b'data'
        self.sock.send.return_value = 2

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.write(data)

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'ta']), transport._buffer)

    def test_write_partial_bytearray(self):
        data = bytearray(b'data')
        self.sock.send.return_value = 2

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.write(data)

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'ta']), transport._buffer)
        self.assertEqual(data, bytearray(b'data'))  # Hasn't been mutated.

    def test_write_partial_memoryview(self):
        data = memoryview(b'data')
        self.sock.send.return_value = 2

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.write(data)

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'ta']), transport._buffer)

    def test_write_partial_none(self):
        data = b'data'
        self.sock.send.return_value = 0
        self.sock.fileno.return_value = 7

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.write(data)

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_tryagain(self):
        self.sock.send.side_effect = BlockingIOError

        data = b'data'
        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.write(data)

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    @unittest.mock.patch('asyncio.selector_events.logger')
    def test_write_exception(self, m_log):
        err = self.sock.send.side_effect = OSError()

        data = b'data'
        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._fatal_error = unittest.mock.Mock()
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
        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        self.assertRaises(TypeError, transport.write, 'str')

    def test_write_closing(self):
        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.close()
        self.assertEqual(transport._conn_lost, 1)
        transport.write(b'data')
        self.assertEqual(transport._conn_lost, 2)

    def test_write_ready(self):
        data = b'data'
        self.sock.send.return_value = len(data)

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._buffer.extend(data)
        self.loop.add_writer(7, transport._write_ready)
        transport._write_ready()
        self.assertTrue(self.sock.send.called)
        self.assertFalse(self.loop.writers)

    def test_write_ready_closing(self):
        data = b'data'
        self.sock.send.return_value = len(data)

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._closing = True
        transport._buffer.extend(data)
        self.loop.add_writer(7, transport._write_ready)
        transport._write_ready()
        self.assertTrue(self.sock.send.called)
        self.assertFalse(self.loop.writers)
        self.sock.close.assert_called_with()
        self.protocol.connection_lost.assert_called_with(None)

    def test_write_ready_no_data(self):
        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        # This is an internal error.
        self.assertRaises(AssertionError, transport._write_ready)

    def test_write_ready_partial(self):
        data = b'data'
        self.sock.send.return_value = 2

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._buffer.extend(data)
        self.loop.add_writer(7, transport._write_ready)
        transport._write_ready()
        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'ta']), transport._buffer)

    def test_write_ready_partial_none(self):
        data = b'data'
        self.sock.send.return_value = 0

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._buffer.extend(data)
        self.loop.add_writer(7, transport._write_ready)
        transport._write_ready()
        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_ready_tryagain(self):
        self.sock.send.side_effect = BlockingIOError

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._buffer = list_to_buffer([b'data1', b'data2'])
        self.loop.add_writer(7, transport._write_ready)
        transport._write_ready()

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'data1data2']), transport._buffer)

    def test_write_ready_exception(self):
        err = self.sock.send.side_effect = OSError()

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport._fatal_error = unittest.mock.Mock()
        transport._buffer.extend(b'data')
        transport._write_ready()
        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal write error on socket transport')

    @unittest.mock.patch('asyncio.base_events.logger')
    def test_write_ready_exception_and_close(self, m_log):
        self.sock.send.side_effect = OSError()
        remove_writer = self.loop.remove_writer = unittest.mock.Mock()

        transport = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        transport.close()
        transport._buffer.extend(b'data')
        transport._write_ready()
        remove_writer.assert_called_with(self.sock_fd)

    def test_write_eof(self):
        tr = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
        self.assertTrue(tr.can_write_eof())
        tr.write_eof()
        self.sock.shutdown.assert_called_with(socket.SHUT_WR)
        tr.write_eof()
        self.assertEqual(self.sock.shutdown.call_count, 1)
        tr.close()

    def test_write_eof_buffer(self):
        tr = _SelectorSocketTransport(
            self.loop, self.sock, self.protocol)
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


@unittest.skipIf(ssl is None, 'No ssl module')
class SelectorSslTransportTests(unittest.TestCase):

    def setUp(self):
        self.loop = test_utils.TestLoop()
        self.protocol = test_utils.make_test_protocol(asyncio.Protocol)
        self.sock = unittest.mock.Mock(socket.socket)
        self.sock.fileno.return_value = 7
        self.sslsock = unittest.mock.Mock()
        self.sslsock.fileno.return_value = 1
        self.sslcontext = unittest.mock.Mock()
        self.sslcontext.wrap_socket.return_value = self.sslsock

    def _make_one(self, create_waiter=None):
        transport = _SelectorSslTransport(
            self.loop, self.sock, self.protocol, self.sslcontext)
        self.sock.reset_mock()
        self.sslsock.reset_mock()
        self.sslcontext.reset_mock()
        self.loop.reset_counters()
        return transport

    def test_on_handshake(self):
        waiter = asyncio.Future(loop=self.loop)
        tr = _SelectorSslTransport(
            self.loop, self.sock, self.protocol, self.sslcontext,
            waiter=waiter)
        self.assertTrue(self.sslsock.do_handshake.called)
        self.loop.assert_reader(1, tr._read_ready)
        test_utils.run_briefly(self.loop)
        self.assertIsNone(waiter.result())

    def test_on_handshake_reader_retry(self):
        self.sslsock.do_handshake.side_effect = ssl.SSLWantReadError
        transport = _SelectorSslTransport(
            self.loop, self.sock, self.protocol, self.sslcontext)
        transport._on_handshake()
        self.loop.assert_reader(1, transport._on_handshake)

    def test_on_handshake_writer_retry(self):
        self.sslsock.do_handshake.side_effect = ssl.SSLWantWriteError
        transport = _SelectorSslTransport(
            self.loop, self.sock, self.protocol, self.sslcontext)
        transport._on_handshake()
        self.loop.assert_writer(1, transport._on_handshake)

    def test_on_handshake_exc(self):
        exc = ValueError()
        self.sslsock.do_handshake.side_effect = exc
        transport = _SelectorSslTransport(
            self.loop, self.sock, self.protocol, self.sslcontext)
        transport._waiter = asyncio.Future(loop=self.loop)
        transport._on_handshake()
        self.assertTrue(self.sslsock.close.called)
        self.assertTrue(transport._waiter.done())
        self.assertIs(exc, transport._waiter.exception())

    def test_on_handshake_base_exc(self):
        transport = _SelectorSslTransport(
            self.loop, self.sock, self.protocol, self.sslcontext)
        transport._waiter = asyncio.Future(loop=self.loop)
        exc = BaseException()
        self.sslsock.do_handshake.side_effect = exc
        self.assertRaises(BaseException, transport._on_handshake)
        self.assertTrue(self.sslsock.close.called)
        self.assertTrue(transport._waiter.done())
        self.assertIs(exc, transport._waiter.exception())

    def test_pause_resume_reading(self):
        tr = self._make_one()
        self.assertFalse(tr._paused)
        self.loop.assert_reader(1, tr._read_ready)
        tr.pause_reading()
        self.assertTrue(tr._paused)
        self.assertFalse(1 in self.loop.readers)
        tr.resume_reading()
        self.assertFalse(tr._paused)
        self.loop.assert_reader(1, tr._read_ready)

    def test_write(self):
        transport = self._make_one()
        transport.write(b'data')
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_bytearray(self):
        transport = self._make_one()
        data = bytearray(b'data')
        transport.write(data)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)
        self.assertEqual(data, bytearray(b'data'))  # Hasn't been mutated.
        self.assertIsNot(data, transport._buffer)  # Hasn't been incorporated.

    def test_write_memoryview(self):
        transport = self._make_one()
        data = memoryview(b'data')
        transport.write(data)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_no_data(self):
        transport = self._make_one()
        transport._buffer.extend(b'data')
        transport.write(b'')
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_str(self):
        transport = self._make_one()
        self.assertRaises(TypeError, transport.write, 'str')

    def test_write_closing(self):
        transport = self._make_one()
        transport.close()
        self.assertEqual(transport._conn_lost, 1)
        transport.write(b'data')
        self.assertEqual(transport._conn_lost, 2)

    @unittest.mock.patch('asyncio.selector_events.logger')
    def test_write_exception(self, m_log):
        transport = self._make_one()
        transport._conn_lost = 1
        transport.write(b'data')
        self.assertEqual(transport._buffer, list_to_buffer())
        transport.write(b'data')
        transport.write(b'data')
        transport.write(b'data')
        transport.write(b'data')
        m_log.warning.assert_called_with('socket.send() raised exception.')

    def test_read_ready_recv(self):
        self.sslsock.recv.return_value = b'data'
        transport = self._make_one()
        transport._read_ready()
        self.assertTrue(self.sslsock.recv.called)
        self.assertEqual((b'data',), self.protocol.data_received.call_args[0])

    def test_read_ready_write_wants_read(self):
        self.loop.add_writer = unittest.mock.Mock()
        self.sslsock.recv.side_effect = BlockingIOError
        transport = self._make_one()
        transport._write_wants_read = True
        transport._write_ready = unittest.mock.Mock()
        transport._buffer.extend(b'data')
        transport._read_ready()

        self.assertFalse(transport._write_wants_read)
        transport._write_ready.assert_called_with()
        self.loop.add_writer.assert_called_with(
            transport._sock_fd, transport._write_ready)

    def test_read_ready_recv_eof(self):
        self.sslsock.recv.return_value = b''
        transport = self._make_one()
        transport.close = unittest.mock.Mock()
        transport._read_ready()
        transport.close.assert_called_with()
        self.protocol.eof_received.assert_called_with()

    def test_read_ready_recv_conn_reset(self):
        err = self.sslsock.recv.side_effect = ConnectionResetError()
        transport = self._make_one()
        transport._force_close = unittest.mock.Mock()
        transport._read_ready()
        transport._force_close.assert_called_with(err)

    def test_read_ready_recv_retry(self):
        self.sslsock.recv.side_effect = ssl.SSLWantReadError
        transport = self._make_one()
        transport._read_ready()
        self.assertTrue(self.sslsock.recv.called)
        self.assertFalse(self.protocol.data_received.called)

        self.sslsock.recv.side_effect = BlockingIOError
        transport._read_ready()
        self.assertFalse(self.protocol.data_received.called)

        self.sslsock.recv.side_effect = InterruptedError
        transport._read_ready()
        self.assertFalse(self.protocol.data_received.called)

    def test_read_ready_recv_write(self):
        self.loop.remove_reader = unittest.mock.Mock()
        self.loop.add_writer = unittest.mock.Mock()
        self.sslsock.recv.side_effect = ssl.SSLWantWriteError
        transport = self._make_one()
        transport._read_ready()
        self.assertFalse(self.protocol.data_received.called)
        self.assertTrue(transport._read_wants_write)

        self.loop.remove_reader.assert_called_with(transport._sock_fd)
        self.loop.add_writer.assert_called_with(
            transport._sock_fd, transport._write_ready)

    def test_read_ready_recv_exc(self):
        err = self.sslsock.recv.side_effect = OSError()
        transport = self._make_one()
        transport._fatal_error = unittest.mock.Mock()
        transport._read_ready()
        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal read error on SSL transport')

    def test_write_ready_send(self):
        self.sslsock.send.return_value = 4
        transport = self._make_one()
        transport._buffer = list_to_buffer([b'data'])
        transport._write_ready()
        self.assertEqual(list_to_buffer(), transport._buffer)
        self.assertTrue(self.sslsock.send.called)

    def test_write_ready_send_none(self):
        self.sslsock.send.return_value = 0
        transport = self._make_one()
        transport._buffer = list_to_buffer([b'data1', b'data2'])
        transport._write_ready()
        self.assertTrue(self.sslsock.send.called)
        self.assertEqual(list_to_buffer([b'data1data2']), transport._buffer)

    def test_write_ready_send_partial(self):
        self.sslsock.send.return_value = 2
        transport = self._make_one()
        transport._buffer = list_to_buffer([b'data1', b'data2'])
        transport._write_ready()
        self.assertTrue(self.sslsock.send.called)
        self.assertEqual(list_to_buffer([b'ta1data2']), transport._buffer)

    def test_write_ready_send_closing_partial(self):
        self.sslsock.send.return_value = 2
        transport = self._make_one()
        transport._buffer = list_to_buffer([b'data1', b'data2'])
        transport._write_ready()
        self.assertTrue(self.sslsock.send.called)
        self.assertFalse(self.sslsock.close.called)

    def test_write_ready_send_closing(self):
        self.sslsock.send.return_value = 4
        transport = self._make_one()
        transport.close()
        transport._buffer = list_to_buffer([b'data'])
        transport._write_ready()
        self.assertFalse(self.loop.writers)
        self.protocol.connection_lost.assert_called_with(None)

    def test_write_ready_send_closing_empty_buffer(self):
        self.sslsock.send.return_value = 4
        transport = self._make_one()
        transport.close()
        transport._buffer = list_to_buffer()
        transport._write_ready()
        self.assertFalse(self.loop.writers)
        self.protocol.connection_lost.assert_called_with(None)

    def test_write_ready_send_retry(self):
        transport = self._make_one()
        transport._buffer = list_to_buffer([b'data'])

        self.sslsock.send.side_effect = ssl.SSLWantWriteError
        transport._write_ready()
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

        self.sslsock.send.side_effect = BlockingIOError()
        transport._write_ready()
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_ready_send_read(self):
        transport = self._make_one()
        transport._buffer = list_to_buffer([b'data'])

        self.loop.remove_writer = unittest.mock.Mock()
        self.sslsock.send.side_effect = ssl.SSLWantReadError
        transport._write_ready()
        self.assertFalse(self.protocol.data_received.called)
        self.assertTrue(transport._write_wants_read)
        self.loop.remove_writer.assert_called_with(transport._sock_fd)

    def test_write_ready_send_exc(self):
        err = self.sslsock.send.side_effect = OSError()

        transport = self._make_one()
        transport._buffer = list_to_buffer([b'data'])
        transport._fatal_error = unittest.mock.Mock()
        transport._write_ready()
        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal write error on SSL transport')
        self.assertEqual(list_to_buffer(), transport._buffer)

    def test_write_ready_read_wants_write(self):
        self.loop.add_reader = unittest.mock.Mock()
        self.sslsock.send.side_effect = BlockingIOError
        transport = self._make_one()
        transport._read_wants_write = True
        transport._read_ready = unittest.mock.Mock()
        transport._write_ready()

        self.assertFalse(transport._read_wants_write)
        transport._read_ready.assert_called_with()
        self.loop.add_reader.assert_called_with(
            transport._sock_fd, transport._read_ready)

    def test_write_eof(self):
        tr = self._make_one()
        self.assertFalse(tr.can_write_eof())
        self.assertRaises(NotImplementedError, tr.write_eof)

    def test_close(self):
        tr = self._make_one()
        tr.close()

        self.assertTrue(tr._closing)
        self.assertEqual(1, self.loop.remove_reader_count[1])
        self.assertEqual(tr._conn_lost, 1)

        tr.close()
        self.assertEqual(tr._conn_lost, 1)
        self.assertEqual(1, self.loop.remove_reader_count[1])

    @unittest.skipIf(ssl is None or not ssl.HAS_SNI, 'No SNI support')
    def test_server_hostname(self):
        _SelectorSslTransport(
            self.loop, self.sock, self.protocol, self.sslcontext,
            server_hostname='localhost')
        self.sslcontext.wrap_socket.assert_called_with(
            self.sock, do_handshake_on_connect=False, server_side=False,
            server_hostname='localhost')


class SelectorSslWithoutSslTransportTests(unittest.TestCase):

    @unittest.mock.patch('asyncio.selector_events.ssl', None)
    def test_ssl_transport_requires_ssl_module(self):
        Mock = unittest.mock.Mock
        with self.assertRaises(RuntimeError):
            transport = _SelectorSslTransport(Mock(), Mock(), Mock(), Mock())


class SelectorDatagramTransportTests(unittest.TestCase):

    def setUp(self):
        self.loop = test_utils.TestLoop()
        self.protocol = test_utils.make_test_protocol(asyncio.DatagramProtocol)
        self.sock = unittest.mock.Mock(spec_set=socket.socket)
        self.sock.fileno.return_value = 7

    def test_read_ready(self):
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)

        self.sock.recvfrom.return_value = (b'data', ('0.0.0.0', 1234))
        transport._read_ready()

        self.protocol.datagram_received.assert_called_with(
            b'data', ('0.0.0.0', 1234))

    def test_read_ready_tryagain(self):
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)

        self.sock.recvfrom.side_effect = BlockingIOError
        transport._fatal_error = unittest.mock.Mock()
        transport._read_ready()

        self.assertFalse(transport._fatal_error.called)

    def test_read_ready_err(self):
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)

        err = self.sock.recvfrom.side_effect = RuntimeError()
        transport._fatal_error = unittest.mock.Mock()
        transport._read_ready()

        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal read error on datagram transport')

    def test_read_ready_oserr(self):
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)

        err = self.sock.recvfrom.side_effect = OSError()
        transport._fatal_error = unittest.mock.Mock()
        transport._read_ready()

        self.assertFalse(transport._fatal_error.called)
        self.protocol.error_received.assert_called_with(err)

    def test_sendto(self):
        data = b'data'
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport.sendto(data, ('0.0.0.0', 1234))
        self.assertTrue(self.sock.sendto.called)
        self.assertEqual(
            self.sock.sendto.call_args[0], (data, ('0.0.0.0', 1234)))

    def test_sendto_bytearray(self):
        data = bytearray(b'data')
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport.sendto(data, ('0.0.0.0', 1234))
        self.assertTrue(self.sock.sendto.called)
        self.assertEqual(
            self.sock.sendto.call_args[0], (data, ('0.0.0.0', 1234)))

    def test_sendto_memoryview(self):
        data = memoryview(b'data')
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport.sendto(data, ('0.0.0.0', 1234))
        self.assertTrue(self.sock.sendto.called)
        self.assertEqual(
            self.sock.sendto.call_args[0], (data, ('0.0.0.0', 1234)))

    def test_sendto_no_data(self):
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport._buffer.append((b'data', ('0.0.0.0', 12345)))
        transport.sendto(b'', ())
        self.assertFalse(self.sock.sendto.called)
        self.assertEqual(
            [(b'data', ('0.0.0.0', 12345))], list(transport._buffer))

    def test_sendto_buffer(self):
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport._buffer.append((b'data1', ('0.0.0.0', 12345)))
        transport.sendto(b'data2', ('0.0.0.0', 12345))
        self.assertFalse(self.sock.sendto.called)
        self.assertEqual(
            [(b'data1', ('0.0.0.0', 12345)),
             (b'data2', ('0.0.0.0', 12345))],
            list(transport._buffer))

    def test_sendto_buffer_bytearray(self):
        data2 = bytearray(b'data2')
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
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
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport._buffer.append((b'data1', ('0.0.0.0', 12345)))
        transport.sendto(data2, ('0.0.0.0', 12345))
        self.assertFalse(self.sock.sendto.called)
        self.assertEqual(
            [(b'data1', ('0.0.0.0', 12345)),
             (b'data2', ('0.0.0.0', 12345))],
            list(transport._buffer))
        self.assertIsInstance(transport._buffer[1][0], bytes)

    def test_sendto_tryagain(self):
        data = b'data'

        self.sock.sendto.side_effect = BlockingIOError

        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport.sendto(data, ('0.0.0.0', 12345))

        self.loop.assert_writer(7, transport._sendto_ready)
        self.assertEqual(
            [(b'data', ('0.0.0.0', 12345))], list(transport._buffer))

    @unittest.mock.patch('asyncio.selector_events.logger')
    def test_sendto_exception(self, m_log):
        data = b'data'
        err = self.sock.sendto.side_effect = RuntimeError()

        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport._fatal_error = unittest.mock.Mock()
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

        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport._fatal_error = unittest.mock.Mock()
        transport.sendto(data, ())

        self.assertEqual(transport._conn_lost, 0)
        self.assertFalse(transport._fatal_error.called)

    def test_sendto_error_received_connected(self):
        data = b'data'

        self.sock.send.side_effect = ConnectionRefusedError

        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol, ('0.0.0.0', 1))
        transport._fatal_error = unittest.mock.Mock()
        transport.sendto(data)

        self.assertFalse(transport._fatal_error.called)
        self.assertTrue(self.protocol.error_received.called)

    def test_sendto_str(self):
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        self.assertRaises(TypeError, transport.sendto, 'str', ())

    def test_sendto_connected_addr(self):
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol, ('0.0.0.0', 1))
        self.assertRaises(
            ValueError, transport.sendto, b'str', ('0.0.0.0', 2))

    def test_sendto_closing(self):
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol, address=(1,))
        transport.close()
        self.assertEqual(transport._conn_lost, 1)
        transport.sendto(b'data', (1,))
        self.assertEqual(transport._conn_lost, 2)

    def test_sendto_ready(self):
        data = b'data'
        self.sock.sendto.return_value = len(data)

        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport._buffer.append((data, ('0.0.0.0', 12345)))
        self.loop.add_writer(7, transport._sendto_ready)
        transport._sendto_ready()
        self.assertTrue(self.sock.sendto.called)
        self.assertEqual(
            self.sock.sendto.call_args[0], (data, ('0.0.0.0', 12345)))
        self.assertFalse(self.loop.writers)

    def test_sendto_ready_closing(self):
        data = b'data'
        self.sock.send.return_value = len(data)

        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport._closing = True
        transport._buffer.append((data, ()))
        self.loop.add_writer(7, transport._sendto_ready)
        transport._sendto_ready()
        self.sock.sendto.assert_called_with(data, ())
        self.assertFalse(self.loop.writers)
        self.sock.close.assert_called_with()
        self.protocol.connection_lost.assert_called_with(None)

    def test_sendto_ready_no_data(self):
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        self.loop.add_writer(7, transport._sendto_ready)
        transport._sendto_ready()
        self.assertFalse(self.sock.sendto.called)
        self.assertFalse(self.loop.writers)

    def test_sendto_ready_tryagain(self):
        self.sock.sendto.side_effect = BlockingIOError

        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport._buffer.extend([(b'data1', ()), (b'data2', ())])
        self.loop.add_writer(7, transport._sendto_ready)
        transport._sendto_ready()

        self.loop.assert_writer(7, transport._sendto_ready)
        self.assertEqual(
            [(b'data1', ()), (b'data2', ())],
            list(transport._buffer))

    def test_sendto_ready_exception(self):
        err = self.sock.sendto.side_effect = RuntimeError()

        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport._fatal_error = unittest.mock.Mock()
        transport._buffer.append((b'data', ()))
        transport._sendto_ready()

        transport._fatal_error.assert_called_with(
                                   err,
                                   'Fatal write error on datagram transport')

    def test_sendto_ready_error_received(self):
        self.sock.sendto.side_effect = ConnectionRefusedError

        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol)
        transport._fatal_error = unittest.mock.Mock()
        transport._buffer.append((b'data', ()))
        transport._sendto_ready()

        self.assertFalse(transport._fatal_error.called)

    def test_sendto_ready_error_received_connection(self):
        self.sock.send.side_effect = ConnectionRefusedError

        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol, ('0.0.0.0', 1))
        transport._fatal_error = unittest.mock.Mock()
        transport._buffer.append((b'data', ()))
        transport._sendto_ready()

        self.assertFalse(transport._fatal_error.called)
        self.assertTrue(self.protocol.error_received.called)

    @unittest.mock.patch('asyncio.base_events.logger.error')
    def test_fatal_error_connected(self, m_exc):
        transport = _SelectorDatagramTransport(
            self.loop, self.sock, self.protocol, ('0.0.0.0', 1))
        err = ConnectionRefusedError()
        transport._fatal_error(err)
        self.assertFalse(self.protocol.error_received.called)
        m_exc.assert_called_with(
            test_utils.MockPattern(
                'Fatal error on transport\nprotocol:.*\ntransport:.*'),
            exc_info=(ConnectionRefusedError, MOCK_ANY, MOCK_ANY))


if __name__ == '__main__':
    unittest.main()
