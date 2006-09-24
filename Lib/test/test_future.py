# Test various flavors of legal and illegal future statements

import unittest
from test import test_support
import re

rx = re.compile('\((\S+).py, line (\d+)')

def get_error_location(msg):
    mo = rx.search(str(msg))
    return mo.group(1, 2)

class FutureTest(unittest.TestCase):

    def test_future1(self):
        test_support.unload('test_future1')
        from test import test_future1
        self.assertEqual(test_future1.result, 6)

    def test_future2(self):
        test_support.unload('test_future2')
        from test import test_future2
        self.assertEqual(test_future2.result, 6)

    def test_future3(self):
        test_support.unload('test_future3')
        from test import test_future3

    def test_badfuture3(self):
        try:
            from test import badsyntax_future3
        except SyntaxError, msg:
            self.assertEqual(get_error_location(msg), ("badsyntax_future3", '3'))
        else:
            self.fail("expected exception didn't occur")

    def test_badfuture4(self):
        try:
            from test import badsyntax_future4
        except SyntaxError, msg:
            self.assertEqual(get_error_location(msg), ("badsyntax_future4", '3'))
        else:
            self.fail("expected exception didn't occur")

    def test_badfuture5(self):
        try:
            from test import badsyntax_future5
        except SyntaxError, msg:
            self.assertEqual(get_error_location(msg), ("badsyntax_future5", '4'))
        else:
            self.fail("expected exception didn't occur")

    def test_badfuture6(self):
        try:
            from test import badsyntax_future6
        except SyntaxError, msg:
            self.assertEqual(get_error_location(msg), ("badsyntax_future6", '3'))
        else:
            self.fail("expected exception didn't occur")

    def test_badfuture7(self):
        try:
            from test import badsyntax_future7
        except SyntaxError, msg:
            self.assertEqual(get_error_location(msg), ("badsyntax_future7", '3'))
        else:
            self.fail("expected exception didn't occur")

    def test_badfuture8(self):
        try:
            from test import badsyntax_future8
        except SyntaxError, msg:
            self.assertEqual(get_error_location(msg), ("badsyntax_future8", '3'))
        else:
            self.fail("expected exception didn't occur")

    def test_badfuture9(self):
        try:
            from test import badsyntax_future9
        except SyntaxError, msg:
            self.assertEqual(get_error_location(msg), ("badsyntax_future9", '3'))
        else:
            self.fail("expected exception didn't occur")

    def test_parserhack(self):
        # test that the parser.c::future_hack function works as expected
        try:
            exec "from __future__ import division, with_statement; with = 0"
        except SyntaxError:
            pass
        else:
            self.fail("syntax error didn't occur")

        try:
            exec "from __future__ import (with_statement, division); with = 0"
        except SyntaxError:
            pass
        else:
            self.fail("syntax error didn't occur")


def test_main():
    test_support.run_unittest(FutureTest)

if __name__ == "__main__":
    test_main()
