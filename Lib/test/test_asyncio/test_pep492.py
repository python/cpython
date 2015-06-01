"""Tests support for new syntax introduced by PEP 492."""

import collections.abc
import unittest

from test import support
from unittest import mock

import asyncio
from asyncio import test_utils


class BaseTest(test_utils.TestCase):

    def setUp(self):
        self.loop = asyncio.BaseEventLoop()
        self.loop._process_events = mock.Mock()
        self.loop._selector = mock.Mock()
        self.loop._selector.select.return_value = ()
        self.set_event_loop(self.loop)


class LockTests(BaseTest):

    def test_context_manager_async_with(self):
        primitives = [
            asyncio.Lock(loop=self.loop),
            asyncio.Condition(loop=self.loop),
            asyncio.Semaphore(loop=self.loop),
            asyncio.BoundedSemaphore(loop=self.loop),
        ]

        async def test(lock):
            await asyncio.sleep(0.01, loop=self.loop)
            self.assertFalse(lock.locked())
            async with lock as _lock:
                self.assertIs(_lock, None)
                self.assertTrue(lock.locked())
                await asyncio.sleep(0.01, loop=self.loop)
                self.assertTrue(lock.locked())
            self.assertFalse(lock.locked())

        for primitive in primitives:
            self.loop.run_until_complete(test(primitive))
            self.assertFalse(primitive.locked())

    def test_context_manager_with_await(self):
        primitives = [
            asyncio.Lock(loop=self.loop),
            asyncio.Condition(loop=self.loop),
            asyncio.Semaphore(loop=self.loop),
            asyncio.BoundedSemaphore(loop=self.loop),
        ]

        async def test(lock):
            await asyncio.sleep(0.01, loop=self.loop)
            self.assertFalse(lock.locked())
            with await lock as _lock:
                self.assertIs(_lock, None)
                self.assertTrue(lock.locked())
                await asyncio.sleep(0.01, loop=self.loop)
                self.assertTrue(lock.locked())
            self.assertFalse(lock.locked())

        for primitive in primitives:
            self.loop.run_until_complete(test(primitive))
            self.assertFalse(primitive.locked())


class StreamReaderTests(BaseTest):

    def test_readline(self):
        DATA = b'line1\nline2\nline3'

        stream = asyncio.StreamReader(loop=self.loop)
        stream.feed_data(DATA)
        stream.feed_eof()

        async def reader():
            data = []
            async for line in stream:
                data.append(line)
            return data

        data = self.loop.run_until_complete(reader())
        self.assertEqual(data, [b'line1\n', b'line2\n', b'line3'])


class CoroutineTests(BaseTest):

    def test_iscoroutine(self):
        async def foo(): pass

        f = foo()
        try:
            self.assertTrue(asyncio.iscoroutine(f))
        finally:
            f.close() # silence warning

        # Test that asyncio.iscoroutine() uses collections.abc.Coroutine
        class FakeCoro:
            def send(self, value): pass
            def throw(self, typ, val=None, tb=None): pass
            def close(self): pass
            def __await__(self): yield

        self.assertTrue(asyncio.iscoroutine(FakeCoro()))

    def test_function_returning_awaitable(self):
        class Awaitable:
            def __await__(self):
                return ('spam',)

        @asyncio.coroutine
        def func():
            return Awaitable()

        coro = func()
        self.assertEqual(coro.send(None), 'spam')
        coro.close()

    def test_async_ded_coroutines(self):
        async def bar():
            return 'spam'
        async def foo():
            return await bar()

        # production mode
        data = self.loop.run_until_complete(foo())
        self.assertEqual(data, 'spam')

        # debug mode
        self.loop.set_debug(True)
        data = self.loop.run_until_complete(foo())
        self.assertEqual(data, 'spam')

if __name__ == '__main__':
    unittest.main()
