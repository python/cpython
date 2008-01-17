# Originally contributed by Sjoerd Mullender.
# Significantly modified by Jeffrey Yasskin <jyasskin at gmail.com>.

"""Rational, infinite-precision, real numbers."""

import math
import numbers
import operator

__all__ = ["Rational"]

RationalAbc = numbers.Rational


def _gcd(a, b):
    """Calculate the Greatest Common Divisor.

    Unless b==0, the result will have the same sign as b (so that when
    b is divided by it, the result comes out positive).
    """
    while b:
        a, b = b, a%b
    return a


def _binary_float_to_ratio(x):
    """x -> (top, bot), a pair of ints s.t. x = top/bot.

    The conversion is done exactly, without rounding.
    bot > 0 guaranteed.
    Some form of binary fp is assumed.
    Pass NaNs or infinities at your own risk.

    >>> _binary_float_to_ratio(10.0)
    (10, 1)
    >>> _binary_float_to_ratio(0.0)
    (0, 1)
    >>> _binary_float_to_ratio(-.25)
    (-1, 4)
    """

    if x == 0:
        return 0, 1
    f, e = math.frexp(x)
    signbit = 1
    if f < 0:
        f = -f
        signbit = -1
    assert 0.5 <= f < 1.0
    # x = signbit * f * 2**e exactly

    # Suck up CHUNK bits at a time; 28 is enough so that we suck
    # up all bits in 2 iterations for all known binary double-
    # precision formats, and small enough to fit in an int.
    CHUNK = 28
    top = 0
    # invariant: x = signbit * (top + f) * 2**e exactly
    while f:
        f = math.ldexp(f, CHUNK)
        digit = trunc(f)
        assert digit >> CHUNK == 0
        top = (top << CHUNK) | digit
        f = f - digit
        assert 0.0 <= f < 1.0
        e = e - CHUNK
    assert top

    # Add in the sign bit.
    top = signbit * top

    # now x = top * 2**e exactly; fold in 2**e
    if e>0:
        return (top * 2**e, 1)
    else:
        return (top, 2 ** -e)


class Rational(RationalAbc):
    """This class implements rational numbers.

    Rational(8, 6) will produce a rational number equivalent to
    4/3. Both arguments must be Integral. The numerator defaults to 0
    and the denominator defaults to 1 so that Rational(3) == 3 and
    Rational() == 0.

    """

    __slots__ = ('_numerator', '_denominator')

    def __init__(self, numerator=0, denominator=1):
        if (not isinstance(numerator, numbers.Integral) and
            isinstance(numerator, RationalAbc) and
            denominator == 1):
            # Handle copies from other rationals.
            other_rational = numerator
            numerator = other_rational.numerator
            denominator = other_rational.denominator

        if (not isinstance(numerator, numbers.Integral) or
            not isinstance(denominator, numbers.Integral)):
            raise TypeError("Rational(%(numerator)s, %(denominator)s):"
                            " Both arguments must be integral." % locals())

        if denominator == 0:
            raise ZeroDivisionError('Rational(%s, 0)' % numerator)

        g = _gcd(numerator, denominator)
        self._numerator = int(numerator // g)
        self._denominator = int(denominator // g)

    @classmethod
    def from_float(cls, f):
        """Converts a float to a rational number, exactly."""
        if not isinstance(f, float):
            raise TypeError("%s.from_float() only takes floats, not %r (%s)" %
                            (cls.__name__, f, type(f).__name__))
        if math.isnan(f) or math.isinf(f):
            raise TypeError("Cannot convert %r to %s." % (f, cls.__name__))
        return cls(*_binary_float_to_ratio(f))

    @property
    def numerator(a):
        return a._numerator

    @property
    def denominator(a):
        return a._denominator

    def __repr__(self):
        """repr(self)"""
        return ('rational.Rational(%r,%r)' %
                (self.numerator, self.denominator))

    def __str__(self):
        """str(self)"""
        if self.denominator == 1:
            return str(self.numerator)
        else:
            return '(%s/%s)' % (self.numerator, self.denominator)

    def _operator_fallbacks(monomorphic_operator, fallback_operator):
        """Generates forward and reverse operators given a purely-rational
        operator and a function from the operator module.

        Use this like:
        __op__, __rop__ = _operator_fallbacks(just_rational_op, operator.op)

        """
        def forward(a, b):
            if isinstance(b, RationalAbc):
                # Includes ints.
                return monomorphic_operator(a, b)
            elif isinstance(b, float):
                return fallback_operator(float(a), b)
            elif isinstance(b, complex):
                return fallback_operator(complex(a), b)
            else:
                return NotImplemented
        forward.__name__ = '__' + fallback_operator.__name__ + '__'
        forward.__doc__ = monomorphic_operator.__doc__

        def reverse(b, a):
            if isinstance(a, RationalAbc):
                # Includes ints.
                return monomorphic_operator(a, b)
            elif isinstance(a, numbers.Real):
                return fallback_operator(float(a), float(b))
            elif isinstance(a, numbers.Complex):
                return fallback_operator(complex(a), complex(b))
            else:
                return NotImplemented
        reverse.__name__ = '__r' + fallback_operator.__name__ + '__'
        reverse.__doc__ = monomorphic_operator.__doc__

        return forward, reverse

    def _add(a, b):
        """a + b"""
        return Rational(a.numerator * b.denominator +
                        b.numerator * a.denominator,
                        a.denominator * b.denominator)

    __add__, __radd__ = _operator_fallbacks(_add, operator.add)

    def _sub(a, b):
        """a - b"""
        return Rational(a.numerator * b.denominator -
                        b.numerator * a.denominator,
                        a.denominator * b.denominator)

    __sub__, __rsub__ = _operator_fallbacks(_sub, operator.sub)

    def _mul(a, b):
        """a * b"""
        return Rational(a.numerator * b.numerator, a.denominator * b.denominator)

    __mul__, __rmul__ = _operator_fallbacks(_mul, operator.mul)

    def _div(a, b):
        """a / b"""
        return Rational(a.numerator * b.denominator,
                        a.denominator * b.numerator)

    __truediv__, __rtruediv__ = _operator_fallbacks(_div, operator.truediv)

    def __floordiv__(a, b):
        """a // b"""
        return math.floor(a / b)

    def __rfloordiv__(b, a):
        """a // b"""
        return math.floor(a / b)

    @classmethod
    def _mod(cls, a, b):
        div = a // b
        return a - b * div

    def __mod__(a, b):
        """a % b"""
        return a._mod(a, b)

    def __rmod__(b, a):
        """a % b"""
        return b._mod(a, b)

    def __pow__(a, b):
        """a ** b

        If b is not an integer, the result will be a float or complex
        since roots are generally irrational. If b is an integer, the
        result will be rational.

        """
        if isinstance(b, RationalAbc):
            if b.denominator == 1:
                power = b.numerator
                if power >= 0:
                    return Rational(a.numerator ** power,
                                    a.denominator ** power)
                else:
                    return Rational(a.denominator ** -power,
                                    a.numerator ** -power)
            else:
                # A fractional power will generally produce an
                # irrational number.
                return float(a) ** float(b)
        else:
            return float(a) ** b

    def __rpow__(b, a):
        """a ** b"""
        if b.denominator == 1 and b.numerator >= 0:
            # If a is an int, keep it that way if possible.
            return a ** b.numerator

        if isinstance(a, RationalAbc):
            return Rational(a.numerator, a.denominator) ** b

        if b.denominator == 1:
            return a ** b.numerator

        return a ** float(b)

    def __pos__(a):
        """+a: Coerces a subclass instance to Rational"""
        return Rational(a.numerator, a.denominator)

    def __neg__(a):
        """-a"""
        return Rational(-a.numerator, a.denominator)

    def __abs__(a):
        """abs(a)"""
        return Rational(abs(a.numerator), a.denominator)

    def __trunc__(a):
        """trunc(a)"""
        if a.numerator < 0:
            return -(-a.numerator // a.denominator)
        else:
            return a.numerator // a.denominator

    def __floor__(a):
        """Will be math.floor(a) in 3.0."""
        return a.numerator // a.denominator

    def __ceil__(a):
        """Will be math.ceil(a) in 3.0."""
        # The negations cleverly convince floordiv to return the ceiling.
        return -(-a.numerator // a.denominator)

    def __round__(self, ndigits=None):
        """Will be round(self, ndigits) in 3.0.

        Rounds half toward even.
        """
        if ndigits is None:
            floor, remainder = divmod(self.numerator, self.denominator)
            if remainder * 2 < self.denominator:
                return floor
            elif remainder * 2 > self.denominator:
                return floor + 1
            # Deal with the half case:
            elif floor % 2 == 0:
                return floor
            else:
                return floor + 1
        shift = 10**abs(ndigits)
        # See _operator_fallbacks.forward to check that the results of
        # these operations will always be Rational and therefore have
        # round().
        if ndigits > 0:
            return Rational(round(self * shift), shift)
        else:
            return Rational(round(self / shift) * shift)

    def __hash__(self):
        """hash(self)

        Tricky because values that are exactly representable as a
        float must have the same hash as that float.

        """
        if self.denominator == 1:
            # Get integers right.
            return hash(self.numerator)
        # Expensive check, but definitely correct.
        if self == float(self):
            return hash(float(self))
        else:
            # Use tuple's hash to avoid a high collision rate on
            # simple fractions.
            return hash((self.numerator, self.denominator))

    def __eq__(a, b):
        """a == b"""
        if isinstance(b, RationalAbc):
            return (a.numerator == b.numerator and
                    a.denominator == b.denominator)
        if isinstance(b, numbers.Complex) and b.imag == 0:
            b = b.real
        if isinstance(b, float):
            return a == a.from_float(b)
        else:
            # XXX: If b.__eq__ is implemented like this method, it may
            # give the wrong answer after float(a) changes a's
            # value. Better ways of doing this are welcome.
            return float(a) == b

    def _subtractAndCompareToZero(a, b, op):
        """Helper function for comparison operators.

        Subtracts b from a, exactly if possible, and compares the
        result with 0 using op, in such a way that the comparison
        won't recurse. If the difference raises a TypeError, returns
        NotImplemented instead.

        """
        if isinstance(b, numbers.Complex) and b.imag == 0:
            b = b.real
        if isinstance(b, float):
            b = a.from_float(b)
        try:
            # XXX: If b <: Real but not <: RationalAbc, this is likely
            # to fall back to a float. If the actual values differ by
            # less than MIN_FLOAT, this could falsely call them equal,
            # which would make <= inconsistent with ==. Better ways of
            # doing this are welcome.
            diff = a - b
        except TypeError:
            return NotImplemented
        if isinstance(diff, RationalAbc):
            return op(diff.numerator, 0)
        return op(diff, 0)

    def __lt__(a, b):
        """a < b"""
        return a._subtractAndCompareToZero(b, operator.lt)

    def __gt__(a, b):
        """a > b"""
        return a._subtractAndCompareToZero(b, operator.gt)

    def __le__(a, b):
        """a <= b"""
        return a._subtractAndCompareToZero(b, operator.le)

    def __ge__(a, b):
        """a >= b"""
        return a._subtractAndCompareToZero(b, operator.ge)

    def __bool__(a):
        """a != 0"""
        return a.numerator != 0
