"""Tests support for new syntax introduced by PEP 492."""

import sys
import types
import unittest

from test import support
from unittest import mock

import asyncio
from test.test_asyncio import utils as test_utils


def tearDownModule():
    asyncio.set_event_loop_policy(None)


# Test that asyncio.iscoroutine() uses collections.abc.Coroutine
class FakeCoro:
    def send(self, value):
        pass

    def throw(self, typ, val=None, tb=None):
        pass

    def close(self):
        pass

    def __await__(self):
        yield


class BaseTest(test_utils.TestCase):

    def setUp(self):
        super().setUp()
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
            with self.assertWarns(DeprecationWarning):
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

        self.assertTrue(asyncio.iscoroutine(FakeCoro()))

    def test_iscoroutinefunction(self):
        async def foo(): pass
        self.assertTrue(asyncio.iscoroutinefunction(foo))

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

    def test_async_def_coroutines(self):
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

    def test_debug_mode_manages_coroutine_origin_tracking(self):
        async def start():
            self.assertTrue(sys.get_coroutine_origin_tracking_depth() > 0)

        self.assertEqual(sys.get_coroutine_origin_tracking_depth(), 0)
        self.loop.set_debug(True)
        self.loop.run_until_complete(start())
        self.assertEqual(sys.get_coroutine_origin_tracking_depth(), 0)

    def test_types_coroutine(self):
        def gen():
            yield from ()
            return 'spam'

        @types.coroutine
        def func():
            return gen()

        async def coro():
            wrapper = func()
            self.assertIsInstance(wrapper, types._GeneratorWrapper)
            return await wrapper

        data = self.loop.run_until_complete(coro())
        self.assertEqual(data, 'spam')

    def test_task_print_stack(self):
        T = None

        async def foo():
            f = T.get_stack(limit=1)
            try:
                self.assertEqual(f[0].f_code.co_name, 'foo')
            finally:
                f = None

        async def runner():
            nonlocal T
            T = asyncio.ensure_future(foo(), loop=self.loop)
            await T

        self.loop.run_until_complete(runner())

    def test_double_await(self):
        async def afunc():
            await asyncio.sleep(0.1, loop=self.loop)

        async def runner():
            coro = afunc()
            t = asyncio.Task(coro, loop=self.loop)
            try:
                await asyncio.sleep(0, loop=self.loop)
                await coro
            finally:
                t.cancel()

        self.loop.set_debug(True)
        with self.assertRaises(
                RuntimeError,
                msg='coroutine is being awaited already'):

            self.loop.run_until_complete(runner())


if __name__ == '__main__':
    unittest.main()
