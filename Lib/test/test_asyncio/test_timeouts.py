"""Tests for asyncio/timeouts.py"""

import unittest
import time

import asyncio

from test.test_asyncio.utils import await_without_task


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
        async with asyncio.timeout(10) as cm:
            await asyncio.sleep(0.01)
        t1 = loop.time()

        self.assertFalse(cm.expired())
        self.assertGreater(cm.when(), t1)

    async def test_timeout_disabled(self):
        async with asyncio.timeout(None) as cm:
            await asyncio.sleep(0.01)

        self.assertFalse(cm.expired())
        self.assertIsNone(cm.when())

    async def test_timeout_at_disabled(self):
        async with asyncio.timeout_at(None) as cm:
            await asyncio.sleep(0.01)

        self.assertFalse(cm.expired())
        self.assertIsNone(cm.when())

    async def test_timeout_zero(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0) as cm:
                await asyncio.sleep(10)
        t1 = loop.time()
        self.assertTrue(cm.expired())
        self.assertTrue(t0 <= cm.when() <= t1)

    async def test_timeout_zero_sleep_zero(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0) as cm:
                await asyncio.sleep(0)
        t1 = loop.time()
        self.assertTrue(cm.expired())
        self.assertTrue(t0 <= cm.when() <= t1)

    async def test_timeout_in_the_past_sleep_zero(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(-11) as cm:
                await asyncio.sleep(0)
        t1 = loop.time()
        self.assertTrue(cm.expired())
        self.assertTrue(t0 >= cm.when() <= t1)

    async def test_foreign_exception_passed(self):
        with self.assertRaises(KeyError):
            async with asyncio.timeout(0.01) as cm:
                raise KeyError
        self.assertFalse(cm.expired())

    async def test_timeout_exception_context(self):
        with self.assertRaises(TimeoutError) as cm:
            async with asyncio.timeout(0.01):
                try:
                    1/0
                finally:
                    await asyncio.sleep(1)
        e = cm.exception
        # Expect TimeoutError caused by CancelledError raised during handling
        # of ZeroDivisionError.
        e2 = e.__cause__
        self.assertIsInstance(e2, asyncio.CancelledError)
        self.assertIs(e.__context__, e2)
        self.assertIsNone(e2.__cause__)
        self.assertIsInstance(e2.__context__, ZeroDivisionError)

    async def test_foreign_exception_on_timeout(self):
        async def crash():
            try:
                await asyncio.sleep(1)
            finally:
                1/0
        with self.assertRaises(ZeroDivisionError) as cm:
            async with asyncio.timeout(0.01):
                await crash()
        e = cm.exception
        # Expect ZeroDivisionError raised during handling of TimeoutError
        # caused by CancelledError.
        self.assertIsNone(e.__cause__)
        e2 = e.__context__
        self.assertIsInstance(e2, TimeoutError)
        e3 = e2.__cause__
        self.assertIsInstance(e3, asyncio.CancelledError)
        self.assertIs(e2.__context__, e3)

    async def test_foreign_exception_on_timeout_2(self):
        with self.assertRaises(ZeroDivisionError) as cm:
            async with asyncio.timeout(0.01):
                try:
                    try:
                        raise ValueError
                    finally:
                        await asyncio.sleep(1)
                finally:
                    try:
                        raise KeyError
                    finally:
                        1/0
        e = cm.exception
        # Expect ZeroDivisionError raised during handling of KeyError
        # raised during handling of TimeoutError caused by CancelledError.
        self.assertIsNone(e.__cause__)
        e2 = e.__context__
        self.assertIsInstance(e2, KeyError)
        self.assertIsNone(e2.__cause__)
        e3 = e2.__context__
        self.assertIsInstance(e3, TimeoutError)
        e4 = e3.__cause__
        self.assertIsInstance(e4, asyncio.CancelledError)
        self.assertIsNone(e4.__cause__)
        self.assertIsInstance(e4.__context__, ValueError)
        self.assertIs(e3.__context__, e4)

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
        with self.assertRaises(TimeoutError) as cm1:
            async with asyncio.timeout(0.01):
                try:
                    await asyncio.sleep(1)
                finally:
                    with self.assertRaises(TimeoutError) as cm2:
                        async with asyncio.timeout(0.01):
                            await asyncio.sleep(10)
        e1 = cm1.exception
        # Expect TimeoutError caused by CancelledError.
        e12 = e1.__cause__
        self.assertIsInstance(e12, asyncio.CancelledError)
        self.assertIsNone(e12.__cause__)
        self.assertIsNone(e12.__context__)
        self.assertIs(e1.__context__, e12)
        e2 = cm2.exception
        # Expect TimeoutError caused by CancelledError raised during
        # handling of other CancelledError (which is the same as in
        # the above chain).
        e22 = e2.__cause__
        self.assertIsInstance(e22, asyncio.CancelledError)
        self.assertIsNone(e22.__cause__)
        self.assertIs(e22.__context__, e12)
        self.assertIs(e2.__context__, e22)

    async def test_timeout_after_cancellation(self):
        try:
            asyncio.current_task().cancel()
            await asyncio.sleep(1)  # work which will be cancelled
        except asyncio.CancelledError:
            pass
        finally:
            with self.assertRaises(TimeoutError) as cm:
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

    async def test_timeout_already_entered(self):
        async with asyncio.timeout(0.01) as cm:
            with self.assertRaisesRegex(RuntimeError, "has already been entered"):
                async with cm:
                    pass

    async def test_timeout_double_enter(self):
        async with asyncio.timeout(0.01) as cm:
            pass
        with self.assertRaisesRegex(RuntimeError, "has already been entered"):
            async with cm:
                pass

    async def test_timeout_finished(self):
        async with asyncio.timeout(0.01) as cm:
            pass
        with self.assertRaisesRegex(RuntimeError, "finished"):
            cm.reschedule(0.02)

    async def test_timeout_expired(self):
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.01) as cm:
                await asyncio.sleep(1)
        with self.assertRaisesRegex(RuntimeError, "expired"):
            cm.reschedule(0.02)

    async def test_timeout_expiring(self):
        async with asyncio.timeout(0.01) as cm:
            with self.assertRaises(asyncio.CancelledError):
                await asyncio.sleep(1)
            with self.assertRaisesRegex(RuntimeError, "expiring"):
                cm.reschedule(0.02)

    async def test_timeout_not_entered(self):
        cm = asyncio.timeout(0.01)
        with self.assertRaisesRegex(RuntimeError, "has not been entered"):
            cm.reschedule(0.02)

    async def test_timeout_without_task(self):
        cm = asyncio.timeout(0.01)
        with self.assertRaisesRegex(RuntimeError, "task"):
            await await_without_task(cm.__aenter__())
        with self.assertRaisesRegex(RuntimeError, "has not been entered"):
            cm.reschedule(0.02)

    async def test_timeout_taskgroup(self):
        async def task():
            try:
                await asyncio.sleep(2)  # Will be interrupted after 0.01 second
            finally:
                1/0  # Crash in cleanup

        with self.assertRaises(ExceptionGroup) as cm:
            async with asyncio.timeout(0.01):
                async with asyncio.TaskGroup() as tg:
                    tg.create_task(task())
                    try:
                        raise ValueError
                    finally:
                        await asyncio.sleep(1)
        eg = cm.exception
        # Expect ExceptionGroup raised during handling of TimeoutError caused
        # by CancelledError raised during handling of ValueError.
        self.assertIsNone(eg.__cause__)
        e_1 = eg.__context__
        self.assertIsInstance(e_1, TimeoutError)
        e_2 = e_1.__cause__
        self.assertIsInstance(e_2, asyncio.CancelledError)
        self.assertIsNone(e_2.__cause__)
        self.assertIsInstance(e_2.__context__, ValueError)
        self.assertIs(e_1.__context__, e_2)

        self.assertEqual(len(eg.exceptions), 1, eg)
        e1 = eg.exceptions[0]
        # Expect ZeroDivisionError raised during handling of TimeoutError
        # caused by CancelledError (it is a different CancelledError).
        self.assertIsInstance(e1, ZeroDivisionError)
        self.assertIsNone(e1.__cause__)
        e2 = e1.__context__
        self.assertIsInstance(e2, TimeoutError)
        e3 = e2.__cause__
        self.assertIsInstance(e3, asyncio.CancelledError)
        self.assertIsNone(e3.__context__)
        self.assertIsNone(e3.__cause__)
        self.assertIs(e2.__context__, e3)


if __name__ == '__main__':
    unittest.main()
