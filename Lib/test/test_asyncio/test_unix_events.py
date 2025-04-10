"""Tests for unix_events.py."""

import contextlib
import errno
import io
import multiprocessing
from multiprocessing.util import _cleanup_tests as multiprocessing_cleanup_tests
import os
import signal
import socket
import stat
import sys
import time
import unittest
from unittest import mock

from test import support
from test.support import os_helper
from test.support import socket_helper
from test.support import wait_process
from test.support import hashlib_helper

if sys.platform == 'win32':
    raise unittest.SkipTest('UNIX only')


import asyncio
from asyncio import unix_events
from test.test_asyncio import utils as test_utils


def tearDownModule():
    asyncio._set_event_loop_policy(None)


MOCK_ANY = mock.ANY


def EXITCODE(exitcode):
    return 32768 + exitcode


def SIGNAL(signum):
    if not 1 <= signum <= 68:
        raise AssertionError(f'invalid signum {signum}')
    return 32768 - signum


def close_pipe_transport(transport):
    # Don't call transport.close() because the event loop and the selector
    # are mocked
    if transport._pipe is None:
        return
    transport._pipe.close()
    transport._pipe = None


@unittest.skipUnless(signal, 'Signals are not supported')
class SelectorEventLoopSignalTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = asyncio.SelectorEventLoop()
        self.set_event_loop(self.loop)

    def test_check_signal(self):
        self.assertRaises(
            TypeError, self.loop._check_signal, '1')
        self.assertRaises(
            ValueError, self.loop._check_signal, signal.NSIG + 1)

    def test_handle_signal_no_handler(self):
        self.loop._handle_signal(signal.NSIG + 1)

    def test_handle_signal_cancelled_handler(self):
        h = asyncio.Handle(mock.Mock(), (),
                           loop=mock.Mock())
        h.cancel()
        self.loop._signal_handlers[signal.NSIG + 1] = h
        self.loop.remove_signal_handler = mock.Mock()
        self.loop._handle_signal(signal.NSIG + 1)
        self.loop.remove_signal_handler.assert_called_with(signal.NSIG + 1)

    @mock.patch('asyncio.unix_events.signal')
    def test_add_signal_handler_setup_error(self, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.valid_signals = signal.valid_signals
        m_signal.set_wakeup_fd.side_effect = ValueError

        self.assertRaises(
            RuntimeError,
            self.loop.add_signal_handler,
            signal.SIGINT, lambda: True)

    @mock.patch('asyncio.unix_events.signal')
    def test_add_signal_handler_coroutine_error(self, m_signal):
        m_signal.NSIG = signal.NSIG

        async def simple_coroutine():
            pass

        # callback must not be a coroutine function
        coro_func = simple_coroutine
        coro_obj = coro_func()
        self.addCleanup(coro_obj.close)
        for func in (coro_func, coro_obj):
            self.assertRaisesRegex(
                TypeError, 'coroutines cannot be used with add_signal_handler',
                self.loop.add_signal_handler,
                signal.SIGINT, func)

    @mock.patch('asyncio.unix_events.signal')
    def test_add_signal_handler(self, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.valid_signals = signal.valid_signals

        cb = lambda: True
        self.loop.add_signal_handler(signal.SIGHUP, cb)
        h = self.loop._signal_handlers.get(signal.SIGHUP)
        self.assertIsInstance(h, asyncio.Handle)
        self.assertEqual(h._callback, cb)

    @mock.patch('asyncio.unix_events.signal')
    def test_add_signal_handler_install_error(self, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.valid_signals = signal.valid_signals

        def set_wakeup_fd(fd):
            if fd == -1:
                raise ValueError()
        m_signal.set_wakeup_fd = set_wakeup_fd

        class Err(OSError):
            errno = errno.EFAULT
        m_signal.signal.side_effect = Err

        self.assertRaises(
            Err,
            self.loop.add_signal_handler,
            signal.SIGINT, lambda: True)

    @mock.patch('asyncio.unix_events.signal')
    @mock.patch('asyncio.base_events.logger')
    def test_add_signal_handler_install_error2(self, m_logging, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.valid_signals = signal.valid_signals

        class Err(OSError):
            errno = errno.EINVAL
        m_signal.signal.side_effect = Err

        self.loop._signal_handlers[signal.SIGHUP] = lambda: True
        self.assertRaises(
            RuntimeError,
            self.loop.add_signal_handler,
            signal.SIGINT, lambda: True)
        self.assertFalse(m_logging.info.called)
        self.assertEqual(1, m_signal.set_wakeup_fd.call_count)

    @mock.patch('asyncio.unix_events.signal')
    @mock.patch('asyncio.base_events.logger')
    def test_add_signal_handler_install_error3(self, m_logging, m_signal):
        class Err(OSError):
            errno = errno.EINVAL
        m_signal.signal.side_effect = Err
        m_signal.NSIG = signal.NSIG
        m_signal.valid_signals = signal.valid_signals

        self.assertRaises(
            RuntimeError,
            self.loop.add_signal_handler,
            signal.SIGINT, lambda: True)
        self.assertFalse(m_logging.info.called)
        self.assertEqual(2, m_signal.set_wakeup_fd.call_count)

    @mock.patch('asyncio.unix_events.signal')
    def test_remove_signal_handler(self, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.valid_signals = signal.valid_signals

        self.loop.add_signal_handler(signal.SIGHUP, lambda: True)

        self.assertTrue(
            self.loop.remove_signal_handler(signal.SIGHUP))
        self.assertTrue(m_signal.set_wakeup_fd.called)
        self.assertTrue(m_signal.signal.called)
        self.assertEqual(
            (signal.SIGHUP, m_signal.SIG_DFL), m_signal.signal.call_args[0])

    @mock.patch('asyncio.unix_events.signal')
    def test_remove_signal_handler_2(self, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.SIGINT = signal.SIGINT
        m_signal.valid_signals = signal.valid_signals

        self.loop.add_signal_handler(signal.SIGINT, lambda: True)
        self.loop._signal_handlers[signal.SIGHUP] = object()
        m_signal.set_wakeup_fd.reset_mock()

        self.assertTrue(
            self.loop.remove_signal_handler(signal.SIGINT))
        self.assertFalse(m_signal.set_wakeup_fd.called)
        self.assertTrue(m_signal.signal.called)
        self.assertEqual(
            (signal.SIGINT, m_signal.default_int_handler),
            m_signal.signal.call_args[0])

    @mock.patch('asyncio.unix_events.signal')
    @mock.patch('asyncio.base_events.logger')
    def test_remove_signal_handler_cleanup_error(self, m_logging, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.valid_signals = signal.valid_signals
        self.loop.add_signal_handler(signal.SIGHUP, lambda: True)

        m_signal.set_wakeup_fd.side_effect = ValueError

        self.loop.remove_signal_handler(signal.SIGHUP)
        self.assertTrue(m_logging.info)

    @mock.patch('asyncio.unix_events.signal')
    def test_remove_signal_handler_error(self, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.valid_signals = signal.valid_signals
        self.loop.add_signal_handler(signal.SIGHUP, lambda: True)

        m_signal.signal.side_effect = OSError

        self.assertRaises(
            OSError, self.loop.remove_signal_handler, signal.SIGHUP)

    @mock.patch('asyncio.unix_events.signal')
    def test_remove_signal_handler_error2(self, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.valid_signals = signal.valid_signals
        self.loop.add_signal_handler(signal.SIGHUP, lambda: True)

        class Err(OSError):
            errno = errno.EINVAL
        m_signal.signal.side_effect = Err

        self.assertRaises(
            RuntimeError, self.loop.remove_signal_handler, signal.SIGHUP)

    @mock.patch('asyncio.unix_events.signal')
    def test_close(self, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.valid_signals = signal.valid_signals

        self.loop.add_signal_handler(signal.SIGHUP, lambda: True)
        self.loop.add_signal_handler(signal.SIGCHLD, lambda: True)

        self.assertEqual(len(self.loop._signal_handlers), 2)

        m_signal.set_wakeup_fd.reset_mock()

        self.loop.close()

        self.assertEqual(len(self.loop._signal_handlers), 0)
        m_signal.set_wakeup_fd.assert_called_once_with(-1)

    @mock.patch('asyncio.unix_events.sys')
    @mock.patch('asyncio.unix_events.signal')
    def test_close_on_finalizing(self, m_signal, m_sys):
        m_signal.NSIG = signal.NSIG
        m_signal.valid_signals = signal.valid_signals
        self.loop.add_signal_handler(signal.SIGHUP, lambda: True)

        self.assertEqual(len(self.loop._signal_handlers), 1)
        m_sys.is_finalizing.return_value = True
        m_signal.signal.reset_mock()

        with self.assertWarnsRegex(ResourceWarning,
                                   "skipping signal handlers removal"):
            self.loop.close()

        self.assertEqual(len(self.loop._signal_handlers), 0)
        self.assertFalse(m_signal.signal.called)


@unittest.skipUnless(hasattr(socket, 'AF_UNIX'),
                     'UNIX Sockets are not supported')
class SelectorEventLoopUnixSocketTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = asyncio.SelectorEventLoop()
        self.set_event_loop(self.loop)

    @socket_helper.skip_unless_bind_unix_socket
    def test_create_unix_server_existing_path_sock(self):
        with test_utils.unix_socket_path() as path:
            sock = socket.socket(socket.AF_UNIX)
            sock.bind(path)
            sock.listen(1)
            sock.close()

            coro = self.loop.create_unix_server(lambda: None, path)
            srv = self.loop.run_until_complete(coro)
            srv.close()
            self.loop.run_until_complete(srv.wait_closed())

    @socket_helper.skip_unless_bind_unix_socket
    def test_create_unix_server_pathlike(self):
        with test_utils.unix_socket_path() as path:
            path = os_helper.FakePath(path)
            srv_coro = self.loop.create_unix_server(lambda: None, path)
            srv = self.loop.run_until_complete(srv_coro)
            srv.close()
            self.loop.run_until_complete(srv.wait_closed())

    def test_create_unix_connection_pathlike(self):
        with test_utils.unix_socket_path() as path:
            path = os_helper.FakePath(path)
            coro = self.loop.create_unix_connection(lambda: None, path)
            with self.assertRaises(FileNotFoundError):
                # If path-like object weren't supported, the exception would be
                # different.
                self.loop.run_until_complete(coro)

    def test_create_unix_server_existing_path_nonsock(self):
        path = test_utils.gen_unix_socket_path()
        self.addCleanup(os_helper.unlink, path)
        # create the file
        open(path, "wb").close()

        coro = self.loop.create_unix_server(lambda: None, path)
        with self.assertRaisesRegex(OSError,
                                    'Address.*is already in use'):
            self.loop.run_until_complete(coro)

    def test_create_unix_server_ssl_bool(self):
        coro = self.loop.create_unix_server(lambda: None, path='spam',
                                            ssl=True)
        with self.assertRaisesRegex(TypeError,
                                    'ssl argument must be an SSLContext'):
            self.loop.run_until_complete(coro)

    def test_create_unix_server_nopath_nosock(self):
        coro = self.loop.create_unix_server(lambda: None, path=None)
        with self.assertRaisesRegex(ValueError,
                                    'path was not specified, and no sock'):
            self.loop.run_until_complete(coro)

    def test_create_unix_server_path_inetsock(self):
        sock = socket.socket()
        with sock:
            coro = self.loop.create_unix_server(lambda: None, path=None,
                                                sock=sock)
            with self.assertRaisesRegex(ValueError,
                                        'A UNIX Domain Stream.*was expected'):
                self.loop.run_until_complete(coro)

    def test_create_unix_server_path_dgram(self):
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
        with sock:
            coro = self.loop.create_unix_server(lambda: None, path=None,
                                                sock=sock)
            with self.assertRaisesRegex(ValueError,
                                        'A UNIX Domain Stream.*was expected'):
                self.loop.run_until_complete(coro)

    @unittest.skipUnless(hasattr(socket, 'SOCK_NONBLOCK'),
                         'no socket.SOCK_NONBLOCK (linux only)')
    @socket_helper.skip_unless_bind_unix_socket
    def test_create_unix_server_path_stream_bittype(self):
        fn = test_utils.gen_unix_socket_path()
        self.addCleanup(os_helper.unlink, fn)

        sock = socket.socket(socket.AF_UNIX,
                             socket.SOCK_STREAM | socket.SOCK_NONBLOCK)
        with sock:
            sock.bind(fn)
            coro = self.loop.create_unix_server(lambda: None, path=None,
                                                sock=sock)
            srv = self.loop.run_until_complete(coro)
            srv.close()
            self.loop.run_until_complete(srv.wait_closed())

    def test_create_unix_server_ssl_timeout_with_plain_sock(self):
        coro = self.loop.create_unix_server(lambda: None, path='spam',
                                            ssl_handshake_timeout=1)
        with self.assertRaisesRegex(
                ValueError,
                'ssl_handshake_timeout is only meaningful with ssl'):
            self.loop.run_until_complete(coro)

    def test_create_unix_connection_path_inetsock(self):
        sock = socket.socket()
        with sock:
            coro = self.loop.create_unix_connection(lambda: None,
                                                    sock=sock)
            with self.assertRaisesRegex(ValueError,
                                        'A UNIX Domain Stream.*was expected'):
                self.loop.run_until_complete(coro)

    @mock.patch('asyncio.unix_events.socket')
    def test_create_unix_server_bind_error(self, m_socket):
        # Ensure that the socket is closed on any bind error
        sock = mock.Mock()
        m_socket.socket.return_value = sock

        sock.bind.side_effect = OSError
        coro = self.loop.create_unix_server(lambda: None, path="/test")
        with self.assertRaises(OSError):
            self.loop.run_until_complete(coro)
        self.assertTrue(sock.close.called)

        sock.bind.side_effect = MemoryError
        coro = self.loop.create_unix_server(lambda: None, path="/test")
        with self.assertRaises(MemoryError):
            self.loop.run_until_complete(coro)
        self.assertTrue(sock.close.called)

    def test_create_unix_connection_path_sock(self):
        coro = self.loop.create_unix_connection(
            lambda: None, os.devnull, sock=object())
        with self.assertRaisesRegex(ValueError, 'path and sock can not be'):
            self.loop.run_until_complete(coro)

    def test_create_unix_connection_nopath_nosock(self):
        coro = self.loop.create_unix_connection(
            lambda: None, None)
        with self.assertRaisesRegex(ValueError,
                                    'no path and sock were specified'):
            self.loop.run_until_complete(coro)

    def test_create_unix_connection_nossl_serverhost(self):
        coro = self.loop.create_unix_connection(
            lambda: None, os.devnull, server_hostname='spam')
        with self.assertRaisesRegex(ValueError,
                                    'server_hostname is only meaningful'):
            self.loop.run_until_complete(coro)

    def test_create_unix_connection_ssl_noserverhost(self):
        coro = self.loop.create_unix_connection(
            lambda: None, os.devnull, ssl=True)

        with self.assertRaisesRegex(
            ValueError, 'you have to pass server_hostname when using ssl'):

            self.loop.run_until_complete(coro)

    def test_create_unix_connection_ssl_timeout_with_plain_sock(self):
        coro = self.loop.create_unix_connection(lambda: None, path='spam',
                                            ssl_handshake_timeout=1)
        with self.assertRaisesRegex(
                ValueError,
                'ssl_handshake_timeout is only meaningful with ssl'):
            self.loop.run_until_complete(coro)


@unittest.skipUnless(hasattr(os, 'sendfile'),
                     'sendfile is not supported')
class SelectorEventLoopUnixSockSendfileTests(test_utils.TestCase):
    DATA = b"12345abcde" * 16 * 1024  # 160 KiB

    class MyProto(asyncio.Protocol):

        def __init__(self, loop):
            self.started = False
            self.closed = False
            self.data = bytearray()
            self.fut = loop.create_future()
            self.transport = None
            self._ready = loop.create_future()

        def connection_made(self, transport):
            self.started = True
            self.transport = transport
            self._ready.set_result(None)

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
        self.loop = asyncio.new_event_loop()
        self.set_event_loop(self.loop)
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
        srv_sock.bind((socket_helper.HOST, port))
        server = self.run_loop(self.loop.create_server(
            lambda: proto, sock=srv_sock))
        self.run_loop(self.loop.sock_connect(sock, (socket_helper.HOST, port)))
        self.run_loop(proto._ready)

        def cleanup():
            proto.transport.close()
            self.run_loop(proto.wait_closed())

            server.close()
            self.run_loop(server.wait_closed())

        self.addCleanup(cleanup)

        return sock, proto

    def test_sock_sendfile_not_available(self):
        sock, proto = self.prepare()
        with mock.patch('asyncio.unix_events.os', spec=[]):
            with self.assertRaisesRegex(asyncio.SendfileNotAvailableError,
                                        "os[.]sendfile[(][)] is not available"):
                self.run_loop(self.loop._sock_sendfile_native(sock, self.file,
                                                              0, None))
        self.assertEqual(self.file.tell(), 0)

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

    def test_sock_sendfile_cancel1(self):
        sock, proto = self.prepare()

        fut = self.loop.create_future()
        fileno = self.file.fileno()
        self.loop._sock_sendfile_native_impl(fut, None, sock, fileno,
                                             0, None, len(self.DATA), 0)
        fut.cancel()
        with contextlib.suppress(asyncio.CancelledError):
            self.run_loop(fut)
        with self.assertRaises(KeyError):
            self.loop._selector.get_key(sock)

    def test_sock_sendfile_cancel2(self):
        sock, proto = self.prepare()

        fut = self.loop.create_future()
        fileno = self.file.fileno()
        self.loop._sock_sendfile_native_impl(fut, None, sock, fileno,
                                             0, None, len(self.DATA), 0)
        fut.cancel()
        self.loop._sock_sendfile_native_impl(fut, sock.fileno(), sock, fileno,
                                             0, None, len(self.DATA), 0)
        with self.assertRaises(KeyError):
            self.loop._selector.get_key(sock)

    def test_sock_sendfile_blocking_error(self):
        sock, proto = self.prepare()

        fileno = self.file.fileno()
        fut = mock.Mock()
        fut.cancelled.return_value = False
        with mock.patch('os.sendfile', side_effect=BlockingIOError()):
            self.loop._sock_sendfile_native_impl(fut, None, sock, fileno,
                                                 0, None, len(self.DATA), 0)
        key = self.loop._selector.get_key(sock)
        self.assertIsNotNone(key)
        fut.add_done_callback.assert_called_once_with(mock.ANY)

    def test_sock_sendfile_os_error_first_call(self):
        sock, proto = self.prepare()

        fileno = self.file.fileno()
        fut = self.loop.create_future()
        with mock.patch('os.sendfile', side_effect=OSError()):
            self.loop._sock_sendfile_native_impl(fut, None, sock, fileno,
                                                 0, None, len(self.DATA), 0)
        with self.assertRaises(KeyError):
            self.loop._selector.get_key(sock)
        exc = fut.exception()
        self.assertIsInstance(exc, asyncio.SendfileNotAvailableError)
        self.assertEqual(0, self.file.tell())

    def test_sock_sendfile_os_error_next_call(self):
        sock, proto = self.prepare()

        fileno = self.file.fileno()
        fut = self.loop.create_future()
        err = OSError()
        with mock.patch('os.sendfile', side_effect=err):
            self.loop._sock_sendfile_native_impl(fut, sock.fileno(),
                                                 sock, fileno,
                                                 1000, None, len(self.DATA),
                                                 1000)
        with self.assertRaises(KeyError):
            self.loop._selector.get_key(sock)
        exc = fut.exception()
        self.assertIs(exc, err)
        self.assertEqual(1000, self.file.tell())

    def test_sock_sendfile_exception(self):
        sock, proto = self.prepare()

        fileno = self.file.fileno()
        fut = self.loop.create_future()
        err = asyncio.SendfileNotAvailableError()
        with mock.patch('os.sendfile', side_effect=err):
            self.loop._sock_sendfile_native_impl(fut, sock.fileno(),
                                                 sock, fileno,
                                                 1000, None, len(self.DATA),
                                                 1000)
        with self.assertRaises(KeyError):
            self.loop._selector.get_key(sock)
        exc = fut.exception()
        self.assertIs(exc, err)
        self.assertEqual(1000, self.file.tell())


class UnixReadPipeTransportTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = self.new_test_loop()
        self.protocol = test_utils.make_test_protocol(asyncio.Protocol)
        self.pipe = mock.Mock(spec_set=io.RawIOBase)
        self.pipe.fileno.return_value = 5

        blocking_patcher = mock.patch('os.set_blocking')
        blocking_patcher.start()
        self.addCleanup(blocking_patcher.stop)

        fstat_patcher = mock.patch('os.fstat')
        m_fstat = fstat_patcher.start()
        st = mock.Mock()
        st.st_mode = stat.S_IFIFO
        m_fstat.return_value = st
        self.addCleanup(fstat_patcher.stop)

    def read_pipe_transport(self, waiter=None):
        transport = unix_events._UnixReadPipeTransport(self.loop, self.pipe,
                                                       self.protocol,
                                                       waiter=waiter)
        self.addCleanup(close_pipe_transport, transport)
        return transport

    def test_ctor(self):
        waiter = self.loop.create_future()
        tr = self.read_pipe_transport(waiter=waiter)
        self.loop.run_until_complete(waiter)

        self.protocol.connection_made.assert_called_with(tr)
        self.loop.assert_reader(5, tr._read_ready)
        self.assertIsNone(waiter.result())

    @mock.patch('os.read')
    def test__read_ready(self, m_read):
        tr = self.read_pipe_transport()
        m_read.return_value = b'data'
        tr._read_ready()

        m_read.assert_called_with(5, tr.max_size)
        self.protocol.data_received.assert_called_with(b'data')

    @mock.patch('os.read')
    def test__read_ready_eof(self, m_read):
        tr = self.read_pipe_transport()
        m_read.return_value = b''
        tr._read_ready()

        m_read.assert_called_with(5, tr.max_size)
        self.assertFalse(self.loop.readers)
        test_utils.run_briefly(self.loop)
        self.protocol.eof_received.assert_called_with()
        self.protocol.connection_lost.assert_called_with(None)

    @mock.patch('os.read')
    def test__read_ready_blocked(self, m_read):
        tr = self.read_pipe_transport()
        m_read.side_effect = BlockingIOError
        tr._read_ready()

        m_read.assert_called_with(5, tr.max_size)
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.data_received.called)

    @mock.patch('asyncio.log.logger.error')
    @mock.patch('os.read')
    def test__read_ready_error(self, m_read, m_logexc):
        tr = self.read_pipe_transport()
        err = OSError()
        m_read.side_effect = err
        tr._close = mock.Mock()
        tr._read_ready()

        m_read.assert_called_with(5, tr.max_size)
        tr._close.assert_called_with(err)
        m_logexc.assert_called_with(
            test_utils.MockPattern(
                'Fatal read error on pipe transport'
                '\nprotocol:.*\ntransport:.*'),
            exc_info=(OSError, MOCK_ANY, MOCK_ANY))

    @mock.patch('os.read')
    def test_pause_reading(self, m_read):
        tr = self.read_pipe_transport()
        m = mock.Mock()
        self.loop.add_reader(5, m)
        tr.pause_reading()
        self.assertFalse(self.loop.readers)

    @mock.patch('os.read')
    def test_resume_reading(self, m_read):
        tr = self.read_pipe_transport()
        tr.pause_reading()
        tr.resume_reading()
        self.loop.assert_reader(5, tr._read_ready)

    @mock.patch('os.read')
    def test_close(self, m_read):
        tr = self.read_pipe_transport()
        tr._close = mock.Mock()
        tr.close()
        tr._close.assert_called_with(None)

    @mock.patch('os.read')
    def test_close_already_closing(self, m_read):
        tr = self.read_pipe_transport()
        tr._closing = True
        tr._close = mock.Mock()
        tr.close()
        self.assertFalse(tr._close.called)

    @mock.patch('os.read')
    def test__close(self, m_read):
        tr = self.read_pipe_transport()
        err = object()
        tr._close(err)
        self.assertTrue(tr.is_closing())
        self.assertFalse(self.loop.readers)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(err)

    def test__call_connection_lost(self):
        tr = self.read_pipe_transport()
        self.assertIsNotNone(tr._protocol)
        self.assertIsNotNone(tr._loop)

        err = None
        tr._call_connection_lost(err)
        self.protocol.connection_lost.assert_called_with(err)
        self.pipe.close.assert_called_with()

        self.assertIsNone(tr._protocol)
        self.assertIsNone(tr._loop)

    def test__call_connection_lost_with_err(self):
        tr = self.read_pipe_transport()
        self.assertIsNotNone(tr._protocol)
        self.assertIsNotNone(tr._loop)

        err = OSError()
        tr._call_connection_lost(err)
        self.protocol.connection_lost.assert_called_with(err)
        self.pipe.close.assert_called_with()

        self.assertIsNone(tr._protocol)
        self.assertIsNone(tr._loop)

    def test_pause_reading_on_closed_pipe(self):
        tr = self.read_pipe_transport()
        tr.close()
        test_utils.run_briefly(self.loop)
        self.assertIsNone(tr._loop)
        tr.pause_reading()

    def test_pause_reading_on_paused_pipe(self):
        tr = self.read_pipe_transport()
        tr.pause_reading()
        # the second call should do nothing
        tr.pause_reading()

    def test_resume_reading_on_closed_pipe(self):
        tr = self.read_pipe_transport()
        tr.close()
        test_utils.run_briefly(self.loop)
        self.assertIsNone(tr._loop)
        tr.resume_reading()

    def test_resume_reading_on_paused_pipe(self):
        tr = self.read_pipe_transport()
        # the pipe is not paused
        # resuming should do nothing
        tr.resume_reading()


class UnixWritePipeTransportTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = self.new_test_loop()
        self.protocol = test_utils.make_test_protocol(asyncio.BaseProtocol)
        self.pipe = mock.Mock(spec_set=io.RawIOBase)
        self.pipe.fileno.return_value = 5

        blocking_patcher = mock.patch('os.set_blocking')
        blocking_patcher.start()
        self.addCleanup(blocking_patcher.stop)

        fstat_patcher = mock.patch('os.fstat')
        m_fstat = fstat_patcher.start()
        st = mock.Mock()
        st.st_mode = stat.S_IFSOCK
        m_fstat.return_value = st
        self.addCleanup(fstat_patcher.stop)

    def write_pipe_transport(self, waiter=None):
        transport = unix_events._UnixWritePipeTransport(self.loop, self.pipe,
                                                        self.protocol,
                                                        waiter=waiter)
        self.addCleanup(close_pipe_transport, transport)
        return transport

    def test_ctor(self):
        waiter = self.loop.create_future()
        tr = self.write_pipe_transport(waiter=waiter)
        self.loop.run_until_complete(waiter)

        self.protocol.connection_made.assert_called_with(tr)
        self.loop.assert_reader(5, tr._read_ready)
        self.assertEqual(None, waiter.result())

    def test_can_write_eof(self):
        tr = self.write_pipe_transport()
        self.assertTrue(tr.can_write_eof())

    @mock.patch('os.write')
    def test_write(self, m_write):
        tr = self.write_pipe_transport()
        m_write.return_value = 4
        tr.write(b'data')
        m_write.assert_called_with(5, b'data')
        self.assertFalse(self.loop.writers)
        self.assertEqual(bytearray(), tr._buffer)

    @mock.patch('os.write')
    def test_write_no_data(self, m_write):
        tr = self.write_pipe_transport()
        tr.write(b'')
        self.assertFalse(m_write.called)
        self.assertFalse(self.loop.writers)
        self.assertEqual(bytearray(b''), tr._buffer)

    @mock.patch('os.write')
    def test_write_partial(self, m_write):
        tr = self.write_pipe_transport()
        m_write.return_value = 2
        tr.write(b'data')
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual(bytearray(b'ta'), tr._buffer)

    @mock.patch('os.write')
    def test_write_buffer(self, m_write):
        tr = self.write_pipe_transport()
        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = bytearray(b'previous')
        tr.write(b'data')
        self.assertFalse(m_write.called)
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual(bytearray(b'previousdata'), tr._buffer)

    @mock.patch('os.write')
    def test_write_again(self, m_write):
        tr = self.write_pipe_transport()
        m_write.side_effect = BlockingIOError()
        tr.write(b'data')
        m_write.assert_called_with(5, bytearray(b'data'))
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual(bytearray(b'data'), tr._buffer)

    @mock.patch('asyncio.unix_events.logger')
    @mock.patch('os.write')
    def test_write_err(self, m_write, m_log):
        tr = self.write_pipe_transport()
        err = OSError()
        m_write.side_effect = err
        tr._fatal_error = mock.Mock()
        tr.write(b'data')
        m_write.assert_called_with(5, b'data')
        self.assertFalse(self.loop.writers)
        self.assertEqual(bytearray(), tr._buffer)
        tr._fatal_error.assert_called_with(
                            err,
                            'Fatal write error on pipe transport')
        self.assertEqual(1, tr._conn_lost)

        tr.write(b'data')
        self.assertEqual(2, tr._conn_lost)
        tr.write(b'data')
        tr.write(b'data')
        tr.write(b'data')
        tr.write(b'data')
        # This is a bit overspecified. :-(
        m_log.warning.assert_called_with(
            'pipe closed by peer or os.write(pipe, data) raised exception.')
        tr.close()

    @mock.patch('os.write')
    def test_write_close(self, m_write):
        tr = self.write_pipe_transport()
        tr._read_ready()  # pipe was closed by peer

        tr.write(b'data')
        self.assertEqual(tr._conn_lost, 1)
        tr.write(b'data')
        self.assertEqual(tr._conn_lost, 2)

    def test__read_ready(self):
        tr = self.write_pipe_transport()
        tr._read_ready()
        self.assertFalse(self.loop.readers)
        self.assertFalse(self.loop.writers)
        self.assertTrue(tr.is_closing())
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)

    @mock.patch('os.write')
    def test__write_ready(self, m_write):
        tr = self.write_pipe_transport()
        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = bytearray(b'data')
        m_write.return_value = 4
        tr._write_ready()
        self.assertFalse(self.loop.writers)
        self.assertEqual(bytearray(), tr._buffer)

    @mock.patch('os.write')
    def test__write_ready_partial(self, m_write):
        tr = self.write_pipe_transport()
        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = bytearray(b'data')
        m_write.return_value = 3
        tr._write_ready()
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual(bytearray(b'a'), tr._buffer)

    @mock.patch('os.write')
    def test__write_ready_again(self, m_write):
        tr = self.write_pipe_transport()
        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = bytearray(b'data')
        m_write.side_effect = BlockingIOError()
        tr._write_ready()
        m_write.assert_called_with(5, bytearray(b'data'))
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual(bytearray(b'data'), tr._buffer)

    @mock.patch('os.write')
    def test__write_ready_empty(self, m_write):
        tr = self.write_pipe_transport()
        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = bytearray(b'data')
        m_write.return_value = 0
        tr._write_ready()
        m_write.assert_called_with(5, bytearray(b'data'))
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual(bytearray(b'data'), tr._buffer)

    @mock.patch('asyncio.log.logger.error')
    @mock.patch('os.write')
    def test__write_ready_err(self, m_write, m_logexc):
        tr = self.write_pipe_transport()
        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = bytearray(b'data')
        m_write.side_effect = err = OSError()
        tr._write_ready()
        self.assertFalse(self.loop.writers)
        self.assertFalse(self.loop.readers)
        self.assertEqual(bytearray(), tr._buffer)
        self.assertTrue(tr.is_closing())
        m_logexc.assert_not_called()
        self.assertEqual(1, tr._conn_lost)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(err)

    @mock.patch('os.write')
    def test__write_ready_closing(self, m_write):
        tr = self.write_pipe_transport()
        self.loop.add_writer(5, tr._write_ready)
        tr._closing = True
        tr._buffer = bytearray(b'data')
        m_write.return_value = 4
        tr._write_ready()
        self.assertFalse(self.loop.writers)
        self.assertFalse(self.loop.readers)
        self.assertEqual(bytearray(), tr._buffer)
        self.protocol.connection_lost.assert_called_with(None)
        self.pipe.close.assert_called_with()

    @mock.patch('os.write')
    def test_abort(self, m_write):
        tr = self.write_pipe_transport()
        self.loop.add_writer(5, tr._write_ready)
        self.loop.add_reader(5, tr._read_ready)
        tr._buffer = [b'da', b'ta']
        tr.abort()
        self.assertFalse(m_write.called)
        self.assertFalse(self.loop.readers)
        self.assertFalse(self.loop.writers)
        self.assertEqual([], tr._buffer)
        self.assertTrue(tr.is_closing())
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)

    def test__call_connection_lost(self):
        tr = self.write_pipe_transport()
        self.assertIsNotNone(tr._protocol)
        self.assertIsNotNone(tr._loop)

        err = None
        tr._call_connection_lost(err)
        self.protocol.connection_lost.assert_called_with(err)
        self.pipe.close.assert_called_with()

        self.assertIsNone(tr._protocol)
        self.assertIsNone(tr._loop)

    def test__call_connection_lost_with_err(self):
        tr = self.write_pipe_transport()
        self.assertIsNotNone(tr._protocol)
        self.assertIsNotNone(tr._loop)

        err = OSError()
        tr._call_connection_lost(err)
        self.protocol.connection_lost.assert_called_with(err)
        self.pipe.close.assert_called_with()

        self.assertIsNone(tr._protocol)
        self.assertIsNone(tr._loop)

    def test_close(self):
        tr = self.write_pipe_transport()
        tr.write_eof = mock.Mock()
        tr.close()
        tr.write_eof.assert_called_with()

        # closing the transport twice must not fail
        tr.close()

    def test_close_closing(self):
        tr = self.write_pipe_transport()
        tr.write_eof = mock.Mock()
        tr._closing = True
        tr.close()
        self.assertFalse(tr.write_eof.called)

    def test_write_eof(self):
        tr = self.write_pipe_transport()
        tr.write_eof()
        self.assertTrue(tr.is_closing())
        self.assertFalse(self.loop.readers)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)

    def test_write_eof_pending(self):
        tr = self.write_pipe_transport()
        tr._buffer = [b'data']
        tr.write_eof()
        self.assertTrue(tr.is_closing())
        self.assertFalse(self.protocol.connection_lost.called)


class TestFunctional(unittest.TestCase):

    def setUp(self):
        self.loop = asyncio.new_event_loop()
        asyncio._set_event_loop(self.loop)

    def tearDown(self):
        self.loop.close()
        asyncio._set_event_loop(None)

    def test_add_reader_invalid_argument(self):
        def assert_raises():
            return self.assertRaisesRegex(ValueError, r'Invalid file object')

        cb = lambda: None

        with assert_raises():
            self.loop.add_reader(object(), cb)
        with assert_raises():
            self.loop.add_writer(object(), cb)

        with assert_raises():
            self.loop.remove_reader(object())
        with assert_raises():
            self.loop.remove_writer(object())

    def test_add_reader_or_writer_transport_fd(self):
        def assert_raises():
            return self.assertRaisesRegex(
                RuntimeError,
                r'File descriptor .* is used by transport')

        async def runner():
            tr, pr = await self.loop.create_connection(
                lambda: asyncio.Protocol(), sock=rsock)

            try:
                cb = lambda: None

                with assert_raises():
                    self.loop.add_reader(rsock, cb)
                with assert_raises():
                    self.loop.add_reader(rsock.fileno(), cb)

                with assert_raises():
                    self.loop.remove_reader(rsock)
                with assert_raises():
                    self.loop.remove_reader(rsock.fileno())

                with assert_raises():
                    self.loop.add_writer(rsock, cb)
                with assert_raises():
                    self.loop.add_writer(rsock.fileno(), cb)

                with assert_raises():
                    self.loop.remove_writer(rsock)
                with assert_raises():
                    self.loop.remove_writer(rsock.fileno())

            finally:
                tr.close()

        rsock, wsock = socket.socketpair()
        try:
            self.loop.run_until_complete(runner())
        finally:
            rsock.close()
            wsock.close()


@support.requires_fork()
class TestFork(unittest.IsolatedAsyncioTestCase):

    async def test_fork_not_share_event_loop(self):
        # The forked process should not share the event loop with the parent
        loop = asyncio.get_running_loop()
        r, w = os.pipe()
        self.addCleanup(os.close, r)
        self.addCleanup(os.close, w)
        pid = os.fork()
        if pid == 0:
            # child
            try:
                loop = asyncio.get_event_loop()
                os.write(w, b'LOOP:' + str(id(loop)).encode())
            except RuntimeError:
                os.write(w, b'NO LOOP')
            except BaseException as e:
                os.write(w, b'ERROR:' + ascii(e).encode())
            finally:
                os._exit(0)
        else:
            # parent
            result = os.read(r, 100)
            self.assertEqual(result, b'NO LOOP')
            wait_process(pid, exitcode=0)

    @hashlib_helper.requires_hashdigest('md5')
    @support.skip_if_sanitizer("TSAN doesn't support threads after fork", thread=True)
    def test_fork_signal_handling(self):
        self.addCleanup(multiprocessing_cleanup_tests)

        # Sending signal to the forked process should not affect the parent
        # process
        ctx = multiprocessing.get_context('fork')
        manager = ctx.Manager()
        self.addCleanup(manager.shutdown)
        child_started = manager.Event()
        child_handled = manager.Event()
        parent_handled = manager.Event()

        def child_main():
            def on_sigterm(*args):
                child_handled.set()
                sys.exit()

            signal.signal(signal.SIGTERM, on_sigterm)
            child_started.set()
            while True:
                time.sleep(1)

        async def main():
            loop = asyncio.get_running_loop()
            loop.add_signal_handler(signal.SIGTERM, lambda *args: parent_handled.set())

            process = ctx.Process(target=child_main)
            process.start()
            child_started.wait()
            os.kill(process.pid, signal.SIGTERM)
            process.join(timeout=support.SHORT_TIMEOUT)

            async def func():
                await asyncio.sleep(0.1)
                return 42

            # Test parent's loop is still functional
            self.assertEqual(await asyncio.create_task(func()), 42)

        asyncio.run(main())

        child_handled.wait(timeout=support.SHORT_TIMEOUT)
        self.assertFalse(parent_handled.is_set())
        self.assertTrue(child_handled.is_set())

    @hashlib_helper.requires_hashdigest('md5')
    @support.skip_if_sanitizer("TSAN doesn't support threads after fork", thread=True)
    def test_fork_asyncio_run(self):
        self.addCleanup(multiprocessing_cleanup_tests)

        ctx = multiprocessing.get_context('fork')
        manager = ctx.Manager()
        self.addCleanup(manager.shutdown)
        result = manager.Value('i', 0)

        async def child_main():
            await asyncio.sleep(0.1)
            result.value = 42

        process = ctx.Process(target=lambda: asyncio.run(child_main()))
        process.start()
        process.join()

        self.assertEqual(result.value, 42)

    @hashlib_helper.requires_hashdigest('md5')
    @support.skip_if_sanitizer("TSAN doesn't support threads after fork", thread=True)
    def test_fork_asyncio_subprocess(self):
        self.addCleanup(multiprocessing_cleanup_tests)

        ctx = multiprocessing.get_context('fork')
        manager = ctx.Manager()
        self.addCleanup(manager.shutdown)
        result = manager.Value('i', 1)

        async def child_main():
            proc = await asyncio.create_subprocess_exec(sys.executable, '-c', 'pass')
            result.value = await proc.wait()

        process = ctx.Process(target=lambda: asyncio.run(child_main()))
        process.start()
        process.join()

        self.assertEqual(result.value, 0)

if __name__ == '__main__':
    unittest.main()
