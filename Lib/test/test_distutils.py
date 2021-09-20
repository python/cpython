"""Tests for distutils.

The tests for distutils are defined in the distutils.tests package;
the test_suite() function there returns a test suite that's ready to
be run.
"""

import distutils.tests
import test.support


def load_tests(*_):
    # used by unittest
    return distutils.tests.test_suite()


def tearDownModule():
    test.support.reap_children()


if __name__ == "__main__":
    unittest.main()
