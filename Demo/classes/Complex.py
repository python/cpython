# Complex numbers
# ---------------

# [Now that Python has a complex data type built-in, this is not very
# useful, but it's still a nice example class]

# This module represents complex numbers as instances of the class Complex.
# A Complex instance z has two data attribues, z.re (the real part) and z.im
# (the imaginary part).  In fact, z.re and z.im can have any value -- all
# arithmetic operators work regardless of the type of z.re and z.im (as long
# as they support numerical operations).
#
# The following functions exist (Complex is actually a class):
# Complex([re [,im]) -> creates a complex number from a real and an imaginary part
# IsComplex(z) -> true iff z is a complex number (== has .re and .im attributes)
# ToComplex(z) -> a complex number equal to z; z itself if IsComplex(z) is true
#                 if z is a tuple(re, im) it will also be converted
# PolarToComplex([r [,phi [,fullcircle]]]) ->
#       the complex number z for which r == z.radius() and phi == z.angle(fullcircle)
#       (r and phi default to 0)
# exp(z) -> returns the complex exponential of z. Equivalent to pow(math.e,z).
#
# Complex numbers have the following methods:
# z.abs() -> absolute value of z
# z.radius() == z.abs()
# z.angle([fullcircle]) -> angle from positive X axis; fullcircle gives units
# z.phi([fullcircle]) == z.angle(fullcircle)
#
# These standard functions and unary operators accept complex arguments:
# abs(z)
# -z
# +z
# not z
# repr(z) == `z`
# str(z)
# hash(z) -> a combination of hash(z.re) and hash(z.im) such that if z.im is zero
#            the result equals hash(z.re)
# Note that hex(z) and oct(z) are not defined.
#
# These conversions accept complex arguments only if their imaginary part is zero:
# int(z)
# long(z)
# float(z)
#
# The following operators accept two complex numbers, or one complex number
# and one real number (int, long or float):
# z1 + z2
# z1 - z2
# z1 * z2
# z1 / z2
# pow(z1, z2)
# cmp(z1, z2)
# Note that z1 % z2 and divmod(z1, z2) are not defined,
# nor are shift and mask operations.
#
# The standard module math does not support complex numbers.
# The cmath modules should be used instead.
#
# Idea:
# add a class Polar(r, phi) and mixed-mode arithmetic which
# chooses the most appropriate type for the result:
# Complex for +,-,cmp
# Polar   for *,/,pow

import math
import sys

twopi = math.pi*2.0
halfpi = math.pi/2.0

def IsComplex(obj):
    return hasattr(obj, 're') and hasattr(obj, 'im')

def ToComplex(obj):
    if IsComplex(obj):
        return obj
    elif isinstance(obj, tuple):
        return Complex(*obj)
    else:
        return Complex(obj)

def PolarToComplex(r = 0, phi = 0, fullcircle = twopi):
    phi = phi * (twopi / fullcircle)
    return Complex(math.cos(phi)*r, math.sin(phi)*r)

def Re(obj):
    if IsComplex(obj):
        return obj.re
    return obj

def Im(obj):
    if IsComplex(obj):
        return obj.im
    return 0

class Complex:

    def __init__(self, re=0, im=0):
        _re = 0
        _im = 0
        if IsComplex(re):
            _re = re.re
            _im = re.im
        else:
            _re = re
        if IsComplex(im):
            _re = _re - im.im
            _im = _im + im.re
        else:
            _im = _im + im
        # this class is immutable, so setting self.re directly is
        # not possible.
        self.__dict__['re'] = _re
        self.__dict__['im'] = _im

    def __setattr__(self, name, value):
        raise TypeError, 'Complex numbers are immutable'

    def __hash__(self):
        if not self.im:
            return hash(self.re)
        return hash((self.re, self.im))

    def __repr__(self):
        if not self.im:
            return 'Complex(%r)' % (self.re,)
        else:
            return 'Complex(%r, %r)' % (self.re, self.im)

    def __str__(self):
        if not self.im:
            return repr(self.re)
        else:
            return 'Complex(%r, %r)' % (self.re, self.im)

    def __neg__(self):
        return Complex(-self.re, -self.im)

    def __pos__(self):
        return self

    def __abs__(self):
        return math.hypot(self.re, self.im)

    def __int__(self):
        if self.im:
            raise ValueError, "can't convert Complex with nonzero im to int"
        return int(self.re)

    def __long__(self):
        if self.im:
            raise ValueError, "can't convert Complex with nonzero im to long"
        return long(self.re)

    def __float__(self):
        if self.im:
            raise ValueError, "can't convert Complex with nonzero im to float"
        return float(self.re)

    def __cmp__(self, other):
        other = ToComplex(other)
        return cmp((self.re, self.im), (other.re, other.im))

    def __rcmp__(self, other):
        other = ToComplex(other)
        return cmp(other, self)

    def __nonzero__(self):
        return not (self.re == self.im == 0)

    abs = radius = __abs__

    def angle(self, fullcircle = twopi):
        return (fullcircle/twopi) * ((halfpi - math.atan2(self.re, self.im)) % twopi)

    phi = angle

    def __add__(self, other):
        other = ToComplex(other)
        return Complex(self.re + other.re, self.im + other.im)

    __radd__ = __add__

    def __sub__(self, other):
        other = ToComplex(other)
        return Complex(self.re - other.re, self.im - other.im)

    def __rsub__(self, other):
        other = ToComplex(other)
        return other - self

    def __mul__(self, other):
        other = ToComplex(other)
        return Complex(self.re*other.re - self.im*other.im,
                       self.re*other.im + self.im*other.re)

    __rmul__ = __mul__

    def __div__(self, other):
        other = ToComplex(other)
        d = float(other.re*other.re + other.im*other.im)
        if not d: raise ZeroDivisionError, 'Complex division'
        return Complex((self.re*other.re + self.im*other.im) / d,
                       (self.im*other.re - self.re*other.im) / d)

    def __rdiv__(self, other):
        other = ToComplex(other)
        return other / self

    def __pow__(self, n, z=None):
        if z is not None:
            raise TypeError, 'Complex does not support ternary pow()'
        if IsComplex(n):
            if n.im:
                if self.im: raise TypeError, 'Complex to the Complex power'
                else: return exp(math.log(self.re)*n)
            n = n.re
        r = pow(self.abs(), n)
        phi = n*self.angle()
        return Complex(math.cos(phi)*r, math.sin(phi)*r)

    def __rpow__(self, base):
        base = ToComplex(base)
        return pow(base, self)

def exp(z):
    r = math.exp(z.re)
    return Complex(math.cos(z.im)*r,math.sin(z.im)*r)


def checkop(expr, a, b, value, fuzz = 1e-6):
    print '       ', a, 'and', b,
    try:
        result = eval(expr)
    except:
        result = sys.exc_type
    print '->', result
    if isinstance(result, str) or isinstance(value, str):
        ok = (result == value)
    else:
        ok = abs(result - value) <= fuzz
    if not ok:
        print '!!\t!!\t!! should be', value, 'diff', abs(result - value)

def test():
    print 'test constructors'
    constructor_test = (
        # "expect" is an array [re,im] "got" the Complex.
            ( (0,0), Complex() ),
            ( (0,0), Complex() ),
            ( (1,0), Complex(1) ),
            ( (0,1), Complex(0,1) ),
            ( (1,2), Complex(Complex(1,2)) ),
            ( (1,3), Complex(Complex(1,2),1) ),
            ( (0,0), Complex(0,Complex(0,0)) ),
            ( (3,4), Complex(3,Complex(4)) ),
            ( (-1,3), Complex(1,Complex(3,2)) ),
            ( (-7,6), Complex(Complex(1,2),Complex(4,8)) ) )
    cnt = [0,0]
    for t in constructor_test:
        cnt[0] += 1
        if ((t[0][0]!=t[1].re)or(t[0][1]!=t[1].im)):
            print "        expected", t[0], "got", t[1]
            cnt[1] += 1
    print "  ", cnt[1], "of", cnt[0], "tests failed"
    # test operators
    testsuite = {
            'a+b': [
                    (1, 10, 11),
                    (1, Complex(0,10), Complex(1,10)),
                    (Complex(0,10), 1, Complex(1,10)),
                    (Complex(0,10), Complex(1), Complex(1,10)),
                    (Complex(1), Complex(0,10), Complex(1,10)),
            ],
            'a-b': [
                    (1, 10, -9),
                    (1, Complex(0,10), Complex(1,-10)),
                    (Complex(0,10), 1, Complex(-1,10)),
                    (Complex(0,10), Complex(1), Complex(-1,10)),
                    (Complex(1), Complex(0,10), Complex(1,-10)),
            ],
            'a*b': [
                    (1, 10, 10),
                    (1, Complex(0,10), Complex(0, 10)),
                    (Complex(0,10), 1, Complex(0,10)),
                    (Complex(0,10), Complex(1), Complex(0,10)),
                    (Complex(1), Complex(0,10), Complex(0,10)),
            ],
            'a/b': [
                    (1., 10, 0.1),
                    (1, Complex(0,10), Complex(0, -0.1)),
                    (Complex(0, 10), 1, Complex(0, 10)),
                    (Complex(0, 10), Complex(1), Complex(0, 10)),
                    (Complex(1), Complex(0,10), Complex(0, -0.1)),
            ],
            'pow(a,b)': [
                    (1, 10, 1),
                    (1, Complex(0,10), 1),
                    (Complex(0,10), 1, Complex(0,10)),
                    (Complex(0,10), Complex(1), Complex(0,10)),
                    (Complex(1), Complex(0,10), 1),
                    (2, Complex(4,0), 16),
            ],
            'cmp(a,b)': [
                    (1, 10, -1),
                    (1, Complex(0,10), 1),
                    (Complex(0,10), 1, -1),
                    (Complex(0,10), Complex(1), -1),
                    (Complex(1), Complex(0,10), 1),
            ],
    }
    for expr in sorted(testsuite):
        print expr + ':'
        t = (expr,)
        for item in testsuite[expr]:
            checkop(*(t+item))


if __name__ == '__main__':
    test()
