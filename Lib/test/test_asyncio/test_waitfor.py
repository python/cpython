import asyncio
import unittest
import time
from test import support


def tearDownModule():
    asyncio.events._set_event_loop_policy(None)


# The following value can be used as a very small timeout:
# it passes check "timeout > 0", but has almost
# no effect on the test performance
_EPSILON = 0.0001


class SlowTask:
    """ Task will run for this defined time, ignoring cancel requests """
    TASK_TIMEOUT = 0.2

    def __init__(self):
        self.exited = False

    async def run(self):
        exitat = time.monotonic() + self.TASK_TIMEOUT

        while True:
            tosleep = exitat - time.monotonic()
            if tosleep <= 0:
                break

            try:
                await asyncio.sleep(tosleep)
            except asyncio.CancelledError:
                pass

        self.exited = True


class AsyncioWaitForTest(unittest.IsolatedAsyncioTestCase):

    async def test_asyncio_wait_for_cancelled(self):
        t = SlowTask()

        waitfortask = asyncio.create_task(
            asyncio.wait_for(t.run(), t.TASK_TIMEOUT * 2))
        await asyncio.sleep(0)
        waitfortask.cancel()
        await asyncio.wait({waitfortask})

        self.assertTrue(t.exited)

    async def test_asyncio_wait_for_timeout(self):
        t = SlowTask()

        try:
            await asyncio.wait_for(t.run(), t.TASK_TIMEOUT / 2)
        except asyncio.TimeoutError:
            pass

        self.assertTrue(t.exited)

    async def test_wait_for_timeout_less_then_0_or_0_future_done(self):
        loop = asyncio.get_running_loop()

        fut = loop.create_future()
        fut.set_result('done')

        ret = await asyncio.wait_for(fut, 0)

        self.assertEqual(ret, 'done')
        self.assertTrue(fut.done())

    async def test_wait_for_timeout_less_then_0_or_0_coroutine_do_not_started(self):
        foo_started = False

        async def foo():
            nonlocal foo_started
            foo_started = True

        with self.assertRaises(asyncio.TimeoutError):
            await asyncio.wait_for(foo(), 0)

        self.assertEqual(foo_started, False)

    async def test_wait_for_timeout_less_then_0_or_0(self):
        loop = asyncio.get_running_loop()

        for timeout in [0, -1]:
            with self.subTest(timeout=timeout):
                foo_running = None
                started = loop.create_future()

                async def foo():
                    nonlocal foo_running
                    foo_running = True
                    started.set_result(None)
                    try:
                        await asyncio.sleep(10)
                    finally:
                        foo_running = False
                    return 'done'

                fut = asyncio.create_task(foo())
                await started

                with self.assertRaises(asyncio.TimeoutError):
                    await asyncio.wait_for(fut, timeout)

                self.assertTrue(fut.done())
                # it should have been cancelled due to the timeout
                self.assertTrue(fut.cancelled())
                self.assertEqual(foo_running, False)

    async def test_wait_for(self):
        foo_running = None

        async def foo():
            nonlocal foo_running
            foo_running = True
            try:
                await asyncio.sleep(support.LONG_TIMEOUT)
            finally:
                foo_running = False
            return 'done'

        fut = asyncio.create_task(foo())

        with self.assertRaises(asyncio.TimeoutError):
            await asyncio.wait_for(fut, 0.1)
        self.assertTrue(fut.done())
        # it should have been cancelled due to the timeout
        self.assertTrue(fut.cancelled())
        self.assertEqual(foo_running, False)

    async def test_wait_for_blocking(self):
        async def coro():
            return 'done'

        res = await asyncio.wait_for(coro(), timeout=None)
        self.assertEqual(res, 'done')

    async def test_wait_for_race_condition(self):
        loop = asyncio.get_running_loop()

        fut = loop.create_future()
        task = asyncio.wait_for(fut, timeout=0.2)
        loop.call_soon(fut.set_result, "ok")
        res = await task
        self.assertEqual(res, "ok")

    async def test_wait_for_cancellation_race_condition(self):
        async def inner():
            with self.assertRaises(asyncio.CancelledError):
                await asyncio.sleep(1)
            return 1

        result = await asyncio.wait_for(inner(), timeout=.01)
        self.assertEqual(result, 1)

    async def test_wait_for_waits_for_task_cancellation(self):
        task_done = False

        async def inner():
            nonlocal task_done
            try:
                await asyncio.sleep(10)
            except asyncio.CancelledError:
                await asyncio.sleep(_EPSILON)
                raise
            finally:
                task_done = True

        inner_task = asyncio.create_task(inner())

        with self.assertRaises(asyncio.TimeoutError) as cm:
            await asyncio.wait_for(inner_task, timeout=_EPSILON)

        self.assertTrue(task_done)
        chained = cm.exception.__context__
        self.assertEqual(type(chained), asyncio.CancelledError)

    async def test_wait_for_waits_for_task_cancellation_w_timeout_0(self):
        task_done = False

        async def foo():
            async def inner():
                nonlocal task_done
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    await asyncio.sleep(_EPSILON)
                    raise
                finally:
                    task_done = True

            inner_task = asyncio.create_task(inner())
            await asyncio.sleep(_EPSILON)
            await asyncio.wait_for(inner_task, timeout=0)

        with self.assertRaises(asyncio.TimeoutError) as cm:
            await foo()

        self.assertTrue(task_done)
        chained = cm.exception.__context__
        self.assertEqual(type(chained), asyncio.CancelledError)

    async def test_wait_for_reraises_exception_during_cancellation(self):
        class FooException(Exception):
            pass

        async def foo():
            async def inner():
                try:
                    await asyncio.sleep(0.2)
                finally:
                    raise FooException

            inner_task = asyncio.create_task(inner())

            await asyncio.wait_for(inner_task, timeout=_EPSILON)

        with self.assertRaises(FooException):
            await foo()

    async def _test_cancel_wait_for(self, timeout):
        loop = asyncio.get_running_loop()

        async def blocking_coroutine():
            fut = loop.create_future()
            # Block: fut result is never set
            await fut

        task = asyncio.create_task(blocking_coroutine())

        wait = asyncio.create_task(asyncio.wait_for(task, timeout))
        loop.call_soon(wait.cancel)

        with self.assertRaises(asyncio.CancelledError):
            await wait

        # Python issue #23219: cancelling the wait must also cancel the task
        self.assertTrue(task.cancelled())

    async def test_cancel_blocking_wait_for(self):
        await self._test_cancel_wait_for(None)

    async def test_cancel_wait_for(self):
        await self._test_cancel_wait_for(60.0)

    async def test_wait_for_cancel_suppressed(self):
        # GH-86296: Suppressing CancelledError is discouraged
        # but if a task suppresses CancelledError and returns a value,
        # `wait_for` should return the value instead of raising CancelledError.
        # This is the same behavior as `asyncio.timeout`.

        async def return_42():
            try:
                await asyncio.sleep(10)
            except asyncio.CancelledError:
                return 42

        res = await asyncio.wait_for(return_42(), timeout=0.1)
        self.assertEqual(res, 42)


    async def test_wait_for_issue86296(self):
        # GH-86296: The task should get cancelled and not run to completion.
        # inner completes in one cycle of the event loop so it
        # completes before the task is cancelled.

        async def inner():
            return 'done'

        inner_task = asyncio.create_task(inner())
        reached_end = False

        async def wait_for_coro():
            await asyncio.wait_for(inner_task, timeout=100)
            await asyncio.sleep(1)
            nonlocal reached_end
            reached_end = True

        task = asyncio.create_task(wait_for_coro())
        self.assertFalse(task.done())
        # Run the task
        await asyncio.sleep(0)
        task.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await task
        self.assertTrue(inner_task.done())
        self.assertEqual(await inner_task, 'done')
        self.assertFalse(reached_end)


class WaitForShieldTests(unittest.IsolatedAsyncioTestCase):

    async def test_zero_timeout(self):
        # `asyncio.shield` creates a new task which wraps the passed in
        # awaitable and shields it from cancellation so with timeout=0
        # the task returned by `asyncio.shield` aka shielded_task gets
        # cancelled immediately and the task wrapped by it is scheduled
        # to run.

        async def coro():
            await asyncio.sleep(0.01)
            return 'done'

        task = asyncio.create_task(coro())
        with self.assertRaises(asyncio.TimeoutError):
            shielded_task = asyncio.shield(task)
            await asyncio.wait_for(shielded_task, timeout=0)

        # Task is running in background
        self.assertFalse(task.done())
        self.assertFalse(task.cancelled())
        self.assertTrue(shielded_task.cancelled())

        # Wait for the task to complete
        await asyncio.sleep(0.1)
        self.assertTrue(task.done())


    async def test_none_timeout(self):
        # With timeout=None the timeout is disabled so it
        # runs till completion.
        async def coro():
            await asyncio.sleep(0.1)
            return 'done'

        task = asyncio.create_task(coro())
        await asyncio.wait_for(asyncio.shield(task), timeout=None)

        self.assertTrue(task.done())
        self.assertEqual(await task, "done")

    async def test_shielded_timeout(self):
        # shield prevents the task from being cancelled.
        async def coro():
            await asyncio.sleep(0.1)
            return 'done'

        task = asyncio.create_task(coro())
        with self.assertRaises(asyncio.TimeoutError):
            await asyncio.wait_for(asyncio.shield(task), timeout=0.01)

        self.assertFalse(task.done())
        self.assertFalse(task.cancelled())
        self.assertEqual(await task, "done")


if __name__ == '__main__':
    unittest.main()
