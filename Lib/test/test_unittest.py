"""Test script for unittest.

This just includes tests for new features.  We really need a
full set of tests.
"""

import unittest

def test_TestSuite_iter():
    """
    >>> test1 = unittest.FunctionTestCase(lambda: None)
    >>> test2 = unittest.FunctionTestCase(lambda: None)
    >>> suite = unittest.TestSuite((test1, test2))
    >>> tests = []
    >>> for test in suite:
    ...     tests.append(test)
    >>> tests == [test1, test2]
    True
    """


######################################################################
## Main
######################################################################

def test_main():
    from test import test_support, test_unittest
    test_support.run_doctest(test_unittest, verbosity=True)

if __name__ == '__main__':
    test_main()
