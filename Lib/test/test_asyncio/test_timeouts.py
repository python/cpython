"""Tests for asyncio/timeouts.py"""

import unittest
import time

import asyncio
from asyncio import tasks


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class BaseTimeoutTests:
    Task = None

    def new_task(self, loop, coro, name='TestTask'):
        return self.__class__.Task(coro, loop=loop, name=name)

    def _setupAsyncioLoop(self):
        assert self._asyncioTestLoop is None, 'asyncio test loop already initialized'
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        loop.set_debug(True)
        self._asyncioTestLoop = loop
        loop.set_task_factory(self.new_task)
        fut = loop.create_future()
        self._asyncioCallsTask = loop.create_task(self._asyncioLoopRunner(fut))
        loop.run_until_complete(fut)

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
        cancel = False
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.01) as cm1:
                try:
                    async with asyncio.timeout(0.01) as cm2:
                        await asyncio.sleep(10)
                except asyncio.CancelledError:
                    cancel = True
                    raise
                except TimeoutError:
                    self.fail(
                        "The only topmost timed out context manager "
                        "raises TimeoutError"
                    )
        self.assertTrue(cancel)
        self.assertTrue(cm1.expired())
        self.assertTrue(cm2.expired())

    async def test_waiter_cancelled(self):
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.01):
                with self.assertRaises(asyncio.CancelledError):
                    await asyncio.sleep(10)

    async def test_timeout_not_called(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        async with asyncio.timeout(10) as cm:
            await asyncio.sleep(0.01)
        t1 = loop.time()

        self.assertFalse(cm.expired())
        # finised fast. Very busy CI box requires high enough limit,
        # that's why 0.01 cannot be used
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
        # finised fast. Very busy CI box requires high enough limit,
        # that's why 0.01 cannot be used
        self.assertLess(t1-t0, 2)

    async def test_timeout_at_disabled(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        async with asyncio.timeout(None) as cm:
            await asyncio.sleep(0.01)
        t1 = loop.time()

        self.assertFalse(cm.expired())
        self.assertIsNone(cm.when())
        # finised fast. Very busy CI box requires high enough limit,
        # that's why 0.01 cannot be used
        self.assertLess(t1-t0, 2)

    async def test_timeout_zero(self):
        loop = asyncio.get_running_loop()
        t0 = loop.time()
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0) as cm:
                await asyncio.sleep(10)
        t1 = loop.time()
        self.assertTrue(cm.expired())
        # finised fast. Very busy CI box requires high enough limit,
        # that's why 0.01 cannot be used
        self.assertLess(t1-t0, 2)
        self.assertTrue(t0 <= cm.when() <= t1)

    async def test_foreign_exception_passed(self):
        with self.assertRaises(KeyError):
            async with asyncio.timeout(0.01) as cm:
                raise KeyError
        self.assertFalse(cm.expired())

    async def test_foreign_cancel_doesnt_timeout_if_not_expired(self):
        with self.assertRaises(asyncio.CancelledError):
            async with asyncio.timeout(10) as cm:
                raise asyncio.CancelledError
        self.assertFalse(cm.expired())

    async def test_outer_task_is_not_cancelled(self):

        has_timeout = False

        async def outer() -> None:
            nonlocal has_timeout
            try:
                async with asyncio.timeout(0.001):
                    await asyncio.sleep(1)
            except asyncio.TimeoutError:
                has_timeout = True

        task = asyncio.create_task(outer())
        await task
        assert has_timeout
        assert not task.cancelled()
        assert task.done()

    async def test_nested_timeouts_concurrent(self):
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.002):
                try:
                    async with asyncio.timeout(0.003):
                        # Pretend we crunch some numbers.
                        time.sleep(0.005)
                        await asyncio.sleep(1)
                except asyncio.TimeoutError:
                    pass

    async def test_nested_timeouts_loop_busy(self):
        """
        After the inner timeout is an expensive operation which should
        be stopped by the outer timeout.

        Note: this fails for now.
        """
        start = time.perf_counter()
        try:
            async with asyncio.timeout(0.002):
                try:
                    async with asyncio.timeout(0.001):
                        # Pretend the loop is busy for a while.
                        time.sleep(0.010)
                        await asyncio.sleep(0.001)
                except asyncio.TimeoutError:
                    # This sleep should be interrupted.
                    await asyncio.sleep(10)
        except asyncio.TimeoutError:
            pass
        took = time.perf_counter() - start
        self.assertTrue(took <= 1)

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
            self.assertRegex(repr(cm), r"<Timeout \[active\] deadline=\d+\.\d*>")

    async def test_repr_expired(self):
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.01) as cm:
                await asyncio.sleep(10)
        self.assertEqual(repr(cm), "<Timeout [expired]>")


@unittest.skipUnless(hasattr(tasks, '_CTask'),
                     'requires the C _asyncio module')
class Timeout_CTask_Tests(BaseTimeoutTests, unittest.IsolatedAsyncioTestCase):
    Task = getattr(tasks, '_CTask', None)


class Timeout_PyTask_Tests(BaseTimeoutTests, unittest.IsolatedAsyncioTestCase):
    Task = tasks._PyTask


if __name__ == '__main__':
    unittest.main()
