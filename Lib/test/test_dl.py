#! /usr/bin/env python
"""Test dlmodule.c
   Roger E. Masse  revised strategy by Barry Warsaw
"""

import dl
from test_support import verbose

sharedlibs = [
    # SunOS/Solaris
    ('/usr/lib/libresolv.so', 'gethostent'),
    # SGI IRIX
    ('/usr/lib/libm.so', 'sin'),
    ]

for s, func in sharedlibs:
    try:
	if verbose:
	    print 'trying to open:', s,
	l = dl.open(s)
    except dl.error:
	if verbose:
	    print 'failed'
	pass
    else:
	if verbose:
	    print 'succeeded...',
	l.call(func)
	l.close()
	if verbose:
	    print 'worked!'
	break
else:
    print 'Could not open any shared libraries'
