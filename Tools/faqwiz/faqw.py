#! /usr/local/bin/python
import posix
t1 = posix.times()
try:
    FAQDIR = "/usr/people/guido/python/FAQ"
    SRCDIR = "/usr/people/guido/python/Tools/faqwiz"
    import os, sys, time, operator
    os.chdir(FAQDIR)
    sys.path.insert(0, SRCDIR)
    import faqwiz
except SystemExit, n:
    sys.exit(n)
except:
    t, v, tb = sys.exc_type, sys.exc_value, sys.exc_traceback
    print
    import cgi
    cgi.print_exception(t, v, tb)
t2 = posix.times()
fmt = "<BR>(times: user %.3g, sys %.3g, ch-user %.3g, ch-sys %.3g, real %.3g)"
print fmt % tuple(map(operator.sub, t2, t1))
