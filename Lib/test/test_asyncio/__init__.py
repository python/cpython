import os
import sys
import unittest
from test.support import run_unittest, import_module

# Skip tests if we don't have threading.
import_module('threading')
# Skip tests if we don't have concurrent.futures.
import_module('concurrent.futures')


def suite():
    tests = unittest.TestSuite()
    loader = unittest.TestLoader()
    for fn in os.listdir(os.path.dirname(__file__)):
        if fn.startswith("test") and fn.endswith(".py"):
            mod_name = 'test.test_asyncio.' + fn[:-3]
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
