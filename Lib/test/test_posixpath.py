import unittest
from test import support

import posixpath, os
from posixpath import realpath, abspath, join, dirname, basename, relpath

# An absolute path to a temporary filename for testing. We can't rely on TESTFN
# being an absolute path, so we need this.

ABSTFN = abspath(support.TESTFN)

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

    def assertIs(self, a, b):
        self.assertTrue(a is b)

    def test_normcase(self):
        # Check that normcase() is idempotent
        p = "FoO/./BaR"
        self.assertEqual(p, posixpath.normcase(p))

        p = b"FoO/./BaR"
        self.assertEqual(p, posixpath.normcase(p))

        self.assertRaises(TypeError, posixpath.normcase)

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

        self.assertRaises(TypeError, posixpath.join)
        self.assertRaises(TypeError, posixpath.join, b"bytes", "str")
        self.assertRaises(TypeError, posixpath.join, "str", b"bytes")

    def test_splitdrive(self):
        self.assertEqual(posixpath.splitdrive("/foo/bar"), ("", "/foo/bar"))
        self.assertEqual(posixpath.splitdrive(b"/foo/bar"), (b"", b"/foo/bar"))

        self.assertRaises(TypeError, posixpath.splitdrive)

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

        self.assertRaises(TypeError, posixpath.split)

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
        self.assertRaises(TypeError, posixpath.splitext)

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

        self.assertRaises(TypeError, posixpath.isabs)

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

        self.assertRaises(TypeError, posixpath.basename)

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

        self.assertRaises(TypeError, posixpath.dirname)

    def test_commonprefix(self):
        self.assertEqual(
            posixpath.commonprefix([]),
            ""
        )
        self.assertEqual(
            posixpath.commonprefix(["/home/swenson/spam", "/home/swen/spam"]),
            "/home/swen"
        )
        self.assertEqual(
            posixpath.commonprefix(["/home/swen/spam", "/home/swen/eggs"]),
            "/home/swen/"
        )
        self.assertEqual(
            posixpath.commonprefix(["/home/swen/spam", "/home/swen/spam"]),
            "/home/swen/spam"
        )

        self.assertEqual(
            posixpath.commonprefix([b"/home/swenson/spam", b"/home/swen/spam"]),
            b"/home/swen"
        )
        self.assertEqual(
            posixpath.commonprefix([b"/home/swen/spam", b"/home/swen/eggs"]),
            b"/home/swen/"
        )
        self.assertEqual(
            posixpath.commonprefix([b"/home/swen/spam", b"/home/swen/spam"]),
            b"/home/swen/spam"
        )

        testlist = ['', 'abc', 'Xbcd', 'Xb', 'XY', 'abcd', 'aXc', 'abd', 'ab', 'aX', 'abcX']
        for s1 in testlist:
            for s2 in testlist:
                p = posixpath.commonprefix([s1, s2])
                self.assertTrue(s1.startswith(p))
                self.assertTrue(s2.startswith(p))
                if s1 != s2:
                    n = len(p)
                    self.assertNotEqual(s1[n:n+1], s2[n:n+1])

    def test_getsize(self):
        f = open(support.TESTFN, "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertEqual(posixpath.getsize(support.TESTFN), 3)
        finally:
            if not f.closed:
                f.close()

    def test_time(self):
        f = open(support.TESTFN, "wb")
        try:
            f.write(b"foo")
            f.close()
            f = open(support.TESTFN, "ab")
            f.write(b"bar")
            f.close()
            f = open(support.TESTFN, "rb")
            d = f.read()
            f.close()
            self.assertEqual(d, b"foobar")

            self.assertTrue(
                posixpath.getctime(support.TESTFN) <=
                posixpath.getmtime(support.TESTFN)
            )
        finally:
            if not f.closed:
                f.close()

    def test_islink(self):
        self.assertIs(posixpath.islink(support.TESTFN + "1"), False)
        f = open(support.TESTFN + "1", "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertIs(posixpath.islink(support.TESTFN + "1"), False)
            if hasattr(os, "symlink"):
                os.symlink(support.TESTFN + "1", support.TESTFN + "2")
                self.assertIs(posixpath.islink(support.TESTFN + "2"), True)
                os.remove(support.TESTFN + "1")
                self.assertIs(posixpath.islink(support.TESTFN + "2"), True)
                self.assertIs(posixpath.exists(support.TESTFN + "2"), False)
                self.assertIs(posixpath.lexists(support.TESTFN + "2"), True)
        finally:
            if not f.close():
                f.close()

        self.assertRaises(TypeError, posixpath.islink)

    def test_exists(self):
        self.assertIs(posixpath.exists(support.TESTFN), False)
        f = open(support.TESTFN, "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertIs(posixpath.exists(support.TESTFN), True)
            self.assertIs(posixpath.lexists(support.TESTFN), True)
        finally:
            if not f.close():
                f.close()

        self.assertRaises(TypeError, posixpath.exists)

    def test_isdir(self):
        self.assertIs(posixpath.isdir(support.TESTFN), False)
        f = open(support.TESTFN, "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertIs(posixpath.isdir(support.TESTFN), False)
            os.remove(support.TESTFN)
            os.mkdir(support.TESTFN)
            self.assertIs(posixpath.isdir(support.TESTFN), True)
            os.rmdir(support.TESTFN)
        finally:
            if not f.close():
                f.close()

        self.assertRaises(TypeError, posixpath.isdir)

    def test_isfile(self):
        self.assertIs(posixpath.isfile(support.TESTFN), False)
        f = open(support.TESTFN, "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertIs(posixpath.isfile(support.TESTFN), True)
            os.remove(support.TESTFN)
            os.mkdir(support.TESTFN)
            self.assertIs(posixpath.isfile(support.TESTFN), False)
            os.rmdir(support.TESTFN)
        finally:
            if not f.close():
                f.close()

        self.assertRaises(TypeError, posixpath.isdir)

    def test_samefile(self):
        f = open(support.TESTFN + "1", "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertIs(
                posixpath.samefile(
                    support.TESTFN + "1",
                    support.TESTFN + "1"
                ),
                True
            )
            # If we don't have links, assume that os.stat doesn't return resonable
            # inode information and thus, that samefile() doesn't work
            if hasattr(os, "symlink"):
                os.symlink(
                    support.TESTFN + "1",
                    support.TESTFN + "2"
                )
                self.assertIs(
                    posixpath.samefile(
                        support.TESTFN + "1",
                        support.TESTFN + "2"
                    ),
                    True
                )
                os.remove(support.TESTFN + "2")
                f = open(support.TESTFN + "2", "wb")
                f.write(b"bar")
                f.close()
                self.assertIs(
                    posixpath.samefile(
                        support.TESTFN + "1",
                        support.TESTFN + "2"
                    ),
                    False
                )
        finally:
            if not f.close():
                f.close()

        self.assertRaises(TypeError, posixpath.samefile)

    def test_samestat(self):
        f = open(support.TESTFN + "1", "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertIs(
                posixpath.samestat(
                    os.stat(support.TESTFN + "1"),
                    os.stat(support.TESTFN + "1")
                ),
                True
            )
            # If we don't have links, assume that os.stat() doesn't return resonable
            # inode information and thus, that samefile() doesn't work
            if hasattr(os, "symlink"):
                if hasattr(os, "symlink"):
                    os.symlink(support.TESTFN + "1", support.TESTFN + "2")
                    self.assertIs(
                        posixpath.samestat(
                            os.stat(support.TESTFN + "1"),
                            os.stat(support.TESTFN + "2")
                        ),
                        True
                    )
                    os.remove(support.TESTFN + "2")
                f = open(support.TESTFN + "2", "wb")
                f.write(b"bar")
                f.close()
                self.assertIs(
                    posixpath.samestat(
                        os.stat(support.TESTFN + "1"),
                        os.stat(support.TESTFN + "2")
                    ),
                    False
                )
        finally:
            if not f.close():
                f.close()

        self.assertRaises(TypeError, posixpath.samestat)

    def test_ismount(self):
        self.assertIs(posixpath.ismount("/"), True)

        self.assertRaises(TypeError, posixpath.ismount)

    def test_expanduser(self):
        self.assertEqual(posixpath.expanduser("foo"), "foo")
        self.assertEqual(posixpath.expanduser(b"foo"), b"foo")
        try:
            import pwd
        except ImportError:
            pass
        else:
            self.assertTrue(isinstance(posixpath.expanduser("~/"), str))
            self.assertTrue(isinstance(posixpath.expanduser(b"~/"), bytes))
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
            self.assertTrue(isinstance(posixpath.expanduser("~root/"), str))
            self.assertTrue(isinstance(posixpath.expanduser("~foo/"), str))
            self.assertTrue(isinstance(posixpath.expanduser(b"~root/"), bytes))
            self.assertTrue(isinstance(posixpath.expanduser(b"~foo/"), bytes))

            with support.EnvironmentVarGuard() as env:
                env['HOME'] = '/'
                self.assertEqual(posixpath.expanduser("~"), "/")

        self.assertRaises(TypeError, posixpath.expanduser)

    def test_expandvars(self):
        with support.EnvironmentVarGuard() as env:
            env.clear()
            env["foo"] = "bar"
            env["{foo"] = "baz1"
            env["{foo}"] = "baz2"
            self.assertEqual(posixpath.expandvars("foo"), "foo")
            self.assertEqual(posixpath.expandvars("$foo bar"), "bar bar")
            self.assertEqual(posixpath.expandvars("${foo}bar"), "barbar")
            self.assertEqual(posixpath.expandvars("$[foo]bar"), "$[foo]bar")
            self.assertEqual(posixpath.expandvars("$bar bar"), "$bar bar")
            self.assertEqual(posixpath.expandvars("$?bar"), "$?bar")
            self.assertEqual(posixpath.expandvars("${foo}bar"), "barbar")
            self.assertEqual(posixpath.expandvars("$foo}bar"), "bar}bar")
            self.assertEqual(posixpath.expandvars("${foo"), "${foo")
            self.assertEqual(posixpath.expandvars("${{foo}}"), "baz1}")
            self.assertEqual(posixpath.expandvars("$foo$foo"), "barbar")
            self.assertEqual(posixpath.expandvars("$bar$bar"), "$bar$bar")

            self.assertEqual(posixpath.expandvars(b"foo"), b"foo")
            self.assertEqual(posixpath.expandvars(b"$foo bar"), b"bar bar")
            self.assertEqual(posixpath.expandvars(b"${foo}bar"), b"barbar")
            self.assertEqual(posixpath.expandvars(b"$[foo]bar"), b"$[foo]bar")
            self.assertEqual(posixpath.expandvars(b"$bar bar"), b"$bar bar")
            self.assertEqual(posixpath.expandvars(b"$?bar"), b"$?bar")
            self.assertEqual(posixpath.expandvars(b"${foo}bar"), b"barbar")
            self.assertEqual(posixpath.expandvars(b"$foo}bar"), b"bar}bar")
            self.assertEqual(posixpath.expandvars(b"${foo"), b"${foo")
            self.assertEqual(posixpath.expandvars(b"${{foo}}"), b"baz1}")
            self.assertEqual(posixpath.expandvars(b"$foo$foo"), b"barbar")
            self.assertEqual(posixpath.expandvars(b"$bar$bar"), b"$bar$bar")
            self.assertRaises(TypeError, posixpath.expandvars)

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

        self.assertRaises(TypeError, posixpath.normpath)

    def test_abspath(self):
        self.assertTrue("foo" in posixpath.abspath("foo"))
        self.assertTrue(b"foo" in posixpath.abspath(b"foo"))

        self.assertRaises(TypeError, posixpath.abspath)

    def test_realpath(self):
        self.assertTrue("foo" in realpath("foo"))
        self.assertTrue(b"foo" in realpath(b"foo"))
        self.assertRaises(TypeError, posixpath.realpath)

    if hasattr(os, "symlink"):
        def test_realpath_basic(self):
            # Basic operation.
            try:
                os.symlink(ABSTFN+"1", ABSTFN)
                self.assertEqual(realpath(ABSTFN), ABSTFN+"1")
            finally:
                support.unlink(ABSTFN)

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

                # Test using relative path as well.
                os.chdir(dirname(ABSTFN))
                self.assertEqual(realpath(basename(ABSTFN)), ABSTFN)
            finally:
                os.chdir(old_path)
                support.unlink(ABSTFN)
                support.unlink(ABSTFN+"1")
                support.unlink(ABSTFN+"2")

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
                self.assertEqual(realpath(basename(ABSTFN) + "/link-y/.."), ABSTFN + "/k")
            finally:
                os.chdir(old_path)
                support.unlink(ABSTFN + "/link-y")
                safe_rmdir(ABSTFN + "/k/y")
                safe_rmdir(ABSTFN + "/k")
                safe_rmdir(ABSTFN)

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

def test_main():
    support.run_unittest(PosixPathTest)

if __name__=="__main__":
    test_main()
