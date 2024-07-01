import inspect
import os
import posixpath
import sys
import unittest
from posixpath import realpath, abspath, dirname, basename
from test import test_genericpath
from test.support import import_helper
from test.support import cpython_only, os_helper
from test.support.os_helper import FakePath
from unittest import mock

try:
    import posix
except ImportError:
    posix = None


# An absolute path to a temporary filename for testing. We can't rely on TESTFN
# being an absolute path, so we need this.

ABSTFN = abspath(os_helper.TESTFN)

def skip_if_ABSTFN_contains_backslash(test):
    """
    On Windows, posixpath.abspath still returns paths with backslashes
    instead of posix forward slashes. If this is the case, several tests
    fail, so skip them.
    """
    found_backslash = '\\' in ABSTFN
    msg = "ABSTFN is not a posix path - tests fail"
    return [test, unittest.skip(msg)(test)][found_backslash]

def safe_rmdir(dirname):
    try:
        os.rmdir(dirname)
    except OSError:
        pass

class PosixPathTest(unittest.TestCase):

    def setUp(self):
        self.tearDown()

    def tearDown(self):
        for suffix in ["", "1", "2"]:
            os_helper.unlink(os_helper.TESTFN + suffix)
            safe_rmdir(os_helper.TESTFN + suffix)

    def test_join(self):
        self.assertEqual(posixpath.join("/foo", "bar", "/bar", "baz"),
                         "/bar/baz")
        self.assertEqual(posixpath.join("/foo", "bar", "baz"), "/foo/bar/baz")
        self.assertEqual(posixpath.join("/foo/", "bar/", "baz/"),
                         "/foo/bar/baz/")

        self.assertEqual(posixpath.join(b"/foo", b"bar", b"/bar", b"baz"),
                         b"/bar/baz")
        self.assertEqual(posixpath.join(b"/foo", b"bar", b"baz"),
                         b"/foo/bar/baz")
        self.assertEqual(posixpath.join(b"/foo/", b"bar/", b"baz/"),
                         b"/foo/bar/baz/")

    def test_split(self):
        self.assertEqual(posixpath.split("/foo/bar"), ("/foo", "bar"))
        self.assertEqual(posixpath.split("/"), ("/", ""))
        self.assertEqual(posixpath.split("foo"), ("", "foo"))
        self.assertEqual(posixpath.split("////foo"), ("////", "foo"))
        self.assertEqual(posixpath.split("//foo//bar"), ("//foo", "bar"))

        self.assertEqual(posixpath.split(b"/foo/bar"), (b"/foo", b"bar"))
        self.assertEqual(posixpath.split(b"/"), (b"/", b""))
        self.assertEqual(posixpath.split(b"foo"), (b"", b"foo"))
        self.assertEqual(posixpath.split(b"////foo"), (b"////", b"foo"))
        self.assertEqual(posixpath.split(b"//foo//bar"), (b"//foo", b"bar"))

    def splitextTest(self, path, filename, ext):
        self.assertEqual(posixpath.splitext(path), (filename, ext))
        self.assertEqual(posixpath.splitext("/" + path), ("/" + filename, ext))
        self.assertEqual(posixpath.splitext("abc/" + path),
                         ("abc/" + filename, ext))
        self.assertEqual(posixpath.splitext("abc.def/" + path),
                         ("abc.def/" + filename, ext))
        self.assertEqual(posixpath.splitext("/abc.def/" + path),
                         ("/abc.def/" + filename, ext))
        self.assertEqual(posixpath.splitext(path + "/"),
                         (filename + ext + "/", ""))

        path = bytes(path, "ASCII")
        filename = bytes(filename, "ASCII")
        ext = bytes(ext, "ASCII")

        self.assertEqual(posixpath.splitext(path), (filename, ext))
        self.assertEqual(posixpath.splitext(b"/" + path),
                         (b"/" + filename, ext))
        self.assertEqual(posixpath.splitext(b"abc/" + path),
                         (b"abc/" + filename, ext))
        self.assertEqual(posixpath.splitext(b"abc.def/" + path),
                         (b"abc.def/" + filename, ext))
        self.assertEqual(posixpath.splitext(b"/abc.def/" + path),
                         (b"/abc.def/" + filename, ext))
        self.assertEqual(posixpath.splitext(path + b"/"),
                         (filename + ext + b"/", b""))

    def test_splitext(self):
        self.splitextTest("foo.bar", "foo", ".bar")
        self.splitextTest("foo.boo.bar", "foo.boo", ".bar")
        self.splitextTest("foo.boo.biff.bar", "foo.boo.biff", ".bar")
        self.splitextTest(".csh.rc", ".csh", ".rc")
        self.splitextTest("nodots", "nodots", "")
        self.splitextTest(".cshrc", ".cshrc", "")
        self.splitextTest("...manydots", "...manydots", "")
        self.splitextTest("...manydots.ext", "...manydots", ".ext")
        self.splitextTest(".", ".", "")
        self.splitextTest("..", "..", "")
        self.splitextTest("........", "........", "")
        self.splitextTest("", "", "")

    def test_splitroot(self):
        f = posixpath.splitroot
        self.assertEqual(f(''), ('', '', ''))
        self.assertEqual(f('a'), ('', '', 'a'))
        self.assertEqual(f('a/b'), ('', '', 'a/b'))
        self.assertEqual(f('a/b/'), ('', '', 'a/b/'))
        self.assertEqual(f('/a'), ('', '/', 'a'))
        self.assertEqual(f('/a/b'), ('', '/', 'a/b'))
        self.assertEqual(f('/a/b/'), ('', '/', 'a/b/'))
        # The root is collapsed when there are redundant slashes
        # except when there are exactly two leading slashes, which
        # is a special case in POSIX.
        self.assertEqual(f('//a'), ('', '//', 'a'))
        self.assertEqual(f('///a'), ('', '/', '//a'))
        self.assertEqual(f('///a/b'), ('', '/', '//a/b'))
        # Paths which look like NT paths aren't treated specially.
        self.assertEqual(f('c:/a/b'), ('', '', 'c:/a/b'))
        self.assertEqual(f('\\/a/b'), ('', '', '\\/a/b'))
        self.assertEqual(f('\\a\\b'), ('', '', '\\a\\b'))
        # Byte paths are supported
        self.assertEqual(f(b''), (b'', b'', b''))
        self.assertEqual(f(b'a'), (b'', b'', b'a'))
        self.assertEqual(f(b'/a'), (b'', b'/', b'a'))
        self.assertEqual(f(b'//a'), (b'', b'//', b'a'))
        self.assertEqual(f(b'///a'), (b'', b'/', b'//a'))

    def test_isabs(self):
        self.assertIs(posixpath.isabs(""), False)
        self.assertIs(posixpath.isabs("/"), True)
        self.assertIs(posixpath.isabs("/foo"), True)
        self.assertIs(posixpath.isabs("/foo/bar"), True)
        self.assertIs(posixpath.isabs("foo/bar"), False)

        self.assertIs(posixpath.isabs(b""), False)
        self.assertIs(posixpath.isabs(b"/"), True)
        self.assertIs(posixpath.isabs(b"/foo"), True)
        self.assertIs(posixpath.isabs(b"/foo/bar"), True)
        self.assertIs(posixpath.isabs(b"foo/bar"), False)

    def test_basename(self):
        self.assertEqual(posixpath.basename("/foo/bar"), "bar")
        self.assertEqual(posixpath.basename("/"), "")
        self.assertEqual(posixpath.basename("foo"), "foo")
        self.assertEqual(posixpath.basename("////foo"), "foo")
        self.assertEqual(posixpath.basename("//foo//bar"), "bar")

        self.assertEqual(posixpath.basename(b"/foo/bar"), b"bar")
        self.assertEqual(posixpath.basename(b"/"), b"")
        self.assertEqual(posixpath.basename(b"foo"), b"foo")
        self.assertEqual(posixpath.basename(b"////foo"), b"foo")
        self.assertEqual(posixpath.basename(b"//foo//bar"), b"bar")

    def test_dirname(self):
        self.assertEqual(posixpath.dirname("/foo/bar"), "/foo")
        self.assertEqual(posixpath.dirname("/"), "/")
        self.assertEqual(posixpath.dirname("foo"), "")
        self.assertEqual(posixpath.dirname("////foo"), "////")
        self.assertEqual(posixpath.dirname("//foo//bar"), "//foo")

        self.assertEqual(posixpath.dirname(b"/foo/bar"), b"/foo")
        self.assertEqual(posixpath.dirname(b"/"), b"/")
        self.assertEqual(posixpath.dirname(b"foo"), b"")
        self.assertEqual(posixpath.dirname(b"////foo"), b"////")
        self.assertEqual(posixpath.dirname(b"//foo//bar"), b"//foo")

    def test_islink(self):
        self.assertIs(posixpath.islink(os_helper.TESTFN + "1"), False)
        self.assertIs(posixpath.lexists(os_helper.TESTFN + "2"), False)

        with open(os_helper.TESTFN + "1", "wb") as f:
            f.write(b"foo")
        self.assertIs(posixpath.islink(os_helper.TESTFN + "1"), False)

        if os_helper.can_symlink():
            os.symlink(os_helper.TESTFN + "1", os_helper.TESTFN + "2")
            self.assertIs(posixpath.islink(os_helper.TESTFN + "2"), True)
            os.remove(os_helper.TESTFN + "1")
            self.assertIs(posixpath.islink(os_helper.TESTFN + "2"), True)
            self.assertIs(posixpath.exists(os_helper.TESTFN + "2"), False)
            self.assertIs(posixpath.lexists(os_helper.TESTFN + "2"), True)

        self.assertIs(posixpath.islink(os_helper.TESTFN + "\udfff"), False)
        self.assertIs(posixpath.islink(os.fsencode(os_helper.TESTFN) + b"\xff"), False)
        self.assertIs(posixpath.islink(os_helper.TESTFN + "\x00"), False)
        self.assertIs(posixpath.islink(os.fsencode(os_helper.TESTFN) + b"\x00"), False)

    def test_ismount(self):
        self.assertIs(posixpath.ismount("/"), True)
        self.assertIs(posixpath.ismount(b"/"), True)
        self.assertIs(posixpath.ismount(FakePath("/")), True)
        self.assertIs(posixpath.ismount(FakePath(b"/")), True)

    def test_ismount_non_existent(self):
        # Non-existent mountpoint.
        self.assertIs(posixpath.ismount(ABSTFN), False)
        try:
            os.mkdir(ABSTFN)
            self.assertIs(posixpath.ismount(ABSTFN), False)
        finally:
            safe_rmdir(ABSTFN)

        self.assertIs(posixpath.ismount('/\udfff'), False)
        self.assertIs(posixpath.ismount(b'/\xff'), False)
        self.assertIs(posixpath.ismount('/\x00'), False)
        self.assertIs(posixpath.ismount(b'/\x00'), False)

    @os_helper.skip_unless_symlink
    def test_ismount_symlinks(self):
        # Symlinks are never mountpoints.
        try:
            os.symlink("/", ABSTFN)
            self.assertIs(posixpath.ismount(ABSTFN), False)
        finally:
            os.unlink(ABSTFN)

    @unittest.skipIf(posix is None, "Test requires posix module")
    def test_ismount_different_device(self):
        # Simulate the path being on a different device from its parent by
        # mocking out st_dev.
        save_lstat = os.lstat
        def fake_lstat(path):
            st_ino = 0
            st_dev = 0
            if path == ABSTFN:
                st_dev = 1
                st_ino = 1
            return posix.stat_result((0, st_ino, st_dev, 0, 0, 0, 0, 0, 0, 0))
        try:
            os.lstat = fake_lstat
            self.assertIs(posixpath.ismount(ABSTFN), True)
        finally:
            os.lstat = save_lstat

    @unittest.skipIf(posix is None, "Test requires posix module")
    def test_ismount_directory_not_readable(self):
        # issue #2466: Simulate ismount run on a directory that is not
        # readable, which used to return False.
        save_lstat = os.lstat
        def fake_lstat(path):
            st_ino = 0
            st_dev = 0
            if path.startswith(ABSTFN) and path != ABSTFN:
                # ismount tries to read something inside the ABSTFN directory;
                # simulate this being forbidden (no read permission).
                raise OSError("Fake [Errno 13] Permission denied")
            if path == ABSTFN:
                st_dev = 1
                st_ino = 1
            return posix.stat_result((0, st_ino, st_dev, 0, 0, 0, 0, 0, 0, 0))
        try:
            os.lstat = fake_lstat
            self.assertIs(posixpath.ismount(ABSTFN), True)
        finally:
            os.lstat = save_lstat

    def test_isjunction(self):
        self.assertFalse(posixpath.isjunction(ABSTFN))

    @unittest.skipIf(sys.platform == 'win32', "Fast paths are not for win32")
    @cpython_only
    def test_fast_paths_in_use(self):
        # There are fast paths of these functions implemented in posixmodule.c.
        # Confirm that they are being used, and not the Python fallbacks
        self.assertTrue(os.path.normpath is posix._path_normpath)
        self.assertFalse(inspect.isfunction(os.path.normpath))

    def test_expanduser(self):
        self.assertEqual(posixpath.expanduser("foo"), "foo")
        self.assertEqual(posixpath.expanduser(b"foo"), b"foo")

    def test_expanduser_home_envvar(self):
        with os_helper.EnvironmentVarGuard() as env:
            env['HOME'] = '/home/victor'
            self.assertEqual(posixpath.expanduser("~"), "/home/victor")

            # expanduser() strips trailing slash
            env['HOME'] = '/home/victor/'
            self.assertEqual(posixpath.expanduser("~"), "/home/victor")

            for home in '/', '', '//', '///':
                with self.subTest(home=home):
                    env['HOME'] = home
                    self.assertEqual(posixpath.expanduser("~"), "/")
                    self.assertEqual(posixpath.expanduser("~/"), "/")
                    self.assertEqual(posixpath.expanduser("~/foo"), "/foo")

    @unittest.skipIf(sys.platform == "vxworks",
                     "no home directory on VxWorks")
    def test_expanduser_pwd(self):
        pwd = import_helper.import_module('pwd')

        self.assertIsInstance(posixpath.expanduser("~/"), str)
        self.assertIsInstance(posixpath.expanduser(b"~/"), bytes)

        # if home directory == root directory, this test makes no sense
        if posixpath.expanduser("~") != '/':
            self.assertEqual(
                posixpath.expanduser("~") + "/",
                posixpath.expanduser("~/")
            )
            self.assertEqual(
                posixpath.expanduser(b"~") + b"/",
                posixpath.expanduser(b"~/")
            )
        self.assertIsInstance(posixpath.expanduser("~root/"), str)
        self.assertIsInstance(posixpath.expanduser("~foo/"), str)
        self.assertIsInstance(posixpath.expanduser(b"~root/"), bytes)
        self.assertIsInstance(posixpath.expanduser(b"~foo/"), bytes)

        with os_helper.EnvironmentVarGuard() as env:
            # expanduser should fall back to using the password database
            del env['HOME']

            home = pwd.getpwuid(os.getuid()).pw_dir
            # $HOME can end with a trailing /, so strip it (see #17809)
            home = home.rstrip("/") or '/'
            self.assertEqual(posixpath.expanduser("~"), home)

            # bpo-10496: If the HOME environment variable is not set and the
            # user (current identifier or name in the path) doesn't exist in
            # the password database (pwd.getuid() or pwd.getpwnam() fail),
            # expanduser() must return the path unchanged.
            with mock.patch.object(pwd, 'getpwuid', side_effect=KeyError), \
                 mock.patch.object(pwd, 'getpwnam', side_effect=KeyError):
                for path in ('~', '~/.local', '~vstinner/'):
                    self.assertEqual(posixpath.expanduser(path), path)

    @unittest.skipIf(sys.platform == "vxworks",
                     "no home directory on VxWorks")
    def test_expanduser_pwd2(self):
        pwd = import_helper.import_module('pwd')
        for all_entry in pwd.getpwall():
            name = all_entry.pw_name

            # gh-121200: pw_dir can be different between getpwall() and
            # getpwnam(), so use getpwnam() pw_dir as expanduser() does.
            entry = pwd.getpwnam(name)
            home = entry.pw_dir
            home = home.rstrip('/') or '/'

            with self.subTest(all_entry=all_entry, entry=entry):
                self.assertEqual(posixpath.expanduser('~' + name), home)
                self.assertEqual(posixpath.expanduser(os.fsencode('~' + name)),
                                 os.fsencode(home))

    NORMPATH_CASES = [
        ("", "."),
        ("/", "/"),
        ("/.", "/"),
        ("/./", "/"),
        ("/.//.", "/"),
        ("/foo", "/foo"),
        ("/foo/bar", "/foo/bar"),
        ("//", "//"),
        ("///", "/"),
        ("///foo/.//bar//", "/foo/bar"),
        ("///foo/.//bar//.//..//.//baz///", "/foo/baz"),
        ("///..//./foo/.//bar", "/foo/bar"),
        (".", "."),
        (".//.", "."),
        ("..", ".."),
        ("../", ".."),
        ("../foo", "../foo"),
        ("../../foo", "../../foo"),
        ("../foo/../bar", "../bar"),
        ("../../foo/../bar/./baz/boom/..", "../../bar/baz"),
        ("/..", "/"),
        ("/..", "/"),
        ("/../", "/"),
        ("/..//", "/"),
        ("//.", "//"),
        ("//..", "//"),
        ("//...", "//..."),
        ("//../foo", "//foo"),
        ("//../../foo", "//foo"),
        ("/../foo", "/foo"),
        ("/../../foo", "/foo"),
        ("/../foo/../", "/"),
        ("/../foo/../bar", "/bar"),
        ("/../../foo/../bar/./baz/boom/..", "/bar/baz"),
        ("/../../foo/../bar/./baz/boom/.", "/bar/baz/boom"),
        ("foo/../bar/baz", "bar/baz"),
        ("foo/../../bar/baz", "../bar/baz"),
        ("foo/../../../bar/baz", "../../bar/baz"),
        ("foo///../bar/.././../baz/boom", "../baz/boom"),
        ("foo/bar/../..///../../baz/boom", "../../baz/boom"),
        ("/foo/..", "/"),
        ("/foo/../..", "/"),
        ("//foo/..", "//"),
        ("//foo/../..", "//"),
        ("///foo/..", "/"),
        ("///foo/../..", "/"),
        ("////foo/..", "/"),
        ("/////foo/..", "/"),
    ]

    def test_normpath(self):
        for path, expected in self.NORMPATH_CASES:
            with self.subTest(path):
                result = posixpath.normpath(path)
                self.assertEqual(result, expected)

            path = path.encode('utf-8')
            expected = expected.encode('utf-8')
            with self.subTest(path, type=bytes):
                result = posixpath.normpath(path)
                self.assertEqual(result, expected)

    @skip_if_ABSTFN_contains_backslash
    def test_realpath_curdir(self):
        self.assertEqual(realpath('.'), os.getcwd())
        self.assertEqual(realpath('./.'), os.getcwd())
        self.assertEqual(realpath('/'.join(['.'] * 100)), os.getcwd())

        self.assertEqual(realpath(b'.'), os.getcwdb())
        self.assertEqual(realpath(b'./.'), os.getcwdb())
        self.assertEqual(realpath(b'/'.join([b'.'] * 100)), os.getcwdb())

    @skip_if_ABSTFN_contains_backslash
    def test_realpath_pardir(self):
        self.assertEqual(realpath('..'), dirname(os.getcwd()))
        self.assertEqual(realpath('../..'), dirname(dirname(os.getcwd())))
        self.assertEqual(realpath('/'.join(['..'] * 100)), '/')

        self.assertEqual(realpath(b'..'), dirname(os.getcwdb()))
        self.assertEqual(realpath(b'../..'), dirname(dirname(os.getcwdb())))
        self.assertEqual(realpath(b'/'.join([b'..'] * 100)), b'/')

    @os_helper.skip_unless_symlink
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_basic(self):
        # Basic operation.
        try:
            os.symlink(ABSTFN+"1", ABSTFN)
            self.assertEqual(realpath(ABSTFN), ABSTFN+"1")
        finally:
            os_helper.unlink(ABSTFN)

    @os_helper.skip_unless_symlink
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_strict(self):
        # Bug #43757: raise FileNotFoundError in strict mode if we encounter
        # a path that does not exist.
        try:
            os.symlink(ABSTFN+"1", ABSTFN)
            self.assertRaises(FileNotFoundError, realpath, ABSTFN, strict=True)
            self.assertRaises(FileNotFoundError, realpath, ABSTFN + "2", strict=True)
        finally:
            os_helper.unlink(ABSTFN)

    @os_helper.skip_unless_symlink
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_relative(self):
        try:
            os.symlink(posixpath.relpath(ABSTFN+"1"), ABSTFN)
            self.assertEqual(realpath(ABSTFN), ABSTFN+"1")
        finally:
            os_helper.unlink(ABSTFN)

    @os_helper.skip_unless_symlink
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_symlink_loops(self):
        # Bug #930024, return the path unchanged if we get into an infinite
        # symlink loop in non-strict mode (default).
        try:
            os.symlink(ABSTFN, ABSTFN)
            self.assertEqual(realpath(ABSTFN), ABSTFN)

            os.symlink(ABSTFN+"1", ABSTFN+"2")
            os.symlink(ABSTFN+"2", ABSTFN+"1")
            self.assertEqual(realpath(ABSTFN+"1"), ABSTFN+"1")
            self.assertEqual(realpath(ABSTFN+"2"), ABSTFN+"2")

            self.assertEqual(realpath(ABSTFN+"1/x"), ABSTFN+"1/x")
            self.assertEqual(realpath(ABSTFN+"1/.."), dirname(ABSTFN))
            self.assertEqual(realpath(ABSTFN+"1/../x"), dirname(ABSTFN) + "/x")
            os.symlink(ABSTFN+"x", ABSTFN+"y")
            self.assertEqual(realpath(ABSTFN+"1/../" + basename(ABSTFN) + "y"),
                             ABSTFN + "y")
            self.assertEqual(realpath(ABSTFN+"1/../" + basename(ABSTFN) + "1"),
                             ABSTFN + "1")

            os.symlink(basename(ABSTFN) + "a/b", ABSTFN+"a")
            self.assertEqual(realpath(ABSTFN+"a"), ABSTFN+"a/b")

            os.symlink("../" + basename(dirname(ABSTFN)) + "/" +
                       basename(ABSTFN) + "c", ABSTFN+"c")
            self.assertEqual(realpath(ABSTFN+"c"), ABSTFN+"c")

            # Test using relative path as well.
            with os_helper.change_cwd(dirname(ABSTFN)):
                self.assertEqual(realpath(basename(ABSTFN)), ABSTFN)
        finally:
            os_helper.unlink(ABSTFN)
            os_helper.unlink(ABSTFN+"1")
            os_helper.unlink(ABSTFN+"2")
            os_helper.unlink(ABSTFN+"y")
            os_helper.unlink(ABSTFN+"c")
            os_helper.unlink(ABSTFN+"a")

    @os_helper.skip_unless_symlink
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_symlink_loops_strict(self):
        # Bug #43757, raise OSError if we get into an infinite symlink loop in
        # strict mode.
        try:
            os.symlink(ABSTFN, ABSTFN)
            self.assertRaises(OSError, realpath, ABSTFN, strict=True)

            os.symlink(ABSTFN+"1", ABSTFN+"2")
            os.symlink(ABSTFN+"2", ABSTFN+"1")
            self.assertRaises(OSError, realpath, ABSTFN+"1", strict=True)
            self.assertRaises(OSError, realpath, ABSTFN+"2", strict=True)

            self.assertRaises(OSError, realpath, ABSTFN+"1/x", strict=True)
            self.assertRaises(OSError, realpath, ABSTFN+"1/..", strict=True)
            self.assertRaises(OSError, realpath, ABSTFN+"1/../x", strict=True)
            os.symlink(ABSTFN+"x", ABSTFN+"y")
            self.assertRaises(OSError, realpath,
                              ABSTFN+"1/../" + basename(ABSTFN) + "y", strict=True)
            self.assertRaises(OSError, realpath,
                              ABSTFN+"1/../" + basename(ABSTFN) + "1", strict=True)

            os.symlink(basename(ABSTFN) + "a/b", ABSTFN+"a")
            self.assertRaises(OSError, realpath, ABSTFN+"a", strict=True)

            os.symlink("../" + basename(dirname(ABSTFN)) + "/" +
                       basename(ABSTFN) + "c", ABSTFN+"c")
            self.assertRaises(OSError, realpath, ABSTFN+"c", strict=True)

            # Test using relative path as well.
            with os_helper.change_cwd(dirname(ABSTFN)):
                self.assertRaises(OSError, realpath, basename(ABSTFN), strict=True)
        finally:
            os_helper.unlink(ABSTFN)
            os_helper.unlink(ABSTFN+"1")
            os_helper.unlink(ABSTFN+"2")
            os_helper.unlink(ABSTFN+"y")
            os_helper.unlink(ABSTFN+"c")
            os_helper.unlink(ABSTFN+"a")

    @os_helper.skip_unless_symlink
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_repeated_indirect_symlinks(self):
        # Issue #6975.
        try:
            os.mkdir(ABSTFN)
            os.symlink('../' + basename(ABSTFN), ABSTFN + '/self')
            os.symlink('self/self/self', ABSTFN + '/link')
            self.assertEqual(realpath(ABSTFN + '/link'), ABSTFN)
        finally:
            os_helper.unlink(ABSTFN + '/self')
            os_helper.unlink(ABSTFN + '/link')
            safe_rmdir(ABSTFN)

    @os_helper.skip_unless_symlink
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_deep_recursion(self):
        depth = 10
        try:
            os.mkdir(ABSTFN)
            for i in range(depth):
                os.symlink('/'.join(['%d' % i] * 10), ABSTFN + '/%d' % (i + 1))
            os.symlink('.', ABSTFN + '/0')
            self.assertEqual(realpath(ABSTFN + '/%d' % depth), ABSTFN)

            # Test using relative path as well.
            with os_helper.change_cwd(ABSTFN):
                self.assertEqual(realpath('%d' % depth), ABSTFN)
        finally:
            for i in range(depth + 1):
                os_helper.unlink(ABSTFN + '/%d' % i)
            safe_rmdir(ABSTFN)

    @os_helper.skip_unless_symlink
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_resolve_parents(self):
        # We also need to resolve any symlinks in the parents of a relative
        # path passed to realpath. E.g.: current working directory is
        # /usr/doc with 'doc' being a symlink to /usr/share/doc. We call
        # realpath("a"). This should return /usr/share/doc/a/.
        try:
            os.mkdir(ABSTFN)
            os.mkdir(ABSTFN + "/y")
            os.symlink(ABSTFN + "/y", ABSTFN + "/k")

            with os_helper.change_cwd(ABSTFN + "/k"):
                self.assertEqual(realpath("a"), ABSTFN + "/y/a")
        finally:
            os_helper.unlink(ABSTFN + "/k")
            safe_rmdir(ABSTFN + "/y")
            safe_rmdir(ABSTFN)

    @os_helper.skip_unless_symlink
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_resolve_before_normalizing(self):
        # Bug #990669: Symbolic links should be resolved before we
        # normalize the path. E.g.: if we have directories 'a', 'k' and 'y'
        # in the following hierarchy:
        # a/k/y
        #
        # and a symbolic link 'link-y' pointing to 'y' in directory 'a',
        # then realpath("link-y/..") should return 'k', not 'a'.
        try:
            os.mkdir(ABSTFN)
            os.mkdir(ABSTFN + "/k")
            os.mkdir(ABSTFN + "/k/y")
            os.symlink(ABSTFN + "/k/y", ABSTFN + "/link-y")

            # Absolute path.
            self.assertEqual(realpath(ABSTFN + "/link-y/.."), ABSTFN + "/k")
            # Relative path.
            with os_helper.change_cwd(dirname(ABSTFN)):
                self.assertEqual(realpath(basename(ABSTFN) + "/link-y/.."),
                                 ABSTFN + "/k")
        finally:
            os_helper.unlink(ABSTFN + "/link-y")
            safe_rmdir(ABSTFN + "/k/y")
            safe_rmdir(ABSTFN + "/k")
            safe_rmdir(ABSTFN)

    @os_helper.skip_unless_symlink
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_resolve_first(self):
        # Bug #1213894: The first component of the path, if not absolute,
        # must be resolved too.

        try:
            os.mkdir(ABSTFN)
            os.mkdir(ABSTFN + "/k")
            os.symlink(ABSTFN, ABSTFN + "link")
            with os_helper.change_cwd(dirname(ABSTFN)):
                base = basename(ABSTFN)
                self.assertEqual(realpath(base + "link"), ABSTFN)
                self.assertEqual(realpath(base + "link/k"), ABSTFN + "/k")
        finally:
            os_helper.unlink(ABSTFN + "link")
            safe_rmdir(ABSTFN + "/k")
            safe_rmdir(ABSTFN)

    def test_relpath(self):
        (real_getcwd, os.getcwd) = (os.getcwd, lambda: r"/home/user/bar")
        try:
            curdir = os.path.split(os.getcwd())[-1]
            self.assertRaises(ValueError, posixpath.relpath, "")
            self.assertEqual(posixpath.relpath("a"), "a")
            self.assertEqual(posixpath.relpath(posixpath.abspath("a")), "a")
            self.assertEqual(posixpath.relpath("a/b"), "a/b")
            self.assertEqual(posixpath.relpath("../a/b"), "../a/b")
            self.assertEqual(posixpath.relpath("a", "../b"), "../"+curdir+"/a")
            self.assertEqual(posixpath.relpath("a/b", "../c"),
                             "../"+curdir+"/a/b")
            self.assertEqual(posixpath.relpath("a", "b/c"), "../../a")
            self.assertEqual(posixpath.relpath("a", "a"), ".")
            self.assertEqual(posixpath.relpath("/foo/bar/bat", "/x/y/z"), '../../../foo/bar/bat')
            self.assertEqual(posixpath.relpath("/foo/bar/bat", "/foo/bar"), 'bat')
            self.assertEqual(posixpath.relpath("/foo/bar/bat", "/"), 'foo/bar/bat')
            self.assertEqual(posixpath.relpath("/", "/foo/bar/bat"), '../../..')
            self.assertEqual(posixpath.relpath("/foo/bar/bat", "/x"), '../foo/bar/bat')
            self.assertEqual(posixpath.relpath("/x", "/foo/bar/bat"), '../../../x')
            self.assertEqual(posixpath.relpath("/", "/"), '.')
            self.assertEqual(posixpath.relpath("/a", "/a"), '.')
            self.assertEqual(posixpath.relpath("/a/b", "/a/b"), '.')
        finally:
            os.getcwd = real_getcwd

    def test_relpath_bytes(self):
        (real_getcwdb, os.getcwdb) = (os.getcwdb, lambda: br"/home/user/bar")
        try:
            curdir = os.path.split(os.getcwdb())[-1]
            self.assertRaises(ValueError, posixpath.relpath, b"")
            self.assertEqual(posixpath.relpath(b"a"), b"a")
            self.assertEqual(posixpath.relpath(posixpath.abspath(b"a")), b"a")
            self.assertEqual(posixpath.relpath(b"a/b"), b"a/b")
            self.assertEqual(posixpath.relpath(b"../a/b"), b"../a/b")
            self.assertEqual(posixpath.relpath(b"a", b"../b"),
                             b"../"+curdir+b"/a")
            self.assertEqual(posixpath.relpath(b"a/b", b"../c"),
                             b"../"+curdir+b"/a/b")
            self.assertEqual(posixpath.relpath(b"a", b"b/c"), b"../../a")
            self.assertEqual(posixpath.relpath(b"a", b"a"), b".")
            self.assertEqual(posixpath.relpath(b"/foo/bar/bat", b"/x/y/z"), b'../../../foo/bar/bat')
            self.assertEqual(posixpath.relpath(b"/foo/bar/bat", b"/foo/bar"), b'bat')
            self.assertEqual(posixpath.relpath(b"/foo/bar/bat", b"/"), b'foo/bar/bat')
            self.assertEqual(posixpath.relpath(b"/", b"/foo/bar/bat"), b'../../..')
            self.assertEqual(posixpath.relpath(b"/foo/bar/bat", b"/x"), b'../foo/bar/bat')
            self.assertEqual(posixpath.relpath(b"/x", b"/foo/bar/bat"), b'../../../x')
            self.assertEqual(posixpath.relpath(b"/", b"/"), b'.')
            self.assertEqual(posixpath.relpath(b"/a", b"/a"), b'.')
            self.assertEqual(posixpath.relpath(b"/a/b", b"/a/b"), b'.')

            self.assertRaises(TypeError, posixpath.relpath, b"bytes", "str")
            self.assertRaises(TypeError, posixpath.relpath, "str", b"bytes")
        finally:
            os.getcwdb = real_getcwdb

    def test_commonpath(self):
        def check(paths, expected):
            self.assertEqual(posixpath.commonpath(paths), expected)
            self.assertEqual(posixpath.commonpath([os.fsencode(p) for p in paths]),
                             os.fsencode(expected))
        def check_error(exc, paths):
            self.assertRaises(exc, posixpath.commonpath, paths)
            self.assertRaises(exc, posixpath.commonpath,
                              [os.fsencode(p) for p in paths])

        self.assertRaises(ValueError, posixpath.commonpath, [])
        check_error(ValueError, ['/usr', 'usr'])
        check_error(ValueError, ['usr', '/usr'])

        check(['/usr/local'], '/usr/local')
        check(['/usr/local', '/usr/local'], '/usr/local')
        check(['/usr/local/', '/usr/local'], '/usr/local')
        check(['/usr/local/', '/usr/local/'], '/usr/local')
        check(['/usr//local', '//usr/local'], '/usr/local')
        check(['/usr/./local', '/./usr/local'], '/usr/local')
        check(['/', '/dev'], '/')
        check(['/usr', '/dev'], '/')
        check(['/usr/lib/', '/usr/lib/python3'], '/usr/lib')
        check(['/usr/lib/', '/usr/lib64/'], '/usr')

        check(['/usr/lib', '/usr/lib64'], '/usr')
        check(['/usr/lib/', '/usr/lib64'], '/usr')

        check(['spam'], 'spam')
        check(['spam', 'spam'], 'spam')
        check(['spam', 'alot'], '')
        check(['and/jam', 'and/spam'], 'and')
        check(['and//jam', 'and/spam//'], 'and')
        check(['and/./jam', './and/spam'], 'and')
        check(['and/jam', 'and/spam', 'alot'], '')
        check(['and/jam', 'and/spam', 'and'], 'and')

        check([''], '')
        check(['', 'spam/alot'], '')
        check_error(ValueError, ['', '/spam/alot'])

        self.assertRaises(TypeError, posixpath.commonpath,
                          [b'/usr/lib/', '/usr/lib/python3'])
        self.assertRaises(TypeError, posixpath.commonpath,
                          [b'/usr/lib/', 'usr/lib/python3'])
        self.assertRaises(TypeError, posixpath.commonpath,
                          [b'usr/lib/', '/usr/lib/python3'])
        self.assertRaises(TypeError, posixpath.commonpath,
                          ['/usr/lib/', b'/usr/lib/python3'])
        self.assertRaises(TypeError, posixpath.commonpath,
                          ['/usr/lib/', b'usr/lib/python3'])
        self.assertRaises(TypeError, posixpath.commonpath,
                          ['usr/lib/', b'/usr/lib/python3'])


class PosixCommonTest(test_genericpath.CommonTest, unittest.TestCase):
    pathmodule = posixpath
    attributes = ['relpath', 'samefile', 'sameopenfile', 'samestat']


class PathLikeTests(unittest.TestCase):

    path = posixpath

    def setUp(self):
        self.file_name = os_helper.TESTFN
        self.file_path = FakePath(os_helper.TESTFN)
        self.addCleanup(os_helper.unlink, self.file_name)
        with open(self.file_name, 'xb', 0) as file:
            file.write(b"test_posixpath.PathLikeTests")

    def assertPathEqual(self, func):
        self.assertEqual(func(self.file_path), func(self.file_name))

    def test_path_normcase(self):
        self.assertPathEqual(self.path.normcase)

    def test_path_isabs(self):
        self.assertPathEqual(self.path.isabs)

    def test_path_join(self):
        self.assertEqual(self.path.join('a', FakePath('b'), 'c'),
                         self.path.join('a', 'b', 'c'))

    def test_path_split(self):
        self.assertPathEqual(self.path.split)

    def test_path_splitext(self):
        self.assertPathEqual(self.path.splitext)

    def test_path_splitdrive(self):
        self.assertPathEqual(self.path.splitdrive)

    def test_path_splitroot(self):
        self.assertPathEqual(self.path.splitroot)

    def test_path_basename(self):
        self.assertPathEqual(self.path.basename)

    def test_path_dirname(self):
        self.assertPathEqual(self.path.dirname)

    def test_path_islink(self):
        self.assertPathEqual(self.path.islink)

    def test_path_lexists(self):
        self.assertPathEqual(self.path.lexists)

    def test_path_ismount(self):
        self.assertPathEqual(self.path.ismount)

    def test_path_expanduser(self):
        self.assertPathEqual(self.path.expanduser)

    def test_path_expandvars(self):
        self.assertPathEqual(self.path.expandvars)

    def test_path_normpath(self):
        self.assertPathEqual(self.path.normpath)

    def test_path_abspath(self):
        self.assertPathEqual(self.path.abspath)

    def test_path_realpath(self):
        self.assertPathEqual(self.path.realpath)

    def test_path_relpath(self):
        self.assertPathEqual(self.path.relpath)

    def test_path_commonpath(self):
        common_path = self.path.commonpath([self.file_path, self.file_name])
        self.assertEqual(common_path, self.file_name)


if __name__=="__main__":
    unittest.main()
