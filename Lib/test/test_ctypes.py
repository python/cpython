import unittest

from test.support import import_module

# Skip tests if _ctypes module was not built.
import_module('_ctypes')

import ctypes.test

def load_tests(*args):
    skipped, testcases = ctypes.test.get_tests(ctypes.test, "test_*.py", verbosity=0)
    suites = [unittest.makeSuite(t) for t in testcases]
    return unittest.TestSuite(suites)

if __name__ == "__main__":
    unittest.main()
