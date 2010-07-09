"""Test cases for the fnmatch module."""

from test import support
import unittest

from fnmatch import fnmatch, fnmatchcase, _MAXCACHE, _cache, _cacheb


class FnmatchTestCase(unittest.TestCase):
    def check_match(self, filename, pattern, should_match=1):
        if should_match:
            self.assertTrue(fnmatch(filename, pattern),
                         "expected %r to match pattern %r"
                         % (filename, pattern))
        else:
            self.assertTrue(not fnmatch(filename, pattern),
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

    def test_mix_bytes_str(self):
        self.assertRaises(TypeError, fnmatch, 'test', b'*')
        self.assertRaises(TypeError, fnmatch, b'test', '*')
        self.assertRaises(TypeError, fnmatchcase, 'test', b'*')
        self.assertRaises(TypeError, fnmatchcase, b'test', '*')

    def test_bytes(self):
        self.check_match(b'test', b'te*')
        self.check_match(b'test\xff', b'te*\xff')
        self.check_match(b'foo\nbar', b'foo*')

    def test_cache_clearing(self):
        # check that caches do not grow too large
        # http://bugs.python.org/issue7846

        # string pattern cache
        for i in range(_MAXCACHE + 1):
            fnmatch('foo', '?' * i)

        self.assertLessEqual(len(_cache), _MAXCACHE)

        # bytes pattern cache
        for i in range(_MAXCACHE + 1):
            fnmatch(b'foo', b'?' * i)
        self.assertLessEqual(len(_cacheb), _MAXCACHE)


def test_main():
    support.run_unittest(FnmatchTestCase)


if __name__ == "__main__":
    test_main()
