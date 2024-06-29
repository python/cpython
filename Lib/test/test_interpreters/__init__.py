import os
from test.support import load_package_tests, Py_GIL_DISABLED
import unittest

if Py_GIL_DISABLED:
    raise unittest.SkipTest("GIL disabled")

def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
