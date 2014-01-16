"""Test dlmodule.c
   Roger E. Masse  revised strategy by Barry Warsaw
"""
import unittest
from test.test_support import verbose, import_module
dl = import_module('dl', deprecated=True)

sharedlibs = [
    ('/usr/lib/libc.so', 'getpid'),
    ('/lib/libc.so.6', 'getpid'),
    ('/usr/bin/cygwin1.dll', 'getpid'),
    ('/usr/lib/libc.dylib', 'getpid'),
    ]

def test_main():
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
        raise unittest.SkipTest, 'Could not open any shared libraries'


if __name__ == '__main__':
    test_main()
