#! /usr/local/bin/python

"""FAQ wizard bootstrap."""

# This is a longer version of the bootstrap script given at the end of
# faqwin.py; it prints timing statistics at the end of the regular CGI
# script's output (so you can monitor how it is doing).

# This script should be placed in your cgi-bin directory and made
# executable.

# You need to edit the first line and the lines that define FAQDIR and
# SRCDIR, below: change /usr/local/bin/python to where your Python
# interpreter lives, change the value for FAQDIR to where your FAQ
# lives, and change the value for SRCDIR to where your faqwiz.py
# module lives.  The faqconf.py and faqcust.py files live there, too.

import os
t1 = os.times() # If this doesn't work, just get rid of the timing code!
try:
    FAQDIR = "/usr/people/guido/python/FAQ"
    SRCDIR = "/usr/people/guido/python/src/Tools/faqwiz"
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
t2 = os.times() # If this doesn't work, get rid of this and what follows!
fmt = "<BR>(times: user %.3g, sys %.3g, ch-user %.3g, ch-sys %.3g, real %.3g)"
print fmt % tuple(map(operator.sub, t2, t1))
