""" Test script for the Unicode implementation.

Written by Bill Tutt.
Modified for Python 2.0 by Fredrik Lundh (fredrik@pythonware.com)

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"

import unittest
import unicodedata
import _testcapi

from test import support
from http.client import HTTPException
from test.test_normalization import check_version

class UnicodeNamesTest(unittest.TestCase):

    def checkletter(self, name, code):
        # Helper that put all \N escapes inside eval'd raw strings,
        # to make sure this script runs even if the compiler
        # chokes on \N escapes
        res = eval(r'"\N{%s}"' % name)
        self.assertEqual(res, code)
        return res

    def test_general(self):
        # General and case insensitivity test:
        chars = [
            "LATIN CAPITAL LETTER T",
            "LATIN SMALL LETTER H",
            "LATIN SMALL LETTER E",
            "SPACE",
            "LATIN SMALL LETTER R",
            "LATIN CAPITAL LETTER E",
            "LATIN SMALL LETTER D",
            "SPACE",
            "LATIN SMALL LETTER f",
            "LATIN CAPITAL LeTtEr o",
            "LATIN SMaLl LETTER x",
            "SPACE",
            "LATIN SMALL LETTER A",
            "LATIN SMALL LETTER T",
            "LATIN SMALL LETTER E",
            "SPACE",
            "LATIN SMALL LETTER T",
            "LATIN SMALL LETTER H",
            "LATIN SMALL LETTER E",
            "SpAcE",
            "LATIN SMALL LETTER S",
            "LATIN SMALL LETTER H",
            "LATIN small LETTER e",
            "LATIN small LETTER e",
            "LATIN SMALL LETTER P",
            "FULL STOP"
        ]
        string = "The rEd fOx ate the sheep."

        self.assertEqual(
            "".join([self.checkletter(*args) for args in zip(chars, string)]),
            string
        )

    def test_ascii_letters(self):
        for char in "".join(map(chr, range(ord("a"), ord("z")))):
            name = "LATIN SMALL LETTER %s" % char.upper()
            code = unicodedata.lookup(name)
            self.assertEqual(unicodedata.name(code), name)

    def test_hangul_syllables(self):
        self.checkletter("HANGUL SYLLABLE GA", "\uac00")
        self.checkletter("HANGUL SYLLABLE GGWEOSS", "\uafe8")
        self.checkletter("HANGUL SYLLABLE DOLS", "\ub3d0")
        self.checkletter("HANGUL SYLLABLE RYAN", "\ub7b8")
        self.checkletter("HANGUL SYLLABLE MWIK", "\ubba0")
        self.checkletter("HANGUL SYLLABLE BBWAEM", "\ubf88")
        self.checkletter("HANGUL SYLLABLE SSEOL", "\uc370")
        self.checkletter("HANGUL SYLLABLE YI", "\uc758")
        self.checkletter("HANGUL SYLLABLE JJYOSS", "\ucb40")
        self.checkletter("HANGUL SYLLABLE KYEOLS", "\ucf28")
        self.checkletter("HANGUL SYLLABLE PAN", "\ud310")
        self.checkletter("HANGUL SYLLABLE HWEOK", "\ud6f8")
        self.checkletter("HANGUL SYLLABLE HIH", "\ud7a3")

        self.assertRaises(ValueError, unicodedata.name, "\ud7a4")

    def test_cjk_unified_ideographs(self):
        self.checkletter("CJK UNIFIED IDEOGRAPH-3400", "\u3400")
        self.checkletter("CJK UNIFIED IDEOGRAPH-4DB5", "\u4db5")
        self.checkletter("CJK UNIFIED IDEOGRAPH-4E00", "\u4e00")
        self.checkletter("CJK UNIFIED IDEOGRAPH-9FCB", "\u9fCB")
        self.checkletter("CJK UNIFIED IDEOGRAPH-20000", "\U00020000")
        self.checkletter("CJK UNIFIED IDEOGRAPH-2A6D6", "\U0002a6d6")
        self.checkletter("CJK UNIFIED IDEOGRAPH-2A700", "\U0002A700")
        self.checkletter("CJK UNIFIED IDEOGRAPH-2B734", "\U0002B734")
        self.checkletter("CJK UNIFIED IDEOGRAPH-2B740", "\U0002B740")
        self.checkletter("CJK UNIFIED IDEOGRAPH-2B81D", "\U0002B81D")

    def test_bmp_characters(self):
        for code in range(0x10000):
            char = chr(code)
            name = unicodedata.name(char, None)
            if name is not None:
                self.assertEqual(unicodedata.lookup(name), char)

    def test_misc_symbols(self):
        self.checkletter("PILCROW SIGN", "\u00b6")
        self.checkletter("REPLACEMENT CHARACTER", "\uFFFD")
        self.checkletter("HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK", "\uFF9F")
        self.checkletter("FULLWIDTH LATIN SMALL LETTER A", "\uFF41")

    def test_aliases(self):
        # Check that the aliases defined in the NameAliases.txt file work.
        # This should be updated when new aliases are added or the file
        # should be downloaded and parsed instead.  See #12753.
        aliases = [
            ('LATIN CAPITAL LETTER GHA', 0x01A2),
            ('LATIN SMALL LETTER GHA', 0x01A3),
            ('KANNADA LETTER LLLA', 0x0CDE),
            ('LAO LETTER FO FON', 0x0E9D),
            ('LAO LETTER FO FAY', 0x0E9F),
            ('LAO LETTER RO', 0x0EA3),
            ('LAO LETTER LO', 0x0EA5),
            ('TIBETAN MARK BKA- SHOG GI MGO RGYAN', 0x0FD0),
            ('YI SYLLABLE ITERATION MARK', 0xA015),
            ('PRESENTATION FORM FOR VERTICAL RIGHT WHITE LENTICULAR BRACKET', 0xFE18),
            ('BYZANTINE MUSICAL SYMBOL FTHORA SKLIRON CHROMA VASIS', 0x1D0C5)
        ]
        for alias, codepoint in aliases:
            self.checkletter(alias, chr(codepoint))
            name = unicodedata.name(chr(codepoint))
            self.assertNotEqual(name, alias)
            self.assertEqual(unicodedata.lookup(alias),
                             unicodedata.lookup(name))
            with self.assertRaises(KeyError):
                unicodedata.ucd_3_2_0.lookup(alias)

    def test_aliases_names_in_pua_range(self):
        # We are storing aliases in the PUA 15, but their names shouldn't leak
        for cp in range(0xf0000, 0xf0100):
            with self.assertRaises(ValueError) as cm:
                unicodedata.name(chr(cp))
            self.assertEqual(str(cm.exception), 'no such name')

    def test_named_sequences_names_in_pua_range(self):
        # We are storing named seq in the PUA 15, but their names shouldn't leak
        for cp in range(0xf0100, 0xf0fff):
            with self.assertRaises(ValueError) as cm:
                unicodedata.name(chr(cp))
            self.assertEqual(str(cm.exception), 'no such name')

    def test_named_sequences_sample(self):
        # Check a few named sequences.  See #12753.
        sequences = [
            ('LATIN SMALL LETTER R WITH TILDE', '\u0072\u0303'),
            ('TAMIL SYLLABLE SAI', '\u0BB8\u0BC8'),
            ('TAMIL SYLLABLE MOO', '\u0BAE\u0BCB'),
            ('TAMIL SYLLABLE NNOO', '\u0BA3\u0BCB'),
            ('TAMIL CONSONANT KSS', '\u0B95\u0BCD\u0BB7\u0BCD'),
        ]
        for seqname, codepoints in sequences:
            self.assertEqual(unicodedata.lookup(seqname), codepoints)
            with self.assertRaises(SyntaxError):
                self.checkletter(seqname, None)
            with self.assertRaises(KeyError):
                unicodedata.ucd_3_2_0.lookup(seqname)

    def test_named_sequences_full(self):
        # Check all the named sequences
        url = ("http://www.unicode.org/Public/%s/ucd/NamedSequences.txt" %
               unicodedata.unidata_version)
        try:
            testdata = support.open_urlresource(url, encoding="utf-8",
                                                check=check_version)
        except (OSError, HTTPException):
            self.skipTest("Could not retrieve " + url)
        self.addCleanup(testdata.close)
        for line in testdata:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            seqname, codepoints = line.split(';')
            codepoints = ''.join(chr(int(cp, 16)) for cp in codepoints.split())
            self.assertEqual(unicodedata.lookup(seqname), codepoints)
            with self.assertRaises(SyntaxError):
                self.checkletter(seqname, None)
            with self.assertRaises(KeyError):
                unicodedata.ucd_3_2_0.lookup(seqname)

    def test_errors(self):
        self.assertRaises(TypeError, unicodedata.name)
        self.assertRaises(TypeError, unicodedata.name, 'xx')
        self.assertRaises(TypeError, unicodedata.lookup)
        self.assertRaises(KeyError, unicodedata.lookup, 'unknown')

    def test_strict_error_handling(self):
        # bogus character name
        self.assertRaises(
            UnicodeError,
            str, b"\\N{blah}", 'unicode-escape', 'strict'
        )
        # long bogus character name
        self.assertRaises(
            UnicodeError,
            str, bytes("\\N{%s}" % ("x" * 100000), "ascii"), 'unicode-escape', 'strict'
        )
        # missing closing brace
        self.assertRaises(
            UnicodeError,
            str, b"\\N{SPACE", 'unicode-escape', 'strict'
        )
        # missing opening brace
        self.assertRaises(
            UnicodeError,
            str, b"\\NSPACE", 'unicode-escape', 'strict'
        )

    @unittest.skipUnless(_testcapi.INT_MAX < _testcapi.PY_SSIZE_T_MAX,
                         "needs UINT_MAX < SIZE_MAX")
    def test_issue16335(self):
        # very very long bogus character name
        try:
            x = b'\\N{SPACE' + b'x' * (_testcapi.UINT_MAX + 1) + b'}'
        except MemoryError:
            raise unittest.SkipTest("not enough memory")
        self.assertEqual(len(x), len(b'\\N{SPACE}') + (_testcapi.UINT_MAX + 1))
        self.assertRaisesRegex(UnicodeError,
            'unknown Unicode character name',
            x.decode, 'unicode-escape'
        )


def test_main():
    support.run_unittest(UnicodeNamesTest)

if __name__ == "__main__":
    test_main()
