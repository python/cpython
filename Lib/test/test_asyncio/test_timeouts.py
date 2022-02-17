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


@unittest.skipUnless(hasattr(tasks, '_CTask'),
                     'requires the C _asyncio module')
class Timeout_CTask_Tests(BaseTimeoutTests, unittest.IsolatedAsyncioTestCase):
    Task = getattr(tasks, '_CTask', None)


class Timeout_PyTask_Tests(BaseTimeoutTests, unittest.IsolatedAsyncioTestCase):
    Task = tasks._PyTask


if __name__ == '__main__':
    unittest.main()
