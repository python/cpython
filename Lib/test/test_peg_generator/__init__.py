import os.path
import unittest
from test import support
from test.support import load_package_tests

# TODO: gh-92584: peg_generator uses distutils which was removed in Python 3.12
raise unittest.SkipTest("distutils has been removed in Python 3.12")


if support.check_sanitizer(address=True, memory=True):
    # bpo-46633: Skip the test because it is too slow when Python is built
    # with ASAN/MSAN: between 5 and 20 minutes on GitHub Actions.
    raise unittest.SkipTest("test too slow on ASAN/MSAN build")


# Load all tests in package
def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
