import itertools
import os
import posixpath
import sys
import unittest
import warnings
from posixpath import realpath, abspath, dirname, basename
from test import support, test_genericpath

try:
    import posix
except ImportError:
    posix = None

# An absolute path to a temporary filename for testing. We can't rely on TESTFN
# being an absolute path, so we need this.

ABSTFN = abspath(support.TESTFN)

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
            support.unlink(support.TESTFN + suffix)
            safe_rmdir(support.TESTFN + suffix)

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

        def check_error_msg(list_of_args, msg):
            """Check posixpath.join raises friendly TypeErrors."""
            for args in (item for perm in list_of_args
                              for item in itertools.permutations(perm)):
                with self.assertRaises(TypeError) as cm:
                    posixpath.join(*args)
                self.assertEqual(msg, cm.exception.args[0])

        check_error_msg([[b'bytes', 'str'], [bytearray(b'bytes'), 'str']],
                        "Can't mix strings and bytes in path components.")
        # regression, see #15377
        with self.assertRaises(TypeError) as cm:
            posixpath.join(None, 'str')
        self.assertNotEqual("Can't mix strings and bytes in path components.",
                            cm.exception.args[0])

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
        self.assertIs(posixpath.islink(support.TESTFN + "1"), False)
        self.assertIs(posixpath.lexists(support.TESTFN + "2"), False)
        f = open(support.TESTFN + "1", "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertIs(posixpath.islink(support.TESTFN + "1"), False)
            if support.can_symlink():
                os.symlink(support.TESTFN + "1", support.TESTFN + "2")
                self.assertIs(posixpath.islink(support.TESTFN + "2"), True)
                os.remove(support.TESTFN + "1")
                self.assertIs(posixpath.islink(support.TESTFN + "2"), True)
                self.assertIs(posixpath.exists(support.TESTFN + "2"), False)
                self.assertIs(posixpath.lexists(support.TESTFN + "2"), True)
        finally:
            if not f.close():
                f.close()

    def test_ismount(self):
        self.assertIs(posixpath.ismount("/"), True)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertIs(posixpath.ismount(b"/"), True)

    def test_ismount_non_existent(self):
        # Non-existent mountpoint.
        self.assertIs(posixpath.ismount(ABSTFN), False)
        try:
            os.mkdir(ABSTFN)
            self.assertIs(posixpath.ismount(ABSTFN), False)
        finally:
            safe_rmdir(ABSTFN)

    @unittest.skipUnless(support.can_symlink(),
                         "Test requires symlink support")
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

    def test_expanduser(self):
        self.assertEqual(posixpath.expanduser("foo"), "foo")
        self.assertEqual(posixpath.expanduser(b"foo"), b"foo")
        try:
            import pwd
        except ImportError:
            pass
        else:
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

            with support.EnvironmentVarGuard() as env:
                env['HOME'] = '/'
                self.assertEqual(posixpath.expanduser("~"), "/")
                self.assertEqual(posixpath.expanduser("~/foo"), "/foo")
                # expanduser should fall back to using the password database
                del env['HOME']
                home = pwd.getpwuid(os.getuid()).pw_dir
                self.assertEqual(posixpath.expanduser("~"), home)

    def test_normpath(self):
        self.assertEqual(posixpath.normpath(""), ".")
        self.assertEqual(posixpath.normpath("/"), "/")
        self.assertEqual(posixpath.normpath("//"), "//")
        self.assertEqual(posixpath.normpath("///"), "/")
        self.assertEqual(posixpath.normpath("///foo/.//bar//"), "/foo/bar")
        self.assertEqual(posixpath.normpath("///foo/.//bar//.//..//.//baz"),
                         "/foo/baz")
        self.assertEqual(posixpath.normpath("///..//./foo/.//bar"), "/foo/bar")

        self.assertEqual(posixpath.normpath(b""), b".")
        self.assertEqual(posixpath.normpath(b"/"), b"/")
        self.assertEqual(posixpath.normpath(b"//"), b"//")
        self.assertEqual(posixpath.normpath(b"///"), b"/")
        self.assertEqual(posixpath.normpath(b"///foo/.//bar//"), b"/foo/bar")
        self.assertEqual(posixpath.normpath(b"///foo/.//bar//.//..//.//baz"),
                         b"/foo/baz")
        self.assertEqual(posixpath.normpath(b"///..//./foo/.//bar"),
                         b"/foo/bar")

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

    @unittest.skipUnless(hasattr(os, "symlink"),
                         "Missing symlink implementation")
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_basic(self):
        # Basic operation.
        try:
            os.symlink(ABSTFN+"1", ABSTFN)
            self.assertEqual(realpath(ABSTFN), ABSTFN+"1")
        finally:
            support.unlink(ABSTFN)

    @unittest.skipUnless(hasattr(os, "symlink"),
                         "Missing symlink implementation")
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_relative(self):
        try:
            os.symlink(posixpath.relpath(ABSTFN+"1"), ABSTFN)
            self.assertEqual(realpath(ABSTFN), ABSTFN+"1")
        finally:
            support.unlink(ABSTFN)

    @unittest.skipUnless(hasattr(os, "symlink"),
                         "Missing symlink implementation")
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_symlink_loops(self):
        # Bug #930024, return the path unchanged if we get into an infinite
        # symlink loop.
        try:
            old_path = abspath('.')
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
            os.chdir(dirname(ABSTFN))
            self.assertEqual(realpath(basename(ABSTFN)), ABSTFN)
        finally:
            os.chdir(old_path)
            support.unlink(ABSTFN)
            support.unlink(ABSTFN+"1")
            support.unlink(ABSTFN+"2")
            support.unlink(ABSTFN+"y")
            support.unlink(ABSTFN+"c")

    @unittest.skipUnless(hasattr(os, "symlink"),
                         "Missing symlink implementation")
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_repeated_indirect_symlinks(self):
        # Issue #6975.
        try:
            os.mkdir(ABSTFN)
            os.symlink('../' + basename(ABSTFN), ABSTFN + '/self')
            os.symlink('self/self/self', ABSTFN + '/link')
            self.assertEqual(realpath(ABSTFN + '/link'), ABSTFN)
        finally:
            support.unlink(ABSTFN + '/self')
            support.unlink(ABSTFN + '/link')
            safe_rmdir(ABSTFN)

    @unittest.skipUnless(hasattr(os, "symlink"),
                         "Missing symlink implementation")
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_deep_recursion(self):
        depth = 10
        old_path = abspath('.')
        try:
            os.mkdir(ABSTFN)
            for i in range(depth):
                os.symlink('/'.join(['%d' % i] * 10), ABSTFN + '/%d' % (i + 1))
            os.symlink('.', ABSTFN + '/0')
            self.assertEqual(realpath(ABSTFN + '/%d' % depth), ABSTFN)

            # Test using relative path as well.
            os.chdir(ABSTFN)
            self.assertEqual(realpath('%d' % depth), ABSTFN)
        finally:
            os.chdir(old_path)
            for i in range(depth + 1):
                support.unlink(ABSTFN + '/%d' % i)
            safe_rmdir(ABSTFN)

    @unittest.skipUnless(hasattr(os, "symlink"),
                         "Missing symlink implementation")
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_resolve_parents(self):
        # We also need to resolve any symlinks in the parents of a relative
        # path passed to realpath. E.g.: current working directory is
        # /usr/doc with 'doc' being a symlink to /usr/share/doc. We call
        # realpath("a"). This should return /usr/share/doc/a/.
        try:
            old_path = abspath('.')
            os.mkdir(ABSTFN)
            os.mkdir(ABSTFN + "/y")
            os.symlink(ABSTFN + "/y", ABSTFN + "/k")

            os.chdir(ABSTFN + "/k")
            self.assertEqual(realpath("a"), ABSTFN + "/y/a")
        finally:
            os.chdir(old_path)
            support.unlink(ABSTFN + "/k")
            safe_rmdir(ABSTFN + "/y")
            safe_rmdir(ABSTFN)

    @unittest.skipUnless(hasattr(os, "symlink"),
                         "Missing symlink implementation")
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
            old_path = abspath('.')
            os.mkdir(ABSTFN)
            os.mkdir(ABSTFN + "/k")
            os.mkdir(ABSTFN + "/k/y")
            os.symlink(ABSTFN + "/k/y", ABSTFN + "/link-y")

            # Absolute path.
            self.assertEqual(realpath(ABSTFN + "/link-y/.."), ABSTFN + "/k")
            # Relative path.
            os.chdir(dirname(ABSTFN))
            self.assertEqual(realpath(basename(ABSTFN) + "/link-y/.."),
                             ABSTFN + "/k")
        finally:
            os.chdir(old_path)
            support.unlink(ABSTFN + "/link-y")
            safe_rmdir(ABSTFN + "/k/y")
            safe_rmdir(ABSTFN + "/k")
            safe_rmdir(ABSTFN)

    @unittest.skipUnless(hasattr(os, "symlink"),
                         "Missing symlink implementation")
    @skip_if_ABSTFN_contains_backslash
    def test_realpath_resolve_first(self):
        # Bug #1213894: The first component of the path, if not absolute,
        # must be resolved too.

        try:
            old_path = abspath('.')
            os.mkdir(ABSTFN)
            os.mkdir(ABSTFN + "/k")
            os.symlink(ABSTFN, ABSTFN + "link")
            os.chdir(dirname(ABSTFN))

            base = basename(ABSTFN)
            self.assertEqual(realpath(base + "link"), ABSTFN)
            self.assertEqual(realpath(base + "link/k"), ABSTFN + "/k")
        finally:
            os.chdir(old_path)
            support.unlink(ABSTFN + "link")
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


class PosixCommonTest(test_genericpath.CommonTest, unittest.TestCase):
    pathmodule = posixpath
    attributes = ['relpath', 'samefile', 'sameopenfile', 'samestat']


if __name__=="__main__":
    unittest.main()
