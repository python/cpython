import asyncio
import select
import socket
import time
import unittest
from asyncio._selector_thread import SelectorThread
from unittest import mock


class SelectorThreadTest((unittest.IsolatedAsyncioTestCase)):
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
        def recv():
            b.recv(100)
        mock_recv = mock.MagicMock(wraps=recv)
        # make sure select is only called once when
        # event loop thread is slow to consume events
        a.sendall(b"msg")
        with mock.patch("select.select", wraps=select.select) as mock_select:
            self.selector_thread.add_reader(b, mock_recv)
            time.sleep(0.1)
        self.assertEqual(mock_select.call_count, 1)
        await asyncio.sleep(0.1)
        self.assertEqual(mock_recv.call_count, 1)

    async def test_reader_error(self):
        # test error handling in callbacks doesn't break handling
        a, b = self.socketpair()
        a.sendall(b"to_b")

        selector_thread = self.selector_thread

        # make sure it's called a few
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
        with mock.patch.object(selector_thread, "_start_select", wraps=selector_thread._start_select) as start_select:
            await asyncio.wait_for(bad_recv_done, timeout=10)

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
        assert b.fileno() in selector_thread._readers
        selector_thread.add_writer(a, write)
        assert a.fileno() in selector_thread._writers
        msg = await asyncio.wait_for(read_future, timeout=10)

        selector_thread.remove_writer(a)
        assert a.fileno() not in selector_thread._writers
        selector_thread.remove_reader(b)
        assert b.fileno() not in selector_thread._readers
        a.close()
        b.close()
        assert msg == sent
