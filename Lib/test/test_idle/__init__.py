import unittest
import os

from test import support
from test.support.import_helper import import_module

if support.check_sanitizer(address=True, memory=True):
    # See gh-90791 for details
    raise unittest.SkipTest("Tests involving libX11 can SEGFAULT on ASAN/MSAN builds")

# Skip test_idle if _tkinter, tkinter, or idlelib are missing.
tk = import_module('tkinter')  # Also imports _tkinter.
idlelib = import_module('idlelib')

# Before importing and executing more of idlelib,
# tell IDLE to avoid changing the environment.
idlelib.testing = True


def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
