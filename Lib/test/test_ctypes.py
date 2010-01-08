import unittest

from test.test_support import run_unittest, import_module
#Skip tests if _ctypes module does not exist
import_module('_ctypes')

import warnings
import ctypes.test

def test_main():
    skipped, testcases = ctypes.test.get_tests(ctypes.test, "test_*.py", verbosity=0)
    suites = [unittest.makeSuite(t) for t in testcases]
    with warnings.catch_warnings():
        # Silence Py3k warnings
        warnings.filterwarnings("ignore", "buffer.. not supported",
                                DeprecationWarning)
        warnings.filterwarnings("ignore", "classic long division",
                                DeprecationWarning)
        run_unittest(unittest.TestSuite(suites))

if __name__ == "__main__":
    test_main()
