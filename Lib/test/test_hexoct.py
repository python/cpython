"""Test correct treatment of hex/oct constants.

This is complex because of changes due to PEP 237.

Some of these tests will have to change in Python 2.4!
"""

import unittest
from test import test_support

import warnings
warnings.filterwarnings("ignore", "hex/oct constants", FutureWarning,
                        "<string>")

class TextHexOct(unittest.TestCase):

    def test_hex_baseline(self):
        # Baseline tests
        self.assertEqual(0x0, 0)
        self.assertEqual(0x10, 16)
        self.assertEqual(0x7fffffff, 2147483647)
        # Ditto with a minus sign and parentheses
        self.assertEqual(-(0x0), 0)
        self.assertEqual(-(0x10), -16)
        self.assertEqual(-(0x7fffffff), -2147483647)
        # Ditto with a minus sign and NO parentheses
        self.assertEqual(-0x0, 0)
        self.assertEqual(-0x10, -16)
        self.assertEqual(-0x7fffffff, -2147483647)

    def test_hex_unsigned(self):
        # This test is in a <string> so we can ignore the warnings
        exec """if 1:
        # Positive-looking constants with negavive values
        self.assertEqual(0x80000000, -2147483648L)
        self.assertEqual(0xffffffff, -1)
        # Ditto with a minus sign and parentheses
        self.assertEqual(-(0x80000000), 2147483648L)
        self.assertEqual(-(0xffffffff), 1)
        # Ditto with a minus sign and NO parentheses
        # This failed in Python 2.2 through 2.2.2 and in 2.3a1
        self.assertEqual(-0x80000000, 2147483648L)
        self.assertEqual(-0xffffffff, 1)
        \n"""

    def test_oct_baseline(self):
        # Baseline tests
        self.assertEqual(00, 0)
        self.assertEqual(020, 16)
        self.assertEqual(017777777777, 2147483647)
        # Ditto with a minus sign and parentheses
        self.assertEqual(-(00), 0)
        self.assertEqual(-(020), -16)
        self.assertEqual(-(017777777777), -2147483647)
        # Ditto with a minus sign and NO parentheses
        self.assertEqual(-00, 0)
        self.assertEqual(-020, -16)
        self.assertEqual(-017777777777, -2147483647)

    def test_oct_unsigned(self):
        # This test is in a <string> so we can ignore the warnings
        exec """if 1:
        # Positive-looking constants with negavive values
        self.assertEqual(020000000000, -2147483648L)
        self.assertEqual(037777777777, -1)
        # Ditto with a minus sign and parentheses
        self.assertEqual(-(020000000000), 2147483648L)
        self.assertEqual(-(037777777777), 1)
        # Ditto with a minus sign and NO parentheses
        # This failed in Python 2.2 through 2.2.2 and in 2.3a1
        self.assertEqual(-020000000000, 2147483648L)
        self.assertEqual(-037777777777, 1)
        \n"""

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TextHexOct))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
