#! /usr/bin/env python
""" Simple test script for cmathmodule.c
    Roger E. Masse
"""
import cmath
from test_support import verbose

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
