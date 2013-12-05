"""Tests for events.py."""

import functools
import gc
import io
import os
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
import unittest.mock
from test import support  # find_unused_port, IPV6_ENABLED, TEST_HOME_DIR


from asyncio import futures
from asyncio import events
from asyncio import transports
from asyncio import protocols
from asyncio import selector_events
from asyncio import tasks
from asyncio import test_utils
from asyncio import locks


def data_file(filename):
    if hasattr(support, 'TEST_HOME_DIR'):
        fullname = os.path.join(support.TEST_HOME_DIR, filename)
        if os.path.isfile(fullname):
            return fullname
    fullname = os.path.join(os.path.dirname(__file__), filename)
    if os.path.isfile(fullname):
        return fullname
    raise FileNotFoundError(filename)

ONLYCERT = data_file('ssl_cert.pem')
ONLYKEY = data_file('ssl_key.pem')
SIGNED_CERTFILE = data_file('keycert3.pem')
SIGNING_CA = data_file('pycacert.pem')


class MyProto(protocols.Protocol):
    done = None

    def __init__(self, loop=None):
        self.transport = None
        self.state = 'INITIAL'
        self.nbytes = 0
        if loop is not None:
            self.done = futures.Future(loop=loop)

    def connection_made(self, transport):
        self.transport = transport
        assert self.state == 'INITIAL', self.state
        self.state = 'CONNECTED'
        transport.write(b'GET / HTTP/1.0\r\nHost: example.com\r\n\r\n')

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


class MyDatagramProto(protocols.DatagramProtocol):
    done = None

    def __init__(self, loop=None):
        self.state = 'INITIAL'
        self.nbytes = 0
        if loop is not None:
            self.done = futures.Future(loop=loop)

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


class MyReadPipeProto(protocols.Protocol):
    done = None

    def __init__(self, loop=None):
        self.state = ['INITIAL']
        self.nbytes = 0
        self.transport = None
        if loop is not None:
            self.done = futures.Future(loop=loop)

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
        assert self.state == ['INITIAL', 'CONNECTED', 'EOF'], self.state
        self.state.append('CLOSED')
        if self.done:
            self.done.set_result(None)


class MyWritePipeProto(protocols.BaseProtocol):
    done = None

    def __init__(self, loop=None):
        self.state = 'INITIAL'
        self.transport = None
        if loop is not None:
            self.done = futures.Future(loop=loop)

    def connection_made(self, transport):
        self.transport = transport
        assert self.state == 'INITIAL', self.state
        self.state = 'CONNECTED'

    def connection_lost(self, exc):
        assert self.state == 'CONNECTED', self.state
        self.state = 'CLOSED'
        if self.done:
            self.done.set_result(None)


class MySubprocessProtocol(protocols.SubprocessProtocol):

    def __init__(self, loop):
        self.state = 'INITIAL'
        self.transport = None
        self.connected = futures.Future(loop=loop)
        self.completed = futures.Future(loop=loop)
        self.disconnects = {fd: futures.Future(loop=loop) for fd in range(3)}
        self.data = {1: b'', 2: b''}
        self.returncode = None
        self.got_data = {1: locks.Event(loop=loop),
                         2: locks.Event(loop=loop)}

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
        events.set_event_loop(None)

    def tearDown(self):
        # just in case if we have transport close callbacks
        test_utils.run_briefly(self.loop)

        self.loop.close()
        gc.collect()
        super().tearDown()

    def test_run_until_complete_nesting(self):
        @tasks.coroutine
        def coro1():
            yield

        @tasks.coroutine
        def coro2():
            self.assertTrue(self.loop.is_running())
            self.loop.run_until_complete(coro1())

        self.assertRaises(
            RuntimeError, self.loop.run_until_complete, coro2())

    # Note: because of the default Windows timing granularity of
    # 15.6 msec, we use fairly long sleep times here (~100 msec).

    def test_run_until_complete(self):
        t0 = self.loop.time()
        self.loop.run_until_complete(tasks.sleep(0.1, loop=self.loop))
        t1 = self.loop.time()
        self.assertTrue(0.08 <= t1-t0 <= 0.8, t1-t0)

    def test_run_until_complete_stopped(self):
        @tasks.coroutine
        def cb():
            self.loop.stop()
            yield from tasks.sleep(0.1, loop=self.loop)
        task = cb()
        self.assertRaises(RuntimeError,
                          self.loop.run_until_complete, task)

    def test_call_later(self):
        results = []

        def callback(arg):
            results.append(arg)
            self.loop.stop()

        self.loop.call_later(0.1, callback, 'hello world')
        t0 = time.monotonic()
        self.loop.run_forever()
        t1 = time.monotonic()
        self.assertEqual(results, ['hello world'])
        self.assertTrue(0.08 <= t1-t0 <= 0.8, t1-t0)

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

    def test_reader_callback(self):
        r, w = test_utils.socketpair()
        bytes_read = []

        def reader():
            try:
                data = r.recv(1024)
            except BlockingIOError:
                # Spurious readiness notifications are possible
                # at least on Linux -- see man select.
                return
            if data:
                bytes_read.append(data)
            else:
                self.assertTrue(self.loop.remove_reader(r.fileno()))
                r.close()

        self.loop.add_reader(r.fileno(), reader)
        self.loop.call_soon(w.send, b'abc')
        test_utils.run_briefly(self.loop)
        self.loop.call_soon(w.send, b'def')
        test_utils.run_briefly(self.loop)
        self.loop.call_soon(w.close)
        self.loop.call_soon(self.loop.stop)
        self.loop.run_forever()
        self.assertEqual(b''.join(bytes_read), b'abcdef')

    def test_writer_callback(self):
        r, w = test_utils.socketpair()
        w.setblocking(False)
        self.loop.add_writer(w.fileno(), w.send, b'x'*(256*1024))
        test_utils.run_briefly(self.loop)

        def remove_writer():
            self.assertTrue(self.loop.remove_writer(w.fileno()))

        self.loop.call_soon(remove_writer)
        self.loop.call_soon(self.loop.stop)
        self.loop.run_forever()
        w.close()
        data = r.recv(256*1024)
        r.close()
        self.assertGreaterEqual(len(data), 200)

    def test_sock_client_ops(self):
        with test_utils.run_test_server() as httpd:
            sock = socket.socket()
            sock.setblocking(False)
            self.loop.run_until_complete(
                self.loop.sock_connect(sock, httpd.address))
            self.loop.run_until_complete(
                self.loop.sock_sendall(sock, b'GET / HTTP/1.0\r\n\r\n'))
            data = self.loop.run_until_complete(
                self.loop.sock_recv(sock, 1024))
            # consume data
            self.loop.run_until_complete(
                self.loop.sock_recv(sock, 1024))
            sock.close()

        self.assertTrue(data.startswith(b'HTTP/1.0 200 OK'))

    def test_sock_client_fail(self):
        # Make sure that we will get an unused port
        address = None
        try:
            s = socket.socket()
            s.bind(('127.0.0.1', 0))
            address = s.getsockname()
        finally:
            s.close()

        sock = socket.socket()
        sock.setblocking(False)
        with self.assertRaises(ConnectionRefusedError):
            self.loop.run_until_complete(
                self.loop.sock_connect(sock, address))
        sock.close()

    def test_sock_accept(self):
        listener = socket.socket()
        listener.setblocking(False)
        listener.bind(('127.0.0.1', 0))
        listener.listen(1)
        client = socket.socket()
        client.connect(listener.getsockname())

        f = self.loop.sock_accept(listener)
        conn, addr = self.loop.run_until_complete(f)
        self.assertEqual(conn.gettimeout(), 0)
        self.assertEqual(addr, client.getsockname())
        self.assertEqual(client.getpeername(), listener.getsockname())
        client.close()
        conn.close()
        listener.close()

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
        test_utils.run_briefly(self.loop)
        os.kill(os.getpid(), signal.SIGINT)
        test_utils.run_briefly(self.loop)
        self.assertEqual(caught, 1)
        # Removing it should restore the default handler.
        self.assertTrue(self.loop.remove_signal_handler(signal.SIGINT))
        self.assertEqual(signal.getsignal(signal.SIGINT),
                         signal.default_int_handler)
        # Removing again returns False.
        self.assertFalse(self.loop.remove_signal_handler(signal.SIGINT))

    @unittest.skipUnless(hasattr(signal, 'SIGALRM'), 'No SIGALRM')
    def test_signal_handling_while_selecting(self):
        # Test with a signal actually arriving during a select() call.
        caught = 0

        def my_handler():
            nonlocal caught
            caught += 1
            self.loop.stop()

        self.loop.add_signal_handler(signal.SIGALRM, my_handler)

        signal.setitimer(signal.ITIMER_REAL, 0.01, 0)  # Send SIGALRM once.
        self.loop.run_forever()
        self.assertEqual(caught, 1)

    @unittest.skipUnless(hasattr(signal, 'SIGALRM'), 'No SIGALRM')
    def test_signal_handling_args(self):
        some_args = (42,)
        caught = 0

        def my_handler(*args):
            nonlocal caught
            caught += 1
            self.assertEqual(args, some_args)

        self.loop.add_signal_handler(signal.SIGALRM, my_handler, *some_args)

        signal.setitimer(signal.ITIMER_REAL, 0.1, 0)  # Send SIGALRM once.
        self.loop.call_later(0.5, self.loop.stop)
        self.loop.run_forever()
        self.assertEqual(caught, 1)

    def test_create_connection(self):
        with test_utils.run_test_server() as httpd:
            f = self.loop.create_connection(
                lambda: MyProto(loop=self.loop), *httpd.address)
            tr, pr = self.loop.run_until_complete(f)
            self.assertIsInstance(tr, transports.Transport)
            self.assertIsInstance(pr, protocols.Protocol)
            self.loop.run_until_complete(pr.done)
            self.assertGreater(pr.nbytes, 0)
            tr.close()

    def test_create_connection_sock(self):
        with test_utils.run_test_server() as httpd:
            sock = None
            infos = self.loop.run_until_complete(
                self.loop.getaddrinfo(
                    *httpd.address, type=socket.SOCK_STREAM))
            for family, type, proto, cname, address in infos:
                try:
                    sock = socket.socket(family=family, type=type, proto=proto)
                    sock.setblocking(False)
                    self.loop.run_until_complete(
                        self.loop.sock_connect(sock, address))
                except:
                    pass
                else:
                    break
            else:
                assert False, 'Can not create socket.'

            f = self.loop.create_connection(
                lambda: MyProto(loop=self.loop), sock=sock)
            tr, pr = self.loop.run_until_complete(f)
            self.assertIsInstance(tr, transports.Transport)
            self.assertIsInstance(pr, protocols.Protocol)
            self.loop.run_until_complete(pr.done)
            self.assertGreater(pr.nbytes, 0)
            tr.close()

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_ssl_connection(self):
        with test_utils.run_test_server(use_ssl=True) as httpd:
            f = self.loop.create_connection(
                lambda: MyProto(loop=self.loop), *httpd.address,
                ssl=test_utils.dummy_ssl_context())
            tr, pr = self.loop.run_until_complete(f)
            self.assertIsInstance(tr, transports.Transport)
            self.assertIsInstance(pr, protocols.Protocol)
            self.assertTrue('ssl' in tr.__class__.__name__.lower())
            self.assertIsNotNone(tr.get_extra_info('sockname'))
            self.loop.run_until_complete(pr.done)
            self.assertGreater(pr.nbytes, 0)
            tr.close()

    def test_create_connection_local_addr(self):
        with test_utils.run_test_server() as httpd:
            port = support.find_unused_port()
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

    def test_create_server(self):
        proto = None

        def factory():
            nonlocal proto
            proto = MyProto()
            return proto

        f = self.loop.create_server(factory, '0.0.0.0', 0)
        server = self.loop.run_until_complete(f)
        self.assertEqual(len(server.sockets), 1)
        sock = server.sockets[0]
        host, port = sock.getsockname()
        self.assertEqual(host, '0.0.0.0')
        client = socket.socket()
        client.connect(('127.0.0.1', port))
        client.sendall(b'xxx')
        test_utils.run_briefly(self.loop)
        test_utils.run_until(self.loop, lambda: proto is not None, 10)
        self.assertIsInstance(proto, MyProto)
        self.assertEqual('INITIAL', proto.state)
        test_utils.run_briefly(self.loop)
        self.assertEqual('CONNECTED', proto.state)
        test_utils.run_until(self.loop, lambda: proto.nbytes > 0,
                             timeout=10)
        self.assertEqual(3, proto.nbytes)

        # extra info is available
        self.assertIsNotNone(proto.transport.get_extra_info('sockname'))
        self.assertEqual('127.0.0.1',
                         proto.transport.get_extra_info('peername')[0])

        # close connection
        proto.transport.close()
        test_utils.run_briefly(self.loop)  # windows iocp

        self.assertEqual('CLOSED', proto.state)

        # the client socket must be closed after to avoid ECONNRESET upon
        # recv()/send() on the serving socket
        client.close()

        # close server
        server.close()

    def _make_ssl_server(self, factory, certfile, keyfile=None):
        sslcontext = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
        sslcontext.options |= ssl.OP_NO_SSLv2
        sslcontext.load_cert_chain(certfile, keyfile)

        f = self.loop.create_server(
            factory, '127.0.0.1', 0, ssl=sslcontext)

        server = self.loop.run_until_complete(f)
        sock = server.sockets[0]
        host, port = sock.getsockname()
        self.assertEqual(host, '127.0.0.1')
        return server, host, port

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_server_ssl(self):
        proto = None

        class ClientMyProto(MyProto):
            def connection_made(self, transport):
                self.transport = transport
                assert self.state == 'INITIAL', self.state
                self.state = 'CONNECTED'

        def factory():
            nonlocal proto
            proto = MyProto(loop=self.loop)
            return proto

        server, host, port = self._make_ssl_server(factory, ONLYCERT, ONLYKEY)

        f_c = self.loop.create_connection(ClientMyProto, host, port,
                                          ssl=test_utils.dummy_ssl_context())
        client, pr = self.loop.run_until_complete(f_c)

        client.write(b'xxx')
        test_utils.run_briefly(self.loop)
        self.assertIsInstance(proto, MyProto)
        test_utils.run_briefly(self.loop)
        self.assertEqual('CONNECTED', proto.state)
        test_utils.run_until(self.loop, lambda: proto.nbytes > 0,
                             timeout=10)
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

        # stop serving
        server.close()

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_server_ssl_verify_failed(self):
        proto = None

        def factory():
            nonlocal proto
            proto = MyProto(loop=self.loop)
            return proto

        server, host, port = self._make_ssl_server(factory, SIGNED_CERTFILE)

        sslcontext_client = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
        sslcontext_client.options |= ssl.OP_NO_SSLv2
        sslcontext_client.verify_mode = ssl.CERT_REQUIRED
        if hasattr(sslcontext_client, 'check_hostname'):
            sslcontext_client.check_hostname = True

        # no CA loaded
        f_c = self.loop.create_connection(MyProto, host, port,
                                          ssl=sslcontext_client)
        with self.assertRaisesRegex(ssl.SSLError,
                                    'certificate verify failed '):
            self.loop.run_until_complete(f_c)

        # close connection
        self.assertIsNone(proto.transport)
        server.close()

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_server_ssl_match_failed(self):
        proto = None

        def factory():
            nonlocal proto
            proto = MyProto(loop=self.loop)
            return proto

        server, host, port = self._make_ssl_server(factory, SIGNED_CERTFILE)

        sslcontext_client = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
        sslcontext_client.options |= ssl.OP_NO_SSLv2
        sslcontext_client.verify_mode = ssl.CERT_REQUIRED
        sslcontext_client.load_verify_locations(
            cafile=SIGNING_CA)
        if hasattr(sslcontext_client, 'check_hostname'):
            sslcontext_client.check_hostname = True

        # incorrect server_hostname
        f_c = self.loop.create_connection(MyProto, host, port,
                                          ssl=sslcontext_client)
        with self.assertRaisesRegex(ssl.CertificateError,
                "hostname '127.0.0.1' doesn't match 'localhost'"):
            self.loop.run_until_complete(f_c)

        # close connection
        proto.transport.close()
        server.close()

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_create_server_ssl_verified(self):
        proto = None

        def factory():
            nonlocal proto
            proto = MyProto(loop=self.loop)
            return proto

        server, host, port = self._make_ssl_server(factory, SIGNED_CERTFILE)

        sslcontext_client = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
        sslcontext_client.options |= ssl.OP_NO_SSLv2
        sslcontext_client.verify_mode = ssl.CERT_REQUIRED
        sslcontext_client.load_verify_locations(cafile=SIGNING_CA)
        if hasattr(sslcontext_client, 'check_hostname'):
            sslcontext_client.check_hostname = True

        # Connection succeeds with correct CA and server hostname.
        f_c = self.loop.create_connection(MyProto, host, port,
                                          ssl=sslcontext_client,
                                          server_hostname='localhost')
        client, pr = self.loop.run_until_complete(f_c)

        # close connection
        proto.transport.close()
        client.close()
        server.close()

    def test_create_server_sock(self):
        proto = futures.Future(loop=self.loop)

        class TestMyProto(MyProto):
            def connection_made(self, transport):
                super().connection_made(transport)
                proto.set_result(self)

        sock_ob = socket.socket(type=socket.SOCK_STREAM)
        sock_ob.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock_ob.bind(('0.0.0.0', 0))

        f = self.loop.create_server(TestMyProto, sock=sock_ob)
        server = self.loop.run_until_complete(f)
        sock = server.sockets[0]
        self.assertIs(sock, sock_ob)

        host, port = sock.getsockname()
        self.assertEqual(host, '0.0.0.0')
        client = socket.socket()
        client.connect(('127.0.0.1', port))
        client.send(b'xxx')
        client.close()
        server.close()

    def test_create_server_addr_in_use(self):
        sock_ob = socket.socket(type=socket.SOCK_STREAM)
        sock_ob.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock_ob.bind(('0.0.0.0', 0))

        f = self.loop.create_server(MyProto, sock=sock_ob)
        server = self.loop.run_until_complete(f)
        sock = server.sockets[0]
        host, port = sock.getsockname()

        f = self.loop.create_server(MyProto, host=host, port=port)
        with self.assertRaises(OSError) as cm:
            self.loop.run_until_complete(f)
        self.assertEqual(cm.exception.errno, errno.EADDRINUSE)

        server.close()

    @unittest.skipUnless(support.IPV6_ENABLED, 'IPv6 not supported or enabled')
    def test_create_server_dual_stack(self):
        f_proto = futures.Future(loop=self.loop)

        class TestMyProto(MyProto):
            def connection_made(self, transport):
                super().connection_made(transport)
                f_proto.set_result(self)

        try_count = 0
        while True:
            try:
                port = support.find_unused_port()
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

        f_proto = futures.Future(loop=self.loop)
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

    def test_create_datagram_endpoint(self):
        class TestMyDatagramProto(MyDatagramProto):
            def __init__(inner_self):
                super().__init__(loop=self.loop)

            def datagram_received(self, data, addr):
                super().datagram_received(data, addr)
                self.transport.sendto(b'resp:'+data, addr)

        coro = self.loop.create_datagram_endpoint(
            TestMyDatagramProto, local_addr=('127.0.0.1', 0))
        s_transport, server = self.loop.run_until_complete(coro)
        host, port = s_transport.get_extra_info('sockname')

        coro = self.loop.create_datagram_endpoint(
            lambda: MyDatagramProto(loop=self.loop),
            remote_addr=(host, port))
        transport, client = self.loop.run_until_complete(coro)

        self.assertEqual('INITIALIZED', client.state)
        transport.sendto(b'xxx')
        for _ in range(1000):
            if server.nbytes:
                break
            test_utils.run_briefly(self.loop)
        self.assertEqual(3, server.nbytes)
        for _ in range(1000):
            if client.nbytes:
                break
            test_utils.run_briefly(self.loop)

        # received
        self.assertEqual(8, client.nbytes)

        # extra info is available
        self.assertIsNotNone(transport.get_extra_info('sockname'))

        # close connection
        transport.close()
        self.loop.run_until_complete(client.done)
        self.assertEqual('CLOSED', client.state)
        server.transport.close()

    def test_internal_fds(self):
        loop = self.create_event_loop()
        if not isinstance(loop, selector_events.BaseSelectorEventLoop):
            return

        self.assertEqual(1, loop._internal_fds)
        loop.close()
        self.assertEqual(0, loop._internal_fds)
        self.assertIsNone(loop._csock)
        self.assertIsNone(loop._ssock)

    @unittest.skipUnless(sys.platform != 'win32',
                         "Don't support pipes for Windows")
    def test_read_pipe(self):
        proto = None

        def factory():
            nonlocal proto
            proto = MyReadPipeProto(loop=self.loop)
            return proto

        rpipe, wpipe = os.pipe()
        pipeobj = io.open(rpipe, 'rb', 1024)

        @tasks.coroutine
        def connect():
            t, p = yield from self.loop.connect_read_pipe(factory, pipeobj)
            self.assertIs(p, proto)
            self.assertIs(t, proto.transport)
            self.assertEqual(['INITIAL', 'CONNECTED'], proto.state)
            self.assertEqual(0, proto.nbytes)

        self.loop.run_until_complete(connect())

        os.write(wpipe, b'1')
        test_utils.run_briefly(self.loop)
        self.assertEqual(1, proto.nbytes)

        os.write(wpipe, b'2345')
        test_utils.run_briefly(self.loop)
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
    def test_write_pipe(self):
        proto = None
        transport = None

        def factory():
            nonlocal proto
            proto = MyWritePipeProto(loop=self.loop)
            return proto

        rpipe, wpipe = os.pipe()
        pipeobj = io.open(wpipe, 'wb', 1024)

        @tasks.coroutine
        def connect():
            nonlocal transport
            t, p = yield from self.loop.connect_write_pipe(factory, pipeobj)
            self.assertIs(p, proto)
            self.assertIs(t, proto.transport)
            self.assertEqual('CONNECTED', proto.state)
            transport = t

        self.loop.run_until_complete(connect())

        transport.write(b'1')
        test_utils.run_briefly(self.loop)
        data = os.read(rpipe, 1024)
        self.assertEqual(b'1', data)

        transport.write(b'2345')
        test_utils.run_briefly(self.loop)
        data = os.read(rpipe, 1024)
        self.assertEqual(b'2345', data)
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
        proto = None
        transport = None

        def factory():
            nonlocal proto
            proto = MyWritePipeProto(loop=self.loop)
            return proto

        rsock, wsock = test_utils.socketpair()
        pipeobj = io.open(wsock.detach(), 'wb', 1024)

        @tasks.coroutine
        def connect():
            nonlocal transport
            t, p = yield from self.loop.connect_write_pipe(factory,
                                                           pipeobj)
            self.assertIs(p, proto)
            self.assertIs(t, proto.transport)
            self.assertEqual('CONNECTED', proto.state)
            transport = t

        self.loop.run_until_complete(connect())
        self.assertEqual('CONNECTED', proto.state)

        transport.write(b'1')
        data = self.loop.run_until_complete(self.loop.sock_recv(rsock, 1024))
        self.assertEqual(b'1', data)

        rsock.close()

        self.loop.run_until_complete(proto.done)
        self.assertEqual('CLOSED', proto.state)

    def test_prompt_cancellation(self):
        r, w = test_utils.socketpair()
        r.setblocking(False)
        f = self.loop.sock_recv(r, 1)
        ov = getattr(f, 'ov', None)
        if ov is not None:
            self.assertTrue(ov.pending)

        @tasks.coroutine
        def main():
            try:
                self.loop.call_soon(f.cancel)
                yield from f
            except futures.CancelledError:
                res = 'cancelled'
            else:
                res = None
            finally:
                self.loop.stop()
            return res

        start = time.monotonic()
        t = tasks.Task(main(), loop=self.loop)
        self.loop.run_forever()
        elapsed = time.monotonic() - start

        self.assertLess(elapsed, 0.1)
        self.assertEqual(t.result(), 'cancelled')
        self.assertRaises(futures.CancelledError, f.result)
        if ov is not None:
            self.assertFalse(ov.pending)
        self.loop._stop_serving(r)

        r.close()
        w.close()


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
        proto = None
        transp = None

        prog = os.path.join(os.path.dirname(__file__), 'echo.py')

        @tasks.coroutine
        def connect():
            nonlocal proto, transp
            transp, proto = yield from self.loop.subprocess_exec(
                functools.partial(MySubprocessProtocol, self.loop),
                sys.executable, prog)
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
        self.loop.run_until_complete(proto.connected)
        self.assertEqual('CONNECTED', proto.state)

        stdin = transp.get_pipe_transport(0)
        stdin.write(b'Python The Winner')
        self.loop.run_until_complete(proto.got_data[1].wait())
        transp.close()
        self.loop.run_until_complete(proto.completed)
        self.check_terminated(proto.returncode)
        self.assertEqual(b'Python The Winner', proto.data[1])

    def test_subprocess_interactive(self):
        proto = None
        transp = None

        prog = os.path.join(os.path.dirname(__file__), 'echo.py')

        @tasks.coroutine
        def connect():
            nonlocal proto, transp
            transp, proto = yield from self.loop.subprocess_exec(
                functools.partial(MySubprocessProtocol, self.loop),
                sys.executable, prog)
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
        self.loop.run_until_complete(proto.connected)
        self.assertEqual('CONNECTED', proto.state)

        try:
            stdin = transp.get_pipe_transport(0)
            stdin.write(b'Python ')
            self.loop.run_until_complete(proto.got_data[1].wait())
            proto.got_data[1].clear()
            self.assertEqual(b'Python ', proto.data[1])

            stdin.write(b'The Winner')
            self.loop.run_until_complete(proto.got_data[1].wait())
            self.assertEqual(b'Python The Winner', proto.data[1])
        finally:
            transp.close()

        self.loop.run_until_complete(proto.completed)
        self.check_terminated(proto.returncode)

    def test_subprocess_shell(self):
        proto = None
        transp = None

        @tasks.coroutine
        def connect():
            nonlocal proto, transp
            transp, proto = yield from self.loop.subprocess_shell(
                functools.partial(MySubprocessProtocol, self.loop),
                'echo Python')
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
        self.loop.run_until_complete(proto.connected)

        transp.get_pipe_transport(0).close()
        self.loop.run_until_complete(proto.completed)
        self.assertEqual(0, proto.returncode)
        self.assertTrue(all(f.done() for f in proto.disconnects.values()))
        self.assertEqual(proto.data[1].rstrip(b'\r\n'), b'Python')
        self.assertEqual(proto.data[2], b'')

    def test_subprocess_exitcode(self):
        proto = None

        @tasks.coroutine
        def connect():
            nonlocal proto
            transp, proto = yield from self.loop.subprocess_shell(
                functools.partial(MySubprocessProtocol, self.loop),
                'exit 7', stdin=None, stdout=None, stderr=None)
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
        self.loop.run_until_complete(proto.completed)
        self.assertEqual(7, proto.returncode)

    def test_subprocess_close_after_finish(self):
        proto = None
        transp = None

        @tasks.coroutine
        def connect():
            nonlocal proto, transp
            transp, proto = yield from self.loop.subprocess_shell(
                functools.partial(MySubprocessProtocol, self.loop),
                'exit 7', stdin=None, stdout=None, stderr=None)
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
        self.assertIsNone(transp.get_pipe_transport(0))
        self.assertIsNone(transp.get_pipe_transport(1))
        self.assertIsNone(transp.get_pipe_transport(2))
        self.loop.run_until_complete(proto.completed)
        self.assertEqual(7, proto.returncode)
        self.assertIsNone(transp.close())

    def test_subprocess_kill(self):
        proto = None
        transp = None

        prog = os.path.join(os.path.dirname(__file__), 'echo.py')

        @tasks.coroutine
        def connect():
            nonlocal proto, transp
            transp, proto = yield from self.loop.subprocess_exec(
                functools.partial(MySubprocessProtocol, self.loop),
                sys.executable, prog)
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
        self.loop.run_until_complete(proto.connected)

        transp.kill()
        self.loop.run_until_complete(proto.completed)
        self.check_killed(proto.returncode)

    def test_subprocess_terminate(self):
        proto = None
        transp = None

        prog = os.path.join(os.path.dirname(__file__), 'echo.py')

        @tasks.coroutine
        def connect():
            nonlocal proto, transp
            transp, proto = yield from self.loop.subprocess_exec(
                functools.partial(MySubprocessProtocol, self.loop),
                sys.executable, prog)
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
        self.loop.run_until_complete(proto.connected)

        transp.terminate()
        self.loop.run_until_complete(proto.completed)
        self.check_terminated(proto.returncode)

    @unittest.skipIf(sys.platform == 'win32', "Don't have SIGHUP")
    def test_subprocess_send_signal(self):
        proto = None
        transp = None

        prog = os.path.join(os.path.dirname(__file__), 'echo.py')

        @tasks.coroutine
        def connect():
            nonlocal proto, transp
            transp, proto = yield from self.loop.subprocess_exec(
                functools.partial(MySubprocessProtocol, self.loop),
                sys.executable, prog)
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
        self.loop.run_until_complete(proto.connected)

        transp.send_signal(signal.SIGHUP)
        self.loop.run_until_complete(proto.completed)
        self.assertEqual(-signal.SIGHUP, proto.returncode)

    def test_subprocess_stderr(self):
        proto = None
        transp = None

        prog = os.path.join(os.path.dirname(__file__), 'echo2.py')

        @tasks.coroutine
        def connect():
            nonlocal proto, transp
            transp, proto = yield from self.loop.subprocess_exec(
                functools.partial(MySubprocessProtocol, self.loop),
                sys.executable, prog)
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
        self.loop.run_until_complete(proto.connected)

        stdin = transp.get_pipe_transport(0)
        stdin.write(b'test')

        self.loop.run_until_complete(proto.completed)

        transp.close()
        self.assertEqual(b'OUT:test', proto.data[1])
        self.assertTrue(proto.data[2].startswith(b'ERR:test'), proto.data[2])
        self.assertEqual(0, proto.returncode)

    def test_subprocess_stderr_redirect_to_stdout(self):
        proto = None
        transp = None

        prog = os.path.join(os.path.dirname(__file__), 'echo2.py')

        @tasks.coroutine
        def connect():
            nonlocal proto, transp
            transp, proto = yield from self.loop.subprocess_exec(
                functools.partial(MySubprocessProtocol, self.loop),
                sys.executable, prog, stderr=subprocess.STDOUT)
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
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
        proto = None
        transp = None

        prog = os.path.join(os.path.dirname(__file__), 'echo3.py')

        @tasks.coroutine
        def connect():
            nonlocal proto, transp
            transp, proto = yield from self.loop.subprocess_exec(
                functools.partial(MySubprocessProtocol, self.loop),
                sys.executable, prog)
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
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
        transp.close()
        self.loop.run_until_complete(proto.completed)
        self.check_terminated(proto.returncode)

    def test_subprocess_wait_no_same_group(self):
        proto = None
        transp = None

        @tasks.coroutine
        def connect():
            nonlocal proto
            # start the new process in a new session
            transp, proto = yield from self.loop.subprocess_shell(
                functools.partial(MySubprocessProtocol, self.loop),
                'exit 7', stdin=None, stdout=None, stderr=None,
                start_new_session=True)
            self.assertIsInstance(proto, MySubprocessProtocol)

        self.loop.run_until_complete(connect())
        self.loop.run_until_complete(proto.completed)
        self.assertEqual(7, proto.returncode)


if sys.platform == 'win32':
    from asyncio import windows_events

    class SelectEventLoopTests(EventLoopTestsMixin, unittest.TestCase):

        def create_event_loop(self):
            return windows_events.SelectorEventLoop()

    class ProactorEventLoopTests(EventLoopTestsMixin,
                                 SubprocessTestsMixin,
                                 unittest.TestCase):

        def create_event_loop(self):
            return windows_events.ProactorEventLoop()

        def test_create_ssl_connection(self):
            raise unittest.SkipTest("IocpEventLoop imcompatible with SSL")

        def test_create_server_ssl(self):
            raise unittest.SkipTest("IocpEventLoop imcompatible with SSL")

        def test_reader_callback(self):
            raise unittest.SkipTest("IocpEventLoop does not have add_reader()")

        def test_reader_callback_cancel(self):
            raise unittest.SkipTest("IocpEventLoop does not have add_reader()")

        def test_writer_callback(self):
            raise unittest.SkipTest("IocpEventLoop does not have add_writer()")

        def test_writer_callback_cancel(self):
            raise unittest.SkipTest("IocpEventLoop does not have add_writer()")

        def test_create_datagram_endpoint(self):
            raise unittest.SkipTest(
                "IocpEventLoop does not have create_datagram_endpoint()")
else:
    from asyncio import selectors
    from asyncio import unix_events

    class UnixEventLoopTestsMixin(EventLoopTestsMixin):
        def setUp(self):
            super().setUp()
            watcher = unix_events.SafeChildWatcher()
            watcher.attach_loop(self.loop)
            events.set_child_watcher(watcher)

        def tearDown(self):
            events.set_child_watcher(None)
            super().tearDown()

    if hasattr(selectors, 'KqueueSelector'):
        class KqueueEventLoopTests(UnixEventLoopTestsMixin,
                                   SubprocessTestsMixin,
                                   unittest.TestCase):

            def create_event_loop(self):
                return unix_events.SelectorEventLoop(
                    selectors.KqueueSelector())

    if hasattr(selectors, 'EpollSelector'):
        class EPollEventLoopTests(UnixEventLoopTestsMixin,
                                  SubprocessTestsMixin,
                                  unittest.TestCase):

            def create_event_loop(self):
                return unix_events.SelectorEventLoop(selectors.EpollSelector())

    if hasattr(selectors, 'PollSelector'):
        class PollEventLoopTests(UnixEventLoopTestsMixin,
                                 SubprocessTestsMixin,
                                 unittest.TestCase):

            def create_event_loop(self):
                return unix_events.SelectorEventLoop(selectors.PollSelector())

    # Should always exist.
    class SelectEventLoopTests(UnixEventLoopTestsMixin,
                               SubprocessTestsMixin,
                               unittest.TestCase):

        def create_event_loop(self):
            return unix_events.SelectorEventLoop(selectors.SelectSelector())


class HandleTests(unittest.TestCase):

    def test_handle(self):
        def callback(*args):
            return args

        args = ()
        h = events.Handle(callback, args)
        self.assertIs(h._callback, callback)
        self.assertIs(h._args, args)
        self.assertFalse(h._cancelled)

        r = repr(h)
        self.assertTrue(r.startswith(
            'Handle('
            '<function HandleTests.test_handle.<locals>.callback'))
        self.assertTrue(r.endswith('())'))

        h.cancel()
        self.assertTrue(h._cancelled)

        r = repr(h)
        self.assertTrue(r.startswith(
            'Handle('
            '<function HandleTests.test_handle.<locals>.callback'))
        self.assertTrue(r.endswith('())<cancelled>'), r)

    def test_make_handle(self):
        def callback(*args):
            return args
        h1 = events.Handle(callback, ())
        self.assertRaises(
            AssertionError, events.make_handle, h1, ())

    @unittest.mock.patch('asyncio.events.logger')
    def test_callback_with_exception(self, log):
        def callback():
            raise ValueError()

        h = events.Handle(callback, ())
        h._run()
        self.assertTrue(log.exception.called)


class TimerTests(unittest.TestCase):

    def test_hash(self):
        when = time.monotonic()
        h = events.TimerHandle(when, lambda: False, ())
        self.assertEqual(hash(h), hash(when))

    def test_timer(self):
        def callback(*args):
            return args

        args = ()
        when = time.monotonic()
        h = events.TimerHandle(when, callback, args)
        self.assertIs(h._callback, callback)
        self.assertIs(h._args, args)
        self.assertFalse(h._cancelled)

        r = repr(h)
        self.assertTrue(r.endswith('())'))

        h.cancel()
        self.assertTrue(h._cancelled)

        r = repr(h)
        self.assertTrue(r.endswith('())<cancelled>'), r)

        self.assertRaises(AssertionError,
                          events.TimerHandle, None, callback, args)

    def test_timer_comparison(self):
        def callback(*args):
            return args

        when = time.monotonic()

        h1 = events.TimerHandle(when, callback, ())
        h2 = events.TimerHandle(when, callback, ())
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

        h1 = events.TimerHandle(when, callback, ())
        h2 = events.TimerHandle(when + 10.0, callback, ())
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

        h3 = events.Handle(callback, ())
        self.assertIs(NotImplemented, h1.__eq__(h3))
        self.assertIs(NotImplemented, h1.__ne__(h3))


class AbstractEventLoopTests(unittest.TestCase):

    def test_not_implemented(self):
        f = unittest.mock.Mock()
        loop = events.AbstractEventLoop()
        self.assertRaises(
            NotImplementedError, loop.run_forever)
        self.assertRaises(
            NotImplementedError, loop.run_until_complete, None)
        self.assertRaises(
            NotImplementedError, loop.stop)
        self.assertRaises(
            NotImplementedError, loop.is_running)
        self.assertRaises(
            NotImplementedError, loop.close)
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
            NotImplementedError, loop.run_in_executor, f, f)
        self.assertRaises(
            NotImplementedError, loop.set_default_executor, f)
        self.assertRaises(
            NotImplementedError, loop.getaddrinfo, 'localhost', 8080)
        self.assertRaises(
            NotImplementedError, loop.getnameinfo, ('localhost', 8080))
        self.assertRaises(
            NotImplementedError, loop.create_connection, f)
        self.assertRaises(
            NotImplementedError, loop.create_server, f)
        self.assertRaises(
            NotImplementedError, loop.create_datagram_endpoint, f)
        self.assertRaises(
            NotImplementedError, loop.add_reader, 1, f)
        self.assertRaises(
            NotImplementedError, loop.remove_reader, 1)
        self.assertRaises(
            NotImplementedError, loop.add_writer, 1, f)
        self.assertRaises(
            NotImplementedError, loop.remove_writer, 1)
        self.assertRaises(
            NotImplementedError, loop.sock_recv, f, 10)
        self.assertRaises(
            NotImplementedError, loop.sock_sendall, f, 10)
        self.assertRaises(
            NotImplementedError, loop.sock_connect, f, f)
        self.assertRaises(
            NotImplementedError, loop.sock_accept, f)
        self.assertRaises(
            NotImplementedError, loop.add_signal_handler, 1, f)
        self.assertRaises(
            NotImplementedError, loop.remove_signal_handler, 1)
        self.assertRaises(
            NotImplementedError, loop.remove_signal_handler, 1)
        self.assertRaises(
            NotImplementedError, loop.connect_read_pipe, f,
            unittest.mock.sentinel.pipe)
        self.assertRaises(
            NotImplementedError, loop.connect_write_pipe, f,
            unittest.mock.sentinel.pipe)
        self.assertRaises(
            NotImplementedError, loop.subprocess_shell, f,
            unittest.mock.sentinel)
        self.assertRaises(
            NotImplementedError, loop.subprocess_exec, f)


class ProtocolsAbsTests(unittest.TestCase):

    def test_empty(self):
        f = unittest.mock.Mock()
        p = protocols.Protocol()
        self.assertIsNone(p.connection_made(f))
        self.assertIsNone(p.connection_lost(f))
        self.assertIsNone(p.data_received(f))
        self.assertIsNone(p.eof_received())

        dp = protocols.DatagramProtocol()
        self.assertIsNone(dp.connection_made(f))
        self.assertIsNone(dp.connection_lost(f))
        self.assertIsNone(dp.error_received(f))
        self.assertIsNone(dp.datagram_received(f, f))

        sp = protocols.SubprocessProtocol()
        self.assertIsNone(sp.connection_made(f))
        self.assertIsNone(sp.connection_lost(f))
        self.assertIsNone(sp.pipe_data_received(1, f))
        self.assertIsNone(sp.pipe_connection_lost(1, f))
        self.assertIsNone(sp.process_exited())


class PolicyTests(unittest.TestCase):

    def create_policy(self):
        if sys.platform == "win32":
            from asyncio import windows_events
            return windows_events.DefaultEventLoopPolicy()
        else:
            from asyncio import unix_events
            return unix_events.DefaultEventLoopPolicy()

    def test_event_loop_policy(self):
        policy = events.AbstractEventLoopPolicy()
        self.assertRaises(NotImplementedError, policy.get_event_loop)
        self.assertRaises(NotImplementedError, policy.set_event_loop, object())
        self.assertRaises(NotImplementedError, policy.new_event_loop)
        self.assertRaises(NotImplementedError, policy.get_child_watcher)
        self.assertRaises(NotImplementedError, policy.set_child_watcher,
                          object())

    def test_get_event_loop(self):
        policy = self.create_policy()
        self.assertIsNone(policy._local._loop)

        loop = policy.get_event_loop()
        self.assertIsInstance(loop, events.AbstractEventLoop)

        self.assertIs(policy._local._loop, loop)
        self.assertIs(loop, policy.get_event_loop())
        loop.close()

    def test_get_event_loop_calls_set_event_loop(self):
        policy = self.create_policy()

        with unittest.mock.patch.object(
                policy, "set_event_loop",
                wraps=policy.set_event_loop) as m_set_event_loop:

            loop = policy.get_event_loop()

            # policy._local._loop must be set through .set_event_loop()
            # (the unix DefaultEventLoopPolicy needs this call to attach
            # the child watcher correctly)
            m_set_event_loop.assert_called_with(loop)

        loop.close()

    def test_get_event_loop_after_set_none(self):
        policy = self.create_policy()
        policy.set_event_loop(None)
        self.assertRaises(AssertionError, policy.get_event_loop)

    @unittest.mock.patch('asyncio.events.threading.current_thread')
    def test_get_event_loop_thread(self, m_current_thread):

        def f():
            policy = self.create_policy()
            self.assertRaises(AssertionError, policy.get_event_loop)

        th = threading.Thread(target=f)
        th.start()
        th.join()

    def test_new_event_loop(self):
        policy = self.create_policy()

        loop = policy.new_event_loop()
        self.assertIsInstance(loop, events.AbstractEventLoop)
        loop.close()

    def test_set_event_loop(self):
        policy = self.create_policy()
        old_loop = policy.get_event_loop()

        self.assertRaises(AssertionError, policy.set_event_loop, object())

        loop = policy.new_event_loop()
        policy.set_event_loop(loop)
        self.assertIs(loop, policy.get_event_loop())
        self.assertIsNot(old_loop, policy.get_event_loop())
        loop.close()
        old_loop.close()

    def test_get_event_loop_policy(self):
        policy = events.get_event_loop_policy()
        self.assertIsInstance(policy, events.AbstractEventLoopPolicy)
        self.assertIs(policy, events.get_event_loop_policy())

    def test_set_event_loop_policy(self):
        self.assertRaises(
            AssertionError, events.set_event_loop_policy, object())

        old_policy = events.get_event_loop_policy()

        policy = self.create_policy()
        events.set_event_loop_policy(policy)
        self.assertIs(policy, events.get_event_loop_policy())
        self.assertIsNot(policy, old_policy)


if __name__ == '__main__':
    unittest.main()
