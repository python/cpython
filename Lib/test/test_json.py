"""Tests for json.

The tests for json are defined in the json.tests package;
the test_suite() function there returns a test suite that's ready to
be run.
"""

from test import json_tests
import test.support


def test_main():
    test.support.run_unittest(json_tests.test_suite())


if __name__ == "__main__":
    test_main()
