"""Tests for distutils.

The tests for distutils are defined in the distutils.tests package;
the test_suite() function there returns a test suite that's ready to
be run.
"""

import warnings
from test import support
from test.support import warnings_helper

with warnings_helper.check_warnings(
    ("The distutils package is deprecated", DeprecationWarning), quiet=True):

    import distutils.tests


def test_main():
    # used by regrtest
    support.run_unittest(distutils.tests.test_suite())
    support.reap_children()


def load_tests(*_):
    # used by unittest
    return distutils.tests.test_suite()


if __name__ == "__main__":
    test_main()
