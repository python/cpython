"""Tests for selector_events.py"""

import errno
import selectors
import socket
import unittest
from unittest import mock
try:
    import ssl
except ImportError:
    ssl = None

import asyncio
from asyncio.selector_events import BaseSelectorEventLoop
from asyncio.selector_events import _SelectorTransport
from asyncio.selector_events import _SelectorSocketTransport
from asyncio.selector_events import _SelectorDatagramTransport
from test.test_asyncio import utils as test_utils


MOCK_ANY = mock.ANY


class TestBaseSelectorEventLoop(BaseSelectorEventLoop):

    def _make_self_pipe(self):
        self._ssock = mock.Mock()
        self._csock = mock.Mock()
        self._internal_fds += 1

    def _close_self_pipe(self):
        pass


def list_to_buffer(l=()):
    return bytearray().join(l)


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
        self.loop.add_reader._is_coroutine = False
        transport = self.loop._make_socket_transport(m, asyncio.Protocol())
        self.assertIsInstance(transport, _SelectorSocketTransport)

        # Calling repr() must not fail when the event loop is closed
        self.loop.close()
        repr(transport)

        close_transport(transport)

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_make_ssl_transport(self):
        m = mock.Mock()
        self.loop._add_reader = mock.Mock()
        self.loop._add_reader._is_coroutine = False
        self.loop._add_writer = mock.Mock()
        self.loop._remove_reader = mock.Mock()
        self.loop._remove_writer = mock.Mock()
        waiter = asyncio.Future(loop=self.loop)
        with test_utils.disable_logger():
            transport = self.loop._make_ssl_transport(
                m, asyncio.Protocol(), m, waiter)

            with self.assertRaisesRegex(RuntimeError,
                                        r'SSL transport.*not.*initialized'):
                transport.is_reading()

            # execute the handshake while the logger is disabled
            # to ignore SSL handshake failure
            test_utils.run_briefly(self.loop)

        self.assertTrue(transport.is_reading())
        transport.pause_reading()
        transport.pause_reading()
        self.assertFalse(transport.is_reading())
        transport.resume_reading()
        transport.resume_reading()
        self.assertTrue(transport.is_reading())

        # Sanity check
        class_name = transport.__class__.__name__
        self.assertIn("ssl", class_name.lower())
        self.assertIn("transport", class_name.lower())

        transport.close()
        # execute pending callbacks to close the socket transport
        test_utils.run_briefly(self.loop)

    @mock.patch('asyncio.selector_events.ssl', None)
    @mock.patch('asyncio.sslproto.ssl', None)
    def test_make_ssl_transport_without_ssl_error(self):
        m = mock.Mock()
        self.loop.add_reader = mock.Mock()
        self.loop.add_writer = mock.Mock()
        self.loop.remove_reader = mock.Mock()
        self.loop.remove_writer = mock.Mock()
        with self.assertRaises(RuntimeError):
            self.loop._make_ssl_transport(m, m, m, m)

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
        f = asyncio.Future(loop=self.loop)
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

    def test_sock_recv(self):
        sock = test_utils.mock_nonblocking_socket()
        self.loop._sock_recv = mock.Mock()

        f = self.loop.create_task(self.loop.sock_recv(sock, 1024))
        self.loop.run_until_complete(asyncio.sleep(0.01, loop=self.loop))

        self.assertEqual(self.loop._sock_recv.call_args[0][1:],
                         (None, sock, 1024))

        f.cancel()
        with self.assertRaises(asyncio.CancelledError):
            self.loop.run_until_complete(f)

    def test_sock_recv_reconnection(self):
        sock = mock.Mock()
        sock.fileno.return_value = 10
        sock.recv.side_effect = BlockingIOError
        sock.gettimeout.return_value = 0.0

        self.loop.add_reader = mock.Mock()
        self.loop.remove_reader = mock.Mock()
        fut = self.loop.create_task(
            self.loop.sock_recv(sock, 1024))

        self.loop.run_until_complete(asyncio.sleep(0.01, loop=self.loop))

        callback = self.loop.add_reader.call_args[0][1]
        params = self.loop.add_reader.call_args[0][2:]

        # emulate the old socket has closed, but the new one has
        # the same fileno, so callback is called with old (closed) socket
        sock.fileno.return_value = -1
        sock.recv.side_effect = OSError(9)
        callback(*params)

        self.loop.run_until_complete(asyncio.sleep(0.01, loop=self.loop))

        self.assertIsInstance(fut.exception(), OSError)
        self.assertEqual((10,), self.loop.remove_reader.call_args[0])

    def test__sock_recv_canceled_fut(self):
        sock = mock.Mock()

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop._sock_recv(f, None, sock, 1024)
        self.assertFalse(sock.recv.called)

    def test__sock_recv_unregister(self):
        sock = mock.Mock()
        sock.fileno.return_value = 10

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop.remove_reader = mock.Mock()
        self.loop._sock_recv(f, 10, sock, 1024)
        self.assertEqual((10,), self.loop.remove_reader.call_args[0])

    def test__sock_recv_tryagain(self):
        f = asyncio.Future(loop=self.loop)
        sock = mock.Mock()
        sock.fileno.return_value = 10
        sock.recv.side_effect = BlockingIOError

        self.loop.add_reader = mock.Mock()
        self.loop._sock_recv(f, None, sock, 1024)
        self.assertEqual((10, self.loop._sock_recv, f, 10, sock, 1024),
                         self.loop.add_reader.call_args[0])

    def test__sock_recv_exception(self):
        f = asyncio.Future(loop=self.loop)
        sock = mock.Mock()
        sock.fileno.return_value = 10
        err = sock.recv.side_effect = OSError()

        self.loop._sock_recv(f, None, sock, 1024)
        self.assertIs(err, f.exception())

    def test_sock_sendall(self):
        sock = test_utils.mock_nonblocking_socket()
        self.loop._sock_sendall = mock.Mock()

        f = self.loop.create_task(
            self.loop.sock_sendall(sock, b'data'))

        self.loop.run_until_complete(asyncio.sleep(0.01, loop=self.loop))

        self.assertEqual(
            (None, sock, b'data'),
            self.loop._sock_sendall.call_args[0][1:])

        f.cancel()
        with self.assertRaises(asyncio.CancelledError):
            self.loop.run_until_complete(f)

    def test_sock_sendall_nodata(self):
        sock = test_utils.mock_nonblocking_socket()
        self.loop._sock_sendall = mock.Mock()

        f = self.loop.create_task(self.loop.sock_sendall(sock, b''))
        self.loop.run_until_complete(asyncio.sleep(0, loop=self.loop))

        self.assertTrue(f.done())
        self.assertIsNone(f.result())
        self.assertFalse(self.loop._sock_sendall.called)

    def test_sock_sendall_reconnection(self):
        sock = mock.Mock()
        sock.fileno.return_value = 10
        sock.send.side_effect = BlockingIOError
        sock.gettimeout.return_value = 0.0

        self.loop.add_writer = mock.Mock()
        self.loop.remove_writer = mock.Mock()
        fut = self.loop.create_task(self.loop.sock_sendall(sock, b'data'))

        self.loop.run_until_complete(asyncio.sleep(0.01, loop=self.loop))

        callback = self.loop.add_writer.call_args[0][1]
        params = self.loop.add_writer.call_args[0][2:]

        # emulate the old socket has closed, but the new one has
        # the same fileno, so callback is called with old (closed) socket
        sock.fileno.return_value = -1
        sock.send.side_effect = OSError(9)
        callback(*params)

        self.loop.run_until_complete(asyncio.sleep(0.01, loop=self.loop))

        self.assertIsInstance(fut.exception(), OSError)
        self.assertEqual((10,), self.loop.remove_writer.call_args[0])

    def test__sock_sendall_canceled_fut(self):
        sock = mock.Mock()

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop._sock_sendall(f, None, sock, b'data')
        self.assertFalse(sock.send.called)

    def test__sock_sendall_unregister(self):
        sock = mock.Mock()
        sock.fileno.return_value = 10

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop.remove_writer = mock.Mock()
        self.loop._sock_sendall(f, 10, sock, b'data')
        self.assertEqual((10,), self.loop.remove_writer.call_args[0])

    def test__sock_sendall_tryagain(self):
        f = asyncio.Future(loop=self.loop)
        sock = mock.Mock()
        sock.fileno.return_value = 10
        sock.send.side_effect = BlockingIOError

        self.loop.add_writer = mock.Mock()
        self.loop._sock_sendall(f, None, sock, b'data')
        self.assertEqual(
            (10, self.loop._sock_sendall, f, 10, sock, b'data'),
            self.loop.add_writer.call_args[0])

    def test__sock_sendall_interrupted(self):
        f = asyncio.Future(loop=self.loop)
        sock = mock.Mock()
        sock.fileno.return_value = 10
        sock.send.side_effect = InterruptedError

        self.loop.add_writer = mock.Mock()
        self.loop._sock_sendall(f, None, sock, b'data')
        self.assertEqual(
            (10, self.loop._sock_sendall, f, 10, sock, b'data'),
            self.loop.add_writer.call_args[0])

    def test__sock_sendall_exception(self):
        f = asyncio.Future(loop=self.loop)
        sock = mock.Mock()
        sock.fileno.return_value = 10
        err = sock.send.side_effect = OSError()

        self.loop._sock_sendall(f, None, sock, b'data')
        self.assertIs(f.exception(), err)

    def test__sock_sendall(self):
        sock = mock.Mock()

        f = asyncio.Future(loop=self.loop)
        sock.fileno.return_value = 10
        sock.send.return_value = 4

        self.loop._sock_sendall(f, None, sock, b'data')
        self.assertTrue(f.done())
        self.assertIsNone(f.result())

    def test__sock_sendall_partial(self):
        sock = mock.Mock()

        f = asyncio.Future(loop=self.loop)
        sock.fileno.return_value = 10
        sock.send.return_value = 2

        self.loop.add_writer = mock.Mock()
        self.loop._sock_sendall(f, None, sock, b'data')
        self.assertFalse(f.done())
        self.assertEqual(
            (10, self.loop._sock_sendall, f, 10, sock, b'ta'),
            self.loop.add_writer.call_args[0])

    def test__sock_sendall_none(self):
        sock = mock.Mock()

        f = asyncio.Future(loop=self.loop)
        sock.fileno.return_value = 10
        sock.send.return_value = 0

        self.loop.add_writer = mock.Mock()
        self.loop._sock_sendall(f, None, sock, b'data')
        self.assertFalse(f.done())
        self.assertEqual(
            (10, self.loop._sock_sendall, f, 10, sock, b'data'),
            self.loop.add_writer.call_args[0])

    def test_sock_connect_timeout(self):
        # asyncio issue #205: sock_connect() must unregister the socket on
        # timeout error

        # prepare mocks
        self.loop.add_writer = mock.Mock()
        self.loop.remove_writer = mock.Mock()
        sock = test_utils.mock_nonblocking_socket()
        sock.connect.side_effect = BlockingIOError

        # first call to sock_connect() registers the socket
        fut = self.loop.create_task(
            self.loop.sock_connect(sock, ('127.0.0.1', 80)))
        self.loop._run_once()
        self.assertTrue(sock.connect.called)
        self.assertTrue(self.loop.add_writer.called)

        # on timeout, the socket must be unregistered
        sock.connect.reset_mock()
        fut.cancel()
        with self.assertRaises(asyncio.CancelledError):
            self.loop.run_until_complete(fut)
        self.assertTrue(self.loop.remove_writer.called)

    @mock.patch('socket.getaddrinfo')
    def test_sock_connect_resolve_using_socket_params(self, m_gai):
        addr = ('need-resolution.com', 8080)
        sock = test_utils.mock_nonblocking_socket()

        m_gai.side_effect = \
            lambda *args: [(None, None, None, None, ('127.0.0.1', 0))]

        con = self.loop.create_task(self.loop.sock_connect(sock, addr))
        self.loop.run_until_complete(con)
        m_gai.assert_called_with(
            addr[0], addr[1], sock.family, sock.type, sock.proto, 0)

        self.loop.run_until_complete(con)
        sock.connect.assert_called_with(('127.0.0.1', 0))

    def test__sock_connect(self):
        f = asyncio.Future(loop=self.loop)

        sock = mock.Mock()
        sock.fileno.return_value = 10

        resolved = self.loop.create_future()
        resolved.set_result([(socket.AF_INET, socket.SOCK_STREAM,
                              socket.IPPROTO_TCP, '', ('127.0.0.1', 8080))])
        self.loop._sock_connect(f, sock, resolved)
        self.assertTrue(f.done())
        self.assertIsNone(f.result())
        self.assertTrue(sock.connect.called)

    def test__sock_connect_cb_cancelled_fut(self):
        sock = mock.Mock()
        self.loop.remove_writer = mock.Mock()

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop._sock_connect_cb(f, sock, ('127.0.0.1', 8080))
        self.assertFalse(sock.getsockopt.called)

    def test__sock_connect_writer(self):
        # check that the fd is registered and then unregistered
        self.loop._process_events = mock.Mock()
        self.loop.add_writer = mock.Mock()
        self.loop.remove_writer = mock.Mock()

        sock = mock.Mock()
        sock.fileno.return_value = 10
        sock.connect.side_effect = BlockingIOError
        sock.getsockopt.return_value = 0
        address = ('127.0.0.1', 8080)
        resolved = self.loop.create_future()
        resolved.set_result([(socket.AF_INET, socket.SOCK_STREAM,
                              socket.IPPROTO_TCP, '', address)])

        f = asyncio.Future(loop=self.loop)
        self.loop._sock_connect(f, sock, resolved)
        self.loop._run_once()
        self.assertTrue(self.loop.add_writer.called)
        self.assertEqual(10, self.loop.add_writer.call_args[0][0])

        self.loop._sock_connect_cb(f, sock, address)
        # need to run the event loop to execute _sock_connect_done() callback
        self.loop.run_until_complete(f)
        self.assertEqual((10,), self.loop.remove_writer.call_args[0])

    def test__sock_connect_cb_tryagain(self):
        f = asyncio.Future(loop=self.loop)
        sock = mock.Mock()
        sock.fileno.return_value = 10
        sock.getsockopt.return_value = errno.EAGAIN

        # check that the exception is handled
        self.loop._sock_connect_cb(f, sock, ('127.0.0.1', 8080))

    def test__sock_connect_cb_exception(self):
        f = asyncio.Future(loop=self.loop)
        sock = mock.Mock()
        sock.fileno.return_value = 10
        sock.getsockopt.return_value = errno.ENOTCONN

        self.loop.remove_writer = mock.Mock()
        self.loop._sock_connect_cb(f, sock, ('127.0.0.1', 8080))
        self.assertIsInstance(f.exception(), OSError)

    def test_sock_accept(self):
        sock = test_utils.mock_nonblocking_socket()
        self.loop._sock_accept = mock.Mock()

        f = self.loop.create_task(self.loop.sock_accept(sock))
        self.loop.run_until_complete(asyncio.sleep(0.01, loop=self.loop))

        self.assertFalse(self.loop._sock_accept.call_args[0][1])
        self.assertIs(self.loop._sock_accept.call_args[0][2], sock)

        f.cancel()
        with self.assertRaises(asyncio.CancelledError):
            self.loop.run_until_complete(f)

    def test__sock_accept(self):
        f = asyncio.Future(loop=self.loop)

        conn = mock.Mock()

        sock = mock.Mock()
        sock.fileno.return_value = 10
        sock.accept.return_value = conn, ('127.0.0.1', 1000)

        self.loop._sock_accept(f, False, sock)
        self.assertTrue(f.done())
        self.assertEqual((conn, ('127.0.0.1', 1000)), f.result())
        self.assertEqual((False,), conn.setblocking.call_args[0])

    def test__sock_accept_canceled_fut(self):
        sock = mock.Mock()

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop._sock_accept(f, False, sock)
        self.assertFalse(sock.accept.called)

    def test__sock_accept_unregister(self):
        sock = mock.Mock()
        sock.fileno.return_value = 10

        f = asyncio.Future(loop=self.loop)
        f.cancel()

        self.loop.remove_reader = mock.Mock()
        self.loop._sock_accept(f, True, sock)
        self.assertEqual((10,), self.loop.remove_reader.call_args[0])

    def test__sock_accept_tryagain(self):
        f = asyncio.Future(loop=self.loop)
        sock = mock.Mock()
        sock.fileno.return_value = 10
        sock.accept.side_effect = BlockingIOError

        self.loop.add_reader = mock.Mock()
        self.loop._sock_accept(f, False, sock)
        self.assertEqual(
            (10, self.loop._sock_accept, f, True, sock),
            self.loop.add_reader.call_args[0])

    def test__sock_accept_exception(self):
        f = asyncio.Future(loop=self.loop)
        sock = mock.Mock()
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
        reader = mock.Mock()
        writer = mock.Mock()
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
        writer = mock.Mock()
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
        reader = mock.Mock()
        writer = mock.Mock()
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
        reader = mock.Mock()
        writer = mock.Mock()
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
        reader = mock.Mock()
        writer = mock.Mock()
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
        # warnings related to un-awaited coroutines.
        mock_obj = mock.patch.object
        with mock_obj(self.loop, '_accept_connection2') as accept2_mock:
            accept2_mock.return_value = None
            with mock_obj(self.loop, 'create_task') as task_mock:
                task_mock.return_value = None
                self.loop._accept_connection(
                    mock.Mock(), sock, backlog=backlog)
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

    def socket_transport(self, waiter=None):
        transport = _SelectorSocketTransport(self.loop, self.sock,
                                             self.protocol, waiter=waiter)
        self.addCleanup(close_transport, transport)
        return transport

    def test_ctor(self):
        waiter = asyncio.Future(loop=self.loop)
        tr = self.socket_transport(waiter=waiter)
        self.loop.run_until_complete(waiter)

        self.loop.assert_reader(7, tr._read_ready)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_made.assert_called_with(tr)

    def test_ctor_with_waiter(self):
        waiter = asyncio.Future(loop=self.loop)
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
        transport._buffer.extend(b'data')
        transport.write(b'')
        self.assertFalse(self.sock.send.called)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_buffer(self):
        transport = self.socket_transport()
        transport._buffer.extend(b'data1')
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
        transport._buffer.extend(data)
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()
        self.assertTrue(self.sock.send.called)
        self.assertFalse(self.loop.writers)

    def test_write_ready_closing(self):
        data = b'data'
        self.sock.send.return_value = len(data)

        transport = self.socket_transport()
        transport._closing = True
        transport._buffer.extend(data)
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()
        self.assertTrue(self.sock.send.called)
        self.assertFalse(self.loop.writers)
        self.sock.close.assert_called_with()
        self.protocol.connection_lost.assert_called_with(None)

    def test_write_ready_no_data(self):
        transport = self.socket_transport()
        # This is an internal error.
        self.assertRaises(AssertionError, transport._write_ready)

    def test_write_ready_partial(self):
        data = b'data'
        self.sock.send.return_value = 2

        transport = self.socket_transport()
        transport._buffer.extend(data)
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()
        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'ta']), transport._buffer)

    def test_write_ready_partial_none(self):
        data = b'data'
        self.sock.send.return_value = 0

        transport = self.socket_transport()
        transport._buffer.extend(data)
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()
        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'data']), transport._buffer)

    def test_write_ready_tryagain(self):
        self.sock.send.side_effect = BlockingIOError

        transport = self.socket_transport()
        transport._buffer = list_to_buffer([b'data1', b'data2'])
        self.loop._add_writer(7, transport._write_ready)
        transport._write_ready()

        self.loop.assert_writer(7, transport._write_ready)
        self.assertEqual(list_to_buffer([b'data1data2']), transport._buffer)

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
        waiter = asyncio.Future(loop=self.loop)
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
        transport._buffer.append((b'data', ('0.0.0.0', 12345)))
        transport.sendto(b'', ())
        self.assertFalse(self.sock.sendto.called)
        self.assertEqual(
            [(b'data', ('0.0.0.0', 12345))], list(transport._buffer))

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
