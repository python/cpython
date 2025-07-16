"""
Various tests for synchronization primitives.
"""

import gc
import sys
import time
from _thread import start_new_thread, TIMEOUT_MAX
import threading
import unittest
import weakref

from test import support
from test.support import threading_helper


requires_fork = unittest.skipUnless(support.has_fork_support,
                                    "platform doesn't support fork "
                                     "(no _at_fork_reinit method)")


def wait_threads_blocked(nthread):
    # Arbitrary sleep to wait until N threads are blocked,
    # like waiting for a lock.
    time.sleep(0.010 * nthread)


class Bunch(object):
    """
    A bunch of threads.
    """
    def __init__(self, func, nthread, wait_before_exit=False):
        """
        Construct a bunch of `nthread` threads running the same function `func`.
        If `wait_before_exit` is True, the threads won't terminate until
        do_finish() is called.
        """
        self.func = func
        self.nthread = nthread
        self.started = []
        self.finished = []
        self.exceptions = []
        self._can_exit = not wait_before_exit
        self._wait_thread = None

    def task(self):
        tid = threading.get_ident()
        self.started.append(tid)
        try:
            self.func()
        except BaseException as exc:
            self.exceptions.append(exc)
        finally:
            self.finished.append(tid)
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if self._can_exit:
                    break

    def __enter__(self):
        self._wait_thread = threading_helper.wait_threads_exit(support.SHORT_TIMEOUT)
        self._wait_thread.__enter__()

        try:
            for _ in range(self.nthread):
                start_new_thread(self.task, ())
        except:
            self._can_exit = True
            raise

        for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
            if len(self.started) >= self.nthread:
                break

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
            if len(self.finished) >= self.nthread:
                break

        # Wait until threads completely exit according to _thread._count()
        self._wait_thread.__exit__(None, None, None)

        # Break reference cycle
        exceptions = self.exceptions
        self.exceptions = None
        if exceptions:
            raise ExceptionGroup(f"{self.func} threads raised exceptions",
                                 exceptions)

    def do_finish(self):
        self._can_exit = True


class BaseTestCase(unittest.TestCase):
    def setUp(self):
        self._threads = threading_helper.threading_setup()

    def tearDown(self):
        threading_helper.threading_cleanup(*self._threads)
        support.reap_children()

    def assertTimeout(self, actual, expected):
        # The waiting and/or time.monotonic() can be imprecise, which
        # is why comparing to the expected value would sometimes fail
        # (especially under Windows).
        self.assertGreaterEqual(actual, expected * 0.6)
        # Test nothing insane happened
        self.assertLess(actual, expected * 10.0)


class BaseLockTests(BaseTestCase):
    """
    Tests for both recursive and non-recursive locks.
    """

    def wait_phase(self, phase, expected):
        for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
            if len(phase) >= expected:
                break
        self.assertEqual(len(phase), expected)

    def test_constructor(self):
        lock = self.locktype()
        del lock

    def test_constructor_noargs(self):
        self.assertRaises(TypeError, self.locktype, 1)
        self.assertRaises(TypeError, self.locktype, x=1)
        self.assertRaises(TypeError, self.locktype, 1, x=2)

    def test_repr(self):
        lock = self.locktype()
        self.assertRegex(repr(lock), "<unlocked .* object (.*)?at .*>")
        del lock

    def test_locked_repr(self):
        lock = self.locktype()
        lock.acquire()
        self.assertRegex(repr(lock), "<locked .* object (.*)?at .*>")
        del lock

    def test_acquire_destroy(self):
        lock = self.locktype()
        lock.acquire()
        del lock

    def test_acquire_release(self):
        lock = self.locktype()
        lock.acquire()
        lock.release()
        del lock

    def test_try_acquire(self):
        lock = self.locktype()
        self.assertTrue(lock.acquire(False))
        lock.release()

    def test_try_acquire_contended(self):
        lock = self.locktype()
        lock.acquire()
        result = []
        def f():
            result.append(lock.acquire(False))
        with Bunch(f, 1):
            pass
        self.assertFalse(result[0])
        lock.release()

    def test_acquire_contended(self):
        lock = self.locktype()
        lock.acquire()
        def f():
            lock.acquire()
            lock.release()

        N = 5
        with Bunch(f, N) as bunch:
            # Threads block on lock.acquire()
            wait_threads_blocked(N)
            self.assertEqual(len(bunch.finished), 0)

            # Threads unblocked
            lock.release()

        self.assertEqual(len(bunch.finished), N)

    def test_with(self):
        lock = self.locktype()
        def f():
            lock.acquire()
            lock.release()

        def with_lock(err=None):
            with lock:
                if err is not None:
                    raise err

        # Acquire the lock, do nothing, with releases the lock
        with lock:
            pass

        # Check that the lock is unacquired
        with Bunch(f, 1):
            pass

        # Acquire the lock, raise an exception, with releases the lock
        with self.assertRaises(TypeError):
            with lock:
                raise TypeError

        # Check that the lock is unacquired even if after an exception
        # was raised in the previous "with lock:" block
        with Bunch(f, 1):
            pass

    def test_thread_leak(self):
        # The lock shouldn't leak a Thread instance when used from a foreign
        # (non-threading) thread.
        lock = self.locktype()
        def f():
            lock.acquire()
            lock.release()

        # We run many threads in the hope that existing threads ids won't
        # be recycled.
        with Bunch(f, 15):
            pass

    def test_timeout(self):
        lock = self.locktype()
        # Can't set timeout if not blocking
        self.assertRaises(ValueError, lock.acquire, False, 1)
        # Invalid timeout values
        self.assertRaises(ValueError, lock.acquire, timeout=-100)
        self.assertRaises(OverflowError, lock.acquire, timeout=1e100)
        self.assertRaises(OverflowError, lock.acquire, timeout=TIMEOUT_MAX + 1)
        # TIMEOUT_MAX is ok
        lock.acquire(timeout=TIMEOUT_MAX)
        lock.release()
        t1 = time.monotonic()
        self.assertTrue(lock.acquire(timeout=5))
        t2 = time.monotonic()
        # Just a sanity test that it didn't actually wait for the timeout.
        self.assertLess(t2 - t1, 5)
        results = []
        def f():
            t1 = time.monotonic()
            results.append(lock.acquire(timeout=0.5))
            t2 = time.monotonic()
            results.append(t2 - t1)
        with Bunch(f, 1):
            pass
        self.assertFalse(results[0])
        self.assertTimeout(results[1], 0.5)

    def test_weakref_exists(self):
        lock = self.locktype()
        ref = weakref.ref(lock)
        self.assertIsNotNone(ref())

    def test_weakref_deleted(self):
        lock = self.locktype()
        ref = weakref.ref(lock)
        del lock
        gc.collect()  # For PyPy or other GCs.
        self.assertIsNone(ref())


class LockTests(BaseLockTests):
    """
    Tests for non-recursive, weak locks
    (which can be acquired and released from different threads).
    """
    def test_reacquire(self):
        # Lock needs to be released before re-acquiring.
        lock = self.locktype()
        phase = []

        def f():
            lock.acquire()
            phase.append(None)
            lock.acquire()
            phase.append(None)

        with threading_helper.wait_threads_exit():
            # Thread blocked on lock.acquire()
            start_new_thread(f, ())
            self.wait_phase(phase, 1)

            # Thread unblocked
            lock.release()
            self.wait_phase(phase, 2)

    def test_different_thread(self):
        # Lock can be released from a different thread.
        lock = self.locktype()
        lock.acquire()
        def f():
            lock.release()
        with Bunch(f, 1):
            pass
        lock.acquire()
        lock.release()

    def test_state_after_timeout(self):
        # Issue #11618: check that lock is in a proper state after a
        # (non-zero) timeout.
        lock = self.locktype()
        lock.acquire()
        self.assertFalse(lock.acquire(timeout=0.01))
        lock.release()
        self.assertFalse(lock.locked())
        self.assertTrue(lock.acquire(blocking=False))

    @requires_fork
    def test_at_fork_reinit(self):
        def use_lock(lock):
            # make sure that the lock still works normally
            # after _at_fork_reinit()
            lock.acquire()
            lock.release()

        # unlocked
        lock = self.locktype()
        lock._at_fork_reinit()
        use_lock(lock)

        # locked: _at_fork_reinit() resets the lock to the unlocked state
        lock2 = self.locktype()
        lock2.acquire()
        lock2._at_fork_reinit()
        use_lock(lock2)


class RLockTests(BaseLockTests):
    """
    Tests for recursive locks.
    """
    def test_repr_count(self):
        # see gh-134322: check that count values are correct:
        # when a rlock is just created,
        # in a second thread when rlock is acquired in the main thread.
        lock = self.locktype()
        self.assertIn("count=0", repr(lock))
        self.assertIn("<unlocked", repr(lock))
        lock.acquire()
        lock.acquire()
        self.assertIn("count=2", repr(lock))
        self.assertIn("<locked", repr(lock))

        result = []
        def call_repr():
            result.append(repr(lock))
        with Bunch(call_repr, 1):
            pass
        self.assertIn("count=2", result[0])
        self.assertIn("<locked", result[0])

    def test_reacquire(self):
        lock = self.locktype()
        lock.acquire()
        lock.acquire()
        lock.release()
        lock.acquire()
        lock.release()
        lock.release()

    def test_release_unacquired(self):
        # Cannot release an unacquired lock
        lock = self.locktype()
        self.assertRaises(RuntimeError, lock.release)
        lock.acquire()
        lock.acquire()
        lock.release()
        lock.acquire()
        lock.release()
        lock.release()
        self.assertRaises(RuntimeError, lock.release)

    def test_locked(self):
        lock = self.locktype()
        self.assertFalse(lock.locked())
        lock.acquire()
        self.assertTrue(lock.locked())
        lock.acquire()
        self.assertTrue(lock.locked())
        lock.release()
        self.assertTrue(lock.locked())
        lock.release()
        self.assertFalse(lock.locked())

    def test_locked_with_2threads(self):
        # see gh-134323: check that a rlock which
        # is acquired in a different thread,
        # is still locked in the main thread.
        result = []
        rlock = self.locktype()
        self.assertFalse(rlock.locked())
        def acquire():
            result.append(rlock.locked())
            rlock.acquire()
            result.append(rlock.locked())

        with Bunch(acquire, 1):
            pass
        self.assertTrue(rlock.locked())
        self.assertFalse(result[0])
        self.assertTrue(result[1])

    def test_release_save_unacquired(self):
        # Cannot _release_save an unacquired lock
        lock = self.locktype()
        self.assertRaises(RuntimeError, lock._release_save)
        lock.acquire()
        lock.acquire()
        lock.release()
        lock.acquire()
        lock.release()
        lock.release()
        self.assertRaises(RuntimeError, lock._release_save)

    def test_recursion_count(self):
        lock = self.locktype()
        self.assertEqual(0, lock._recursion_count())
        lock.acquire()
        self.assertEqual(1, lock._recursion_count())
        lock.acquire()
        lock.acquire()
        self.assertEqual(3, lock._recursion_count())
        lock.release()
        self.assertEqual(2, lock._recursion_count())
        lock.release()
        lock.release()
        self.assertEqual(0, lock._recursion_count())

        phase = []

        def f():
            lock.acquire()
            phase.append(None)

            self.wait_phase(phase, 2)
            lock.release()
            phase.append(None)

        with threading_helper.wait_threads_exit():
            # Thread blocked on lock.acquire()
            start_new_thread(f, ())
            self.wait_phase(phase, 1)
            self.assertEqual(0, lock._recursion_count())

            # Thread unblocked
            phase.append(None)
            self.wait_phase(phase, 3)
            self.assertEqual(0, lock._recursion_count())

    def test_different_thread(self):
        # Cannot release from a different thread
        lock = self.locktype()
        def f():
            lock.acquire()

        with Bunch(f, 1, True) as bunch:
            try:
                self.assertRaises(RuntimeError, lock.release)
            finally:
                bunch.do_finish()

    def test__is_owned(self):
        lock = self.locktype()
        self.assertFalse(lock._is_owned())
        lock.acquire()
        self.assertTrue(lock._is_owned())
        lock.acquire()
        self.assertTrue(lock._is_owned())
        result = []
        def f():
            result.append(lock._is_owned())
        with Bunch(f, 1):
            pass
        self.assertFalse(result[0])
        lock.release()
        self.assertTrue(lock._is_owned())
        lock.release()
        self.assertFalse(lock._is_owned())


class EventTests(BaseTestCase):
    """
    Tests for Event objects.
    """

    def test_is_set(self):
        evt = self.eventtype()
        self.assertFalse(evt.is_set())
        evt.set()
        self.assertTrue(evt.is_set())
        evt.set()
        self.assertTrue(evt.is_set())
        evt.clear()
        self.assertFalse(evt.is_set())
        evt.clear()
        self.assertFalse(evt.is_set())

    def _check_notify(self, evt):
        # All threads get notified
        N = 5
        results1 = []
        results2 = []
        def f():
            results1.append(evt.wait())
            results2.append(evt.wait())

        with Bunch(f, N):
            # Threads blocked on first evt.wait()
            wait_threads_blocked(N)
            self.assertEqual(len(results1), 0)

            # Threads unblocked
            evt.set()

        self.assertEqual(results1, [True] * N)
        self.assertEqual(results2, [True] * N)

    def test_notify(self):
        evt = self.eventtype()
        self._check_notify(evt)
        # Another time, after an explicit clear()
        evt.set()
        evt.clear()
        self._check_notify(evt)

    def test_timeout(self):
        evt = self.eventtype()
        results1 = []
        results2 = []
        N = 5
        def f():
            results1.append(evt.wait(0.0))
            t1 = time.monotonic()
            r = evt.wait(0.5)
            t2 = time.monotonic()
            results2.append((r, t2 - t1))

        with Bunch(f, N):
            pass

        self.assertEqual(results1, [False] * N)
        for r, dt in results2:
            self.assertFalse(r)
            self.assertTimeout(dt, 0.5)

        # The event is set
        results1 = []
        results2 = []
        evt.set()
        with Bunch(f, N):
            pass

        self.assertEqual(results1, [True] * N)
        for r, dt in results2:
            self.assertTrue(r)

    def test_set_and_clear(self):
        # gh-57711: check that wait() returns true even when the event is
        # cleared before the waiting thread is woken up.
        event = self.eventtype()
        results = []
        def f():
            results.append(event.wait(support.LONG_TIMEOUT))

        N = 5
        with Bunch(f, N):
            # Threads blocked on event.wait()
            wait_threads_blocked(N)

            # Threads unblocked
            event.set()
            event.clear()

        self.assertEqual(results, [True] * N)

    @requires_fork
    def test_at_fork_reinit(self):
        # ensure that condition is still using a Lock after reset
        evt = self.eventtype()
        with evt._cond:
            self.assertFalse(evt._cond.acquire(False))
        evt._at_fork_reinit()
        with evt._cond:
            self.assertFalse(evt._cond.acquire(False))

    def test_repr(self):
        evt = self.eventtype()
        self.assertRegex(repr(evt), r"<\w+\.Event at .*: unset>")
        evt.set()
        self.assertRegex(repr(evt), r"<\w+\.Event at .*: set>")


class ConditionTests(BaseTestCase):
    """
    Tests for condition variables.
    """

    def test_acquire(self):
        cond = self.condtype()
        # Be default we have an RLock: the condition can be acquired multiple
        # times.
        cond.acquire()
        cond.acquire()
        cond.release()
        cond.release()
        lock = threading.Lock()
        cond = self.condtype(lock)
        cond.acquire()
        self.assertFalse(lock.acquire(False))
        cond.release()
        self.assertTrue(lock.acquire(False))
        self.assertFalse(cond.acquire(False))
        lock.release()
        with cond:
            self.assertFalse(lock.acquire(False))

    def test_unacquired_wait(self):
        cond = self.condtype()
        self.assertRaises(RuntimeError, cond.wait)

    def test_unacquired_notify(self):
        cond = self.condtype()
        self.assertRaises(RuntimeError, cond.notify)

    def _check_notify(self, cond):
        # Note that this test is sensitive to timing.  If the worker threads
        # don't execute in a timely fashion, the main thread may think they
        # are further along then they are.  The main thread therefore issues
        # wait_threads_blocked() statements to try to make sure that it doesn't
        # race ahead of the workers.
        # Secondly, this test assumes that condition variables are not subject
        # to spurious wakeups.  The absence of spurious wakeups is an implementation
        # detail of Condition Variables in current CPython, but in general, not
        # a guaranteed property of condition variables as a programming
        # construct.  In particular, it is possible that this can no longer
        # be conveniently guaranteed should their implementation ever change.
        ready = []
        results1 = []
        results2 = []
        phase_num = 0
        def f():
            cond.acquire()
            ready.append(phase_num)
            result = cond.wait()

            cond.release()
            results1.append((result, phase_num))

            cond.acquire()
            ready.append(phase_num)

            result = cond.wait()
            cond.release()
            results2.append((result, phase_num))

        N = 5
        with Bunch(f, N):
            # first wait, to ensure all workers settle into cond.wait() before
            # we continue. See issues #8799 and #30727.
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if len(ready) >= N:
                    break

            ready.clear()
            self.assertEqual(results1, [])

            # Notify 3 threads at first
            count1 = 3
            cond.acquire()
            cond.notify(count1)
            wait_threads_blocked(count1)

            # Phase 1
            phase_num = 1
            cond.release()
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if len(results1) >= count1:
                    break

            self.assertEqual(results1, [(True, 1)] * count1)
            self.assertEqual(results2, [])

            # Wait until awaken workers are blocked on cond.wait()
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if len(ready) >= count1 :
                    break

            # Notify 5 threads: they might be in their first or second wait
            cond.acquire()
            cond.notify(5)
            wait_threads_blocked(N)

            # Phase 2
            phase_num = 2
            cond.release()
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if len(results1) + len(results2) >= (N + count1):
                    break

            count2 = N - count1
            self.assertEqual(results1, [(True, 1)] * count1 + [(True, 2)] * count2)
            self.assertEqual(results2, [(True, 2)] * count1)

            # Make sure all workers settle into cond.wait()
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if len(ready) >= N:
                    break

            # Notify all threads: they are all in their second wait
            cond.acquire()
            cond.notify_all()
            wait_threads_blocked(N)

            # Phase 3
            phase_num = 3
            cond.release()
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if len(results2) >= N:
                    break
            self.assertEqual(results1, [(True, 1)] * count1 + [(True, 2)] * count2)
            self.assertEqual(results2, [(True, 2)] * count1 + [(True, 3)] * count2)

    def test_notify(self):
        cond = self.condtype()
        self._check_notify(cond)
        # A second time, to check internal state is still ok.
        self._check_notify(cond)

    def test_timeout(self):
        cond = self.condtype()
        timeout = 0.5
        results = []
        def f():
            cond.acquire()
            t1 = time.monotonic()
            result = cond.wait(timeout)
            t2 = time.monotonic()
            cond.release()
            results.append((t2 - t1, result))

        N = 5
        with Bunch(f, N):
            pass
        self.assertEqual(len(results), N)

        for dt, result in results:
            self.assertTimeout(dt, timeout)
            # Note that conceptually (that"s the condition variable protocol)
            # a wait() may succeed even if no one notifies us and before any
            # timeout occurs.  Spurious wakeups can occur.
            # This makes it hard to verify the result value.
            # In practice, this implementation has no spurious wakeups.
            self.assertFalse(result)

    def test_waitfor(self):
        cond = self.condtype()
        state = 0
        def f():
            with cond:
                result = cond.wait_for(lambda: state == 4)
                self.assertTrue(result)
                self.assertEqual(state, 4)

        with Bunch(f, 1):
            for i in range(4):
                time.sleep(0.010)
                with cond:
                    state += 1
                    cond.notify()

    def test_waitfor_timeout(self):
        cond = self.condtype()
        state = 0
        success = []
        def f():
            with cond:
                dt = time.monotonic()
                result = cond.wait_for(lambda : state==4, timeout=0.1)
                dt = time.monotonic() - dt
                self.assertFalse(result)
                self.assertTimeout(dt, 0.1)
                success.append(None)

        with Bunch(f, 1):
            # Only increment 3 times, so state == 4 is never reached.
            for i in range(3):
                time.sleep(0.010)
                with cond:
                    state += 1
                    cond.notify()

        self.assertEqual(len(success), 1)


class BaseSemaphoreTests(BaseTestCase):
    """
    Common tests for {bounded, unbounded} semaphore objects.
    """

    def test_constructor(self):
        self.assertRaises(ValueError, self.semtype, value = -1)
        self.assertRaises(ValueError, self.semtype, value = -sys.maxsize)

    def test_acquire(self):
        sem = self.semtype(1)
        sem.acquire()
        sem.release()
        sem = self.semtype(2)
        sem.acquire()
        sem.acquire()
        sem.release()
        sem.release()

    def test_acquire_destroy(self):
        sem = self.semtype()
        sem.acquire()
        del sem

    def test_acquire_contended(self):
        sem_value = 7
        sem = self.semtype(sem_value)
        sem.acquire()

        sem_results = []
        results1 = []
        results2 = []
        phase_num = 0

        def func():
            sem_results.append(sem.acquire())
            results1.append(phase_num)

            sem_results.append(sem.acquire())
            results2.append(phase_num)

        def wait_count(count):
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if len(results1) + len(results2) >= count:
                    break

        N = 10
        with Bunch(func, N):
            # Phase 0
            count1 = sem_value - 1
            wait_count(count1)
            self.assertEqual(results1 + results2, [0] * count1)

            # Phase 1
            phase_num = 1
            for i in range(sem_value):
                sem.release()
            count2 = sem_value
            wait_count(count1 + count2)
            self.assertEqual(sorted(results1 + results2),
                             [0] * count1 + [1] * count2)

            # Phase 2
            phase_num = 2
            count3 = (sem_value - 1)
            for i in range(count3):
                sem.release()
            wait_count(count1 + count2 + count3)
            self.assertEqual(sorted(results1 + results2),
                             [0] * count1 + [1] * count2 + [2] * count3)
            # The semaphore is still locked
            self.assertFalse(sem.acquire(False))

            # Final release, to let the last thread finish
            count4 = 1
            sem.release()

        self.assertEqual(sem_results,
                         [True] * (count1 + count2 + count3 + count4))

    def test_multirelease(self):
        sem_value = 7
        sem = self.semtype(sem_value)
        sem.acquire()

        results1 = []
        results2 = []
        phase_num = 0
        def func():
            sem.acquire()
            results1.append(phase_num)

            sem.acquire()
            results2.append(phase_num)

        def wait_count(count):
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if len(results1) + len(results2) >= count:
                    break

        with Bunch(func, 10):
            # Phase 0
            count1 = sem_value - 1
            wait_count(count1)
            self.assertEqual(results1 + results2, [0] * count1)

            # Phase 1
            phase_num = 1
            count2 = sem_value
            sem.release(count2)
            wait_count(count1 + count2)
            self.assertEqual(sorted(results1 + results2),
                             [0] * count1 + [1] * count2)

            # Phase 2
            phase_num = 2
            count3 = sem_value - 1
            sem.release(count3)
            wait_count(count1 + count2 + count3)
            self.assertEqual(sorted(results1 + results2),
                             [0] * count1 + [1] * count2 + [2] * count3)
            # The semaphore is still locked
            self.assertFalse(sem.acquire(False))

            # Final release, to let the last thread finish
            sem.release()

    def test_try_acquire(self):
        sem = self.semtype(2)
        self.assertTrue(sem.acquire(False))
        self.assertTrue(sem.acquire(False))
        self.assertFalse(sem.acquire(False))
        sem.release()
        self.assertTrue(sem.acquire(False))

    def test_try_acquire_contended(self):
        sem = self.semtype(4)
        sem.acquire()
        results = []
        def f():
            results.append(sem.acquire(False))
            results.append(sem.acquire(False))
        with Bunch(f, 5):
            pass
        # There can be a thread switch between acquiring the semaphore and
        # appending the result, therefore results will not necessarily be
        # ordered.
        self.assertEqual(sorted(results), [False] * 7 + [True] *  3 )

    def test_acquire_timeout(self):
        sem = self.semtype(2)
        self.assertRaises(ValueError, sem.acquire, False, timeout=1.0)
        self.assertTrue(sem.acquire(timeout=0.005))
        self.assertTrue(sem.acquire(timeout=0.005))
        self.assertFalse(sem.acquire(timeout=0.005))
        sem.release()
        self.assertTrue(sem.acquire(timeout=0.005))
        t = time.monotonic()
        self.assertFalse(sem.acquire(timeout=0.5))
        dt = time.monotonic() - t
        self.assertTimeout(dt, 0.5)

    def test_default_value(self):
        # The default initial value is 1.
        sem = self.semtype()
        sem.acquire()
        def f():
            sem.acquire()
            sem.release()

        with Bunch(f, 1) as bunch:
            # Thread blocked on sem.acquire()
            wait_threads_blocked(1)
            self.assertFalse(bunch.finished)

            # Thread unblocked
            sem.release()

    def test_with(self):
        sem = self.semtype(2)
        def _with(err=None):
            with sem:
                self.assertTrue(sem.acquire(False))
                sem.release()
                with sem:
                    self.assertFalse(sem.acquire(False))
                    if err:
                        raise err
        _with()
        self.assertTrue(sem.acquire(False))
        sem.release()
        self.assertRaises(TypeError, _with, TypeError)
        self.assertTrue(sem.acquire(False))
        sem.release()

class SemaphoreTests(BaseSemaphoreTests):
    """
    Tests for unbounded semaphores.
    """

    def test_release_unacquired(self):
        # Unbounded releases are allowed and increment the semaphore's value
        sem = self.semtype(1)
        sem.release()
        sem.acquire()
        sem.acquire()
        sem.release()

    def test_repr(self):
        sem = self.semtype(3)
        self.assertRegex(repr(sem), r"<\w+\.Semaphore at .*: value=3>")
        sem.acquire()
        self.assertRegex(repr(sem), r"<\w+\.Semaphore at .*: value=2>")
        sem.release()
        sem.release()
        self.assertRegex(repr(sem), r"<\w+\.Semaphore at .*: value=4>")


class BoundedSemaphoreTests(BaseSemaphoreTests):
    """
    Tests for bounded semaphores.
    """

    def test_release_unacquired(self):
        # Cannot go past the initial value
        sem = self.semtype()
        self.assertRaises(ValueError, sem.release)
        sem.acquire()
        sem.release()
        self.assertRaises(ValueError, sem.release)

    def test_repr(self):
        sem = self.semtype(3)
        self.assertRegex(repr(sem), r"<\w+\.BoundedSemaphore at .*: value=3/3>")
        sem.acquire()
        self.assertRegex(repr(sem), r"<\w+\.BoundedSemaphore at .*: value=2/3>")


class BarrierTests(BaseTestCase):
    """
    Tests for Barrier objects.
    """
    N = 5
    defaultTimeout = 2.0

    def setUp(self):
        self.barrier = self.barriertype(self.N, timeout=self.defaultTimeout)

    def tearDown(self):
        self.barrier.abort()

    def run_threads(self, f):
        with Bunch(f, self.N):
            pass

    def multipass(self, results, n):
        m = self.barrier.parties
        self.assertEqual(m, self.N)
        for i in range(n):
            results[0].append(True)
            self.assertEqual(len(results[1]), i * m)
            self.barrier.wait()
            results[1].append(True)
            self.assertEqual(len(results[0]), (i + 1) * m)
            self.barrier.wait()
        self.assertEqual(self.barrier.n_waiting, 0)
        self.assertFalse(self.barrier.broken)

    def test_constructor(self):
        self.assertRaises(ValueError, self.barriertype, parties=0)
        self.assertRaises(ValueError, self.barriertype, parties=-1)

    def test_barrier(self, passes=1):
        """
        Test that a barrier is passed in lockstep
        """
        results = [[],[]]
        def f():
            self.multipass(results, passes)
        self.run_threads(f)

    def test_barrier_10(self):
        """
        Test that a barrier works for 10 consecutive runs
        """
        return self.test_barrier(10)

    def test_wait_return(self):
        """
        test the return value from barrier.wait
        """
        results = []
        def f():
            r = self.barrier.wait()
            results.append(r)

        self.run_threads(f)
        self.assertEqual(sum(results), sum(range(self.N)))

    def test_action(self):
        """
        Test the 'action' callback
        """
        results = []
        def action():
            results.append(True)
        barrier = self.barriertype(self.N, action)
        def f():
            barrier.wait()
            self.assertEqual(len(results), 1)

        self.run_threads(f)

    def test_abort(self):
        """
        Test that an abort will put the barrier in a broken state
        """
        results1 = []
        results2 = []
        def f():
            try:
                i = self.barrier.wait()
                if i == self.N//2:
                    raise RuntimeError
                self.barrier.wait()
                results1.append(True)
            except threading.BrokenBarrierError:
                results2.append(True)
            except RuntimeError:
                self.barrier.abort()
                pass

        self.run_threads(f)
        self.assertEqual(len(results1), 0)
        self.assertEqual(len(results2), self.N-1)
        self.assertTrue(self.barrier.broken)

    def test_reset(self):
        """
        Test that a 'reset' on a barrier frees the waiting threads
        """
        results1 = []
        results2 = []
        results3 = []
        def f():
            i = self.barrier.wait()
            if i == self.N//2:
                # Wait until the other threads are all in the barrier.
                for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                    if self.barrier.n_waiting >= (self.N - 1):
                        break
                self.barrier.reset()
            else:
                try:
                    self.barrier.wait()
                    results1.append(True)
                except threading.BrokenBarrierError:
                    results2.append(True)
            # Now, pass the barrier again
            self.barrier.wait()
            results3.append(True)

        self.run_threads(f)
        self.assertEqual(len(results1), 0)
        self.assertEqual(len(results2), self.N-1)
        self.assertEqual(len(results3), self.N)


    def test_abort_and_reset(self):
        """
        Test that a barrier can be reset after being broken.
        """
        results1 = []
        results2 = []
        results3 = []
        barrier2 = self.barriertype(self.N)
        def f():
            try:
                i = self.barrier.wait()
                if i == self.N//2:
                    raise RuntimeError
                self.barrier.wait()
                results1.append(True)
            except threading.BrokenBarrierError:
                results2.append(True)
            except RuntimeError:
                self.barrier.abort()
                pass
            # Synchronize and reset the barrier.  Must synchronize first so
            # that everyone has left it when we reset, and after so that no
            # one enters it before the reset.
            if barrier2.wait() == self.N//2:
                self.barrier.reset()
            barrier2.wait()
            self.barrier.wait()
            results3.append(True)

        self.run_threads(f)
        self.assertEqual(len(results1), 0)
        self.assertEqual(len(results2), self.N-1)
        self.assertEqual(len(results3), self.N)

    def test_timeout(self):
        """
        Test wait(timeout)
        """
        def f():
            i = self.barrier.wait()
            if i == self.N // 2:
                # One thread is late!
                time.sleep(self.defaultTimeout / 2)
            # Default timeout is 2.0, so this is shorter.
            self.assertRaises(threading.BrokenBarrierError,
                              self.barrier.wait, self.defaultTimeout / 4)
        self.run_threads(f)

    def test_default_timeout(self):
        """
        Test the barrier's default timeout
        """
        timeout = 0.100
        barrier = self.barriertype(2, timeout=timeout)
        def f():
            self.assertRaises(threading.BrokenBarrierError,
                              barrier.wait)

        start_time = time.monotonic()
        with Bunch(f, 1):
            pass
        dt = time.monotonic() - start_time
        self.assertGreaterEqual(dt, timeout)

    def test_single_thread(self):
        b = self.barriertype(1)
        b.wait()
        b.wait()

    def test_repr(self):
        barrier = self.barriertype(3)
        timeout = support.LONG_TIMEOUT
        self.assertRegex(repr(barrier), r"<\w+\.Barrier at .*: waiters=0/3>")
        def f():
            barrier.wait(timeout)

        N = 2
        with Bunch(f, N):
            # Threads blocked on barrier.wait()
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if barrier.n_waiting >= N:
                    break
            self.assertRegex(repr(barrier),
                             r"<\w+\.Barrier at .*: waiters=2/3>")

            # Threads unblocked
            barrier.wait(timeout)

        self.assertRegex(repr(barrier),
                         r"<\w+\.Barrier at .*: waiters=0/3>")

        # Abort the barrier
        barrier.abort()
        self.assertRegex(repr(barrier),
                         r"<\w+\.Barrier at .*: broken>")
