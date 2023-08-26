import os.path
import sys
import unittest
from test import support

if support.PGO:
    raise unittest.SkipTest("test is not helpful for PGO")

if sys.platform == "win32":
    raise unittest.SkipTest("fork is not available on Windows")

if sys.platform == 'darwin':
    raise unittest.SkipTest("test may crash on macOS (bpo-33725)")

def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
