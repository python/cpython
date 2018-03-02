import ntpath
import os
import sys
import unittest
import warnings
from test.support import TestFailed, FakePath
from test import support, test_genericpath
from tempfile import TemporaryFile

try:
    import nt
except ImportError:
    # Most tests can complete without the nt module,
    # but for those that require it we import here.
    nt = None

def tester(fn, wantResult):
    fn = fn.replace("\\", "\\\\")
    gotResult = eval(fn)
    if wantResult != gotResult:
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
    if isinstance(wantResult, str):
        wantResult = os.fsencode(wantResult)
    elif isinstance(wantResult, tuple):
        wantResult = tuple(os.fsencode(r) for r in wantResult)

    gotResult = eval(fn)
    if wantResult != gotResult:
        raise TestFailed("%s should return: %s but returned: %s" \
              %(str(fn), str(wantResult), repr(gotResult)))


class TestNtpath(unittest.TestCase):
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

    def test_expandvars(self):
        with support.EnvironmentVarGuard() as env:
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

    @unittest.skipUnless(support.FS_NONASCII, 'need support.FS_NONASCII')
    def test_expandvars_nonascii(self):
        def check(value, expected):
            tester('ntpath.expandvars(%r)' % value, expected)
        with support.EnvironmentVarGuard() as env:
            env.clear()
            nonascii = support.FS_NONASCII
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

        with support.EnvironmentVarGuard() as env:
            env.clear()
            tester('ntpath.expanduser("~test")', '~test')

            env['HOMEPATH'] = 'eric\\idle'
            env['HOMEDRIVE'] = 'C:\\'
            tester('ntpath.expanduser("~test")', 'C:\\eric\\test')
            tester('ntpath.expanduser("~")', 'C:\\eric\\idle')

            del env['HOMEDRIVE']
            tester('ntpath.expanduser("~test")', 'eric\\test')
            tester('ntpath.expanduser("~")', 'eric\\idle')

            env.clear()
            env['USERPROFILE'] = 'C:\\eric\\idle'
            tester('ntpath.expanduser("~test")', 'C:\\eric\\test')
            tester('ntpath.expanduser("~")', 'C:\\eric\\idle')

            env.clear()
            env['HOME'] = 'C:\\idle\\eric'
            tester('ntpath.expanduser("~test")', 'C:\\idle\\test')
            tester('ntpath.expanduser("~")', 'C:\\idle\\eric')

            tester('ntpath.expanduser("~test\\foo\\bar")',
                   'C:\\idle\\test\\foo\\bar')
            tester('ntpath.expanduser("~test/foo/bar")',
                   'C:\\idle\\test/foo/bar')
            tester('ntpath.expanduser("~\\foo\\bar")',
                   'C:\\idle\\eric\\foo\\bar')
            tester('ntpath.expanduser("~/foo/bar")',
                   'C:\\idle\\eric/foo/bar')

    @unittest.skipUnless(nt, "abspath requires 'nt' module")
    def test_abspath(self):
        tester('ntpath.abspath("C:\\")', "C:\\")

    def test_relpath(self):
        tester('ntpath.relpath("a")', 'a')
        tester('ntpath.relpath(os.path.abspath("a"))', 'a')
        tester('ntpath.relpath("a/b")', 'a\\b')
        tester('ntpath.relpath("../a/b")', '..\\a\\b')
        with support.temp_cwd(support.TESTFN) as cwd_dir:
            currentdir = os.path.basename(cwd_dir)
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

        with support.temp_dir() as d:
            self.assertFalse(ntpath.ismount(d))

        if sys.platform == "win32":
            #
            # Make sure the current folder isn't the root folder
            # (or any other volume root). The drive-relative
            # locations below cannot then refer to mount points
            #
            drive, path = ntpath.splitdrive(sys.executable)
            with support.change_cwd(os.path.dirname(sys.executable)):
                self.assertFalse(ntpath.ismount(drive.lower()))
                self.assertFalse(ntpath.ismount(drive.upper()))

            self.assertTrue(ntpath.ismount("\\\\localhost\\c$"))
            self.assertTrue(ntpath.ismount("\\\\localhost\\c$\\"))

            self.assertTrue(ntpath.ismount(b"\\\\localhost\\c$"))
            self.assertTrue(ntpath.ismount(b"\\\\localhost\\c$\\"))

    @unittest.skipUnless(nt, "OS helpers require 'nt' module")
    def test_nt_helpers(self):
        # Trivial validation that the helpers do not break, and support both
        # unicode and bytes (UTF-8) paths

        drive, path = ntpath.splitdrive(sys.executable)
        drive = drive.rstrip(ntpath.sep) + ntpath.sep
        self.assertEqual(drive, nt._getvolumepathname(sys.executable))
        self.assertEqual(drive.encode(),
                         nt._getvolumepathname(sys.executable.encode()))

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


class PathLikeTests(unittest.TestCase):

    path = ntpath

    def setUp(self):
        self.file_name = support.TESTFN.lower()
        self.file_path = FakePath(support.TESTFN)
        self.addCleanup(support.unlink, self.file_name)
        with open(self.file_name, 'xb', 0) as file:
            file.write(b"test_ntpath.PathLikeTests")

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

    def test_path_isdir(self):
        self.assertPathEqual(self.path.isdir)


if __name__ == "__main__":
    unittest.main()
