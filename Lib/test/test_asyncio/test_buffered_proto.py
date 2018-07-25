import asyncio
import unittest

from test.test_asyncio import functional as func_tests


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class ReceiveStuffProto(asyncio.BufferedProtocol):
    def __init__(self, cb, con_lost_fut):
        self.cb = cb
        self.con_lost_fut = con_lost_fut

    def get_buffer(self, sizehint):
        self.buffer = bytearray(100)
        return self.buffer

    def buffer_updated(self, nbytes):
        self.cb(self.buffer[:nbytes])

    def connection_lost(self, exc):
        if exc is None:
            self.con_lost_fut.set_result(None)
        else:
            self.con_lost_fut.set_exception(exc)


class BaseTestBufferedProtocol(func_tests.FunctionalTestCaseMixin):

    def new_loop(self):
        raise NotImplementedError

    def test_buffered_proto_create_connection(self):

        NOISE = b'12345678+' * 1024

        async def client(addr):
            data = b''

            def on_buf(buf):
                nonlocal data
                data += buf
                if data == NOISE:
                    tr.write(b'1')

            conn_lost_fut = self.loop.create_future()

            tr, pr = await self.loop.create_connection(
                lambda: ReceiveStuffProto(on_buf, conn_lost_fut), *addr)

            await conn_lost_fut

        async def on_server_client(reader, writer):
            writer.write(NOISE)
            await reader.readexactly(1)
            writer.close()
            await writer.wait_closed()

        srv = self.loop.run_until_complete(
            asyncio.start_server(
                on_server_client, '127.0.0.1', 0))

        addr = srv.sockets[0].getsockname()
        self.loop.run_until_complete(
            asyncio.wait_for(client(addr), 5, loop=self.loop))

        srv.close()
        self.loop.run_until_complete(srv.wait_closed())


class BufferedProtocolSelectorTests(BaseTestBufferedProtocol,
                                    unittest.TestCase):

    def new_loop(self):
        return asyncio.SelectorEventLoop()


@unittest.skipUnless(hasattr(asyncio, 'ProactorEventLoop'), 'Windows only')
class BufferedProtocolProactorTests(BaseTestBufferedProtocol,
                                    unittest.TestCase):

    def new_loop(self):
        return asyncio.ProactorEventLoop()


if __name__ == '__main__':
    unittest.main()
