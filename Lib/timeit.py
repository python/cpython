"""Framework for timing execution speed of small code snippets.

This avoids a number of common traps for timing frameworks (see also
Tim Peters' introduction to the timing chapter in the Python
Cookbook).

(To use this with older versions of Python, the dependency on the
itertools module is easily removed; instead of itertools.repeat(None,
count) you can use [None]*count; this is barely slower.)

Command line usage:
  python timeit.py [-n N] [-r N] [-s S] [-t] [-c] [statement]

Options:
  -n/--number N: how many times to execute 'statement' (default varies)
  -r/--repeat N: how many times to repeat the timer (default 1)
  -s/--setup S: statements executed once before 'statement' (default 'pass')
  -t/--time: use time.time() (default on Unix)
  -c/--clock: use time.clock() (default on Windows)
  statement: statement to be timed (default 'pass')
"""

import sys
import math
import time
import itertools

__all__ = ["Timer"]

default_number = 1000000
default_repeat = 10

if sys.platform == "win32":
    # On Windows, the best timer is time.clock()
    default_timer = time.clock
else:
    # On most other platforms the best timer is time.time()
    default_timer = time.time

template = """
def inner(number, timer):
    %(setup)s
    seq = itertools.repeat(None, number)
    t0 = timer()
    for i in seq:
        %(stmt)s
    t1 = timer()
    return t1-t0
"""

def reindent(src, indent):
    return ("\n" + " "*indent).join(src.split("\n"))

class Timer:

    def __init__(self, stmt="pass", setup="pass", timer=default_timer):
        self.timer = timer
        stmt = reindent(stmt, 8)
        setup = reindent(setup, 4)
        src = template % {'stmt': stmt, 'setup': setup}
        code = compile(src, "<src>", "exec")
        ns = {}
        exec code in globals(), ns
        self.inner = ns["inner"]

    def timeit(self, number=default_number):
        return self.inner(number, self.timer)

    def repeat(self, repeat=default_repeat, number=default_number):
        r = []
        for i in range(repeat):
            t = self.timeit(number)
            r.append(t)
        return r

def main(args=None):
    if args is None:
        args = sys.argv[1:]
    import getopt
    try:
        opts, args = getopt.getopt(args, "n:s:r:tc",
                                   ["number=", "setup=", "repeat=",
                                    "time", "clock"])
    except getopt.error, err:
        print err
        return 2
    timer = default_timer
    stmt = "\n".join(args) or "pass"
    number = 0 # auto-determine
    setup = "pass"
    repeat = 1
    for o, a in opts:
        if o in ("-n", "--number"):
            number = int(a)
        if o in ("-s", "--setup"):
            setup = a
        if o in ("-r", "--repeat"):
            repeat = int(a)
            if repeat <= 0:
                repeat = 1
        if o in ("-t", "time"):
            timer = time.time
        if o in ("-c", "clock"):
            timer = time.clock
    t = Timer(stmt, setup, timer)
    if number == 0:
        # determine number so that 0.2 <= total time < 2.0
        for i in range(1, 10):
            number = 10**i
            x = t.timeit(number)
            if x >= 0.2:
                break
    r = t.repeat(repeat, number)
    best = min(r)
    print "%d loops," % number,
    usec = best * 1e6 / number
    if repeat > 1:
        print "best of %d: %.3f usec" % (repeat, usec)
    else:
        print "time: %.3f usec" % usec

if __name__ == "__main__":
    sys.exit(main())
