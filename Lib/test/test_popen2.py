#! /usr/bin/env python
"""Test script for popen2.py
   Christian Tismer
"""

import os
import sys
from test_support import TestSkipped

# popen2 contains its own testing routine
# which is especially useful to see if open files
# like stdin can be read successfully by a forked
# subprocess.

def main():
    print "Test popen2 module:"
    if sys.platform[:4] == 'beos' and __name__ != '__main__':
        #  Locks get messed up or something.  Generally we're supposed
        #  to avoid mixing "posix" fork & exec with native threads, and
        #  they may be right about that after all.
        raise TestSkipped, "popen2() doesn't work during import on BeOS"
    try:
        from os import popen
    except ImportError:
        # if we don't have os.popen, check that
        # we have os.fork.  if not, skip the test
        # (by raising an ImportError)
        from os import fork
    import popen2
    popen2._test()


def _test():
    # same test as popen2._test(), but using the os.popen*() API
    print "Testing os module:"
    import popen2
    cmd  = "cat"
    teststr = "ab cd\n"
    if os.name == "nt":
        cmd = "more"
    # "more" doesn't act the same way across Windows flavors,
    # sometimes adding an extra newline at the start or the
    # end.  So we strip whitespace off both ends for comparison.
    expected = teststr.strip()
    print "testing popen2..."
    w, r = os.popen2(cmd)
    w.write(teststr)
    w.close()
    got = r.read()
    if got.strip() != expected:
        raise ValueError("wrote %s read %s" % (`teststr`, `got`))
    print "testing popen3..."
    try:
        w, r, e = os.popen3([cmd])
    except:
        w, r, e = os.popen3(cmd)
    w.write(teststr)
    w.close()
    got = r.read()
    if got.strip() != expected:
        raise ValueError("wrote %s read %s" % (`teststr`, `got`))
    got = e.read()
    if got:
        raise ValueError("unexected %s on stderr" % `got`)
    for inst in popen2._active[:]:
        inst.wait()
    if popen2._active:
        raise ValueError("_active not empty")
    print "All OK"

main()
_test()
