import os
import sys
from test.support import load_package_tests
import unittest

if sys.platform != "win32":
    try:
        import termios
    except ImportError:
        raise unittest.SkipTest("termios required")
    else:
        del termios


def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
