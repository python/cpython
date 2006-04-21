"""Tests for setuptools.

The tests for setuptools are defined in the setuptools.tests package;
this runs them from there.
"""

import test.test_support
from setuptools.command.test import ScanningLoader

def test_main():
    test.test_support.run_suite(
        ScanningLoader().loadTestsFromName('setuptools.tests')
    )

if __name__ == "__main__":
    test_main()
