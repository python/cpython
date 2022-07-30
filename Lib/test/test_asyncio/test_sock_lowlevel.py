import socket
import asyncio
import sys
import unittest

from asyncio import proactor_events
from itertools import cycle, islice
from unittest.mock import Mock
from test.test_asyncio import utils as test_utils
from test import support
from test.support import socket_helper

def tearDownModule():
    asyncio.set_event_loop_policy(None)


class MyProto(asyncio.Protocol):
    connected = None
    done = None

    def __init__(self, loop=None):
        self.transport = None
        self.state = 'INITIAL'
        self.nbytes = 0
        if loop is not None:
            self.connected = loop.create_future()
            self.done = loop.create_future()

    def _assert_state(self, *expected):
        if self.state not in expected:
            raise AssertionError(f'state: {self.state!r}, expected: {expected!r}')

    def connection_made(self, transport):
        self.transport = transport
        self._assert_state('INITIAL')
        self.state = 'CONNECTED'
        if self.connected:
            self.connected.set_result(None)
        transport.write(b'GET / HTTP/1.0\r\nHost: example.com\r\n\r\n')

    def data_received(self, data):
        self._assert_state('CONNECTED')
        self.nbytes += len(data)

    def eof_received(self):
        self._assert_state('CONNECTED')
        self.state = 'EOF'

    def connection_lost(self, exc):
        self._assert_state('CONNECTED', 'EOF')
        self.state = 'CLOSED'
        if self.done:
            self.done.set_result(None)


class BaseSockTestsMixin:

    def create_event_loop(self):
        raise NotImplementedError

    def setUp(self):
        self.loop = self.create_event_loop()
        self.set_event_loop(self.loop)
        super().setUp()

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

    async def _basetest_sock_recv_racing(self, httpd, sock):
        sock.setblocking(False)
        await self.loop.sock_connect(sock, httpd.address)

        task = asyncio.create_task(self.loop.sock_recv(sock, 1024))
        await asyncio.sleep(0)
        task.cancel()

        asyncio.create_task(
            self.loop.sock_sendall(sock, b'GET / HTTP/1.0\r\n\r\n'))
        data = await self.loop.sock_recv(sock, 1024)
        # consume data
        await self.loop.sock_recv(sock, 1024)

        self.assertTrue(data.startswith(b'HTTP/1.0 200 OK'))

    async def _basetest_sock_recv_into_racing(self, httpd, sock):
        sock.setblocking(False)
        await self.loop.sock_connect(sock, httpd.address)

        data = bytearray(1024)
        with memoryview(data) as buf:
            task = asyncio.create_task(
                self.loop.sock_recv_into(sock, buf[:1024]))
            await asyncio.sleep(0)
            task.cancel()

            task = asyncio.create_task(
                self.loop.sock_sendall(sock, b'GET / HTTP/1.0\r\n\r\n'))
            nbytes = await self.loop.sock_recv_into(sock, buf[:1024])
            # consume data
            await self.loop.sock_recv_into(sock, buf[nbytes:])
            self.assertTrue(data.startswith(b'HTTP/1.0 200 OK'))

        await task

    async def _basetest_sock_send_racing(self, listener, sock):
        listener.bind(('127.0.0.1', 0))
        listener.listen(1)

        # make connection
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1024)
        sock.setblocking(False)
        task = asyncio.create_task(
            self.loop.sock_connect(sock, listener.getsockname()))
        await asyncio.sleep(0)
        server = listener.accept()[0]
        server.setblocking(False)

        with server:
            await task

            # fill the buffer until sending 5 chars would block
            size = 8192
            while size >= 4:
                with self.assertRaises(BlockingIOError):
                    while True:
                        sock.send(b' ' * size)
                size = int(size / 2)

            # cancel a blocked sock_sendall
            task = asyncio.create_task(
                self.loop.sock_sendall(sock, b'hello'))
            await asyncio.sleep(0)
            task.cancel()

            # receive everything that is not a space
            async def recv_all():
                rv = b''
                while True:
                    buf = await self.loop.sock_recv(server, 8192)
                    if not buf:
                        return rv
                    rv += buf.strip()
            task = asyncio.create_task(recv_all())

            # immediately make another sock_sendall call
            await self.loop.sock_sendall(sock, b'world')
            sock.shutdown(socket.SHUT_WR)
            data = await task
            # ProactorEventLoop could deliver hello, so endswith is necessary
            self.assertTrue(data.endswith(b'world'))

    # After the first connect attempt before the listener is ready,
    # the socket needs time to "recover" to make the next connect call.
    # On Linux, a second retry will do. On Windows, the waiting time is
    # unpredictable; and on FreeBSD the socket may never come back
    # because it's a loopback address. Here we'll just retry for a few
    # times, and have to skip the test if it's not working. See also:
    # https://stackoverflow.com/a/54437602/3316267
    # https://lists.freebsd.org/pipermail/freebsd-current/2005-May/049876.html
    async def _basetest_sock_connect_racing(self, listener, sock):
        listener.bind(('127.0.0.1', 0))
        addr = listener.getsockname()
        sock.setblocking(False)

        task = asyncio.create_task(self.loop.sock_connect(sock, addr))
        await asyncio.sleep(0)
        task.cancel()

        listener.listen(1)

        skip_reason = "Max retries reached"
        for i in range(128):
            try:
                await self.loop.sock_connect(sock, addr)
            except ConnectionRefusedError as e:
                skip_reason = e
            except OSError as e:
                skip_reason = e

                # Retry only for this error:
                # [WinError 10022] An invalid argument was supplied
                if getattr(e, 'winerror', 0) != 10022:
                    break
            else:
                # success
                return

        self.skipTest(skip_reason)

    def test_sock_client_racing(self):
        with test_utils.run_test_server() as httpd:
            sock = socket.socket()
            with sock:
                self.loop.run_until_complete(asyncio.wait_for(
                    self._basetest_sock_recv_racing(httpd, sock), 10))
            sock = socket.socket()
            with sock:
                self.loop.run_until_complete(asyncio.wait_for(
                    self._basetest_sock_recv_into_racing(httpd, sock), 10))
        listener = socket.socket()
        sock = socket.socket()
        with listener, sock:
            self.loop.run_until_complete(asyncio.wait_for(
                self._basetest_sock_send_racing(listener, sock), 10))

    def test_sock_client_connect_racing(self):
        listener = socket.socket()
        sock = socket.socket()
        with listener, sock:
            self.loop.run_until_complete(asyncio.wait_for(
                self._basetest_sock_connect_racing(listener, sock), 10))

    async def _basetest_huge_content(self, address):
        sock = socket.socket()
        sock.setblocking(False)
        DATA_SIZE = 10_000_00

        chunk = b'0123456789' * (DATA_SIZE // 10)

        await self.loop.sock_connect(sock, address)
        await self.loop.sock_sendall(sock,
                                     (b'POST /loop HTTP/1.0\r\n' +
                                      b'Content-Length: %d\r\n' % DATA_SIZE +
                                      b'\r\n'))

        task = asyncio.create_task(self.loop.sock_sendall(sock, chunk))

        data = await self.loop.sock_recv(sock, DATA_SIZE)
        # HTTP headers size is less than MTU,
        # they are sent by the first packet always
        self.assertTrue(data.startswith(b'HTTP/1.0 200 OK'))
        while data.find(b'\r\n\r\n') == -1:
            data += await self.loop.sock_recv(sock, DATA_SIZE)
        # Strip headers
        headers = data[:data.index(b'\r\n\r\n') + 4]
        data = data[len(headers):]

        size = DATA_SIZE
        checker = cycle(b'0123456789')

        expected = bytes(islice(checker, len(data)))
        self.assertEqual(data, expected)
        size -= len(data)

        while True:
            data = await self.loop.sock_recv(sock, DATA_SIZE)
            if not data:
                break
            expected = bytes(islice(checker, len(data)))
            self.assertEqual(data, expected)
            size -= len(data)
        self.assertEqual(size, 0)

        await task
        sock.close()

    def test_huge_content(self):
        with test_utils.run_test_server() as httpd:
            self.loop.run_until_complete(
                self._basetest_huge_content(httpd.address))

    async def _basetest_huge_content_recvinto(self, address):
        sock = socket.socket()
        sock.setblocking(False)
        DATA_SIZE = 10_000_00

        chunk = b'0123456789' * (DATA_SIZE // 10)

        await self.loop.sock_connect(sock, address)
        await self.loop.sock_sendall(sock,
                                     (b'POST /loop HTTP/1.0\r\n' +
                                      b'Content-Length: %d\r\n' % DATA_SIZE +
                                      b'\r\n'))

        task = asyncio.create_task(self.loop.sock_sendall(sock, chunk))

        array = bytearray(DATA_SIZE)
        buf = memoryview(array)

        nbytes = await self.loop.sock_recv_into(sock, buf)
        data = bytes(buf[:nbytes])
        # HTTP headers size is less than MTU,
        # they are sent by the first packet always
        self.assertTrue(data.startswith(b'HTTP/1.0 200 OK'))
        while data.find(b'\r\n\r\n') == -1:
            nbytes = await self.loop.sock_recv_into(sock, buf)
            data = bytes(buf[:nbytes])
        # Strip headers
        headers = data[:data.index(b'\r\n\r\n') + 4]
        data = data[len(headers):]

        size = DATA_SIZE
        checker = cycle(b'0123456789')

        expected = bytes(islice(checker, len(data)))
        self.assertEqual(data, expected)
        size -= len(data)

        while True:
            nbytes = await self.loop.sock_recv_into(sock, buf)
            data = buf[:nbytes]
            if not data:
                break
            expected = bytes(islice(checker, len(data)))
            self.assertEqual(data, expected)
            size -= len(data)
        self.assertEqual(size, 0)

        await task
        sock.close()

    def test_huge_content_recvinto(self):
        with test_utils.run_test_server() as httpd:
            self.loop.run_until_complete(
                self._basetest_huge_content_recvinto(httpd.address))

    async def _basetest_datagram_recvfrom(self, server_address):
        # Happy path, sock.sendto() returns immediately
        data = b'\x01' * 4096
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.setblocking(False)
            await self.loop.sock_sendto(sock, data, server_address)
            received_data, from_addr = await self.loop.sock_recvfrom(
                sock, 4096)
            self.assertEqual(received_data, data)
            self.assertEqual(from_addr, server_address)

    def test_recvfrom(self):
        with test_utils.run_udp_echo_server() as server_address:
            self.loop.run_until_complete(
                self._basetest_datagram_recvfrom(server_address))

    async def _basetest_datagram_recvfrom_into(self, server_address):
        # Happy path, sock.sendto() returns immediately
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.setblocking(False)

            buf = bytearray(4096)
            data = b'\x01' * 4096
            await self.loop.sock_sendto(sock, data, server_address)
            num_bytes, from_addr = await self.loop.sock_recvfrom_into(
                sock, buf)
            self.assertEqual(num_bytes, 4096)
            self.assertEqual(buf, data)
            self.assertEqual(from_addr, server_address)

            buf = bytearray(8192)
            await self.loop.sock_sendto(sock, data, server_address)
            num_bytes, from_addr = await self.loop.sock_recvfrom_into(
                sock, buf, 4096)
            self.assertEqual(num_bytes, 4096)
            self.assertEqual(buf[:4096], data[:4096])
            self.assertEqual(from_addr, server_address)

    def test_recvfrom_into(self):
        with test_utils.run_udp_echo_server() as server_address:
            self.loop.run_until_complete(
                self._basetest_datagram_recvfrom_into(server_address))

    async def _basetest_datagram_sendto_blocking(self, server_address):
        # Sad path, sock.sendto() raises BlockingIOError
        # This involves patching sock.sendto() to raise BlockingIOError but
        # sendto() is not used by the proactor event loop
        data = b'\x01' * 4096
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.setblocking(False)
            mock_sock = Mock(sock)
            mock_sock.gettimeout = sock.gettimeout
            mock_sock.sendto.configure_mock(side_effect=BlockingIOError)
            mock_sock.fileno = sock.fileno
            self.loop.call_soon(
                lambda: setattr(mock_sock, 'sendto', sock.sendto)
            )
            await self.loop.sock_sendto(mock_sock, data, server_address)

            received_data, from_addr = await self.loop.sock_recvfrom(
                sock, 4096)
            self.assertEqual(received_data, data)
            self.assertEqual(from_addr, server_address)

    def test_sendto_blocking(self):
        if sys.platform == 'win32':
            if isinstance(self.loop, asyncio.ProactorEventLoop):
                raise unittest.SkipTest('Not relevant to ProactorEventLoop')

        with test_utils.run_udp_echo_server() as server_address:
            self.loop.run_until_complete(
                self._basetest_datagram_sendto_blocking(server_address))

    @socket_helper.skip_unless_bind_unix_socket
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

    def test_cancel_sock_accept(self):
        listener = socket.socket()
        listener.setblocking(False)
        listener.bind(('127.0.0.1', 0))
        listener.listen(1)
        sockaddr = listener.getsockname()
        f = asyncio.wait_for(self.loop.sock_accept(listener), 0.1)
        with self.assertRaises(asyncio.TimeoutError):
            self.loop.run_until_complete(f)

        listener.close()
        client = socket.socket()
        client.setblocking(False)
        f = self.loop.sock_connect(client, sockaddr)
        with self.assertRaises(ConnectionRefusedError):
            self.loop.run_until_complete(f)

        client.close()

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
                except BaseException:
                    pass
                else:
                    break
            else:
                self.fail('Can not create socket.')

            f = self.loop.create_connection(
                lambda: MyProto(loop=self.loop), sock=sock)
            tr, pr = self.loop.run_until_complete(f)
            self.assertIsInstance(tr, asyncio.Transport)
            self.assertIsInstance(pr, asyncio.Protocol)
            self.loop.run_until_complete(pr.done)
            self.assertGreater(pr.nbytes, 0)
            tr.close()


if sys.platform == 'win32':

    class SelectEventLoopTests(BaseSockTestsMixin,
                               test_utils.TestCase):

        def create_event_loop(self):
            return asyncio.SelectorEventLoop()

    class ProactorEventLoopTests(BaseSockTestsMixin,
                                 test_utils.TestCase):

        def create_event_loop(self):
            return asyncio.ProactorEventLoop()

else:
    import selectors

    if hasattr(selectors, 'KqueueSelector'):
        class KqueueEventLoopTests(BaseSockTestsMixin,
                                   test_utils.TestCase):

            def create_event_loop(self):
                return asyncio.SelectorEventLoop(
                    selectors.KqueueSelector())

    if hasattr(selectors, 'EpollSelector'):
        class EPollEventLoopTests(BaseSockTestsMixin,
                                  test_utils.TestCase):

            def create_event_loop(self):
                return asyncio.SelectorEventLoop(selectors.EpollSelector())

    if hasattr(selectors, 'PollSelector'):
        class PollEventLoopTests(BaseSockTestsMixin,
                                 test_utils.TestCase):

            def create_event_loop(self):
                return asyncio.SelectorEventLoop(selectors.PollSelector())

    # Should always exist.
    class SelectEventLoopTests(BaseSockTestsMixin,
                               test_utils.TestCase):

        def create_event_loop(self):
            return asyncio.SelectorEventLoop(selectors.SelectSelector())


if __name__ == '__main__':
    unittest.main()
