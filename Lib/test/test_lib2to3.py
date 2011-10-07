# Skipping test_parser and test_all_fixers
# because of running
from lib2to3.tests import (test_fixers, test_pytree, test_util, test_refactor,
                           test_parser,
                           test_main as test_main_)
import unittest
from test.support import run_unittest

def suite():
    tests = unittest.TestSuite()
    loader = unittest.TestLoader()
    for m in (test_fixers, test_pytree, test_util, test_refactor, test_parser,
              test_main_):
        tests.addTests(loader.loadTestsFromModule(m))
    return tests

def test_main():
    run_unittest(suite())


if __name__ == '__main__':
    test_main()
