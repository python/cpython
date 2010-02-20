import macpath
from test import test_support
import unittest
import test_genericpath


class MacPathTestCase(unittest.TestCase):

    def test_abspath(self):
        self.assertEqual(macpath.abspath("xx:yy"), "xx:yy")

    def test_abspath_with_ascii_cwd(self):
        test_genericpath._issue3426(self, u'cwd', macpath.abspath)

    def test_abspath_with_nonascii_cwd(self):
        test_genericpath._issue3426(self, u'\xe7w\xf0', macpath.abspath)

    def test_isabs(self):
        isabs = macpath.isabs
        self.assertTrue(isabs("xx:yy"))
        self.assertTrue(isabs("xx:yy:"))
        self.assertTrue(isabs("xx:"))
        self.assertFalse(isabs("foo"))
        self.assertFalse(isabs(":foo"))
        self.assertFalse(isabs(":foo:bar"))
        self.assertFalse(isabs(":foo:bar:"))


    def test_commonprefix(self):
        commonprefix = macpath.commonprefix
        self.assertEqual(commonprefix(["home:swenson:spam", "home:swen:spam"]),
                         "home:swen")
        self.assertEqual(commonprefix([":home:swen:spam", ":home:swen:eggs"]),
                         ":home:swen:")
        self.assertEqual(commonprefix([":home:swen:spam", ":home:swen:spam"]),
                         ":home:swen:spam")

    def test_split(self):
        split = macpath.split
        self.assertEqual(split("foo:bar"),
                          ('foo:', 'bar'))
        self.assertEqual(split("conky:mountpoint:foo:bar"),
                          ('conky:mountpoint:foo', 'bar'))

        self.assertEqual(split(":"), ('', ''))
        self.assertEqual(split(":conky:mountpoint:"),
                          (':conky:mountpoint', ''))

    def test_splitdrive(self):
        splitdrive = macpath.splitdrive
        self.assertEqual(splitdrive("foo:bar"), ('', 'foo:bar'))
        self.assertEqual(splitdrive(":foo:bar"), ('', ':foo:bar'))

    def test_splitext(self):
        splitext = macpath.splitext
        self.assertEqual(splitext(":foo.ext"), (':foo', '.ext'))
        self.assertEqual(splitext("foo:foo.ext"), ('foo:foo', '.ext'))
        self.assertEqual(splitext(".ext"), ('.ext', ''))
        self.assertEqual(splitext("foo.ext:foo"), ('foo.ext:foo', ''))
        self.assertEqual(splitext(":foo.ext:"), (':foo.ext:', ''))
        self.assertEqual(splitext(""), ('', ''))
        self.assertEqual(splitext("foo.bar.ext"), ('foo.bar', '.ext'))


def test_main():
    test_support.run_unittest(MacPathTestCase)


if __name__ == "__main__":
    test_main()
