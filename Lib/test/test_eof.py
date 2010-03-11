#! /usr/bin/env python3
"""test script for a few new invalid token catches"""

import unittest
from test import support

class EOFTestCase(unittest.TestCase):
    def test_EOFC(self):
        expect = "EOL while scanning string literal (<string>, line 1)"
        try:
            eval("""'this is a test\
            """)
        except SyntaxError as msg:
            self.assertEqual(str(msg), expect)
        else:
            raise support.TestFailed

    def test_EOFS(self):
        expect = ("EOF while scanning triple-quoted string literal "
                  "(<string>, line 1)")
        try:
            eval("""'''this is a test""")
        except SyntaxError as msg:
            self.assertEqual(str(msg), expect)
        else:
            raise support.TestFailed

def test_main():
    support.run_unittest(EOFTestCase)

if __name__ == "__main__":
    test_main()
