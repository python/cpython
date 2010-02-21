import os
import sys
import macpath
from test import test_support
import unittest


class MacPathTestCase(unittest.TestCase):

    def test_abspath(self):
        self.assert_(macpath.abspath("xx:yy") == "xx:yy")

        # Issue 3426: check that abspath retuns unicode when the arg is unicode
        # and str when it's str, with both ASCII and non-ASCII cwds
        saved_cwd = os.getcwd()
        cwds = ['cwd']
        try:
            cwds.append(u'\xe7w\xf0'.encode(sys.getfilesystemencoding()
                                            or 'ascii'))
        except UnicodeEncodeError:
            pass # the cwd can't be encoded -- test with ascii cwd only
        for cwd in cwds:
            try:
                os.mkdir(cwd)
                os.chdir(cwd)
                for path in ('', 'foo', 'f\xf2\xf2', '/foo', 'C:\\'):
                    self.assertTrue(isinstance(macpath.abspath(path), str))
                for upath in (u'', u'fuu', u'f\xf9\xf9', u'/fuu', u'U:\\'):
                    self.assertTrue(isinstance(macpath.abspath(upath), unicode))
            finally:
                os.chdir(saved_cwd)
                os.rmdir(cwd)


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
