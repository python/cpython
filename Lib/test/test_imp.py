
import imp
import unittest
from test_support import TestFailed

class ImpLock(unittest.TestCase):

    # XXX this test is woefully inadequate, please fix me
    def testLock(self):
        LOOPS = 50
        for i in range(LOOPS):
            imp.acquire_lock()
        for i in range(LOOPS):
            imp.release_lock()

        for i in range(LOOPS):
            try:
                imp.release_lock()
            except RuntimeError:
                pass
            else:
                raise TestFailed, \
                    "release_lock() without lock should raise RuntimeError"

if __name__ == "__main__":
    test_support.run_unittest(ImpLock)
