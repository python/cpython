"""Tests for distutils.

The tests for distutils are defined in the distutils.tests package;
the test_suite() function there returns a test suite that's ready to
be run.
"""

from test import test_support
import distutils.tests


def test_main():
    test_support.run_unittest(distutils.tests.test_suite())
    test_support.reap_children()


if __name__ == "__main__":
    test_main()
