#!/usr/bin/env python
# UserString is a wrapper around the native builtin string type.
# UserString instances should behave similar to builtin string objects.

import unittest
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

def test_main():
    test_support.run_unittest(UserStringTest, MutableStringTest)

if __name__ == "__main__":
    test_main()
