"""Test correct treatment of hex/oct constants.

This is complex because of changes due to PEP 237.
"""

import sys
platform_long_is_32_bits = sys.maxint == 2147483647

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
        if platform_long_is_32_bits:
            self.assertEqual(0x7fffffff, 2147483647)
        else:
            self.assertEqual(0x7fffffffffffffff, 9223372036854775807)
        # Ditto with a minus sign and parentheses
        self.assertEqual(-(0x0), 0)
        self.assertEqual(-(0x10), -16)
        if platform_long_is_32_bits:
            self.assertEqual(-(0x7fffffff), -2147483647)
        else:
            self.assertEqual(-(0x7fffffffffffffff), -9223372036854775807)
        # Ditto with a minus sign and NO parentheses
        self.assertEqual(-0x0, 0)
        self.assertEqual(-0x10, -16)
        if platform_long_is_32_bits:
            self.assertEqual(-0x7fffffff, -2147483647)
        else:
            self.assertEqual(-0x7fffffffffffffff, -9223372036854775807)

    def test_hex_unsigned(self):
        if platform_long_is_32_bits:
            # Positive constants
            self.assertEqual(0x80000000, 2147483648L)
            self.assertEqual(0xffffffff, 4294967295L)
            # Ditto with a minus sign and parentheses
            self.assertEqual(-(0x80000000), -2147483648L)
            self.assertEqual(-(0xffffffff), -4294967295L)
            # Ditto with a minus sign and NO parentheses
            # This failed in Python 2.2 through 2.2.2 and in 2.3a1
            self.assertEqual(-0x80000000, -2147483648L)
            self.assertEqual(-0xffffffff, -4294967295L)
        else:
            # Positive constants
            self.assertEqual(0x8000000000000000, 9223372036854775808L)
            self.assertEqual(0xffffffffffffffff, 18446744073709551615L)
            # Ditto with a minus sign and parentheses
            self.assertEqual(-(0x8000000000000000), -9223372036854775808L)
            self.assertEqual(-(0xffffffffffffffff), -18446744073709551615L)
            # Ditto with a minus sign and NO parentheses
            # This failed in Python 2.2 through 2.2.2 and in 2.3a1
            self.assertEqual(-0x8000000000000000, -9223372036854775808L)
            self.assertEqual(-0xffffffffffffffff, -18446744073709551615L)

    def test_oct_baseline(self):
        # Baseline tests
        self.assertEqual(00, 0)
        self.assertEqual(020, 16)
        if platform_long_is_32_bits:
            self.assertEqual(017777777777, 2147483647)
        else:
            self.assertEqual(0777777777777777777777, 9223372036854775807)
        # Ditto with a minus sign and parentheses
        self.assertEqual(-(00), 0)
        self.assertEqual(-(020), -16)
        if platform_long_is_32_bits:
            self.assertEqual(-(017777777777), -2147483647)
        else:
            self.assertEqual(-(0777777777777777777777), -9223372036854775807)
        # Ditto with a minus sign and NO parentheses
        self.assertEqual(-00, 0)
        self.assertEqual(-020, -16)
        if platform_long_is_32_bits:
            self.assertEqual(-017777777777, -2147483647)
        else:
            self.assertEqual(-0777777777777777777777, -9223372036854775807)

    def test_oct_unsigned(self):
        if platform_long_is_32_bits:
            # Positive constants
            self.assertEqual(020000000000, 2147483648L)
            self.assertEqual(037777777777, 4294967295L)
            # Ditto with a minus sign and parentheses
            self.assertEqual(-(020000000000), -2147483648L)
            self.assertEqual(-(037777777777), -4294967295L)
            # Ditto with a minus sign and NO parentheses
            # This failed in Python 2.2 through 2.2.2 and in 2.3a1
            self.assertEqual(-020000000000, -2147483648L)
            self.assertEqual(-037777777777, -4294967295L)
        else:
            # Positive constants
            self.assertEqual(01000000000000000000000, 9223372036854775808L)
            self.assertEqual(01777777777777777777777, 18446744073709551615L)
            # Ditto with a minus sign and parentheses
            self.assertEqual(-(01000000000000000000000), -9223372036854775808L)
            self.assertEqual(-(01777777777777777777777), -18446744073709551615L)
            # Ditto with a minus sign and NO parentheses
            # This failed in Python 2.2 through 2.2.2 and in 2.3a1
            self.assertEqual(-01000000000000000000000, -9223372036854775808L)
            self.assertEqual(-01777777777777777777777, -18446744073709551615L)

def test_main():
    test_support.run_unittest(TextHexOct)

if __name__ == "__main__":
    test_main()
