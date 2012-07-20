from importlib import _bootstrap
import time
import unittest
import weakref

from test import support

try:
    import threading
except ImportError:
    threading = None
else:
    from test import lock_tests


LockType = _bootstrap._ModuleLock
DeadlockError = _bootstrap._DeadlockError


if threading is not None:
    class ModuleLockAsRLockTests(lock_tests.RLockTests):
        locktype = staticmethod(lambda: LockType("some_lock"))

        # _is_owned() unsupported
        test__is_owned = None
        # acquire(blocking=False) unsupported
        test_try_acquire = None
        test_try_acquire_contended = None
        # `with` unsupported
        test_with = None
        # acquire(timeout=...) unsupported
        test_timeout = None
        # _release_save() unsupported
        test_release_save_unacquired = None

else:
    class ModuleLockAsRLockTests(unittest.TestCase):
        pass


@unittest.skipUnless(threading, "threads needed for this test")
class DeadlockAvoidanceTests(unittest.TestCase):

    def run_deadlock_avoidance_test(self, create_deadlock):
        NLOCKS = 10
        locks = [LockType(str(i)) for i in range(NLOCKS)]
        pairs = [(locks[i], locks[(i+1)%NLOCKS]) for i in range(NLOCKS)]
        if create_deadlock:
            NTHREADS = NLOCKS
        else:
            NTHREADS = NLOCKS - 1
        barrier = threading.Barrier(NTHREADS)
        results = []
        def _acquire(lock):
            """Try to acquire the lock. Return True on success, False on deadlock."""
            try:
                lock.acquire()
            except DeadlockError:
                return False
            else:
                return True
        def f():
            a, b = pairs.pop()
            ra = _acquire(a)
            barrier.wait()
            rb = _acquire(b)
            results.append((ra, rb))
            if rb:
                b.release()
            if ra:
                a.release()
        lock_tests.Bunch(f, NTHREADS).wait_for_finished()
        self.assertEqual(len(results), NTHREADS)
        return results

    def test_deadlock(self):
        results = self.run_deadlock_avoidance_test(True)
        # One of the threads detected a potential deadlock on its second
        # acquire() call.
        self.assertEqual(results.count((True, False)), 1)
        self.assertEqual(results.count((True, True)), len(results) - 1)

    def test_no_deadlock(self):
        results = self.run_deadlock_avoidance_test(False)
        self.assertEqual(results.count((True, False)), 0)
        self.assertEqual(results.count((True, True)), len(results))


class LifetimeTests(unittest.TestCase):

    def test_lock_lifetime(self):
        name = "xyzzy"
        self.assertNotIn(name, _bootstrap._module_locks)
        lock = _bootstrap._get_module_lock(name)
        self.assertIn(name, _bootstrap._module_locks)
        wr = weakref.ref(lock)
        del lock
        support.gc_collect()
        self.assertNotIn(name, _bootstrap._module_locks)
        self.assertIsNone(wr())

    def test_all_locks(self):
        support.gc_collect()
        self.assertEqual(0, len(_bootstrap._module_locks), _bootstrap._module_locks)


@support.reap_threads
def test_main():
    support.run_unittest(ModuleLockAsRLockTests,
                         DeadlockAvoidanceTests,
                         LifetimeTests)


if __name__ == '__main__':
    test_main()
