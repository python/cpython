import imp
import unittest
from test import test_support

try:
    import thread
except ImportError:
    thread = None

@unittest.skipUnless(thread, 'threading not available')
class LockTests(unittest.TestCase):

    """Very basic test of import lock functions."""

    def verify_lock_state(self, expected):
        self.assertEqual(imp.lock_held(), expected,
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
        # XXX (ncoghlan): It would be nice to use test_support.CleanImport
        # here, but that breaks because the os module registers some
        # handlers in copy_reg on import. Since CleanImport doesn't
        # revert that registration, the module is left in a broken
        # state after reversion. Reinitialising the module contents
        # and just reverting os.environ to its previous state is an OK
        # workaround
        with test_support.EnvironmentVarGuard():
            import os
            imp.reload(os)

    def test_extension(self):
        with test_support.CleanImport('time'):
            import time
            imp.reload(time)

    def test_builtin(self):
        with test_support.CleanImport('marshal'):
            import marshal
            imp.reload(marshal)


def test_main():
    tests = [
        ReloadTests,
        LockTests,
    ]
    test_support.run_unittest(*tests)

if __name__ == "__main__":
    test_main()
