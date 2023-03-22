"""Tests for asyncio/timeouts.py"""

import unittest
import time

import asyncio
from asyncio import tasks


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class TimeoutTests(unittest.IsolatedAsyncioTestCase):

    async def test_timeout_basic(self):
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.01) as cm:
                await asyncio.sleep(10)
        self.assertTrue(cm.expired())

    async def test_timeout_at_basic(self):
        loop = asyncio.get_running_loop()

        with self.assertRaises(TimeoutError):
            deadline = loop.time() + 0.01
            async with asyncio.timeout_at(deadline) as cm:
                await asyncio.sleep(10)
        self.assertTrue(cm.expired())
        self.assertEqual(deadline, cm.when())

    async def test_nested_timeouts(self):
        loop = asyncio.get_running_loop()
        cancelled = False
        with self.assertRaises(TimeoutError):
            deadline = loop.time() + 0.01
            async with asyncio.timeout_at(deadline) as cm1:
                # Only the topmost context manager should raise TimeoutError
                try:
                    async with asyncio.timeout_at(deadline) as cm2:
                        await asyncio.sleep(10)
                except asyncio.CancelledError:
                    cancelled = True
                    raise
        self.assertTrue(cancelled)
        self.assertTrue(cm1.expired())
        self.assertTrue(cm2.expired())

    async def test_waiter_cancelled(self):
        loop = asyncio.get_running_loop()
        cancelled = False
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.01):
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    cancelled = True
                    raise
        self.assertTrue(cancelled)

    async def test_timeout_not_called(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        async with asyncio.timeout(10) as cm:
            await asyncio.sleep(0.01)
        t1 = loop.time()

        self.assertFalse(cm.expired())
        # 2 sec for slow CI boxes
        self.assertLess(t1-t0, 2)
        self.assertGreater(cm.when(), t1)

    async def test_timeout_disabled(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        async with asyncio.timeout(None) as cm:
            await asyncio.sleep(0.01)
        t1 = loop.time()

        self.assertFalse(cm.expired())
        self.assertIsNone(cm.when())
        # 2 sec for slow CI boxes
        self.assertLess(t1-t0, 2)

    async def test_timeout_at_disabled(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        async with asyncio.timeout_at(None) as cm:
            await asyncio.sleep(0.01)
        t1 = loop.time()

        self.assertFalse(cm.expired())
        self.assertIsNone(cm.when())
        # 2 sec for slow CI boxes
        self.assertLess(t1-t0, 2)

    async def test_timeout_zero(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0) as cm:
                await asyncio.sleep(10)
        t1 = loop.time()
        self.assertTrue(cm.expired())
        # 2 sec for slow CI boxes
        self.assertLess(t1-t0, 2)
        self.assertTrue(t0 <= cm.when() <= t1)

    async def test_timeout_zero_sleep_zero(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0) as cm:
                await asyncio.sleep(0)
        t1 = loop.time()
        self.assertTrue(cm.expired())
        # 2 sec for slow CI boxes
        self.assertLess(t1-t0, 2)
        self.assertTrue(t0 <= cm.when() <= t1)

    async def test_timeout_in_the_past_sleep_zero(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(-11) as cm:
                await asyncio.sleep(0)
        t1 = loop.time()
        self.assertTrue(cm.expired())
        # 2 sec for slow CI boxes
        self.assertLess(t1-t0, 2)
        self.assertTrue(t0 >= cm.when() <= t1)

    async def test_foreign_exception_passed(self):
        with self.assertRaises(KeyError):
            async with asyncio.timeout(0.01) as cm:
                raise KeyError
        self.assertFalse(cm.expired())

    async def test_foreign_exception_on_timeout(self):
        async def crash():
            try:
                await asyncio.sleep(1)
            finally:
                1/0
        with self.assertRaises(ZeroDivisionError):
            async with asyncio.timeout(0.01):
                await crash()

    async def test_foreign_cancel_doesnt_timeout_if_not_expired(self):
        with self.assertRaises(asyncio.CancelledError):
            async with asyncio.timeout(10) as cm:
                asyncio.current_task().cancel()
                await asyncio.sleep(10)
        self.assertFalse(cm.expired())

    async def test_outer_task_is_not_cancelled(self):
        async def outer() -> None:
            with self.assertRaises(TimeoutError):
                async with asyncio.timeout(0.001):
                    await asyncio.sleep(10)

        task = asyncio.create_task(outer())
        await task
        self.assertFalse(task.cancelled())
        self.assertTrue(task.done())

    async def test_nested_timeouts_concurrent(self):
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.002):
                with self.assertRaises(TimeoutError):
                    async with asyncio.timeout(0.1):
                        # Pretend we crunch some numbers.
                        time.sleep(0.01)
                        await asyncio.sleep(1)

    async def test_nested_timeouts_loop_busy(self):
        # After the inner timeout is an expensive operation which should
        # be stopped by the outer timeout.
        loop = asyncio.get_running_loop()
        # Disable a message about long running task
        loop.slow_callback_duration = 10
        t0 = loop.time()
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.1):  # (1)
                with self.assertRaises(TimeoutError):
                    async with asyncio.timeout(0.01):  # (2)
                        # Pretend the loop is busy for a while.
                        time.sleep(0.1)
                        await asyncio.sleep(1)
                # TimeoutError was cought by (2)
                await asyncio.sleep(10) # This sleep should be interrupted by (1)
        t1 = loop.time()
        self.assertTrue(t0 <= t1 <= t0 + 1)

    async def test_reschedule(self):
        loop = asyncio.get_running_loop()
        fut = loop.create_future()
        deadline1 = loop.time() + 10
        deadline2 = deadline1 + 20

        async def f():
            async with asyncio.timeout_at(deadline1) as cm:
                fut.set_result(cm)
                await asyncio.sleep(50)

        task = asyncio.create_task(f())
        cm = await fut

        self.assertEqual(cm.when(), deadline1)
        cm.reschedule(deadline2)
        self.assertEqual(cm.when(), deadline2)
        cm.reschedule(None)
        self.assertIsNone(cm.when())

        task.cancel()

        with self.assertRaises(asyncio.CancelledError):
            await task
        self.assertFalse(cm.expired())

    async def test_repr_active(self):
        async with asyncio.timeout(10) as cm:
            self.assertRegex(repr(cm), r"<Timeout \[active\] when=\d+\.\d*>")

    async def test_repr_expired(self):
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.01) as cm:
                await asyncio.sleep(10)
        self.assertEqual(repr(cm), "<Timeout [expired]>")

    async def test_repr_finished(self):
        async with asyncio.timeout(10) as cm:
            await asyncio.sleep(0)

        self.assertEqual(repr(cm), "<Timeout [finished]>")

    async def test_repr_disabled(self):
        async with asyncio.timeout(None) as cm:
            self.assertEqual(repr(cm), r"<Timeout [active] when=None>")

    async def test_nested_timeout_in_finally(self):
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.01):
                try:
                    await asyncio.sleep(1)
                finally:
                    with self.assertRaises(TimeoutError):
                        async with asyncio.timeout(0.01):
                            await asyncio.sleep(10)

    async def test_timeout_after_cancellation(self):
        try:
            asyncio.current_task().cancel()
            await asyncio.sleep(1)  # work which will be cancelled
        except asyncio.CancelledError:
            pass
        finally:
            with self.assertRaises(TimeoutError):
                async with asyncio.timeout(0.0):
                    await asyncio.sleep(1)  # some cleanup

    async def test_cancel_in_timeout_after_cancellation(self):
        try:
            asyncio.current_task().cancel()
            await asyncio.sleep(1)  # work which will be cancelled
        except asyncio.CancelledError:
            pass
        finally:
            with self.assertRaises(asyncio.CancelledError):
                async with asyncio.timeout(1.0):
                    asyncio.current_task().cancel()
                    await asyncio.sleep(2)  # some cleanup

    async def test_timeout_exception_cause (self):
        with self.assertRaises(asyncio.TimeoutError) as exc:
            async with asyncio.timeout(0):
                await asyncio.sleep(1)
        cause = exc.exception.__cause__
        assert isinstance(cause, asyncio.CancelledError)


if __name__ == '__main__':
    unittest.main()
