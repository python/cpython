#! /usr/bin/env python
"""Test dlmodule.c
   Roger E. Masse  revised strategy by Barry Warsaw
"""
verbose = 0
if __name__ == '__main__':
    verbose = 1

import dl

sharedlibs = [
    # SunOS/Solaris
    ('/usr/lib/libresolv.so', 'gethostent'),
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
