import os
import signal
import socket
import sys
import time
import threading
import unittest
from unittest import mock

if sys.platform != 'win32':
    raise unittest.SkipTest('Windows only')

import _overlapped
import _winapi

import asyncio
from asyncio import windows_events
from test.test_asyncio import utils as test_utils


def tearDownModule():
    asyncio.set_event_loop_policy(None)


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


class ProactorLoopCtrlC(test_utils.TestCase):

    def test_ctrl_c(self):

        def SIGINT_after_delay():
            time.sleep(0.1)
            signal.raise_signal(signal.SIGINT)

        thread = threading.Thread(target=SIGINT_after_delay)
        loop = asyncio.new_event_loop()
        try:
            # only start the loop once the event loop is running
            loop.call_soon(thread.start)
            loop.run_forever()
            self.fail("should not fall through 'run_forever'")
        except KeyboardInterrupt:
            pass
        finally:
            self.close_loop(loop)
        thread.join()


class ProactorMultithreading(test_utils.TestCase):
    def test_run_from_nonmain_thread(self):
        finished = False

        async def coro():
            await asyncio.sleep(0)

        def func():
            nonlocal finished
            loop = asyncio.new_event_loop()
            loop.run_until_complete(coro())
            # close() must not call signal.set_wakeup_fd()
            loop.close()
            finished = True

        thread = threading.Thread(target=func)
        thread.start()
        thread.join()
        self.assertTrue(finished)


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

    def test_read_self_pipe_restart(self):
        # Regression test for https://bugs.python.org/issue39010
        # Previously, restarting a proactor event loop in certain states
        # would lead to spurious ConnectionResetErrors being logged.
        self.loop.call_exception_handler = mock.Mock()
        # Start an operation in another thread so that the self-pipe is used.
        # This is theoretically timing-dependent (the task in the executor
        # must complete before our start/stop cycles), but in practice it
        # seems to work every time.
        f = self.loop.run_in_executor(None, lambda: None)
        self.loop.stop()
        self.loop.run_forever()
        self.loop.stop()
        self.loop.run_forever()

        # Shut everything down cleanly. This is an important part of the
        # test - in issue 39010, the error occurred during loop.close(),
        # so we want to close the loop during the test instead of leaving
        # it for tearDown.
        #
        # First wait for f to complete to avoid a "future's result was never
        # retrieved" error.
        self.loop.run_until_complete(f)
        # Now shut down the loop itself (self.close_loop also shuts down the
        # loop's default executor).
        self.close_loop(self.loop)
        self.assertFalse(self.loop.call_exception_handler.called)

    def test_address_argument_type_error(self):
        # Regression test for https://github.com/python/cpython/issues/98793
        proactor = self.loop._proactor
        sock = socket.socket(type=socket.SOCK_DGRAM)
        bad_address = None
        with self.assertRaises(TypeError):
            proactor.connect(sock, bad_address)
        with self.assertRaises(TypeError):
            proactor.sendto(sock, b'abc', addr=bad_address)
        sock.close()

    def test_client_pipe_stat(self):
        res = self.loop.run_until_complete(self._test_client_pipe_stat())
        self.assertEqual(res, 'done')

    async def _test_client_pipe_stat(self):
        # Regression test for https://github.com/python/cpython/issues/100573
        ADDRESS = r'\\.\pipe\test_client_pipe_stat-%s' % os.getpid()

        async def probe():
            # See https://github.com/python/cpython/pull/100959#discussion_r1068533658
            h = _overlapped.ConnectPipe(ADDRESS)
            try:
                _winapi.CloseHandle(_overlapped.ConnectPipe(ADDRESS))
            except OSError as e:
                if e.winerror != _overlapped.ERROR_PIPE_BUSY:
                    raise
            finally:
                _winapi.CloseHandle(h)

        with self.assertRaises(FileNotFoundError):
            await probe()

        [server] = await self.loop.start_serving_pipe(asyncio.Protocol, ADDRESS)
        self.assertIsInstance(server, windows_events.PipeServer)

        errors = []
        self.loop.set_exception_handler(lambda _, data: errors.append(data))

        for i in range(5):
            await self.loop.create_task(probe())

        self.assertEqual(len(errors), 0, errors)

        server.close()

        with self.assertRaises(FileNotFoundError):
            await probe()

        return "done"


class WinPolicyTests(test_utils.TestCase):

    def test_selector_win_policy(self):
        async def main():
            self.assertIsInstance(
                asyncio.get_running_loop(),
                asyncio.SelectorEventLoop)

        old_policy = asyncio.get_event_loop_policy()
        try:
            asyncio.set_event_loop_policy(
                asyncio.WindowsSelectorEventLoopPolicy())
            asyncio.run(main())
        finally:
            asyncio.set_event_loop_policy(old_policy)

    def test_proactor_win_policy(self):
        async def main():
            self.assertIsInstance(
                asyncio.get_running_loop(),
                asyncio.ProactorEventLoop)

        old_policy = asyncio.get_event_loop_policy()
        try:
            asyncio.set_event_loop_policy(
                asyncio.WindowsProactorEventLoopPolicy())
            asyncio.run(main())
        finally:
            asyncio.set_event_loop_policy(old_policy)


if __name__ == '__main__':
    unittest.main()
