"""Test suite for packaging.

This test suite consists of a collection of test modules in the
packaging.tests package.  Each test module has a name starting with
'test' and contains a function test_suite().  The function is expected
to return an initialized unittest.TestSuite instance.

Utility code is included in packaging.tests.support.

Always import unittest from this module: it will be unittest from the
standard library for packaging tests and unittest2 for distutils2 tests.
"""

import os
import sys
import unittest


def test_suite():
    suite = unittest.TestSuite()
    here = os.path.dirname(__file__) or os.curdir
    for fn in os.listdir(here):
        if fn.startswith("test") and fn.endswith(".py"):
            modname = "packaging.tests." + fn[:-3]
            __import__(modname)
            module = sys.modules[modname]
            suite.addTest(module.test_suite())
    return suite
