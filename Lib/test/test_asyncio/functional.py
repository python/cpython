import asyncio
import asyncio.events
import contextlib
import os
import pprint
import select
import socket
import tempfile
import threading


class FunctionalTestCaseMixin:

    def new_loop(self):
        return asyncio.new_event_loop()

    def run_loop_briefly(self, *, delay=0.01):
        self.loop.run_until_complete(asyncio.sleep(delay))

    def loop_exception_handler(self, loop, context):
        self.__unhandled_exceptions.append(context)
        self.loop.default_exception_handler(context)

    def setUp(self):
        self.loop = self.new_loop()
        asyncio.set_event_loop(None)

        self.loop.set_exception_handler(self.loop_exception_handler)
        self.__unhandled_exceptions = []

        # Disable `_get_running_loop`.
        self._old_get_running_loop = asyncio.events._get_running_loop
        asyncio.events._get_running_loop = lambda: None

    def tearDown(self):
        try:
            self.loop.close()

            if self.__unhandled_exceptions:
                print('Unexpected calls to loop.call_exception_handler():')
                pprint.pprint(self.__unhandled_exceptions)
                self.fail('unexpected calls to loop.call_exception_handler()')

        finally:
            asyncio.events._get_running_loop = self._old_get_running_loop
            asyncio.set_event_loop(None)
            self.loop = None

    def tcp_server(self, server_prog, *,
                   family=socket.AF_INET,
                   addr=None,
                   timeout=5,
                   backlog=1,
                   max_clients=10):

        if addr is None:
            if hasattr(socket, 'AF_UNIX') and family == socket.AF_UNIX:
                with tempfile.NamedTemporaryFile() as tmp:
                    addr = tmp.name
            else:
                addr = ('127.0.0.1', 0)

        sock = socket.create_server(addr, family=family, backlog=backlog)
        if timeout is None:
            raise RuntimeError('timeout is required')
        if timeout <= 0:
            raise RuntimeError('only blocking sockets are supported')
        sock.settimeout(timeout)

        return TestThreadedServer(
            self, sock, server_prog, timeout, max_clients)

    def tcp_client(self, client_prog,
                   family=socket.AF_INET,
                   timeout=10):

        sock = socket.socket(family, socket.SOCK_STREAM)

        if timeout is None:
            raise RuntimeError('timeout is required')
        if timeout <= 0:
            raise RuntimeError('only blocking sockets are supported')
        sock.settimeout(timeout)

        return TestThreadedClient(
            self, sock, client_prog, timeout)

    def unix_server(self, *args, **kwargs):
        if not hasattr(socket, 'AF_UNIX'):
            raise NotImplementedError
        return self.tcp_server(*args, family=socket.AF_UNIX, **kwargs)

    def unix_client(self, *args, **kwargs):
        if not hasattr(socket, 'AF_UNIX'):
            raise NotImplementedError
        return self.tcp_client(*args, family=socket.AF_UNIX, **kwargs)

    @contextlib.contextmanager
    def unix_sock_name(self):
        with tempfile.TemporaryDirectory() as td:
            fn = os.path.join(td, 'sock')
            try:
                yield fn
            finally:
                try:
                    os.unlink(fn)
                except OSError:
                    pass

    def _abort_socket_test(self, ex):
        try:
            self.loop.stop()
        finally:
            self.fail(ex)


##############################################################################
# Socket Testing Utilities
##############################################################################


class TestSocketWrapper:

    def __init__(self, sock):
        self.__sock = sock

    def recv_all(self, n):
        buf = b''
        while len(buf) < n:
            data = self.recv(n - len(buf))
            if data == b'':
                raise ConnectionAbortedError
            buf += data
        return buf

    def start_tls(self, ssl_context, *,
                  server_side=False,
                  server_hostname=None):

        ssl_sock = ssl_context.wrap_socket(
            self.__sock, server_side=server_side,
            server_hostname=server_hostname,
            do_handshake_on_connect=False)

        try:
            ssl_sock.do_handshake()
        except:
            ssl_sock.close()
            raise
        finally:
            self.__sock.close()

        self.__sock = ssl_sock

    def __getattr__(self, name):
        return getattr(self.__sock, name)

    def __repr__(self):
        return '<{} {!r}>'.format(type(self).__name__, self.__sock)


class SocketThread(threading.Thread):

    def stop(self):
        self._active = False
        self.join()

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *exc):
        self.stop()


class TestThreadedClient(SocketThread):

    def __init__(self, test, sock, prog, timeout):
        threading.Thread.__init__(self, None, None, 'test-client')
        self.daemon = True

        self._timeout = timeout
        self._sock = sock
        self._active = True
        self._prog = prog
        self._test = test

    def run(self):
        try:
            self._prog(TestSocketWrapper(self._sock))
        except Exception as ex:
            self._test._abort_socket_test(ex)


class TestThreadedServer(SocketThread):

    def __init__(self, test, sock, prog, timeout, max_clients):
        threading.Thread.__init__(self, None, None, 'test-server')
        self.daemon = True

        self._clients = 0
        self._finished_clients = 0
        self._max_clients = max_clients
        self._timeout = timeout
        self._sock = sock
        self._active = True

        self._prog = prog

        self._s1, self._s2 = socket.socketpair()
        self._s1.setblocking(False)

        self._test = test

    def stop(self):
        try:
            if self._s2 and self._s2.fileno() != -1:
                try:
                    self._s2.send(b'stop')
                except OSError:
                    pass
        finally:
            super().stop()

    def run(self):
        try:
            with self._sock:
                self._sock.setblocking(False)
                self._run()
        finally:
            self._s1.close()
            self._s2.close()

    def _run(self):
        while self._active:
            if self._clients >= self._max_clients:
                return

            r, w, x = select.select(
                [self._sock, self._s1], [], [], self._timeout)

            if self._s1 in r:
                return

            if self._sock in r:
                try:
                    conn, addr = self._sock.accept()
                except BlockingIOError:
                    continue
                except socket.timeout:
                    if not self._active:
                        return
                    else:
                        raise
                else:
                    self._clients += 1
                    conn.settimeout(self._timeout)
                    try:
                        with conn:
                            self._handle_client(conn)
                    except Exception as ex:
                        self._active = False
                        try:
                            raise
                        finally:
                            self._test._abort_socket_test(ex)

    def _handle_client(self, sock):
        self._prog(TestSocketWrapper(sock))

    @property
    def addr(self):
        return self._sock.getsockname()
