"""Tool for measuring execution time of small code snippets.

This module avoids a number of common traps for measuring execution
times.  See also Tim Peters' introduction to the Algorithms chapter in
the Python Cookbook, published by O'Reilly.

Library usage: see the Timer class.

Command line usage:
    python timeit.py [-n N] [-r N] [-s S] [-t] [-c] [-h] [statement]

Options:
  -n/--number N: how many times to execute 'statement' (default: see below)
  -r/--repeat N: how many times to repeat the timer (default 1)
  -s/--setup S: statement to be executed once initially (default 'pass')
  -t/--time: use time.time() (default on Unix)
  -c/--clock: use time.clock() (default on Windows)
  -h/--help: print this usage message and exit
  statement: statement to be timed (default 'pass')

A multi-line statement may be given by specifying each line as a
separate argument; indented lines are possible by enclosing an
argument in quotes and using leading spaces.  Multiple -s options are
treated similarly.

If -n is not given, a suitable number of loops is calculated by trying
successive powers of 10 until the total time is at least 0.2 seconds.

The difference in default timer function is because on Windows,
clock() has microsecond granularity but time()'s granularity is 1/60th
of a second; on Unix, clock() has 1/100th of a second granularity and
time() is much more precise.  On either platform, the default timer
functions measures wall clock time, not the CPU time.  This means that
other processes running on the same computer may interfere with the
timing.  The best thing to do when accurate timing is necessary is to
repeat the timing a few times and use the best time; the -r option is
good for this.  On Unix, you can use clock() to measure CPU time.

Note: there is a certain baseline overhead associated with executing a
pass statement.  The code here doesn't try to hide it, but you should
be aware of it.  The baseline overhead can be measured by invoking the
program without arguments.

The baseline overhead differs between Python versions!  Also, to
fairly compare older Python versions to Python 2.3, you may want to
use python -O for the older versions to avoid timing SET_LINENO
instructions.
"""

import sys
import math
import time
try:
    import itertools
except ImportError:
    # Must be an older Python version (see timeit() below)
    itertools = None

__all__ = ["Timer"]

default_number = 1000000
default_repeat = 10

if sys.platform == "win32":
    # On Windows, the best timer is time.clock()
    default_timer = time.clock
else:
    # On most other platforms the best timer is time.time()
    default_timer = time.time

# Don't change the indentation of the template; the reindent() calls
# in Timer.__init__() depend on setup being indented 4 spaces and stmt
# being indented 8 spaces.
template = """
def inner(seq, timer):
    %(setup)s
    t0 = timer()
    for i in seq:
        %(stmt)s
    t1 = timer()
    return t1-t0
"""

def reindent(src, indent):
    """Helper to reindent a multi-line statement."""
    return src.replace("\n", "\n" + " "*indent)

class Timer:
    """Class for timing execution speed of small code snippets.

    The constructor takes a statement to be timed, an additional
    statement used for setup, and a timer function.  Both statements
    default to 'pass'; the timer function is platform-dependent (see
    module doc string).

    To measure the execution time of the first statement, use the
    timeit() method.  The repeat() method is a convenience to call
    timeit() multiple times and return a list of results.

    The statements may contain newlines, as long as they don't contain
    multi-line string literals.
    """

    def __init__(self, stmt="pass", setup="pass", timer=default_timer):
        """Constructor.  See class doc string."""
        self.timer = timer
        stmt = reindent(stmt, 8)
        setup = reindent(setup, 4)
        src = template % {'stmt': stmt, 'setup': setup}
        code = compile(src, "<src>", "exec")
        ns = {}
        exec code in globals(), ns
        self.inner = ns["inner"]

    def timeit(self, number=default_number):
        """Time 'number' executions of the main statement.

        To be precise, this executes the setup statement once, and
        then returns the time it takes to execute the main statement
        a number of times, as a float measured in seconds.  The
        argument is the number of times through the loop, defaulting
        to one million.  The main statement, the setup statement and
        the timer function to be used are passed to the constructor.
        """
        if itertools:
            seq = itertools.repeat(None, number)
        else:
            seq = [None] * number
        return self.inner(seq, self.timer)

    def repeat(self, repeat=default_repeat, number=default_number):
        """Call timer() a few times.

        This is a convenience function that calls the timer()
        repeatedly, returning a list of results.  The first argument
        specifies how many times to call timer(), defaulting to 10;
        the second argument specifies the timer argument, defaulting
        to one million.

        Note: it's tempting to calculate mean and standard deviation
        from the result vector and report these.  However, this is not
        very useful.  In a typical case, the lowest value gives a
        lower bound for how fast your machine can run the given code
        snippet; higher values in the result vector are typically not
        caused by variability in Python's speed, but by other
        processes interfering with your timing accuracy.  So the min()
        of the result is probably the only number you should be
        interested in.  After that, you should look at the entire
        vector and apply common sense rather than statistics.
        """
        r = []
        for i in range(repeat):
            t = self.timeit(number)
            r.append(t)
        return r

def main(args=None):
    """Main program, used when run as a script.

    The optional argument specifies the command line to be parsed,
    defaulting to sys.argv[1:].

    The return value is an exit code to be passed to sys.exit(); it
    may be None to indicate success.
    """
    if args is None:
        args = sys.argv[1:]
    import getopt
    try:
        opts, args = getopt.getopt(args, "n:s:r:tch",
                                   ["number=", "setup=", "repeat=",
                                    "time", "clock", "help"])
    except getopt.error, err:
        print err
        print "use -h/--help for command line help"
        return 2
    timer = default_timer
    stmt = "\n".join(args) or "pass"
    number = 0 # auto-determine
    setup = []
    repeat = 1
    for o, a in opts:
        if o in ("-n", "--number"):
            number = int(a)
        if o in ("-s", "--setup"):
            setup.append(a)
        if o in ("-r", "--repeat"):
            repeat = int(a)
            if repeat <= 0:
                repeat = 1
        if o in ("-t", "--time"):
            timer = time.time
        if o in ("-c", "--clock"):
            timer = time.clock
        if o in ("-h", "--help"):
            print __doc__,
            return 0
    setup = "\n".join(setup) or "pass"
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
    return None

if __name__ == "__main__":
    sys.exit(main())
