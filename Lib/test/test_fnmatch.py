"""Test cases for the fnmatch module."""

import unittest
import os

from fnmatch import fnmatch, fnmatchcase, translate, filter

class FnmatchTestCase(unittest.TestCase):

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

    def test_mix_bytes_str(self):
        self.assertRaises(TypeError, fnmatch, 'test', b'*')
        self.assertRaises(TypeError, fnmatch, b'test', '*')
        self.assertRaises(TypeError, fnmatchcase, 'test', b'*')
        self.assertRaises(TypeError, fnmatchcase, b'test', '*')

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


class TranslateTestCase(unittest.TestCase):

    def test_translate(self):
        self.assertEqual(translate('*'), r'(?s:.*)\Z')
        self.assertEqual(translate('?'), r'(?s:.)\Z')
        self.assertEqual(translate('a?b*'), r'(?s:a.b.*)\Z')
        self.assertEqual(translate('[abc]'), r'(?s:[abc])\Z')
        self.assertEqual(translate('[]]'), r'(?s:[]])\Z')
        self.assertEqual(translate('[!x]'), r'(?s:[^x])\Z')
        self.assertEqual(translate('[^x]'), r'(?s:[\^x])\Z')
        self.assertEqual(translate('[x'), r'(?s:\[x)\Z')


class FilterTestCase(unittest.TestCase):

    def test_filter(self):
        self.assertEqual(filter(['Python', 'Ruby', 'Perl', 'Tcl'], 'P*'),
                         ['Python', 'Perl'])
        self.assertEqual(filter([b'Python', b'Ruby', b'Perl', b'Tcl'], b'P*'),
                         [b'Python', b'Perl'])

    def test_mix_bytes_str(self):
        self.assertRaises(TypeError, filter, ['test'], b'*')
        self.assertRaises(TypeError, filter, [b'test'], '*')

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


if __name__ == "__main__":
    unittest.main()
