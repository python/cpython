"""Tests for distutils.

The tests for distutils are defined in the distutils.tests package;
the test_suite() function there returns a test suite that's ready to
be run.
"""

import unittest
from test import support
from test.support import warnings_helper

with warnings_helper.check_warnings(
    ("The distutils package is deprecated", DeprecationWarning), quiet=True):

    import distutils.tests


def load_tests(*_):
    # used by unittest
    return distutils.tests.test_suite()


def tearDownModule():
    support.reap_children()

if support.check_sanitizer(address=True):
    raise unittest.SkipTest("Exposes ASAN flakiness in GitHub CI")

if __name__ == "__main__":
    unittest.main()
