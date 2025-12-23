"""Tests for asyncio/threads.py"""

import asyncio
import unittest

from contextvars import ContextVar
from unittest import mock


def tearDownModule():
    asyncio.events._set_event_loop_policy(None)


class ToThreadTests(unittest.IsolatedAsyncioTestCase):
    async def test_to_thread(self):
        result = await asyncio.to_thread(sum, [40, 2])
        self.assertEqual(result, 42)

    async def test_to_thread_exception(self):
        def raise_runtime():
            raise RuntimeError("test")

        with self.assertRaisesRegex(RuntimeError, "test"):
            await asyncio.to_thread(raise_runtime)

    async def test_to_thread_once(self):
        func = mock.Mock()

        await asyncio.to_thread(func)
        func.assert_called_once()

    async def test_to_thread_concurrent(self):
        calls = []
        def func():
            calls.append(1)

        futs = []
        for _ in range(10):
            fut = asyncio.to_thread(func)
            futs.append(fut)
        await asyncio.gather(*futs)

        self.assertEqual(sum(calls), 10)

    async def test_to_thread_args_kwargs(self):
        # Unlike run_in_executor(), to_thread() should directly accept kwargs.
        func = mock.Mock()

        await asyncio.to_thread(func, 'test', something=True)

        func.assert_called_once_with('test', something=True)

    async def test_to_thread_contextvars(self):
        test_ctx = ContextVar('test_ctx')

        def get_ctx():
            return test_ctx.get()

        test_ctx.set('parrot')
        result = await asyncio.to_thread(get_ctx)

        self.assertEqual(result, 'parrot')


if __name__ == "__main__":
    unittest.main()
