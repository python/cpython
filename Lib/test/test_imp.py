import imp
from test.test_support import TestFailed, TestSkipped
try:
    import thread
except ImportError:
    raise TestSkipped("test only valid when thread support is available")

def verify_lock_state(expected):
    if imp.lock_held() != expected:
        raise TestFailed("expected imp.lock_held() to be %r" % expected)

def testLock():
    LOOPS = 50

    # The import lock may already be held, e.g. if the test suite is run
    # via "import test.autotest".
    lock_held_at_start = imp.lock_held()
    verify_lock_state(lock_held_at_start)

    for i in range(LOOPS):
        imp.acquire_lock()
        verify_lock_state(True)

    for i in range(LOOPS):
        imp.release_lock()

    # The original state should be restored now.
    verify_lock_state(lock_held_at_start)

    if not lock_held_at_start:
        try:
            imp.release_lock()
        except RuntimeError:
            pass
        else:
            raise TestFailed("release_lock() without lock should raise "
                             "RuntimeError")

def test_main():
    testLock()

if __name__ == "__main__":
    test_main()
