import os
import sys
import unittest
import test.support

# used by regrtest and __main__.
def test_main():
    here = os.path.dirname(__file__)
    # Unittest mucks with the path, so we have to save and restore
    # it to keep regrtest happy.
    savepath = sys.path[:]
    test.support._run_suite(unittest.defaultTestLoader.discover(here))
    sys.path[:] = savepath
