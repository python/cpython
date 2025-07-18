import asyncio
import itertools
import unittest
from asyncio import Executor


class ExecutorSubmitTests(unittest.IsolatedAsyncioTestCase):

    async def test_submit_sleeping_function(self):
        async with asyncio.timeout(1), Executor(max_workers=2) as executor:
            async def async_fn(x):
                await asyncio.sleep(0.1)
                return x * 2

            future = await executor.submit(async_fn, 5)
            result = await future
            self.assertEqual(result, 10)

    async def test_submit_function_raises_exception(self):
        async with asyncio.timeout(1), Executor(max_workers=2) as executor:
            async def sync_fn(x):
                raise ValueError("Test exception")

            future = await executor.submit(sync_fn, 5)
            with self.assertRaises(ValueError):
                await future

    async def test_submit_cancel_task(self):
        async with asyncio.timeout(2), Executor(max_workers=2) as executor:
            was_not_cancelled = False

            async def async_fn(x):
                nonlocal was_not_cancelled
                await asyncio.sleep(1)
                was_not_cancelled = True
                return x * 2

            future = await executor.submit(async_fn, 5)
            await asyncio.sleep(0.1)
            future.cancel()
            with self.assertRaises(asyncio.CancelledError):
                await future
            self.assertFalse(was_not_cancelled)


class ExecutorMapTests(unittest.IsolatedAsyncioTestCase):

    async def test_map_sleeping_function(self):
        async with asyncio.timeout(1), Executor(max_workers=2) as executor:
            async def async_fn(x):
                await asyncio.sleep(0.1)
                return x * 2

            results = [
                result
                async for result in executor.map(async_fn, range(5))
            ]
            self.assertEqual(results, [0, 2, 4, 6, 8])

    async def test_map_function_raises_exception(self):
        async with asyncio.timeout(1), Executor(max_workers=2) as executor:
            async def sync_fn(x):
                if x == 3:
                    raise ValueError("Test exception")
                return x * 2

            with self.assertRaises(ValueError):
                _ = [
                    result
                    async for result in executor.map(sync_fn, range(5))
                ]

    async def test_map_cancel_task(self):
        async with asyncio.timeout(3), Executor(max_workers=2) as executor:
            was_not_cancelled = False

            async def async_fn(x):
                nonlocal was_not_cancelled
                await asyncio.sleep(1)
                if x == 3:
                    was_not_cancelled = True
                return x * 2

            async for _ in executor.map(async_fn, range(5)):
                # There are 2 workers, therefore the first 2 tasks will be
                # completed when we reach the break statement, at which point
                # the third task would have started.
                break

            await asyncio.sleep(0.1)  # Make sure the 3rd task is running.
            await executor.shutdown(cancel_futures=True)
            self.assertFalse(was_not_cancelled)


class ExecutorStressTests(unittest.IsolatedAsyncioTestCase):

    async def test_map_with_large_number_of_tasks(self):
        async with asyncio.timeout(4), Executor(max_workers=64) as executor:
            async def sync_fn(x):
                return x * 2

            results = [
                result
                async for result in executor.map(sync_fn, range(1000))
            ]
            self.assertEqual(results, [x * 2 for x in range(1000)])

    async def test_map_with_infinite_iterable(self):
        async with asyncio.timeout(1), Executor(max_workers=2) as executor:
            async def sync_fn(x):
                return x * 2

            results = []
            async for result in executor.map(sync_fn, itertools.count()):
                results.append(result)
                if len(results) >= 10:
                    break
            self.assertEqual(results, [x * 2 for x in range(10)])


class ExecutorEdgeCasesTests(unittest.IsolatedAsyncioTestCase):

    async def test_map_timeout(self):
        async with asyncio.timeout(2), Executor(max_workers=2) as executor:
            async def async_fn(x):
                await asyncio.sleep(1)
                return x * 2

            with self.assertRaises(asyncio.TimeoutError):
                _ = [
                    result
                    async for result in executor.map(
                        async_fn,
                        range(5),
                        timeout=0.5,
                    )
                ]

    async def test_shutdown_while_tasks_running(self):
        async with asyncio.timeout(3), Executor(max_workers=2) as executor:
            async def async_fn(x):
                await asyncio.sleep(1)
                return x * 2

            futures = [await executor.submit(async_fn, i) for i in range(5)]
            # Since we used submit instead of map, and the input queue is 2,
            # we have 2 finished tasks, 2 running tasks, and 1 pending task.
            # Therefore, the graceful shutdown will let the 2 running tasks
            # finish, and cancel the pending task.
            await executor.shutdown(cancel_futures=True)
            for future in futures[:4]:
                await future
            with self.assertRaises(asyncio.CancelledError):
                await futures[-1]

    async def test_resource_cleanup(self):
        async with asyncio.timeout(1), Executor(max_workers=2) as executor:
            async def async_fn(x):
                await asyncio.sleep(0.1)
                return x * 2

            futures = [await executor.submit(async_fn, i) for i in range(5)]
            await executor.shutdown()
            for future in futures:
                self.assertTrue(future.done())

    async def test_reject_submit_on_shutdown_executor(self):
        executor = Executor(max_workers=2)
        await executor.shutdown()
        with self.assertRaises(RuntimeError):
            await executor.submit(lambda x: x, 5)

    async def test_reject_map_on_shutdown_executor(self):
        executor = Executor(max_workers=2)
        await executor.shutdown()
        with self.assertRaises(RuntimeError):
            async for _ in executor.map(lambda x: x, range(5)):
                pass

    async def test_submitted_task_cancellation_after_leaving_context(self):
        # This test also verifies that all created async generators are
        # terminated gracefully, which requires gracefully terminating the
        # feeders.
        async with asyncio.timeout(1), Executor(max_workers=1) as executor:
            async def async_fn(x):
                await asyncio.sleep(0.1)
                return x * 2

            results = []
            results_stream = executor.map(async_fn, range(3))
            results = [await anext(results_stream)]
        # Exiting the async content manager calls shutdown(wait=True,
        # cancel_futures=False). With max_workers=1, the first task will be
        # running, and the second task will be pending in queue. The third task
        # will not be submitted yet. Therefore, the first and second tasks will
        # be completed, and the third task will be cancelled (cancel_futures
        # cancels already scheduled tasks).

        await asyncio.sleep(0.1)

        results += [
            result
            async for result in results_stream
        ]
        self.assertEqual(results, [0, 2])


if __name__ == '__main__':
    unittest.main()
