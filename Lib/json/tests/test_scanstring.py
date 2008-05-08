import sys
import decimal
from unittest import TestCase

import json.decoder

class TestScanString(TestCase):
    def test_py_scanstring(self):
        self._test_scanstring(json.decoder.py_scanstring)

    def test_c_scanstring(self):
        if json.decoder.c_scanstring is not None:
            self._test_scanstring(json.decoder.c_scanstring)

    def _test_scanstring(self, scanstring):
        self.assertEquals(
            scanstring('"z\\ud834\\udd20x"', 1, None, True),
            ('z\U0001d120x', 16))

        if sys.maxunicode == 65535:
            self.assertEquals(
                scanstring('"z\U0001d120x"', 1, None, True),
                ('z\U0001d120x', 6))
        else:
            self.assertEquals(
                scanstring('"z\U0001d120x"', 1, None, True),
                ('z\U0001d120x', 5))

        self.assertEquals(
            scanstring('"\\u007b"', 1, None, True),
            ('{', 8))

        self.assertEquals(
            scanstring('"A JSON payload should be an object or array, not a string."', 1, None, True),
            ('A JSON payload should be an object or array, not a string.', 60))

        self.assertEquals(
            scanstring('["Unclosed array"', 2, None, True),
            ('Unclosed array', 17))

        self.assertEquals(
            scanstring('["extra comma",]', 2, None, True),
            ('extra comma', 14))

        self.assertEquals(
            scanstring('["double extra comma",,]', 2, None, True),
            ('double extra comma', 21))

        self.assertEquals(
            scanstring('["Comma after the close"],', 2, None, True),
            ('Comma after the close', 24))

        self.assertEquals(
            scanstring('["Extra close"]]', 2, None, True),
            ('Extra close', 14))

        self.assertEquals(
            scanstring('{"Extra comma": true,}', 2, None, True),
            ('Extra comma', 14))

        self.assertEquals(
            scanstring('{"Extra value after close": true} "misplaced quoted value"', 2, None, True),
            ('Extra value after close', 26))

        self.assertEquals(
            scanstring('{"Illegal expression": 1 + 2}', 2, None, True),
            ('Illegal expression', 21))

        self.assertEquals(
            scanstring('{"Illegal invocation": alert()}', 2, None, True),
            ('Illegal invocation', 21))

        self.assertEquals(
            scanstring('{"Numbers cannot have leading zeroes": 013}', 2, None, True),
            ('Numbers cannot have leading zeroes', 37))

        self.assertEquals(
            scanstring('{"Numbers cannot be hex": 0x14}', 2, None, True),
            ('Numbers cannot be hex', 24))

        self.assertEquals(
            scanstring('[[[[[[[[[[[[[[[[[[[["Too deep"]]]]]]]]]]]]]]]]]]]]', 21, None, True),
            ('Too deep', 30))

        self.assertEquals(
            scanstring('{"Missing colon" null}', 2, None, True),
            ('Missing colon', 16))

        self.assertEquals(
            scanstring('{"Double colon":: null}', 2, None, True),
            ('Double colon', 15))

        self.assertEquals(
            scanstring('{"Comma instead of colon", null}', 2, None, True),
            ('Comma instead of colon', 25))

        self.assertEquals(
            scanstring('["Colon instead of comma": false]', 2, None, True),
            ('Colon instead of comma', 25))

        self.assertEquals(
            scanstring('["Bad value", truth]', 2, None, True),
            ('Bad value', 12))
