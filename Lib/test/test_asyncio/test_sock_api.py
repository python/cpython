"""Tests for low level loop.sock*() methods."""


from test import support
from test.test_asyncio import utils as test_utils


class SockMethodsTestsMixin:
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

    def _basetest_sock_client_ops(self, httpd, sock):
        if not isinstance(self.loop, proactor_events.BaseProactorEventLoop):
            # in debug mode, socket operations must fail
            # if the socket is not in blocking mode
            self.loop.set_debug(True)
            sock.setblocking(True)
            with self.assertRaises(ValueError):
                self.loop.run_until_complete(
                    self.loop.sock_connect(sock, httpd.address))
            with self.assertRaises(ValueError):
                self.loop.run_until_complete(
                    self.loop.sock_sendall(sock, b'GET / HTTP/1.0\r\n\r\n'))
            with self.assertRaises(ValueError):
                self.loop.run_until_complete(
                    self.loop.sock_recv(sock, 1024))
            with self.assertRaises(ValueError):
                self.loop.run_until_complete(
                    self.loop.sock_recv_into(sock, bytearray()))
            with self.assertRaises(ValueError):
                self.loop.run_until_complete(
                    self.loop.sock_accept(sock))

        # test in non-blocking mode
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

    def _basetest_sock_recv_into(self, httpd, sock):
        # same as _basetest_sock_client_ops, but using sock_recv_into
        sock.setblocking(False)
        self.loop.run_until_complete(
            self.loop.sock_connect(sock, httpd.address))
        self.loop.run_until_complete(
            self.loop.sock_sendall(sock, b'GET / HTTP/1.0\r\n\r\n'))
        data = bytearray(1024)
        with memoryview(data) as buf:
            nbytes = self.loop.run_until_complete(
                self.loop.sock_recv_into(sock, buf[:1024]))
            # consume data
            self.loop.run_until_complete(
                self.loop.sock_recv_into(sock, buf[nbytes:]))
        sock.close()
        self.assertTrue(data.startswith(b'HTTP/1.0 200 OK'))

    def test_sock_client_ops(self):
        with test_utils.run_test_server() as httpd:
            sock = socket.socket()
            self._basetest_sock_client_ops(httpd, sock)
            sock = socket.socket()
            self._basetest_sock_recv_into(httpd, sock)

    @support.skip_unless_bind_unix_socket
    def test_unix_sock_client_ops(self):
        with test_utils.run_test_unix_server() as httpd:
            sock = socket.socket(socket.AF_UNIX)
            self._basetest_sock_client_ops(httpd, sock)
            sock = socket.socket(socket.AF_UNIX)
            self._basetest_sock_recv_into(httpd, sock)

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

