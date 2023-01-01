"""Tests for locks.py"""

import unittest
from unittest import mock
import re

import asyncio
import collections

STR_RGX_REPR = (
    r'^<(?P<class>.*?) object at (?P<address>.*?)'
    r'\[(?P<extras>'
    r'(set|unset|locked|unlocked|filling|draining|resetting|broken)'
    r'(, value:\d)?'
    r'(, waiters:\d+)?'
    r'(, waiters:\d+\/\d+)?' # barrier
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
                rf"{cls.__name__}\.__init__\(\) got an unexpected "
                rf"keyword argument 'loop'"
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

        if sem._waiters is None:
            sem._waiters = collections.deque()

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
        self.assertEqual(0, sem._value)

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
        self.assertEqual(2, len(done_tasks))

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
        await asyncio.sleep(0)
        self.assertTrue(sem.locked())
        self.assertTrue(t2.done())

    async def test_acquire_no_hang(self):

        sem = asyncio.Semaphore(1)

        async def c1():
            async with sem:
                await asyncio.sleep(0)
            t2.cancel()

        async def c2():
            async with sem:
                self.assertFalse(True)

        t1 = asyncio.create_task(c1())
        t2 = asyncio.create_task(c2())

        r1, r2 = await asyncio.gather(t1, t2, return_exceptions=True)
        self.assertTrue(r1 is None)
        self.assertTrue(isinstance(r2, asyncio.CancelledError))

        await asyncio.wait_for(sem.acquire(), timeout=1.0)

    def test_release_not_acquired(self):
        sem = asyncio.BoundedSemaphore()

        self.assertRaises(ValueError, sem.release)

    async def test_release_no_waiters(self):
        sem = asyncio.Semaphore()
        await sem.acquire()
        self.assertTrue(sem.locked())

        sem.release()
        self.assertFalse(sem.locked())

    async def test_acquire_fifo_order(self):
        sem = asyncio.Semaphore(1)
        result = []

        async def coro(tag):
            await sem.acquire()
            result.append(f'{tag}_1')
            await asyncio.sleep(0.01)
            sem.release()

            await sem.acquire()
            result.append(f'{tag}_2')
            await asyncio.sleep(0.01)
            sem.release()

        async with asyncio.TaskGroup() as tg:
            tg.create_task(coro('c1'))
            tg.create_task(coro('c2'))
            tg.create_task(coro('c3'))

        self.assertEqual(
            ['c1_1', 'c2_1', 'c3_1', 'c1_2', 'c2_2', 'c3_2'],
            result
        )

    async def test_acquire_fifo_order_2(self):
        sem = asyncio.Semaphore(1)
        result = []

        async def c1(result):
            await sem.acquire()
            result.append(1)
            return True

        async def c2(result):
            await sem.acquire()
            result.append(2)
            sem.release()
            await sem.acquire()
            result.append(4)
            return True

        async def c3(result):
            await sem.acquire()
            result.append(3)
            return True

        t1 = asyncio.create_task(c1(result))
        t2 = asyncio.create_task(c2(result))
        t3 = asyncio.create_task(c3(result))

        await asyncio.sleep(0)

        sem.release()
        sem.release()

        tasks = [t1, t2, t3]
        await asyncio.gather(*tasks)
        self.assertEqual([1, 2, 3, 4], result)

    async def test_acquire_fifo_order_3(self):
        sem = asyncio.Semaphore(0)
        result = []

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

        t1 = asyncio.create_task(c1(result))
        t2 = asyncio.create_task(c2(result))
        t3 = asyncio.create_task(c3(result))

        await asyncio.sleep(0)

        t1.cancel()

        await asyncio.sleep(0)

        sem.release()
        sem.release()

        tasks = [t1, t2, t3]
        await asyncio.gather(*tasks, return_exceptions=True)
        self.assertEqual([2, 3], result)


class BarrierTests(unittest.IsolatedAsyncioTestCase):

    async def asyncSetUp(self):
        await super().asyncSetUp()
        self.N = 5

    def make_tasks(self, n, coro):
        tasks = [asyncio.create_task(coro()) for _ in range(n)]
        return tasks

    async def gather_tasks(self, n, coro):
        tasks = self.make_tasks(n, coro)
        res = await asyncio.gather(*tasks)
        return res, tasks

    async def test_barrier(self):
        barrier = asyncio.Barrier(self.N)
        self.assertIn("filling", repr(barrier))
        with self.assertRaisesRegex(
            TypeError,
            "object Barrier can't be used in 'await' expression",
        ):
            await barrier

        self.assertIn("filling", repr(barrier))

    async def test_repr(self):
        barrier = asyncio.Barrier(self.N)

        self.assertTrue(RGX_REPR.match(repr(barrier)))
        self.assertIn("filling", repr(barrier))

        waiters = []
        async def wait(barrier):
            await barrier.wait()

        incr = 2
        for i in range(incr):
            waiters.append(asyncio.create_task(wait(barrier)))
        await asyncio.sleep(0)

        self.assertTrue(RGX_REPR.match(repr(barrier)))
        self.assertTrue(f"waiters:{incr}/{self.N}" in repr(barrier))
        self.assertIn("filling", repr(barrier))

        # create missing waiters
        for i in range(barrier.parties - barrier.n_waiting):
            waiters.append(asyncio.create_task(wait(barrier)))
        await asyncio.sleep(0)

        self.assertTrue(RGX_REPR.match(repr(barrier)))
        self.assertIn("draining", repr(barrier))

        # add a part of waiters
        for i in range(incr):
            waiters.append(asyncio.create_task(wait(barrier)))
        await asyncio.sleep(0)
        # and reset
        await barrier.reset()

        self.assertTrue(RGX_REPR.match(repr(barrier)))
        self.assertIn("resetting", repr(barrier))

        # add a part of waiters again
        for i in range(incr):
            waiters.append(asyncio.create_task(wait(barrier)))
        await asyncio.sleep(0)
        # and abort
        await barrier.abort()

        self.assertTrue(RGX_REPR.match(repr(barrier)))
        self.assertIn("broken", repr(barrier))
        self.assertTrue(barrier.broken)

        # suppress unhandled exceptions
        await asyncio.gather(*waiters, return_exceptions=True)

    async def test_barrier_parties(self):
        self.assertRaises(ValueError, lambda: asyncio.Barrier(0))
        self.assertRaises(ValueError, lambda: asyncio.Barrier(-4))

        self.assertIsInstance(asyncio.Barrier(self.N), asyncio.Barrier)

    async def test_context_manager(self):
        self.N = 3
        barrier = asyncio.Barrier(self.N)
        results = []

        async def coro():
            async with barrier as i:
                results.append(i)

        await self.gather_tasks(self.N, coro)

        self.assertListEqual(sorted(results), list(range(self.N)))
        self.assertEqual(barrier.n_waiting, 0)
        self.assertFalse(barrier.broken)

    async def test_filling_one_task(self):
        barrier = asyncio.Barrier(1)

        async def f():
            async with barrier as i:
                return True

        ret = await f()

        self.assertTrue(ret)
        self.assertEqual(barrier.n_waiting, 0)
        self.assertFalse(barrier.broken)

    async def test_filling_one_task_twice(self):
        barrier = asyncio.Barrier(1)

        t1 = asyncio.create_task(barrier.wait())
        await asyncio.sleep(0)
        self.assertEqual(barrier.n_waiting, 0)

        t2 = asyncio.create_task(barrier.wait())
        await asyncio.sleep(0)

        self.assertEqual(t1.result(), t2.result())
        self.assertEqual(t1.done(), t2.done())

        self.assertEqual(barrier.n_waiting, 0)
        self.assertFalse(barrier.broken)

    async def test_filling_task_by_task(self):
        self.N = 3
        barrier = asyncio.Barrier(self.N)

        t1 = asyncio.create_task(barrier.wait())
        await asyncio.sleep(0)
        self.assertEqual(barrier.n_waiting, 1)
        self.assertIn("filling", repr(barrier))

        t2 = asyncio.create_task(barrier.wait())
        await asyncio.sleep(0)
        self.assertEqual(barrier.n_waiting, 2)
        self.assertIn("filling", repr(barrier))

        t3 = asyncio.create_task(barrier.wait())
        await asyncio.sleep(0)

        await asyncio.wait([t1, t2, t3])

        self.assertEqual(barrier.n_waiting, 0)
        self.assertFalse(barrier.broken)

    async def test_filling_tasks_wait_twice(self):
        barrier = asyncio.Barrier(self.N)
        results = []

        async def coro():
            async with barrier:
                results.append(True)

                async with barrier:
                    results.append(False)

        await self.gather_tasks(self.N, coro)

        self.assertEqual(len(results), self.N*2)
        self.assertEqual(results.count(True), self.N)
        self.assertEqual(results.count(False), self.N)

        self.assertEqual(barrier.n_waiting, 0)
        self.assertFalse(barrier.broken)

    async def test_filling_tasks_check_return_value(self):
        barrier = asyncio.Barrier(self.N)
        results1 = []
        results2 = []

        async def coro():
            async with barrier:
                results1.append(True)

                async with barrier as i:
                    results2.append(True)
                    return i

        res, _ = await self.gather_tasks(self.N, coro)

        self.assertEqual(len(results1), self.N)
        self.assertTrue(all(results1))
        self.assertEqual(len(results2), self.N)
        self.assertTrue(all(results2))
        self.assertListEqual(sorted(res), list(range(self.N)))

        self.assertEqual(barrier.n_waiting, 0)
        self.assertFalse(barrier.broken)

    async def test_draining_state(self):
        barrier = asyncio.Barrier(self.N)
        results = []

        async def coro():
            async with barrier:
                # barrier state change to filling for the last task release
                results.append("draining" in repr(barrier))

        await self.gather_tasks(self.N, coro)

        self.assertEqual(len(results), self.N)
        self.assertEqual(results[-1], False)
        self.assertTrue(all(results[:self.N-1]))

        self.assertEqual(barrier.n_waiting, 0)
        self.assertFalse(barrier.broken)

    async def test_blocking_tasks_while_draining(self):
        rewait = 2
        barrier = asyncio.Barrier(self.N)
        barrier_nowaiting = asyncio.Barrier(self.N - rewait)
        results = []
        rewait_n = rewait
        counter = 0

        async def coro():
            nonlocal rewait_n

            # first time waiting
            await barrier.wait()

            # after wainting once for all tasks
            if rewait_n > 0:
                rewait_n -= 1
                # wait again only for rewait tasks
                await barrier.wait()
            else:
                # wait for end of draining state`
                await barrier_nowaiting.wait()
                # wait for other waiting tasks
                await barrier.wait()

        # a success means that barrier_nowaiting
        # was waited for exactly N-rewait=3 times
        await self.gather_tasks(self.N, coro)

    async def test_filling_tasks_cancel_one(self):
        self.N = 3
        barrier = asyncio.Barrier(self.N)
        results = []

        async def coro():
            await barrier.wait()
            results.append(True)

        t1 = asyncio.create_task(coro())
        await asyncio.sleep(0)
        self.assertEqual(barrier.n_waiting, 1)

        t2 = asyncio.create_task(coro())
        await asyncio.sleep(0)
        self.assertEqual(barrier.n_waiting, 2)

        t1.cancel()
        await asyncio.sleep(0)
        self.assertEqual(barrier.n_waiting, 1)
        with self.assertRaises(asyncio.CancelledError):
            await t1
        self.assertTrue(t1.cancelled())

        t3 = asyncio.create_task(coro())
        await asyncio.sleep(0)
        self.assertEqual(barrier.n_waiting, 2)

        t4 = asyncio.create_task(coro())
        await asyncio.gather(t2, t3, t4)

        self.assertEqual(len(results), self.N)
        self.assertTrue(all(results))

        self.assertEqual(barrier.n_waiting, 0)
        self.assertFalse(barrier.broken)

    async def test_reset_barrier(self):
        barrier = asyncio.Barrier(1)

        asyncio.create_task(barrier.reset())
        await asyncio.sleep(0)

        self.assertEqual(barrier.n_waiting, 0)
        self.assertFalse(barrier.broken)

    async def test_reset_barrier_while_tasks_waiting(self):
        barrier = asyncio.Barrier(self.N)
        results = []

        async def coro():
            try:
                await barrier.wait()
            except asyncio.BrokenBarrierError:
                results.append(True)

        async def coro_reset():
            await barrier.reset()

        # N-1 tasks waiting on barrier with N parties
        tasks  = self.make_tasks(self.N-1, coro)
        await asyncio.sleep(0)

        # reset the barrier
        asyncio.create_task(coro_reset())
        await asyncio.gather(*tasks)

        self.assertEqual(len(results), self.N-1)
        self.assertTrue(all(results))
        self.assertEqual(barrier.n_waiting, 0)
        self.assertNotIn("resetting", repr(barrier))
        self.assertFalse(barrier.broken)

    async def test_reset_barrier_when_tasks_half_draining(self):
        barrier = asyncio.Barrier(self.N)
        results1 = []
        rest_of_tasks = self.N//2

        async def coro():
            try:
                await barrier.wait()
            except asyncio.BrokenBarrierError:
                # catch here waiting tasks
                results1.append(True)
            else:
                # here drained task outside the barrier
                if rest_of_tasks == barrier._count:
                    # tasks outside the barrier
                    await barrier.reset()

        await self.gather_tasks(self.N, coro)

        self.assertEqual(results1, [True]*rest_of_tasks)
        self.assertEqual(barrier.n_waiting, 0)
        self.assertNotIn("resetting", repr(barrier))
        self.assertFalse(barrier.broken)

    async def test_reset_barrier_when_tasks_half_draining_half_blocking(self):
        barrier = asyncio.Barrier(self.N)
        results1 = []
        results2 = []
        blocking_tasks = self.N//2
        count = 0

        async def coro():
            nonlocal count
            try:
                await barrier.wait()
            except asyncio.BrokenBarrierError:
                # here catch still waiting tasks
                results1.append(True)

                # so now waiting again to reach nb_parties
                await barrier.wait()
            else:
                count += 1
                if count > blocking_tasks:
                    # reset now: raise asyncio.BrokenBarrierError for waiting tasks
                    await barrier.reset()

                    # so now waiting again to reach nb_parties
                    await barrier.wait()
                else:
                    try:
                        await barrier.wait()
                    except asyncio.BrokenBarrierError:
                        # here no catch - blocked tasks go to wait
                        results2.append(True)

        await self.gather_tasks(self.N, coro)

        self.assertEqual(results1, [True]*blocking_tasks)
        self.assertEqual(results2, [])
        self.assertEqual(barrier.n_waiting, 0)
        self.assertNotIn("resetting", repr(barrier))
        self.assertFalse(barrier.broken)

    async def test_reset_barrier_while_tasks_waiting_and_waiting_again(self):
        barrier = asyncio.Barrier(self.N)
        results1 = []
        results2 = []

        async def coro1():
            try:
                await barrier.wait()
            except asyncio.BrokenBarrierError:
                results1.append(True)
            finally:
                await barrier.wait()
                results2.append(True)

        async def coro2():
            async with barrier:
                results2.append(True)

        tasks = self.make_tasks(self.N-1, coro1)

        # reset barrier, N-1 waiting tasks raise an BrokenBarrierError
        asyncio.create_task(barrier.reset())
        await asyncio.sleep(0)

        # complete waiting tasks in the `finally`
        asyncio.create_task(coro2())

        await asyncio.gather(*tasks)

        self.assertFalse(barrier.broken)
        self.assertEqual(len(results1), self.N-1)
        self.assertTrue(all(results1))
        self.assertEqual(len(results2), self.N)
        self.assertTrue(all(results2))

        self.assertEqual(barrier.n_waiting, 0)


    async def test_reset_barrier_while_tasks_draining(self):
        barrier = asyncio.Barrier(self.N)
        results1 = []
        results2 = []
        results3 = []
        count = 0

        async def coro():
            nonlocal count

            i = await barrier.wait()
            count += 1
            if count == self.N:
                # last task exited from barrier
                await barrier.reset()

                # wait here to reach the `parties`
                await barrier.wait()
            else:
                try:
                    # second waiting
                    await barrier.wait()

                    # N-1 tasks here
                    results1.append(True)
                except Exception as e:
                    # never goes here
                    results2.append(True)

            # Now, pass the barrier again
            # last wait, must be completed
            k = await barrier.wait()
            results3.append(True)

        await self.gather_tasks(self.N, coro)

        self.assertFalse(barrier.broken)
        self.assertTrue(all(results1))
        self.assertEqual(len(results1), self.N-1)
        self.assertEqual(len(results2), 0)
        self.assertEqual(len(results3), self.N)
        self.assertTrue(all(results3))

        self.assertEqual(barrier.n_waiting, 0)

    async def test_abort_barrier(self):
        barrier = asyncio.Barrier(1)

        asyncio.create_task(barrier.abort())
        await asyncio.sleep(0)

        self.assertEqual(barrier.n_waiting, 0)
        self.assertTrue(barrier.broken)

    async def test_abort_barrier_when_tasks_half_draining_half_blocking(self):
        barrier = asyncio.Barrier(self.N)
        results1 = []
        results2 = []
        blocking_tasks = self.N//2
        count = 0

        async def coro():
            nonlocal count
            try:
                await barrier.wait()
            except asyncio.BrokenBarrierError:
                # here catch tasks waiting to drain
                results1.append(True)
            else:
                count += 1
                if count > blocking_tasks:
                    # abort now: raise asyncio.BrokenBarrierError for all tasks
                    await barrier.abort()
                else:
                    try:
                        await barrier.wait()
                    except asyncio.BrokenBarrierError:
                        # here catch blocked tasks (already drained)
                        results2.append(True)

        await self.gather_tasks(self.N, coro)

        self.assertTrue(barrier.broken)
        self.assertEqual(results1, [True]*blocking_tasks)
        self.assertEqual(results2, [True]*(self.N-blocking_tasks-1))
        self.assertEqual(barrier.n_waiting, 0)
        self.assertNotIn("resetting", repr(barrier))

    async def test_abort_barrier_when_exception(self):
        # test from threading.Barrier: see `lock_tests.test_reset`
        barrier = asyncio.Barrier(self.N)
        results1 = []
        results2 = []

        async def coro():
            try:
                async with barrier as i :
                    if i == self.N//2:
                        raise RuntimeError
                async with barrier:
                    results1.append(True)
            except asyncio.BrokenBarrierError:
                results2.append(True)
            except RuntimeError:
                await barrier.abort()

        await self.gather_tasks(self.N, coro)

        self.assertTrue(barrier.broken)
        self.assertEqual(len(results1), 0)
        self.assertEqual(len(results2), self.N-1)
        self.assertTrue(all(results2))
        self.assertEqual(barrier.n_waiting, 0)

    async def test_abort_barrier_when_exception_then_resetting(self):
        # test from threading.Barrier: see `lock_tests.test_abort_and_reset``
        barrier1 = asyncio.Barrier(self.N)
        barrier2 = asyncio.Barrier(self.N)
        results1 = []
        results2 = []
        results3 = []

        async def coro():
            try:
                i = await barrier1.wait()
                if i == self.N//2:
                    raise RuntimeError
                await barrier1.wait()
                results1.append(True)
            except asyncio.BrokenBarrierError:
                results2.append(True)
            except RuntimeError:
                await barrier1.abort()

            # Synchronize and reset the barrier.  Must synchronize first so
            # that everyone has left it when we reset, and after so that no
            # one enters it before the reset.
            i = await barrier2.wait()
            if  i == self.N//2:
                await barrier1.reset()
            await barrier2.wait()
            await barrier1.wait()
            results3.append(True)

        await self.gather_tasks(self.N, coro)

        self.assertFalse(barrier1.broken)
        self.assertEqual(len(results1), 0)
        self.assertEqual(len(results2), self.N-1)
        self.assertTrue(all(results2))
        self.assertEqual(len(results3), self.N)
        self.assertTrue(all(results3))

        self.assertEqual(barrier1.n_waiting, 0)


if __name__ == '__main__':
    unittest.main()
