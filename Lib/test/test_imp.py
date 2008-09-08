import imp
import unittest
from test import test_support


class LockTests(unittest.TestCase):

    """Very basic test of import lock functions."""

    def verify_lock_state(self, expected):
        self.failUnlessEqual(imp.lock_held(), expected,
                             "expected imp.lock_held() to be %r" % expected)
    def testLock(self):
        LOOPS = 50

        # The import lock may already be held, e.g. if the test suite is run
        # via "import test.autotest".
        lock_held_at_start = imp.lock_held()
        self.verify_lock_state(lock_held_at_start)

        for i in range(LOOPS):
            imp.acquire_lock()
            self.verify_lock_state(True)

        for i in range(LOOPS):
            imp.release_lock()

        # The original state should be restored now.
        self.verify_lock_state(lock_held_at_start)

        if not lock_held_at_start:
            try:
                imp.release_lock()
            except RuntimeError:
                pass
            else:
                self.fail("release_lock() without lock should raise "
                            "RuntimeError")

class ReloadTests(unittest.TestCase):

    """Very basic tests to make sure that imp.reload() operates just like
    reload()."""

    def test_source(self):
        import os
        imp.reload(os)

    def test_extension(self):
        import time
        imp.reload(time)

    def test_builtin(self):
        import marshal
        imp.reload(marshal)


def test_main():
    tests = [
        ReloadTests,
    ]
    try:
        import thread
    except ImportError:
        pass
    else:
        tests.append(LockTests)
    test_support.run_unittest(*tests)

if __name__ == "__main__":
    test_main()
