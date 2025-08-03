import os
import unittest

from test.support import Py_GIL_DISABLED, load_package_tests

if Py_GIL_DISABLED:
    raise unittest.SkipTest("GIL disabled")

def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
