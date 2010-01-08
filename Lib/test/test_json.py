"""Tests for json.

The tests for json are defined in the json.tests package;
the test_suite() function there returns a test suite that's ready to
be run.
"""

import json.tests
import test.test_support


def test_main():
    test.test_support.run_unittest(json.tests.test_suite())


if __name__ == "__main__":
    test_main()
