import ntpath
import string
import os

errors = 0

def tester(fn, wantResult):
	fn = string.replace(fn, "\\", "\\\\")
	gotResult = eval(fn)
	if wantResult != gotResult:
		print "error!"
		print "evaluated: " + str(fn)
		print "should be: " + str(wantResult)
		print " returned: " + str(gotResult)
		print ""
		global errors
		errors = errors + 1

tester('ntpath.splitdrive("c:\\foo\\bar")', ('c:', '\\foo\\bar'))
tester('ntpath.splitunc("\\\\conky\\mountpoint\\foo\\bar")', ('\\\\conky\\mountpoint', '\\foo\\bar'))
tester('ntpath.splitdrive("c:/foo/bar")', ('c:', '/foo/bar'))
tester('ntpath.splitunc("//conky/mountpoint/foo/bar")', ('//conky/mountpoint', '/foo/bar'))

tester('ntpath.split("c:\\foo\\bar")', ('c:\\foo', 'bar'))
tester('ntpath.split("\\\\conky\\mountpoint\\foo\\bar")', ('\\\\conky\\mountpoint\\foo', 'bar'))

tester('ntpath.split("c:\\")', ('c:\\', ''))
tester('ntpath.split("\\\\conky\\mountpoint\\")', ('\\\\conky\\mountpoint', ''))

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

if errors:
	print str(errors) + " errors."
else:
	print "No errors.  Thank your lucky stars."

