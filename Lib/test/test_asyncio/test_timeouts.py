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

    async def test_timeout_basic(self):
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.1) as cm:
                await asyncio.sleep(10)
        self.assertTrue(cm.expired())

    async def test_timeout_at_basic(self):
        loop = asyncio.get_running_loop()

        with self.assertRaises(TimeoutError):
            async with asyncio.timeout_at(loop.time() + 0.1) as cm:
                await asyncio.sleep(10)
        self.assertTrue(cm.expired())

    async def test_nested_timeouts(self):
        cancel = False
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.1) as cm1:
                try:
                    async with asyncio.timeout(0.1) as cm2:
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


@unittest.skipUnless(hasattr(tasks, '_CTask'),
                     'requires the C _asyncio module')
class Timeout_CTask_Tests(BaseTimeoutTests, unittest.IsolatedAsyncioTestCase):
    Task = getattr(tasks, '_CTask', None)


class Timeout_PyTask_Tests(BaseTimeoutTests, unittest.IsolatedAsyncioTestCase):
    Task = tasks._PyTask


if __name__ == '__main__':
    unittest.main()
