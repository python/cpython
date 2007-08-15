# -*- coding: utf-8 -*-
import unittest
from test import test_support

class PEP3131Test(unittest.TestCase):

    def test_valid(self):
        class T:
            ä = 1
            µ = 2 # this is a compatibility character
            蟒 = 3
        self.assertEquals(getattr(T, "\xe4"), 1)
        self.assertEquals(getattr(T, "\u03bc"), 2)
        self.assertEquals(getattr(T, '\u87d2'), 3)

    def test_invalid(self):
        try:
            from test import badsyntax_3131
        except SyntaxError as s:
            self.assertEquals(str(s),
              "invalid character in identifier (badsyntax_3131.py, line 2)")
        else:
            self.fail("expected exception didn't occur")

def test_main():
    test_support.run_unittest(PEP3131Test)

if __name__=="__main__":
    test_main()
