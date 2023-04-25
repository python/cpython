"""Tests for base_events.py"""

import gc
import time
import unittest
from types import GenericAlias
from unittest import mock

import asyncio
from asyncio import base_events
from asyncio import tasks
from test.test_asyncio import utils as test_utils
from test.test_asyncio.test_tasks import get_innermost_context
from test import support

MOCK_ANY = mock.ANY


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class EagerTaskFactoryLoopTests(test_utils.TestCase):

    def new_task(self, loop, coro, name='TestTask', context=None):
        return tasks.Task(coro, loop=loop, name=name, context=context)

    def new_future(self, loop):
        return asyncio.Future(loop=loop)

    def setUp(self):
        super().setUp()
        self.loop = asyncio.new_event_loop()
        self.loop.set_task_factory(asyncio.eager_task_factory)
        self.set_event_loop(self.loop)

    def test_eager_task_factory_set(self):
        self.assertIs(self.loop.get_task_factory(), asyncio.eager_task_factory)

    def test_await_future_during_eager_step(self):

        async def set_result(fut, val):
            fut.set_result(val)

        async def run():
            fut = self.loop.create_future()
            t = self.loop.create_task(set_result(fut, 'my message'))
            # assert the eager step completed the task
            self.assertTrue(t.done())
            return await fut

        result = self.loop.run_until_complete(run())
        self.assertEqual(result,'my message')

    def test_eager_completion(self):

        async def coro():
            return 'hello'

        async def run():
            await asyncio.sleep(0.1)
            t = self.loop.create_task(coro())
            # assert the eager step completed the task
            self.assertTrue(t.done())
            return await t

        result = self.loop.run_until_complete(run())
        self.assertEqual(result, 'hello')

    def test_block_after_eager_step(self):

        async def coro():
            await asyncio.sleep(0.1)
            return 'finished after blocking'

        async def run():
            await asyncio.sleep(0.1)
            t = self.loop.create_task(coro())
            self.assertFalse(t.done())
            result = await t
            self.assertTrue(t.done())
            return result

        result = self.loop.run_until_complete(run())
        self.assertEqual(result, 'finished after blocking')

    def test_cancellation_after_eager_completion(self):

        async def coro():
            return 'finished without blocking'

        async def run():
            await asyncio.sleep(0.1)
            t = self.loop.create_task(coro())
            t.cancel()
            result = await t
            # finished task can't be cancelled
            self.assertFalse(t.cancelled())
            return result

        result = self.loop.run_until_complete(run())
        self.assertEqual(result, 'finished without blocking')

    def test_cancellation_after_eager_step_blocks(self):

        async def coro():
            await asyncio.sleep(0.1)
            return 'finished after blocking'

        async def run():
            await asyncio.sleep(0.1)
            t = self.loop.create_task(coro())
            t.cancel('cancellation message')
            self.assertGreater(t.cancelling(), 0)
            try:
                result = await t
            except asyncio.CancelledError:
                # finished task can't be cancelled
                self.assertTrue(t.cancelled())
                raise

        with self.assertRaises(asyncio.CancelledError) as cm:
            self.loop.run_until_complete(run())

        self.assertEqual('cancellation message', cm.exception.args[0])


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
