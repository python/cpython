import dospath
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

tester('dospath.splitdrive("c:\\foo\\bar")', ('c:', '\\foo\\bar'))
tester('dospath.splitdrive("c:/foo/bar")', ('c:', '/foo/bar'))

tester('dospath.split("c:\\foo\\bar")', ('c:\\foo', 'bar'))
tester('dospath.split("\\\\conky\\mountpoint\\foo\\bar")', ('\\\\conky\\mountpoint\\foo', 'bar'))

tester('dospath.split("c:\\")', ('c:\\', ''))
tester('dospath.split("\\\\conky\\mountpoint\\")', ('\\\\conky\\mountpoint', ''))

tester('dospath.split("c:/")', ('c:/', ''))
tester('dospath.split("//conky/mountpoint/")', ('//conky/mountpoint', ''))

tester('dospath.isabs("c:\\")', 1)
tester('dospath.isabs("\\\\conky\\mountpoint\\")', 1)
tester('dospath.isabs("\\foo")', 1)
tester('dospath.isabs("\\foo\\bar")', 1)

tester('dospath.abspath("C:\\")', "C:\\")

tester('dospath.commonprefix(["/home/swenson/spam", "/home/swen/spam"])',
       "/home/swen")
tester('dospath.commonprefix(["\\home\\swen\\spam", "\\home\\swen\\eggs"])',
       "\\home\\swen\\")
tester('dospath.commonprefix(["/home/swen/spam", "/home/swen/spam"])',
       "/home/swen/spam")

if errors:
	print str(errors) + " errors."
else:
	print "No errors.  Thank your lucky stars."

