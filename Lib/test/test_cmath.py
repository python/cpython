#! /usr/bin/env python
""" Simple test script for cmathmodule.c
    Roger E. Masse
"""
import cmath, math
from test.test_support import verbose, verify, TestFailed

verify(abs(cmath.log(10) - math.log(10)) < 1e-9)
verify(abs(cmath.log(10,2) - math.log(10,2)) < 1e-9)
try:
    cmath.log('a')
except TypeError:
    pass
else:
    raise TestFailed

try:
    cmath.log(10, 'a')
except TypeError:
    pass
else:
    raise TestFailed


testdict = {'acos' : 1.0,
            'acosh' : 1.0,
            'asin' : 1.0,
            'asinh' : 1.0,
            'atan' : 0.2,
            'atanh' : 0.2,
            'cos' : 1.0,
            'cosh' : 1.0,
            'exp' : 1.0,
            'log' : 1.0,
            'log10' : 1.0,
            'sin' : 1.0,
            'sinh' : 1.0,
            'sqrt' : 1.0,
            'tan' : 1.0,
            'tanh' : 1.0}

for func in testdict.keys():
    f = getattr(cmath, func)
    r = f(testdict[func])
    if verbose:
        print 'Calling %s(%f) = %f' % (func, testdict[func], abs(r))

p = cmath.pi
e = cmath.e
if verbose:
    print 'PI = ', abs(p)
    print 'E = ', abs(e)
