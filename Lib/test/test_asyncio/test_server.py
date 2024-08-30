import asyncio
import os
import socket
import time
import threading
import unittest

from test.support import socket_helper
from test.test_asyncio import utils as test_utils
from test.test_asyncio import functional as func_tests


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class BaseStartServer(func_tests.FunctionalTestCaseMixin):

    def new_loop(self):
        raise NotImplementedError

    def test_start_server_1(self):
        HELLO_MSG = b'1' * 1024 * 5 + b'\n'

        def client(sock, addr):
            for i in range(10):
                time.sleep(0.2)
                if srv.is_serving():
                    break
            else:
                raise RuntimeError

            sock.settimeout(2)
            sock.connect(addr)
            sock.send(HELLO_MSG)
            sock.recv_all(1)
            sock.close()

        async def serve(reader, writer):
            await reader.readline()
            main_task.cancel()
            writer.write(b'1')
            writer.close()
            await writer.wait_closed()

        async def main(srv):
            async with srv:
                await srv.serve_forever()

        srv = self.loop.run_until_complete(asyncio.start_server(
            serve, socket_helper.HOSTv4, 0, start_serving=False))

        self.assertFalse(srv.is_serving())

        main_task = self.loop.create_task(main(srv))

        addr = srv.sockets[0].getsockname()
        with self.assertRaises(asyncio.CancelledError):
            with self.tcp_client(lambda sock: client(sock, addr)):
                self.loop.run_until_complete(main_task)

        self.assertEqual(srv.sockets, ())

        self.assertIsNone(srv._sockets)
        self.assertIsNone(srv._waiters)
        self.assertFalse(srv.is_serving())

        with self.assertRaisesRegex(RuntimeError, r'is closed'):
            self.loop.run_until_complete(srv.serve_forever())


class SelectorStartServerTests(BaseStartServer, unittest.TestCase):

    def new_loop(self):
        return asyncio.SelectorEventLoop()

    @socket_helper.skip_unless_bind_unix_socket
    def test_start_unix_server_1(self):
        HELLO_MSG = b'1' * 1024 * 5 + b'\n'
        started = threading.Event()

        def client(sock, addr):
            sock.settimeout(2)
            started.wait(5)
            sock.connect(addr)
            sock.send(HELLO_MSG)
            sock.recv_all(1)
            sock.close()

        async def serve(reader, writer):
            await reader.readline()
            main_task.cancel()
            writer.write(b'1')
            writer.close()
            await writer.wait_closed()

        async def main(srv):
            async with srv:
                self.assertFalse(srv.is_serving())
                await srv.start_serving()
                self.assertTrue(srv.is_serving())
                started.set()
                await srv.serve_forever()

        with test_utils.unix_socket_path() as addr:
            srv = self.loop.run_until_complete(asyncio.start_unix_server(
                serve, addr, start_serving=False))

            main_task = self.loop.create_task(main(srv))

            with self.assertRaises(asyncio.CancelledError):
                with self.unix_client(lambda sock: client(sock, addr)):
                    self.loop.run_until_complete(main_task)

            self.assertEqual(srv.sockets, ())

            self.assertIsNone(srv._sockets)
            self.assertIsNone(srv._waiters)
            self.assertFalse(srv.is_serving())

            with self.assertRaisesRegex(RuntimeError, r'is closed'):
                self.loop.run_until_complete(srv.serve_forever())


class TestServer2(unittest.IsolatedAsyncioTestCase):

    async def test_wait_closed_basic(self):
        async def serve(rd, wr):
            try:
                await rd.read()
            finally:
                wr.close()
                await wr.wait_closed()

        srv = await asyncio.start_server(serve, socket_helper.HOSTv4, 0)
        self.addCleanup(srv.close)

        # active count = 0, not closed: should block
        task1 = asyncio.create_task(srv.wait_closed())
        await asyncio.sleep(0)
        self.assertFalse(task1.done())

        # active count != 0, not closed: should block
        addr = srv.sockets[0].getsockname()
        (rd, wr) = await asyncio.open_connection(addr[0], addr[1])
        task2 = asyncio.create_task(srv.wait_closed())
        await asyncio.sleep(0)
        self.assertFalse(task1.done())
        self.assertFalse(task2.done())

        srv.close()
        await asyncio.sleep(0)
        # active count != 0, closed: should block
        task3 = asyncio.create_task(srv.wait_closed())
        await asyncio.sleep(0)
        self.assertFalse(task1.done())
        self.assertFalse(task2.done())
        self.assertFalse(task3.done())

        wr.close()
        await wr.wait_closed()
        # active count == 0, closed: should unblock
        await task1
        await task2
        await task3
        await srv.wait_closed()  # Return immediately

    async def test_wait_closed_race(self):
        # Test a regression in 3.12.0, should be fixed in 3.12.1
        async def serve(rd, wr):
            try:
                await rd.read()
            finally:
                wr.close()
                await wr.wait_closed()

        srv = await asyncio.start_server(serve, socket_helper.HOSTv4, 0)
        self.addCleanup(srv.close)

        task = asyncio.create_task(srv.wait_closed())
        await asyncio.sleep(0)
        self.assertFalse(task.done())
        addr = srv.sockets[0].getsockname()
        (rd, wr) = await asyncio.open_connection(addr[0], addr[1])
        loop = asyncio.get_running_loop()
        loop.call_soon(srv.close)
        loop.call_soon(wr.close)
        await srv.wait_closed()

    async def test_close_clients(self):
        async def serve(rd, wr):
            try:
                await rd.read()
            finally:
                wr.close()
                await wr.wait_closed()

        srv = await asyncio.start_server(serve, socket_helper.HOSTv4, 0)
        self.addCleanup(srv.close)

        addr = srv.sockets[0].getsockname()
        (rd, wr) = await asyncio.open_connection(addr[0], addr[1])
        self.addCleanup(wr.close)

        task = asyncio.create_task(srv.wait_closed())
        await asyncio.sleep(0)
        self.assertFalse(task.done())

        srv.close()
        srv.close_clients()
        await asyncio.sleep(0)
        await asyncio.sleep(0)
        self.assertTrue(task.done())

    async def test_abort_clients(self):
        async def serve(rd, wr):
            fut.set_result((rd, wr))
            await wr.wait_closed()

        fut = asyncio.Future()
        srv = await asyncio.start_server(serve, socket_helper.HOSTv4, 0)
        self.addCleanup(srv.close)

        addr = srv.sockets[0].getsockname()
        (c_rd, c_wr) = await asyncio.open_connection(addr[0], addr[1], limit=4096)
        self.addCleanup(c_wr.close)

        (s_rd, s_wr) = await fut

        # Limit the socket buffers so we can more reliably overfill them
        s_sock = s_wr.get_extra_info('socket')
        s_sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)
        c_sock = c_wr.get_extra_info('socket')
        c_sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)

        # Get the reader in to a paused state by sending more than twice
        # the configured limit
        s_wr.write(b'a' * 4096)
        s_wr.write(b'a' * 4096)
        s_wr.write(b'a' * 4096)
        while c_wr.transport.is_reading():
            await asyncio.sleep(0)

        # Get the writer in a waiting state by sending data until the
        # kernel stops accepting more data in the send buffer.
        # gh-122136: getsockopt() does not reliably report the buffer size
        # available for message content.
        # We loop until we start filling up the asyncio buffer.
        # To avoid an infinite loop we cap at 10 times the expected value
        c_bufsize = c_sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
        s_bufsize = s_sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
        for i in range(10):
            s_wr.write(b'a' * c_bufsize)
            s_wr.write(b'a' * s_bufsize)
            if s_wr.transport.get_write_buffer_size() > 0:
                break
        self.assertNotEqual(s_wr.transport.get_write_buffer_size(), 0)

        task = asyncio.create_task(srv.wait_closed())
        await asyncio.sleep(0)
        self.assertFalse(task.done())

        srv.close()
        srv.abort_clients()
        await asyncio.sleep(0)
        await asyncio.sleep(0)
        self.assertTrue(task.done())


# Test the various corner cases of Unix server socket removal
class UnixServerCleanupTests(unittest.IsolatedAsyncioTestCase):
    @socket_helper.skip_unless_bind_unix_socket
    async def test_unix_server_addr_cleanup(self):
        # Default scenario
        with test_utils.unix_socket_path() as addr:
            async def serve(*args):
                pass

            srv = await asyncio.start_unix_server(serve, addr)

            srv.close()
            self.assertFalse(os.path.exists(addr))

    @socket_helper.skip_unless_bind_unix_socket
    async def test_unix_server_sock_cleanup(self):
        # Using already bound socket
        with test_utils.unix_socket_path() as addr:
            async def serve(*args):
                pass

            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
                sock.bind(addr)

                srv = await asyncio.start_unix_server(serve, sock=sock)

                srv.close()
                self.assertFalse(os.path.exists(addr))

    @socket_helper.skip_unless_bind_unix_socket
    async def test_unix_server_cleanup_gone(self):
        # Someone else has already cleaned up the socket
        with test_utils.unix_socket_path() as addr:
            async def serve(*args):
                pass

            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
                sock.bind(addr)

                srv = await asyncio.start_unix_server(serve, sock=sock)

                os.unlink(addr)

                srv.close()

    @socket_helper.skip_unless_bind_unix_socket
    async def test_unix_server_cleanup_replaced(self):
        # Someone else has replaced the socket with their own
        with test_utils.unix_socket_path() as addr:
            async def serve(*args):
                pass

            srv = await asyncio.start_unix_server(serve, addr)

            os.unlink(addr)
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
                sock.bind(addr)

                srv.close()
                self.assertTrue(os.path.exists(addr))

    @socket_helper.skip_unless_bind_unix_socket
    async def test_unix_server_cleanup_prevented(self):
        # Automatic cleanup explicitly disabled
        with test_utils.unix_socket_path() as addr:
            async def serve(*args):
                pass

            srv = await asyncio.start_unix_server(serve, addr, cleanup_socket=False)

            srv.close()
            self.assertTrue(os.path.exists(addr))


@unittest.skipUnless(hasattr(asyncio, 'ProactorEventLoop'), 'Windows only')
class ProactorStartServerTests(BaseStartServer, unittest.TestCase):

    def new_loop(self):
        return asyncio.ProactorEventLoop()


if __name__ == '__main__':
    unittest.main()
