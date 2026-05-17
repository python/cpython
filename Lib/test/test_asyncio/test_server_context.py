import asyncio
import contextvars
import unittest
import sys

from unittest import TestCase

try:
    import ssl
except ImportError:
    ssl = None

from test.test_asyncio import utils as test_utils

def tearDownModule():
    asyncio.events._set_event_loop_policy(None)

class ServerContextvarsTestCase:
    loop_factory = None  # To be defined in subclasses
    server_ssl_context = None  # To be defined in subclasses for SSL tests
    client_ssl_context = None  # To be defined in subclasses for SSL tests

    def run_coro(self, coro):
        return asyncio.run(coro, loop_factory=self.loop_factory)

    def test_start_server1(self):
        # Test that asyncio.start_server captures the context at the time of server creation
        async def test():
            var = contextvars.ContextVar("var", default="default")

            async def handle_client(reader, writer):
                value = var.get()
                writer.write(value.encode())
                await writer.drain()
                writer.close()

            server = await asyncio.start_server(handle_client, '127.0.0.1', 0,
                                               ssl=self.server_ssl_context)
            # change the value
            var.set("after_server")

            async def client(addr):
                reader, writer = await asyncio.open_connection(*addr,
                                                               ssl=self.client_ssl_context)
                data = await reader.read(100)
                writer.close()
                await writer.wait_closed()
                return data.decode()

            async with server:
                addr = server.sockets[0].getsockname()
                self.assertEqual(await client(addr), "default")

            self.assertEqual(var.get(), "after_server")

        self.run_coro(test())

    def test_start_server2(self):
        # Test that mutations to the context in one handler don't affect other handlers or the server's context
        async def test():
            var = contextvars.ContextVar("var", default="default")

            async def handle_client(reader, writer):
                value = var.get()
                writer.write(value.encode())
                var.set("in_handler")
                await writer.drain()
                writer.close()

            server = await asyncio.start_server(handle_client, '127.0.0.1', 0,
                                               ssl=self.server_ssl_context)
            var.set("after_server")

            async def client(addr):
                reader, writer = await asyncio.open_connection(*addr,
                                                               ssl=self.client_ssl_context)
                data = await reader.read(100)
                writer.close()
                await writer.wait_closed()
                return data.decode()

            async with server:
                addr = server.sockets[0].getsockname()
                self.assertEqual(await client(addr), "default")
                self.assertEqual(await client(addr), "default")
                self.assertEqual(await client(addr), "default")

            self.assertEqual(var.get(), "after_server")

        self.run_coro(test())

    def test_start_server3(self):
        # Test that mutations to context in concurrent handlers don't affect each other or the server's context
        async def test():
            var = contextvars.ContextVar("var", default="default")
            var.set("before_server")

            async def handle_client(reader, writer):
                writer.write(var.get().encode())
                await writer.drain()
                writer.close()

            server = await asyncio.start_server(handle_client, '127.0.0.1', 0,
                                               ssl=self.server_ssl_context)
            var.set("after_server")

            async def client(addr):
                reader, writer = await asyncio.open_connection(*addr,
                                                               ssl=self.client_ssl_context)
                data = await reader.read(100)
                self.assertEqual(data.decode(), "before_server")
                writer.close()
                await writer.wait_closed()

            async with server:
                addr = server.sockets[0].getsockname()
                async with asyncio.TaskGroup() as tg:
                    for _ in range(100):
                        tg.create_task(client(addr))

            self.assertEqual(var.get(), "after_server")

        self.run_coro(test())

    def test_create_server1(self):
        # Test that loop.create_server captures the context at the time of server creation
        # and that mutations to the context in protocol callbacks don't affect the server's context
        async def test():
            var = contextvars.ContextVar("var", default="default")

            class EchoProtocol(asyncio.Protocol):
                def connection_made(self, transport):
                    self.transport = transport
                    value = var.get()
                    var.set("in_handler")
                    self.transport.write(value.encode())
                    self.transport.close()

            server = await asyncio.get_running_loop().create_server(
                lambda: EchoProtocol(), '127.0.0.1', 0,
                ssl=self.server_ssl_context)
            var.set("after_server")

            async def client(addr):
                reader, writer = await asyncio.open_connection(*addr,
                                                               ssl=self.client_ssl_context)
                data = await reader.read(100)
                self.assertEqual(data.decode(), "default")
                writer.close()
                await writer.wait_closed()

            async with server:
                addr = server.sockets[0].getsockname()
                await client(addr)

            self.assertEqual(var.get(), "after_server")

        self.run_coro(test())

    def test_create_server2(self):
        # Test that mutations to context in one protocol instance don't affect other instances or the server's context
        async def test():
            var = contextvars.ContextVar("var", default="default")

            class EchoProtocol(asyncio.Protocol):
                def __init__(self):
                    super().__init__()
                    assert var.get() == "default", var.get()
                def connection_made(self, transport):
                    self.transport = transport
                    value = var.get()
                    var.set("in_handler")
                    self.transport.write(value.encode())
                    self.transport.close()

            server = await asyncio.get_running_loop().create_server(
                lambda: EchoProtocol(), '127.0.0.1', 0,
                ssl=self.server_ssl_context)

            var.set("after_server")

            async def client(addr, expected):
                reader, writer = await asyncio.open_connection(*addr,
                                                               ssl=self.client_ssl_context)
                data = await reader.read(100)
                self.assertEqual(data.decode(), expected)
                writer.close()
                await writer.wait_closed()

            async with server:
                addr = server.sockets[0].getsockname()
                await client(addr, "default")
                await client(addr, "default")

            self.assertEqual(var.get(), "after_server")

        self.run_coro(test())

    def test_gh140947(self):
        # See https://github.com/python/cpython/issues/140947

        cvar1 = contextvars.ContextVar("cvar1")
        cvar2 = contextvars.ContextVar("cvar2")
        cvar3 = contextvars.ContextVar("cvar3")
        results = {}
        is_ssl = self.server_ssl_context is not None

        def capture_context(meth):
            result = []
            for k,v in contextvars.copy_context().items():
                if k.name.startswith("cvar"):
                    result.append((k.name, v))
            results[meth] = sorted(result)

        class DemoProtocol(asyncio.Protocol):
            def __init__(self, on_conn_lost):
                self.transport = None
                self.on_conn_lost = on_conn_lost
                self.tasks = set()

            def connection_made(self, transport):
                capture_context("connection_made")
                self.transport = transport

            def data_received(self, data):
                capture_context("data_received")

                task = asyncio.create_task(self.asgi())
                self.tasks.add(task)
                task.add_done_callback(self.tasks.discard)

                self.transport.pause_reading()

            def connection_lost(self, exc):
                capture_context("connection_lost")
                if not self.on_conn_lost.done():
                    self.on_conn_lost.set_result(True)

            async def asgi(self):
                capture_context("asgi start")
                cvar1.set(True)
                # make sure that we only resume after the pause
                # otherwise the resume does nothing
                if is_ssl:
                    while not self.transport._ssl_protocol._app_reading_paused:
                        await asyncio.sleep(0.01)
                else:
                    while not self.transport._paused:
                        await asyncio.sleep(0.01)
                cvar2.set(True)
                self.transport.resume_reading()
                cvar3.set(True)
                capture_context("asgi end")

        async def main():
            loop = asyncio.get_running_loop()
            on_conn_lost = loop.create_future()

            server = await loop.create_server(
                lambda: DemoProtocol(on_conn_lost), '127.0.0.1', 0,
                ssl=self.server_ssl_context)
            async with server:
                addr = server.sockets[0].getsockname()
                reader, writer = await asyncio.open_connection(*addr,
                                                               ssl=self.client_ssl_context)
                writer.write(b"anything")
                await writer.drain()
                writer.close()
                await writer.wait_closed()
                await on_conn_lost

        self.run_coro(main())
        self.assertDictEqual(results, {
            "connection_made": [],
            "data_received": [],
            "asgi start": [],
            "asgi end": [("cvar1", True), ("cvar2", True), ("cvar3", True)],
            "connection_lost": [],
        })


class AsyncioEventLoopTests(TestCase, ServerContextvarsTestCase):
    loop_factory = staticmethod(asyncio.new_event_loop)

@unittest.skipUnless(ssl, "SSL not available")
class AsyncioEventLoopSSLTests(AsyncioEventLoopTests):
    def setUp(self):
        super().setUp()
        self.server_ssl_context = test_utils.simple_server_sslcontext()
        self.client_ssl_context = test_utils.simple_client_sslcontext()

if sys.platform == "win32":
    class AsyncioProactorEventLoopTests(TestCase, ServerContextvarsTestCase):
        loop_factory = asyncio.ProactorEventLoop

    class AsyncioSelectorEventLoopTests(TestCase, ServerContextvarsTestCase):
        loop_factory = asyncio.SelectorEventLoop

    @unittest.skipUnless(ssl, "SSL not available")
    class AsyncioProactorEventLoopSSLTests(AsyncioProactorEventLoopTests):
        def setUp(self):
            super().setUp()
            self.server_ssl_context = test_utils.simple_server_sslcontext()
            self.client_ssl_context = test_utils.simple_client_sslcontext()

    @unittest.skipUnless(ssl, "SSL not available")
    class AsyncioSelectorEventLoopSSLTests(AsyncioSelectorEventLoopTests):
        def setUp(self):
            super().setUp()
            self.server_ssl_context = test_utils.simple_server_sslcontext()
            self.client_ssl_context = test_utils.simple_client_sslcontext()

if __name__ == "__main__":
    unittest.main()
