import os
import unittest
from test.support import load_package_tests
from test.support import TEST_MODULES


if TEST_MODULES != "yes":
    raise unittest.SkipTest("requires test modules")


def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
