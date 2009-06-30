import macpath
from test import support
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


    def test_commonprefix(self):
        commonprefix = macpath.commonprefix
        self.assertTrue(commonprefix(["home:swenson:spam", "home:swen:spam"])
                     == "home:swen")
        self.assertTrue(commonprefix([":home:swen:spam", ":home:swen:eggs"])
                     == ":home:swen:")
        self.assertTrue(commonprefix([":home:swen:spam", ":home:swen:spam"])
                     == ":home:swen:spam")

        self.assertTrue(commonprefix([b"home:swenson:spam", b"home:swen:spam"])
                     == b"home:swen")
        self.assertTrue(commonprefix([b":home:swen:spam", b":home:swen:eggs"])
                     == b":home:swen:")
        self.assertTrue(commonprefix([b":home:swen:spam", b":home:swen:spam"])
                     == b":home:swen:spam")

    def test_split(self):
        split = macpath.split
        self.assertEquals(split("foo:bar"),
                          ('foo:', 'bar'))
        self.assertEquals(split("conky:mountpoint:foo:bar"),
                          ('conky:mountpoint:foo', 'bar'))

        self.assertEquals(split(":"), ('', ''))
        self.assertEquals(split(":conky:mountpoint:"),
                          (':conky:mountpoint', ''))

        self.assertEquals(split(b"foo:bar"),
                          (b'foo:', b'bar'))
        self.assertEquals(split(b"conky:mountpoint:foo:bar"),
                          (b'conky:mountpoint:foo', b'bar'))

        self.assertEquals(split(b":"), (b'', b''))
        self.assertEquals(split(b":conky:mountpoint:"),
                          (b':conky:mountpoint', b''))

    def test_join(self):
        join = macpath.join
        self.assertEquals(join('a', 'b'), ':a:b')
        self.assertEquals(join('', 'a:b'), 'a:b')
        self.assertEquals(join('a:b', 'c'), 'a:b:c')
        self.assertEquals(join('a:b', ':c'), 'a:b:c')
        self.assertEquals(join('a', ':b', ':c'), ':a:b:c')

        self.assertEquals(join(b'a', b'b'), b':a:b')
        self.assertEquals(join(b'', b'a:b'), b'a:b')
        self.assertEquals(join(b'a:b', b'c'), b'a:b:c')
        self.assertEquals(join(b'a:b', b':c'), b'a:b:c')
        self.assertEquals(join(b'a', b':b', b':c'), b':a:b:c')

    def test_splitdrive(self):
        splitdrive = macpath.splitdrive
        self.assertEquals(splitdrive("foo:bar"), ('', 'foo:bar'))
        self.assertEquals(splitdrive(":foo:bar"), ('', ':foo:bar'))

        self.assertEquals(splitdrive(b"foo:bar"), (b'', b'foo:bar'))
        self.assertEquals(splitdrive(b":foo:bar"), (b'', b':foo:bar'))

    def test_splitext(self):
        splitext = macpath.splitext
        self.assertEquals(splitext(":foo.ext"), (':foo', '.ext'))
        self.assertEquals(splitext("foo:foo.ext"), ('foo:foo', '.ext'))
        self.assertEquals(splitext(".ext"), ('.ext', ''))
        self.assertEquals(splitext("foo.ext:foo"), ('foo.ext:foo', ''))
        self.assertEquals(splitext(":foo.ext:"), (':foo.ext:', ''))
        self.assertEquals(splitext(""), ('', ''))
        self.assertEquals(splitext("foo.bar.ext"), ('foo.bar', '.ext'))

        self.assertEquals(splitext(b":foo.ext"), (b':foo', b'.ext'))
        self.assertEquals(splitext(b"foo:foo.ext"), (b'foo:foo', b'.ext'))
        self.assertEquals(splitext(b".ext"), (b'.ext', b''))
        self.assertEquals(splitext(b"foo.ext:foo"), (b'foo.ext:foo', b''))
        self.assertEquals(splitext(b":foo.ext:"), (b':foo.ext:', b''))
        self.assertEquals(splitext(b""), (b'', b''))
        self.assertEquals(splitext(b"foo.bar.ext"), (b'foo.bar', b'.ext'))

    def test_ismount(self):
        ismount = macpath.ismount
        self.assertEquals(ismount("a:"), True)
        self.assertEquals(ismount("a:b"), False)
        self.assertEquals(ismount("a:b:"), True)
        self.assertEquals(ismount(""), False)
        self.assertEquals(ismount(":"), False)

        self.assertEquals(ismount(b"a:"), True)
        self.assertEquals(ismount(b"a:b"), False)
        self.assertEquals(ismount(b"a:b:"), True)
        self.assertEquals(ismount(b""), False)
        self.assertEquals(ismount(b":"), False)

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

def test_main():
    support.run_unittest(MacPathTestCase)


if __name__ == "__main__":
    test_main()
