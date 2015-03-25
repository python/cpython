# coding: utf-8
import ntpath
import os
import sys
from test.test_support import TestFailed
from test import test_support, test_genericpath
import unittest

def tester0(fn, wantResult):
    gotResult = eval(fn)
    if wantResult != gotResult:
        raise TestFailed, "%s should return: %r but returned: %r" \
              %(fn, wantResult, gotResult)

def tester(fn, wantResult):
    fn = fn.replace("\\", "\\\\")
    tester0(fn, wantResult)


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
        self.assertEqual(ntpath.splitdrive(u'//conky/MOUNTPOİNT/foo/bar'),
                         (u'//conky/MOUNTPOİNT', '/foo/bar'))
        self.assertEqual(ntpath.splitdrive("//"), ("", "//"))

    def test_splitunc(self):
        tester('ntpath.splitunc("c:\\foo\\bar")',
               ('', 'c:\\foo\\bar'))
        tester('ntpath.splitunc("c:/foo/bar")',
               ('', 'c:/foo/bar'))
        tester('ntpath.splitunc("\\\\conky\\mountpoint\\foo\\bar")',
               ('\\\\conky\\mountpoint', '\\foo\\bar'))
        tester('ntpath.splitunc("//conky/mountpoint/foo/bar")',
               ('//conky/mountpoint', '/foo/bar'))
        tester('ntpath.splitunc("\\\\\\conky\\mountpoint\\foo\\bar")',
               ('', '\\\\\\conky\\mountpoint\\foo\\bar'))
        tester('ntpath.splitunc("///conky/mountpoint/foo/bar")',
               ('', '///conky/mountpoint/foo/bar'))
        tester('ntpath.splitunc("\\\\conky\\\\mountpoint\\foo\\bar")',
               ('', '\\\\conky\\\\mountpoint\\foo\\bar'))
        tester('ntpath.splitunc("//conky//mountpoint/foo/bar")',
               ('', '//conky//mountpoint/foo/bar'))
        if test_support.have_unicode:
            self.assertEqual(ntpath.splitunc(u'//conky/MOUNTPO%cNT/foo/bar' % 0x0130),
                             (u'//conky/MOUNTPO%cNT' % 0x0130, u'/foo/bar'))

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

        for x in ('', 'a/b', '/a/b', 'c:', 'c:a/b', 'c:/', 'c:/a/b'):
            for y in ('d:', 'd:x/y', 'd:/', 'd:/x/y'):
                tester("ntpath.join(%r, %r)" % (x, y), y)

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
        with test_support.EnvironmentVarGuard() as env:
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

    @unittest.skipUnless(test_support.FS_NONASCII, 'need test_support.FS_NONASCII')
    def test_expandvars_nonascii(self):
        encoding = sys.getfilesystemencoding()
        def check(value, expected):
            tester0("ntpath.expandvars(%r)" % value, expected)
            tester0("ntpath.expandvars(%r)" % value.decode(encoding),
                    expected.decode(encoding))
        with test_support.EnvironmentVarGuard() as env:
            env.clear()
            unonascii = test_support.FS_NONASCII
            snonascii = unonascii.encode(encoding)
            env['spam'] = snonascii
            env[snonascii] = 'ham' + snonascii
            check('$spam bar', '%s bar' % snonascii)
            check('$%s bar' % snonascii, '$%s bar' % snonascii)
            check('${spam}bar', '%sbar' % snonascii)
            check('${%s}bar' % snonascii, 'ham%sbar' % snonascii)
            check('$spam}bar', '%s}bar' % snonascii)
            check('$%s}bar' % snonascii, '$%s}bar' % snonascii)
            check('%spam% bar', '%s bar' % snonascii)
            check('%{}% bar'.format(snonascii), 'ham%s bar' % snonascii)
            check('%spam%bar', '%sbar' % snonascii)
            check('%{}%bar'.format(snonascii), 'ham%sbar' % snonascii)

    def test_expanduser(self):
        tester('ntpath.expanduser("test")', 'test')

        with test_support.EnvironmentVarGuard() as env:
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

    def test_abspath(self):
        # ntpath.abspath() can only be used on a system with the "nt" module
        # (reasonably), so we protect this test with "import nt".  This allows
        # the rest of the tests for the ntpath module to be run to completion
        # on any platform, since most of the module is intended to be usable
        # from any platform.
        # XXX this needs more tests
        try:
            import nt
        except ImportError:
            # check that the function is there even if we are not on Windows
            ntpath.abspath
        else:
            tester('ntpath.abspath("C:\\")', "C:\\")

    def test_relpath(self):
        tester('ntpath.relpath("a")', 'a')
        tester('ntpath.relpath(os.path.abspath("a"))', 'a')
        tester('ntpath.relpath("a/b")', 'a\\b')
        tester('ntpath.relpath("../a/b")', '..\\a\\b')
        with test_support.temp_cwd(test_support.TESTFN) as cwd_dir:
            currentdir = os.path.basename(cwd_dir)
            tester('ntpath.relpath("a", "../b")', '..\\'+currentdir+'\\a')
            tester('ntpath.relpath("a/b", "../c")', '..\\'+currentdir+'\\a\\b')
        tester('ntpath.relpath("a", "b/c")', '..\\..\\a')
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


class NtCommonTest(test_genericpath.CommonTest):
    pathmodule = ntpath
    attributes = ['relpath', 'splitunc']


def test_main():
    test_support.run_unittest(TestNtpath, NtCommonTest)


if __name__ == "__main__":
    unittest.main()
