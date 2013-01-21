""" Test script for the Unicode implementation.

Written by Bill Tutt.
Modified for Python 2.0 by Fredrik Lundh (fredrik@pythonware.com)

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"

import unittest
import _testcapi

from test import support

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
        import unicodedata

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

        import unicodedata
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
        import unicodedata
        count = 0
        for code in range(0x10000):
            char = chr(code)
            name = unicodedata.name(char, None)
            if name is not None:
                self.assertEqual(unicodedata.lookup(name), char)
                count += 1

    def test_misc_symbols(self):
        self.checkletter("PILCROW SIGN", "\u00b6")
        self.checkletter("REPLACEMENT CHARACTER", "\uFFFD")
        self.checkletter("HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK", "\uFF9F")
        self.checkletter("FULLWIDTH LATIN SMALL LETTER A", "\uFF41")

    def test_errors(self):
        import unicodedata
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
            self.assertEqual(len(x), len(b'\\N{SPACE}') +
                                     (_testcapi.UINT_MAX + 1))
            self.assertRaisesRegex(UnicodeError,
                'unknown Unicode character name',
                x.decode, 'unicode-escape'
            )
        except MemoryError:
            raise unittest.SkipTest("not enough memory")


def test_main():
    support.run_unittest(UnicodeNamesTest)

if __name__ == "__main__":
    test_main()
