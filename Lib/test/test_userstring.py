#!/usr/bin/env python
# UserString is a wrapper around the native builtin string type.
# UserString instances should behave similar to builtin string objects.

import unittest
import string
from test import test_support, string_tests

from UserString import UserString, MutableString

class UserStringTest(
    string_tests.CommonTest,
    string_tests.MixinStrUnicodeUserStringTest,
    string_tests.MixinStrStringUserStringTest,
    string_tests.MixinStrUserStringTest
    ):

    type2test = UserString

    # Overwrite the three testing methods, because UserString
    # can't cope with arguments propagated to UserString
    # (and we don't test with subclasses)
    def checkequal(self, result, object, methodname, *args):
        result = self.fixtype(result)
        object = self.fixtype(object)
        # we don't fix the arguments, because UserString can't cope with it
        realresult = getattr(object, methodname)(*args)
        self.assertEqual(
            result,
            realresult
        )

    def checkraises(self, exc, object, methodname, *args):
        object = self.fixtype(object)
        # we don't fix the arguments, because UserString can't cope with it
        self.assertRaises(
            exc,
            getattr(object, methodname),
            *args
        )

    def checkcall(self, object, methodname, *args):
        object = self.fixtype(object)
        # we don't fix the arguments, because UserString can't cope with it
        getattr(object, methodname)(*args)

class MutableStringTest(UserStringTest):
    type2test = MutableString

    # MutableStrings can be hashed => deactivate test
    def test_hash(self):
        pass

    def test_setitem(self):
        s = self.type2test("foo")
        self.assertRaises(IndexError, s.__setitem__, -4, "bar")
        self.assertRaises(IndexError, s.__setitem__, 3, "bar")
        s[-1] = "bar"
        self.assertEqual(s, "fobar")
        s[0] = "bar"
        self.assertEqual(s, "barobar")

    def test_delitem(self):
        s = self.type2test("foo")
        self.assertRaises(IndexError, s.__delitem__, -4)
        self.assertRaises(IndexError, s.__delitem__, 3)
        del s[-1]
        self.assertEqual(s, "fo")
        del s[0]
        self.assertEqual(s, "o")
        del s[0]
        self.assertEqual(s, "")

    def test_setslice(self):
        s = self.type2test("foo")
        s[:] = "bar"
        self.assertEqual(s, "bar")
        s[1:2] = "foo"
        self.assertEqual(s, "bfoor")
        s[1:-1] = UserString("a")
        self.assertEqual(s, "bar")
        s[0:10] = 42
        self.assertEqual(s, "42")

    def test_delslice(self):
        s = self.type2test("foobar")
        del s[3:10]
        self.assertEqual(s, "foo")
        del s[-1:10]
        self.assertEqual(s, "fo")

    def test_extended_set_del_slice(self):
        indices = (0, None, 1, 3, 19, 100, -1, -2, -31, -100)
        orig = string.ascii_letters + string.digits
        for start in indices:
            for stop in indices:
                # Use indices[1:] when MutableString can handle real
                # extended slices
                for step in (None, 1, -1):
                    s = self.type2test(orig)
                    L = list(orig)
                    # Make sure we have a slice of exactly the right length,
                    # but with (hopefully) different data.
                    data = L[start:stop:step]
                    data.reverse()
                    L[start:stop:step] = data
                    s[start:stop:step] = "".join(data)
                    self.assertEquals(s, "".join(L))

                    del L[start:stop:step]
                    del s[start:stop:step]
                    self.assertEquals(s, "".join(L))

    def test_immutable(self):
        s = self.type2test("foobar")
        s2 = s.immutable()
        self.assertEqual(s, s2)
        self.assert_(isinstance(s2, UserString))

    def test_iadd(self):
        s = self.type2test("foo")
        s += "bar"
        self.assertEqual(s, "foobar")
        s += UserString("baz")
        self.assertEqual(s, "foobarbaz")
        s += 42
        self.assertEqual(s, "foobarbaz42")

    def test_imul(self):
        s = self.type2test("foo")
        s *= 1
        self.assertEqual(s, "foo")
        s *= 2
        self.assertEqual(s, "foofoo")
        s *= -1
        self.assertEqual(s, "")

def test_main():
    test_support.run_unittest(UserStringTest, MutableStringTest)

if __name__ == "__main__":
    test_main()
