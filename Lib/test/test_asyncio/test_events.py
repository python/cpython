"""Tests for events.py."""

import collections.abc
import concurrent.futures
import functools
import io
import os
import platform
import re
import signal
import socket
try:
    import ssl
except ImportError:
    ssl = None
import subprocess
import sys
import threading
import time
import errno
import unittest
from unittest import mock
import weakref

if sys.platform not in ('win32', 'vxworks'):
    import tty

import asyncio
from asyncio import coroutines
from asyncio import events
from asyncio import proactor_events
from asyncio import selector_events
from test.test_asyncio import utils as test_utils
from test import support
from test.support import socket_helper
from test.support import threading_helper
from test.support import ALWAYS_EQ, LARGEST, SMALLEST


def tearDownModule():
    asyncio.set_event_loop_policy(None)


def broken_unix_getsockname():
    """Return True if the platform is Mac OS 10.4 or older."""
    if sys.platform.startswith("aix"):
        return True
    elif sys.platform != 'darwin':
        return False
    version = platform.mac_ver()[0]
    version = tuple(map(int, version.split('.')))
    return version < (10, 5)


def _test_get_event_loop_new_process__sub_proc():
    async def doit():
        return 'hello'

    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    return loop.run_until_complete(doit())


class CoroLike:
    def send(self, v):
        pass

    def throw(self, *exc):
        pass

    def close(self):
        pass

    def __await__(self):
        pass


class MyBaseProto(asyncio.Protocol):
    connected = None
    done = None

    def __init__(self, loop=None):
        self.transport = None
        self.state = 'INITIAL'
        self.nbytes = 0
        if loop is not None:
            self.connected = loop.create_future()
            self.done = loop.create_future()

    def connection_made(self, transport):
        self.transport = transport
        assert self.state == 'INITIAL', self.state
        self.state = 'CONNECTED'
        if self.connected:
            self.connected.set_result(None)

    def data_received(self, data):
        assert self.state == 'CONNECTED', self.state
        self.nbytes += len(data)

    def eof_received(self):
        assert self.state == 'CONNECTED', self.state
        self.state = 'EOF'

    def connection_lost(self, exc):
        assert self.state in ('CONNECTED', 'EOF'), self.state
        self.state = 'CLOSED'
        if self.done:
            self.done.set_result(None)


class MyProto(MyBaseProto):
    def connection_made(self, transport):
        super().connection_made(transport)
        transport.write(b'GET / HTTP/1.0\r\nHost: example.com\r\n\r\n')


class MyDatagramProto(asyncio.DatagramProtocol):
    done = None

    def __init__(self, loop=None):
        self.state = 'INITIAL'
        self.nbytes = 0
        if loop is not None:
            self.done = loop.create_future()

    def connection_made(self, transport):
        self.transport = transport
        assert self.state == 'INITIAL', self.state
        self.state = 'INITIALIZED'

    def datagram_received(self, data, addr):
        assert self.state == 'INITIALIZED', self.state
        self.nbytes += len(data)

    def error_received(self, exc):
        assert self.state == 'INITIALIZED', self.state

    def connection_lost(self, exc):
        assert self.state == 'INITIALIZED', self.state
        self.state = 'CLOSED'
        if self.done:
            self.done.set_result(None)


class MyReadPipeProto(asyncio.Protocol):
    done = None

    def __init__(self, loop=None):
        self.state = ['INITIAL']
        self.nbytes = 0
        self.transport = None
        if loop is not None:
            self.done = loop.create_future()

    def connection_made(self, transport):
        self.transport = transport
        assert self.state == ['INITIAL'], self.state
        self.state.append('CONNECTED')

    def data_received(self, data):
        assert self.state == ['INITIAL', 'CONNECTED'], self.state
        self.nbytes += len(data)

    def eof_received(self):
        assert self.state == ['INITIAL', 'CONNECTED'], self.state
        self.state.append('EOF')

    def connection_lost(self, exc):
        if 'EOF' not in self.state:
            self.state.append('EOF')  # It is okay if EOF is missed.
        assert self.state == ['INITIAL', 'CONNECTED', 'EOF'], self.state
        self.state.append('CLOSED')
        if self.done:
            self.done.set_result(None)


class MyWritePipeProto(asyncio.BaseProtocol):
    done = None

    def __init__(self, loop=None):
        self.state = 'INITIAL'
        self.transport = None
        if loop is not None:
            self.done = loop.create_future()

    def connection_made(self, transport):
        self.transport = transport
        assert self.state == 'INITIAL', self.state
        self.state = 'CONNECTED'

    def connection_lost(self, exc):
        assert self.state == 'CONNECTED', self.state
        self.state = 'CLOSED'
        if self.done:
            self.done.set_result(None)


class MySubprocessProtocol(asyncio.SubprocessProtocol):

    def __init__(self, loop):
        self.state = 'INITIAL'
        self.transport = None
        self.connected = loop.create_future()
        self.completed = loop.create_future()
        self.disconnects = {fd: loop.create_future() for fd in range(3)}
        self.data = {1: b'', 2: b''}
        self.returncode = None
        self.got_data = {1: asyncio.Event(),
                         2: asyncio.Event()}

    def connection_made(self, transport):
        self.transport = transport
        assert self.state == 'INITIAL', self.state
        self.state = 'CONNECTED'
        self.connected.set_result(None)

    def connection_lost(self, exc):
        assert self.state == 'CONNECTED', self.state
        self.state = 'CLOSED'
        self.completed.set_result(None)

    def pipe_data_received(self, fd, data):
        assert self.state == 'CONNECTED', self.state
        self.data[fd] += data
        self.got_data[fd].set()

    def pipe_connection_lost(self, fd, exc):
        assert self.state == 'CONNECTED', self.state
        if exc:
            self.disconnects[fd].set_exception(exc)
        else:
            self.disconnects[fd].set_result(exc)

    def process_exited(self):
        assert self.state == 'CONNECTED', self.state
        self.returncode = self.transport.get_returncode()


class EventLoopTestsMixin:

    def setUp(self):
        super().setUp()
        self.loop = self.create_event_loop()
        self.set_event_loop(self.loop)

    def tearDown(self):
        # just in case if we have transport close callbacks
        if not self.loop.is_closed():
            test_utils.run_briefly(self.loop)

        self.doCleanups()
        support.gc_collect()
        super().tearDown()

    def test_run_until_complete_nesting(self):
        async def coro1():
            await asyncio.sleep(0)

        async def coro2():
            self.assertTrue(self.loop.is_running())
            self.loop.run_until_complete(coro1())

        with self.assertWarnsRegex(
            RuntimeWarning,
            r"coroutine \S+ was never awaited"
        ):
            self.assertRaises(
                RuntimeError, self.loop.run_until_complete, coro2())

    # Note: because of the default Windows timing granularity of
    # 15.6 msec, we use fairly long sleep times here (~100 msec).

    def test_run_until_complete(self):
        t0 = self.loop.time()
        self.loop.run_until_complete(asyncio.sleep(0.1))
        t1 = self.loop.time()
        self.assertTrue(0.08 <= t1-t0 <= 0.8, t1-t0)

    def test_run_until_complete_stopped(self):

        async def cb():
            self.loop.stop()
            await asyncio.sleep(0.1)
        task = cb()
        self.assertRaises(RuntimeError,
                          self.loop.run_until_complete, task)

    def test_call_later(self):
        results = []

        def callback(arg):
            results.append(arg)
            self.loop.stop()

        self.loop.call_later(0.1, callback, 'hello world')
        self.loop.run_forever()
        self.assertEqual(results, ['hello world'])

    def test_call_soon(self):
        results = []

        def callback(arg1, arg2):
            results.append((arg1, arg2))
            self.loop.stop()

        self.loop.call_soon(callback, 'hello', 'world')
        self.loop.run_forever()
        self.assertEqual(results, [('hello', 'world')])

    def test_call_soon_threadsafe(self):
        results = []
        lock = threading.Lock()

        def callback(arg):
            results.append(arg)
            if len(results) >= 2:
                self.loop.stop()

        def run_in_thread():
            self.loop.call_soon_threadsafe(callback, 'hello')
            lock.release()

        lock.acquire()
        t = threading.Thread(target=run_in_thread)
        t.start()

        with lock:
            self.loop.call_soon(callback, 'world')
            self.loop.run_forever()
        t.join()
        self.assertEqual(results, ['hello', 'world'])

    def test_call_soon_threadsafe_same_thread(self):
        results = []

        def callback(arg):
            results.append(arg)
            if len(results) >= 2:
                self.loop.stop()

        self.loop.call_soon_threadsafe(callback, 'hello')
        self.loop.call_soon(callback, 'world')
        self.loop.run_forever()
        self.assertEqual(results, ['hello', 'world'])

    def test_run_in_executor(self):
        def run(arg):
            return (arg, threading.get_ident())
        f2 = self.loop.run_in_executor(None, run, 'yo')
        res, thread_id = self.loop.run_until_complete(f2)
        self.assertEqual(res, 'yo')
        self.assertNotEqual(thread_id, threading.get_ident())

    def test_run_in_executor_cancel(self):
        called = False

        def patched_call_soon(*args):
            nonlocal called
            called = True

        def run():
            time.sleep(0.05)

        f2 = self.loop.run_in_executor(None, run)
        f2.cancel()
        self.loop.run_until_complete(
                self.loop.shutdown_default_executor())
        self.loop.close()
        self.loop.call_soon = patched_call_soon
        self.loop.call_soon_threadsafe = patched_call_soon
        time.sleep(0.4)
        self.assertFalse(called)

    def test_reader_callback(self):
        r, w = socket.socketpair()
        r.setblocking(False)
        bytes_read = bytearray()

        def reader():
            try:
                data = r.recv(1024)
            except BlockingIOError:
                # Spurious readiness notifications are possible
                # at least on Linux -- see man select.
                return
            if data:
                bytes_read.extend(data)
            else:
                self.assertTrue(self.loop.remove_reader(r.fileno()))
                r.close()

        self.loop.add_reader(r.fileno(), reader)
        self.loop.call_soon(w.send, b'abc')
        test_utils.run_until(self.loop, lambda: len(bytes_read) >= 3)
        self.loop.call_soon(w.send, b'def')
        test_utils.run_until(self.loop, lambda: len(bytes_read) >= 6)
        self.loop.call_soon(w.close)
        self.loop.call_soon(self.loop.stop)
        self.loop.run_forever()
        self.assertEqual(bytes_read, b'abcdef')

    def test_writer_callback(self):
        r, w = socket.socketpair()
        w.setblocking(False)

        def writer(data):
            w.send(data)
            self.loop.stop()

        data = b'x' * 1024
        self.loop.add_writer(w.fileno(), writer, data)
        self.loop.run_forever()

        self.assertTrue(self.loop.remove_writer(w.fileno()))
        self.assertFalse(self.loop.remove_writer(w.fileno()))

        w.close()
        read = r.recv(len(data) * 2)
        r.close()
        self.assertEqual(read, data)

    @unittest.skipUnless(hasattr(signal, 'SIGKILL'), 'No SIGKILL')
    def test_add_signal_handler(self):
        caught = 0

        def my_handler():
            nonlocal caught
            caught += 1

        # Check error behavior first.
        self.assertRaises(
            TypeError, self.loop.add_signal_handler, 'boom', my_handler)
        self.assertRaises(
            TypeError, self.loop.remove_signal_handler, 'boom')
        self.assertRaises(
            ValueError, self.loop.add_signal_handler, signal.NSIG+1,
            my_handler)
        self.assertRaises(
            ValueError, self.loop.remove_signal_handler, signal.NSIG+1)
        self.assertRaises(
            ValueError, self.loop.add_signal_handler, 0, my_handler)
        self.assertRaises(
            ValueError, self.loop.remove_signal_handler, 0)
        self.assertRaises(
            ValueError, self.loop.add_signal_handler, -1, my_handler)
        self.assertRaises(
            ValueError, self.loop.remove_signal_handler, -1)
        self.assertRaises(
            RuntimeError, self.loop.add_signal_handler, signal.SIGKILL,
            my_handler)
        # Removing SIGKILL doesn't raise, since we don't call signal().
        self.assertFalse(self.loop.remove_signal_handler(signal.SIGKILL))
        # Now set a handler and handle it.
        self.loop.add_signal_handler(signal.SIGINT, my_handler)

        os.kill(os.getpid(), signal.SIGINT)
        test_utils.run_until(self.loop, lambda: caught)

        # Removing it should restore the default handler.
        self.assertTrue(self.loop.remove_signal_handler(signal.SIGINT))
        self.assertEqual(signal.getsignal(signal.SIGINT),
                         signal.default_int_handler)
        # Removing again returns False.
        self.assertFalse(self.loop.remove_signal_handler(signal.SIGINT))

    @unittest.skipUnless(hasattr(signal, 'SIGALRM'), 'No SIGALRM')
    @unittest.skipUnless(hasattr(signal, 'setitimer'),
                         'need signal.setitimer()')
    def test_signal_handling_while_selecting(self):
        # Test with a signal actually arriving during a select() call.
        caught = 0

        def my_handler():
            nonlocal caught
            caught += 1
            self.loop.stop()

        self.loop.add_signal_handler(signal.SIGALRM, my_handler)

        signal.setitimer(signal.ITIMER_REAL, 0.01, 0)  # Send SIGALRM once.
        self.loop.call_later(60, self.loop.stop)
        self.loop.run_forever()
        self.assertEqual(caught, 1)

    @unittest.skipUnless(hasattr(signal, 'SIGALRM'), 'No SIGALRM')
    @unittest.skipUnless(hasattr(signal, 'setitimer'),
                         'need signal.setitimer()')
    def test_signal_handling_args(self):
        some_args = (42,)
        caught = 0

        def my_handler(*args):
            nonlocal caught
            caught += 1
            self.assertEqual(args, some_args)
            self.loop.stop()

        self.loop.add_signal_handler(signal.SIGALRM, my_handler, *some_args)

        signal.setitimer(signal.ITIMER_REAL, 0.1, 0)  # Send SIGALRM once.
        self.loop.call_later(60, self.loop.stop)
        self.loop.run_forever()
        self.assertEqual(caught, 1)

    def _basetest_create_connection(self, connection_fut, check_sockname=True):
        tr, pr = self.loop.run_until_complete(connection_fut)
        self.assertIsInstance(tr, asyncio.Transport)
        self.assertIsInstance(pr, asyncio.Protocol)
        self.assertIs(pr.transport, tr)
        if check_sockname:
            self.assertIsNotNone(tr.get_extra_info('sockname'))
        self.loop.run_until_complete(pr.done)
        self.assertGreater(pr.nbytes, 0)
        tr.close()

    def test_create_connection(self):
        with test_utils.run_test_server() as httpd:
            conn_fut = self.loop.create_connection(
                lambda: MyProto(loop=self.loop), *httpd.address)
            self._basetest_create_connection(conn_fut)

    @socket_helper.skip_unless_bind_unix_socket
    def test_create_unix_connection(self):
        # Issue #20682: On Mac OS X Tiger, getsockname() returns a
        # zero-length address for UNIX socket.
        check_sockname = not broken_unix_getsockname()

        with test_utils.run_test_unix_server() as httpd:
            conn_fut = self.loop.create_unix_connection(
                lambda: MyProto(loop=self.loop), httpd.address)
            self._basetest_create_connection(conn_fut, check_sockname)

    def check_ssl_extra_info(self, client, check_sockname=True,
                             peername=None, peercert={}):
        if check_sockname:
            self.assertIsNotNone(client.get_extra_info('sockname'))
        if peername:
            self.assertEqual(peername,
                             client.get_extra_info('peername'))
        else:
            self.assertIsNotNone(client.get_extra_info('peername'))
        self.assertEqual(peercert,
                         client.get_extra_info('peercert'))

        # test SSL cipher
        cipher = client.get_extra_info('cipher')
        self.assertIsInstance(cipher, tuple)
        self.assertEqual(len(cipher), 3, cipher)
        self.assertIsInstance(cipher[0], str)
        self.assertIsInstance(cipher[1], str)
        self.assertIsInstance(cipher[2], int)

        # test SSL object
        sslobj = client.get_extra_info('ssl_object')
        self.assertIsNotNone(sslobj)
        self.assertEqual(sslobj.compression(),
                         client.get_extra_info('compression'))
        self.assertEqual(sslobj.cipher(),
                         client.get_extra_info('cipher'))
        self.assertEqual(sslobj.getpeercert(),
                         client.get_extra_info('peercert'))
        self.assertEqual(sslobj.compression(),
                         client.get_extra_info('compression'))

    def _basetest_create_ssl_connection(self, connection_fut,
                                        check_sockname=True,
                                        peername=None):
        tr, pr = self.loop.run_until_complete(connection_fut)
        self.assertIsInstance(tr, asyncio.Transport)
        self.assertIsInstance(pr, asyncio.Protocol)
        self.assertTrue('ssl' in tr.__class__.__name__.lower())
        self.check_ssl_extra_info(tr, check_sockname, peername)
        self.loop.run_until_complete(pr.done)
        self.assertGreater(pr.nbytes, 0)
        tr.close()

    def _test_create_ssl_connection(self, httpd, create_connection,
                                    check_sockname=True, peername=None):
        conn_fut = create_connection(ssl=test_utils.dummy_ssl_context())
        self._basetest_create_ssl_connection(conn_fut, check_sockname,
                                             peername)

        # ssl.Purpose was introduced in Python 3.4
        if hasattr(ssl, 'Purpose'):
            def _dummy_ssl_create_context(purpose=ssl.Purpose.SERVER_AUTH, *,
                                          cafile=None, capath=None,
                                          cadata=None):
                """
                A ssl.create_default_context() replacement that doesn't enable
                cert validation.
                """
                self.assertEqual(purpose, ssl.Purpose.SERVER_AUTH)
                return test_utils.dummy_ssl_context()

            # With ssl=True, ssl.create_default_context() should be called
            with mock.patch('ssl.create_default_context',
                            side_effect=_dummy_ssl_create_context) as m:
                conn_fut = create_connection(ssl=True)
                self._basetest_create_ssl_connection(conn_fut, check_sockname,
                                                     peername)
                self.assertEqual(m.call_count, 1)

        # With the real ssl.create_default_context(), certificate
        # validation will fail
        with self.assertRaises(ssl.SSLError) as cm:
            conn_fut = create_connection(ssl=True)
            # Ignore the "SSL handshake failed" log in debug mode
            with test_utils.disable_logger():
                self._basetest_create_ssl_connection(conn_fut, check_sockname,
                                                     peername)

        self.assertEqual(cm.exception.reason, 'CERTIFICATE_VERIFY_FAILED')

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_ssl_connection(self):
        with test_utils.run_test_server(use_ssl=True) as httpd:
            create_connection = functools.partial(
                self.loop.create_connection,
                lambda: MyProto(loop=self.loop),
                *httpd.address)
            self._test_create_ssl_connection(httpd, create_connection,
                                             peername=httpd.address)

    @socket_helper.skip_unless_bind_unix_socket
    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_ssl_unix_connection(self):
        # Issue #20682: On Mac OS X Tiger, getsockname() returns a
        # zero-length address for UNIX socket.
        check_sockname = not broken_unix_getsockname()

        with test_utils.run_test_unix_server(use_ssl=True) as httpd:
            create_connection = functools.partial(
                self.loop.create_unix_connection,
                lambda: MyProto(loop=self.loop), httpd.address,
                server_hostname='127.0.0.1')

            self._test_create_ssl_connection(httpd, create_connection,
                                             check_sockname,
                                             peername=httpd.address)

    def test_create_connection_local_addr(self):
        with test_utils.run_test_server() as httpd:
            port = socket_helper.find_unused_port()
            f = self.loop.create_connection(
                lambda: MyProto(loop=self.loop),
                *httpd.address, local_addr=(httpd.address[0], port))
            tr, pr = self.loop.run_until_complete(f)
            expected = pr.transport.get_extra_info('sockname')[1]
            self.assertEqual(port, expected)
            tr.close()

    def test_create_connection_local_addr_in_use(self):
        with test_utils.run_test_server() as httpd:
            f = self.loop.create_connection(
                lambda: MyProto(loop=self.loop),
                *httpd.address, local_addr=httpd.address)
            with self.assertRaises(OSError) as cm:
                self.loop.run_until_complete(f)
            self.assertEqual(cm.exception.errno, errno.EADDRINUSE)
            self.assertIn(str(httpd.address), cm.exception.strerror)

    def test_connect_accepted_socket(self, server_ssl=None, client_ssl=None):
        loop = self.loop

        class MyProto(MyBaseProto):

            def connection_lost(self, exc):
                super().connection_lost(exc)
                loop.call_soon(loop.stop)

            def data_received(self, data):
                super().data_received(data)
                self.transport.write(expected_response)

        lsock = socket.create_server(('127.0.0.1', 0), backlog=1)
        addr = lsock.getsockname()

        message = b'test data'
        response = None
        expected_response = b'roger'

        def client():
            nonlocal response
            try:
                csock = socket.socket()
                if client_ssl is not None:
                    csock = client_ssl.wrap_socket(csock)
                csock.connect(addr)
                csock.sendall(message)
                response = csock.recv(99)
                csock.close()
            except Exception as exc:
                print(
                    "Failure in client thread in test_connect_accepted_socket",
                    exc)

        thread = threading.Thread(target=client, daemon=True)
        thread.start()

        conn, _ = lsock.accept()
        proto = MyProto(loop=loop)
        proto.loop = loop
        loop.run_until_complete(
            loop.connect_accepted_socket(
                (lambda: proto), conn, ssl=server_ssl))
        loop.run_forever()
        proto.transport.close()
        lsock.close()

        threading_helper.join_thread(thread)
        self.assertFalse(thread.is_alive())
        self.assertEqual(proto.state, 'CLOSED')
        self.assertEqual(proto.nbytes, len(message))
        self.assertEqual(response, expected_response)

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_ssl_connect_accepted_socket(self):
        if (sys.platform == 'win32' and
            sys.version_info < (3, 5) and
            isinstance(self.loop, proactor_events.BaseProactorEventLoop)
            ):
            raise unittest.SkipTest(
                'SSL not supported with proactor event loops before Python 3.5'
                )

        server_context = test_utils.simple_server_sslcontext()
        client_context = test_utils.simple_client_sslcontext()

        self.test_connect_accepted_socket(server_context, client_context)

    def test_connect_accepted_socket_ssl_timeout_for_plain_socket(self):
        sock = socket.socket()
        self.addCleanup(sock.close)
        coro = self.loop.connect_accepted_socket(
            MyProto, sock, ssl_handshake_timeout=support.LOOPBACK_TIMEOUT)
        with self.assertRaisesRegex(
                ValueError,
                'ssl_handshake_timeout is only meaningful with ssl'):
            self.loop.run_until_complete(coro)

    @mock.patch('asyncio.base_events.socket')
    def create_server_multiple_hosts(self, family, hosts, mock_sock):
        async def getaddrinfo(host, port, *args, **kw):
            if family == socket.AF_INET:
                return [(family, socket.SOCK_STREAM, 6, '', (host, port))]
            else:
                return [(family, socket.SOCK_STREAM, 6, '', (host, port, 0, 0))]

        def getaddrinfo_task(*args, **kwds):
            return self.loop.create_task(getaddrinfo(*args, **kwds))

        unique_hosts = set(hosts)

        if family == socket.AF_INET:
            mock_sock.socket().getsockbyname.side_effect = [
                (host, 80) for host in unique_hosts]
        else:
            mock_sock.socket().getsockbyname.side_effect = [
                (host, 80, 0, 0) for host in unique_hosts]
        self.loop.getaddrinfo = getaddrinfo_task
        self.loop._start_serving = mock.Mock()
        self.loop._stop_serving = mock.Mock()
        f = self.loop.create_server(lambda: MyProto(self.loop), hosts, 80)
        server = self.loop.run_until_complete(f)
        self.addCleanup(server.close)
        server_hosts = {sock.getsockbyname()[0] for sock in server.sockets}
        self.assertEqual(server_hosts, unique_hosts)

    def test_create_server_multiple_hosts_ipv4(self):
        self.create_server_multiple_hosts(socket.AF_INET,
                                          ['1.2.3.4', '5.6.7.8', '1.2.3.4'])

    def test_create_server_multiple_hosts_ipv6(self):
        self.create_server_multiple_hosts(socket.AF_INET6,
                                          ['::1', '::2', '::1'])

    def test_create_server(self):
        proto = MyProto(self.loop)
        f = self.loop.create_server(lambda: proto, '0.0.0.0', 0)
        server = self.loop.run_until_complete(f)
        self.assertEqual(len(server.sockets), 1)
        sock = server.sockets[0]
        host, port = sock.getsockname()
        self.assertEqual(host, '0.0.0.0')
        client = socket.socket()
        client.connect(('127.0.0.1', port))
        client.sendall(b'xxx')

        self.loop.run_until_complete(proto.connected)
        self.assertEqual('CONNECTED', proto.state)

        test_utils.run_until(self.loop, lambda: proto.nbytes > 0)
        self.assertEqual(3, proto.nbytes)

        # extra info is available
        self.assertIsNotNone(proto.transport.get_extra_info('sockname'))
        self.assertEqual('127.0.0.1',
                         proto.transport.get_extra_info('peername')[0])

        # close connection
        proto.transport.close()
        self.loop.run_until_complete(proto.done)

        self.assertEqual('CLOSED', proto.state)

        # the client socket must be closed after to avoid ECONNRESET upon
        # recv()/send() on the serving socket
        client.close()

        # close server
        server.close()

    @unittest.skipUnless(hasattr(socket, 'SO_REUSEPORT'), 'No SO_REUSEPORT')
    def test_create_server_reuse_port(self):
        proto = MyProto(self.loop)
        f = self.loop.create_server(
            lambda: proto, '0.0.0.0', 0)
        server = self.loop.run_until_complete(f)
        self.assertEqual(len(server.sockets), 1)
        sock = server.sockets[0]
        self.assertFalse(
            sock.getsockopt(
                socket.SOL_SOCKET, socket.SO_REUSEPORT))
        server.close()

        test_utils.run_briefly(self.loop)

        proto = MyProto(self.loop)
        f = self.loop.create_server(
            lambda: proto, '0.0.0.0', 0, reuse_port=True)
        server = self.loop.run_until_complete(f)
        self.assertEqual(len(server.sockets), 1)
        sock = server.sockets[0]
        self.assertTrue(
            sock.getsockopt(
                socket.SOL_SOCKET, socket.SO_REUSEPORT))
        server.close()

    def _make_unix_server(self, factory, **kwargs):
        path = test_utils.gen_unix_socket_path()
        self.addCleanup(lambda: os.path.exists(path) and os.unlink(path))

        f = self.loop.create_unix_server(factory, path, **kwargs)
        server = self.loop.run_until_complete(f)

        return server, path

    @socket_helper.skip_unless_bind_unix_socket
    def test_create_unix_server(self):
        proto = MyProto(loop=self.loop)
        server, path = self._make_unix_server(lambda: proto)
        self.assertEqual(len(server.sockets), 1)

        client = socket.socket(socket.AF_UNIX)
        client.connect(path)
        client.sendall(b'xxx')

        self.loop.run_until_complete(proto.connected)
        self.assertEqual('CONNECTED', proto.state)
        test_utils.run_until(self.loop, lambda: proto.nbytes > 0)
        self.assertEqual(3, proto.nbytes)

        # close connection
        proto.transport.close()
        self.loop.run_until_complete(proto.done)

        self.assertEqual('CLOSED', proto.state)

        # the client socket must be closed after to avoid ECONNRESET upon
        # recv()/send() on the serving socket
        client.close()

        # close server
        server.close()

    @unittest.skipUnless(hasattr(socket, 'AF_UNIX'), 'No UNIX Sockets')
    def test_create_unix_server_path_socket_error(self):
        proto = MyProto(loop=self.loop)
        sock = socket.socket()
        with sock:
            f = self.loop.create_unix_server(lambda: proto, '/test', sock=sock)
            with self.assertRaisesRegex(ValueError,
                                        'path and sock can not be specified '
                                        'at the same time'):
                self.loop.run_until_complete(f)

    def _create_ssl_context(self, certfile, keyfile=None):
        sslcontext = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        sslcontext.options |= ssl.OP_NO_SSLv2
        sslcontext.load_cert_chain(certfile, keyfile)
        return sslcontext

    def _make_ssl_server(self, factory, certfile, keyfile=None):
        sslcontext = self._create_ssl_context(certfile, keyfile)

        f = self.loop.create_server(factory, '127.0.0.1', 0, ssl=sslcontext)
        server = self.loop.run_until_complete(f)

        sock = server.sockets[0]
        host, port = sock.getsockname()
        self.assertEqual(host, '127.0.0.1')
        return server, host, port

    def _make_ssl_unix_server(self, factory, certfile, keyfile=None):
        sslcontext = self._create_ssl_context(certfile, keyfile)
        return self._make_unix_server(factory, ssl=sslcontext)

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_server_ssl(self):
        proto = MyProto(loop=self.loop)
        server, host, port = self._make_ssl_server(
            lambda: proto, test_utils.ONLYCERT, test_utils.ONLYKEY)

        f_c = self.loop.create_connection(MyBaseProto, host, port,
                                          ssl=test_utils.dummy_ssl_context())
        client, pr = self.loop.run_until_complete(f_c)

        client.write(b'xxx')
        self.loop.run_until_complete(proto.connected)
        self.assertEqual('CONNECTED', proto.state)

        test_utils.run_until(self.loop, lambda: proto.nbytes > 0)
        self.assertEqual(3, proto.nbytes)

        # extra info is available
        self.check_ssl_extra_info(client, peername=(host, port))

        # close connection
        proto.transport.close()
        self.loop.run_until_complete(proto.done)
        self.assertEqual('CLOSED', proto.state)

        # the client socket must be closed after to avoid ECONNRESET upon
        # recv()/send() on the serving socket
        client.close()

        # stop serving
        server.close()

    @socket_helper.skip_unless_bind_unix_socket
    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_unix_server_ssl(self):
        proto = MyProto(loop=self.loop)
        server, path = self._make_ssl_unix_server(
            lambda: proto, test_utils.ONLYCERT, test_utils.ONLYKEY)

        f_c = self.loop.create_unix_connection(
            MyBaseProto, path, ssl=test_utils.dummy_ssl_context(),
            server_hostname='')

        client, pr = self.loop.run_until_complete(f_c)

        client.write(b'xxx')
        self.loop.run_until_complete(proto.connected)
        self.assertEqual('CONNECTED', proto.state)
        test_utils.run_until(self.loop, lambda: proto.nbytes > 0)
        self.assertEqual(3, proto.nbytes)

        # close connection
        proto.transport.close()
        self.loop.run_until_complete(proto.done)
        self.assertEqual('CLOSED', proto.state)

        # the client socket must be closed after to avoid ECONNRESET upon
        # recv()/send() on the serving socket
        client.close()

        # stop serving
        server.close()

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_server_ssl_verify_failed(self):
        proto = MyProto(loop=self.loop)
        server, host, port = self._make_ssl_server(
            lambda: proto, test_utils.SIGNED_CERTFILE)

        sslcontext_client = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        sslcontext_client.options |= ssl.OP_NO_SSLv2
        sslcontext_client.verify_mode = ssl.CERT_REQUIRED
        if hasattr(sslcontext_client, 'check_hostname'):
            sslcontext_client.check_hostname = True


        # no CA loaded
        f_c = self.loop.create_connection(MyProto, host, port,
                                          ssl=sslcontext_client)
        with mock.patch.object(self.loop, 'call_exception_handler'):
            with test_utils.disable_logger():
                with self.assertRaisesRegex(ssl.SSLError,
                                            '(?i)certificate.verify.failed'):
                    self.loop.run_until_complete(f_c)

            # execute the loop to log the connection error
            test_utils.run_briefly(self.loop)

        # close connection
        self.assertIsNone(proto.transport)
        server.close()

    @socket_helper.skip_unless_bind_unix_socket
    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_unix_server_ssl_verify_failed(self):
        proto = MyProto(loop=self.loop)
        server, path = self._make_ssl_unix_server(
            lambda: proto, test_utils.SIGNED_CERTFILE)

        sslcontext_client = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        sslcontext_client.options |= ssl.OP_NO_SSLv2
        sslcontext_client.verify_mode = ssl.CERT_REQUIRED
        if hasattr(sslcontext_client, 'check_hostname'):
            sslcontext_client.check_hostname = True

        # no CA loaded
        f_c = self.loop.create_unix_connection(MyProto, path,
                                               ssl=sslcontext_client,
                                               server_hostname='invalid')
        with mock.patch.object(self.loop, 'call_exception_handler'):
            with test_utils.disable_logger():
                with self.assertRaisesRegex(ssl.SSLError,
                                            '(?i)certificate.verify.failed'):
                    self.loop.run_until_complete(f_c)

            # execute the loop to log the connection error
            test_utils.run_briefly(self.loop)

        # close connection
        self.assertIsNone(proto.transport)
        server.close()

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_server_ssl_match_failed(self):
        proto = MyProto(loop=self.loop)
        server, host, port = self._make_ssl_server(
            lambda: proto, test_utils.SIGNED_CERTFILE)

        sslcontext_client = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        sslcontext_client.options |= ssl.OP_NO_SSLv2
        sslcontext_client.verify_mode = ssl.CERT_REQUIRED
        sslcontext_client.load_verify_locations(
            cafile=test_utils.SIGNING_CA)
        if hasattr(sslcontext_client, 'check_hostname'):
            sslcontext_client.check_hostname = True

        # incorrect server_hostname
        f_c = self.loop.create_connection(MyProto, host, port,
                                          ssl=sslcontext_client)
        with mock.patch.object(self.loop, 'call_exception_handler'):
            with test_utils.disable_logger():
                with self.assertRaisesRegex(
                        ssl.CertificateError,
                        "IP address mismatch, certificate is not valid for "
                        "'127.0.0.1'"):
                    self.loop.run_until_complete(f_c)

        # close connection
        # transport is None because TLS ALERT aborted the handshake
        self.assertIsNone(proto.transport)
        server.close()

    @socket_helper.skip_unless_bind_unix_socket
    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_unix_server_ssl_verified(self):
        proto = MyProto(loop=self.loop)
        server, path = self._make_ssl_unix_server(
            lambda: proto, test_utils.SIGNED_CERTFILE)

        sslcontext_client = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        sslcontext_client.options |= ssl.OP_NO_SSLv2
        sslcontext_client.verify_mode = ssl.CERT_REQUIRED
        sslcontext_client.load_verify_locations(cafile=test_utils.SIGNING_CA)
        if hasattr(sslcontext_client, 'check_hostname'):
            sslcontext_client.check_hostname = True

        # Connection succeeds with correct CA and server hostname.
        f_c = self.loop.create_unix_connection(MyProto, path,
                                               ssl=sslcontext_client,
                                               server_hostname='localhost')
        client, pr = self.loop.run_until_complete(f_c)
        self.loop.run_until_complete(proto.connected)

        # close connection
        proto.transport.close()
        client.close()
        server.close()
        self.loop.run_until_complete(proto.done)

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_server_ssl_verified(self):
        proto = MyProto(loop=self.loop)
        server, host, port = self._make_ssl_server(
            lambda: proto, test_utils.SIGNED_CERTFILE)

        sslcontext_client = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        sslcontext_client.options |= ssl.OP_NO_SSLv2
        sslcontext_client.verify_mode = ssl.CERT_REQUIRED
        sslcontext_client.load_verify_locations(cafile=test_utils.SIGNING_CA)
        if hasattr(sslcontext_client, 'check_hostname'):
            sslcontext_client.check_hostname = True

        # Connection succeeds with correct CA and server hostname.
        f_c = self.loop.create_connection(MyProto, host, port,
                                          ssl=sslcontext_client,
                                          server_hostname='localhost')
        client, pr = self.loop.run_until_complete(f_c)
        self.loop.run_until_complete(proto.connected)

        # extra info is available
        self.check_ssl_extra_info(client, peername=(host, port),
                                  peercert=test_utils.PEERCERT)

        # close connection
        proto.transport.close()
        client.close()
        server.close()
        self.loop.run_until_complete(proto.done)

    def test_create_server_sock(self):
        proto = self.loop.create_future()

        class TestMyProto(MyProto):
            def connection_made(self, transport):
                super().connection_made(transport)
                proto.set_result(self)

        sock_ob = socket.create_server(('0.0.0.0', 0))

        f = self.loop.create_server(TestMyProto, sock=sock_ob)
        server = self.loop.run_until_complete(f)
        sock = server.sockets[0]
        self.assertEqual(sock.fileno(), sock_ob.fileno())

        host, port = sock.getsockname()
        self.assertEqual(host, '0.0.0.0')
        client = socket.socket()
        client.connect(('127.0.0.1', port))
        client.send(b'xxx')
        client.close()
        server.close()

    def test_create_server_addr_in_use(self):
        sock_ob = socket.create_server(('0.0.0.0', 0))

        f = self.loop.create_server(MyProto, sock=sock_ob)
        server = self.loop.run_until_complete(f)
        sock = server.sockets[0]
        host, port = sock.getsockname()

        f = self.loop.create_server(MyProto, host=host, port=port)
        with self.assertRaises(OSError) as cm:
            self.loop.run_until_complete(f)
        self.assertEqual(cm.exception.errno, errno.EADDRINUSE)

        server.close()

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 not supported or enabled')
    def test_create_server_dual_stack(self):
        f_proto = self.loop.create_future()

        class TestMyProto(MyProto):
            def connection_made(self, transport):
                super().connection_made(transport)
                f_proto.set_result(self)

        try_count = 0
        while True:
            try:
                port = socket_helper.find_unused_port()
                f = self.loop.create_server(TestMyProto, host=None, port=port)
                server = self.loop.run_until_complete(f)
            except OSError as ex:
                if ex.errno == errno.EADDRINUSE:
                    try_count += 1
                    self.assertGreaterEqual(5, try_count)
                    continue
                else:
                    raise
            else:
                break
        client = socket.socket()
        client.connect(('127.0.0.1', port))
        client.send(b'xxx')
        proto = self.loop.run_until_complete(f_proto)
        proto.transport.close()
        client.close()

        f_proto = self.loop.create_future()
        client = socket.socket(socket.AF_INET6)
        client.connect(('::1', port))
        client.send(b'xxx')
        proto = self.loop.run_until_complete(f_proto)
        proto.transport.close()
        client.close()

        server.close()

    def test_server_close(self):
        f = self.loop.create_server(MyProto, '0.0.0.0', 0)
        server = self.loop.run_until_complete(f)
        sock = server.sockets[0]
        host, port = sock.getsockname()

        client = socket.socket()
        client.connect(('127.0.0.1', port))
        client.send(b'xxx')
        client.close()

        server.close()

        client = socket.socket()
        self.assertRaises(
            ConnectionRefusedError, client.connect, ('127.0.0.1', port))
        client.close()

    def _test_create_datagram_endpoint(self, local_addr, family):
        class TestMyDatagramProto(MyDatagramProto):
            def __init__(inner_self):
                super().__init__(loop=self.loop)

            def datagram_received(self, data, addr):
                super().datagram_received(data, addr)
                self.transport.sendto(b'resp:'+data, addr)

        coro = self.loop.create_datagram_endpoint(
            TestMyDatagramProto, local_addr=local_addr, family=family)
        s_transport, server = self.loop.run_until_complete(coro)
        sockname = s_transport.get_extra_info('sockname')
        host, port = socket.getnameinfo(
            sockname, socket.NI_NUMERICHOST|socket.NI_NUMERICSERV)

        self.assertIsInstance(s_transport, asyncio.Transport)
        self.assertIsInstance(server, TestMyDatagramProto)
        self.assertEqual('INITIALIZED', server.state)
        self.assertIs(server.transport, s_transport)

        coro = self.loop.create_datagram_endpoint(
            lambda: MyDatagramProto(loop=self.loop),
            remote_addr=(host, port))
        transport, client = self.loop.run_until_complete(coro)

        self.assertIsInstance(transport, asyncio.Transport)
        self.assertIsInstance(client, MyDatagramProto)
        self.assertEqual('INITIALIZED', client.state)
        self.assertIs(client.transport, transport)

        transport.sendto(b'xxx')
        test_utils.run_until(self.loop, lambda: server.nbytes)
        self.assertEqual(3, server.nbytes)
        test_utils.run_until(self.loop, lambda: client.nbytes)

        # received
        self.assertEqual(8, client.nbytes)

        # extra info is available
        self.assertIsNotNone(transport.get_extra_info('sockname'))

        # close connection
        transport.close()
        self.loop.run_until_complete(client.done)
        self.assertEqual('CLOSED', client.state)
        server.transport.close()

    def test_create_datagram_endpoint(self):
        self._test_create_datagram_endpoint(('127.0.0.1', 0), socket.AF_INET)

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 not supported or enabled')
    def test_create_datagram_endpoint_ipv6(self):
        self._test_create_datagram_endpoint(('::1', 0), socket.AF_INET6)

    def test_create_datagram_endpoint_sock(self):
        sock = None
        local_address = ('127.0.0.1', 0)
        infos = self.loop.run_until_complete(
            self.loop.getaddrinfo(
                *local_address, type=socket.SOCK_DGRAM))
        for family, type, proto, cname, address in infos:
            try:
                sock = socket.socket(family=family, type=type, proto=proto)
                sock.setblocking(False)
                sock.bind(address)
            except:
                pass
            else:
                break
        else:
            assert False, 'Can not create socket.'

        f = self.loop.create_datagram_endpoint(
            lambda: MyDatagramProto(loop=self.loop), sock=sock)
        tr, pr = self.loop.run_until_complete(f)
        self.assertIsInstance(tr, asyncio.Transport)
        self.assertIsInstance(pr, MyDatagramProto)
        tr.close()
        self.loop.run_until_complete(pr.done)

    def test_internal_fds(self):
        loop = self.create_event_loop()
        if not isinstance(loop, selector_events.BaseSelectorEventLoop):
            loop.close()
            self.skipTest('loop is not a BaseSelectorEventLoop')

        self.assertEqual(1, loop._internal_fds)
        loop.close()
        self.assertEqual(0, loop._internal_fds)
        self.assertIsNone(loop._csock)
        self.assertIsNone(loop._ssock)

    @unittest.skipUnless(sys.platform != 'win32',
                         "Don't support pipes for Windows")
    def test_read_pipe(self):
        proto = MyReadPipeProto(loop=self.loop)

        rpipe, wpipe = os.pipe()
        pipeobj = io.open(rpipe, 'rb', 1024)

        async def connect():
            t, p = await self.loop.connect_read_pipe(
                lambda: proto, pipeobj)
            self.assertIs(p, proto)
            self.assertIs(t, proto.transport)
            self.assertEqual(['INITIAL', 'CONNECTED'], proto.state)
            self.assertEqual(0, proto.nbytes)

        self.loop.run_until_complete(connect())

        os.write(wpipe, b'1')
        test_utils.run_until(self.loop, lambda: proto.nbytes >= 1)
        self.assertEqual(1, proto.nbytes)

        os.write(wpipe, b'2345')
        test_utils.run_until(self.loop, lambda: proto.nbytes >= 5)
        self.assertEqual(['INITIAL', 'CONNECTED'], proto.state)
        self.assertEqual(5, proto.nbytes)

        os.close(wpipe)
        self.loop.run_until_complete(proto.done)
        self.assertEqual(
            ['INITIAL', 'CONNECTED', 'EOF', 'CLOSED'], proto.state)
        # extra info is available
        self.assertIsNotNone(proto.transport.get_extra_info('pipe'))

    @unittest.skipUnless(sys.platform != 'win32',
                         "Don't support pipes for Windows")
    def test_unclosed_pipe_transport(self):
        # This test reproduces the issue #314 on GitHub
        loop = self.create_event_loop()
        read_proto = MyReadPipeProto(loop=loop)
        write_proto = MyWritePipeProto(loop=loop)

        rpipe, wpipe = os.pipe()
        rpipeobj = io.open(rpipe, 'rb', 1024)
        wpipeobj = io.open(wpipe, 'w', 1024, encoding="utf-8")

        async def connect():
            read_transport, _ = await loop.connect_read_pipe(
                lambda: read_proto, rpipeobj)
            write_transport, _ = await loop.connect_write_pipe(
                lambda: write_proto, wpipeobj)
            return read_transport, write_transport

        # Run and close the loop without closing the transports
        read_transport, write_transport = loop.run_until_complete(connect())
        loop.close()

        # These 'repr' calls used to raise an AttributeError
        # See Issue #314 on GitHub
        self.assertIn('open', repr(read_transport))
        self.assertIn('open', repr(write_transport))

        # Clean up (avoid ResourceWarning)
        rpipeobj.close()
        wpipeobj.close()
        read_transport._pipe = None
        write_transport._pipe = None

    @unittest.skipUnless(sys.platform != 'win32',
                         "Don't support pipes for Windows")
    @unittest.skipUnless(hasattr(os, 'openpty'), 'need os.openpty()')
    def test_read_pty_output(self):
        proto = MyReadPipeProto(loop=self.loop)

        master, slave = os.openpty()
        master_read_obj = io.open(master, 'rb', 0)

        async def connect():
            t, p = await self.loop.connect_read_pipe(lambda: proto,
                                                     master_read_obj)
            self.assertIs(p, proto)
            self.assertIs(t, proto.transport)
            self.assertEqual(['INITIAL', 'CONNECTED'], proto.state)
            self.assertEqual(0, proto.nbytes)

        self.loop.run_until_complete(connect())

        os.write(slave, b'1')
        test_utils.run_until(self.loop, lambda: proto.nbytes)
        self.assertEqual(1, proto.nbytes)

        os.write(slave, b'2345')
        test_utils.run_until(self.loop, lambda: proto.nbytes >= 5)
        self.assertEqual(['INITIAL', 'CONNECTED'], proto.state)
        self.assertEqual(5, proto.nbytes)

        os.close(slave)
        proto.transport.close()
        self.loop.run_until_complete(proto.done)
        self.assertEqual(
            ['INITIAL', 'CONNECTED', 'EOF', 'CLOSED'], proto.state)
        # extra info is available
        self.assertIsNotNone(proto.transport.get_extra_info('pipe'))

    @unittest.skipUnless(sys.platform != 'win32',
                         "Don't support pipes for Windows")
    def test_write_pipe(self):
        rpipe, wpipe = os.pipe()
        pipeobj = io.open(wpipe, 'wb', 1024)

        proto = MyWritePipeProto(loop=self.loop)
        connect = self.loop.connect_write_pipe(lambda: proto, pipeobj)
        transport, p = self.loop.run_until_complete(connect)
        self.assertIs(p, proto)
        self.assertIs(transport, proto.transport)
        self.assertEqual('CONNECTED', proto.state)

        transport.write(b'1')

        data = bytearray()
        def reader(data):
            chunk = os.read(rpipe, 1024)
            data += chunk
            return len(data)

        test_utils.run_until(self.loop, lambda: reader(data) >= 1)
        self.assertEqual(b'1', data)

        transport.write(b'2345')
        test_utils.run_until(self.loop, lambda: reader(data) >= 5)
        self.assertEqual(b'12345', data)
        self.assertEqual('CONNECTED', proto.state)

        os.close(rpipe)

        # extra info is available
        self.assertIsNotNone(proto.transport.get_extra_info('pipe'))

        # close connection
        proto.transport.close()
        self.loop.run_until_complete(proto.done)
        self.assertEqual('CLOSED', proto.state)

    @unittest.skipUnless(sys.platform != 'win32',
                         "Don't support pipes for Windows")
    def test_write_pipe_disconnect_on_close(self):
        rsock, wsock = socket.socketpair()
        rsock.setblocking(False)
        pipeobj = io.open(wsock.detach(), 'wb', 1024)

        proto = MyWritePipeProto(loop=self.loop)
        connect = self.loop.connect_write_pipe(lambda: proto, pipeobj)
        transport, p = self.loop.run_until_complete(connect)
        self.assertIs(p, proto)
        self.assertIs(transport, proto.transport)
        self.assertEqual('CONNECTED', proto.state)

        transport.write(b'1')
        data = self.loop.run_until_complete(self.loop.sock_recv(rsock, 1024))
        self.assertEqual(b'1', data)

        rsock.close()

        self.loop.run_until_complete(proto.done)
        self.assertEqual('CLOSED', proto.state)

    @unittest.skipUnless(sys.platform != 'win32',
                         "Don't support pipes for Windows")
    @unittest.skipUnless(hasattr(os, 'openpty'), 'need os.openpty()')
    # select, poll and kqueue don't support character devices (PTY) on Mac OS X
    # older than 10.6 (Snow Leopard)
    @support.requires_mac_ver(10, 6)
    def test_write_pty(self):
        master, slave = os.openpty()
        slave_write_obj = io.open(slave, 'wb', 0)

        proto = MyWritePipeProto(loop=self.loop)
        connect = self.loop.connect_write_pipe(lambda: proto, slave_write_obj)
        transport, p = self.loop.run_until_complete(connect)
        self.assertIs(p, proto)
        self.assertIs(transport, proto.transport)
        self.assertEqual('CONNECTED', proto.state)

        transport.write(b'1')

        data = bytearray()
        def reader(data):
            chunk = os.read(master, 1024)
            data += chunk
            return len(data)

        test_utils.run_until(self.loop, lambda: reader(data) >= 1,
                             timeout=support.SHORT_TIMEOUT)
        self.assertEqual(b'1', data)

        transport.write(b'2345')
        test_utils.run_until(self.loop, lambda: reader(data) >= 5,
                             timeout=support.SHORT_TIMEOUT)
        self.assertEqual(b'12345', data)
        self.assertEqual('CONNECTED', proto.state)

        os.close(master)

        # extra info is available
        self.assertIsNotNone(proto.transport.get_extra_info('pipe'))

        # close connection
        proto.transport.close()
        self.loop.run_until_complete(proto.done)
        self.assertEqual('CLOSED', proto.state)

    @unittest.skipUnless(sys.platform != 'win32',
                         "Don't support pipes for Windows")
    @unittest.skipUnless(hasattr(os, 'openpty'), 'need os.openpty()')
    # select, poll and kqueue don't support character devices (PTY) on Mac OS X
    # older than 10.6 (Snow Leopard)
    @support.requires_mac_ver(10, 6)
    def test_bidirectional_pty(self):
        master, read_slave = os.openpty()
        write_slave = os.dup(read_slave)
        tty.setraw(read_slave)

        slave_read_obj = io.open(read_slave, 'rb', 0)
        read_proto = MyReadPipeProto(loop=self.loop)
        read_connect = self.loop.connect_read_pipe(lambda: read_proto,
                                                   slave_read_obj)
        read_transport, p = self.loop.run_until_complete(read_connect)
        self.assertIs(p, read_proto)
        self.assertIs(read_transport, read_proto.transport)
        self.assertEqual(['INITIAL', 'CONNECTED'], read_proto.state)
        self.assertEqual(0, read_proto.nbytes)


        slave_write_obj = io.open(write_slave, 'wb', 0)
        write_proto = MyWritePipeProto(loop=self.loop)
        write_connect = self.loop.connect_write_pipe(lambda: write_proto,
                                                     slave_write_obj)
        write_transport, p = self.loop.run_until_complete(write_connect)
        self.assertIs(p, write_proto)
        self.assertIs(write_transport, write_proto.transport)
        self.assertEqual('CONNECTED', write_proto.state)

        data = bytearray()
        def reader(data):
            chunk = os.read(master, 1024)
            data += chunk
            return len(data)

        write_transport.write(b'1')
        test_utils.run_until(self.loop, lambda: reader(data) >= 1,
                             timeout=support.SHORT_TIMEOUT)
        self.assertEqual(b'1', data)
        self.assertEqual(['INITIAL', 'CONNECTED'], read_proto.state)
        self.assertEqual('CONNECTED', write_proto.state)

        os.write(master, b'a')
        test_utils.run_until(self.loop, lambda: read_proto.nbytes >= 1,
                             timeout=support.SHORT_TIMEOUT)
        self.assertEqual(['INITIAL', 'CONNECTED'], read_proto.state)
        self.assertEqual(1, read_proto.nbytes)
        self.assertEqual('CONNECTED', write_proto.state)

        write_transport.write(b'2345')
        test_utils.run_until(self.loop, lambda: reader(data) >= 5,
                             timeout=support.SHORT_TIMEOUT)
        self.assertEqual(b'12345', data)
        self.assertEqual(['INITIAL', 'CONNECTED'], read_proto.state)
        self.assertEqual('CONNECTED', write_proto.state)

        os.write(master, b'bcde')
        test_utils.run_until(self.loop, lambda: read_proto.nbytes >= 5,
                             timeout=support.SHORT_TIMEOUT)
        self.assertEqual(['INITIAL', 'CONNECTED'], read_proto.state)
        self.assertEqual(5, read_proto.nbytes)
        self.assertEqual('CONNECTED', write_proto.state)

        os.close(master)

        read_transport.close()
        self.loop.run_until_complete(read_proto.done)
        self.assertEqual(
            ['INITIAL', 'CONNECTED', 'EOF', 'CLOSED'], read_proto.state)

        write_transport.close()
        self.loop.run_until_complete(write_proto.done)
        self.assertEqual('CLOSED', write_proto.state)

    def test_prompt_cancellation(self):
        r, w = socket.socketpair()
        r.setblocking(False)
        f = self.loop.create_task(self.loop.sock_recv(r, 1))
        ov = getattr(f, 'ov', None)
        if ov is not None:
            self.assertTrue(ov.pending)

        async def main():
            try:
                self.loop.call_soon(f.cancel)
                await f
            except asyncio.CancelledError:
                res = 'cancelled'
            else:
                res = None
            finally:
                self.loop.stop()
            return res

        start = time.monotonic()
        t = self.loop.create_task(main())
        self.loop.run_forever()
        elapsed = time.monotonic() - start

        self.assertLess(elapsed, 0.1)
        self.assertEqual(t.result(), 'cancelled')
        self.assertRaises(asyncio.CancelledError, f.result)
        if ov is not None:
            self.assertFalse(ov.pending)
        self.loop._stop_serving(r)

        r.close()
        w.close()

    def test_timeout_rounding(self):
        def _run_once():
            self.loop._run_once_counter += 1
            orig_run_once()

        orig_run_once = self.loop._run_once
        self.loop._run_once_counter = 0
        self.loop._run_once = _run_once

        async def wait():
            loop = self.loop
            await asyncio.sleep(1e-2)
            await asyncio.sleep(1e-4)
            await asyncio.sleep(1e-6)
            await asyncio.sleep(1e-8)
            await asyncio.sleep(1e-10)

        self.loop.run_until_complete(wait())
        # The ideal number of call is 12, but on some platforms, the selector
        # may sleep at little bit less than timeout depending on the resolution
        # of the clock used by the kernel. Tolerate a few useless calls on
        # these platforms.
        self.assertLessEqual(self.loop._run_once_counter, 20,
            {'clock_resolution': self.loop._clock_resolution,
             'selector': self.loop._selector.__class__.__name__})

    def test_remove_fds_after_closing(self):
        loop = self.create_event_loop()
        callback = lambda: None
        r, w = socket.socketpair()
        self.addCleanup(r.close)
        self.addCleanup(w.close)
        loop.add_reader(r, callback)
        loop.add_writer(w, callback)
        loop.close()
        self.assertFalse(loop.remove_reader(r))
        self.assertFalse(loop.remove_writer(w))

    def test_add_fds_after_closing(self):
        loop = self.create_event_loop()
        callback = lambda: None
        r, w = socket.socketpair()
        self.addCleanup(r.close)
        self.addCleanup(w.close)
        loop.close()
        with self.assertRaises(RuntimeError):
            loop.add_reader(r, callback)
        with self.assertRaises(RuntimeError):
            loop.add_writer(w, callback)

    def test_close_running_event_loop(self):
        async def close_loop(loop):
            self.loop.close()

        coro = close_loop(self.loop)
        with self.assertRaises(RuntimeError):
            self.loop.run_until_complete(coro)

    def test_close(self):
        self.loop.close()

        async def test():
            pass

        func = lambda: False
        coro = test()
        self.addCleanup(coro.close)

        # operation blocked when the loop is closed
        with self.assertRaises(RuntimeError):
            self.loop.run_forever()
        with self.assertRaises(RuntimeError):
            fut = self.loop.create_future()
            self.loop.run_until_complete(fut)
        with self.assertRaises(RuntimeError):
            self.loop.call_soon(func)
        with self.assertRaises(RuntimeError):
            self.loop.call_soon_threadsafe(func)
        with self.assertRaises(RuntimeError):
            self.loop.call_later(1.0, func)
        with self.assertRaises(RuntimeError):
            self.loop.call_at(self.loop.time() + .0, func)
        with self.assertRaises(RuntimeError):
            self.loop.create_task(coro)
        with self.assertRaises(RuntimeError):
            self.loop.add_signal_handler(signal.SIGTERM, func)

        # run_in_executor test is tricky: the method is a coroutine,
        # but run_until_complete cannot be called on closed loop.
        # Thus iterate once explicitly.
        with self.assertRaises(RuntimeError):
            it = self.loop.run_in_executor(None, func).__await__()
            next(it)


class SubprocessTestsMixin:

    def check_terminated(self, returncode):
        if sys.platform == 'win32':
            self.assertIsInstance(returncode, int)
            # expect 1 but sometimes get 0
        else:
            self.assertEqual(-signal.SIGTERM, returncode)

    def check_killed(self, returncode):
        if sys.platform == 'win32':
            self.assertIsInstance(returncode, int)
            # expect 1 but sometimes get 0
        else:
            self.assertEqual(-signal.SIGKILL, returncode)

    def test_subprocess_exec(self):
        prog = os.path.join(os.path.dirname(__file__), 'echo.py')

        connect = self.loop.subprocess_exec(
                        functools.partial(MySubprocessProtocol, self.loop),
                        sys.executable, prog)

        transp, proto = self.loop.run_until_complete(connect)
        self.assertIsInstance(proto, MySubprocessProtocol)
        self.loop.run_until_complete(proto.connected)
        self.assertEqual('CONNECTED', proto.state)

        stdin = transp.get_pipe_transport(0)
        stdin.write(b'Python The Winner')
        self.loop.run_until_complete(proto.got_data[1].wait())
        with test_utils.disable_logger():
            transp.close()
        self.loop.run_until_complete(proto.completed)
        self.check_killed(proto.returncode)
        self.assertEqual(b'Python The Winner', proto.data[1])

    def test_subprocess_interactive(self):
        prog = os.path.join(os.path.dirname(__file__), 'echo.py')

        connect = self.loop.subprocess_exec(
                        functools.partial(MySubprocessProtocol, self.loop),
                        sys.executable, prog)

        transp, proto = self.loop.run_until_complete(connect)
        self.assertIsInstance(proto, MySubprocessProtocol)
        self.loop.run_until_complete(proto.connected)
        self.assertEqual('CONNECTED', proto.state)

        stdin = transp.get_pipe_transport(0)
        stdin.write(b'Python ')
        self.loop.run_until_complete(proto.got_data[1].wait())
        proto.got_data[1].clear()
        self.assertEqual(b'Python ', proto.data[1])

        stdin.write(b'The Winner')
        self.loop.run_until_complete(proto.got_data[1].wait())
        self.assertEqual(b'Python The Winner', proto.data[1])

        with test_utils.disable_logger():
            transp.close()
        self.loop.run_until_complete(proto.completed)
        self.check_killed(proto.returncode)

    def test_subprocess_shell(self):
        connect = self.loop.subprocess_shell(
                        functools.partial(MySubprocessProtocol, self.loop),
                        'echo Python')
        transp, proto = self.loop.run_until_complete(connect)
        self.assertIsInstance(proto, MySubprocessProtocol)
        self.loop.run_until_complete(proto.connected)

        transp.get_pipe_transport(0).close()
        self.loop.run_until_complete(proto.completed)
        self.assertEqual(0, proto.returncode)
        self.assertTrue(all(f.done() for f in proto.disconnects.values()))
        self.assertEqual(proto.data[1].rstrip(b'\r\n'), b'Python')
        self.assertEqual(proto.data[2], b'')
        transp.close()

    def test_subprocess_exitcode(self):
        connect = self.loop.subprocess_shell(
                        functools.partial(MySubprocessProtocol, self.loop),
                        'exit 7', stdin=None, stdout=None, stderr=None)

        transp, proto = self.loop.run_until_complete(connect)
        self.assertIsInstance(proto, MySubprocessProtocol)
        self.loop.run_until_complete(proto.completed)
        self.assertEqual(7, proto.returncode)
        transp.close()

    def test_subprocess_close_after_finish(self):
        connect = self.loop.subprocess_shell(
                        functools.partial(MySubprocessProtocol, self.loop),
                        'exit 7', stdin=None, stdout=None, stderr=None)

        transp, proto = self.loop.run_until_complete(connect)
        self.assertIsInstance(proto, MySubprocessProtocol)
        self.assertIsNone(transp.get_pipe_transport(0))
        self.assertIsNone(transp.get_pipe_transport(1))
        self.assertIsNone(transp.get_pipe_transport(2))
        self.loop.run_until_complete(proto.completed)
        self.assertEqual(7, proto.returncode)
        self.assertIsNone(transp.close())

    def test_subprocess_kill(self):
        prog = os.path.join(os.path.dirname(__file__), 'echo.py')

        connect = self.loop.subprocess_exec(
                        functools.partial(MySubprocessProtocol, self.loop),
                        sys.executable, prog)

        transp, proto = self.loop.run_until_complete(connect)
        self.assertIsInstance(proto, MySubprocessProtocol)
        self.loop.run_until_complete(proto.connected)

        transp.kill()
        self.loop.run_until_complete(proto.completed)
        self.check_killed(proto.returncode)
        transp.close()

    def test_subprocess_terminate(self):
        prog = os.path.join(os.path.dirname(__file__), 'echo.py')

        connect = self.loop.subprocess_exec(
                        functools.partial(MySubprocessProtocol, self.loop),
                        sys.executable, prog)

        transp, proto = self.loop.run_until_complete(connect)
        self.assertIsInstance(proto, MySubprocessProtocol)
        self.loop.run_until_complete(proto.connected)

        transp.terminate()
        self.loop.run_until_complete(proto.completed)
        self.check_terminated(proto.returncode)
        transp.close()

    @unittest.skipIf(sys.platform == 'win32', "Don't have SIGHUP")
    def test_subprocess_send_signal(self):
        # bpo-31034: Make sure that we get the default signal handler (killing
        # the process). The parent process may have decided to ignore SIGHUP,
        # and signal handlers are inherited.
        old_handler = signal.signal(signal.SIGHUP, signal.SIG_DFL)
        try:
            prog = os.path.join(os.path.dirname(__file__), 'echo.py')

            connect = self.loop.subprocess_exec(
                            functools.partial(MySubprocessProtocol, self.loop),
                            sys.executable, prog)


            transp, proto = self.loop.run_until_complete(connect)
            self.assertIsInstance(proto, MySubprocessProtocol)
            self.loop.run_until_complete(proto.connected)

            transp.send_signal(signal.SIGHUP)
            self.loop.run_until_complete(proto.completed)
            self.assertEqual(-signal.SIGHUP, proto.returncode)
            transp.close()
        finally:
            signal.signal(signal.SIGHUP, old_handler)

    def test_subprocess_stderr(self):
        prog = os.path.join(os.path.dirname(__file__), 'echo2.py')

        connect = self.loop.subprocess_exec(
                        functools.partial(MySubprocessProtocol, self.loop),
                        sys.executable, prog)

        transp, proto = self.loop.run_until_complete(connect)
        self.assertIsInstance(proto, MySubprocessProtocol)
        self.loop.run_until_complete(proto.connected)

        stdin = transp.get_pipe_transport(0)
        stdin.write(b'test')

        self.loop.run_until_complete(proto.completed)

        transp.close()
        self.assertEqual(b'OUT:test', proto.data[1])
        self.assertTrue(proto.data[2].startswith(b'ERR:test'), proto.data[2])
        self.assertEqual(0, proto.returncode)

    def test_subprocess_stderr_redirect_to_stdout(self):
        prog = os.path.join(os.path.dirname(__file__), 'echo2.py')

        connect = self.loop.subprocess_exec(
                        functools.partial(MySubprocessProtocol, self.loop),
                        sys.executable, prog, stderr=subprocess.STDOUT)


        transp, proto = self.loop.run_until_complete(connect)
        self.assertIsInstance(proto, MySubprocessProtocol)
        self.loop.run_until_complete(proto.connected)

        stdin = transp.get_pipe_transport(0)
        self.assertIsNotNone(transp.get_pipe_transport(1))
        self.assertIsNone(transp.get_pipe_transport(2))

        stdin.write(b'test')
        self.loop.run_until_complete(proto.completed)
        self.assertTrue(proto.data[1].startswith(b'OUT:testERR:test'),
                        proto.data[1])
        self.assertEqual(b'', proto.data[2])

        transp.close()
        self.assertEqual(0, proto.returncode)

    def test_subprocess_close_client_stream(self):
        prog = os.path.join(os.path.dirname(__file__), 'echo3.py')

        connect = self.loop.subprocess_exec(
                        functools.partial(MySubprocessProtocol, self.loop),
                        sys.executable, prog)

        transp, proto = self.loop.run_until_complete(connect)
        self.assertIsInstance(proto, MySubprocessProtocol)
        self.loop.run_until_complete(proto.connected)

        stdin = transp.get_pipe_transport(0)
        stdout = transp.get_pipe_transport(1)
        stdin.write(b'test')
        self.loop.run_until_complete(proto.got_data[1].wait())
        self.assertEqual(b'OUT:test', proto.data[1])

        stdout.close()
        self.loop.run_until_complete(proto.disconnects[1])
        stdin.write(b'xxx')
        self.loop.run_until_complete(proto.got_data[2].wait())
        if sys.platform != 'win32':
            self.assertEqual(b'ERR:BrokenPipeError', proto.data[2])
        else:
            # After closing the read-end of a pipe, writing to the
            # write-end using os.write() fails with errno==EINVAL and
            # GetLastError()==ERROR_INVALID_NAME on Windows!?!  (Using
            # WriteFile() we get ERROR_BROKEN_PIPE as expected.)
            self.assertEqual(b'ERR:OSError', proto.data[2])
        with test_utils.disable_logger():
            transp.close()
        self.loop.run_until_complete(proto.completed)
        self.check_killed(proto.returncode)

    def test_subprocess_wait_no_same_group(self):
        # start the new process in a new session
        connect = self.loop.subprocess_shell(
                        functools.partial(MySubprocessProtocol, self.loop),
                        'exit 7', stdin=None, stdout=None, stderr=None,
                        start_new_session=True)
        _, proto = yield self.loop.run_until_complete(connect)
        self.assertIsInstance(proto, MySubprocessProtocol)
        self.loop.run_until_complete(proto.completed)
        self.assertEqual(7, proto.returncode)

    def test_subprocess_exec_invalid_args(self):
        async def connect(**kwds):
            await self.loop.subprocess_exec(
                asyncio.SubprocessProtocol,
                'pwd', **kwds)

        with self.assertRaises(ValueError):
            self.loop.run_until_complete(connect(universal_newlines=True))
        with self.assertRaises(ValueError):
            self.loop.run_until_complete(connect(bufsize=4096))
        with self.assertRaises(ValueError):
            self.loop.run_until_complete(connect(shell=True))

    def test_subprocess_shell_invalid_args(self):

        async def connect(cmd=None, **kwds):
            if not cmd:
                cmd = 'pwd'
            await self.loop.subprocess_shell(
                asyncio.SubprocessProtocol,
                cmd, **kwds)

        with self.assertRaises(ValueError):
            self.loop.run_until_complete(connect(['ls', '-l']))
        with self.assertRaises(ValueError):
            self.loop.run_until_complete(connect(universal_newlines=True))
        with self.assertRaises(ValueError):
            self.loop.run_until_complete(connect(bufsize=4096))
        with self.assertRaises(ValueError):
            self.loop.run_until_complete(connect(shell=False))


if sys.platform == 'win32':

    class SelectEventLoopTests(EventLoopTestsMixin,
                               test_utils.TestCase):

        def create_event_loop(self):
            return asyncio.SelectorEventLoop()

    class ProactorEventLoopTests(EventLoopTestsMixin,
                                 SubprocessTestsMixin,
                                 test_utils.TestCase):

        def create_event_loop(self):
            return asyncio.ProactorEventLoop()

        def test_reader_callback(self):
            raise unittest.SkipTest("IocpEventLoop does not have add_reader()")

        def test_reader_callback_cancel(self):
            raise unittest.SkipTest("IocpEventLoop does not have add_reader()")

        def test_writer_callback(self):
            raise unittest.SkipTest("IocpEventLoop does not have add_writer()")

        def test_writer_callback_cancel(self):
            raise unittest.SkipTest("IocpEventLoop does not have add_writer()")

        def test_remove_fds_after_closing(self):
            raise unittest.SkipTest("IocpEventLoop does not have add_reader()")
else:
    import selectors

    class UnixEventLoopTestsMixin(EventLoopTestsMixin):
        def setUp(self):
            super().setUp()
            watcher = asyncio.SafeChildWatcher()
            watcher.attach_loop(self.loop)
            asyncio.set_child_watcher(watcher)

        def tearDown(self):
            asyncio.set_child_watcher(None)
            super().tearDown()


    if hasattr(selectors, 'KqueueSelector'):
        class KqueueEventLoopTests(UnixEventLoopTestsMixin,
                                   SubprocessTestsMixin,
                                   test_utils.TestCase):

            def create_event_loop(self):
                return asyncio.SelectorEventLoop(
                    selectors.KqueueSelector())

            # kqueue doesn't support character devices (PTY) on Mac OS X older
            # than 10.9 (Maverick)
            @support.requires_mac_ver(10, 9)
            # Issue #20667: KqueueEventLoopTests.test_read_pty_output()
            # hangs on OpenBSD 5.5
            @unittest.skipIf(sys.platform.startswith('openbsd'),
                             'test hangs on OpenBSD')
            def test_read_pty_output(self):
                super().test_read_pty_output()

            # kqueue doesn't support character devices (PTY) on Mac OS X older
            # than 10.9 (Maverick)
            @support.requires_mac_ver(10, 9)
            def test_write_pty(self):
                super().test_write_pty()

    if hasattr(selectors, 'EpollSelector'):
        class EPollEventLoopTests(UnixEventLoopTestsMixin,
                                  SubprocessTestsMixin,
                                  test_utils.TestCase):

            def create_event_loop(self):
                return asyncio.SelectorEventLoop(selectors.EpollSelector())

    if hasattr(selectors, 'PollSelector'):
        class PollEventLoopTests(UnixEventLoopTestsMixin,
                                 SubprocessTestsMixin,
                                 test_utils.TestCase):

            def create_event_loop(self):
                return asyncio.SelectorEventLoop(selectors.PollSelector())

    # Should always exist.
    class SelectEventLoopTests(UnixEventLoopTestsMixin,
                               SubprocessTestsMixin,
                               test_utils.TestCase):

        def create_event_loop(self):
            return asyncio.SelectorEventLoop(selectors.SelectSelector())


def noop(*args, **kwargs):
    pass


class HandleTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = mock.Mock()
        self.loop.get_debug.return_value = True

    def test_handle(self):
        def callback(*args):
            return args

        args = ()
        h = asyncio.Handle(callback, args, self.loop)
        self.assertIs(h._callback, callback)
        self.assertIs(h._args, args)
        self.assertFalse(h.cancelled())

        h.cancel()
        self.assertTrue(h.cancelled())

    def test_callback_with_exception(self):
        def callback():
            raise ValueError()

        self.loop = mock.Mock()
        self.loop.call_exception_handler = mock.Mock()

        h = asyncio.Handle(callback, (), self.loop)
        h._run()

        self.loop.call_exception_handler.assert_called_with({
            'message': test_utils.MockPattern('Exception in callback.*'),
            'exception': mock.ANY,
            'handle': h,
            'source_traceback': h._source_traceback,
        })

    def test_handle_weakref(self):
        wd = weakref.WeakValueDictionary()
        h = asyncio.Handle(lambda: None, (), self.loop)
        wd['h'] = h  # Would fail without __weakref__ slot.

    def test_handle_repr(self):
        self.loop.get_debug.return_value = False

        # simple function
        h = asyncio.Handle(noop, (1, 2), self.loop)
        filename, lineno = test_utils.get_function_source(noop)
        self.assertEqual(repr(h),
                        '<Handle noop(1, 2) at %s:%s>'
                        % (filename, lineno))

        # cancelled handle
        h.cancel()
        self.assertEqual(repr(h),
                        '<Handle cancelled>')

        # decorated function
        with self.assertWarns(DeprecationWarning):
            cb = asyncio.coroutine(noop)
        h = asyncio.Handle(cb, (), self.loop)
        self.assertEqual(repr(h),
                        '<Handle noop() at %s:%s>'
                        % (filename, lineno))

        # partial function
        cb = functools.partial(noop, 1, 2)
        h = asyncio.Handle(cb, (3,), self.loop)
        regex = (r'^<Handle noop\(1, 2\)\(3\) at %s:%s>$'
                 % (re.escape(filename), lineno))
        self.assertRegex(repr(h), regex)

        # partial function with keyword args
        cb = functools.partial(noop, x=1)
        h = asyncio.Handle(cb, (2, 3), self.loop)
        regex = (r'^<Handle noop\(x=1\)\(2, 3\) at %s:%s>$'
                 % (re.escape(filename), lineno))
        self.assertRegex(repr(h), regex)

        # partial method
        if sys.version_info >= (3, 4):
            method = HandleTests.test_handle_repr
            cb = functools.partialmethod(method)
            filename, lineno = test_utils.get_function_source(method)
            h = asyncio.Handle(cb, (), self.loop)

            cb_regex = r'<function HandleTests.test_handle_repr .*>'
            cb_regex = (r'functools.partialmethod\(%s, , \)\(\)' % cb_regex)
            regex = (r'^<Handle %s at %s:%s>$'
                     % (cb_regex, re.escape(filename), lineno))
            self.assertRegex(repr(h), regex)

    def test_handle_repr_debug(self):
        self.loop.get_debug.return_value = True

        # simple function
        create_filename = __file__
        create_lineno = sys._getframe().f_lineno + 1
        h = asyncio.Handle(noop, (1, 2), self.loop)
        filename, lineno = test_utils.get_function_source(noop)
        self.assertEqual(repr(h),
                        '<Handle noop(1, 2) at %s:%s created at %s:%s>'
                        % (filename, lineno, create_filename, create_lineno))

        # cancelled handle
        h.cancel()
        self.assertEqual(
            repr(h),
            '<Handle cancelled noop(1, 2) at %s:%s created at %s:%s>'
            % (filename, lineno, create_filename, create_lineno))

        # double cancellation won't overwrite _repr
        h.cancel()
        self.assertEqual(
            repr(h),
            '<Handle cancelled noop(1, 2) at %s:%s created at %s:%s>'
            % (filename, lineno, create_filename, create_lineno))

    def test_handle_source_traceback(self):
        loop = asyncio.get_event_loop_policy().new_event_loop()
        loop.set_debug(True)
        self.set_event_loop(loop)

        def check_source_traceback(h):
            lineno = sys._getframe(1).f_lineno - 1
            self.assertIsInstance(h._source_traceback, list)
            self.assertEqual(h._source_traceback[-1][:3],
                             (__file__,
                              lineno,
                              'test_handle_source_traceback'))

        # call_soon
        h = loop.call_soon(noop)
        check_source_traceback(h)

        # call_soon_threadsafe
        h = loop.call_soon_threadsafe(noop)
        check_source_traceback(h)

        # call_later
        h = loop.call_later(0, noop)
        check_source_traceback(h)

        # call_at
        h = loop.call_later(0, noop)
        check_source_traceback(h)

    @unittest.skipUnless(hasattr(collections.abc, 'Coroutine'),
                         'No collections.abc.Coroutine')
    def test_coroutine_like_object_debug_formatting(self):
        # Test that asyncio can format coroutines that are instances of
        # collections.abc.Coroutine, but lack cr_core or gi_code attributes
        # (such as ones compiled with Cython).

        coro = CoroLike()
        coro.__name__ = 'AAA'
        self.assertTrue(asyncio.iscoroutine(coro))
        self.assertEqual(coroutines._format_coroutine(coro), 'AAA()')

        coro.__qualname__ = 'BBB'
        self.assertEqual(coroutines._format_coroutine(coro), 'BBB()')

        coro.cr_running = True
        self.assertEqual(coroutines._format_coroutine(coro), 'BBB() running')

        coro.__name__ = coro.__qualname__ = None
        self.assertEqual(coroutines._format_coroutine(coro),
                         '<CoroLike without __name__>() running')

        coro = CoroLike()
        coro.__qualname__ = 'CoroLike'
        # Some coroutines might not have '__name__', such as
        # built-in async_gen.asend().
        self.assertEqual(coroutines._format_coroutine(coro), 'CoroLike()')

        coro = CoroLike()
        coro.__qualname__ = 'AAA'
        coro.cr_code = None
        self.assertEqual(coroutines._format_coroutine(coro), 'AAA()')


class TimerTests(unittest.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = mock.Mock()

    def test_hash(self):
        when = time.monotonic()
        h = asyncio.TimerHandle(when, lambda: False, (),
                                mock.Mock())
        self.assertEqual(hash(h), hash(when))

    def test_when(self):
        when = time.monotonic()
        h = asyncio.TimerHandle(when, lambda: False, (),
                                mock.Mock())
        self.assertEqual(when, h.when())

    def test_timer(self):
        def callback(*args):
            return args

        args = (1, 2, 3)
        when = time.monotonic()
        h = asyncio.TimerHandle(when, callback, args, mock.Mock())
        self.assertIs(h._callback, callback)
        self.assertIs(h._args, args)
        self.assertFalse(h.cancelled())

        # cancel
        h.cancel()
        self.assertTrue(h.cancelled())
        self.assertIsNone(h._callback)
        self.assertIsNone(h._args)

        # when cannot be None
        self.assertRaises(AssertionError,
                          asyncio.TimerHandle, None, callback, args,
                          self.loop)

    def test_timer_repr(self):
        self.loop.get_debug.return_value = False

        # simple function
        h = asyncio.TimerHandle(123, noop, (), self.loop)
        src = test_utils.get_function_source(noop)
        self.assertEqual(repr(h),
                        '<TimerHandle when=123 noop() at %s:%s>' % src)

        # cancelled handle
        h.cancel()
        self.assertEqual(repr(h),
                        '<TimerHandle cancelled when=123>')

    def test_timer_repr_debug(self):
        self.loop.get_debug.return_value = True

        # simple function
        create_filename = __file__
        create_lineno = sys._getframe().f_lineno + 1
        h = asyncio.TimerHandle(123, noop, (), self.loop)
        filename, lineno = test_utils.get_function_source(noop)
        self.assertEqual(repr(h),
                        '<TimerHandle when=123 noop() '
                        'at %s:%s created at %s:%s>'
                        % (filename, lineno, create_filename, create_lineno))

        # cancelled handle
        h.cancel()
        self.assertEqual(repr(h),
                        '<TimerHandle cancelled when=123 noop() '
                        'at %s:%s created at %s:%s>'
                        % (filename, lineno, create_filename, create_lineno))


    def test_timer_comparison(self):
        def callback(*args):
            return args

        when = time.monotonic()

        h1 = asyncio.TimerHandle(when, callback, (), self.loop)
        h2 = asyncio.TimerHandle(when, callback, (), self.loop)
        # TODO: Use assertLess etc.
        self.assertFalse(h1 < h2)
        self.assertFalse(h2 < h1)
        self.assertTrue(h1 <= h2)
        self.assertTrue(h2 <= h1)
        self.assertFalse(h1 > h2)
        self.assertFalse(h2 > h1)
        self.assertTrue(h1 >= h2)
        self.assertTrue(h2 >= h1)
        self.assertTrue(h1 == h2)
        self.assertFalse(h1 != h2)

        h2.cancel()
        self.assertFalse(h1 == h2)

        h1 = asyncio.TimerHandle(when, callback, (), self.loop)
        h2 = asyncio.TimerHandle(when + 10.0, callback, (), self.loop)
        self.assertTrue(h1 < h2)
        self.assertFalse(h2 < h1)
        self.assertTrue(h1 <= h2)
        self.assertFalse(h2 <= h1)
        self.assertFalse(h1 > h2)
        self.assertTrue(h2 > h1)
        self.assertFalse(h1 >= h2)
        self.assertTrue(h2 >= h1)
        self.assertFalse(h1 == h2)
        self.assertTrue(h1 != h2)

        h3 = asyncio.Handle(callback, (), self.loop)
        self.assertIs(NotImplemented, h1.__eq__(h3))
        self.assertIs(NotImplemented, h1.__ne__(h3))

        with self.assertRaises(TypeError):
            h1 < ()
        with self.assertRaises(TypeError):
            h1 > ()
        with self.assertRaises(TypeError):
            h1 <= ()
        with self.assertRaises(TypeError):
            h1 >= ()
        self.assertFalse(h1 == ())
        self.assertTrue(h1 != ())

        self.assertTrue(h1 == ALWAYS_EQ)
        self.assertFalse(h1 != ALWAYS_EQ)
        self.assertTrue(h1 < LARGEST)
        self.assertFalse(h1 > LARGEST)
        self.assertTrue(h1 <= LARGEST)
        self.assertFalse(h1 >= LARGEST)
        self.assertFalse(h1 < SMALLEST)
        self.assertTrue(h1 > SMALLEST)
        self.assertFalse(h1 <= SMALLEST)
        self.assertTrue(h1 >= SMALLEST)


class AbstractEventLoopTests(unittest.TestCase):

    def test_not_implemented(self):
        f = mock.Mock()
        loop = asyncio.AbstractEventLoop()
        self.assertRaises(
            NotImplementedError, loop.run_forever)
        self.assertRaises(
            NotImplementedError, loop.run_until_complete, None)
        self.assertRaises(
            NotImplementedError, loop.stop)
        self.assertRaises(
            NotImplementedError, loop.is_running)
        self.assertRaises(
            NotImplementedError, loop.is_closed)
        self.assertRaises(
            NotImplementedError, loop.close)
        self.assertRaises(
            NotImplementedError, loop.create_task, None)
        self.assertRaises(
            NotImplementedError, loop.call_later, None, None)
        self.assertRaises(
            NotImplementedError, loop.call_at, f, f)
        self.assertRaises(
            NotImplementedError, loop.call_soon, None)
        self.assertRaises(
            NotImplementedError, loop.time)
        self.assertRaises(
            NotImplementedError, loop.call_soon_threadsafe, None)
        self.assertRaises(
            NotImplementedError, loop.set_default_executor, f)
        self.assertRaises(
            NotImplementedError, loop.add_reader, 1, f)
        self.assertRaises(
            NotImplementedError, loop.remove_reader, 1)
        self.assertRaises(
            NotImplementedError, loop.add_writer, 1, f)
        self.assertRaises(
            NotImplementedError, loop.remove_writer, 1)
        self.assertRaises(
            NotImplementedError, loop.add_signal_handler, 1, f)
        self.assertRaises(
            NotImplementedError, loop.remove_signal_handler, 1)
        self.assertRaises(
            NotImplementedError, loop.remove_signal_handler, 1)
        self.assertRaises(
            NotImplementedError, loop.set_exception_handler, f)
        self.assertRaises(
            NotImplementedError, loop.default_exception_handler, f)
        self.assertRaises(
            NotImplementedError, loop.call_exception_handler, f)
        self.assertRaises(
            NotImplementedError, loop.get_debug)
        self.assertRaises(
            NotImplementedError, loop.set_debug, f)

    def test_not_implemented_async(self):

        async def inner():
            f = mock.Mock()
            loop = asyncio.AbstractEventLoop()

            with self.assertRaises(NotImplementedError):
                await loop.run_in_executor(f, f)
            with self.assertRaises(NotImplementedError):
                await loop.getaddrinfo('localhost', 8080)
            with self.assertRaises(NotImplementedError):
                await loop.getnameinfo(('localhost', 8080))
            with self.assertRaises(NotImplementedError):
                await loop.create_connection(f)
            with self.assertRaises(NotImplementedError):
                await loop.create_server(f)
            with self.assertRaises(NotImplementedError):
                await loop.create_datagram_endpoint(f)
            with self.assertRaises(NotImplementedError):
                await loop.sock_recv(f, 10)
            with self.assertRaises(NotImplementedError):
                await loop.sock_recv_into(f, 10)
            with self.assertRaises(NotImplementedError):
                await loop.sock_sendall(f, 10)
            with self.assertRaises(NotImplementedError):
                await loop.sock_connect(f, f)
            with self.assertRaises(NotImplementedError):
                await loop.sock_accept(f)
            with self.assertRaises(NotImplementedError):
                await loop.sock_sendfile(f, f)
            with self.assertRaises(NotImplementedError):
                await loop.sendfile(f, f)
            with self.assertRaises(NotImplementedError):
                await loop.connect_read_pipe(f, mock.sentinel.pipe)
            with self.assertRaises(NotImplementedError):
                await loop.connect_write_pipe(f, mock.sentinel.pipe)
            with self.assertRaises(NotImplementedError):
                await loop.subprocess_shell(f, mock.sentinel)
            with self.assertRaises(NotImplementedError):
                await loop.subprocess_exec(f)

        loop = asyncio.new_event_loop()
        loop.run_until_complete(inner())
        loop.close()


class PolicyTests(unittest.TestCase):

    def test_event_loop_policy(self):
        policy = asyncio.AbstractEventLoopPolicy()
        self.assertRaises(NotImplementedError, policy.get_event_loop)
        self.assertRaises(NotImplementedError, policy.set_event_loop, object())
        self.assertRaises(NotImplementedError, policy.new_event_loop)
        self.assertRaises(NotImplementedError, policy.get_child_watcher)
        self.assertRaises(NotImplementedError, policy.set_child_watcher,
                          object())

    def test_get_event_loop(self):
        policy = asyncio.DefaultEventLoopPolicy()
        self.assertIsNone(policy._local._loop)

        loop = policy.get_event_loop()
        self.assertIsInstance(loop, asyncio.AbstractEventLoop)

        self.assertIs(policy._local._loop, loop)
        self.assertIs(loop, policy.get_event_loop())
        loop.close()

    def test_get_event_loop_calls_set_event_loop(self):
        policy = asyncio.DefaultEventLoopPolicy()

        with mock.patch.object(
                policy, "set_event_loop",
                wraps=policy.set_event_loop) as m_set_event_loop:

            loop = policy.get_event_loop()

            # policy._local._loop must be set through .set_event_loop()
            # (the unix DefaultEventLoopPolicy needs this call to attach
            # the child watcher correctly)
            m_set_event_loop.assert_called_with(loop)

        loop.close()

    def test_get_event_loop_after_set_none(self):
        policy = asyncio.DefaultEventLoopPolicy()
        policy.set_event_loop(None)
        self.assertRaises(RuntimeError, policy.get_event_loop)

    @mock.patch('asyncio.events.threading.current_thread')
    def test_get_event_loop_thread(self, m_current_thread):

        def f():
            policy = asyncio.DefaultEventLoopPolicy()
            self.assertRaises(RuntimeError, policy.get_event_loop)

        th = threading.Thread(target=f)
        th.start()
        th.join()

    def test_new_event_loop(self):
        policy = asyncio.DefaultEventLoopPolicy()

        loop = policy.new_event_loop()
        self.assertIsInstance(loop, asyncio.AbstractEventLoop)
        loop.close()

    def test_set_event_loop(self):
        policy = asyncio.DefaultEventLoopPolicy()
        old_loop = policy.get_event_loop()

        self.assertRaises(AssertionError, policy.set_event_loop, object())

        loop = policy.new_event_loop()
        policy.set_event_loop(loop)
        self.assertIs(loop, policy.get_event_loop())
        self.assertIsNot(old_loop, policy.get_event_loop())
        loop.close()
        old_loop.close()

    def test_get_event_loop_policy(self):
        policy = asyncio.get_event_loop_policy()
        self.assertIsInstance(policy, asyncio.AbstractEventLoopPolicy)
        self.assertIs(policy, asyncio.get_event_loop_policy())

    def test_set_event_loop_policy(self):
        self.assertRaises(
            AssertionError, asyncio.set_event_loop_policy, object())

        old_policy = asyncio.get_event_loop_policy()

        policy = asyncio.DefaultEventLoopPolicy()
        asyncio.set_event_loop_policy(policy)
        self.assertIs(policy, asyncio.get_event_loop_policy())
        self.assertIsNot(policy, old_policy)


class GetEventLoopTestsMixin:

    _get_running_loop_impl = None
    _set_running_loop_impl = None
    get_running_loop_impl = None
    get_event_loop_impl = None

    def setUp(self):
        self._get_running_loop_saved = events._get_running_loop
        self._set_running_loop_saved = events._set_running_loop
        self.get_running_loop_saved = events.get_running_loop
        self.get_event_loop_saved = events.get_event_loop

        events._get_running_loop = type(self)._get_running_loop_impl
        events._set_running_loop = type(self)._set_running_loop_impl
        events.get_running_loop = type(self).get_running_loop_impl
        events.get_event_loop = type(self).get_event_loop_impl

        asyncio._get_running_loop = type(self)._get_running_loop_impl
        asyncio._set_running_loop = type(self)._set_running_loop_impl
        asyncio.get_running_loop = type(self).get_running_loop_impl
        asyncio.get_event_loop = type(self).get_event_loop_impl

        super().setUp()

        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)

        if sys.platform != 'win32':
            watcher = asyncio.SafeChildWatcher()
            watcher.attach_loop(self.loop)
            asyncio.set_child_watcher(watcher)

    def tearDown(self):
        try:
            if sys.platform != 'win32':
                asyncio.set_child_watcher(None)

            super().tearDown()
        finally:
            self.loop.close()
            asyncio.set_event_loop(None)

            events._get_running_loop = self._get_running_loop_saved
            events._set_running_loop = self._set_running_loop_saved
            events.get_running_loop = self.get_running_loop_saved
            events.get_event_loop = self.get_event_loop_saved

            asyncio._get_running_loop = self._get_running_loop_saved
            asyncio._set_running_loop = self._set_running_loop_saved
            asyncio.get_running_loop = self.get_running_loop_saved
            asyncio.get_event_loop = self.get_event_loop_saved

    if sys.platform != 'win32':

        def test_get_event_loop_new_process(self):
            # bpo-32126: The multiprocessing module used by
            # ProcessPoolExecutor is not functional when the
            # multiprocessing.synchronize module cannot be imported.
            support.skip_if_broken_multiprocessing_synchronize()

            async def main():
                pool = concurrent.futures.ProcessPoolExecutor()
                result = await self.loop.run_in_executor(
                    pool, _test_get_event_loop_new_process__sub_proc)
                pool.shutdown()
                return result

            self.assertEqual(
                self.loop.run_until_complete(main()),
                'hello')

    def test_get_event_loop_returns_running_loop(self):
        class TestError(Exception):
            pass

        class Policy(asyncio.DefaultEventLoopPolicy):
            def get_event_loop(self):
                raise TestError

        old_policy = asyncio.get_event_loop_policy()
        try:
            asyncio.set_event_loop_policy(Policy())
            loop = asyncio.new_event_loop()

            with self.assertRaises(TestError):
                asyncio.get_event_loop()
            asyncio.set_event_loop(None)
            with self.assertRaises(TestError):
                asyncio.get_event_loop()

            with self.assertRaisesRegex(RuntimeError, 'no running'):
                self.assertIs(asyncio.get_running_loop(), None)
            self.assertIs(asyncio._get_running_loop(), None)

            async def func():
                self.assertIs(asyncio.get_event_loop(), loop)
                self.assertIs(asyncio.get_running_loop(), loop)
                self.assertIs(asyncio._get_running_loop(), loop)

            loop.run_until_complete(func())

            asyncio.set_event_loop(loop)
            with self.assertRaises(TestError):
                asyncio.get_event_loop()

            asyncio.set_event_loop(None)
            with self.assertRaises(TestError):
                asyncio.get_event_loop()

        finally:
            asyncio.set_event_loop_policy(old_policy)
            if loop is not None:
                loop.close()

        with self.assertRaisesRegex(RuntimeError, 'no running'):
            self.assertIs(asyncio.get_running_loop(), None)

        self.assertIs(asyncio._get_running_loop(), None)


class TestPyGetEventLoop(GetEventLoopTestsMixin, unittest.TestCase):

    _get_running_loop_impl = events._py__get_running_loop
    _set_running_loop_impl = events._py__set_running_loop
    get_running_loop_impl = events._py_get_running_loop
    get_event_loop_impl = events._py_get_event_loop


try:
    import _asyncio  # NoQA
except ImportError:
    pass
else:

    class TestCGetEventLoop(GetEventLoopTestsMixin, unittest.TestCase):

        _get_running_loop_impl = events._c__get_running_loop
        _set_running_loop_impl = events._c__set_running_loop
        get_running_loop_impl = events._c_get_running_loop
        get_event_loop_impl = events._c_get_event_loop


class TestServer(unittest.TestCase):

    def test_get_loop(self):
        loop = asyncio.new_event_loop()
        self.addCleanup(loop.close)
        proto = MyProto(loop)
        server = loop.run_until_complete(loop.create_server(lambda: proto, '0.0.0.0', 0))
        self.assertEqual(server.get_loop(), loop)
        server.close()
        loop.run_until_complete(server.wait_closed())


class TestAbstractServer(unittest.TestCase):

    def test_close(self):
        with self.assertRaises(NotImplementedError):
            events.AbstractServer().close()

    def test_wait_closed(self):
        loop = asyncio.new_event_loop()
        self.addCleanup(loop.close)

        with self.assertRaises(NotImplementedError):
            loop.run_until_complete(events.AbstractServer().wait_closed())

    def test_get_loop(self):
        with self.assertRaises(NotImplementedError):
            events.AbstractServer().get_loop()


if __name__ == '__main__':
    unittest.main()
