import os.path
import unittest
from test import support

if support.PGO:
    raise unittest.SkipTest("test is not helpful for PGO")

def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
