import macpath
from test import test_support, test_genericpath
import unittest


class MacPathTestCase(unittest.TestCase):

    def test_abspath(self):
        self.assertEqual(macpath.abspath("xx:yy"), "xx:yy")

    def test_isabs(self):
        isabs = macpath.isabs
        self.assertTrue(isabs("xx:yy"))
        self.assertTrue(isabs("xx:yy:"))
        self.assertTrue(isabs("xx:"))
        self.assertFalse(isabs("foo"))
        self.assertFalse(isabs(":foo"))
        self.assertFalse(isabs(":foo:bar"))
        self.assertFalse(isabs(":foo:bar:"))

    def test_split(self):
        split = macpath.split
        self.assertEqual(split("foo:bar"),
                          ('foo:', 'bar'))
        self.assertEqual(split("conky:mountpoint:foo:bar"),
                          ('conky:mountpoint:foo', 'bar'))

        self.assertEqual(split(":"), ('', ''))
        self.assertEqual(split(":conky:mountpoint:"),
                          (':conky:mountpoint', ''))

    def test_join(self):
        join = macpath.join
        self.assertEqual(join('a', 'b'), ':a:b')
        self.assertEqual(join(':a', 'b'), ':a:b')
        self.assertEqual(join(':a:', 'b'), ':a:b')
        self.assertEqual(join(':a::', 'b'), ':a::b')
        self.assertEqual(join(':a', '::b'), ':a::b')
        self.assertEqual(join('a', ':'), ':a:')
        self.assertEqual(join('a:', ':'), 'a:')
        self.assertEqual(join('a', ''), ':a:')
        self.assertEqual(join('a:', ''), 'a:')
        self.assertEqual(join('', ''), '')
        self.assertEqual(join('', 'a:b'), 'a:b')
        self.assertEqual(join('', 'a', 'b'), ':a:b')
        self.assertEqual(join('a:b', 'c'), 'a:b:c')
        self.assertEqual(join('a:b', ':c'), 'a:b:c')
        self.assertEqual(join('a', ':b', ':c'), ':a:b:c')
        self.assertEqual(join('a', 'b:'), 'b:')
        self.assertEqual(join('a:', 'b:'), 'b:')

    def test_splitext(self):
        splitext = macpath.splitext
        self.assertEqual(splitext(":foo.ext"), (':foo', '.ext'))
        self.assertEqual(splitext("foo:foo.ext"), ('foo:foo', '.ext'))
        self.assertEqual(splitext(".ext"), ('.ext', ''))
        self.assertEqual(splitext("foo.ext:foo"), ('foo.ext:foo', ''))
        self.assertEqual(splitext(":foo.ext:"), (':foo.ext:', ''))
        self.assertEqual(splitext(""), ('', ''))
        self.assertEqual(splitext("foo.bar.ext"), ('foo.bar', '.ext'))

    def test_normpath(self):
        # Issue 5827: Make sure normpath preserves unicode
        for path in (u'', u'.', u'/', u'\\', u':', u'///foo/.//bar//'):
            self.assertIsInstance(macpath.normpath(path), unicode,
                                  'normpath() returned str instead of unicode')

class MacCommonTest(test_genericpath.CommonTest):
    pathmodule = macpath


def test_main():
    test_support.run_unittest(MacPathTestCase, MacCommonTest)


if __name__ == "__main__":
    test_main()
