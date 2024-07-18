"""Test cases for the fnmatch module."""

import itertools
import os
import string
import unittest
import warnings

import test.support.import_helper

c_fnmatch = test.support.import_helper.import_fresh_module("_fnmatch")
py_fnmatch = test.support.import_helper.import_fresh_module("fnmatch", blocked=["_fnmatch"])

class FnmatchTestCaseMixin:
    fnmatch = None

    def check_match(self, filename, pattern, should_match=True, func=None):
        if func is None:
            func = self.fnmatch.fnmatch

        with self.subTest(fn=func, name=filename, pattern=pattern):
            res = func(filename, pattern)
            if should_match:
                self.assertTrue(res, f"expected {filename!r} to match pattern {pattern!r}")
            else:
                self.assertFalse(res, f"expected {filename!r} not to match pattern {pattern!r}")

    def test_fnmatch(self):
        check = self.check_match
        check('abc', 'abc')
        check('abc', '?*?')
        check('abc', '???*')
        check('abc', '*???')
        check('abc', '???')
        check('abc', '*')
        check('abc', 'ab[cd]')
        check('abc', 'ab[!de]')
        check('abc', 'ab[de]', False)
        check('a', '??', False)
        check('a', 'b', False)

        # these test that '\' is handled correctly in character sets;
        # see SF bug #409651
        check('\\', r'[\]')
        check('a', r'[!\]')
        check('\\', r'[!\]', False)

        # test that filenames with newlines in them are handled correctly.
        # http://bugs.python.org/issue6665
        check('foo\nbar', 'foo*')
        check('foo\nbar\n', 'foo*')
        check('\nfoo', 'foo*', False)
        check('\n', '*')

    def test_slow_fnmatch(self):
        check = self.check_match
        check('a' * 50, '*a*a*a*a*a*a*a*a*a*a')
        # The next "takes forever" if the regexp translation is
        # straightforward.  See bpo-40480.
        check('a' * 50 + 'b', '*a*a*a*a*a*a*a*a*a*a', False)

    def test_mix_bytes_str(self):
        fnmatch = self.fnmatch.fnmatch
        self.assertRaises(TypeError, fnmatch, 'test', b'*')
        self.assertRaises(TypeError, fnmatch, b'test', '*')

        fnmatchcase = self.fnmatch.fnmatchcase
        self.assertRaises(TypeError, fnmatchcase, 'test', b'*')
        self.assertRaises(TypeError, fnmatchcase, b'test', '*')

    def test_fnmatchcase(self):
        check = self.check_match
        fnmatchcase = self.fnmatch.fnmatchcase
        check('abc', 'abc', True, fnmatchcase)
        check('AbC', 'abc', False, fnmatchcase)
        check('abc', 'AbC', False, fnmatchcase)
        check('AbC', 'AbC', True, fnmatchcase)

        check('usr/bin', 'usr/bin', True, fnmatchcase)
        check('usr\\bin', 'usr/bin', False, fnmatchcase)
        check('usr/bin', 'usr\\bin', False, fnmatchcase)
        check('usr\\bin', 'usr\\bin', True, fnmatchcase)

    def test_bytes(self):
        self.check_match(b'test', b'te*')
        self.check_match(b'test\xff', b'te*\xff')
        self.check_match(b'foo\nbar', b'foo*')

    def test_case(self):
        ignorecase = os.path.normcase('ABC') == os.path.normcase('abc')
        check = self.check_match
        check('abc', 'abc')
        check('AbC', 'abc', ignorecase)
        check('abc', 'AbC', ignorecase)
        check('AbC', 'AbC')

    def test_sep(self):
        normsep = os.path.normcase('\\') == os.path.normcase('/')
        check = self.check_match
        check('usr/bin', 'usr/bin')
        check('usr\\bin', 'usr/bin', normsep)
        check('usr/bin', 'usr\\bin', normsep)
        check('usr\\bin', 'usr\\bin')

    def test_char_set(self):
        ignorecase = os.path.normcase('ABC') == os.path.normcase('abc')
        check = self.check_match
        tescases = string.ascii_lowercase + string.digits + string.punctuation
        for c in tescases:
            check(c, '[az]', c in 'az')
            check(c, '[!az]', c not in 'az')
        # Case insensitive.
        for c in tescases:
            check(c, '[AZ]', (c in 'az') and ignorecase)
            check(c, '[!AZ]', (c not in 'az') or not ignorecase)
        for c in string.ascii_uppercase:
            check(c, '[az]', (c in 'AZ') and ignorecase)
            check(c, '[!az]', (c not in 'AZ') or not ignorecase)
        # Repeated same character.
        for c in tescases:
            check(c, '[aa]', c == 'a')
        # Special cases.
        for c in tescases:
            check(c, '[^az]', c in '^az')
            check(c, '[[az]', c in '[az')
            check(c, r'[!]]', c != ']')
        check('[', '[')
        check('[]', '[]')
        check('[!', '[!')
        check('[!]', '[!]')

    def test_range(self):
        ignorecase = os.path.normcase('ABC') == os.path.normcase('abc')
        normsep = os.path.normcase('\\') == os.path.normcase('/')
        check = self.check_match
        tescases = string.ascii_lowercase + string.digits + string.punctuation
        for c in tescases:
            check(c, '[b-d]', c in 'bcd')
            check(c, '[!b-d]', c not in 'bcd')
            check(c, '[b-dx-z]', c in 'bcdxyz')
            check(c, '[!b-dx-z]', c not in 'bcdxyz')
        # Case insensitive.
        for c in tescases:
            check(c, '[B-D]', (c in 'bcd') and ignorecase)
            check(c, '[!B-D]', (c not in 'bcd') or not ignorecase)
        for c in string.ascii_uppercase:
            check(c, '[b-d]', (c in 'BCD') and ignorecase)
            check(c, '[!b-d]', (c not in 'BCD') or not ignorecase)
        # Upper bound == lower bound.
        for c in tescases:
            check(c, '[b-b]', c == 'b')
        # Special cases.
        for c in tescases:
            check(c, '[!-#]', c not in '-#')
            check(c, '[!--.]', c not in '-.')
            check(c, '[^-`]', c in '^_`')
            if not (normsep and c == '/'):
                check(c, '[[-^]', c in r'[\]^')
                check(c, r'[\-^]', c in r'\]^')
            check(c, '[b-]', c in '-b')
            check(c, '[!b-]', c not in '-b')
            check(c, '[-b]', c in '-b')
            check(c, '[!-b]', c not in '-b')
            check(c, '[-]', c in '-')
            check(c, '[!-]', c not in '-')
        # Upper bound is less that lower bound: error in RE.
        for c in tescases:
            check(c, '[d-b]', False)
            check(c, '[!d-b]', True)
            check(c, '[d-bx-z]', c in 'xyz')
            check(c, '[!d-bx-z]', c not in 'xyz')
            check(c, '[d-b^-`]', c in '^_`')
            if not (normsep and c == '/'):
                check(c, '[d-b[-^]', c in r'[\]^')

    def test_sep_in_char_set(self):
        normsep = os.path.normcase('\\') == os.path.normcase('/')
        check = self.check_match
        check('/', r'[/]')
        check('\\', r'[\]')
        check('/', r'[\]', normsep)
        check('\\', r'[/]', normsep)
        check('[/]', r'[/]', False)
        check(r'[\\]', r'[/]', False)
        check('\\', r'[\t]')
        check('/', r'[\t]', normsep)
        check('t', r'[\t]')
        check('\t', r'[\t]', False)

    def test_sep_in_range(self):
        normsep = os.path.normcase('\\') == os.path.normcase('/')
        check = self.check_match
        check('a/b', 'a[.-0]b', not normsep)
        check('a\\b', 'a[.-0]b', False)
        check('a\\b', 'a[Z-^]b', not normsep)
        check('a/b', 'a[Z-^]b', False)

        check('a/b', 'a[/-0]b', not normsep)
        check(r'a\b', 'a[/-0]b', False)
        check('a[/-0]b', 'a[/-0]b', False)
        check(r'a[\-0]b', 'a[/-0]b', False)

        check('a/b', 'a[.-/]b')
        check(r'a\b', 'a[.-/]b', normsep)
        check('a[.-/]b', 'a[.-/]b', False)
        check(r'a[.-\]b', 'a[.-/]b', False)

        check(r'a\b', r'a[\-^]b')
        check('a/b', r'a[\-^]b', normsep)
        check(r'a[\-^]b', r'a[\-^]b', False)
        check('a[/-^]b', r'a[\-^]b', False)

        check(r'a\b', r'a[Z-\]b', not normsep)
        check('a/b', r'a[Z-\]b', False)
        check(r'a[Z-\]b', r'a[Z-\]b', False)
        check('a[Z-/]b', r'a[Z-\]b', False)

    def test_warnings(self):
        with warnings.catch_warnings():
            warnings.simplefilter('error', Warning)
            check = self.check_match
            check('[', '[[]')
            check('&', '[a&&b]')
            check('|', '[a||b]')
            check('~', '[a~~b]')
            check(',', '[a-z+--A-Z]')
            check('.', '[a-z--/A-Z]')

class PurePythonFnmatchTestCase(FnmatchTestCaseMixin, unittest.TestCase):
    fnmatch = py_fnmatch

class CPythonFnmatchTestCase(FnmatchTestCaseMixin, unittest.TestCase):
    fnmatch = c_fnmatch

class TranslateTestCaseMixin:
    fnmatch = None

    def test_translate(self):
        import re
        translate = self.fnmatch.translate
        self.assertEqual(translate('*'), r'(?s:.*)\Z')
        self.assertEqual(translate('?'), r'(?s:.)\Z')
        self.assertEqual(translate('a?b*'), r'(?s:a.b.*)\Z')
        self.assertEqual(translate('[abc]'), r'(?s:[abc])\Z')
        self.assertEqual(translate('[]]'), r'(?s:[]])\Z')
        self.assertEqual(translate('[!x]'), r'(?s:[^x])\Z')
        self.assertEqual(translate('[^x]'), r'(?s:[\^x])\Z')
        self.assertEqual(translate('[x'), r'(?s:\[x)\Z')
        # from the docs
        self.assertEqual(translate('*.txt'), r'(?s:.*\.txt)\Z')
        # squash consecutive stars
        self.assertEqual(translate('*********'), r'(?s:.*)\Z')
        self.assertEqual(translate('A*********'), r'(?s:A.*)\Z')
        self.assertEqual(translate('*********A'), r'(?s:.*A)\Z')
        self.assertEqual(translate('A*********?[?]?'), r'(?s:A.*.[?].)\Z')
        # fancy translation to prevent exponential-time match failure
        t = translate('**a*a****a')
        self.assertEqual(t, r'(?s:(?>.*?a)(?>.*?a).*a)\Z')
        # and try pasting multiple translate results - it's an undocumented
        # feature that this works
        r1 = translate('**a**a**a*')
        r2 = translate('**b**b**b*')
        r3 = translate('*c*c*c*')
        fatre = "|".join([r1, r2, r3])
        self.assertTrue(re.match(fatre, 'abaccad'))
        self.assertTrue(re.match(fatre, 'abxbcab'))
        self.assertTrue(re.match(fatre, 'cbabcaxc'))
        self.assertFalse(re.match(fatre, 'dabccbad'))

    def test_translate_wildcards(self):
        for pattern, expect in [
            ('ab*', r'(?s:ab.*)\Z'),
            ('ab*cd', r'(?s:ab.*cd)\Z'),
            ('ab*cd*', r'(?s:ab(?>.*?cd).*)\Z'),
            ('ab*cd*12', r'(?s:ab(?>.*?cd).*12)\Z'),
            ('ab*cd*12*', r'(?s:ab(?>.*?cd)(?>.*?12).*)\Z'),
            ('ab*cd*12*34', r'(?s:ab(?>.*?cd)(?>.*?12).*34)\Z'),
            ('ab*cd*12*34*', r'(?s:ab(?>.*?cd)(?>.*?12)(?>.*?34).*)\Z'),
        ]:
            translated = self.fnmatch.translate(pattern)
            self.assertEqual(translated, expect, pattern)

        for pattern, expect in [
            ('*ab', r'(?s:.*ab)\Z'),
            ('*ab*', r'(?s:(?>.*?ab).*)\Z'),
            ('*ab*cd', r'(?s:(?>.*?ab).*cd)\Z'),
            ('*ab*cd*', r'(?s:(?>.*?ab)(?>.*?cd).*)\Z'),
            ('*ab*cd*12', r'(?s:(?>.*?ab)(?>.*?cd).*12)\Z'),
            ('*ab*cd*12*', r'(?s:(?>.*?ab)(?>.*?cd)(?>.*?12).*)\Z'),
            ('*ab*cd*12*34', r'(?s:(?>.*?ab)(?>.*?cd)(?>.*?12).*34)\Z'),
            ('*ab*cd*12*34*', r'(?s:(?>.*?ab)(?>.*?cd)(?>.*?12)(?>.*?34).*)\Z'),
        ]:
            translated = self.fnmatch.translate(pattern)
            self.assertEqual(translated, expect, pattern)

    def test_translate_expressions(self):
        '[', '[-abc]', '[[]b', '[[a]b', '[\\\\]', '[\\]', '[]-]', '[][!]',
        '[]]b', '[]a[]b', '[^a-c]*', '[a-\\z]',
        '[a-c]b*', '[a-y]*[^c]', '[abc-]', '\\*',
        '[0-4-3-2]', '[b-ac-z9-1]', '[!b-ac-z9-1]', '[!]b-ac-z9-1]',
        '[]b-ac-z9-1]', '[]b-ac-z9-1]*', '*[]b-ac-z9-1]',
        for pattern, expect in [
            ('[', r'(?s:\[)\Z'),
            ('[!', r'(?s:\[!)\Z'),
            ('[]', r'(?s:\[\])\Z'),
            ('[abc', r'(?s:\[abc)\Z'),
            ('[!abc', r'(?s:\[!abc)\Z'),
            ('[abc]', r'(?s:[abc])\Z'),
            ('[!abc]', r'(?s:[^abc])\Z'),
            # with [[
            ('[[', r'(?s:\[\[)\Z'),
            ('[[a', r'(?s:\[\[a)\Z'),
            ('[[]', r'(?s:[\[])\Z'),
            ('[[]a', r'(?s:[\[]a)\Z'),
            ('[[]]', r'(?s:[\[]\])\Z'),
            ('[[]a]', r'(?s:[\[]a\])\Z'),
            ('[[a]', r'(?s:[\[a])\Z'),
            ('[[a]]', r'(?s:[\[a]\])\Z'),
            ('[[a]b', r'(?s:[\[a]b)\Z'),
            # backslashes
            ('[\\', r'(?s:\[\\)\Z'),
            (r'[\]', r'(?s:[\\])\Z'),
            (r'[\\]', r'(?s:[\\\\])\Z'),
        ]:
            translated = self.fnmatch.translate(pattern)
            self.assertEqual(translated, expect, pattern)

class PurePythonTranslateTestCase(TranslateTestCaseMixin, unittest.TestCase):
    fnmatch = py_fnmatch

class CPythonTranslateTestCase(TranslateTestCaseMixin, unittest.TestCase):
    fnmatch = c_fnmatch

    @staticmethod
    def translate_func(pattern):
        # Pure Python implementation of translate()
        STAR = object()
        parts = py_fnmatch._translate(pattern, STAR, '.')
        return py_fnmatch._join_translated_parts(parts, STAR)

    def test_translate(self):
        # We want to check that the C implementation is EXACTLY the same
        # as the Python implementation. For that, we will need to cover
        # a lot of cases.
        translate = self.fnmatch.translate

        for choice in itertools.combinations_with_replacement('*?.', 5):
            for suffix in ['', '!']:
                pat = suffix + ''.join(choice)
                with self.subTest(pattern=pat):
                    self.assertEqual(translate(pat), self.translate_func(pat))

        for pat in [
            '',
            '!!a*', '!\\!a*', '!a*', '*', '**', '*******?', '*******c', '*****??', '**/',
            '*.js', '*/man*/bash.*', '*???', '?', '?*****??', '?*****?c', '?***?****',
            '?***?****?', '?***?****c', '?*?', '??', '???', '???*', '[!\\]',
            '\\**', '\\*\\*', 'a*', 'a*****?c', 'a****c**?**??*****', 'a***c',
            'a**?**cd**?**??***k', 'a**?**cd**?**??***k**', 'a**?**cd**?**??k',
            'a**?**cd**?**??k***', 'a*[^c]',
            'a*cd**?**??k', 'a/*', 'a/**', 'a/**/b',
            'a/**/b/**/c', 'a/.*/c', 'a/?', 'a/??', 'a[X-]b', 'a[\\.]c',
            'a[\\b]c', 'a[bc', 'a\\*?/*', 'a\\*b/*',
            'ab[!de]', 'ab[cd]', 'ab[cd]ef', 'abc', 'b*/', 'foo*',
            'man/man1/bash.1'
        ]:
            with self.subTest(pattern=pat):
                self.assertEqual(translate(pat), self.translate_func(pat))

class FilterTestCaseMixin:
    fnmatch = None

    def test_filter(self):
        filter = self.fnmatch.filter
        self.assertEqual(filter(['Python', 'Ruby', 'Perl', 'Tcl'], 'P*'),
                         ['Python', 'Perl'])
        self.assertEqual(filter([b'Python', b'Ruby', b'Perl', b'Tcl'], b'P*'),
                         [b'Python', b'Perl'])

    def test_case(self):
        ignorecase = os.path.normcase('P') == os.path.normcase('p')
        filter = self.fnmatch.filter
        self.assertEqual(filter(['Test.py', 'Test.rb', 'Test.PL'], '*.p*'),
                         ['Test.py', 'Test.PL'] if ignorecase else ['Test.py'])
        self.assertEqual(filter(['Test.py', 'Test.rb', 'Test.PL'], '*.P*'),
                         ['Test.py', 'Test.PL'] if ignorecase else ['Test.PL'])

    def test_sep(self):
        filter = self.fnmatch.filter
        normsep = os.path.normcase('\\') == os.path.normcase('/')
        self.assertEqual(filter(['usr/bin', 'usr', 'usr\\lib'], 'usr/*'),
                         ['usr/bin', 'usr\\lib'] if normsep else ['usr/bin'])
        self.assertEqual(filter(['usr/bin', 'usr', 'usr\\lib'], 'usr\\*'),
                         ['usr/bin', 'usr\\lib'] if normsep else ['usr\\lib'])

    def test_mix_bytes_str(self):
        filter = self.fnmatch.filter
        self.assertRaises(TypeError, filter, ['test'], b'*')
        self.assertRaises(TypeError, filter, [b'test'], '*')

class PurePythonFilterTestCase(FilterTestCaseMixin, unittest.TestCase):
    fnmatch = py_fnmatch

class CPythonFilterTestCase(FilterTestCaseMixin, unittest.TestCase):
    fnmatch = c_fnmatch

if __name__ == "__main__":
    unittest.main()
