import os
from test.support import load_package_tests
import unittest


try:
    import termios
except ImportError:
    raise unittest.SkipTest("termios required")
else:
    del termios


def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
