
import unittest
import struct
import sys
from test import test_support, string_tests


class StrTest(
    string_tests.CommonTest,
    string_tests.MixinStrUnicodeUserStringTest,
    string_tests.MixinStrUnicodeTest,
    ):

    type2test = str8

    # We don't need to propagate to str
    def fixtype(self, obj):
        return obj

    def test_formatting(self):
        string_tests.MixinStrUnicodeUserStringTest.test_formatting(self)
        self.assertRaises(OverflowError, '%c'.__mod__, 0x12341234)

    def test_iterators(self):
        # Make sure str objects have an __iter__ method
        it = "abc".__iter__()
        self.assertEqual(next(it), "a")
        self.assertEqual(next(it), "b")
        self.assertEqual(next(it), "c")
        self.assertRaises(StopIteration, next, it)

    def test_conversion(self):
        # Make sure __str__() behaves properly

        class Foo1:
            def __str__(self):
                return "foo"

        class Foo7(str):
            def __str__(self):
                return "foos"

        class Foo8(str):
            def __new__(cls, content=""):
                return str.__new__(cls, 2*content)
            def __str__(self):
                return self

        self.assertEqual(str(Foo1()), "foo")
        self.assertEqual(str(Foo7("bar")), "foos")
        self.assertEqual(str(Foo8("foo")), "foofoo")

    def test_expandtabs_overflows_gracefully(self):
        # This test only affects 32-bit platforms because expandtabs can only take
        # an int as the max value, not a 64-bit C long.  If expandtabs is changed
        # to take a 64-bit long, this test should apply to all platforms.
        if sys.maxint > (1 << 32) or struct.calcsize('P') != 4:
            return
        self.assertRaises(OverflowError, 't\tt\t'.expandtabs, sys.maxint)


def test_main():
    test_support.run_unittest(StrTest)

if __name__ == "__main__":
    test_main()
