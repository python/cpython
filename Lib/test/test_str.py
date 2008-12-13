
import unittest
import sys
from test import test_support, string_tests


class StrTest(
    string_tests.CommonTest,
    string_tests.MixinStrUnicodeUserStringTest,
    string_tests.MixinStrUserStringTest,
    string_tests.MixinStrUnicodeTest,
    ):

    type2test = str

    # We don't need to propagate to str
    def fixtype(self, obj):
        return obj

    def test_formatting(self):
        string_tests.MixinStrUnicodeUserStringTest.test_formatting(self)
        self.assertRaises(OverflowError, '%c'.__mod__, 0x1234)

    def test_expandtabs_overflows_gracefully(self):
        # This test only affects 32-bit platforms because expandtabs can only take
        # an int as the max value, not a 64-bit C long.  If expandtabs is changed
        # to take a 64-bit long, this test should apply to all platforms.
        if sys.maxint > (1 << 32):
            return
        self.assertRaises(OverflowError, 't\tt\t'.expandtabs, sys.maxint)


def test_main():
    test_support.run_unittest(StrTest)

if __name__ == "__main__":
    test_main()
