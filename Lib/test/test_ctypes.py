import unittest

from test.test_support import run_unittest, import_module, _check_py3k_warnings
#Skip tests if _ctypes module does not exist
import_module('_ctypes')


def test_main():
    with _check_py3k_warnings(("buffer.. not supported", DeprecationWarning),
                              ("classic (int|long) division", DeprecationWarning),):
        import ctypes.test
        skipped, testcases = ctypes.test.get_tests(ctypes.test, "test_*.py", verbosity=0)
        suites = [unittest.makeSuite(t) for t in testcases]
        run_unittest(unittest.TestSuite(suites))

if __name__ == "__main__":
    test_main()
