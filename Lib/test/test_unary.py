"""Test compiler changes for unary ops (+, -, ~) introduced in Python 2.2"""

import unittest
from test_support import run_unittest

class UnaryOpTestCase(unittest.TestCase):

    def test_negative(self):
        self.assert_(-2 == 0 - 2)
        self.assert_(-0 == 0)
        self.assert_(--2 == 2)
        self.assert_(-2L == 0 - 2L)
        self.assert_(-2.0 == 0 - 2.0)
        self.assert_(-2j == 0 - 2j)

    def test_positive(self):
        self.assert_(+2 == 2)
        self.assert_(+0 == 0)
        self.assert_(++2 == 2)
        self.assert_(+2L == 2L)
        self.assert_(+2.0 == 2.0)
        self.assert_(+2j == 2j)

    def test_invert(self):
        self.assert_(-2 == 0 - 2)
        self.assert_(-0 == 0)
        self.assert_(--2 == 2)
        self.assert_(-2L == 0 - 2L)

    def test_overflow(self):
        self.assertRaises(OverflowError, eval, "+" + ("9" * 32))
        self.assertRaises(OverflowError, eval, "-" + ("9" * 32))
        self.assertRaises(OverflowError, eval, "~" + ("9" * 32))

    def test_bad_types(self):
        for op in '+', '-', '~':
            self.assertRaises(TypeError, eval, op + "'a'")
            self.assertRaises(TypeError, eval, op + "u'a'")

        self.assertRaises(TypeError, eval, "~2j")
        self.assertRaises(TypeError, eval, "~2.0")

run_unittest(UnaryOpTestCase)
