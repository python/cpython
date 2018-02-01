import os
import socket
import sys
import unittest
from unittest import mock

if sys.platform != 'win32':
    raise unittest.SkipTest('Windows only')

import _overlapped
import _winapi

import asyncio
from asyncio import windows_events
from test.test_asyncio import utils as test_utils


class UpperProto(asyncio.Protocol):
    def __init__(self):
        self.buf = []

    def connection_made(self, trans):
        self.trans = trans

    def data_received(self, data):
        self.buf.append(data)
        if b'\n' in data:
            self.trans.write(b''.join(self.buf).upper())
            self.trans.close()


class ProactorTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = asyncio.ProactorEventLoop()
        self.set_event_loop(self.loop)

    def test_close(self):
        a, b = socket.socketpair()
        trans = self.loop._make_socket_transport(a, asyncio.Protocol())
        f = asyncio.ensure_future(self.loop.sock_recv(b, 100), loop=self.loop)
        trans.close()
        self.loop.run_until_complete(f)
        self.assertEqual(f.result(), b'')
        b.close()

    def test_double_bind(self):
        ADDRESS = r'\\.\pipe\test_double_bind-%s' % os.getpid()
        server1 = windows_events.PipeServer(ADDRESS)
        with self.assertRaises(PermissionError):
            windows_events.PipeServer(ADDRESS)
        server1.close()

    def test_pipe(self):
        res = self.loop.run_until_complete(self._test_pipe())
        self.assertEqual(res, 'done')

    async def _test_pipe(self):
        ADDRESS = r'\\.\pipe\_test_pipe-%s' % os.getpid()

        with self.assertRaises(FileNotFoundError):
            await self.loop.create_pipe_connection(
                asyncio.Protocol, ADDRESS)

        [server] = await self.loop.start_serving_pipe(
            UpperProto, ADDRESS)
        self.assertIsInstance(server, windows_events.PipeServer)

        clients = []
        for i in range(5):
            stream_reader = asyncio.StreamReader(loop=self.loop)
            protocol = asyncio.StreamReaderProtocol(stream_reader,
                                                    loop=self.loop)
            trans, proto = await self.loop.create_pipe_connection(
                lambda: protocol, ADDRESS)
            self.assertIsInstance(trans, asyncio.Transport)
            self.assertEqual(protocol, proto)
            clients.append((stream_reader, trans))

        for i, (r, w) in enumerate(clients):
            w.write('lower-{}\n'.format(i).encode())

        for i, (r, w) in enumerate(clients):
            response = await r.readline()
            self.assertEqual(response, 'LOWER-{}\n'.format(i).encode())
            w.close()

        server.close()

        with self.assertRaises(FileNotFoundError):
            await self.loop.create_pipe_connection(
                asyncio.Protocol, ADDRESS)

        return 'done'

    def test_connect_pipe_cancel(self):
        exc = OSError()
        exc.winerror = _overlapped.ERROR_PIPE_BUSY
        with mock.patch.object(_overlapped, 'ConnectPipe',
                               side_effect=exc) as connect:
            coro = self.loop._proactor.connect_pipe('pipe_address')
            task = self.loop.create_task(coro)

            # check that it's possible to cancel connect_pipe()
            task.cancel()
            with self.assertRaises(asyncio.CancelledError):
                self.loop.run_until_complete(task)

    def test_wait_for_handle(self):
        event = _overlapped.CreateEvent(None, True, False, None)
        self.addCleanup(_winapi.CloseHandle, event)

        # Wait for unset event with 0.5s timeout;
        # result should be False at timeout
        fut = self.loop._proactor.wait_for_handle(event, 0.5)
        start = self.loop.time()
        done = self.loop.run_until_complete(fut)
        elapsed = self.loop.time() - start

        self.assertEqual(done, False)
        self.assertFalse(fut.result())
        # bpo-31008: Tolerate only 450 ms (at least 500 ms expected),
        # because of bad clock resolution on Windows
        self.assertTrue(0.45 <= elapsed <= 0.9, elapsed)

        _overlapped.SetEvent(event)

        # Wait for set event;
        # result should be True immediately
        fut = self.loop._proactor.wait_for_handle(event, 10)
        start = self.loop.time()
        done = self.loop.run_until_complete(fut)
        elapsed = self.loop.time() - start

        self.assertEqual(done, True)
        self.assertTrue(fut.result())
        self.assertTrue(0 <= elapsed < 0.3, elapsed)

        # asyncio issue #195: cancelling a done _WaitHandleFuture
        # must not crash
        fut.cancel()

    def test_wait_for_handle_cancel(self):
        event = _overlapped.CreateEvent(None, True, False, None)
        self.addCleanup(_winapi.CloseHandle, event)

        # Wait for unset event with a cancelled future;
        # CancelledError should be raised immediately
        fut = self.loop._proactor.wait_for_handle(event, 10)
        fut.cancel()
        start = self.loop.time()
        with self.assertRaises(asyncio.CancelledError):
            self.loop.run_until_complete(fut)
        elapsed = self.loop.time() - start
        self.assertTrue(0 <= elapsed < 0.1, elapsed)

        # asyncio issue #195: cancelling a _WaitHandleFuture twice
        # must not crash
        fut = self.loop._proactor.wait_for_handle(event)
        fut.cancel()
        fut.cancel()


if __name__ == '__main__':
    unittest.main()
