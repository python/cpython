import os.path
import unittest
from test import support
from test.support import threading_helper


# Adjust if we ever have a platform with processes but not threads.
threading_helper.requires_working_threading(module=True)


if support.check_sanitizer(address=True, memory=True):
    # gh-90791: Skip the test because it is too slow when Python is built
    # with ASAN/MSAN: between 5 and 20 minutes on GitHub Actions.
    raise unittest.SkipTest("test too slow on ASAN/MSAN build")


def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
