"""Tests for asyncio/threads.py"""

import asyncio
import unittest

from unittest import mock
from test.test_asyncio import utils as test_utils


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class ToThreadTests(test_utils.TestCase):
    def setUp(self):
        super().setUp()
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)

    def tearDown(self):
        self.loop.run_until_complete(
            self.loop.shutdown_default_executor())
        self.loop.close()
        asyncio.set_event_loop(None)
        self.loop = None
        super().tearDown()

    def test_to_thread(self):
        async def main():
            return await asyncio.to_thread(sum, [40, 2])

        result = self.loop.run_until_complete(main())
        self.assertEqual(result, 42)

    def test_to_thread_exception(self):
        def raise_runtime():
            raise RuntimeError("test")

        async def main():
            await asyncio.to_thread(raise_runtime)

        with self.assertRaisesRegex(RuntimeError, "test"):
            self.loop.run_until_complete(main())

    def test_to_thread_once(self):
        func = mock.Mock()

        async def main():
            await asyncio.to_thread(func)

        self.loop.run_until_complete(main())
        func.assert_called_once()

    def test_to_thread_concurrent(self):
        func = mock.Mock()

        async def main():
            futs = []
            for _ in range(10):
                fut = asyncio.to_thread(func)
                futs.append(fut)
            await asyncio.gather(*futs)

        self.loop.run_until_complete(main())
        self.assertEqual(func.call_count, 10)

    def test_to_thread_args_kwargs(self):
        # Unlike run_in_executor(), to_thread() should directly accept kwargs.
        func = mock.Mock()

        async def main():
            await asyncio.to_thread(func, 'test', something=True)

        self.loop.run_until_complete(main())
        func.assert_called_once_with('test', something=True)


if __name__ == "__main__":
    unittest.main()
