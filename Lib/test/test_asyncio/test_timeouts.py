"""Tests for asyncio/timeouts.py"""

import unittest

import asyncio
from asyncio import tasks


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class BaseTimeoutTests:
    Task = None

    def new_task(self, loop, coro, name='TestTask'):
        return self.__class__.Task(coro, loop=loop, name=name)

    async def test_cancel_scope_basic(self):
        with asyncio.cancel_after(0.1) as scope:
            await asyncio.sleep(10)
        self.assertTrue(scope.cancelling())
        self.assertTrue(scope.cancelled())

    async def test_cancel_scope_at_basic(self):
        loop = asyncio.get_running_loop()

        with asyncio.cancel_at(loop.time() + 0.1) as scope:
            await asyncio.sleep(10)
        self.assertTrue(scope.cancelling())
        self.assertTrue(scope.cancelled())

    async def test_timeout_basic(self):
        with self.assertRaises(TimeoutError):
            with asyncio.timeout(0.1) as scope:
                await asyncio.sleep(10)
        self.assertTrue(scope.cancelling())
        self.assertTrue(scope.cancelled())

    async def test_timeout_at_basic(self):
        loop = asyncio.get_running_loop()

        with self.assertRaises(TimeoutError):
            with asyncio.timeout_at(loop.time() + 0.1) as scope:
                await asyncio.sleep(10)
        self.assertTrue(scope.cancelling())
        self.assertTrue(scope.cancelled())


@unittest.skipUnless(hasattr(tasks, '_CTask'),
                     'requires the C _asyncio module')
class Timeout_CTask_Tests(BaseTimeoutTests, unittest.IsolatedAsyncioTestCase):
    Task = getattr(tasks, '_CTask', None)


class Timeout_PyTask_Tests(BaseTimeoutTests, unittest.IsolatedAsyncioTestCase):
    Task = tasks._PyTask


if __name__ == '__main__':
    unittest.main()
