import dospath
import test_support
import unittest


class DOSPathTestCase(unittest.TestCase):

    def test_abspath(self):
        self.assert_(dospath.abspath("C:\\") == "C:\\")

    def test_isabs(self):
        isabs = dospath.isabs
        self.assert_(isabs("c:\\"))
        self.assert_(isabs("\\\\conky\\mountpoint\\"))
        self.assert_(isabs("\\foo"))
        self.assert_(isabs("\\foo\\bar"))
        self.failIf(isabs("foo"))
        self.failIf(isabs("foo\\"))
        self.failIf(isabs("foo\\bar"))
        self.failIf(isabs("c:foo"))
        self.failIf(isabs("c:foo\\"))
        self.failIf(isabs("c:foo\\bar"))

    def test_commonprefix(self):
        commonprefix = dospath.commonprefix
        self.assert_(commonprefix(["/home/swenson/spam", "/home/swen/spam"])
                     == "/home/swen")
        self.assert_(commonprefix(["\\home\\swen\\spam", "\\home\\swen\\eggs"])
                     == "\\home\\swen\\")
        self.assert_(commonprefix(["/home/swen/spam", "/home/swen/spam"])
                     == "/home/swen/spam")

    def test_split(self):
        split = dospath.split
        self.assertEquals(split("c:\\foo\\bar"),
                          ('c:\\foo', 'bar'))
        self.assertEquals(split("\\\\conky\\mountpoint\\foo\\bar"),
                          ('\\\\conky\\mountpoint\\foo', 'bar'))

        self.assertEquals(split("c:\\"), ('c:\\', ''))
        self.assertEquals(split("\\\\conky\\mountpoint\\"),
                          ('\\\\conky\\mountpoint', ''))

        self.assertEquals(split("c:/"), ('c:/', ''))
        self.assertEquals(split("//conky/mountpoint/"),
                          ('//conky/mountpoint', ''))

    def test_splitdrive(self):
        splitdrive = dospath.splitdrive
        self.assertEquals(splitdrive("c:\\foo\\bar"), ('c:', '\\foo\\bar'))
        self.assertEquals(splitdrive("c:/foo/bar"), ('c:', '/foo/bar'))
        self.assertEquals(splitdrive("foo\\bar"), ('', 'foo\\bar'))
        self.assertEquals(splitdrive("c:"), ('c:', ''))


def test_main():
    test_support.run_unittest(DOSPathTestCase)


if __name__ == "__main__":
    test_main()
