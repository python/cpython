import ntpath
from test.test_support import verbose, TestFailed
import os

errors = 0

def tester(fn, wantResult):
    global errors
    fn = fn.replace("\\", "\\\\")
    gotResult = eval(fn)
    if wantResult != gotResult:
        print "error!"
        print "evaluated: " + str(fn)
        print "should be: " + str(wantResult)
        print " returned: " + str(gotResult)
        print ""
        errors = errors + 1

tester('ntpath.splitext("foo.ext")', ('foo', '.ext'))
tester('ntpath.splitext("/foo/foo.ext")', ('/foo/foo', '.ext'))
tester('ntpath.splitext(".ext")', ('', '.ext'))
tester('ntpath.splitext("\\foo.ext\\foo")', ('\\foo.ext\\foo', ''))
tester('ntpath.splitext("foo.ext\\")', ('foo.ext\\', ''))
tester('ntpath.splitext("")', ('', ''))
tester('ntpath.splitext("foo.bar.ext")', ('foo.bar', '.ext'))
tester('ntpath.splitext("xx/foo.bar.ext")', ('xx/foo.bar', '.ext'))
tester('ntpath.splitext("xx\\foo.bar.ext")', ('xx\\foo.bar', '.ext'))

tester('ntpath.splitdrive("c:\\foo\\bar")',
       ('c:', '\\foo\\bar'))
tester('ntpath.splitunc("\\\\conky\\mountpoint\\foo\\bar")',
       ('\\\\conky\\mountpoint', '\\foo\\bar'))
tester('ntpath.splitdrive("c:/foo/bar")',
       ('c:', '/foo/bar'))
tester('ntpath.splitunc("//conky/mountpoint/foo/bar")',
       ('//conky/mountpoint', '/foo/bar'))

tester('ntpath.split("c:\\foo\\bar")', ('c:\\foo', 'bar'))
tester('ntpath.split("\\\\conky\\mountpoint\\foo\\bar")',
       ('\\\\conky\\mountpoint\\foo', 'bar'))

tester('ntpath.split("c:\\")', ('c:\\', ''))
tester('ntpath.split("\\\\conky\\mountpoint\\")',
       ('\\\\conky\\mountpoint', ''))

tester('ntpath.split("c:/")', ('c:/', ''))
tester('ntpath.split("//conky/mountpoint/")', ('//conky/mountpoint', ''))

tester('ntpath.isabs("c:\\")', 1)
tester('ntpath.isabs("\\\\conky\\mountpoint\\")', 1)
tester('ntpath.isabs("\\foo")', 1)
tester('ntpath.isabs("\\foo\\bar")', 1)

tester('ntpath.commonprefix(["/home/swenson/spam", "/home/swen/spam"])',
       "/home/swen")
tester('ntpath.commonprefix(["\\home\\swen\\spam", "\\home\\swen\\eggs"])',
       "\\home\\swen\\")
tester('ntpath.commonprefix(["/home/swen/spam", "/home/swen/spam"])',
       "/home/swen/spam")

tester('ntpath.join("")', '')
tester('ntpath.join("", "", "")', '')
tester('ntpath.join("a")', 'a')
tester('ntpath.join("/a")', '/a')
tester('ntpath.join("\\a")', '\\a')
tester('ntpath.join("a:")', 'a:')
tester('ntpath.join("a:", "b")', 'a:b')
tester('ntpath.join("a:", "/b")', 'a:/b')
tester('ntpath.join("a:", "\\b")', 'a:\\b')
tester('ntpath.join("a", "/b")', '/b')
tester('ntpath.join("a", "\\b")', '\\b')
tester('ntpath.join("a", "b", "c")', 'a\\b\\c')
tester('ntpath.join("a\\", "b", "c")', 'a\\b\\c')
tester('ntpath.join("a", "b\\", "c")', 'a\\b\\c')
tester('ntpath.join("a", "b", "\\c")', '\\c')
tester('ntpath.join("d:\\", "\\pleep")', 'd:\\pleep')
tester('ntpath.join("d:\\", "a", "b")', 'd:\\a\\b')
tester("ntpath.join('c:', '/a')", 'c:/a')
tester("ntpath.join('c:/', '/a')", 'c:/a')
tester("ntpath.join('c:/a', '/b')", '/b')
tester("ntpath.join('c:', 'd:/')", 'd:/')
tester("ntpath.join('c:/', 'd:/')", 'd:/')
tester("ntpath.join('c:/', 'd:/a/b')", 'd:/a/b')

tester("ntpath.join('')", '')
tester("ntpath.join('', '', '', '', '')", '')
tester("ntpath.join('a')", 'a')
tester("ntpath.join('', 'a')", 'a')
tester("ntpath.join('', '', '', '', 'a')", 'a')
tester("ntpath.join('a', '')", 'a\\')
tester("ntpath.join('a', '', '', '', '')", 'a\\')
tester("ntpath.join('a\\', '')", 'a\\')
tester("ntpath.join('a\\', '', '', '', '')", 'a\\')

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

# ntpath.abspath() can only be used on a system with the "nt" module
# (reasonably), so we protect this test with "import nt".  This allows
# the rest of the tests for the ntpath module to be run to completion
# on any platform, since most of the module is intended to be usable
# from any platform.
try:
    import nt
except ImportError:
    pass
else:
    tester('ntpath.abspath("C:\\")', "C:\\")

if errors:
    raise TestFailed(str(errors) + " errors.")
elif verbose:
    print "No errors.  Thank your lucky stars."
