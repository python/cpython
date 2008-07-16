# Skipping test_parser and test_all_fixers
# because of running
from lib2to3.tests import test_fixers, test_pytree, test_util
import unittest
from test.test_support import run_unittest, requires

# Don't run lib2to3 tests by default since they take too long
if __name__ != '__main__':
    requires('lib2to3')

def suite():
    tests = unittest.TestSuite()
    loader = unittest.TestLoader()
    for m in (test_fixers,test_pytree,test_util):
        tests.addTests(loader.loadTestsFromModule(m))
    return tests

def test_main():
    run_unittest(suite())


if __name__ == '__main__':
    test_main()
