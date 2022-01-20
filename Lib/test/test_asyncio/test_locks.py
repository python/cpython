"""Tests for lock.py"""

import unittest
from unittest import mock
import re

import asyncio

STR_RGX_REPR = (
    r'^<(?P<class>.*?) object at (?P<address>.*?)'
    r'\[(?P<extras>'
    r'(set|unset|locked|unlocked)(, value:\d)?(, waiters:\d+)?'
    r')\]>\Z'
)
RGX_REPR = re.compile(STR_RGX_REPR)


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class LockTests(unittest.IsolatedAsyncioTestCase):

    async def test_repr(self):
        lock = asyncio.Lock()
        self.assertTrue(repr(lock).endswith('[unlocked]>'))
        self.assertTrue(RGX_REPR.match(repr(lock)))

        await lock.acquire()
        self.assertTrue(repr(lock).endswith('[locked]>'))
        self.assertTrue(RGX_REPR.match(repr(lock)))

    async def test_lock(self):
        lock = asyncio.Lock()

        with self.assertRaisesRegex(
            TypeError,
            "object Lock can't be used in 'await' expression"
        ):
            await lock

        self.assertFalse(lock.locked())

    async def test_lock_doesnt_accept_loop_parameter(self):
        primitives_cls = [
            asyncio.Lock,
            asyncio.Condition,
            asyncio.Event,
            asyncio.Semaphore,
            asyncio.BoundedSemaphore,
        ]

        loop = asyncio.get_running_loop()

        for cls in primitives_cls:
            with self.assertRaisesRegex(
                TypeError,
                rf'As of 3.10, the \*loop\* parameter was removed from '
                rf'{cls.__name__}\(\) since it is no longer necessary'
            ):
                cls(loop=loop)

    async def test_lock_by_with_statement(self):
        primitives = [
            asyncio.Lock(),
            asyncio.Condition(),
            asyncio.Semaphore(),
            asyncio.BoundedSemaphore(),
        ]

        for lock in primitives:
            await asyncio.sleep(0.01)
            self.assertFalse(lock.locked())
            with self.assertRaisesRegex(
                TypeError,
                r"object \w+ can't be used in 'await' expression"
            ):
                with await lock:
                    pass
            self.assertFalse(lock.locked())

    async def test_acquire(self):
        lock = asyncio.Lock()
        result = []

        self.assertTrue(await lock.acquire())

        async def c1(result):
            if await lock.acquire():
                result.append(1)
            return True

        async def c2(result):
            if await lock.acquire():
                result.append(2)
            return True

        async def c3(result):
            if await lock.acquire():
                result.append(3)
            return True

        t1 = asyncio.create_task(c1(result))
        t2 = asyncio.create_task(c2(result))

        await asyncio.sleep(0)
        self.assertEqual([], result)

        lock.release()
        await asyncio.sleep(0)
        self.assertEqual([1], result)

        await asyncio.sleep(0)
        self.assertEqual([1], result)

        t3 = asyncio.create_task(c3(result))

        lock.release()
        await asyncio.sleep(0)
        self.assertEqual([1, 2], result)

        lock.release()
        await asyncio.sleep(0)
        self.assertEqual([1, 2, 3], result)

        self.assertTrue(t1.done())
        self.assertTrue(t1.result())
        self.assertTrue(t2.done())
        self.assertTrue(t2.result())
        self.assertTrue(t3.done())
        self.assertTrue(t3.result())

    async def test_acquire_cancel(self):
        lock = asyncio.Lock()
        self.assertTrue(await lock.acquire())

        task = asyncio.create_task(lock.acquire())
        asyncio.get_running_loop().call_soon(task.cancel)
        with self.assertRaises(asyncio.CancelledError):
            await task
        self.assertFalse(lock._waiters)

    async def test_cancel_race(self):
        # Several tasks:
        # - A acquires the lock
        # - B is blocked in acquire()
        # - C is blocked in acquire()
        #
        # Now, concurrently:
        # - B is cancelled
        # - A releases the lock
        #
        # If B's waiter is marked cancelled but not yet removed from
        # _waiters, A's release() call will crash when trying to set
        # B's waiter; instead, it should move on to C's waiter.

        # Setup: A has the lock, b and c are waiting.
        lock = asyncio.Lock()

        async def lockit(name, blocker):
            await lock.acquire()
            try:
                if blocker is not None:
                    await blocker
            finally:
                lock.release()

        fa = asyncio.get_running_loop().create_future()
        ta = asyncio.create_task(lockit('A', fa))
        await asyncio.sleep(0)
        self.assertTrue(lock.locked())
        tb = asyncio.create_task(lockit('B', None))
        await asyncio.sleep(0)
        self.assertEqual(len(lock._waiters), 1)
        tc = asyncio.create_task(lockit('C', None))
        await asyncio.sleep(0)
        self.assertEqual(len(lock._waiters), 2)

        # Create the race and check.
        # Without the fix this failed at the last assert.
        fa.set_result(None)
        tb.cancel()
        self.assertTrue(lock._waiters[0].cancelled())
        await asyncio.sleep(0)
        self.assertFalse(lock.locked())
        self.assertTrue(ta.done())
        self.assertTrue(tb.cancelled())
        await tc

    async def test_cancel_release_race(self):
        # Issue 32734
        # Acquire 4 locks, cancel second, release first
        # and 2 locks are taken at once.
        loop = asyncio.get_running_loop()
        lock = asyncio.Lock()
        lock_count = 0
        call_count = 0

        async def lockit():
            nonlocal lock_count
            nonlocal call_count
            call_count += 1
            await lock.acquire()
            lock_count += 1

        def trigger():
            t1.cancel()
            lock.release()

        await lock.acquire()

        t1 = asyncio.create_task(lockit())
        t2 = asyncio.create_task(lockit())
        t3 = asyncio.create_task(lockit())

        # Start scheduled tasks
        await asyncio.sleep(0)

        loop.call_soon(trigger)
        with self.assertRaises(asyncio.CancelledError):
            # Wait for cancellation
            await t1

        # Make sure only one lock was taken
        self.assertEqual(lock_count, 1)
        # While 3 calls were made to lockit()
        self.assertEqual(call_count, 3)
        self.assertTrue(t1.cancelled() and t2.done())

        # Cleanup the task that is stuck on acquire.
        t3.cancel()
        await asyncio.sleep(0)
        self.assertTrue(t3.cancelled())

    async def test_finished_waiter_cancelled(self):
        lock = asyncio.Lock()

        await lock.acquire()
        self.assertTrue(lock.locked())

        tb = asyncio.create_task(lock.acquire())
        await asyncio.sleep(0)
        self.assertEqual(len(lock._waiters), 1)

        # Create a second waiter, wake up the first, and cancel it.
        # Without the fix, the second was not woken up.
        tc = asyncio.create_task(lock.acquire())
        tb.cancel()
        lock.release()
        await asyncio.sleep(0)

        self.assertTrue(lock.locked())
        self.assertTrue(tb.cancelled())

        # Cleanup
        await tc

    async def test_release_not_acquired(self):
        lock = asyncio.Lock()

        self.assertRaises(RuntimeError, lock.release)

    async def test_release_no_waiters(self):
        lock = asyncio.Lock()
        await lock.acquire()
        self.assertTrue(lock.locked())

        lock.release()
        self.assertFalse(lock.locked())

    async def test_context_manager(self):
        lock = asyncio.Lock()
        self.assertFalse(lock.locked())

        async with lock:
            self.assertTrue(lock.locked())

        self.assertFalse(lock.locked())


class EventTests(unittest.IsolatedAsyncioTestCase):

    def test_repr(self):
        ev = asyncio.Event()
        self.assertTrue(repr(ev).endswith('[unset]>'))
        match = RGX_REPR.match(repr(ev))
        self.assertEqual(match.group('extras'), 'unset')

        ev.set()
        self.assertTrue(repr(ev).endswith('[set]>'))
        self.assertTrue(RGX_REPR.match(repr(ev)))

        ev._waiters.append(mock.Mock())
        self.assertTrue('waiters:1' in repr(ev))
        self.assertTrue(RGX_REPR.match(repr(ev)))

    async def test_wait(self):
        ev = asyncio.Event()
        self.assertFalse(ev.is_set())

        result = []

        async def c1(result):
            if await ev.wait():
                result.append(1)

        async def c2(result):
            if await ev.wait():
                result.append(2)

        async def c3(result):
            if await ev.wait():
                result.append(3)

        t1 = asyncio.create_task(c1(result))
        t2 = asyncio.create_task(c2(result))

        await asyncio.sleep(0)
        self.assertEqual([], result)

        t3 = asyncio.create_task(c3(result))

        ev.set()
        await asyncio.sleep(0)
        self.assertEqual([3, 1, 2], result)

        self.assertTrue(t1.done())
        self.assertIsNone(t1.result())
        self.assertTrue(t2.done())
        self.assertIsNone(t2.result())
        self.assertTrue(t3.done())
        self.assertIsNone(t3.result())

    async def test_wait_on_set(self):
        ev = asyncio.Event()
        ev.set()

        res = await ev.wait()
        self.assertTrue(res)

    async def test_wait_cancel(self):
        ev = asyncio.Event()

        wait = asyncio.create_task(ev.wait())
        asyncio.get_running_loop().call_soon(wait.cancel)
        with self.assertRaises(asyncio.CancelledError):
            await wait
        self.assertFalse(ev._waiters)

    async def test_clear(self):
        ev = asyncio.Event()
        self.assertFalse(ev.is_set())

        ev.set()
        self.assertTrue(ev.is_set())

        ev.clear()
        self.assertFalse(ev.is_set())

    async def test_clear_with_waiters(self):
        ev = asyncio.Event()
        result = []

        async def c1(result):
            if await ev.wait():
                result.append(1)
            return True

        t = asyncio.create_task(c1(result))
        await asyncio.sleep(0)
        self.assertEqual([], result)

        ev.set()
        ev.clear()
        self.assertFalse(ev.is_set())

        ev.set()
        ev.set()
        self.assertEqual(1, len(ev._waiters))

        await asyncio.sleep(0)
        self.assertEqual([1], result)
        self.assertEqual(0, len(ev._waiters))

        self.assertTrue(t.done())
        self.assertTrue(t.result())


class ConditionTests(unittest.IsolatedAsyncioTestCase):

    async def test_wait(self):
        cond = asyncio.Condition()
        result = []

        async def c1(result):
            await cond.acquire()
            if await cond.wait():
                result.append(1)
            return True

        async def c2(result):
            await cond.acquire()
            if await cond.wait():
                result.append(2)
            return True

        async def c3(result):
            await cond.acquire()
            if await cond.wait():
                result.append(3)
            return True

        t1 = asyncio.create_task(c1(result))
        t2 = asyncio.create_task(c2(result))
        t3 = asyncio.create_task(c3(result))

        await asyncio.sleep(0)
        self.assertEqual([], result)
        self.assertFalse(cond.locked())

        self.assertTrue(await cond.acquire())
        cond.notify()
        await asyncio.sleep(0)
        self.assertEqual([], result)
        self.assertTrue(cond.locked())

        cond.release()
        await asyncio.sleep(0)
        self.assertEqual([1], result)
        self.assertTrue(cond.locked())

        cond.notify(2)
        await asyncio.sleep(0)
        self.assertEqual([1], result)
        self.assertTrue(cond.locked())

        cond.release()
        await asyncio.sleep(0)
        self.assertEqual([1, 2], result)
        self.assertTrue(cond.locked())

        cond.release()
        await asyncio.sleep(0)
        self.assertEqual([1, 2, 3], result)
        self.assertTrue(cond.locked())

        self.assertTrue(t1.done())
        self.assertTrue(t1.result())
        self.assertTrue(t2.done())
        self.assertTrue(t2.result())
        self.assertTrue(t3.done())
        self.assertTrue(t3.result())

    async def test_wait_cancel(self):
        cond = asyncio.Condition()
        await cond.acquire()

        wait = asyncio.create_task(cond.wait())
        asyncio.get_running_loop().call_soon(wait.cancel)
        with self.assertRaises(asyncio.CancelledError):
            await wait
        self.assertFalse(cond._waiters)
        self.assertTrue(cond.locked())

    async def test_wait_cancel_contested(self):
        cond = asyncio.Condition()

        await cond.acquire()
        self.assertTrue(cond.locked())

        wait_task = asyncio.create_task(cond.wait())
        await asyncio.sleep(0)
        self.assertFalse(cond.locked())

        # Notify, but contest the lock before cancelling
        await cond.acquire()
        self.assertTrue(cond.locked())
        cond.notify()
        asyncio.get_running_loop().call_soon(wait_task.cancel)
        asyncio.get_running_loop().call_soon(cond.release)

        try:
            await wait_task
        except asyncio.CancelledError:
            # Should not happen, since no cancellation points
            pass

        self.assertTrue(cond.locked())

    async def test_wait_cancel_after_notify(self):
        # See bpo-32841
        waited = False

        cond = asyncio.Condition()

        async def wait_on_cond():
            nonlocal waited
            async with cond:
                waited = True  # Make sure this area was reached
                await cond.wait()

        waiter = asyncio.create_task(wait_on_cond())
        await asyncio.sleep(0)  # Start waiting

        await cond.acquire()
        cond.notify()
        await asyncio.sleep(0)  # Get to acquire()
        waiter.cancel()
        await asyncio.sleep(0)  # Activate cancellation
        cond.release()
        await asyncio.sleep(0)  # Cancellation should occur

        self.assertTrue(waiter.cancelled())
        self.assertTrue(waited)

    async def test_wait_unacquired(self):
        cond = asyncio.Condition()
        with self.assertRaises(RuntimeError):
            await cond.wait()

    async def test_wait_for(self):
        cond = asyncio.Condition()
        presult = False

        def predicate():
            return presult

        result = []

        async def c1(result):
            await cond.acquire()
            if await cond.wait_for(predicate):
                result.append(1)
                cond.release()
            return True

        t = asyncio.create_task(c1(result))

        await asyncio.sleep(0)
        self.assertEqual([], result)

        await cond.acquire()
        cond.notify()
        cond.release()
        await asyncio.sleep(0)
        self.assertEqual([], result)

        presult = True
        await cond.acquire()
        cond.notify()
        cond.release()
        await asyncio.sleep(0)
        self.assertEqual([1], result)

        self.assertTrue(t.done())
        self.assertTrue(t.result())

    async def test_wait_for_unacquired(self):
        cond = asyncio.Condition()

        # predicate can return true immediately
        res = await cond.wait_for(lambda: [1, 2, 3])
        self.assertEqual([1, 2, 3], res)

        with self.assertRaises(RuntimeError):
            await cond.wait_for(lambda: False)

    async def test_notify(self):
        cond = asyncio.Condition()
        result = []

        async def c1(result):
            await cond.acquire()
            if await cond.wait():
                result.append(1)
                cond.release()
            return True

        async def c2(result):
            await cond.acquire()
            if await cond.wait():
                result.append(2)
                cond.release()
            return True

        async def c3(result):
            await cond.acquire()
            if await cond.wait():
                result.append(3)
                cond.release()
            return True

        t1 = asyncio.create_task(c1(result))
        t2 = asyncio.create_task(c2(result))
        t3 = asyncio.create_task(c3(result))

        await asyncio.sleep(0)
        self.assertEqual([], result)

        await cond.acquire()
        cond.notify(1)
        cond.release()
        await asyncio.sleep(0)
        self.assertEqual([1], result)

        await cond.acquire()
        cond.notify(1)
        cond.notify(2048)
        cond.release()
        await asyncio.sleep(0)
        self.assertEqual([1, 2, 3], result)

        self.assertTrue(t1.done())
        self.assertTrue(t1.result())
        self.assertTrue(t2.done())
        self.assertTrue(t2.result())
        self.assertTrue(t3.done())
        self.assertTrue(t3.result())

    async def test_notify_all(self):
        cond = asyncio.Condition()

        result = []

        async def c1(result):
            await cond.acquire()
            if await cond.wait():
                result.append(1)
                cond.release()
            return True

        async def c2(result):
            await cond.acquire()
            if await cond.wait():
                result.append(2)
                cond.release()
            return True

        t1 = asyncio.create_task(c1(result))
        t2 = asyncio.create_task(c2(result))

        await asyncio.sleep(0)
        self.assertEqual([], result)

        await cond.acquire()
        cond.notify_all()
        cond.release()
        await asyncio.sleep(0)
        self.assertEqual([1, 2], result)

        self.assertTrue(t1.done())
        self.assertTrue(t1.result())
        self.assertTrue(t2.done())
        self.assertTrue(t2.result())

    def test_notify_unacquired(self):
        cond = asyncio.Condition()
        self.assertRaises(RuntimeError, cond.notify)

    def test_notify_all_unacquired(self):
        cond = asyncio.Condition()
        self.assertRaises(RuntimeError, cond.notify_all)

    async def test_repr(self):
        cond = asyncio.Condition()
        self.assertTrue('unlocked' in repr(cond))
        self.assertTrue(RGX_REPR.match(repr(cond)))

        await cond.acquire()
        self.assertTrue('locked' in repr(cond))

        cond._waiters.append(mock.Mock())
        self.assertTrue('waiters:1' in repr(cond))
        self.assertTrue(RGX_REPR.match(repr(cond)))

        cond._waiters.append(mock.Mock())
        self.assertTrue('waiters:2' in repr(cond))
        self.assertTrue(RGX_REPR.match(repr(cond)))

    async def test_context_manager(self):
        cond = asyncio.Condition()
        self.assertFalse(cond.locked())
        async with cond:
            self.assertTrue(cond.locked())
        self.assertFalse(cond.locked())

    async def test_explicit_lock(self):
        async def f(lock=None, cond=None):
            if lock is None:
                lock = asyncio.Lock()
            if cond is None:
                cond = asyncio.Condition(lock)
            self.assertIs(cond._lock, lock)
            self.assertFalse(lock.locked())
            self.assertFalse(cond.locked())
            async with cond:
                self.assertTrue(lock.locked())
                self.assertTrue(cond.locked())
            self.assertFalse(lock.locked())
            self.assertFalse(cond.locked())
            async with lock:
                self.assertTrue(lock.locked())
                self.assertTrue(cond.locked())
            self.assertFalse(lock.locked())
            self.assertFalse(cond.locked())

        # All should work in the same way.
        await f()
        await f(asyncio.Lock())
        lock = asyncio.Lock()
        await f(lock, asyncio.Condition(lock))

    async def test_ambiguous_loops(self):
        loop = asyncio.new_event_loop()
        self.addCleanup(loop.close)

        async def wrong_loop_in_lock():
            with self.assertRaises(TypeError):
                asyncio.Lock(loop=loop)  # actively disallowed since 3.10
            lock = asyncio.Lock()
            lock._loop = loop  # use private API for testing
            async with lock:
                # acquired immediately via the fast-path
                # without interaction with any event loop.
                cond = asyncio.Condition(lock)
                # cond.acquire() will trigger waiting on the lock
                # and it will discover the event loop mismatch.
                with self.assertRaisesRegex(
                    RuntimeError,
                    "is bound to a different event loop",
                ):
                    await cond.acquire()

        async def wrong_loop_in_cond():
            # Same analogy here with the condition's loop.
            lock = asyncio.Lock()
            async with lock:
                with self.assertRaises(TypeError):
                    asyncio.Condition(lock, loop=loop)
                cond = asyncio.Condition(lock)
                cond._loop = loop
                with self.assertRaisesRegex(
                    RuntimeError,
                    "is bound to a different event loop",
                ):
                    await cond.wait()

        await wrong_loop_in_lock()
        await wrong_loop_in_cond()

    async def test_timeout_in_block(self):
        condition = asyncio.Condition()
        async with condition:
            with self.assertRaises(asyncio.TimeoutError):
                await asyncio.wait_for(condition.wait(), timeout=0.5)


class SemaphoreTests(unittest.IsolatedAsyncioTestCase):

    def test_initial_value_zero(self):
        sem = asyncio.Semaphore(0)
        self.assertTrue(sem.locked())

    async def test_repr(self):
        sem = asyncio.Semaphore()
        self.assertTrue(repr(sem).endswith('[unlocked, value:1]>'))
        self.assertTrue(RGX_REPR.match(repr(sem)))

        await sem.acquire()
        self.assertTrue(repr(sem).endswith('[locked]>'))
        self.assertTrue('waiters' not in repr(sem))
        self.assertTrue(RGX_REPR.match(repr(sem)))

        sem._waiters.append(mock.Mock())
        self.assertTrue('waiters:1' in repr(sem))
        self.assertTrue(RGX_REPR.match(repr(sem)))

        sem._waiters.append(mock.Mock())
        self.assertTrue('waiters:2' in repr(sem))
        self.assertTrue(RGX_REPR.match(repr(sem)))

    async def test_semaphore(self):
        sem = asyncio.Semaphore()
        self.assertEqual(1, sem._value)

        with self.assertRaisesRegex(
            TypeError,
            "object Semaphore can't be used in 'await' expression",
        ):
            await sem

        self.assertFalse(sem.locked())
        self.assertEqual(1, sem._value)

    def test_semaphore_value(self):
        self.assertRaises(ValueError, asyncio.Semaphore, -1)

    async def test_acquire(self):
        sem = asyncio.Semaphore(3)
        result = []

        self.assertTrue(await sem.acquire())
        self.assertTrue(await sem.acquire())
        self.assertFalse(sem.locked())

        async def c1(result):
            await sem.acquire()
            result.append(1)
            return True

        async def c2(result):
            await sem.acquire()
            result.append(2)
            return True

        async def c3(result):
            await sem.acquire()
            result.append(3)
            return True

        async def c4(result):
            await sem.acquire()
            result.append(4)
            return True

        t1 = asyncio.create_task(c1(result))
        t2 = asyncio.create_task(c2(result))
        t3 = asyncio.create_task(c3(result))

        await asyncio.sleep(0)
        self.assertEqual([1], result)
        self.assertTrue(sem.locked())
        self.assertEqual(2, len(sem._waiters))
        self.assertEqual(0, sem._value)

        t4 = asyncio.create_task(c4(result))

        sem.release()
        sem.release()
        self.assertEqual(2, sem._value)

        await asyncio.sleep(0)
        self.assertEqual(0, sem._value)
        self.assertEqual(3, len(result))
        self.assertTrue(sem.locked())
        self.assertEqual(1, len(sem._waiters))
        self.assertEqual(0, sem._value)

        self.assertTrue(t1.done())
        self.assertTrue(t1.result())
        race_tasks = [t2, t3, t4]
        done_tasks = [t for t in race_tasks if t.done() and t.result()]
        self.assertTrue(2, len(done_tasks))

        # cleanup locked semaphore
        sem.release()
        await asyncio.gather(*race_tasks)

    async def test_acquire_cancel(self):
        sem = asyncio.Semaphore()
        await sem.acquire()

        acquire = asyncio.create_task(sem.acquire())
        asyncio.get_running_loop().call_soon(acquire.cancel)
        with self.assertRaises(asyncio.CancelledError):
            await acquire
        self.assertTrue((not sem._waiters) or
                        all(waiter.done() for waiter in sem._waiters))

    async def test_acquire_cancel_before_awoken(self):
        sem = asyncio.Semaphore(value=0)

        t1 = asyncio.create_task(sem.acquire())
        t2 = asyncio.create_task(sem.acquire())
        t3 = asyncio.create_task(sem.acquire())
        t4 = asyncio.create_task(sem.acquire())

        await asyncio.sleep(0)

        t1.cancel()
        t2.cancel()
        sem.release()

        await asyncio.sleep(0)
        num_done = sum(t.done() for t in [t3, t4])
        self.assertEqual(num_done, 1)
        self.assertTrue(t3.done())
        self.assertFalse(t4.done())

        t3.cancel()
        t4.cancel()
        await asyncio.sleep(0)

    async def test_acquire_hang(self):
        sem = asyncio.Semaphore(value=0)

        t1 = asyncio.create_task(sem.acquire())
        t2 = asyncio.create_task(sem.acquire())
        await asyncio.sleep(0)

        t1.cancel()
        sem.release()
        await asyncio.sleep(0)
        self.assertTrue(sem.locked())
        self.assertTrue(t2.done())

    def test_release_not_acquired(self):
        sem = asyncio.BoundedSemaphore()

        self.assertRaises(ValueError, sem.release)

    async def test_release_no_waiters(self):
        sem = asyncio.Semaphore()
        await sem.acquire()
        self.assertTrue(sem.locked())

        sem.release()
        self.assertFalse(sem.locked())


if __name__ == '__main__':
    unittest.main()
