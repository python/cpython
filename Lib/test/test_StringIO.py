# Tests StringIO and cStringIO

import string

def do_test(module):
    s = (string.letters+'\n')*5
    f = module.StringIO(s)
    print f.read(10)
    print f.readline()
    print len(f.readlines(60))

    f = module.StringIO()
    f.write(s)
    f.seek(10)
    f.truncate()
    print `f.getvalue()`
    # This test fails for cStringIO; reported as SourceForge bug #115531;
    # please uncomment this test when that bug is fixed.
    # http://sourceforge.net/bugs/?func=detailbug&bug_id=115531&group_id=5470
##     f.seek(0)
##     f.truncate(5)
##     print `f.getvalue()`

    # This test fails for cStringIO; reported as SourceForge bug #115530;
    # please uncomment this test when that bug is fixed.
    # http://sourceforge.net/bugs/?func=detailbug&bug_id=115530&group_id=5470
##     try:
##         f.write("frobnitz")
##     except ValueError, e:
##         print "Caught expected ValueError writing to closed StringIO:"
##         print e
##     else:
##         print "Failed to catch ValueError writing to closed StringIO."

# Don't bother testing cStringIO without
import StringIO, cStringIO
do_test(StringIO)
do_test(cStringIO)
