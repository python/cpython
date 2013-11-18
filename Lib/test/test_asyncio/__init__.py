import os
import sys
import unittest
from test.support import run_unittest, import_module

# Skip tests if we don't have threading.
import_module('threading')
# Skip tests if we don't have concurrent.futures.
import_module('concurrent.futures')


def suite():
    tests_file = os.path.join(os.path.dirname(__file__), 'tests.txt')
    with open(tests_file) as fp:
        test_names = fp.read().splitlines()
    tests = unittest.TestSuite()
    loader = unittest.TestLoader()
    for test_name in test_names:
        mod_name = 'test.' + test_name
        try:
            __import__(mod_name)
        except unittest.SkipTest:
            pass
        else:
            mod = sys.modules[mod_name]
            tests.addTests(loader.loadTestsFromModule(mod))
    return tests


def test_main():
    run_unittest(suite())
