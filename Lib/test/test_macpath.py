import macpath
from test import support, test_genericpath
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

        self.assertTrue(isabs(b"xx:yy"))
        self.assertTrue(isabs(b"xx:yy:"))
        self.assertTrue(isabs(b"xx:"))
        self.assertFalse(isabs(b"foo"))
        self.assertFalse(isabs(b":foo"))
        self.assertFalse(isabs(b":foo:bar"))
        self.assertFalse(isabs(b":foo:bar:"))

    def test_split(self):
        split = macpath.split
        self.assertEqual(split("foo:bar"),
                          ('foo:', 'bar'))
        self.assertEqual(split("conky:mountpoint:foo:bar"),
                          ('conky:mountpoint:foo', 'bar'))

        self.assertEqual(split(":"), ('', ''))
        self.assertEqual(split(":conky:mountpoint:"),
                          (':conky:mountpoint', ''))

        self.assertEqual(split(b"foo:bar"),
                          (b'foo:', b'bar'))
        self.assertEqual(split(b"conky:mountpoint:foo:bar"),
                          (b'conky:mountpoint:foo', b'bar'))

        self.assertEqual(split(b":"), (b'', b''))
        self.assertEqual(split(b":conky:mountpoint:"),
                          (b':conky:mountpoint', b''))

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

        self.assertEqual(join(b'a', b'b'), b':a:b')
        self.assertEqual(join(b':a', b'b'), b':a:b')
        self.assertEqual(join(b':a:', b'b'), b':a:b')
        self.assertEqual(join(b':a::', b'b'), b':a::b')
        self.assertEqual(join(b':a', b'::b'), b':a::b')
        self.assertEqual(join(b'a', b':'), b':a:')
        self.assertEqual(join(b'a:', b':'), b'a:')
        self.assertEqual(join(b'a', b''), b':a:')
        self.assertEqual(join(b'a:', b''), b'a:')
        self.assertEqual(join(b'', b''), b'')
        self.assertEqual(join(b'', b'a:b'), b'a:b')
        self.assertEqual(join(b'', b'a', b'b'), b':a:b')
        self.assertEqual(join(b'a:b', b'c'), b'a:b:c')
        self.assertEqual(join(b'a:b', b':c'), b'a:b:c')
        self.assertEqual(join(b'a', b':b', b':c'), b':a:b:c')
        self.assertEqual(join(b'a', b'b:'), b'b:')
        self.assertEqual(join(b'a:', b'b:'), b'b:')

    def test_splitext(self):
        splitext = macpath.splitext
        self.assertEqual(splitext(":foo.ext"), (':foo', '.ext'))
        self.assertEqual(splitext("foo:foo.ext"), ('foo:foo', '.ext'))
        self.assertEqual(splitext(".ext"), ('.ext', ''))
        self.assertEqual(splitext("foo.ext:foo"), ('foo.ext:foo', ''))
        self.assertEqual(splitext(":foo.ext:"), (':foo.ext:', ''))
        self.assertEqual(splitext(""), ('', ''))
        self.assertEqual(splitext("foo.bar.ext"), ('foo.bar', '.ext'))

        self.assertEqual(splitext(b":foo.ext"), (b':foo', b'.ext'))
        self.assertEqual(splitext(b"foo:foo.ext"), (b'foo:foo', b'.ext'))
        self.assertEqual(splitext(b".ext"), (b'.ext', b''))
        self.assertEqual(splitext(b"foo.ext:foo"), (b'foo.ext:foo', b''))
        self.assertEqual(splitext(b":foo.ext:"), (b':foo.ext:', b''))
        self.assertEqual(splitext(b""), (b'', b''))
        self.assertEqual(splitext(b"foo.bar.ext"), (b'foo.bar', b'.ext'))

    def test_ismount(self):
        ismount = macpath.ismount
        self.assertEqual(ismount("a:"), True)
        self.assertEqual(ismount("a:b"), False)
        self.assertEqual(ismount("a:b:"), True)
        self.assertEqual(ismount(""), False)
        self.assertEqual(ismount(":"), False)

        self.assertEqual(ismount(b"a:"), True)
        self.assertEqual(ismount(b"a:b"), False)
        self.assertEqual(ismount(b"a:b:"), True)
        self.assertEqual(ismount(b""), False)
        self.assertEqual(ismount(b":"), False)

    def test_normpath(self):
        normpath = macpath.normpath
        self.assertEqual(normpath("a:b"), "a:b")
        self.assertEqual(normpath("a"), ":a")
        self.assertEqual(normpath("a:b::c"), "a:c")
        self.assertEqual(normpath("a:b:c:::d"), "a:d")
        self.assertRaises(macpath.norm_error, normpath, "a::b")
        self.assertRaises(macpath.norm_error, normpath, "a:b:::c")
        self.assertEqual(normpath(":"), ":")
        self.assertEqual(normpath("a:"), "a:")
        self.assertEqual(normpath("a:b:"), "a:b")

        self.assertEqual(normpath(b"a:b"), b"a:b")
        self.assertEqual(normpath(b"a"), b":a")
        self.assertEqual(normpath(b"a:b::c"), b"a:c")
        self.assertEqual(normpath(b"a:b:c:::d"), b"a:d")
        self.assertRaises(macpath.norm_error, normpath, b"a::b")
        self.assertRaises(macpath.norm_error, normpath, b"a:b:::c")
        self.assertEqual(normpath(b":"), b":")
        self.assertEqual(normpath(b"a:"), b"a:")
        self.assertEqual(normpath(b"a:b:"), b"a:b")


class MacCommonTest(test_genericpath.CommonTest, unittest.TestCase):
    pathmodule = macpath


if __name__ == "__main__":
    unittest.main()
