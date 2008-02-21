#!/usr/bin/env python
# UserString is a wrapper around the native builtin string type.
# UserString instances should behave similar to builtin string objects.

import unittest
import string
from test import test_support, string_tests

from collections import UserString

class UserStringTest(
    string_tests.CommonTest,
    string_tests.MixinStrUnicodeUserStringTest,
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


def test_main():
    test_support.run_unittest(UserStringTest)

if __name__ == "__main__":
    test_main()
