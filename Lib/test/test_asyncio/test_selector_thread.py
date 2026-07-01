import asyncio
import select
import socket
import time
import unittest
from asyncio._selector_thread import SelectorThread
from test import support
from unittest import mock


def tearDownModule():
    asyncio.events._set_event_loop_policy(None)


class SelectorThreadTest(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self._sockets = []
        self.selector_thread = SelectorThread(asyncio.get_running_loop())

    def socketpair(self):
        pair = socket.socketpair()
        self._sockets.extend(pair)
        return pair

    async def asyncTearDown(self):
        self.selector_thread.close()
        for s in self._sockets:
            s.close()

    async def test_slow_reader(self):
        a, b = self.socketpair()
        first_recv = asyncio.Future()

        def recv():
            msg = b.recv(100)
            if not first_recv.done():
                first_recv.set_result(msg)

        mock_recv = mock.MagicMock(wraps=recv)
        # make sure select is only called once when
        # event loop thread is slow to consume events
        a.sendall(b"msg")
        with mock.patch("select.select", wraps=select.select) as mock_select:
            self.selector_thread.add_reader(b, mock_recv)
            # ready event, but main event loop is blocked for some time
            time.sleep(0.1)
            recvd = await asyncio.wait_for(first_recv, timeout=support.SHORT_TIMEOUT)
        self.assertEqual(recvd, b"msg")
        # make sure recv wasn't scheduled more than once
        self.assertEqual(mock_recv.call_count, 1)
        # 1 for add_reader
        # 1 for finishing reader callback
        # up to 2 more for wake FD calls if CI is slow
        # this would be thousands if select is busy-looping while the main thread blocks
        self.assertLessEqual(mock_select.call_count, 5)

    async def test_reader_error(self):
        # test error handling in callbacks doesn't break handling
        a, b = self.socketpair()
        a.sendall(b"to_b")

        selector_thread = self.selector_thread

        # make sure it's called a few times,
        # and errors don't prevent rescheduling
        n_failures = 5
        counter = 0
        bad_recv_done = asyncio.Future()

        def bad_recv(sock):
            # fail the first n_failures calls, then succeed
            nonlocal counter
            counter += 1
            if counter > n_failures:
                bad_recv_done.set_result(None)
                sock.recv(10)
                return
            raise Exception("Testing reader error")

        recv_callback = mock.MagicMock(wraps=bad_recv)

        exception_handler = mock.MagicMock()
        asyncio.get_running_loop().set_exception_handler(exception_handler)

        selector_thread.add_reader(b, recv_callback, b)

        # make sure start_select is called
        # even when recv callback errors,
        with mock.patch.object(
            selector_thread, "_start_select", wraps=selector_thread._start_select
        ) as start_select:
            await asyncio.wait_for(bad_recv_done, timeout=support.SHORT_TIMEOUT)

        # make sure recv is called N + 1 times,
        # exception N times,
        # start_select at least that many
        self.assertEqual(recv_callback.call_count, n_failures + 1)
        self.assertEqual(exception_handler.call_count, n_failures)
        self.assertGreaterEqual(start_select.call_count, n_failures)

    async def test_read_write(self):
        a, b = self.socketpair()
        read_future = asyncio.Future()
        sent = b"asdf"
        loop = asyncio.get_running_loop()
        selector_thread = self.selector_thread

        def write():
            a.sendall(sent)
            loop.remove_writer(a)

        def read():
            msg = b.recv(100)
            read_future.set_result(msg)

        selector_thread.add_reader(b, read)
        self.assertIn(b.fileno(), selector_thread._readers)
        selector_thread.add_writer(a, write)
        self.assertIn(a.fileno(), selector_thread._writers)
        msg = await asyncio.wait_for(read_future, timeout=10)

        selector_thread.remove_writer(a)
        self.assertNotIn(a.fileno(), selector_thread._writers)
        selector_thread.remove_reader(b)
        self.assertNotIn(b.fileno(), selector_thread._readers)
        a.close()
        b.close()
        self.assertEqual(msg, sent)
