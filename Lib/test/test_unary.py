"""Test compiler changes for unary ops (+, -, ~) introduced in Python 2.2"""

import unittest
from test.test_support import run_unittest, have_unicode

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

    def test_no_overflow(self):
        nines = "9" * 32
        self.assert_(eval("+" + nines) == eval("+" + nines + "L"))
        self.assert_(eval("-" + nines) == eval("-" + nines + "L"))
        self.assert_(eval("~" + nines) == eval("~" + nines + "L"))

    def test_negation_of_exponentiation(self):
        # Make sure '**' does the right thing; these form a
        # regression test for SourceForge bug #456756.
        self.assertEqual(-2 ** 3, -8)
        self.assertEqual((-2) ** 3, -8)
        self.assertEqual(-2 ** 4, -16)
        self.assertEqual((-2) ** 4, 16)

    def test_bad_types(self):
        for op in '+', '-', '~':
            self.assertRaises(TypeError, eval, op + "'a'")
            if have_unicode:
                self.assertRaises(TypeError, eval, op + "u'a'")

        self.assertRaises(TypeError, eval, "~2j")
        self.assertRaises(TypeError, eval, "~2.0")


def test_main():
    run_unittest(UnaryOpTestCase)


if __name__ == "__main__":
    test_main()
