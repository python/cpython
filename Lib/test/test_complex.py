from test_support import TestFailed
from random import random

# These tests ensure that complex math does the right thing; tests of
# the complex() function/constructor are in test_b1.py.

# XXX need many, many more tests here.

nerrors = 0

def check_close_real(x, y, eps=1e-9):
    """Return true iff floats x and y "are close\""""
    # put the one with larger magnitude second
    if abs(x) > abs(y):
        x, y = y, x
    if y == 0:
        return abs(x) < eps
    if x == 0:
        return abs(y) < eps
    # check that relative difference < eps
    return abs((x-y)/y) < eps

def check_close(x, y, eps=1e-9):
    """Return true iff complexes x and y "are close\""""
    return check_close_real(x.real, y.real, eps) and \
           check_close_real(x.imag, y.imag, eps)

def test_div(x, y):
    """Compute complex z=x*y, and check that z/x==y and z/y==x."""
    global nerrors
    z = x * y
    if x != 0:
        q = z / x
        if not check_close(q, y):
            nerrors += 1
            print "%r / %r == %r but expected %r" % (z, x, q, y)
    if y != 0:
        q = z / y
        if not check_close(q, x):
            nerrors += 1
            print "%r / %r == %r but expected %r" % (z, y, q, x)

simple_real = [float(i) for i in range(-5, 6)]
simple_complex = [complex(x, y) for x in simple_real for y in simple_real]
for x in simple_complex:
    for y in simple_complex:
        test_div(x, y)

# A naive complex division algorithm (such as in 2.0) is very prone to
# nonsense errors for these (overflows and underflows).
test_div(complex(1e200, 1e200), 1+0j)
test_div(complex(1e-200, 1e-200), 1+0j)

# Just for fun.
for i in range(100):
    test_div(complex(random(), random()),
             complex(random(), random()))

try:
    z = 1.0 / (0+0j)
except ZeroDivisionError:
    pass
else:
    nerrors += 1
    raise TestFailed("Division by complex 0 didn't raise ZeroDivisionError")

if nerrors:
    raise TestFailed("%d tests failed" % nerrors)
