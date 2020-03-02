"""Tests for pools.py"""

import asyncio
import unittest

from unittest import mock


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class AbstractPoolTests(unittest.TestCase):

    def setUp(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)

    def tearDown(self):
        asyncio.set_event_loop(None)
        self.loop.close()

    def test_methods_not_implemented(self):
        with self.assertRaises(TypeError):
            asyncio.pools.AbstractPool()

    def test_context_manager_not_implemented(self):
        async def pool_cm():
            async with asyncio.pools.AbstractPool() as pool:
                pass

        with self.assertRaises(TypeError):
            self.loop.run_until_complete(pool_cm())


class ThreadPoolTests(unittest.TestCase):

    def setUp(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)

    def tearDown(self):
        self.loop.close()
        asyncio.set_event_loop(None)
        self.loop = None

    def test_running(self):
        async def main():
            async with asyncio.ThreadPool() as pool:
                self.assertTrue(pool._running)
                self.assertFalse(pool._closed)

        self.loop.run_until_complete(main())

    def test_closed(self):
        async def main():
            async with asyncio.ThreadPool() as pool:
                pass

            self.assertTrue(pool._closed)
            self.assertFalse(pool._running)

        self.loop.run_until_complete(main())

    def test_concurrency(self):
        async def main():
            async with asyncio.ThreadPool() as pool:
                # We can't guarantee the number of threads since it's system
                # specific, but it should never be None after the thread pool
                # is started.
                self.assertIsNotNone(pool._concurrency)

        self.loop.run_until_complete(main())

    def test_concurrency_explicit(self):
        async def main():
            async with asyncio.ThreadPool(concurrency=5) as pool:
                self.assertEqual(pool._concurrency, 5)

        self.loop.run_until_complete(main())

    def test_run(self):
        async def main():
            async with asyncio.ThreadPool(concurrency=5) as pool:
                return await pool.run(sum, [40, 2])

        result = self.loop.run_until_complete(main())
        self.assertEqual(result, 42)

    def test_run_once(self):
        func = mock.Mock()

        async def main():
            async with asyncio.ThreadPool(concurrency=5) as pool:
                await pool.run(func)

        self.loop.run_until_complete(main())
        func.assert_called_once()

    def test_run_concurrent(self):
        func = mock.Mock()

        async def main():
            futs = []
            async with asyncio.ThreadPool(concurrency=5) as pool:
                for _ in range(10):
                    fut = pool.run(func)
                    futs.append(fut)
                await asyncio.gather(*futs)

        self.loop.run_until_complete(main())
        self.assertEqual(func.call_count, 10)

    def test_run_after_exit(self):
        func = mock.Mock()

        async def main():
            async with asyncio.ThreadPool() as pool:
                pass

            with self.assertRaises(RuntimeError):
                await pool.run(func)

        self.loop.run_until_complete(main())
        func.assert_not_called()

    def test_run_before_start(self):
        func = mock.Mock()

        async def main():
            pool = asyncio.ThreadPool()
            with self.assertRaises(RuntimeError):
                await pool.run(func)

        self.loop.run_until_complete(main())
        func.assert_not_called()

    def test_start_twice(self):
        async def main():
            pool = asyncio.ThreadPool()
            async with pool:
                with self.assertRaises(RuntimeError):
                    async with pool:
                        pass

        self.loop.run_until_complete(main())

    def test_start_after_close(self):
        async def main():
            pool = asyncio.ThreadPool()
            async with pool:
                pass

            with self.assertRaises(RuntimeError):
                async with pool:
                    pass

        self.loop.run_until_complete(main())

    def test_close_twice(self):
        async def main():
            pool = asyncio.ThreadPool()
            async with pool:
                pass

            with self.assertRaises(RuntimeError):
                await pool.aclose()

        self.loop.run_until_complete(main())


if __name__ == '__main__':
    unittest.main()
