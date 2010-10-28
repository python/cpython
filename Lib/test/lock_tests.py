"""
Various tests for synchronization primitives.
"""

import sys
import time
from _thread import start_new_thread, get_ident, TIMEOUT_MAX
import threading
import unittest

from test import support


def _wait():
    # A crude wait/yield function not relying on synchronization primitives.
    time.sleep(0.01)

class Bunch(object):
    """
    A bunch of threads.
    """
    def __init__(self, f, n, wait_before_exit=False):
        """
        Construct a bunch of `n` threads running the same function `f`.
        If `wait_before_exit` is True, the threads won't terminate until
        do_finish() is called.
        """
        self.f = f
        self.n = n
        self.started = []
        self.finished = []
        self._can_exit = not wait_before_exit
        def task():
            tid = get_ident()
            self.started.append(tid)
            try:
                f()
            finally:
                self.finished.append(tid)
                while not self._can_exit:
                    _wait()
        for i in range(n):
            start_new_thread(task, ())

    def wait_for_started(self):
        while len(self.started) < self.n:
            _wait()

    def wait_for_finished(self):
        while len(self.finished) < self.n:
            _wait()

    def do_finish(self):
        self._can_exit = True


class BaseTestCase(unittest.TestCase):
    def setUp(self):
        self._threads = support.threading_setup()

    def tearDown(self):
        support.threading_cleanup(*self._threads)
        support.reap_children()

    def assertTimeout(self, actual, expected):
        # The waiting and/or time.time() can be imprecise, which
        # is why comparing to the expected value would sometimes fail
        # (especially under Windows).
        self.assertGreaterEqual(actual, expected * 0.6)
        # Test nothing insane happened
        self.assertLess(actual, expected * 10.0)


class BaseLockTests(BaseTestCase):
    """
    Tests for both recursive and non-recursive locks.
    """

    def test_constructor(self):
        lock = self.locktype()
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
        Bunch(f, 1).wait_for_finished()
        self.assertFalse(result[0])
        lock.release()

    def test_acquire_contended(self):
        lock = self.locktype()
        lock.acquire()
        N = 5
        def f():
            lock.acquire()
            lock.release()

        b = Bunch(f, N)
        b.wait_for_started()
        _wait()
        self.assertEqual(len(b.finished), 0)
        lock.release()
        b.wait_for_finished()
        self.assertEqual(len(b.finished), N)

    def test_with(self):
        lock = self.locktype()
        def f():
            lock.acquire()
            lock.release()
        def _with(err=None):
            with lock:
                if err is not None:
                    raise err
        _with()
        # Check the lock is unacquired
        Bunch(f, 1).wait_for_finished()
        self.assertRaises(TypeError, _with, TypeError)
        # Check the lock is unacquired
        Bunch(f, 1).wait_for_finished()

    def test_thread_leak(self):
        # The lock shouldn't leak a Thread instance when used from a foreign
        # (non-threading) thread.
        lock = self.locktype()
        def f():
            lock.acquire()
            lock.release()
        n = len(threading.enumerate())
        # We run many threads in the hope that existing threads ids won't
        # be recycled.
        Bunch(f, 15).wait_for_finished()
        self.assertEqual(n, len(threading.enumerate()))

    def test_timeout(self):
        lock = self.locktype()
        # Can't set timeout if not blocking
        self.assertRaises(ValueError, lock.acquire, 0, 1)
        # Invalid timeout values
        self.assertRaises(ValueError, lock.acquire, timeout=-100)
        self.assertRaises(OverflowError, lock.acquire, timeout=1e100)
        self.assertRaises(OverflowError, lock.acquire, timeout=TIMEOUT_MAX + 1)
        # TIMEOUT_MAX is ok
        lock.acquire(timeout=TIMEOUT_MAX)
        lock.release()
        t1 = time.time()
        self.assertTrue(lock.acquire(timeout=5))
        t2 = time.time()
        # Just a sanity test that it didn't actually wait for the timeout.
        self.assertLess(t2 - t1, 5)
        results = []
        def f():
            t1 = time.time()
            results.append(lock.acquire(timeout=0.5))
            t2 = time.time()
            results.append(t2 - t1)
        Bunch(f, 1).wait_for_finished()
        self.assertFalse(results[0])
        self.assertTimeout(results[1], 0.5)


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
        start_new_thread(f, ())
        while len(phase) == 0:
            _wait()
        _wait()
        self.assertEqual(len(phase), 1)
        lock.release()
        while len(phase) == 1:
            _wait()
        self.assertEqual(len(phase), 2)

    def test_different_thread(self):
        # Lock can be released from a different thread.
        lock = self.locktype()
        lock.acquire()
        def f():
            lock.release()
        b = Bunch(f, 1)
        b.wait_for_finished()
        lock.acquire()
        lock.release()


class RLockTests(BaseLockTests):
    """
    Tests for recursive locks.
    """
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

    def test_different_thread(self):
        # Cannot release from a different thread
        lock = self.locktype()
        def f():
            lock.acquire()
        b = Bunch(f, 1, True)
        try:
            self.assertRaises(RuntimeError, lock.release)
        finally:
            b.do_finish()

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
        Bunch(f, 1).wait_for_finished()
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
        b = Bunch(f, N)
        b.wait_for_started()
        _wait()
        self.assertEqual(len(results1), 0)
        evt.set()
        b.wait_for_finished()
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
            t1 = time.time()
            r = evt.wait(0.5)
            t2 = time.time()
            results2.append((r, t2 - t1))
        Bunch(f, N).wait_for_finished()
        self.assertEqual(results1, [False] * N)
        for r, dt in results2:
            self.assertFalse(r)
            self.assertTimeout(dt, 0.5)
        # The event is set
        results1 = []
        results2 = []
        evt.set()
        Bunch(f, N).wait_for_finished()
        self.assertEqual(results1, [True] * N)
        for r, dt in results2:
            self.assertTrue(r)


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
        N = 5
        results1 = []
        results2 = []
        phase_num = 0
        def f():
            cond.acquire()
            result = cond.wait()
            cond.release()
            results1.append((result, phase_num))
            cond.acquire()
            result = cond.wait()
            cond.release()
            results2.append((result, phase_num))
        b = Bunch(f, N)
        b.wait_for_started()
        _wait()
        self.assertEqual(results1, [])
        # Notify 3 threads at first
        cond.acquire()
        cond.notify(3)
        _wait()
        phase_num = 1
        cond.release()
        while len(results1) < 3:
            _wait()
        self.assertEqual(results1, [(True, 1)] * 3)
        self.assertEqual(results2, [])
        # Notify 5 threads: they might be in their first or second wait
        cond.acquire()
        cond.notify(5)
        _wait()
        phase_num = 2
        cond.release()
        while len(results1) + len(results2) < 8:
            _wait()
        self.assertEqual(results1, [(True, 1)] * 3 + [(True, 2)] * 2)
        self.assertEqual(results2, [(True, 2)] * 3)
        # Notify all threads: they are all in their second wait
        cond.acquire()
        cond.notify_all()
        _wait()
        phase_num = 3
        cond.release()
        while len(results2) < 5:
            _wait()
        self.assertEqual(results1, [(True, 1)] * 3 + [(True,2)] * 2)
        self.assertEqual(results2, [(True, 2)] * 3 + [(True, 3)] * 2)
        b.wait_for_finished()

    def test_notify(self):
        cond = self.condtype()
        self._check_notify(cond)
        # A second time, to check internal state is still ok.
        self._check_notify(cond)

    def test_timeout(self):
        cond = self.condtype()
        results = []
        N = 5
        def f():
            cond.acquire()
            t1 = time.time()
            result = cond.wait(0.5)
            t2 = time.time()
            cond.release()
            results.append((t2 - t1, result))
        Bunch(f, N).wait_for_finished()
        self.assertEqual(len(results), N)
        for dt, result in results:
            self.assertTimeout(dt, 0.5)
            # Note that conceptually (that"s the condition variable protocol)
            # a wait() may succeed even if no one notifies us and before any
            # timeout occurs.  Spurious wakeups can occur.
            # This makes it hard to verify the result value.
            # In practice, this implementation has no spurious wakeups.
            self.assertFalse(result)


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
        sem = self.semtype(7)
        sem.acquire()
        N = 10
        results1 = []
        results2 = []
        phase_num = 0
        def f():
            sem.acquire()
            results1.append(phase_num)
            sem.acquire()
            results2.append(phase_num)
        b = Bunch(f, 10)
        b.wait_for_started()
        while len(results1) + len(results2) < 6:
            _wait()
        self.assertEqual(results1 + results2, [0] * 6)
        phase_num = 1
        for i in range(7):
            sem.release()
        while len(results1) + len(results2) < 13:
            _wait()
        self.assertEqual(sorted(results1 + results2), [0] * 6 + [1] * 7)
        phase_num = 2
        for i in range(6):
            sem.release()
        while len(results1) + len(results2) < 19:
            _wait()
        self.assertEqual(sorted(results1 + results2), [0] * 6 + [1] * 7 + [2] * 6)
        # The semaphore is still locked
        self.assertFalse(sem.acquire(False))
        # Final release, to let the last thread finish
        sem.release()
        b.wait_for_finished()

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
        Bunch(f, 5).wait_for_finished()
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
        t = time.time()
        self.assertFalse(sem.acquire(timeout=0.5))
        dt = time.time() - t
        self.assertTimeout(dt, 0.5)

    def test_default_value(self):
        # The default initial value is 1.
        sem = self.semtype()
        sem.acquire()
        def f():
            sem.acquire()
            sem.release()
        b = Bunch(f, 1)
        b.wait_for_started()
        _wait()
        self.assertFalse(b.finished)
        sem.release()
        b.wait_for_finished()

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
