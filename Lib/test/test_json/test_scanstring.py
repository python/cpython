import sys
from test.test_json import PyTest, CTest


class TestScanstring:
    def test_scanstring(self):
        scanstring = self.json.decoder.scanstring
        self.assertEqual(
            scanstring('"z\\ud834\\udd20x"', 1, True),
            ('z\U0001d120x', 16))

        self.assertEqual(
            scanstring('"z\U0001d120x"', 1, True),
            ('z\U0001d120x', 5))

        self.assertEqual(
            scanstring('"\\u007b"', 1, True),
            ('{', 8))

        self.assertEqual(
            scanstring('"A JSON payload should be an object or array, not a string."', 1, True),
            ('A JSON payload should be an object or array, not a string.', 60))

        self.assertEqual(
            scanstring('["Unclosed array"', 2, True),
            ('Unclosed array', 17))

        self.assertEqual(
            scanstring('["extra comma",]', 2, True),
            ('extra comma', 14))

        self.assertEqual(
            scanstring('["double extra comma",,]', 2, True),
            ('double extra comma', 21))

        self.assertEqual(
            scanstring('["Comma after the close"],', 2, True),
            ('Comma after the close', 24))

        self.assertEqual(
            scanstring('["Extra close"]]', 2, True),
            ('Extra close', 14))

        self.assertEqual(
            scanstring('{"Extra comma": true,}', 2, True),
            ('Extra comma', 14))

        self.assertEqual(
            scanstring('{"Extra value after close": true} "misplaced quoted value"', 2, True),
            ('Extra value after close', 26))

        self.assertEqual(
            scanstring('{"Illegal expression": 1 + 2}', 2, True),
            ('Illegal expression', 21))

        self.assertEqual(
            scanstring('{"Illegal invocation": alert()}', 2, True),
            ('Illegal invocation', 21))

        self.assertEqual(
            scanstring('{"Numbers cannot have leading zeroes": 013}', 2, True),
            ('Numbers cannot have leading zeroes', 37))

        self.assertEqual(
            scanstring('{"Numbers cannot be hex": 0x14}', 2, True),
            ('Numbers cannot be hex', 24))

        self.assertEqual(
            scanstring('[[[[[[[[[[[[[[[[[[[["Too deep"]]]]]]]]]]]]]]]]]]]]', 21, True),
            ('Too deep', 30))

        self.assertEqual(
            scanstring('{"Missing colon" null}', 2, True),
            ('Missing colon', 16))

        self.assertEqual(
            scanstring('{"Double colon":: null}', 2, True),
            ('Double colon', 15))

        self.assertEqual(
            scanstring('{"Comma instead of colon", null}', 2, True),
            ('Comma instead of colon', 25))

        self.assertEqual(
            scanstring('["Colon instead of comma": false]', 2, True),
            ('Colon instead of comma', 25))

        self.assertEqual(
            scanstring('["Bad value", truth]', 2, True),
            ('Bad value', 12))

    def test_overflow(self):
        with self.assertRaises(OverflowError):
            self.json.decoder.scanstring(b"xxx", sys.maxsize+1)


class TestPyScanstring(TestScanstring, PyTest): pass
class TestCScanstring(TestScanstring, CTest): pass
