"""Tests for distutils.

The tests for distutils are defined in the distutils.tests package.
"""

import unittest
from test import support
from test.support import warnings_helper

with warnings_helper.check_warnings(
    ("The distutils package is deprecated", DeprecationWarning), quiet=True):

    from distutils.tests import load_tests

def tearDownModule():
    support.reap_children()

if support.check_sanitizer(address=True):
    raise unittest.SkipTest("Exposes ASAN flakiness in GitHub CI")

if __name__ == "__main__":
    unittest.main()
