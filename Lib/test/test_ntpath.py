import ntpath
import os
import sys
import unittest
import warnings
from test.support import os_helper
from test.support import TestFailed
from test.support.os_helper import FakePath
from test import test_genericpath
from tempfile import TemporaryFile


try:
    import nt
except ImportError:
    # Most tests can complete without the nt module,
    # but for those that require it we import here.
    nt = None

try:
    ntpath._getfinalpathname
except AttributeError:
    HAVE_GETFINALPATHNAME = False
else:
    HAVE_GETFINALPATHNAME = True

try:
    import ctypes
except ImportError:
    HAVE_GETSHORTPATHNAME = False
else:
    HAVE_GETSHORTPATHNAME = True
    def _getshortpathname(path):
        GSPN = ctypes.WinDLL("kernel32", use_last_error=True).GetShortPathNameW
        GSPN.argtypes = [ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint32]
        GSPN.restype = ctypes.c_uint32
        result_len = GSPN(path, None, 0)
        if not result_len:
            raise OSError("failed to get short path name 0x{:08X}"
                          .format(ctypes.get_last_error()))
        result = ctypes.create_unicode_buffer(result_len)
        result_len = GSPN(path, result, result_len)
        return result[:result_len]

def _norm(path):
    if isinstance(path, (bytes, str, os.PathLike)):
        return ntpath.normcase(os.fsdecode(path))
    elif hasattr(path, "__iter__"):
        return tuple(ntpath.normcase(os.fsdecode(p)) for p in path)
    return path


def tester(fn, wantResult):
    fn = fn.replace("\\", "\\\\")
    gotResult = eval(fn)
    if wantResult != gotResult and _norm(wantResult) != _norm(gotResult):
        raise TestFailed("%s should return: %s but returned: %s" \
              %(str(fn), str(wantResult), str(gotResult)))

    # then with bytes
    fn = fn.replace("('", "(b'")
    fn = fn.replace('("', '(b"')
    fn = fn.replace("['", "[b'")
    fn = fn.replace('["', '[b"')
    fn = fn.replace(", '", ", b'")
    fn = fn.replace(', "', ', b"')
    fn = os.fsencode(fn).decode('latin1')
    fn = fn.encode('ascii', 'backslashreplace').decode('ascii')
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", DeprecationWarning)
        gotResult = eval(fn)
    if _norm(wantResult) != _norm(gotResult):
        raise TestFailed("%s should return: %s but returned: %s" \
              %(str(fn), str(wantResult), repr(gotResult)))


class NtpathTestCase(unittest.TestCase):
    def assertPathEqual(self, path1, path2):
        if path1 == path2 or _norm(path1) == _norm(path2):
            return
        self.assertEqual(path1, path2)

    def assertPathIn(self, path, pathset):
        self.assertIn(_norm(path), _norm(pathset))


class TestNtpath(NtpathTestCase):
    def test_splitext(self):
        tester('ntpath.splitext("foo.ext")', ('foo', '.ext'))
        tester('ntpath.splitext("/foo/foo.ext")', ('/foo/foo', '.ext'))
        tester('ntpath.splitext(".ext")', ('.ext', ''))
        tester('ntpath.splitext("\\foo.ext\\foo")', ('\\foo.ext\\foo', ''))
        tester('ntpath.splitext("foo.ext\\")', ('foo.ext\\', ''))
        tester('ntpath.splitext("")', ('', ''))
        tester('ntpath.splitext("foo.bar.ext")', ('foo.bar', '.ext'))
        tester('ntpath.splitext("xx/foo.bar.ext")', ('xx/foo.bar', '.ext'))
        tester('ntpath.splitext("xx\\foo.bar.ext")', ('xx\\foo.bar', '.ext'))
        tester('ntpath.splitext("c:a/b\\c.d")', ('c:a/b\\c', '.d'))

    def test_splitdrive(self):
        tester('ntpath.splitdrive("c:\\foo\\bar")',
               ('c:', '\\foo\\bar'))
        tester('ntpath.splitdrive("c:/foo/bar")',
               ('c:', '/foo/bar'))
        tester('ntpath.splitdrive("\\\\conky\\mountpoint\\foo\\bar")',
               ('\\\\conky\\mountpoint', '\\foo\\bar'))
        tester('ntpath.splitdrive("//conky/mountpoint/foo/bar")',
               ('//conky/mountpoint', '/foo/bar'))
        tester('ntpath.splitdrive("\\\\\\conky\\mountpoint\\foo\\bar")',
            ('', '\\\\\\conky\\mountpoint\\foo\\bar'))
        tester('ntpath.splitdrive("///conky/mountpoint/foo/bar")',
            ('', '///conky/mountpoint/foo/bar'))
        tester('ntpath.splitdrive("\\\\conky\\\\mountpoint\\foo\\bar")',
               ('', '\\\\conky\\\\mountpoint\\foo\\bar'))
        tester('ntpath.splitdrive("//conky//mountpoint/foo/bar")',
               ('', '//conky//mountpoint/foo/bar'))
        # Issue #19911: UNC part containing U+0130
        self.assertEqual(ntpath.splitdrive('//conky/MOUNTPOİNT/foo/bar'),
                         ('//conky/MOUNTPOİNT', '/foo/bar'))

    def test_split(self):
        tester('ntpath.split("c:\\foo\\bar")', ('c:\\foo', 'bar'))
        tester('ntpath.split("\\\\conky\\mountpoint\\foo\\bar")',
               ('\\\\conky\\mountpoint\\foo', 'bar'))

        tester('ntpath.split("c:\\")', ('c:\\', ''))
        tester('ntpath.split("\\\\conky\\mountpoint\\")',
               ('\\\\conky\\mountpoint\\', ''))

        tester('ntpath.split("c:/")', ('c:/', ''))
        tester('ntpath.split("//conky/mountpoint/")', ('//conky/mountpoint/', ''))

    def test_isabs(self):
        tester('ntpath.isabs("c:\\")', 1)
        tester('ntpath.isabs("\\\\conky\\mountpoint\\")', 1)
        tester('ntpath.isabs("\\foo")', 1)
        tester('ntpath.isabs("\\foo\\bar")', 1)

    def test_commonprefix(self):
        tester('ntpath.commonprefix(["/home/swenson/spam", "/home/swen/spam"])',
               "/home/swen")
        tester('ntpath.commonprefix(["\\home\\swen\\spam", "\\home\\swen\\eggs"])',
               "\\home\\swen\\")
        tester('ntpath.commonprefix(["/home/swen/spam", "/home/swen/spam"])',
               "/home/swen/spam")

    def test_join(self):
        tester('ntpath.join("")', '')
        tester('ntpath.join("", "", "")', '')
        tester('ntpath.join("a")', 'a')
        tester('ntpath.join("/a")', '/a')
        tester('ntpath.join("\\a")', '\\a')
        tester('ntpath.join("a:")', 'a:')
        tester('ntpath.join("a:", "\\b")', 'a:\\b')
        tester('ntpath.join("a", "\\b")', '\\b')
        tester('ntpath.join("a", "b", "c")', 'a\\b\\c')
        tester('ntpath.join("a\\", "b", "c")', 'a\\b\\c')
        tester('ntpath.join("a", "b\\", "c")', 'a\\b\\c')
        tester('ntpath.join("a", "b", "\\c")', '\\c')
        tester('ntpath.join("d:\\", "\\pleep")', 'd:\\pleep')
        tester('ntpath.join("d:\\", "a", "b")', 'd:\\a\\b')

        tester("ntpath.join('', 'a')", 'a')
        tester("ntpath.join('', '', '', '', 'a')", 'a')
        tester("ntpath.join('a', '')", 'a\\')
        tester("ntpath.join('a', '', '', '', '')", 'a\\')
        tester("ntpath.join('a\\', '')", 'a\\')
        tester("ntpath.join('a\\', '', '', '', '')", 'a\\')
        tester("ntpath.join('a/', '')", 'a/')

        tester("ntpath.join('a/b', 'x/y')", 'a/b\\x/y')
        tester("ntpath.join('/a/b', 'x/y')", '/a/b\\x/y')
        tester("ntpath.join('/a/b/', 'x/y')", '/a/b/x/y')
        tester("ntpath.join('c:', 'x/y')", 'c:x/y')
        tester("ntpath.join('c:a/b', 'x/y')", 'c:a/b\\x/y')
        tester("ntpath.join('c:a/b/', 'x/y')", 'c:a/b/x/y')
        tester("ntpath.join('c:/', 'x/y')", 'c:/x/y')
        tester("ntpath.join('c:/a/b', 'x/y')", 'c:/a/b\\x/y')
        tester("ntpath.join('c:/a/b/', 'x/y')", 'c:/a/b/x/y')
        tester("ntpath.join('//computer/share', 'x/y')", '//computer/share\\x/y')
        tester("ntpath.join('//computer/share/', 'x/y')", '//computer/share/x/y')
        tester("ntpath.join('//computer/share/a/b', 'x/y')", '//computer/share/a/b\\x/y')

        tester("ntpath.join('a/b', '/x/y')", '/x/y')
        tester("ntpath.join('/a/b', '/x/y')", '/x/y')
        tester("ntpath.join('c:', '/x/y')", 'c:/x/y')
        tester("ntpath.join('c:a/b', '/x/y')", 'c:/x/y')
        tester("ntpath.join('c:/', '/x/y')", 'c:/x/y')
        tester("ntpath.join('c:/a/b', '/x/y')", 'c:/x/y')
        tester("ntpath.join('//computer/share', '/x/y')", '//computer/share/x/y')
        tester("ntpath.join('//computer/share/', '/x/y')", '//computer/share/x/y')
        tester("ntpath.join('//computer/share/a', '/x/y')", '//computer/share/x/y')

        tester("ntpath.join('c:', 'C:x/y')", 'C:x/y')
        tester("ntpath.join('c:a/b', 'C:x/y')", 'C:a/b\\x/y')
        tester("ntpath.join('c:/', 'C:x/y')", 'C:/x/y')
        tester("ntpath.join('c:/a/b', 'C:x/y')", 'C:/a/b\\x/y')

        for x in ('', 'a/b', '/a/b', 'c:', 'c:a/b', 'c:/', 'c:/a/b',
                  '//computer/share', '//computer/share/', '//computer/share/a/b'):
            for y in ('d:', 'd:x/y', 'd:/', 'd:/x/y',
                      '//machine/common', '//machine/common/', '//machine/common/x/y'):
                tester("ntpath.join(%r, %r)" % (x, y), y)

        tester("ntpath.join('\\\\computer\\share\\', 'a', 'b')", '\\\\computer\\share\\a\\b')
        tester("ntpath.join('\\\\computer\\share', 'a', 'b')", '\\\\computer\\share\\a\\b')
        tester("ntpath.join('\\\\computer\\share', 'a\\b')", '\\\\computer\\share\\a\\b')
        tester("ntpath.join('//computer/share/', 'a', 'b')", '//computer/share/a\\b')
        tester("ntpath.join('//computer/share', 'a', 'b')", '//computer/share\\a\\b')
        tester("ntpath.join('//computer/share', 'a/b')", '//computer/share\\a/b')

    def test_normpath(self):
        tester("ntpath.normpath('A//////././//.//B')", r'A\B')
        tester("ntpath.normpath('A/./B')", r'A\B')
        tester("ntpath.normpath('A/foo/../B')", r'A\B')
        tester("ntpath.normpath('C:A//B')", r'C:A\B')
        tester("ntpath.normpath('D:A/./B')", r'D:A\B')
        tester("ntpath.normpath('e:A/foo/../B')", r'e:A\B')

        tester("ntpath.normpath('C:///A//B')", r'C:\A\B')
        tester("ntpath.normpath('D:///A/./B')", r'D:\A\B')
        tester("ntpath.normpath('e:///A/foo/../B')", r'e:\A\B')

        tester("ntpath.normpath('..')", r'..')
        tester("ntpath.normpath('.')", r'.')
        tester("ntpath.normpath('')", r'.')
        tester("ntpath.normpath('/')", '\\')
        tester("ntpath.normpath('c:/')", 'c:\\')
        tester("ntpath.normpath('/../.././..')", '\\')
        tester("ntpath.normpath('c:/../../..')", 'c:\\')
        tester("ntpath.normpath('../.././..')", r'..\..\..')
        tester("ntpath.normpath('K:../.././..')", r'K:..\..\..')
        tester("ntpath.normpath('C:////a/b')", r'C:\a\b')
        tester("ntpath.normpath('//machine/share//a/b')", r'\\machine\share\a\b')

        tester("ntpath.normpath('\\\\.\\NUL')", r'\\.\NUL')
        tester("ntpath.normpath('\\\\?\\D:/XY\\Z')", r'\\?\D:/XY\Z')
        tester("ntpath.normpath('handbook/../../Tests/image.png')", r'..\Tests\image.png')
        tester("ntpath.normpath('handbook/../../../Tests/image.png')", r'..\..\Tests\image.png')
        tester("ntpath.normpath('handbook///../a/.././../b/c')", r'..\b\c')
        tester("ntpath.normpath('handbook/a/../..///../../b/c')", r'..\..\b\c')

        tester("ntpath.normpath('//server/share/..')" ,    '\\\\server\\share\\')
        tester("ntpath.normpath('//server/share/../')" ,   '\\\\server\\share\\')
        tester("ntpath.normpath('//server/share/../..')",  '\\\\server\\share\\')
        tester("ntpath.normpath('//server/share/../../')", '\\\\server\\share\\')

    def test_realpath_curdir(self):
        expected = ntpath.normpath(os.getcwd())
        tester("ntpath.realpath('.')", expected)
        tester("ntpath.realpath('./.')", expected)
        tester("ntpath.realpath('/'.join(['.'] * 100))", expected)
        tester("ntpath.realpath('.\\.')", expected)
        tester("ntpath.realpath('\\'.join(['.'] * 100))", expected)

    def test_realpath_pardir(self):
        expected = ntpath.normpath(os.getcwd())
        tester("ntpath.realpath('..')", ntpath.dirname(expected))
        tester("ntpath.realpath('../..')",
               ntpath.dirname(ntpath.dirname(expected)))
        tester("ntpath.realpath('/'.join(['..'] * 50))",
               ntpath.splitdrive(expected)[0] + '\\')
        tester("ntpath.realpath('..\\..')",
               ntpath.dirname(ntpath.dirname(expected)))
        tester("ntpath.realpath('\\'.join(['..'] * 50))",
               ntpath.splitdrive(expected)[0] + '\\')

    @os_helper.skip_unless_symlink
    @unittest.skipUnless(HAVE_GETFINALPATHNAME, 'need _getfinalpathname')
    def test_realpath_basic(self):
        ABSTFN = ntpath.abspath(os_helper.TESTFN)
        open(ABSTFN, "wb").close()
        self.addCleanup(os_helper.unlink, ABSTFN)
        self.addCleanup(os_helper.unlink, ABSTFN + "1")

        os.symlink(ABSTFN, ABSTFN + "1")
        self.assertPathEqual(ntpath.realpath(ABSTFN + "1"), ABSTFN)
        self.assertPathEqual(ntpath.realpath(os.fsencode(ABSTFN + "1")),
                         os.fsencode(ABSTFN))

    @os_helper.skip_unless_symlink
    @unittest.skipUnless(HAVE_GETFINALPATHNAME, 'need _getfinalpathname')
    def test_realpath_strict(self):
        # Bug #43757: raise FileNotFoundError in strict mode if we encounter
        # a path that does not exist.
        ABSTFN = ntpath.abspath(os_helper.TESTFN)
        os.symlink(ABSTFN + "1", ABSTFN)
        self.addCleanup(os_helper.unlink, ABSTFN)
        self.assertRaises(FileNotFoundError, ntpath.realpath, ABSTFN, strict=True)
        self.assertRaises(FileNotFoundError, ntpath.realpath, ABSTFN + "2", strict=True)

    @os_helper.skip_unless_symlink
    @unittest.skipUnless(HAVE_GETFINALPATHNAME, 'need _getfinalpathname')
    def test_realpath_relative(self):
        ABSTFN = ntpath.abspath(os_helper.TESTFN)
        open(ABSTFN, "wb").close()
        self.addCleanup(os_helper.unlink, ABSTFN)
        self.addCleanup(os_helper.unlink, ABSTFN + "1")

        os.symlink(ABSTFN, ntpath.relpath(ABSTFN + "1"))
        self.assertPathEqual(ntpath.realpath(ABSTFN + "1"), ABSTFN)

    @os_helper.skip_unless_symlink
    @unittest.skipUnless(HAVE_GETFINALPATHNAME, 'need _getfinalpathname')
    def test_realpath_broken_symlinks(self):
        ABSTFN = ntpath.abspath(os_helper.TESTFN)
        os.mkdir(ABSTFN)
        self.addCleanup(os_helper.rmtree, ABSTFN)

        with os_helper.change_cwd(ABSTFN):
            os.mkdir("subdir")
            os.chdir("subdir")
            os.symlink(".", "recursive")
            os.symlink("..", "parent")
            os.chdir("..")
            os.symlink(".", "self")
            os.symlink("missing", "broken")
            os.symlink(r"broken\bar", "broken1")
            os.symlink(r"self\self\broken", "broken2")
            os.symlink(r"subdir\parent\subdir\parent\broken", "broken3")
            os.symlink(ABSTFN + r"\broken", "broken4")
            os.symlink(r"recursive\..\broken", "broken5")

            self.assertPathEqual(ntpath.realpath("broken"),
                                 ABSTFN + r"\missing")
            self.assertPathEqual(ntpath.realpath(r"broken\foo"),
                                 ABSTFN + r"\missing\foo")
            # bpo-38453: We no longer recursively resolve segments of relative
            # symlinks that the OS cannot resolve.
            self.assertPathEqual(ntpath.realpath(r"broken1"),
                                 ABSTFN + r"\broken\bar")
            self.assertPathEqual(ntpath.realpath(r"broken1\baz"),
                                 ABSTFN + r"\broken\bar\baz")
            self.assertPathEqual(ntpath.realpath("broken2"),
                                 ABSTFN + r"\self\self\missing")
            self.assertPathEqual(ntpath.realpath("broken3"),
                                 ABSTFN + r"\subdir\parent\subdir\parent\missing")
            self.assertPathEqual(ntpath.realpath("broken4"),
                                 ABSTFN + r"\missing")
            self.assertPathEqual(ntpath.realpath("broken5"),
                                 ABSTFN + r"\missing")

            self.assertPathEqual(ntpath.realpath(b"broken"),
                                 os.fsencode(ABSTFN + r"\missing"))
            self.assertPathEqual(ntpath.realpath(rb"broken\foo"),
                                 os.fsencode(ABSTFN + r"\missing\foo"))
            self.assertPathEqual(ntpath.realpath(rb"broken1"),
                                 os.fsencode(ABSTFN + r"\broken\bar"))
            self.assertPathEqual(ntpath.realpath(rb"broken1\baz"),
                                 os.fsencode(ABSTFN + r"\broken\bar\baz"))
            self.assertPathEqual(ntpath.realpath(b"broken2"),
                                 os.fsencode(ABSTFN + r"\self\self\missing"))
            self.assertPathEqual(ntpath.realpath(rb"broken3"),
                                 os.fsencode(ABSTFN + r"\subdir\parent\subdir\parent\missing"))
            self.assertPathEqual(ntpath.realpath(b"broken4"),
                                 os.fsencode(ABSTFN + r"\missing"))
            self.assertPathEqual(ntpath.realpath(b"broken5"),
                                 os.fsencode(ABSTFN + r"\missing"))

    @os_helper.skip_unless_symlink
    @unittest.skipUnless(HAVE_GETFINALPATHNAME, 'need _getfinalpathname')
    def test_realpath_symlink_loops(self):
        # Symlink loops in non-strict mode are non-deterministic as to which
        # path is returned, but it will always be the fully resolved path of
        # one member of the cycle
        ABSTFN = ntpath.abspath(os_helper.TESTFN)
        self.addCleanup(os_helper.unlink, ABSTFN)
        self.addCleanup(os_helper.unlink, ABSTFN + "1")
        self.addCleanup(os_helper.unlink, ABSTFN + "2")
        self.addCleanup(os_helper.unlink, ABSTFN + "y")
        self.addCleanup(os_helper.unlink, ABSTFN + "c")
        self.addCleanup(os_helper.unlink, ABSTFN + "a")

        os.symlink(ABSTFN, ABSTFN)
        self.assertPathEqual(ntpath.realpath(ABSTFN), ABSTFN)

        os.symlink(ABSTFN + "1", ABSTFN + "2")
        os.symlink(ABSTFN + "2", ABSTFN + "1")
        expected = (ABSTFN + "1", ABSTFN + "2")
        self.assertPathIn(ntpath.realpath(ABSTFN + "1"), expected)
        self.assertPathIn(ntpath.realpath(ABSTFN + "2"), expected)

        self.assertPathIn(ntpath.realpath(ABSTFN + "1\\x"),
                          (ntpath.join(r, "x") for r in expected))
        self.assertPathEqual(ntpath.realpath(ABSTFN + "1\\.."),
                             ntpath.dirname(ABSTFN))
        self.assertPathEqual(ntpath.realpath(ABSTFN + "1\\..\\x"),
                             ntpath.dirname(ABSTFN) + "\\x")
        os.symlink(ABSTFN + "x", ABSTFN + "y")
        self.assertPathEqual(ntpath.realpath(ABSTFN + "1\\..\\"
                                             + ntpath.basename(ABSTFN) + "y"),
                             ABSTFN + "x")
        self.assertPathIn(ntpath.realpath(ABSTFN + "1\\..\\"
                                          + ntpath.basename(ABSTFN) + "1"),
                          expected)

        os.symlink(ntpath.basename(ABSTFN) + "a\\b", ABSTFN + "a")
        self.assertPathEqual(ntpath.realpath(ABSTFN + "a"), ABSTFN + "a")

        os.symlink("..\\" + ntpath.basename(ntpath.dirname(ABSTFN))
                   + "\\" + ntpath.basename(ABSTFN) + "c", ABSTFN + "c")
        self.assertPathEqual(ntpath.realpath(ABSTFN + "c"), ABSTFN + "c")

        # Test using relative path as well.
        self.assertPathEqual(ntpath.realpath(ntpath.basename(ABSTFN)), ABSTFN)

    @os_helper.skip_unless_symlink
    @unittest.skipUnless(HAVE_GETFINALPATHNAME, 'need _getfinalpathname')
    def test_realpath_symlink_loops_strict(self):
        # Symlink loops raise OSError in strict mode
        ABSTFN = ntpath.abspath(os_helper.TESTFN)
        self.addCleanup(os_helper.unlink, ABSTFN)
        self.addCleanup(os_helper.unlink, ABSTFN + "1")
        self.addCleanup(os_helper.unlink, ABSTFN + "2")
        self.addCleanup(os_helper.unlink, ABSTFN + "y")
        self.addCleanup(os_helper.unlink, ABSTFN + "c")
        self.addCleanup(os_helper.unlink, ABSTFN + "a")

        os.symlink(ABSTFN, ABSTFN)
        self.assertRaises(OSError, ntpath.realpath, ABSTFN, strict=True)

        os.symlink(ABSTFN + "1", ABSTFN + "2")
        os.symlink(ABSTFN + "2", ABSTFN + "1")
        self.assertRaises(OSError, ntpath.realpath, ABSTFN + "1", strict=True)
        self.assertRaises(OSError, ntpath.realpath, ABSTFN + "2", strict=True)
        self.assertRaises(OSError, ntpath.realpath, ABSTFN + "1\\x", strict=True)
        # Windows eliminates '..' components before resolving links, so the
        # following call is not expected to raise.
        self.assertPathEqual(ntpath.realpath(ABSTFN + "1\\..", strict=True),
                             ntpath.dirname(ABSTFN))
        self.assertRaises(OSError, ntpath.realpath, ABSTFN + "1\\..\\x", strict=True)
        os.symlink(ABSTFN + "x", ABSTFN + "y")
        self.assertRaises(OSError, ntpath.realpath, ABSTFN + "1\\..\\"
                                             + ntpath.basename(ABSTFN) + "y",
                                             strict=True)
        self.assertRaises(OSError, ntpath.realpath,
                          ABSTFN + "1\\..\\" + ntpath.basename(ABSTFN) + "1",
                          strict=True)

        os.symlink(ntpath.basename(ABSTFN) + "a\\b", ABSTFN + "a")
        self.assertRaises(OSError, ntpath.realpath, ABSTFN + "a", strict=True)

        os.symlink("..\\" + ntpath.basename(ntpath.dirname(ABSTFN))
                   + "\\" + ntpath.basename(ABSTFN) + "c", ABSTFN + "c")
        self.assertRaises(OSError, ntpath.realpath, ABSTFN + "c", strict=True)

        # Test using relative path as well.
        self.assertRaises(OSError, ntpath.realpath, ntpath.basename(ABSTFN),
                          strict=True)

    @os_helper.skip_unless_symlink
    @unittest.skipUnless(HAVE_GETFINALPATHNAME, 'need _getfinalpathname')
    def test_realpath_symlink_prefix(self):
        ABSTFN = ntpath.abspath(os_helper.TESTFN)
        self.addCleanup(os_helper.unlink, ABSTFN + "3")
        self.addCleanup(os_helper.unlink, "\\\\?\\" + ABSTFN + "3.")
        self.addCleanup(os_helper.unlink, ABSTFN + "3link")
        self.addCleanup(os_helper.unlink, ABSTFN + "3.link")

        with open(ABSTFN + "3", "wb") as f:
            f.write(b'0')
        os.symlink(ABSTFN + "3", ABSTFN + "3link")

        with open("\\\\?\\" + ABSTFN + "3.", "wb") as f:
            f.write(b'1')
        os.symlink("\\\\?\\" + ABSTFN + "3.", ABSTFN + "3.link")

        self.assertPathEqual(ntpath.realpath(ABSTFN + "3link"),
                             ABSTFN + "3")
        self.assertPathEqual(ntpath.realpath(ABSTFN + "3.link"),
                             "\\\\?\\" + ABSTFN + "3.")

        # Resolved paths should be usable to open target files
        with open(ntpath.realpath(ABSTFN + "3link"), "rb") as f:
            self.assertEqual(f.read(), b'0')
        with open(ntpath.realpath(ABSTFN + "3.link"), "rb") as f:
            self.assertEqual(f.read(), b'1')

        # When the prefix is included, it is not stripped
        self.assertPathEqual(ntpath.realpath("\\\\?\\" + ABSTFN + "3link"),
                             "\\\\?\\" + ABSTFN + "3")
        self.assertPathEqual(ntpath.realpath("\\\\?\\" + ABSTFN + "3.link"),
                             "\\\\?\\" + ABSTFN + "3.")

    @unittest.skipUnless(HAVE_GETFINALPATHNAME, 'need _getfinalpathname')
    def test_realpath_nul(self):
        tester("ntpath.realpath('NUL')", r'\\.\NUL')

    @unittest.skipUnless(HAVE_GETFINALPATHNAME, 'need _getfinalpathname')
    @unittest.skipUnless(HAVE_GETSHORTPATHNAME, 'need _getshortpathname')
    def test_realpath_cwd(self):
        ABSTFN = ntpath.abspath(os_helper.TESTFN)

        os_helper.unlink(ABSTFN)
        os_helper.rmtree(ABSTFN)
        os.mkdir(ABSTFN)
        self.addCleanup(os_helper.rmtree, ABSTFN)

        test_dir_long = ntpath.join(ABSTFN, "MyVeryLongDirectoryName")
        os.mkdir(test_dir_long)

        test_dir_short = _getshortpathname(test_dir_long)
        test_file_long = ntpath.join(test_dir_long, "file.txt")
        test_file_short = ntpath.join(test_dir_short, "file.txt")

        with open(test_file_long, "wb") as f:
            f.write(b"content")

        self.assertPathEqual(test_file_long, ntpath.realpath(test_file_short))

        with os_helper.change_cwd(test_dir_long):
            self.assertPathEqual(test_file_long, ntpath.realpath("file.txt"))
        with os_helper.change_cwd(test_dir_long.lower()):
            self.assertPathEqual(test_file_long, ntpath.realpath("file.txt"))
        with os_helper.change_cwd(test_dir_short):
            self.assertPathEqual(test_file_long, ntpath.realpath("file.txt"))

    def test_expandvars(self):
        with os_helper.EnvironmentVarGuard() as env:
            env.clear()
            env["foo"] = "bar"
            env["{foo"] = "baz1"
            env["{foo}"] = "baz2"
            tester('ntpath.expandvars("foo")', "foo")
            tester('ntpath.expandvars("$foo bar")', "bar bar")
            tester('ntpath.expandvars("${foo}bar")', "barbar")
            tester('ntpath.expandvars("$[foo]bar")', "$[foo]bar")
            tester('ntpath.expandvars("$bar bar")', "$bar bar")
            tester('ntpath.expandvars("$?bar")', "$?bar")
            tester('ntpath.expandvars("$foo}bar")', "bar}bar")
            tester('ntpath.expandvars("${foo")', "${foo")
            tester('ntpath.expandvars("${{foo}}")', "baz1}")
            tester('ntpath.expandvars("$foo$foo")', "barbar")
            tester('ntpath.expandvars("$bar$bar")', "$bar$bar")
            tester('ntpath.expandvars("%foo% bar")', "bar bar")
            tester('ntpath.expandvars("%foo%bar")', "barbar")
            tester('ntpath.expandvars("%foo%%foo%")', "barbar")
            tester('ntpath.expandvars("%%foo%%foo%foo%")', "%foo%foobar")
            tester('ntpath.expandvars("%?bar%")', "%?bar%")
            tester('ntpath.expandvars("%foo%%bar")', "bar%bar")
            tester('ntpath.expandvars("\'%foo%\'%bar")', "\'%foo%\'%bar")
            tester('ntpath.expandvars("bar\'%foo%")', "bar\'%foo%")

    @unittest.skipUnless(os_helper.FS_NONASCII, 'need os_helper.FS_NONASCII')
    def test_expandvars_nonascii(self):
        def check(value, expected):
            tester('ntpath.expandvars(%r)' % value, expected)
        with os_helper.EnvironmentVarGuard() as env:
            env.clear()
            nonascii = os_helper.FS_NONASCII
            env['spam'] = nonascii
            env[nonascii] = 'ham' + nonascii
            check('$spam bar', '%s bar' % nonascii)
            check('$%s bar' % nonascii, '$%s bar' % nonascii)
            check('${spam}bar', '%sbar' % nonascii)
            check('${%s}bar' % nonascii, 'ham%sbar' % nonascii)
            check('$spam}bar', '%s}bar' % nonascii)
            check('$%s}bar' % nonascii, '$%s}bar' % nonascii)
            check('%spam% bar', '%s bar' % nonascii)
            check('%{}% bar'.format(nonascii), 'ham%s bar' % nonascii)
            check('%spam%bar', '%sbar' % nonascii)
            check('%{}%bar'.format(nonascii), 'ham%sbar' % nonascii)

    def test_expanduser(self):
        tester('ntpath.expanduser("test")', 'test')

        with os_helper.EnvironmentVarGuard() as env:
            env.clear()
            tester('ntpath.expanduser("~test")', '~test')

            env['HOMEDRIVE'] = 'C:\\'
            env['HOMEPATH'] = 'Users\\eric'
            env['USERNAME'] = 'eric'
            tester('ntpath.expanduser("~test")', 'C:\\Users\\test')
            tester('ntpath.expanduser("~")', 'C:\\Users\\eric')

            del env['HOMEDRIVE']
            tester('ntpath.expanduser("~test")', 'Users\\test')
            tester('ntpath.expanduser("~")', 'Users\\eric')

            env.clear()
            env['USERPROFILE'] = 'C:\\Users\\eric'
            env['USERNAME'] = 'eric'
            tester('ntpath.expanduser("~test")', 'C:\\Users\\test')
            tester('ntpath.expanduser("~")', 'C:\\Users\\eric')
            tester('ntpath.expanduser("~test\\foo\\bar")',
                   'C:\\Users\\test\\foo\\bar')
            tester('ntpath.expanduser("~test/foo/bar")',
                   'C:\\Users\\test/foo/bar')
            tester('ntpath.expanduser("~\\foo\\bar")',
                   'C:\\Users\\eric\\foo\\bar')
            tester('ntpath.expanduser("~/foo/bar")',
                   'C:\\Users\\eric/foo/bar')

            # bpo-36264: ignore `HOME` when set on windows
            env.clear()
            env['HOME'] = 'F:\\'
            env['USERPROFILE'] = 'C:\\Users\\eric'
            env['USERNAME'] = 'eric'
            tester('ntpath.expanduser("~test")', 'C:\\Users\\test')
            tester('ntpath.expanduser("~")', 'C:\\Users\\eric')

            # bpo-39899: don't guess another user's home directory if
            # `%USERNAME% != basename(%USERPROFILE%)`
            env.clear()
            env['USERPROFILE'] = 'C:\\Users\\eric'
            env['USERNAME'] = 'idle'
            tester('ntpath.expanduser("~test")', '~test')
            tester('ntpath.expanduser("~")', 'C:\\Users\\eric')



    @unittest.skipUnless(nt, "abspath requires 'nt' module")
    def test_abspath(self):
        tester('ntpath.abspath("C:\\")', "C:\\")
        with os_helper.temp_cwd(os_helper.TESTFN) as cwd_dir: # bpo-31047
            tester('ntpath.abspath("")', cwd_dir)
            tester('ntpath.abspath(" ")', cwd_dir + "\\ ")
            tester('ntpath.abspath("?")', cwd_dir + "\\?")
            drive, _ = ntpath.splitdrive(cwd_dir)
            tester('ntpath.abspath("/abc/")', drive + "\\abc")

    def test_relpath(self):
        tester('ntpath.relpath("a")', 'a')
        tester('ntpath.relpath(ntpath.abspath("a"))', 'a')
        tester('ntpath.relpath("a/b")', 'a\\b')
        tester('ntpath.relpath("../a/b")', '..\\a\\b')
        with os_helper.temp_cwd(os_helper.TESTFN) as cwd_dir:
            currentdir = ntpath.basename(cwd_dir)
            tester('ntpath.relpath("a", "../b")', '..\\'+currentdir+'\\a')
            tester('ntpath.relpath("a/b", "../c")', '..\\'+currentdir+'\\a\\b')
        tester('ntpath.relpath("a", "b/c")', '..\\..\\a')
        tester('ntpath.relpath("c:/foo/bar/bat", "c:/x/y")', '..\\..\\foo\\bar\\bat')
        tester('ntpath.relpath("//conky/mountpoint/a", "//conky/mountpoint/b/c")', '..\\..\\a')
        tester('ntpath.relpath("a", "a")', '.')
        tester('ntpath.relpath("/foo/bar/bat", "/x/y/z")', '..\\..\\..\\foo\\bar\\bat')
        tester('ntpath.relpath("/foo/bar/bat", "/foo/bar")', 'bat')
        tester('ntpath.relpath("/foo/bar/bat", "/")', 'foo\\bar\\bat')
        tester('ntpath.relpath("/", "/foo/bar/bat")', '..\\..\\..')
        tester('ntpath.relpath("/foo/bar/bat", "/x")', '..\\foo\\bar\\bat')
        tester('ntpath.relpath("/x", "/foo/bar/bat")', '..\\..\\..\\x')
        tester('ntpath.relpath("/", "/")', '.')
        tester('ntpath.relpath("/a", "/a")', '.')
        tester('ntpath.relpath("/a/b", "/a/b")', '.')
        tester('ntpath.relpath("c:/foo", "C:/FOO")', '.')

    def test_commonpath(self):
        def check(paths, expected):
            tester(('ntpath.commonpath(%r)' % paths).replace('\\\\', '\\'),
                   expected)
        def check_error(exc, paths):
            self.assertRaises(exc, ntpath.commonpath, paths)
            self.assertRaises(exc, ntpath.commonpath,
                              [os.fsencode(p) for p in paths])

        self.assertRaises(ValueError, ntpath.commonpath, [])
        check_error(ValueError, ['C:\\Program Files', 'Program Files'])
        check_error(ValueError, ['C:\\Program Files', 'C:Program Files'])
        check_error(ValueError, ['\\Program Files', 'Program Files'])
        check_error(ValueError, ['Program Files', 'C:\\Program Files'])
        check(['C:\\Program Files'], 'C:\\Program Files')
        check(['C:\\Program Files', 'C:\\Program Files'], 'C:\\Program Files')
        check(['C:\\Program Files\\', 'C:\\Program Files'],
              'C:\\Program Files')
        check(['C:\\Program Files\\', 'C:\\Program Files\\'],
              'C:\\Program Files')
        check(['C:\\\\Program Files', 'C:\\Program Files\\\\'],
              'C:\\Program Files')
        check(['C:\\.\\Program Files', 'C:\\Program Files\\.'],
              'C:\\Program Files')
        check(['C:\\', 'C:\\bin'], 'C:\\')
        check(['C:\\Program Files', 'C:\\bin'], 'C:\\')
        check(['C:\\Program Files', 'C:\\Program Files\\Bar'],
              'C:\\Program Files')
        check(['C:\\Program Files\\Foo', 'C:\\Program Files\\Bar'],
              'C:\\Program Files')
        check(['C:\\Program Files', 'C:\\Projects'], 'C:\\')
        check(['C:\\Program Files\\', 'C:\\Projects'], 'C:\\')

        check(['C:\\Program Files\\Foo', 'C:/Program Files/Bar'],
              'C:\\Program Files')
        check(['C:\\Program Files\\Foo', 'c:/program files/bar'],
              'C:\\Program Files')
        check(['c:/program files/bar', 'C:\\Program Files\\Foo'],
              'c:\\program files')

        check_error(ValueError, ['C:\\Program Files', 'D:\\Program Files'])

        check(['spam'], 'spam')
        check(['spam', 'spam'], 'spam')
        check(['spam', 'alot'], '')
        check(['and\\jam', 'and\\spam'], 'and')
        check(['and\\\\jam', 'and\\spam\\\\'], 'and')
        check(['and\\.\\jam', '.\\and\\spam'], 'and')
        check(['and\\jam', 'and\\spam', 'alot'], '')
        check(['and\\jam', 'and\\spam', 'and'], 'and')
        check(['C:and\\jam', 'C:and\\spam'], 'C:and')

        check([''], '')
        check(['', 'spam\\alot'], '')
        check_error(ValueError, ['', '\\spam\\alot'])

        self.assertRaises(TypeError, ntpath.commonpath,
                          [b'C:\\Program Files', 'C:\\Program Files\\Foo'])
        self.assertRaises(TypeError, ntpath.commonpath,
                          [b'C:\\Program Files', 'Program Files\\Foo'])
        self.assertRaises(TypeError, ntpath.commonpath,
                          [b'Program Files', 'C:\\Program Files\\Foo'])
        self.assertRaises(TypeError, ntpath.commonpath,
                          ['C:\\Program Files', b'C:\\Program Files\\Foo'])
        self.assertRaises(TypeError, ntpath.commonpath,
                          ['C:\\Program Files', b'Program Files\\Foo'])
        self.assertRaises(TypeError, ntpath.commonpath,
                          ['Program Files', b'C:\\Program Files\\Foo'])

    def test_sameopenfile(self):
        with TemporaryFile() as tf1, TemporaryFile() as tf2:
            # Make sure the same file is really the same
            self.assertTrue(ntpath.sameopenfile(tf1.fileno(), tf1.fileno()))
            # Make sure different files are really different
            self.assertFalse(ntpath.sameopenfile(tf1.fileno(), tf2.fileno()))
            # Make sure invalid values don't cause issues on win32
            if sys.platform == "win32":
                with self.assertRaises(OSError):
                    # Invalid file descriptors shouldn't display assert
                    # dialogs (#4804)
                    ntpath.sameopenfile(-1, -1)

    def test_ismount(self):
        self.assertTrue(ntpath.ismount("c:\\"))
        self.assertTrue(ntpath.ismount("C:\\"))
        self.assertTrue(ntpath.ismount("c:/"))
        self.assertTrue(ntpath.ismount("C:/"))
        self.assertTrue(ntpath.ismount("\\\\.\\c:\\"))
        self.assertTrue(ntpath.ismount("\\\\.\\C:\\"))

        self.assertTrue(ntpath.ismount(b"c:\\"))
        self.assertTrue(ntpath.ismount(b"C:\\"))
        self.assertTrue(ntpath.ismount(b"c:/"))
        self.assertTrue(ntpath.ismount(b"C:/"))
        self.assertTrue(ntpath.ismount(b"\\\\.\\c:\\"))
        self.assertTrue(ntpath.ismount(b"\\\\.\\C:\\"))

        with os_helper.temp_dir() as d:
            self.assertFalse(ntpath.ismount(d))

        if sys.platform == "win32":
            #
            # Make sure the current folder isn't the root folder
            # (or any other volume root). The drive-relative
            # locations below cannot then refer to mount points
            #
            test_cwd = os.getenv("SystemRoot")
            drive, path = ntpath.splitdrive(test_cwd)
            with os_helper.change_cwd(test_cwd):
                self.assertFalse(ntpath.ismount(drive.lower()))
                self.assertFalse(ntpath.ismount(drive.upper()))

            self.assertTrue(ntpath.ismount("\\\\localhost\\c$"))
            self.assertTrue(ntpath.ismount("\\\\localhost\\c$\\"))

            self.assertTrue(ntpath.ismount(b"\\\\localhost\\c$"))
            self.assertTrue(ntpath.ismount(b"\\\\localhost\\c$\\"))

    def assertEqualCI(self, s1, s2):
        """Assert that two strings are equal ignoring case differences."""
        self.assertEqual(s1.lower(), s2.lower())

    @unittest.skipUnless(nt, "OS helpers require 'nt' module")
    def test_nt_helpers(self):
        # Trivial validation that the helpers do not break, and support both
        # unicode and bytes (UTF-8) paths

        executable = nt._getfinalpathname(sys.executable)

        for path in executable, os.fsencode(executable):
            volume_path = nt._getvolumepathname(path)
            path_drive = ntpath.splitdrive(path)[0]
            volume_path_drive = ntpath.splitdrive(volume_path)[0]
            self.assertEqualCI(path_drive, volume_path_drive)

        cap, free = nt._getdiskusage(sys.exec_prefix)
        self.assertGreater(cap, 0)
        self.assertGreater(free, 0)
        b_cap, b_free = nt._getdiskusage(sys.exec_prefix.encode())
        # Free space may change, so only test the capacity is equal
        self.assertEqual(b_cap, cap)
        self.assertGreater(b_free, 0)

        for path in [sys.prefix, sys.executable]:
            final_path = nt._getfinalpathname(path)
            self.assertIsInstance(final_path, str)
            self.assertGreater(len(final_path), 0)

            b_final_path = nt._getfinalpathname(path.encode())
            self.assertIsInstance(b_final_path, bytes)
            self.assertGreater(len(b_final_path), 0)

class NtCommonTest(test_genericpath.CommonTest, unittest.TestCase):
    pathmodule = ntpath
    attributes = ['relpath']


class PathLikeTests(NtpathTestCase):

    path = ntpath

    def setUp(self):
        self.file_name = os_helper.TESTFN
        self.file_path = FakePath(os_helper.TESTFN)
        self.addCleanup(os_helper.unlink, self.file_name)
        with open(self.file_name, 'xb', 0) as file:
            file.write(b"test_ntpath.PathLikeTests")

    def _check_function(self, func):
        self.assertPathEqual(func(self.file_path), func(self.file_name))

    def test_path_normcase(self):
        self._check_function(self.path.normcase)

    def test_path_isabs(self):
        self._check_function(self.path.isabs)

    def test_path_join(self):
        self.assertEqual(self.path.join('a', FakePath('b'), 'c'),
                         self.path.join('a', 'b', 'c'))

    def test_path_split(self):
        self._check_function(self.path.split)

    def test_path_splitext(self):
        self._check_function(self.path.splitext)

    def test_path_splitdrive(self):
        self._check_function(self.path.splitdrive)

    def test_path_basename(self):
        self._check_function(self.path.basename)

    def test_path_dirname(self):
        self._check_function(self.path.dirname)

    def test_path_islink(self):
        self._check_function(self.path.islink)

    def test_path_lexists(self):
        self._check_function(self.path.lexists)

    def test_path_ismount(self):
        self._check_function(self.path.ismount)

    def test_path_expanduser(self):
        self._check_function(self.path.expanduser)

    def test_path_expandvars(self):
        self._check_function(self.path.expandvars)

    def test_path_normpath(self):
        self._check_function(self.path.normpath)

    def test_path_abspath(self):
        self._check_function(self.path.abspath)

    def test_path_realpath(self):
        self._check_function(self.path.realpath)

    def test_path_relpath(self):
        self._check_function(self.path.relpath)

    def test_path_commonpath(self):
        common_path = self.path.commonpath([self.file_path, self.file_name])
        self.assertPathEqual(common_path, self.file_name)

    def test_path_isdir(self):
        self._check_function(self.path.isdir)


if __name__ == "__main__":
    unittest.main()
