"""Tests for asyncio/threads.py"""

import asyncio
import unittest
import functools
import warnings

from contextvars import ContextVar
from unittest import mock


def tearDownModule():
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", DeprecationWarning)
        asyncio.set_event_loop_policy(None)


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

    @mock.patch('asyncio.base_events.BaseEventLoop.run_in_executor')
    async def test_to_thread_optimization_path(self, run_in_executor):
        # This test ensures that `to_thread` uses the correct execution path
        # based on whether the context is empty or not.

        # `to_thread` awaits the future returned by `run_in_executor`.
        # We need to provide a completed future as a return value for the mock.
        fut = asyncio.Future()
        fut.set_result(None)
        run_in_executor.return_value = fut

        def myfunc():
            pass

        # Test with an empty context (optimized path)
        await asyncio.to_thread(myfunc)
        run_in_executor.assert_called_once()

        callback = run_in_executor.call_args.args[1]
        self.assertIsInstance(callback, functools.partial)
        self.assertIs(callback.func, myfunc)
        run_in_executor.reset_mock()

        # Test with a non-empty context (standard path)
        var = ContextVar('var')
        var.set('value')

        await asyncio.to_thread(myfunc)
        run_in_executor.assert_called_once()

        callback = run_in_executor.call_args.args[1]
        self.assertIsInstance(callback, functools.partial)
        self.assertIsNot(callback.func, myfunc)  # Should be ctx.run
        self.assertIs(callback.args[0], myfunc)


if __name__ == "__main__":
    unittest.main()
