#! /usr/bin/env python
"""test script for a few new invalid token catches"""

import os
import unittest
from test import test_support

class EOFTestCase(unittest.TestCase):
    def test_EOFC(self):
        try:
            eval("""'this is a test\
            """)
        except SyntaxError, msg:
            self.assertEqual(str(msg),
                             "EOL while scanning single-quoted string (line 1)")
        else:
            raise test_support.TestFailed

    def test_EOFS(self):
        try:
            eval("""'''this is a test""")
        except SyntaxError, msg:
            self.assertEqual(str(msg),
                             "EOF while scanning triple-quoted string (line 1)")
        else:
            raise test_support.TestFailed

def test_main():
    test_support.run_unittest(EOFTestCase)

if __name__ == "__main__":
    test_main()
