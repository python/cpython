#! /usr/bin/env python
"""Test script for popen2.py
   Christian Tismer
"""

import os

# popen2 contains its own testing routine
# which is especially useful to see if open files
# like stdin can be read successfully by a forked
# subprocess.

def main():
    print "Test popen2 module:"
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
    teststr = "abc\n"
    resultstr = teststr
    if os.name == "nt":
        cmd = "more"
        resultstr = "\n" + resultstr
    print "testing popen2..."
    w, r = os.popen2(cmd)
    w.write(teststr)
    w.close()
    assert r.read() == resultstr
    print "testing popen3..."
    try:
        w, r, e = os.popen3([cmd])
    except:
        w, r, e = os.popen3(cmd)
    w.write(teststr)
    w.close()
    assert r.read() == resultstr
    assert e.read() == ""
    for inst in popen2._active[:]:
        inst.wait()
    assert not popen2._active
    print "All OK"

main()
_test()
