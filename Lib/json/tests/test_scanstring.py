import sys
from json.tests import PyTest, CTest


class TestScanstring(object):
    def test_scanstring(self):
        scanstring = self.json.decoder.scanstring
        if sys.maxunicode == 65535:
            self.assertEqual(
                scanstring(u'"z\U0001d120x"', 1, None, True),
                (u'z\U0001d120x', 6))
        else:
            self.assertEqual(
                scanstring(u'"z\U0001d120x"', 1, None, True),
                (u'z\U0001d120x', 5))

        self.assertEqual(
            scanstring('"\\u007b"', 1, None, True),
            (u'{', 8))

        self.assertEqual(
            scanstring('"A JSON payload should be an object or array, not a string."', 1, None, True),
            (u'A JSON payload should be an object or array, not a string.', 60))

        self.assertEqual(
            scanstring('["Unclosed array"', 2, None, True),
            (u'Unclosed array', 17))

        self.assertEqual(
            scanstring('["extra comma",]', 2, None, True),
            (u'extra comma', 14))

        self.assertEqual(
            scanstring('["double extra comma",,]', 2, None, True),
            (u'double extra comma', 21))

        self.assertEqual(
            scanstring('["Comma after the close"],', 2, None, True),
            (u'Comma after the close', 24))

        self.assertEqual(
            scanstring('["Extra close"]]', 2, None, True),
            (u'Extra close', 14))

        self.assertEqual(
            scanstring('{"Extra comma": true,}', 2, None, True),
            (u'Extra comma', 14))

        self.assertEqual(
            scanstring('{"Extra value after close": true} "misplaced quoted value"', 2, None, True),
            (u'Extra value after close', 26))

        self.assertEqual(
            scanstring('{"Illegal expression": 1 + 2}', 2, None, True),
            (u'Illegal expression', 21))

        self.assertEqual(
            scanstring('{"Illegal invocation": alert()}', 2, None, True),
            (u'Illegal invocation', 21))

        self.assertEqual(
            scanstring('{"Numbers cannot have leading zeroes": 013}', 2, None, True),
            (u'Numbers cannot have leading zeroes', 37))

        self.assertEqual(
            scanstring('{"Numbers cannot be hex": 0x14}', 2, None, True),
            (u'Numbers cannot be hex', 24))

        self.assertEqual(
            scanstring('[[[[[[[[[[[[[[[[[[[["Too deep"]]]]]]]]]]]]]]]]]]]]', 21, None, True),
            (u'Too deep', 30))

        self.assertEqual(
            scanstring('{"Missing colon" null}', 2, None, True),
            (u'Missing colon', 16))

        self.assertEqual(
            scanstring('{"Double colon":: null}', 2, None, True),
            (u'Double colon', 15))

        self.assertEqual(
            scanstring('{"Comma instead of colon", null}', 2, None, True),
            (u'Comma instead of colon', 25))

        self.assertEqual(
            scanstring('["Colon instead of comma": false]', 2, None, True),
            (u'Colon instead of comma', 25))

        self.assertEqual(
            scanstring('["Bad value", truth]', 2, None, True),
            (u'Bad value', 12))

    def test_surrogates(self):
        scanstring = self.json.decoder.scanstring
        def assertScan(given, expect):
            self.assertEqual(scanstring(given, 1, None, True),
                             (expect, len(given)))
            if not isinstance(given, unicode):
                given = unicode(given)
                self.assertEqual(scanstring(given, 1, None, True),
                                 (expect, len(given)))

        surrogates = unichr(0xd834) + unichr(0xdd20)
        assertScan('"z\\ud834\\u0079x"', u'z\ud834yx')
        assertScan('"z\\ud834\\udd20x"', u'z\U0001d120x')
        assertScan('"z\\ud834\\ud834\\udd20x"', u'z\ud834\U0001d120x')
        assertScan('"z\\ud834x"', u'z\ud834x')
        assertScan(u'"z\\ud834\udd20x12345"', u'z%sx12345' % surrogates)
        assertScan('"z\\udd20x"', u'z\udd20x')
        assertScan(u'"z\ud834\udd20x"', u'z\ud834\udd20x')
        assertScan(u'"z\ud834\\udd20x"', u'z%sx' % surrogates)
        assertScan(u'"z\ud834x"', u'z\ud834x')

    def test_bad_escapes(self):
        scanstring = self.json.decoder.scanstring
        bad_escapes = [
            '"\\"',
            '"\\x"',
            '"\\u"',
            '"\\u0"',
            '"\\u01"',
            '"\\u012"',
            '"\\uz012"',
            '"\\u0z12"',
            '"\\u01z2"',
            '"\\u012z"',
            '"\\u0x12"',
            '"\\u0X12"',
            '"\\ud834\\"',
            '"\\ud834\\u"',
            '"\\ud834\\ud"',
            '"\\ud834\\udd"',
            '"\\ud834\\udd2"',
            '"\\ud834\\uzdd2"',
            '"\\ud834\\udzd2"',
            '"\\ud834\\uddz2"',
            '"\\ud834\\udd2z"',
            '"\\ud834\\u0x20"',
            '"\\ud834\\u0X20"',
        ]
        for s in bad_escapes:
            with self.assertRaises(ValueError):
                scanstring(s, 1, None, True)

    def test_issue3623(self):
        self.assertRaises(ValueError, self.json.decoder.scanstring, b"xxx", 1,
                          "xxx")
        self.assertRaises(UnicodeDecodeError,
                          self.json.encoder.encode_basestring_ascii, b"xx\xff")

    def test_overflow(self):
        with self.assertRaises(OverflowError):
            self.json.decoder.scanstring(b"xxx", sys.maxsize+1)


class TestPyScanstring(TestScanstring, PyTest): pass
class TestCScanstring(TestScanstring, CTest): pass
