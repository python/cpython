"""
Tests common to genericpath, macpath, ntpath and posixpath
"""

import unittest
from test import test_support
import os
import genericpath
import sys


def safe_rmdir(dirname):
    try:
        os.rmdir(dirname)
    except OSError:
        pass


class GenericTest(unittest.TestCase):
    # The path module to be tested
    pathmodule = genericpath
    common_attributes = ['commonprefix', 'getsize', 'getatime', 'getctime',
                         'getmtime', 'exists', 'isdir', 'isfile']
    attributes = []

    def test_no_argument(self):
        for attr in self.common_attributes + self.attributes:
            with self.assertRaises(TypeError):
                getattr(self.pathmodule, attr)()
                raise self.fail("{}.{}() did not raise a TypeError"
                                .format(self.pathmodule.__name__, attr))

    def test_commonprefix(self):
        commonprefix = self.pathmodule.commonprefix
        self.assertEqual(
            commonprefix([]),
            ""
        )
        self.assertEqual(
            commonprefix(["/home/swenson/spam", "/home/swen/spam"]),
            "/home/swen"
        )
        self.assertEqual(
            commonprefix(["/home/swen/spam", "/home/swen/eggs"]),
            "/home/swen/"
        )
        self.assertEqual(
            commonprefix(["/home/swen/spam", "/home/swen/spam"]),
            "/home/swen/spam"
        )
        self.assertEqual(
            commonprefix(["home:swenson:spam", "home:swen:spam"]),
            "home:swen"
        )
        self.assertEqual(
            commonprefix([":home:swen:spam", ":home:swen:eggs"]),
            ":home:swen:"
        )
        self.assertEqual(
            commonprefix([":home:swen:spam", ":home:swen:spam"]),
            ":home:swen:spam"
        )

        testlist = ['', 'abc', 'Xbcd', 'Xb', 'XY', 'abcd',
                    'aXc', 'abd', 'ab', 'aX', 'abcX']
        for s1 in testlist:
            for s2 in testlist:
                p = commonprefix([s1, s2])
                self.assertTrue(s1.startswith(p))
                self.assertTrue(s2.startswith(p))
                if s1 != s2:
                    n = len(p)
                    self.assertNotEqual(s1[n:n+1], s2[n:n+1])

    def test_getsize(self):
        f = open(test_support.TESTFN, "wb")
        try:
            f.write("foo")
            f.close()
            self.assertEqual(self.pathmodule.getsize(test_support.TESTFN), 3)
        finally:
            if not f.closed:
                f.close()
            test_support.unlink(test_support.TESTFN)

    def test_time(self):
        f = open(test_support.TESTFN, "wb")
        try:
            f.write("foo")
            f.close()
            f = open(test_support.TESTFN, "ab")
            f.write("bar")
            f.close()
            f = open(test_support.TESTFN, "rb")
            d = f.read()
            f.close()
            self.assertEqual(d, "foobar")

            self.assertLessEqual(
                self.pathmodule.getctime(test_support.TESTFN),
                self.pathmodule.getmtime(test_support.TESTFN)
            )
        finally:
            if not f.closed:
                f.close()
            test_support.unlink(test_support.TESTFN)

    def test_exists(self):
        self.assertIs(self.pathmodule.exists(test_support.TESTFN), False)
        f = open(test_support.TESTFN, "wb")
        try:
            f.write("foo")
            f.close()
            self.assertIs(self.pathmodule.exists(test_support.TESTFN), True)
            if not self.pathmodule == genericpath:
                self.assertIs(self.pathmodule.lexists(test_support.TESTFN),
                              True)
        finally:
            if not f.close():
                f.close()
            test_support.unlink(test_support.TESTFN)

    def test_isdir(self):
        self.assertIs(self.pathmodule.isdir(test_support.TESTFN), False)
        f = open(test_support.TESTFN, "wb")
        try:
            f.write("foo")
            f.close()
            self.assertIs(self.pathmodule.isdir(test_support.TESTFN), False)
            os.remove(test_support.TESTFN)
            os.mkdir(test_support.TESTFN)
            self.assertIs(self.pathmodule.isdir(test_support.TESTFN), True)
            os.rmdir(test_support.TESTFN)
        finally:
            if not f.close():
                f.close()
            test_support.unlink(test_support.TESTFN)
            safe_rmdir(test_support.TESTFN)

    def test_isfile(self):
        self.assertIs(self.pathmodule.isfile(test_support.TESTFN), False)
        f = open(test_support.TESTFN, "wb")
        try:
            f.write("foo")
            f.close()
            self.assertIs(self.pathmodule.isfile(test_support.TESTFN), True)
            os.remove(test_support.TESTFN)
            os.mkdir(test_support.TESTFN)
            self.assertIs(self.pathmodule.isfile(test_support.TESTFN), False)
            os.rmdir(test_support.TESTFN)
        finally:
            if not f.close():
                f.close()
            test_support.unlink(test_support.TESTFN)
            safe_rmdir(test_support.TESTFN)


# Following TestCase is not supposed to be run from test_genericpath.
# It is inherited by other test modules (macpath, ntpath, posixpath).

class CommonTest(GenericTest):
    # The path module to be tested
    pathmodule = None
    common_attributes = GenericTest.common_attributes + [
        # Properties
        'curdir', 'pardir', 'extsep', 'sep',
        'pathsep', 'defpath', 'altsep', 'devnull',
        # Methods
        'normcase', 'splitdrive', 'expandvars', 'normpath', 'abspath',
        'join', 'split', 'splitext', 'isabs', 'basename', 'dirname',
        'lexists', 'islink', 'ismount', 'expanduser', 'normpath', 'realpath',
    ]

    def test_normcase(self):
        # Check that normcase() is idempotent
        p = "FoO/./BaR"
        p = self.pathmodule.normcase(p)
        self.assertEqual(p, self.pathmodule.normcase(p))

    def test_splitdrive(self):
        # splitdrive for non-NT paths
        splitdrive = self.pathmodule.splitdrive
        self.assertEqual(splitdrive("/foo/bar"), ("", "/foo/bar"))
        self.assertEqual(splitdrive("foo:bar"), ("", "foo:bar"))
        self.assertEqual(splitdrive(":foo:bar"), ("", ":foo:bar"))

    def test_expandvars(self):
        if self.pathmodule.__name__ == 'macpath':
            self.skipTest('macpath.expandvars is a stub')
        expandvars = self.pathmodule.expandvars
        with test_support.EnvironmentVarGuard() as env:
            env.clear()
            env["foo"] = "bar"
            env["{foo"] = "baz1"
            env["{foo}"] = "baz2"
            self.assertEqual(expandvars("foo"), "foo")
            self.assertEqual(expandvars("$foo bar"), "bar bar")
            self.assertEqual(expandvars("${foo}bar"), "barbar")
            self.assertEqual(expandvars("$[foo]bar"), "$[foo]bar")
            self.assertEqual(expandvars("$bar bar"), "$bar bar")
            self.assertEqual(expandvars("$?bar"), "$?bar")
            self.assertEqual(expandvars("$foo}bar"), "bar}bar")
            self.assertEqual(expandvars("${foo"), "${foo")
            self.assertEqual(expandvars("${{foo}}"), "baz1}")
            self.assertEqual(expandvars("$foo$foo"), "barbar")
            self.assertEqual(expandvars("$bar$bar"), "$bar$bar")

    @unittest.skipUnless(test_support.FS_NONASCII, 'need test_support.FS_NONASCII')
    def test_expandvars_nonascii(self):
        if self.pathmodule.__name__ == 'macpath':
            self.skipTest('macpath.expandvars is a stub')
        expandvars = self.pathmodule.expandvars
        def check(value, expected):
            self.assertEqual(expandvars(value), expected)
        encoding = sys.getfilesystemencoding()
        with test_support.EnvironmentVarGuard() as env:
            env.clear()
            unonascii = test_support.FS_NONASCII
            snonascii = unonascii.encode(encoding)
            env['spam'] = snonascii
            env[snonascii] = 'ham' + snonascii
            check(snonascii, snonascii)
            check('$spam bar', '%s bar' % snonascii)
            check('${spam}bar', '%sbar' % snonascii)
            check('${%s}bar' % snonascii, 'ham%sbar' % snonascii)
            check('$bar%s bar' % snonascii, '$bar%s bar' % snonascii)
            check('$spam}bar', '%s}bar' % snonascii)

            check(unonascii, unonascii)
            check(u'$spam bar', u'%s bar' % unonascii)
            check(u'${spam}bar', u'%sbar' % unonascii)
            check(u'${%s}bar' % unonascii, u'ham%sbar' % unonascii)
            check(u'$bar%s bar' % unonascii, u'$bar%s bar' % unonascii)
            check(u'$spam}bar', u'%s}bar' % unonascii)

    def test_abspath(self):
        self.assertIn("foo", self.pathmodule.abspath("foo"))

        # Abspath returns bytes when the arg is bytes
        for path in ('', 'foo', 'f\xf2\xf2', '/foo', 'C:\\'):
            self.assertIsInstance(self.pathmodule.abspath(path), str)

    def test_realpath(self):
        self.assertIn("foo", self.pathmodule.realpath("foo"))

    def test_normpath_issue5827(self):
        # Make sure normpath preserves unicode
        for path in (u'', u'.', u'/', u'\\', u'///foo/.//bar//'):
            self.assertIsInstance(self.pathmodule.normpath(path), unicode)

    def test_abspath_issue3426(self):
        # Check that abspath returns unicode when the arg is unicode
        # with both ASCII and non-ASCII cwds.
        abspath = self.pathmodule.abspath
        for path in (u'', u'fuu', u'f\xf9\xf9', u'/fuu', u'U:\\'):
            self.assertIsInstance(abspath(path), unicode)

        unicwd = u'\xe7w\xf0'
        try:
            fsencoding = test_support.TESTFN_ENCODING or "ascii"
            unicwd.encode(fsencoding)
        except (AttributeError, UnicodeEncodeError):
            # FS encoding is probably ASCII
            pass
        else:
            with test_support.temp_cwd(unicwd):
                for path in (u'', u'fuu', u'f\xf9\xf9', u'/fuu', u'U:\\'):
                    self.assertIsInstance(abspath(path), unicode)

    @unittest.skipIf(sys.platform == 'darwin',
        "Mac OS X denies the creation of a directory with an invalid utf8 name")
    def test_nonascii_abspath(self):
        # Test non-ASCII, non-UTF8 bytes in the path.
        with test_support.temp_cwd('\xe7w\xf0'):
            self.test_abspath()


def test_main():
    test_support.run_unittest(GenericTest)


if __name__=="__main__":
    test_main()
