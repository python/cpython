""" Tests for the unicodedata module.

    Written by Marc-Andre Lemburg (mal@lemburg.com).

    (c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""

from functools import partial
import hashlib
from http.client import HTTPException
import sys
import unicodedata
import unittest
from test.support import (
    open_urlresource,
    requires_resource,
    script_helper,
    cpython_only,
    check_disallow_instantiation,
    force_not_colorized,
    is_resource_enabled,
    findfile,
)


quicktest = not is_resource_enabled('cpu')

def iterallchars():
    maxunicode = 0xffff if quicktest else sys.maxunicode
    return map(chr, range(maxunicode + 1))


def check_version(testfile):
    hdr = testfile.readline()
    return unicodedata.unidata_version in hdr


def download_test_data_file(filename):
    TESTDATAURL = f"http://www.pythontest.net/unicode/{unicodedata.unidata_version}/{filename}"

    try:
        return open_urlresource(TESTDATAURL, encoding="utf-8", check=check_version)
    except PermissionError:
        raise unittest.SkipTest(
            f"Permission error when downloading {TESTDATAURL} "
            f"into the test data directory"
        )
    except (OSError, HTTPException) as exc:
        raise unittest.SkipTest(f"Failed to download {TESTDATAURL}: {exc}")


class UnicodeMethodsTest(unittest.TestCase):

    # update this, if the database changes
    expectedchecksum = ('47a99fa654ef1f50e89d2e9697b7b041fccb5a05'
                        if quicktest else
                        '8b2615a9fc627676cbc0b6fac0191177df97ef5f')

    def test_method_checksum(self):
        h = hashlib.sha1()
        for char in iterallchars():
            s1 = char + 'abc'
            s2 = char + 'ABC'
            s3 = char + '123'
            data = (
                # Predicates (single char)
                "01"[char.isalnum()],
                "01"[char.isalpha()],
                "01"[char.isdecimal()],
                "01"[char.isdigit()],
                "01"[char.islower()],
                "01"[char.isnumeric()],
                "01"[char.isspace()],
                "01"[char.istitle()],
                "01"[char.isupper()],

                # Predicates (multiple chars)
                "01"[s1.isalnum()],
                "01"[s1.isalpha()],
                "01"[s3.isdecimal()],
                "01"[s3.isdigit()],
                "01"[s1.islower()],
                "01"[s3.isnumeric()],
                "01"[(char + ' \t').isspace()],
                "01"[s1.istitle()],
                "01"[s2.isupper()],

                # Mappings (single char)
                char.lower(),
                char.upper(),
                char.title(),

                # Mappings (multiple chars)
                s1.lower(),
                s2.upper(),
                s1.title(),
                s2.title(),

                )
            h.update(''.join(data).encode('utf-8', 'surrogatepass'))
        result = h.hexdigest()
        self.assertEqual(result, self.expectedchecksum)


class BaseUnicodeFunctionsTest:

    def test_function_checksum(self):
        db = self.db
        data = []
        h = hashlib.sha1()

        for char in iterallchars():
            data = "%.12g%.12g%.12g%s%s%s%s%s%s%s" % (
                # Properties
                db.digit(char, -1),
                db.numeric(char, -1),
                db.decimal(char, -1),
                db.category(char),
                db.bidirectional(char),
                db.decomposition(char),
                db.mirrored(char),
                db.combining(char),
                db.east_asian_width(char),
                db.name(char, ""),
            )
            h.update(data.encode("ascii"))
        result = h.hexdigest()
        self.assertEqual(result, self.expectedchecksum)

    def test_name_inverse_lookup(self):
        for char in iterallchars():
            looked_name = self.db.name(char, None)
            if looked_name is not None:
                self.assertEqual(self.db.lookup(looked_name), char)

    def test_no_names_in_pua(self):
        puas = [*range(0xe000, 0xf8ff),
                *range(0xf0000, 0xfffff),
                *range(0x100000, 0x10ffff)]
        for i in puas:
            char = chr(i)
            self.assertRaises(ValueError, self.db.name, char)

    def test_lookup_nonexistant(self):
        # just make sure that lookup can fail
        for nonexistent in [
            "LATIN SMLL LETR A",
            "OPEN HANDS SIGHS",
            "DREGS",
            "HANDBUG",
            "MODIFIER LETTER CYRILLIC SMALL QUESTION MARK",
            "???",
        ]:
            self.assertRaises(KeyError, self.db.lookup, nonexistent)

    def test_digit(self):
        self.assertEqual(self.db.digit('A', None), None)
        self.assertEqual(self.db.digit('9'), 9)
        self.assertEqual(self.db.digit('\u215b', None), None)
        self.assertEqual(self.db.digit('\u2468'), 9)
        self.assertEqual(self.db.digit('\U00020000', None), None)
        self.assertEqual(self.db.digit('\U0001D7FD'), 7)

        # New in 13.0.0
        self.assertEqual(self.db.digit('\U0001fbf9', None), 9)
        # New in 14.0.0
        self.assertEqual(self.db.digit('\U00016ac9', None), 9)
        # New in 15.0.0
        self.assertEqual(self.db.digit('\U0001e4f9', None), 9)

        self.assertRaises(TypeError, self.db.digit)
        self.assertRaises(TypeError, self.db.digit, 'xx')
        self.assertRaises(ValueError, self.db.digit, 'x')

    def test_numeric(self):
        self.assertEqual(self.db.numeric('A',None), None)
        self.assertEqual(self.db.numeric('9'), 9)
        self.assertEqual(self.db.numeric('\u215b'), 0.125)
        self.assertEqual(self.db.numeric('\u2468'), 9.0)
        self.assertEqual(self.db.numeric('\U00020000', None), None)

        # New in 4.1.0
        self.assertEqual(self.db.numeric('\U0001012A', None), None if self.old else 9000)
        # Changed in 4.1.0
        self.assertEqual(self.db.numeric('\u5793', None), 1e20 if self.old else None)
        # New in 5.0.0
        self.assertEqual(self.db.numeric('\u07c0', None), None if self.old else 0.0)
        # New in 5.1.0
        self.assertEqual(self.db.numeric('\ua627', None), None if self.old else 7.0)
        # Changed in 5.2.0
        self.assertEqual(self.db.numeric('\u09f6'), 3.0 if self.old else 3/16)
        # New in 6.0.0
        self.assertEqual(self.db.numeric('\u0b72', None), None if self.old else 0.25)
        # New in 12.0.0
        self.assertEqual(self.db.numeric('\U0001ed3c', None), None if self.old else 0.5)
        # New in 13.0.0
        self.assertEqual(self.db.numeric('\U0001fbf9', None), None if self.old else 9)
        # New in 14.0.0
        self.assertEqual(self.db.numeric('\U00016ac9', None), None if self.old else 9)
        # New in 15.0.0
        self.assertEqual(self.db.numeric('\U0001e4f9', None), None if self.old else 9)

        self.assertRaises(TypeError, self.db.numeric)
        self.assertRaises(TypeError, self.db.numeric, 'xx')
        self.assertRaises(ValueError, self.db.numeric, 'x')

    def test_decimal(self):
        self.assertEqual(self.db.decimal('A',None), None)
        self.assertEqual(self.db.decimal('9'), 9)
        self.assertEqual(self.db.decimal('\u215b', None), None)
        self.assertEqual(self.db.decimal('\u2468', None), None)
        self.assertEqual(self.db.decimal('\U00020000', None), None)
        self.assertEqual(self.db.decimal('\U0001D7FD'), 7)

        # New in 4.1.0
        self.assertEqual(self.db.decimal('\xb2', None), 2 if self.old else None)
        self.assertEqual(self.db.decimal('\u1369', None), 1 if self.old else None)
        # New in 5.0.0
        self.assertEqual(self.db.decimal('\u07c0', None), None if self.old else 0)
        # New in 13.0.0
        self.assertEqual(self.db.decimal('\U0001fbf9', None), None if self.old else 9)
        # New in 14.0.0
        self.assertEqual(self.db.decimal('\U00016ac9', None), None if self.old else 9)
        # New in 15.0.0
        self.assertEqual(self.db.decimal('\U0001e4f9', None), None if self.old else 9)

        self.assertRaises(TypeError, self.db.decimal)
        self.assertRaises(TypeError, self.db.decimal, 'xx')
        self.assertRaises(ValueError, self.db.decimal, 'x')

    def test_category(self):
        self.assertEqual(self.db.category('\uFFFE'), 'Cn')
        self.assertEqual(self.db.category('a'), 'Ll')
        self.assertEqual(self.db.category('A'), 'Lu')
        self.assertEqual(self.db.category('\U00020000'), 'Lo')

        # New in 4.1.0
        self.assertEqual(self.db.category('\U0001012A'), 'Cn' if self.old else 'No')
        self.assertEqual(self.db.category('\U000e01ef'), 'Cn' if self.old else 'Mn')
        # New in 5.1.0
        self.assertEqual(self.db.category('\u0374'), 'Sk' if self.old else 'Lm')
        # Changed in 13.0.0
        self.assertEqual(self.db.category('\u0b55'), 'Cn' if self.old else 'Mn')
        self.assertEqual(self.db.category('\U0003134a'), 'Cn' if self.old else 'Lo')
        # Changed in 14.0.0
        self.assertEqual(self.db.category('\u061d'), 'Cn' if self.old else 'Po')
        self.assertEqual(self.db.category('\U0002b738'), 'Cn' if self.old else 'Lo')
        # Changed in 15.0.0
        self.assertEqual(self.db.category('\u0cf3'), 'Cn' if self.old else 'Mc')
        self.assertEqual(self.db.category('\U000323af'), 'Cn' if self.old else 'Lo')

        self.assertRaises(TypeError, self.db.category)
        self.assertRaises(TypeError, self.db.category, 'xx')

    def test_bidirectional(self):
        self.assertEqual(self.db.bidirectional('\uFFFE'), '')
        self.assertEqual(self.db.bidirectional(' '), 'WS')
        self.assertEqual(self.db.bidirectional('A'), 'L')
        self.assertEqual(self.db.bidirectional('\U00020000'), 'L')

        # New in 4.1.0
        self.assertEqual(self.db.bidirectional('+'), 'ET' if self.old else 'ES')
        self.assertEqual(self.db.bidirectional('\u0221'), '' if self.old else 'L')
        self.assertEqual(self.db.bidirectional('\U000e01ef'), '' if self.old else 'NSM')
        # New in 13.0.0
        self.assertEqual(self.db.bidirectional('\u0b55'), '' if self.old else 'NSM')
        self.assertEqual(self.db.bidirectional('\U0003134a'), '' if self.old else 'L')
        # New in 14.0.0
        self.assertEqual(self.db.bidirectional('\u061d'), '' if self.old else 'AL')
        self.assertEqual(self.db.bidirectional('\U0002b738'), '' if self.old else 'L')
        # New in 15.0.0
        self.assertEqual(self.db.bidirectional('\u0cf3'), '' if self.old else 'L')
        self.assertEqual(self.db.bidirectional('\U000323af'), '' if self.old else 'L')
        # New in 16.0.0
        self.assertEqual(self.db.bidirectional('\u0897'), '' if self.old else 'NSM')
        self.assertEqual(self.db.bidirectional('\U0001fbef'), '' if self.old else 'ON')
        # New in 17.0.0
        self.assertEqual(self.db.bidirectional('\u088f'), '' if self.old else 'AL')
        self.assertEqual(self.db.bidirectional('\U0001fbfa'), '' if self.old else 'ON')

        self.assertRaises(TypeError, self.db.bidirectional)
        self.assertRaises(TypeError, self.db.bidirectional, 'xx')

    def test_decomposition(self):
        self.assertEqual(self.db.decomposition('\uFFFE'),'')
        self.assertEqual(self.db.decomposition('\u00bc'), '<fraction> 0031 2044 0034')

        # New in 4.1.0
        self.assertEqual(self.db.decomposition('\u03f9'), '' if self.old else '<compat> 03A3')
        # New in 13.0.0
        self.assertEqual(self.db.decomposition('\uab69'), '' if self.old else '<super> 028D')
        self.assertEqual(self.db.decomposition('\U00011938'), '' if self.old else '11935 11930')
        self.assertEqual(self.db.decomposition('\U0001fbf9'), '' if self.old else '<font> 0039')
        # New in 14.0.0
        self.assertEqual(self.db.decomposition('\ua7f2'), '' if self.old else '<super> 0043')
        self.assertEqual(self.db.decomposition('\U000107ba'), '' if self.old else '<super> 1DF1E')
        # New in 15.0.0
        self.assertEqual(self.db.decomposition('\U0001e06d'), '' if self.old else '<super> 04B1')
        # New in 16.0.0
        self.assertEqual(self.db.decomposition('\U0001CCD6'), '' if self.old else '<font> 0041')
        # New in 17.0.0
        self.assertEqual(self.db.decomposition('\uA7F1'), '' if self.old else '<super> 0053')

        self.assertRaises(TypeError, self.db.decomposition)
        self.assertRaises(TypeError, self.db.decomposition, 'xx')

    def test_mirrored(self):
        self.assertEqual(self.db.mirrored('\uFFFE'), 0)
        self.assertEqual(self.db.mirrored('a'), 0)
        self.assertEqual(self.db.mirrored('\u2201'), 1)
        self.assertEqual(self.db.mirrored('\U00020000'), 0)

        # New in 5.0.0
        self.assertEqual(self.db.mirrored('\u0f3a'), 0 if self.old else 1)
        self.assertEqual(self.db.mirrored('\U0001d7c3'), 0 if self.old else 1)
        # New in 11.0.0
        self.assertEqual(self.db.mirrored('\u29a1'), 1 if self.old else 0)
        # New in 14.0.0
        self.assertEqual(self.db.mirrored('\u2e5c'), 0 if self.old else 1)
        # New in 16.0.0
        self.assertEqual(self.db.mirrored('\u226D'), 0 if self.old else 1)

        self.assertRaises(TypeError, self.db.mirrored)
        self.assertRaises(TypeError, self.db.mirrored, 'xx')

    def test_combining(self):
        self.assertEqual(self.db.combining('\uFFFE'), 0)
        self.assertEqual(self.db.combining('a'), 0)
        self.assertEqual(self.db.combining('\u20e1'), 230)
        self.assertEqual(self.db.combining('\U00020000'), 0)

        # New in 4.1.0
        self.assertEqual(self.db.combining('\u0350'), 0 if self.old else 230)
        # New in 9.0.0
        self.assertEqual(self.db.combining('\U0001e94a'), 0 if self.old else 7)
        # New in 13.0.0
        self.assertEqual(self.db.combining('\u1abf'), 0 if self.old else 220)
        self.assertEqual(self.db.combining('\U00016ff1'), 0 if self.old else 6)
        # New in 14.0.0
        self.assertEqual(self.db.combining('\u0c3c'), 0 if self.old else 7)
        self.assertEqual(self.db.combining('\U0001e2ae'), 0 if self.old else 230)
        # New in 15.0.0
        self.assertEqual(self.db.combining('\U00010efd'), 0 if self.old else 220)
        # New in 16.0.0
        self.assertEqual(self.db.combining('\u0897'), 0 if self.old else 230)
        # New in 17.0.0
        self.assertEqual(self.db.combining('\u1ACF'), 0 if self.old else 230)

        self.assertRaises(TypeError, self.db.combining)
        self.assertRaises(TypeError, self.db.combining, 'xx')

    def test_normalization(self):
        # Test normalize() and is_normalized()
        def check(ch, expected):
            if isinstance(expected, str):
                expected = [expected]*4
            forms = ('NFC', 'NFD', 'NFKC', 'NFKD')
            result = [self.db.normalize(form, ch) for form in forms]
            self.assertEqual(ascii(result), ascii(list(expected)))
            self.assertEqual([self.db.is_normalized(form, ch) for form in forms],
                             [ch == y for x, y in zip(result, expected)])

        check('', '')
        check('A', 'A')
        check(' ', ' ')
        check('\U0010ffff', '\U0010ffff')
        check('abc', 'abc')
        # Broken in 4.0.0
        check('\u0340', '\u0300')
        check('\u0300', '\u0300')
        check('\U0002fa1d', '\U0002a600')
        check('\U0002a600', '\U0002a600')
        check('\u0344', '\u0308\u0301')
        check('\u0308\u0301', '\u0308\u0301')
        # Broken in 4.0.0 and 4.0.1
        check('\U0001d1bc', '\U0001d1ba\U0001d165')
        check('\U0001d1ba\U0001d165', '\U0001d1ba\U0001d165')
        check('\ufb2c', '\u05e9\u05bc\u05c1')
        check('\u05e9\u05bc\u05c1', '\u05e9\u05bc\u05c1')
        check('\U0001d1c0', '\U0001d1ba\U0001d165\U0001d16f')
        check('\U0001d1ba\U0001d165\U0001d16f', '\U0001d1ba\U0001d165\U0001d16f')

        # Broken in 4.0.0
        check('\xa0', ['\xa0', '\xa0', ' ', ' '])
        check('\u2003', ['\u2003', '\u2003', ' ', ' '])
        check('\U0001d7ff', ['\U0001d7ff', '\U0001d7ff', '9', '9'])

        check('\xa8', ['\xa8', '\xa8', ' \u0308', ' \u0308'])
        check(' \u0308', ' \u0308')

        check('\xc0', ['\xc0', 'A\u0300']*2)
        check('A\u0300', ['\xc0', 'A\u0300']*2)

        check('\ud7a3', ['\ud7a3', '\u1112\u1175\u11c2']*2)
        check('\u1112\u1175\u11c2', ['\ud7a3', '\u1112\u1175\u11c2']*2)

        check('\xb4', ['\xb4', '\xb4', ' \u0301', ' \u0301'])
        check('\u1ffd', ['\xb4', '\xb4', ' \u0301', ' \u0301'])
        check(' \u0301', ' \u0301')

        check('\xc5', ['\xc5', 'A\u030a']*2)
        check('\u212b', ['\xc5', 'A\u030a']*2)
        check('A\u030a', ['\xc5', 'A\u030a']*2)

        check('\u1f71', ['\u03ac', '\u03b1\u0301']*2)
        check('\u03ac', ['\u03ac', '\u03b1\u0301']*2)
        check('\u03b1\u0301', ['\u03ac', '\u03b1\u0301']*2)

        check('\u01c4', ['\u01c4', '\u01c4', 'D\u017d', 'DZ\u030c'])
        check('D\u017d', ['D\u017d', 'DZ\u030c']*2)
        check('DZ\u030c', ['D\u017d', 'DZ\u030c']*2)

        check('\u1fed', ['\u1fed', '\xa8\u0300', ' \u0308\u0300', ' \u0308\u0300'])
        check('\xa8\u0300', ['\u1fed', '\xa8\u0300', ' \u0308\u0300', ' \u0308\u0300'])
        check(' \u0308\u0300', ' \u0308\u0300')

        check('\u326e', ['\u326e', '\u326e', '\uac00', '\u1100\u1161'])
        check('\u320e', ['\u320e', '\u320e', '(\uac00)', '(\u1100\u1161)'])
        check('(\uac00)', ['(\uac00)', '(\u1100\u1161)']*2)
        check('(\u1100\u1161)', ['(\uac00)', '(\u1100\u1161)']*2)

        check('\u0385', ['\u0385', '\xa8\u0301', ' \u0308\u0301', ' \u0308\u0301'])
        check('\u1fee', ['\u0385', '\xa8\u0301', ' \u0308\u0301', ' \u0308\u0301'])
        check('\xa8\u0301', ['\u0385', '\xa8\u0301', ' \u0308\u0301', ' \u0308\u0301'])
        check(' \u0308\u0301', ' \u0308\u0301')

        check('\u1fdf', ['\u1fdf', '\u1ffe\u0342', ' \u0314\u0342', ' \u0314\u0342'])
        check('\u1ffe\u0342', ['\u1fdf', '\u1ffe\u0342', ' \u0314\u0342', ' \u0314\u0342'])
        check('\u1ffe', ['\u1ffe', '\u1ffe', ' \u0314', ' \u0314'])
        check(' \u0314\u0342', ' \u0314\u0342')

        check('\u03d3', ['\u03d3', '\u03d2\u0301', '\u038e', '\u03a5\u0301'])
        check('\u03d2\u0301', ['\u03d3', '\u03d2\u0301', '\u038e', '\u03a5\u0301'])
        check('\u038e', ['\u038e', '\u03a5\u0301']*2)
        check('\u1feb', ['\u038e', '\u03a5\u0301']*2)
        check('\u03a5\u0301', ['\u038e', '\u03a5\u0301']*2)

        check('\u0626', ['\u0626', '\u064a\u0654']*2)
        check('\u064a\u0654', ['\u0626', '\u064a\u0654']*2)
        check('\ufe89', ['\ufe89', '\ufe89', '\u0626', '\u064a\u0654'])
        check('\ufe8a', ['\ufe8a', '\ufe8a', '\u0626', '\u064a\u0654'])
        check('\ufe8b', ['\ufe8b', '\ufe8b', '\u0626', '\u064a\u0654'])
        check('\ufe8c', ['\ufe8c', '\ufe8c', '\u0626', '\u064a\u0654'])

        check('\ufef9', ['\ufef9', '\ufef9', '\u0644\u0625', '\u0644\u0627\u0655'])
        check('\ufefa', ['\ufefa', '\ufefa', '\u0644\u0625', '\u0644\u0627\u0655'])
        check('\ufefb', ['\ufefb', '\ufefb', '\u0644\u0627', '\u0644\u0627'])
        check('\ufefc', ['\ufefc', '\ufefc', '\u0644\u0627', '\u0644\u0627'])
        check('\u0644\u0625', ['\u0644\u0625', '\u0644\u0627\u0655']*2)
        check('\u0644\u0627\u0655', ['\u0644\u0625', '\u0644\u0627\u0655']*2)
        check('\u0644\u0627', '\u0644\u0627')

        # Broken in 4.0.0
        check('\u327c', '\u327c' if self.old else
              ['\u327c', '\u327c', '\ucc38\uace0', '\u110e\u1161\u11b7\u1100\u1169'])
        check('\ucc38\uace0', ['\ucc38\uace0', '\u110e\u1161\u11b7\u1100\u1169']*2)
        check('\ucc38', ['\ucc38', '\u110e\u1161\u11b7']*2)
        check('\u110e\u1161\u11b7\u1100\u1169',
              ['\ucc38\uace0', '\u110e\u1161\u11b7\u1100\u1169']*2)
        check('\u110e\u1161\u11b7\u1100',
              ['\ucc38\u1100', '\u110e\u1161\u11b7\u1100']*2)
        check('\u110e\u1161\u11b7',
              ['\ucc38', '\u110e\u1161\u11b7']*2)
        check('\u110e\u1161',
              ['\ucc28', '\u110e\u1161']*2)
        check('\u110e', '\u110e')
        # Broken in 4.0.0-12.0.0
        check('\U00011938', '\U00011938' if self.old else
              ['\U00011938', '\U00011935\U00011930']*2)
        check('\U00011935\U00011930', ['\U00011938', '\U00011935\U00011930']*2)
        # New in 4.0.1
        check('\u321d', '\u321d' if self.old else
              ['\u321d', '\u321d', '(\uc624\uc804)', '(\u110b\u1169\u110c\u1165\u11ab)'])
        check('(\uc624\uc804)',
              ['(\uc624\uc804)', '(\u110b\u1169\u110c\u1165\u11ab)']*2)
        check('(\u110b\u1169\u110c\u1165\u11ab)',
              ['(\uc624\uc804)', '(\u110b\u1169\u110c\u1165\u11ab)']*2)
        check('\u4d57', '\u4d57')
        check('\u45d7', '\u45d7' if self.old else '\u45d7')
        check('\U0002f9bf', '\u4d57' if self.old else '\u45d7')
        # New in 4.1.0
        check('\u03a3', '\u03a3')
        check('\u03f9', '\u03f9' if self.old else
              ['\u03f9', '\u03f9', '\u03a3', '\u03a3'])
        # New in 5.0.0
        check('\u1b06', '\u1b06' if self.old else ['\u1b06', '\u1b05\u1b35']*2)
        # New in 5.2.0
        check('\U0001f213', '\U0001f213' if self.old else
                ['\U0001f213', '\U0001f213', '\u30c7', '\u30c6\u3099'])
        # New in 6.1.0
        check('\ufa2e', '\ufa2e' if self.old else '\u90de')
        # New in 13.0.0
        check('\U00011938', '\U00011938' if self.old else
                ['\U00011938', '\U00011935\U00011930', '\U00011938', '\U00011935\U00011930'])
        check('\U0001fbf9', '\U0001fbf9' if self.old else
                ['\U0001fbf9', '\U0001fbf9', '9', '9'])
        # New in 14.0.0
        check('\U000107ba', '\U000107ba' if self.old else
                ['\U000107ba', '\U000107ba', '\U0001df1e', '\U0001df1e'])
        # New in 15.0.0
        check('\U0001e06d', '\U0001e06d' if self.old else
                ['\U0001e06d', '\U0001e06d', '\u04b1', '\u04b1'])
        # New in 16.0.0
        check('\U0001ccd6', '\U0001ccd6' if self.old else
              ['\U0001ccd6', '\U0001ccd6', 'A', 'A'])

        self.assertRaises(TypeError, self.db.normalize)
        self.assertRaises(TypeError, self.db.normalize, 'NFC')
        self.assertRaises(ValueError, self.db.normalize, 'SPAM', 'A')

        self.assertRaises(TypeError, self.db.is_normalized)
        self.assertRaises(TypeError, self.db.is_normalized, 'NFC')
        self.assertRaises(ValueError, self.db.is_normalized, 'SPAM', 'A')

    def test_pr29(self):
        # https://www.unicode.org/review/pr-29.html
        # See issues #1054943 and #10254.
        composed = ("\u0b47\u0300\u0b3e", "\u1100\u0300\u1161",
                    'Li\u030dt-s\u1e73\u0301',
                    '\u092e\u093e\u0930\u094d\u0915 \u091c\u093c'
                    + '\u0941\u0915\u0947\u0930\u092c\u0930\u094d\u0917',
                    '\u0915\u093f\u0930\u094d\u0917\u093f\u091c\u093c'
                    + '\u0938\u094d\u0924\u093e\u0928')
        for text in composed:
            self.assertEqual(self.db.normalize('NFC', text), text)

    def test_issue10254(self):
        # Crash reported in #10254
        # New in 4.1.0
        a = 'C\u0338' * 20  + 'C\u0327'
        b = 'C\u0338' * 20  + '\xC7'
        self.assertEqual(self.db.normalize('NFC', a), b)

    def test_issue29456(self):
        # Fix #29456
        u1176_str_a = '\u1100\u1176\u11a8'
        u1176_str_b = '\u1100\u1176\u11a8'
        u11a7_str_a = '\u1100\u1175\u11a7'
        u11a7_str_b = '\uae30\u11a7'
        u11c3_str_a = '\u1100\u1175\u11c3'
        u11c3_str_b = '\uae30\u11c3'
        self.assertEqual(self.db.normalize('NFC', u1176_str_a), u1176_str_b)
        # New in 4.1.0
        self.assertEqual(self.db.normalize('NFC', u11a7_str_a), u11a7_str_b)
        self.assertEqual(self.db.normalize('NFC', u11c3_str_a), u11c3_str_b)

    def test_east_asian_width(self):
        eaw = self.db.east_asian_width
        self.assertRaises(TypeError, eaw, b'a')
        self.assertRaises(TypeError, eaw, bytearray())
        self.assertRaises(TypeError, eaw, '')
        self.assertRaises(TypeError, eaw, 'ra')
        self.assertEqual(eaw('\x1e'), 'N')
        self.assertEqual(eaw('\x20'), 'Na')
        self.assertEqual(eaw('\uC894'), 'W')
        self.assertEqual(eaw('\uFF66'), 'H')
        self.assertEqual(eaw('\uFF1F'), 'F')
        self.assertEqual(eaw('\u2010'), 'A')
        self.assertEqual(eaw('\U00020000'), 'W')

        # New in 4.1.0
        self.assertEqual(eaw('\u0350'), 'N' if self.old else 'A')
        self.assertEqual(eaw('\U000e01ef'), 'N' if self.old else 'A')
        # New in 5.2.0
        self.assertEqual(eaw('\u115a'), 'N' if self.old else 'W')
        # New in 9.0.0
        self.assertEqual(eaw('\u231a'), 'N' if self.old else 'W')
        self.assertEqual(eaw('\u2614'), 'N' if self.old else 'W')
        self.assertEqual(eaw('\U0001f19a'), 'N' if self.old else 'W')
        self.assertEqual(eaw('\U0001f991'), 'N' if self.old else 'W')
        self.assertEqual(eaw('\U0001f9c0'), 'N' if self.old else 'W')
        # New in 12.0.0
        self.assertEqual(eaw('\u32ff'), 'N' if self.old else 'W')
        self.assertEqual(eaw('\U0001fa95'), 'N' if self.old else 'W')
        # New in 13.0.0
        self.assertEqual(eaw('\u31bb'), 'N' if self.old else 'W')
        self.assertEqual(eaw('\U0003134a'), 'N' if self.old else 'W')
        # New in 14.0.0
        self.assertEqual(eaw('\u9ffd'), 'N' if self.old else 'W')
        self.assertEqual(eaw('\U0002b738'), 'N' if self.old else 'W')
        # New in 15.0.0
        self.assertEqual(eaw('\U000323af'), 'N' if self.old else 'W')
        # New in 16.0.0
        self.assertEqual(eaw('\u2630'), 'N' if self.old else 'W')
        self.assertEqual(eaw('\U0001FAE9'), 'N' if self.old else 'W')
        # New in 17.0.0
        self.assertEqual(eaw('\U00016FF2'), 'N' if self.old else 'W')

    def test_east_asian_width_unassigned(self):
        eaw = self.db.east_asian_width
        # unassigned
        for char in '\u0530\u0ecf\u10c6\u20fc\uaaca\U000107bd\U000115f2':
            self.assertEqual(eaw(char), 'N')
            self.assertIs(self.db.name(char, None), None)

        # unassigned but reserved for CJK
        for char in ('\U0002A6E0\U0002FA20\U0003134B\U0003FFFD'
                     '\uFA6E\uFADA'): # New in 5.2.0
            self.assertEqual(eaw(char), 'W')
            self.assertIs(self.db.name(char, None), None)

        # private use areas
        for char in '\uE000\uF800\U000F0000\U000FFFEE\U00100000\U0010FFF0':
            self.assertEqual(eaw(char), 'A')
            self.assertIs(self.db.name(char, None), None)

class UnicodeFunctionsTest(unittest.TestCase, BaseUnicodeFunctionsTest):
    db = unicodedata
    old = False

    # Update this if the database changes. Make sure to do a full rebuild
    # (e.g. 'make distclean && make') to get the correct checksum.
    expectedchecksum = ('83cc43a2fbb779185832b4c049217d80b05bf349'
                        if quicktest else
                        '65670ae03a324c5f9e826a4de3e25bae4d73c9b7')

    def test_isxidstart(self):
        self.assertTrue(self.db.isxidstart('S'))
        self.assertTrue(self.db.isxidstart('\u0AD0'))  # GUJARATI OM
        self.assertTrue(self.db.isxidstart('\u0EC6'))  # LAO KO LA
        self.assertTrue(self.db.isxidstart('\u17DC'))  # KHMER SIGN AVAKRAHASANYA
        self.assertTrue(self.db.isxidstart('\uA015'))  # YI SYLLABLE WU
        self.assertTrue(self.db.isxidstart('\uFE7B'))  # ARABIC KASRA MEDIAL FORM

        self.assertFalse(self.db.isxidstart(' '))
        self.assertFalse(self.db.isxidstart('0'))
        self.assertRaises(TypeError, self.db.isxidstart)
        self.assertRaises(TypeError, self.db.isxidstart, 'xx')

    def test_isxidcontinue(self):
        self.assertTrue(self.db.isxidcontinue('S'))
        self.assertTrue(self.db.isxidcontinue('_'))
        self.assertTrue(self.db.isxidcontinue('0'))
        self.assertTrue(self.db.isxidcontinue('\u00BA'))  # MASCULINE ORDINAL INDICATOR
        self.assertTrue(self.db.isxidcontinue('\u0640'))  # ARABIC TATWEEL
        self.assertTrue(self.db.isxidcontinue('\u0710'))  # SYRIAC LETTER ALAPH
        self.assertTrue(self.db.isxidcontinue('\u0B3E'))  # ORIYA VOWEL SIGN AA
        self.assertTrue(self.db.isxidcontinue('\u17D7'))  # KHMER SIGN LEK TOO

        self.assertFalse(self.db.isxidcontinue(' '))
        self.assertRaises(TypeError, self.db.isxidcontinue)
        self.assertRaises(TypeError, self.db.isxidcontinue, 'xx')

    def test_grapheme_cluster_break(self):
        gcb = self.db.grapheme_cluster_break
        self.assertEqual(gcb(' '), 'Other')
        self.assertEqual(gcb('x'), 'Other')
        self.assertEqual(gcb('\U0010FFFF'), 'Other')
        self.assertEqual(gcb('\r'), 'CR')
        self.assertEqual(gcb('\n'), 'LF')
        self.assertEqual(gcb('\0'), 'Control')
        self.assertEqual(gcb('\t'), 'Control')
        self.assertEqual(gcb('\x1F'), 'Control')
        self.assertEqual(gcb('\x7F'), 'Control')
        self.assertEqual(gcb('\x9F'), 'Control')
        self.assertEqual(gcb('\U000E0001'), 'Control')
        self.assertEqual(gcb('\u0300'), 'Extend')
        self.assertEqual(gcb('\u200C'), 'Extend')
        self.assertEqual(gcb('\U000E01EF'), 'Extend')
        self.assertEqual(gcb('\u1159'), 'L')
        self.assertEqual(gcb('\u11F9'), 'T')
        self.assertEqual(gcb('\uD788'), 'LV')
        self.assertEqual(gcb('\uD7A3'), 'LVT')
        # New in 5.0.0
        self.assertEqual(gcb('\u05BA'), 'Extend')
        self.assertEqual(gcb('\u20EF'), 'Extend')
        # New in 5.1.0
        self.assertEqual(gcb('\u2064'), 'Control')
        self.assertEqual(gcb('\uAA4D'), 'SpacingMark')
        # New in 5.2.0
        self.assertEqual(gcb('\u0816'), 'Extend')
        self.assertEqual(gcb('\uA97C'), 'L')
        self.assertEqual(gcb('\uD7C6'), 'V')
        self.assertEqual(gcb('\uD7FB'), 'T')
        # New in 6.0.0
        self.assertEqual(gcb('\u093A'), 'Extend')
        self.assertEqual(gcb('\U00011002'), 'SpacingMark')
        # New in 6.1.0
        self.assertEqual(gcb('\U000E0FFF'), 'Control')
        self.assertEqual(gcb('\U00016F7E'), 'SpacingMark')
        # New in 6.2.0
        self.assertEqual(gcb('\U0001F1E6'), 'Regional_Indicator')
        self.assertEqual(gcb('\U0001F1FF'), 'Regional_Indicator')
        # New in 6.3.0
        self.assertEqual(gcb('\u180E'), 'Control')
        self.assertEqual(gcb('\u1A1B'), 'Extend')
        # New in 7.0.0
        self.assertEqual(gcb('\u0E33'), 'SpacingMark')
        self.assertEqual(gcb('\u0EB3'), 'SpacingMark')
        self.assertEqual(gcb('\U0001BCA3'), 'Control')
        self.assertEqual(gcb('\U0001E8D6'), 'Extend')
        self.assertEqual(gcb('\U0001163E'), 'SpacingMark')
        # New in 8.0.0
        self.assertEqual(gcb('\u08E3'), 'Extend')
        self.assertEqual(gcb('\U00011726'), 'SpacingMark')
        # New in 9.0.0
        self.assertEqual(gcb('\u0600'), 'Prepend')
        self.assertEqual(gcb('\U000E007F'), 'Extend')
        self.assertEqual(gcb('\U00011CB4'), 'SpacingMark')
        self.assertEqual(gcb('\u200D'), 'ZWJ')
        # New in 10.0.0
        self.assertEqual(gcb('\U00011D46'), 'Prepend')
        self.assertEqual(gcb('\U00011D47'), 'Extend')
        self.assertEqual(gcb('\U00011A97'), 'SpacingMark')
        # New in 11.0.0
        self.assertEqual(gcb('\U000110CD'), 'Prepend')
        self.assertEqual(gcb('\u07FD'), 'Extend')
        self.assertEqual(gcb('\U00011EF6'), 'SpacingMark')
        # New in 12.0.0
        self.assertEqual(gcb('\U00011A84'), 'Prepend')
        self.assertEqual(gcb('\U00013438'), 'Control')
        self.assertEqual(gcb('\U0001E2EF'), 'Extend')
        self.assertEqual(gcb('\U00016F87'), 'SpacingMark')
        # New in 13.0.0
        self.assertEqual(gcb('\U00011941'), 'Prepend')
        self.assertEqual(gcb('\U00016FE4'), 'Extend')
        self.assertEqual(gcb('\U00011942'), 'SpacingMark')
        # New in 14.0.0
        self.assertEqual(gcb('\u0891'), 'Prepend')
        self.assertEqual(gcb('\U0001E2AE'), 'Extend')
        # New in 15.0.0
        self.assertEqual(gcb('\U00011F02'), 'Prepend')
        self.assertEqual(gcb('\U0001343F'), 'Control')
        self.assertEqual(gcb('\U0001E4EF'), 'Extend')
        self.assertEqual(gcb('\U00011F3F'), 'SpacingMark')
        # New in 16.0.0
        self.assertEqual(gcb('\U000113D1'), 'Prepend')
        self.assertEqual(gcb('\U0001E5EF'), 'Extend')
        self.assertEqual(gcb('\U0001612C'), 'SpacingMark')
        self.assertEqual(gcb('\U00016D63'), 'V')
        # New in 17.0.0
        self.assertEqual(gcb('\u1AEB'), 'Extend')
        self.assertEqual(gcb('\U00011B67'), 'SpacingMark')

        self.assertRaises(TypeError, gcb)
        self.assertRaises(TypeError, gcb, b'x')
        self.assertRaises(TypeError, gcb, 120)
        self.assertRaises(TypeError, gcb, '')
        self.assertRaises(TypeError, gcb, 'xx')

    def test_indic_conjunct_break(self):
        incb = self.db.indic_conjunct_break
        self.assertEqual(incb(' '), 'None')
        self.assertEqual(incb('x'), 'None')
        self.assertEqual(incb('\U0010FFFF'), 'None')
        # New in 15.1.0
        self.assertEqual(incb('\u094D'), 'Linker')
        self.assertEqual(incb('\u0D4D'), 'Linker')
        self.assertEqual(incb('\u0915'), 'Consonant')
        self.assertEqual(incb('\u0D3A'), 'Consonant')
        self.assertEqual(incb('\u0300'), 'Extend')
        self.assertEqual(incb('\U0001E94A'), 'Extend')
        # New in 16.0.0
        self.assertEqual(incb('\u034F'), 'Extend')
        self.assertEqual(incb('\U000E01EF'), 'Extend')
        # New in 17.0.0
        self.assertEqual(incb('\u1039'), 'Linker')
        self.assertEqual(incb('\U00011F42'), 'Linker')
        self.assertEqual(incb('\u1000'), 'Consonant')
        self.assertEqual(incb('\U00011F33'), 'Consonant')
        self.assertEqual(incb('\U0001E6F5'), 'Extend')

        self.assertRaises(TypeError, incb)
        self.assertRaises(TypeError, incb, b'x')
        self.assertRaises(TypeError, incb, 120)
        self.assertRaises(TypeError, incb, '')
        self.assertRaises(TypeError, incb, 'xx')

    def test_extended_pictographic(self):
        ext_pict = self.db.extended_pictographic
        self.assertIs(ext_pict(' '), False)
        self.assertIs(ext_pict('x'), False)
        self.assertIs(ext_pict('\U0010FFFF'), False)
        # New in 13.0.0
        self.assertIs(ext_pict('\xA9'), True)
        self.assertIs(ext_pict('\u203C'), True)
        self.assertIs(ext_pict('\U0001FAD6'), True)
        self.assertIs(ext_pict('\U0001FFFD'), True)
        # New in 17.0.0
        self.assertIs(ext_pict('\u2388'), False)
        self.assertIs(ext_pict('\U0001FA6D'), False)

        self.assertRaises(TypeError, ext_pict)
        self.assertRaises(TypeError, ext_pict, b'x')
        self.assertRaises(TypeError, ext_pict, 120)
        self.assertRaises(TypeError, ext_pict, '')
        self.assertRaises(TypeError, ext_pict, 'xx')

    def test_grapheme_break(self):
        def graphemes(*args):
            return list(map(str, self.db.iter_graphemes(*args)))

        self.assertRaises(TypeError, self.db.iter_graphemes)
        self.assertRaises(TypeError, self.db.iter_graphemes, b'x')
        self.assertRaises(TypeError, self.db.iter_graphemes, 'x', 0, 0, 0)

        self.assertEqual(graphemes(''), [])
        self.assertEqual(graphemes('abcd'), ['a', 'b', 'c', 'd'])
        self.assertEqual(graphemes('abcd', 1), ['b', 'c', 'd'])
        self.assertEqual(graphemes('abcd', 1, 3), ['b', 'c'])
        self.assertEqual(graphemes('abcd', -3), ['b', 'c', 'd'])
        self.assertEqual(graphemes('abcd', 1, -1), ['b', 'c'])
        self.assertEqual(graphemes('abcd', 3, 1), [])
        self.assertEqual(graphemes('abcd', 5), [])
        self.assertEqual(graphemes('abcd', 0, 5), ['a', 'b', 'c', 'd'])
        self.assertEqual(graphemes('abcd', -5), ['a', 'b', 'c', 'd'])
        self.assertEqual(graphemes('abcd', 0, -5), [])
        # GB3
        self.assertEqual(graphemes('\r\n'), ['\r\n'])
        # GB4
        self.assertEqual(graphemes('\r\u0308'), ['\r', '\u0308'])
        self.assertEqual(graphemes('\n\u0308'), ['\n', '\u0308'])
        self.assertEqual(graphemes('\0\u0308'), ['\0', '\u0308'])
        # GB5
        self.assertEqual(graphemes('\u06dd\r'), ['\u06dd', '\r'])
        self.assertEqual(graphemes('\u06dd\n'), ['\u06dd', '\n'])
        self.assertEqual(graphemes('\u06dd\0'), ['\u06dd', '\0'])
        # GB6
        self.assertEqual(graphemes('\u1100\u1160'), ['\u1100\u1160'])
        self.assertEqual(graphemes('\u1100\uAC00'), ['\u1100\uAC00'])
        self.assertEqual(graphemes('\u1100\uAC01'), ['\u1100\uAC01'])
        # GB7
        self.assertEqual(graphemes('\uAC00\u1160'), ['\uAC00\u1160'])
        self.assertEqual(graphemes('\uAC00\u11A8'), ['\uAC00\u11A8'])
        self.assertEqual(graphemes('\u1160\u1160'), ['\u1160\u1160'])
        self.assertEqual(graphemes('\u1160\u11A8'), ['\u1160\u11A8'])
        # GB8
        self.assertEqual(graphemes('\uAC01\u11A8'), ['\uAC01\u11A8'])
        self.assertEqual(graphemes('\u11A8\u11A8'), ['\u11A8\u11A8'])
        # GB9
        self.assertEqual(graphemes('a\u0300'), ['a\u0300'])
        self.assertEqual(graphemes('a\u200D'), ['a\u200D'])
        # GB9a
        self.assertEqual(graphemes('\u0905\u0903'), ['\u0905\u0903'])
        # GB9b
        self.assertEqual(graphemes('\u06dd\u0661'), ['\u06dd\u0661'])
        # GB9c
        self.assertEqual(graphemes('\u0915\u094d\u0924'),
                         ['\u0915\u094d\u0924'])
        self.assertEqual(graphemes('\u0915\u094D\u094D\u0924'),
                         ['\u0915\u094D\u094D\u0924'])
        self.assertEqual(graphemes('\u0915\u094D\u0924\u094D\u092F'),
                         ['\u0915\u094D\u0924\u094D\u092F'])
        # GB11
        self.assertEqual(graphemes(
                '\U0001F9D1\U0001F3FE\u200D\u2764\uFE0F'
                '\u200D\U0001F48B\u200D\U0001F9D1\U0001F3FC'),
                ['\U0001F9D1\U0001F3FE\u200D\u2764\uFE0F'
                '\u200D\U0001F48B\u200D\U0001F9D1\U0001F3FC'])
        # GB12
        self.assertEqual(graphemes(
            '\U0001F1FA\U0001F1E6\U0001F1FA\U0001F1F3'),
            ['\U0001F1FA\U0001F1E6', '\U0001F1FA\U0001F1F3'])
        # GB13
        self.assertEqual(graphemes(
            'a\U0001F1FA\U0001F1E6\U0001F1FA\U0001F1F3'),
            ['a', '\U0001F1FA\U0001F1E6', '\U0001F1FA\U0001F1F3'])


class Unicode_3_2_0_FunctionsTest(unittest.TestCase, BaseUnicodeFunctionsTest):
    db = unicodedata.ucd_3_2_0
    old = True
    expectedchecksum = ('4154d8d1232837e255edf3cdcbb5ab184d71f4a4'
                        if quicktest else
                        '3aabaf66823b21b3d305dad804a62f6f6387c93e')


class UnicodeMiscTest(unittest.TestCase):
    db = unicodedata

    @cpython_only
    def test_disallow_instantiation(self):
        # Ensure that the type disallows instantiation (bpo-43916)
        check_disallow_instantiation(self, unicodedata.UCD)

    @force_not_colorized
    def test_failed_import_during_compiling(self):
        # Issue 4367
        # Decoding \N escapes requires the unicodedata module. If it can't be
        # imported, we shouldn't segfault.

        # This program should raise a SyntaxError in the eval.
        code = "import sys;" \
            "sys.modules['unicodedata'] = None;" \
            """eval("'\\\\N{SOFT HYPHEN}'")"""
        # We use a separate process because the unicodedata module may already
        # have been loaded in this process.
        result = script_helper.assert_python_failure("-c", code)
        error = "SyntaxError: (unicode error) \\N escapes not supported " \
            "(can't load unicodedata module)"
        self.assertIn(error, result.err.decode("ascii"))

    def test_decimal_numeric_consistent(self):
        # Test that decimal and numeric are consistent,
        # i.e. if a character has a decimal value,
        # its numeric value should be the same.
        count = 0
        for c in iterallchars():
            dec = self.db.decimal(c, -1)
            if dec != -1:
                self.assertEqual(dec, self.db.numeric(c))
                count += 1
        self.assertTrue(count >= 10, count) # should have tested at least the ASCII digits

    def test_digit_numeric_consistent(self):
        # Test that digit and numeric are consistent,
        # i.e. if a character has a digit value,
        # its numeric value should be the same.
        count = 0
        for c in iterallchars():
            dec = self.db.digit(c, -1)
            if dec != -1:
                self.assertEqual(dec, self.db.numeric(c))
                count += 1
        self.assertTrue(count >= 10, count) # should have tested at least the ASCII digits

    def test_normalize_consistent(self):
        allchars = list(iterallchars())
        for form in ('NFC', 'NFD', 'NFKC', 'NFKD'):
            for c in allchars:
                norm = self.db.normalize(form, c)
                self.assertEqual(self.db.is_normalized(form, c), norm == c)
                if norm != c:
                    self.assertEqual(self.db.normalize(form, norm), norm)
                    self.assertTrue(self.db.is_normalized(form, norm))

    def test_bug_1704793(self):
        self.assertEqual(self.db.lookup("GOTHIC LETTER FAIHU"), '\U00010346')

    def test_ucd_510(self):
        # In UCD 5.1.0, a mirrored property changed wrt. UCD 3.2.0
        self.assertTrue(unicodedata.mirrored("\u0f3a"))
        self.assertTrue(not unicodedata.ucd_3_2_0.mirrored("\u0f3a"))
        # Also, we now have two ways of representing
        # the upper-case mapping: as delta, or as absolute value
        self.assertTrue("a".upper()=='A')
        self.assertTrue("\u1d79".upper()=='\ua77d')
        self.assertTrue(".".upper()=='.')

    @requires_resource('cpu')
    def test_bug_5828(self):
        self.assertEqual("\u1d79".lower(), "\u1d79")
        # Only U+0000 should have U+0000 as its upper/lower/titlecase variant
        self.assertEqual(
            [
                c for c in iterallchars()
                if "\x00" in (c.lower(), c.upper(), c.title())
            ],
            ["\x00"]
        )

    def test_bug_4971(self):
        # LETTER DZ WITH CARON: DZ, Dz, dz
        self.assertEqual("\u01c4".title(), "\u01c5")
        self.assertEqual("\u01c5".title(), "\u01c5")
        self.assertEqual("\u01c6".title(), "\u01c5")

    def test_linebreak_7643(self):
        for c in iterallchars():
            lines = (c + 'A').splitlines()
            if c in ('\x0a', '\x0b', '\x0c', '\x0d', '\x85',
                     '\x1c', '\x1d', '\x1e', '\u2028', '\u2029'):
                self.assertEqual(len(lines), 2,
                                 r"%a should be a linebreak" % c)
            else:
                self.assertEqual(len(lines), 1,
                                 r"%a should not be a linebreak" % c)

    def test_segment_object(self):
        segments = list(unicodedata.iter_graphemes('spa\u0300m'))
        self.assertEqual(len(segments), 4, segments)
        segment = segments[2]
        self.assertEqual(segment.start, 2)
        self.assertEqual(segment.end, 4)
        self.assertEqual(str(segment), 'a\u0300')
        self.assertEqual(repr(segment), '<Segment 2:4>')
        self.assertRaises(TypeError, iter, segment)
        self.assertRaises(TypeError, len, segment)


class NormalizationTest(unittest.TestCase):
    @staticmethod
    def unistr(data):
        data = [int(x, 16) for x in data.split(" ")]
        return "".join([chr(x) for x in data])

    @requires_resource('network')
    @requires_resource('cpu')
    def test_normalization(self):
        TESTDATAFILE = "NormalizationTest.txt"
        testdata = download_test_data_file(TESTDATAFILE)

        with testdata:
            self.run_normalization_tests(testdata, unicodedata)

    @requires_resource('cpu')
    def test_normalization_3_2_0(self):
        testdatafile = findfile('NormalizationTest-3.2.0.txt')
        with open(testdatafile, encoding='utf-8') as testdata:
            self.run_normalization_tests(testdata, unicodedata.ucd_3_2_0)

    def run_normalization_tests(self, testdata, ucd):
        part = None
        part1_data = set()

        NFC = partial(ucd.normalize, "NFC")
        NFKC = partial(ucd.normalize, "NFKC")
        NFD = partial(ucd.normalize, "NFD")
        NFKD = partial(ucd.normalize, "NFKD")
        is_normalized = ucd.is_normalized

        for line in testdata:
            if '#' in line:
                line = line.split('#')[0]
            line = line.strip()
            if not line:
                continue
            if line.startswith("@Part"):
                part = line.split()[0]
                continue
            c1,c2,c3,c4,c5 = [self.unistr(x) for x in line.split(';')[:-1]]

            # Perform tests
            self.assertTrue(c2 ==  NFC(c1) ==  NFC(c2) ==  NFC(c3), line)
            self.assertTrue(c4 ==  NFC(c4) ==  NFC(c5), line)
            self.assertTrue(c3 ==  NFD(c1) ==  NFD(c2) ==  NFD(c3), line)
            self.assertTrue(c5 ==  NFD(c4) ==  NFD(c5), line)
            self.assertTrue(c4 == NFKC(c1) == NFKC(c2) == \
                            NFKC(c3) == NFKC(c4) == NFKC(c5),
                            line)
            self.assertTrue(c5 == NFKD(c1) == NFKD(c2) == \
                            NFKD(c3) == NFKD(c4) == NFKD(c5),
                            line)

            self.assertTrue(is_normalized("NFC", c2))
            self.assertTrue(is_normalized("NFC", c4))

            self.assertTrue(is_normalized("NFD", c3))
            self.assertTrue(is_normalized("NFD", c5))

            self.assertTrue(is_normalized("NFKC", c4))
            self.assertTrue(is_normalized("NFKD", c5))

            # Record part 1 data
            if part == "@Part1":
                part1_data.add(c1)

        # Perform tests for all other data
        for X in iterallchars():
            if X in part1_data:
                continue
            self.assertTrue(X == NFC(X) == NFD(X) == NFKC(X) == NFKD(X), ord(X))

    def test_edge_cases(self):
        self.assertRaises(TypeError, unicodedata.normalize)
        self.assertRaises(ValueError, unicodedata.normalize, 'unknown', 'xx')
        self.assertEqual(unicodedata.normalize('NFKC', ''), '')

    def test_bug_834676(self):
        # Check for bug 834676
        unicodedata.normalize('NFC', '\ud55c\uae00')

    def test_normalize_return_type(self):
        # gh-129569: normalize() return type must always be str
        normalize = unicodedata.normalize

        class MyStr(str):
            pass

        normalization_forms = ("NFC", "NFKC", "NFD", "NFKD")
        input_strings = (
            # normalized strings
            "",
            "ascii",
            # unnormalized strings
            "\u1e0b\u0323",
            "\u0071\u0307\u0323",
        )

        for form in normalization_forms:
            for input_str in input_strings:
                with self.subTest(form=form, input_str=input_str):
                    self.assertIs(type(normalize(form, input_str)), str)
                    self.assertIs(type(normalize(form, MyStr(input_str))), str)


class GraphemeBreakTest(unittest.TestCase):
    @requires_resource('network')
    def test_grapheme_break(self):
        TESTDATAFILE = "GraphemeBreakTest.txt"
        testdata = download_test_data_file(TESTDATAFILE)

        with testdata:
            self.run_grapheme_break_tests(testdata)

    def run_grapheme_break_tests(self, testdata):
        for line in testdata:
            line, _, comment = line.partition('#')
            line = line.strip()
            if not line:
                continue
            comment = comment.strip()

            chunks = []
            breaks = []
            pos = 0
            for field in line.replace('×', ' ').split():
                if field == '÷':
                    chunks.append('')
                    breaks.append(pos)
                else:
                    chunks[-1] += chr(int(field, 16))
                    pos += 1
            self.assertEqual(chunks.pop(), '', line)
            input = ''.join(chunks)
            with self.subTest(line):
                result = list(unicodedata.iter_graphemes(input))
                self.assertEqual(list(map(str, result)), chunks, comment)
                self.assertEqual([x.start for x in result], breaks[:-1], comment)
                self.assertEqual([x.end for x in result], breaks[1:], comment)
                for i in range(1, len(breaks) - 1):
                    result = list(unicodedata.iter_graphemes(input, breaks[i]))
                    self.assertEqual(list(map(str, result)), chunks[i:], comment)
                    self.assertEqual([x.start for x in result], breaks[i:-1], comment)
                    self.assertEqual([x.end for x in result], breaks[i+1:], comment)


if __name__ == "__main__":
    unittest.main()
