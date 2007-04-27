import macpath
from test import test_support
import unittest


class MacPathTestCase(unittest.TestCase):

    def test_abspath(self):
        self.assert_(macpath.abspath("xx:yy") == "xx:yy")

    def test_isabs(self):
        isabs = macpath.isabs
        self.assert_(isabs("xx:yy"))
        self.assert_(isabs("xx:yy:"))
        self.assert_(isabs("xx:"))
        self.failIf(isabs("foo"))
        self.failIf(isabs(":foo"))
        self.failIf(isabs(":foo:bar"))
        self.failIf(isabs(":foo:bar:"))


    def test_commonprefix(self):
        commonprefix = macpath.commonprefix
        self.assert_(commonprefix(["home:swenson:spam", "home:swen:spam"])
                     == "home:swen")
        self.assert_(commonprefix([":home:swen:spam", ":home:swen:eggs"])
                     == ":home:swen:")
        self.assert_(commonprefix([":home:swen:spam", ":home:swen:spam"])
                     == ":home:swen:spam")

    def test_split(self):
        split = macpath.split
        self.assertEquals(split("foo:bar"),
                          ('foo:', 'bar'))
        self.assertEquals(split("conky:mountpoint:foo:bar"),
                          ('conky:mountpoint:foo', 'bar'))

        self.assertEquals(split(":"), ('', ''))
        self.assertEquals(split(":conky:mountpoint:"),
                          (':conky:mountpoint', ''))

    def test_splitdrive(self):
        splitdrive = macpath.splitdrive
        self.assertEquals(splitdrive("foo:bar"), ('', 'foo:bar'))
        self.assertEquals(splitdrive(":foo:bar"), ('', ':foo:bar'))

    def test_splitext(self):
        splitext = macpath.splitext
        self.assertEquals(splitext(":foo.ext"), (':foo', '.ext'))
        self.assertEquals(splitext("foo:foo.ext"), ('foo:foo', '.ext'))
        self.assertEquals(splitext(".ext"), ('.ext', ''))
        self.assertEquals(splitext("foo.ext:foo"), ('foo.ext:foo', ''))
        self.assertEquals(splitext(":foo.ext:"), (':foo.ext:', ''))
        self.assertEquals(splitext(""), ('', ''))
        self.assertEquals(splitext("foo.bar.ext"), ('foo.bar', '.ext'))


def test_main():
    test_support.run_unittest(MacPathTestCase)


if __name__ == "__main__":
    test_main()
