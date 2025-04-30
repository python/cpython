"""
Tests common to genericpath, ntpath and posixpath
"""

import genericpath
import os
import sys
import unittest
import warnings
from test.support import (
    is_apple, is_emscripten, os_helper, warnings_helper
)
from test.support.script_helper import assert_python_ok
from test.support.os_helper import FakePath


def create_file(filename, data=b'foo'):
    with open(filename, 'xb', 0) as fp:
        fp.write(data)


class GenericTest:
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

        self.assertEqual(
            commonprefix([b"/home/swenson/spam", b"/home/swen/spam"]),
            b"/home/swen"
        )
        self.assertEqual(
            commonprefix([b"/home/swen/spam", b"/home/swen/eggs"]),
            b"/home/swen/"
        )
        self.assertEqual(
            commonprefix([b"/home/swen/spam", b"/home/swen/spam"]),
            b"/home/swen/spam"
        )
        self.assertEqual(
            commonprefix([b"home:swenson:spam", b"home:swen:spam"]),
            b"home:swen"
        )
        self.assertEqual(
            commonprefix([b":home:swen:spam", b":home:swen:eggs"]),
            b":home:swen:"
        )
        self.assertEqual(
            commonprefix([b":home:swen:spam", b":home:swen:spam"]),
            b":home:swen:spam"
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
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        create_file(filename, b'Hello')
        self.assertEqual(self.pathmodule.getsize(filename), 5)
        os.remove(filename)

        create_file(filename, b'Hello World!')
        self.assertEqual(self.pathmodule.getsize(filename), 12)

    def test_filetime(self):
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        create_file(filename, b'foo')

        with open(filename, "ab", 0) as f:
            f.write(b"bar")

        with open(filename, "rb", 0) as f:
            data = f.read()
        self.assertEqual(data, b"foobar")

        self.assertLessEqual(
            self.pathmodule.getctime(filename),
            self.pathmodule.getmtime(filename)
        )

    def test_exists(self):
        filename = os_helper.TESTFN
        bfilename = os.fsencode(filename)
        self.addCleanup(os_helper.unlink, filename)

        self.assertIs(self.pathmodule.exists(filename), False)
        self.assertIs(self.pathmodule.exists(bfilename), False)

        self.assertIs(self.pathmodule.lexists(filename), False)
        self.assertIs(self.pathmodule.lexists(bfilename), False)

        create_file(filename)

        self.assertIs(self.pathmodule.exists(filename), True)
        self.assertIs(self.pathmodule.exists(bfilename), True)

        self.assertIs(self.pathmodule.exists(filename + '\udfff'), False)
        self.assertIs(self.pathmodule.exists(bfilename + b'\xff'), False)
        self.assertIs(self.pathmodule.exists(filename + '\x00'), False)
        self.assertIs(self.pathmodule.exists(bfilename + b'\x00'), False)

        self.assertIs(self.pathmodule.lexists(filename), True)
        self.assertIs(self.pathmodule.lexists(bfilename), True)

        self.assertIs(self.pathmodule.lexists(filename + '\udfff'), False)
        self.assertIs(self.pathmodule.lexists(bfilename + b'\xff'), False)
        self.assertIs(self.pathmodule.lexists(filename + '\x00'), False)
        self.assertIs(self.pathmodule.lexists(bfilename + b'\x00'), False)

        # Keyword arguments are accepted
        self.assertIs(self.pathmodule.exists(path=filename), True)
        self.assertIs(self.pathmodule.lexists(path=filename), True)

    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_exists_fd(self):
        r, w = os.pipe()
        try:
            self.assertTrue(self.pathmodule.exists(r))
        finally:
            os.close(r)
            os.close(w)
        self.assertFalse(self.pathmodule.exists(r))

    def test_exists_bool(self):
        for fd in False, True:
            with self.assertWarnsRegex(RuntimeWarning,
                    'bool is used as a file descriptor'):
                self.pathmodule.exists(fd)

    def test_isdir(self):
        filename = os_helper.TESTFN
        bfilename = os.fsencode(filename)
        self.assertIs(self.pathmodule.isdir(filename), False)
        self.assertIs(self.pathmodule.isdir(bfilename), False)

        self.assertIs(self.pathmodule.isdir(filename + '\udfff'), False)
        self.assertIs(self.pathmodule.isdir(bfilename + b'\xff'), False)
        self.assertIs(self.pathmodule.isdir(filename + '\x00'), False)
        self.assertIs(self.pathmodule.isdir(bfilename + b'\x00'), False)

        try:
            create_file(filename)
            self.assertIs(self.pathmodule.isdir(filename), False)
            self.assertIs(self.pathmodule.isdir(bfilename), False)
        finally:
            os_helper.unlink(filename)

        try:
            os.mkdir(filename)
            self.assertIs(self.pathmodule.isdir(filename), True)
            self.assertIs(self.pathmodule.isdir(bfilename), True)
        finally:
            os_helper.rmdir(filename)

    def test_isfile(self):
        filename = os_helper.TESTFN
        bfilename = os.fsencode(filename)
        self.assertIs(self.pathmodule.isfile(filename), False)
        self.assertIs(self.pathmodule.isfile(bfilename), False)

        self.assertIs(self.pathmodule.isfile(filename + '\udfff'), False)
        self.assertIs(self.pathmodule.isfile(bfilename + b'\xff'), False)
        self.assertIs(self.pathmodule.isfile(filename + '\x00'), False)
        self.assertIs(self.pathmodule.isfile(bfilename + b'\x00'), False)

        try:
            create_file(filename)
            self.assertIs(self.pathmodule.isfile(filename), True)
            self.assertIs(self.pathmodule.isfile(bfilename), True)
        finally:
            os_helper.unlink(filename)

        try:
            os.mkdir(filename)
            self.assertIs(self.pathmodule.isfile(filename), False)
            self.assertIs(self.pathmodule.isfile(bfilename), False)
        finally:
            os_helper.rmdir(filename)

    def test_samefile(self):
        file1 = os_helper.TESTFN
        file2 = os_helper.TESTFN + "2"
        self.addCleanup(os_helper.unlink, file1)
        self.addCleanup(os_helper.unlink, file2)

        create_file(file1)
        self.assertTrue(self.pathmodule.samefile(file1, file1))

        create_file(file2)
        self.assertFalse(self.pathmodule.samefile(file1, file2))

        self.assertRaises(TypeError, self.pathmodule.samefile)

    def _test_samefile_on_link_func(self, func):
        test_fn1 = os_helper.TESTFN
        test_fn2 = os_helper.TESTFN + "2"
        self.addCleanup(os_helper.unlink, test_fn1)
        self.addCleanup(os_helper.unlink, test_fn2)

        create_file(test_fn1)

        func(test_fn1, test_fn2)
        self.assertTrue(self.pathmodule.samefile(test_fn1, test_fn2))
        os.remove(test_fn2)

        create_file(test_fn2)
        self.assertFalse(self.pathmodule.samefile(test_fn1, test_fn2))

    @os_helper.skip_unless_symlink
    def test_samefile_on_symlink(self):
        self._test_samefile_on_link_func(os.symlink)

    @unittest.skipUnless(hasattr(os, 'link'), 'requires os.link')
    def test_samefile_on_link(self):
        try:
            self._test_samefile_on_link_func(os.link)
        except PermissionError as e:
            self.skipTest('os.link(): %s' % e)

    def test_samestat(self):
        test_fn1 = os_helper.TESTFN
        test_fn2 = os_helper.TESTFN + "2"
        self.addCleanup(os_helper.unlink, test_fn1)
        self.addCleanup(os_helper.unlink, test_fn2)

        create_file(test_fn1)
        stat1 = os.stat(test_fn1)
        self.assertTrue(self.pathmodule.samestat(stat1, os.stat(test_fn1)))

        create_file(test_fn2)
        stat2 = os.stat(test_fn2)
        self.assertFalse(self.pathmodule.samestat(stat1, stat2))

        self.assertRaises(TypeError, self.pathmodule.samestat)

    def _test_samestat_on_link_func(self, func):
        test_fn1 = os_helper.TESTFN + "1"
        test_fn2 = os_helper.TESTFN + "2"
        self.addCleanup(os_helper.unlink, test_fn1)
        self.addCleanup(os_helper.unlink, test_fn2)

        create_file(test_fn1)
        func(test_fn1, test_fn2)
        self.assertTrue(self.pathmodule.samestat(os.stat(test_fn1),
                                                 os.stat(test_fn2)))
        os.remove(test_fn2)

        create_file(test_fn2)
        self.assertFalse(self.pathmodule.samestat(os.stat(test_fn1),
                                                  os.stat(test_fn2)))

    @os_helper.skip_unless_symlink
    def test_samestat_on_symlink(self):
        self._test_samestat_on_link_func(os.symlink)

    @unittest.skipUnless(hasattr(os, 'link'), 'requires os.link')
    def test_samestat_on_link(self):
        try:
            self._test_samestat_on_link_func(os.link)
        except PermissionError as e:
            self.skipTest('os.link(): %s' % e)

    def test_sameopenfile(self):
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)
        create_file(filename)

        with open(filename, "rb", 0) as fp1:
            fd1 = fp1.fileno()
            with open(filename, "rb", 0) as fp2:
                fd2 = fp2.fileno()
                self.assertTrue(self.pathmodule.sameopenfile(fd1, fd2))


class TestGenericTest(GenericTest, unittest.TestCase):
    # Issue 16852: GenericTest can't inherit from unittest.TestCase
    # for test discovery purposes; CommonTest inherits from GenericTest
    # and is only meant to be inherited by others.
    pathmodule = genericpath

    def test_invalid_paths(self):
        for attr in GenericTest.common_attributes:
            # os.path.commonprefix doesn't raise ValueError
            if attr == 'commonprefix':
                continue
            func = getattr(self.pathmodule, attr)
            with self.subTest(attr=attr):
                if attr in ('exists', 'isdir', 'isfile'):
                    func('/tmp\udfffabcds')
                    func(b'/tmp\xffabcds')
                    func('/tmp\x00abcds')
                    func(b'/tmp\x00abcds')
                else:
                    with self.assertRaises((OSError, UnicodeEncodeError)):
                        func('/tmp\udfffabcds')
                    with self.assertRaises((OSError, UnicodeDecodeError)):
                        func(b'/tmp\xffabcds')
                    with self.assertRaisesRegex(ValueError, 'embedded null'):
                        func('/tmp\x00abcds')
                    with self.assertRaisesRegex(ValueError, 'embedded null'):
                        func(b'/tmp\x00abcds')

# Following TestCase is not supposed to be run from test_genericpath.
# It is inherited by other test modules (ntpath, posixpath).

class CommonTest(GenericTest):
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
        normcase = self.pathmodule.normcase
        # check that normcase() is idempotent
        for p in ["FoO/./BaR", b"FoO/./BaR"]:
            p = normcase(p)
            self.assertEqual(p, normcase(p))

        self.assertEqual(normcase(''), '')
        self.assertEqual(normcase(b''), b'')

        # check that normcase raises a TypeError for invalid types
        for path in (None, True, 0, 2.5, [], bytearray(b''), {'o','o'}):
            self.assertRaises(TypeError, normcase, path)

    def test_splitdrive(self):
        # splitdrive for non-NT paths
        splitdrive = self.pathmodule.splitdrive
        self.assertEqual(splitdrive("/foo/bar"), ("", "/foo/bar"))
        self.assertEqual(splitdrive("foo:bar"), ("", "foo:bar"))
        self.assertEqual(splitdrive(":foo:bar"), ("", ":foo:bar"))

        self.assertEqual(splitdrive(b"/foo/bar"), (b"", b"/foo/bar"))
        self.assertEqual(splitdrive(b"foo:bar"), (b"", b"foo:bar"))
        self.assertEqual(splitdrive(b":foo:bar"), (b"", b":foo:bar"))

    def test_expandvars(self):
        expandvars = self.pathmodule.expandvars
        with os_helper.EnvironmentVarGuard() as env:
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

            self.assertEqual(expandvars(b"foo"), b"foo")
            self.assertEqual(expandvars(b"$foo bar"), b"bar bar")
            self.assertEqual(expandvars(b"${foo}bar"), b"barbar")
            self.assertEqual(expandvars(b"$[foo]bar"), b"$[foo]bar")
            self.assertEqual(expandvars(b"$bar bar"), b"$bar bar")
            self.assertEqual(expandvars(b"$?bar"), b"$?bar")
            self.assertEqual(expandvars(b"$foo}bar"), b"bar}bar")
            self.assertEqual(expandvars(b"${foo"), b"${foo")
            self.assertEqual(expandvars(b"${{foo}}"), b"baz1}")
            self.assertEqual(expandvars(b"$foo$foo"), b"barbar")
            self.assertEqual(expandvars(b"$bar$bar"), b"$bar$bar")

    @unittest.skipUnless(os_helper.FS_NONASCII, 'need os_helper.FS_NONASCII')
    def test_expandvars_nonascii(self):
        expandvars = self.pathmodule.expandvars
        def check(value, expected):
            self.assertEqual(expandvars(value), expected)
        with os_helper.EnvironmentVarGuard() as env:
            env.clear()
            nonascii = os_helper.FS_NONASCII
            env['spam'] = nonascii
            env[nonascii] = 'ham' + nonascii
            check(nonascii, nonascii)
            check('$spam bar', '%s bar' % nonascii)
            check('${spam}bar', '%sbar' % nonascii)
            check('${%s}bar' % nonascii, 'ham%sbar' % nonascii)
            check('$bar%s bar' % nonascii, '$bar%s bar' % nonascii)
            check('$spam}bar', '%s}bar' % nonascii)

            check(os.fsencode(nonascii), os.fsencode(nonascii))
            check(b'$spam bar', os.fsencode('%s bar' % nonascii))
            check(b'${spam}bar', os.fsencode('%sbar' % nonascii))
            check(os.fsencode('${%s}bar' % nonascii),
                  os.fsencode('ham%sbar' % nonascii))
            check(os.fsencode('$bar%s bar' % nonascii),
                  os.fsencode('$bar%s bar' % nonascii))
            check(b'$spam}bar', os.fsencode('%s}bar' % nonascii))

    def test_abspath(self):
        self.assertIn("foo", self.pathmodule.abspath("foo"))
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertIn(b"foo", self.pathmodule.abspath(b"foo"))

        # avoid UnicodeDecodeError on Windows
        undecodable_path = b'' if sys.platform == 'win32' else b'f\xf2\xf2'

        # Abspath returns bytes when the arg is bytes
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            for path in (b'', b'foo', undecodable_path, b'/foo', b'C:\\'):
                self.assertIsInstance(self.pathmodule.abspath(path), bytes)

    def test_realpath(self):
        self.assertIn("foo", self.pathmodule.realpath("foo"))
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertIn(b"foo", self.pathmodule.realpath(b"foo"))

    def test_normpath_issue5827(self):
        # Make sure normpath preserves unicode
        for path in ('', '.', '/', '\\', '///foo/.//bar//'):
            self.assertIsInstance(self.pathmodule.normpath(path), str)

    def test_normpath_issue106242(self):
        for path in ('\x00', 'foo\x00bar', '\x00\x00', '\x00foo', 'foo\x00'):
            self.assertEqual(self.pathmodule.normpath(path), path)

    def test_abspath_issue3426(self):
        # Check that abspath returns unicode when the arg is unicode
        # with both ASCII and non-ASCII cwds.
        abspath = self.pathmodule.abspath
        for path in ('', 'fuu', 'f\xf9\xf9', '/fuu', 'U:\\'):
            self.assertIsInstance(abspath(path), str)

        unicwd = '\xe7w\xf0'
        try:
            os.fsencode(unicwd)
        except (AttributeError, UnicodeEncodeError):
            # FS encoding is probably ASCII
            pass
        else:
            with os_helper.temp_cwd(unicwd):
                for path in ('', 'fuu', 'f\xf9\xf9', '/fuu', 'U:\\'):
                    self.assertIsInstance(abspath(path), str)

    def test_nonascii_abspath(self):
        if (
            os_helper.TESTFN_UNDECODABLE
            # Apple platforms and Emscripten/WASI deny the creation of a
            # directory with an invalid UTF-8 name. Windows allows creating a
            # directory with an arbitrary bytes name, but fails to enter this
            # directory (when the bytes name is used).
            and sys.platform not in {
                "win32", "emscripten", "wasi"
            } and not is_apple
        ):
            name = os_helper.TESTFN_UNDECODABLE
        elif os_helper.TESTFN_NONASCII:
            name = os_helper.TESTFN_NONASCII
        else:
            self.skipTest("need os_helper.TESTFN_NONASCII")

        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            with os_helper.temp_cwd(name):
                self.test_abspath()

    def test_join_errors(self):
        # Check join() raises friendly TypeErrors.
        with warnings_helper.check_warnings(('', BytesWarning), quiet=True):
            errmsg = "Can't mix strings and bytes in path components"
            with self.assertRaisesRegex(TypeError, errmsg):
                self.pathmodule.join(b'bytes', 'str')
            with self.assertRaisesRegex(TypeError, errmsg):
                self.pathmodule.join('str', b'bytes')
            # regression, see #15377
            with self.assertRaisesRegex(TypeError, 'int'):
                self.pathmodule.join(42, 'str')
            with self.assertRaisesRegex(TypeError, 'int'):
                self.pathmodule.join('str', 42)
            with self.assertRaisesRegex(TypeError, 'int'):
                self.pathmodule.join(42)
            with self.assertRaisesRegex(TypeError, 'list'):
                self.pathmodule.join([])
            with self.assertRaisesRegex(TypeError, 'bytearray'):
                self.pathmodule.join(bytearray(b'foo'), bytearray(b'bar'))

    def test_relpath_errors(self):
        # Check relpath() raises friendly TypeErrors.
        with warnings_helper.check_warnings(
                ('', (BytesWarning, DeprecationWarning)), quiet=True):
            errmsg = "Can't mix strings and bytes in path components"
            with self.assertRaisesRegex(TypeError, errmsg):
                self.pathmodule.relpath(b'bytes', 'str')
            with self.assertRaisesRegex(TypeError, errmsg):
                self.pathmodule.relpath('str', b'bytes')
            with self.assertRaisesRegex(TypeError, 'int'):
                self.pathmodule.relpath(42, 'str')
            with self.assertRaisesRegex(TypeError, 'int'):
                self.pathmodule.relpath('str', 42)
            with self.assertRaisesRegex(TypeError, 'bytearray'):
                self.pathmodule.relpath(bytearray(b'foo'), bytearray(b'bar'))

    def test_import(self):
        assert_python_ok('-S', '-c', 'import ' + self.pathmodule.__name__)


class PathLikeTests(unittest.TestCase):

    def setUp(self):
        self.file_name = os_helper.TESTFN
        self.file_path = FakePath(os_helper.TESTFN)
        self.addCleanup(os_helper.unlink, self.file_name)
        create_file(self.file_name, b"test_genericpath.PathLikeTests")

    def assertPathEqual(self, func):
        self.assertEqual(func(self.file_path), func(self.file_name))

    def test_path_exists(self):
        self.assertPathEqual(os.path.exists)

    def test_path_isfile(self):
        self.assertPathEqual(os.path.isfile)

    def test_path_isdir(self):
        self.assertPathEqual(os.path.isdir)

    def test_path_commonprefix(self):
        self.assertEqual(os.path.commonprefix([self.file_path, self.file_name]),
                         self.file_name)

    def test_path_getsize(self):
        self.assertPathEqual(os.path.getsize)

    def test_path_getmtime(self):
        self.assertPathEqual(os.path.getatime)

    def test_path_getctime(self):
        self.assertPathEqual(os.path.getctime)

    def test_path_samefile(self):
        self.assertTrue(os.path.samefile(self.file_path, self.file_name))


if __name__ == "__main__":
    unittest.main()
