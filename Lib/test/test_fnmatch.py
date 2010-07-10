"""Test cases for the fnmatch module."""

from test import test_support
import unittest

from fnmatch import fnmatch, fnmatchcase, _MAXCACHE, _cache
from fnmatch import fnmatch, fnmatchcase, _MAXCACHE, _cache, _purge


class FnmatchTestCase(unittest.TestCase):

    def tearDown(self):
        _purge()

    def check_match(self, filename, pattern, should_match=1, fn=fnmatch):
        if should_match:
            self.assertTrue(fn(filename, pattern),
                            "expected %r to match pattern %r"
                            % (filename, pattern))
        else:
            self.assertTrue(not fn(filename, pattern),
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
        check('abc', 'ab[de]', 0)
        check('a', '??', 0)
        check('a', 'b', 0)

        # these test that '\' is handled correctly in character sets;
        # see SF bug #409651
        check('\\', r'[\]')
        check('a', r'[!\]')
        check('\\', r'[!\]', 0)

        # test that filenames with newlines in them are handled correctly.
        # http://bugs.python.org/issue6665
        check('foo\nbar', 'foo*')
        check('foo\nbar\n', 'foo*')
        check('\nfoo', 'foo*', False)
        check('\n', '*')

    def test_fnmatchcase(self):
        check = self.check_match
        check('AbC', 'abc', 0, fnmatchcase)
        check('abc', 'AbC', 0, fnmatchcase)

    def test_cache_clearing(self):
        # check that caches do not grow too large
        # http://bugs.python.org/issue7846

        # string pattern cache
        for i in range(_MAXCACHE + 1):
            fnmatch('foo', '?' * i)

        self.assertTrue(len(_cache) <= _MAXCACHE)

def test_main():
    test_support.run_unittest(FnmatchTestCase)


if __name__ == "__main__":
    test_main()
