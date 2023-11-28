"""Tests harness for distutils.versionpredicate.

"""

import distutils.versionpredicate
import doctest

def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite(distutils.versionpredicate))
    return tests

if __name__ == '__main__':
    unittest.main()
