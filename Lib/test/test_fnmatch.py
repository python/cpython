"""Test cases for the fnmatch module."""

from test import test_support
import unittest
import os

from fnmatch import (fnmatch, fnmatchcase, translate, filter,
                     _MAXCACHE, _cache, _purge)


class FnmatchTestCase(unittest.TestCase):

    def tearDown(self):
        _purge()

    def check_match(self, filename, pattern, should_match=True, fn=fnmatch):
        if should_match:
            self.assertTrue(fn(filename, pattern),
                         "expected %r to match pattern %r"
                         % (filename, pattern))
        else:
            self.assertFalse(fn(filename, pattern),
                         "expected %r not to match pattern %r"
                         % (filename, pattern))

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

    def test_mix_unicode_str(self):
        check = self.check_match
        check('test', u'*')
        check(u'test', '*')
        check('test', u'*', fn=fnmatchcase)
        check(u'test', '*', fn=fnmatchcase)
        with test_support.check_warnings(("", UnicodeWarning), quiet=True):
            check('test\xff', u'*\xff')
            check(u'test\xff', '*\xff')
            check('test\xff', u'*\xff', fn=fnmatchcase)
            check(u'test\xff', '*\xff', fn=fnmatchcase)

    def test_fnmatchcase(self):
        check = self.check_match
        check('abc', 'abc', True, fnmatchcase)
        check('AbC', 'abc', False, fnmatchcase)
        check('abc', 'AbC', False, fnmatchcase)
        check('AbC', 'AbC', True, fnmatchcase)

        check('usr/bin', 'usr/bin', True, fnmatchcase)
        check('usr\\bin', 'usr/bin', False, fnmatchcase)
        check('usr/bin', 'usr\\bin', False, fnmatchcase)
        check('usr\\bin', 'usr\\bin', True, fnmatchcase)

    def test_cache_clearing(self):
        # check that caches do not grow too large
        # http://bugs.python.org/issue7846

        # string pattern cache
        for i in range(_MAXCACHE + 1):
            fnmatch('foo', '?' * i)

        self.assertLessEqual(len(_cache), _MAXCACHE)

    @test_support.requires_unicode
    def test_unicode(self):
        with test_support.check_warnings(("", UnicodeWarning), quiet=True):
            self.check_match(u'test', u'te*')
            self.check_match(u'test\xff', u'te*\xff')
            self.check_match(u'test'+unichr(0x20ac), u'te*'+unichr(0x20ac))
            self.check_match(u'foo\nbar', u'foo*')

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


class TranslateTestCase(unittest.TestCase):

    def test_translate(self):
        self.assertEqual(translate('*'), r'.*\Z(?ms)')
        self.assertEqual(translate('?'), r'.\Z(?ms)')
        self.assertEqual(translate('a?b*'), r'a.b.*\Z(?ms)')
        self.assertEqual(translate('[abc]'), r'[abc]\Z(?ms)')
        self.assertEqual(translate('[]]'), r'[]]\Z(?ms)')
        self.assertEqual(translate('[!x]'), r'[^x]\Z(?ms)')
        self.assertEqual(translate('[^x]'), r'[\^x]\Z(?ms)')
        self.assertEqual(translate('[x'), r'\[x\Z(?ms)')


class FilterTestCase(unittest.TestCase):

    def test_filter(self):
        self.assertEqual(filter(['Python', 'Ruby', 'Perl', 'Tcl'], 'P*'),
                         ['Python', 'Perl'])
        self.assertEqual(filter([u'Python', u'Ruby', u'Perl', u'Tcl'], u'P*'),
                         [u'Python', u'Perl'])
        with test_support.check_warnings(("", UnicodeWarning), quiet=True):
            self.assertEqual(filter([u'test\xff'], u'*\xff'), [u'test\xff'])

    @test_support.requires_unicode
    def test_mix_bytes_str(self):
        with test_support.check_warnings(("", UnicodeWarning), quiet=True):
            self.assertEqual(filter(['test'], u'*'), ['test'])
            self.assertEqual(filter([u'test'], '*'), [u'test'])
            self.assertEqual(filter(['test\xff'], u'*'), ['test\xff'])
            self.assertEqual(filter([u'test\xff'], '*'), [u'test\xff'])
            self.assertEqual(filter(['test\xff'], u'*\xff'), ['test\xff'])
            self.assertEqual(filter([u'test\xff'], '*\xff'), [u'test\xff'])

    def test_case(self):
        ignorecase = os.path.normcase('P') == os.path.normcase('p')
        self.assertEqual(filter(['Test.py', 'Test.rb', 'Test.PL'], '*.p*'),
                         ['Test.py', 'Test.PL'] if ignorecase else ['Test.py'])
        self.assertEqual(filter(['Test.py', 'Test.rb', 'Test.PL'], '*.P*'),
                         ['Test.py', 'Test.PL'] if ignorecase else ['Test.PL'])

    def test_sep(self):
        normsep = os.path.normcase('\\') == os.path.normcase('/')
        self.assertEqual(filter(['usr/bin', 'usr', 'usr\\lib'], 'usr/*'),
                         ['usr/bin', 'usr\\lib'] if normsep else ['usr/bin'])
        self.assertEqual(filter(['usr/bin', 'usr', 'usr\\lib'], 'usr\\*'),
                         ['usr/bin', 'usr\\lib'] if normsep else ['usr\\lib'])


def test_main():
    test_support.run_unittest(FnmatchTestCase, TranslateTestCase, FilterTestCase)


if __name__ == "__main__":
    test_main()
