#! /usr/bin/env python
"""Test dlmodule.c
   Roger E. Masse  revised strategy by Barry Warsaw
"""

import dl
from test_support import verbose,TestSkipped

sharedlibs = [
    ('/usr/lib/libc.so', 'getpid'),
    ('/lib/libc.so.6', 'getpid'),
    ]

for s, func in sharedlibs:
    try:
        if verbose:
            print 'trying to open:', s,
        l = dl.open(s)
    except dl.error, err:
        if verbose:
            print 'failed', repr(str(err))
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
    raise TestSkipped, 'Could not open any shared libraries'
