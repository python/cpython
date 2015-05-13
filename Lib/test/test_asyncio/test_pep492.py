"""Tests support for new syntax introduced by PEP 492."""

import unittest
from unittest import mock

import asyncio
from asyncio import test_utils


class BaseTest(test_utils.TestCase):

    def setUp(self):
        self.loop = asyncio.BaseEventLoop()
        self.loop._process_events = mock.Mock()
        self.loop._selector = mock.Mock()
        self.loop._selector.select.return_value = ()
        self.set_event_loop(self.loop)


class LockTests(BaseTest):

    def test_context_manager_async_with(self):
        primitives = [
            asyncio.Lock(loop=self.loop),
            asyncio.Condition(loop=self.loop),
            asyncio.Semaphore(loop=self.loop),
            asyncio.BoundedSemaphore(loop=self.loop),
        ]

        async def test(lock):
            await asyncio.sleep(0.01, loop=self.loop)
            self.assertFalse(lock.locked())
            async with lock as _lock:
                self.assertIs(_lock, None)
                self.assertTrue(lock.locked())
                await asyncio.sleep(0.01, loop=self.loop)
                self.assertTrue(lock.locked())
            self.assertFalse(lock.locked())

        for primitive in primitives:
            self.loop.run_until_complete(test(primitive))
            self.assertFalse(primitive.locked())

    def test_context_manager_with_await(self):
        primitives = [
            asyncio.Lock(loop=self.loop),
            asyncio.Condition(loop=self.loop),
            asyncio.Semaphore(loop=self.loop),
            asyncio.BoundedSemaphore(loop=self.loop),
        ]

        async def test(lock):
            await asyncio.sleep(0.01, loop=self.loop)
            self.assertFalse(lock.locked())
            with await lock as _lock:
                self.assertIs(_lock, None)
                self.assertTrue(lock.locked())
                await asyncio.sleep(0.01, loop=self.loop)
                self.assertTrue(lock.locked())
            self.assertFalse(lock.locked())

        for primitive in primitives:
            self.loop.run_until_complete(test(primitive))
            self.assertFalse(primitive.locked())


if __name__ == '__main__':
    unittest.main()
