"""Tests for distutils.

The tests for distutils are defined in the distutils.tests package;
the test_suite() function there returns a test suite that's ready to
be run.
"""

import distutils.tests
import test.test_support


def test_main():
    test.test_support.run_unittest(distutils.tests.test_suite())


if __name__ == "__main__":
    test_main()
