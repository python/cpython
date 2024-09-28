"""Tests for base_events.py"""

import asyncio
import contextvars
import unittest

from unittest import mock
from asyncio import tasks
from test.test_asyncio import utils as test_utils
from test.support.script_helper import assert_python_ok

MOCK_ANY = mock.ANY


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class EagerTaskFactoryLoopTests:

    Task = None

    def run_coro(self, coro):
        """
        Helper method to run the `coro` coroutine in the test event loop.
        It helps with making sure the event loop is running before starting
        to execute `coro`. This is important for testing the eager step
        functionality, since an eager step is taken only if the event loop
        is already running.
        """

        async def coro_runner():
            self.assertTrue(asyncio.get_event_loop().is_running())
            return await coro

        return self.loop.run_until_complete(coro)

    def setUp(self):
        super().setUp()
        self.loop = asyncio.new_event_loop()
        self.eager_task_factory = asyncio.create_eager_task_factory(self.Task)
        self.loop.set_task_factory(self.eager_task_factory)
        self.set_event_loop(self.loop)

    def test_eager_task_factory_set(self):
        self.assertIsNotNone(self.eager_task_factory)
        self.assertIs(self.loop.get_task_factory(), self.eager_task_factory)

        async def noop(): pass

        async def run():
            t = self.loop.create_task(noop())
            self.assertIsInstance(t, self.Task)
            await t

        self.run_coro(run())

    def test_await_future_during_eager_step(self):

        async def set_result(fut, val):
            fut.set_result(val)

        async def run():
            fut = self.loop.create_future()
            t = self.loop.create_task(set_result(fut, 'my message'))
            # assert the eager step completed the task
            self.assertTrue(t.done())
            return await fut

        self.assertEqual(self.run_coro(run()), 'my message')

    def test_eager_completion(self):

        async def coro():
            return 'hello'

        async def run():
            t = self.loop.create_task(coro())
            # assert the eager step completed the task
            self.assertTrue(t.done())
            return await t

        self.assertEqual(self.run_coro(run()), 'hello')

    def test_block_after_eager_step(self):

        async def coro():
            await asyncio.sleep(0.1)
            return 'finished after blocking'

        async def run():
            t = self.loop.create_task(coro())
            self.assertFalse(t.done())
            result = await t
            self.assertTrue(t.done())
            return result

        self.assertEqual(self.run_coro(run()), 'finished after blocking')

    def test_cancellation_after_eager_completion(self):

        async def coro():
            return 'finished without blocking'

        async def run():
            t = self.loop.create_task(coro())
            t.cancel()
            result = await t
            # finished task can't be cancelled
            self.assertFalse(t.cancelled())
            return result

        self.assertEqual(self.run_coro(run()), 'finished without blocking')

    def test_cancellation_after_eager_step_blocks(self):

        async def coro():
            await asyncio.sleep(0.1)
            return 'finished after blocking'

        async def run():
            t = self.loop.create_task(coro())
            t.cancel('cancellation message')
            self.assertGreater(t.cancelling(), 0)
            result = await t

        with self.assertRaises(asyncio.CancelledError) as cm:
            self.run_coro(run())

        self.assertEqual('cancellation message', cm.exception.args[0])

    def test_current_task(self):
        captured_current_task = None

        async def coro():
            nonlocal captured_current_task
            captured_current_task = asyncio.current_task()
            # verify the task before and after blocking is identical
            await asyncio.sleep(0.1)
            self.assertIs(asyncio.current_task(), captured_current_task)

        async def run():
            t = self.loop.create_task(coro())
            self.assertIs(captured_current_task, t)
            await t

        self.run_coro(run())
        captured_current_task = None

    def test_all_tasks_with_eager_completion(self):
        captured_all_tasks = None

        async def coro():
            nonlocal captured_all_tasks
            captured_all_tasks = asyncio.all_tasks()

        async def run():
            t = self.loop.create_task(coro())
            self.assertIn(t, captured_all_tasks)
            self.assertNotIn(t, asyncio.all_tasks())

        self.run_coro(run())

    def test_all_tasks_with_blocking(self):
        captured_eager_all_tasks = None

        async def coro(fut1, fut2):
            nonlocal captured_eager_all_tasks
            captured_eager_all_tasks = asyncio.all_tasks()
            await fut1
            fut2.set_result(None)

        async def run():
            fut1 = self.loop.create_future()
            fut2 = self.loop.create_future()
            t = self.loop.create_task(coro(fut1, fut2))
            self.assertIn(t, captured_eager_all_tasks)
            self.assertIn(t, asyncio.all_tasks())
            fut1.set_result(None)
            await fut2
            self.assertNotIn(t, asyncio.all_tasks())

        self.run_coro(run())

    def test_context_vars(self):
        cv = contextvars.ContextVar('cv', default=0)

        coro_first_step_ran = False
        coro_second_step_ran = False

        async def coro():
            nonlocal coro_first_step_ran
            nonlocal coro_second_step_ran
            self.assertEqual(cv.get(), 1)
            cv.set(2)
            self.assertEqual(cv.get(), 2)
            coro_first_step_ran = True
            await asyncio.sleep(0.1)
            self.assertEqual(cv.get(), 2)
            cv.set(3)
            self.assertEqual(cv.get(), 3)
            coro_second_step_ran = True

        async def run():
            cv.set(1)
            t = self.loop.create_task(coro())
            self.assertTrue(coro_first_step_ran)
            self.assertFalse(coro_second_step_ran)
            self.assertEqual(cv.get(), 1)
            await t
            self.assertTrue(coro_second_step_ran)
            self.assertEqual(cv.get(), 1)

        self.run_coro(run())


class PyEagerTaskFactoryLoopTests(EagerTaskFactoryLoopTests, test_utils.TestCase):
    Task = tasks._PyTask


@unittest.skipUnless(hasattr(tasks, '_CTask'),
                     'requires the C _asyncio module')
class CEagerTaskFactoryLoopTests(EagerTaskFactoryLoopTests, test_utils.TestCase):
    Task = getattr(tasks, '_CTask', None)

    def test_issue105987(self):
        code = """if 1:
        from _asyncio import _swap_current_task

        class DummyTask:
            pass

        class DummyLoop:
            pass

        l = DummyLoop()
        _swap_current_task(l, DummyTask())
        t = _swap_current_task(l, None)
        """

        _, out, err = assert_python_ok("-c", code)
        self.assertFalse(err)

    def test_issue122332(self):
       async def coro():
           pass

       async def run():
           task = self.loop.create_task(coro())
           await task
           self.assertIsNone(task.get_coro())

       self.run_coro(run())


class AsyncTaskCounter:
    def __init__(self, loop, *, task_class, eager):
        self.suspense_count = 0
        self.task_count = 0

        def CountingTask(*args, eager_start=False, **kwargs):
            if not eager_start:
                self.task_count += 1
            kwargs["eager_start"] = eager_start
            return task_class(*args, **kwargs)

        if eager:
            factory = asyncio.create_eager_task_factory(CountingTask)
        else:
            def factory(loop, coro, **kwargs):
                return CountingTask(coro, loop=loop, **kwargs)
        loop.set_task_factory(factory)

    def get(self):
        return self.task_count


async def awaitable_chain(depth):
    if depth == 0:
        return 0
    return 1 + await awaitable_chain(depth - 1)


async def recursive_taskgroups(width, depth):
    if depth == 0:
        return

    async with asyncio.TaskGroup() as tg:
        futures = [
            tg.create_task(recursive_taskgroups(width, depth - 1))
            for _ in range(width)
        ]


async def recursive_gather(width, depth):
    if depth == 0:
        return

    await asyncio.gather(
        *[recursive_gather(width, depth - 1) for _ in range(width)]
    )


class BaseTaskCountingTests:

    Task = None
    eager = None
    expected_task_count = None

    def setUp(self):
        super().setUp()
        self.loop = asyncio.new_event_loop()
        self.counter = AsyncTaskCounter(self.loop, task_class=self.Task, eager=self.eager)
        self.set_event_loop(self.loop)

    def test_awaitables_chain(self):
        observed_depth = self.loop.run_until_complete(awaitable_chain(100))
        self.assertEqual(observed_depth, 100)
        self.assertEqual(self.counter.get(), 0 if self.eager else 1)

    def test_recursive_taskgroups(self):
        num_tasks = self.loop.run_until_complete(recursive_taskgroups(5, 4))
        self.assertEqual(self.counter.get(), self.expected_task_count)

    def test_recursive_gather(self):
        self.loop.run_until_complete(recursive_gather(5, 4))
        self.assertEqual(self.counter.get(), self.expected_task_count)


class BaseNonEagerTaskFactoryTests(BaseTaskCountingTests):
    eager = False
    expected_task_count = 781  # 1 + 5 + 5^2 + 5^3 + 5^4


class BaseEagerTaskFactoryTests(BaseTaskCountingTests):
    eager = True
    expected_task_count = 0


class NonEagerTests(BaseNonEagerTaskFactoryTests, test_utils.TestCase):
    Task = asyncio.Task


class EagerTests(BaseEagerTaskFactoryTests, test_utils.TestCase):
    Task = asyncio.Task


class NonEagerPyTaskTests(BaseNonEagerTaskFactoryTests, test_utils.TestCase):
    Task = tasks._PyTask


class EagerPyTaskTests(BaseEagerTaskFactoryTests, test_utils.TestCase):
    Task = tasks._PyTask


@unittest.skipUnless(hasattr(tasks, '_CTask'),
                     'requires the C _asyncio module')
class NonEagerCTaskTests(BaseNonEagerTaskFactoryTests, test_utils.TestCase):
    Task = getattr(tasks, '_CTask', None)


@unittest.skipUnless(hasattr(tasks, '_CTask'),
                     'requires the C _asyncio module')
class EagerCTaskTests(BaseEagerTaskFactoryTests, test_utils.TestCase):
    Task = getattr(tasks, '_CTask', None)

if __name__ == '__main__':
    unittest.main()
