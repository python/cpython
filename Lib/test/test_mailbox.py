import mailbox
import os
import test_support

# cleanup the turds of some of the other tests. :(
try:
    os.unlink(test_support.TESTFN)
except OSError:
    pass

# create a new maildir mailbox to work with:
curdir = os.path.join(test_support.TESTFN, "cur")
newdir = os.path.join(test_support.TESTFN, "new")
try:
    os.mkdir(test_support.TESTFN)
    os.mkdir(curdir)
    os.mkdir(newdir)

    # Test for regression on bug #117490:
    # http://sourceforge.net/bugs/?func=detailbug&bug_id=117490&group_id=5470
    # Make sure the boxes attribute actually gets set.
    mbox = mailbox.Maildir(test_support.TESTFN)
    mbox.boxes
    print "newly created maildir contains", len(mbox.boxes), "messages"

    # XXX We still need more tests!

finally:
    try: os.rmdir(newdir)
    except OSError: pass
    try: os.rmdir(curdir)
    except OSError: pass
    try: os.rmdir(test_support.TESTFN)
    except OSError: pass
