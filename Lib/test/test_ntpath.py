import ntpath
from test_support import verbose, TestFailed
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

tester('ntpath.abspath("C:\\")', "C:\\")

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

# Next 3 seem dubious, and especially the 3rd, but normpath is possibly
# trying to leave UNC paths alone without actually knowing anything about
# them.
tester("ntpath.normpath('C:///A//B')", r'C:\\\A\B')
tester("ntpath.normpath('D:///A/./B')", r'D:\\\A\B')
tester("ntpath.normpath('e:///A/foo/../B')", r'e:\\\A\B')

tester("ntpath.normpath('..')", r'..')
tester("ntpath.normpath('.')", r'.')
tester("ntpath.normpath('')", r'.')
tester("ntpath.normpath('/')", '\\')
tester("ntpath.normpath('c:/')", 'c:\\')
tester("ntpath.normpath('/../.././..')", '\\')
tester("ntpath.normpath('c:/../../..')", 'c:\\')
tester("ntpath.normpath('../.././..')", r'..\..\..')
tester("ntpath.normpath('K:../.././..')", r'K:..\..\..')

if errors:
    raise TestFailed(str(errors) + " errors.")
elif verbose:
    print "No errors.  Thank your lucky stars."
