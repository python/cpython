
import unittest
import struct
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

    def test_conversion(self):
        # Make sure __str__() behaves properly
        class Foo0:
            def __unicode__(self):
                return u"foo"

        class Foo1:
            def __str__(self):
                return "foo"

        class Foo2(object):
            def __str__(self):
                return "foo"

        class Foo3(object):
            def __str__(self):
                return u"foo"

        class Foo4(unicode):
            def __str__(self):
                return u"foo"

        class Foo5(str):
            def __str__(self):
                return u"foo"

        class Foo6(str):
            def __str__(self):
                return "foos"

            def __unicode__(self):
                return u"foou"

        class Foo7(unicode):
            def __str__(self):
                return "foos"
            def __unicode__(self):
                return u"foou"

        class Foo8(str):
            def __new__(cls, content=""):
                return str.__new__(cls, 2*content)
            def __str__(self):
                return self

        class Foo9(str):
            def __str__(self):
                return "string"
            def __unicode__(self):
                return "not unicode"

        self.assert_(str(Foo0()).startswith("<")) # this is different from __unicode__
        self.assertEqual(str(Foo1()), "foo")
        self.assertEqual(str(Foo2()), "foo")
        self.assertEqual(str(Foo3()), "foo")
        self.assertEqual(str(Foo4("bar")), "foo")
        self.assertEqual(str(Foo5("bar")), "foo")
        self.assertEqual(str(Foo6("bar")), "foos")
        self.assertEqual(str(Foo7("bar")), "foos")
        self.assertEqual(str(Foo8("foo")), "foofoo")
        self.assertEqual(str(Foo9("foo")), "string")
        self.assertEqual(unicode(Foo9("foo")), u"not unicode")

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
