import posixpath

errors = 0

def tester(fn, wantResult):
    gotResult = eval(fn)
    if wantResult != gotResult:
        print "error!"
        print "evaluated: " + str(fn)
        print "should be: " + str(wantResult)
        print " returned: " + str(gotResult)
        print ""
        global errors
        errors = errors + 1

tester('posixpath.splitdrive("/foo/bar")', ('', '/foo/bar'))

tester('posixpath.split("/foo/bar")', ('/foo', 'bar'))
tester('posixpath.split("/")', ('/', ''))
tester('posixpath.split("foo")', ('', 'foo'))

tester('posixpath.splitext("foo.ext")', ('foo', '.ext'))
tester('posixpath.splitext("/foo/foo.ext")', ('/foo/foo', '.ext'))

tester('posixpath.isabs("/")', 1)
tester('posixpath.isabs("/foo")', 1)
tester('posixpath.isabs("/foo/bar")', 1)
tester('posixpath.isabs("foo/bar")', 0)

tester('posixpath.commonprefix(["/home/swenson/spam", "/home/swen/spam"])',
       "/home/swen")
tester('posixpath.commonprefix(["/home/swen/spam", "/home/swen/eggs"])',
       "/home/swen/")
tester('posixpath.commonprefix(["/home/swen/spam", "/home/swen/spam"])',
       "/home/swen/spam")

if errors:
    print str(errors) + " errors."
else:
    print "No errors.  Thank your lucky stars."
