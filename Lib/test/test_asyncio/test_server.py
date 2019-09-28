import asyncio
import time
import threading
import unittest

from test import support
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

        with self.assertWarns(DeprecationWarning):
            srv = self.loop.run_until_complete(asyncio.start_server(
                serve, support.HOSTv4, 0, loop=self.loop, start_serving=False))

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

    @support.skip_unless_bind_unix_socket
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
            with self.assertWarns(DeprecationWarning):
                srv = self.loop.run_until_complete(asyncio.start_unix_server(
                    serve, addr, loop=self.loop, start_serving=False))

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


@unittest.skipUnless(hasattr(asyncio, 'ProactorEventLoop'), 'Windows only')
class ProactorStartServerTests(BaseStartServer, unittest.TestCase):

    def new_loop(self):
        return asyncio.ProactorEventLoop()


if __name__ == '__main__':
    unittest.main()
