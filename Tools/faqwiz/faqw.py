#! /usr/local/bin/python
import posix
t1 = posix.times()
FAQDIR = "/usr/people/guido/python/FAQ"
import os, sys, time, operator
os.chdir(FAQDIR)
sys.path.insert(0, FAQDIR)
try:
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
