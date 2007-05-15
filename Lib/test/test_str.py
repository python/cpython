import unittest
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

        class Foo2(object):
            def __str__(self):
                return "foo"

        class Foo3(object):
            def __str__(self):
                return "foo"

        class Foo4(str8):
            def __str__(self):
                return "foo"

        class Foo5(str):
            def __unicode__(self):
                return "foo"

        class Foo6(str8):
            def __str__(self):
                return "foos"

            def __unicode__(self):
                return "foou"

        class Foo7(str):
            def __str__(self):
                return "foos"
            def __unicode__(self):
                return "foou"

        class Foo8(str):
            def __new__(cls, content=""):
                return str.__new__(cls, 2*content)
            def __str__(self):
                return self

        class Foo9(str8):
            def __str__(self):
                return "string"
            def __unicode__(self):
                return "not unicode"

        self.assertEqual(str(Foo1()), "foo")
        self.assertEqual(str(Foo2()), "foo")
        self.assertEqual(str(Foo3()), "foo")
        self.assertEqual(str(Foo4("bar")), "foo")
        self.assertEqual(str(Foo5("bar")), "foo")
        self.assertEqual(str8(Foo6("bar")), "foos")
        self.assertEqual(str(Foo6("bar")), "foou")
        self.assertEqual(str8(Foo7("bar")), "foos")
        self.assertEqual(str(Foo7("bar")), "foou")
        self.assertEqual(str(Foo8("foo")), "foofoo")
        self.assertEqual(str8(Foo9("foo")), "string")
        self.assertEqual(str(Foo9("foo")), "not unicode")

def test_main():
    test_support.run_unittest(StrTest)

if __name__ == "__main__":
    test_main()
