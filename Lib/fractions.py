# Originally contributed by Sjoerd Mullender.
# Significantly modified by Jeffrey Yasskin <jyasskin at gmail.com>.

"""Rational, infinite-precision, real numbers."""

from __future__ import division
import math
import numbers
import operator
import re

__all__ = ["Fraction"]

Rational = numbers.Rational


def gcd(a, b):
    """Calculate the Greatest Common Divisor of a and b.

    Unless b==0, the result will have the same sign as b (so that when
    b is divided by it, the result comes out positive).
    """
    while b:
        a, b = b, a%b
    return a


_RATIONAL_FORMAT = re.compile(r"""
    \A\s*                      # optional whitespace at the start, then
    (?P<sign>[-+]?)            # an optional sign, then
    (?=\d|\.\d)                # lookahead for digit or .digit
    (?P<num>\d*)               # numerator (possibly empty)
    (?:                        # followed by an optional
       /(?P<denom>\d+)         # / and denominator
    |                          # or
       \.(?P<decimal>\d*)      # decimal point and fractional part
    )?
    \s*\Z                      # and optional whitespace to finish
""", re.VERBOSE)


class Fraction(Rational):
    """This class implements rational numbers.

    Fraction(8, 6) will produce a rational number equivalent to
    4/3. Both arguments must be Integral. The numerator defaults to 0
    and the denominator defaults to 1 so that Fraction(3) == 3 and
    Fraction() == 0.

    Fractions can also be constructed from strings of the form
    '[-+]?[0-9]+((/|.)[0-9]+)?', optionally surrounded by spaces.

    """

    __slots__ = ('_numerator', '_denominator')

    # We're immutable, so use __new__ not __init__
    def __new__(cls, numerator=0, denominator=1):
        """Constructs a Fraction.

        Takes a string like '3/2' or '1.5', another Fraction, or a
        numerator/denominator pair.

        """
        self = super(Fraction, cls).__new__(cls)

        if type(numerator) not in (int, long) and denominator == 1:
            if isinstance(numerator, basestring):
                # Handle construction from strings.
                input = numerator
                m = _RATIONAL_FORMAT.match(input)
                if m is None:
                    raise ValueError('Invalid literal for Fraction: ' + input)
                numerator = m.group('num')
                decimal = m.group('decimal')
                if decimal:
                    # The literal is a decimal number.
                    numerator = int(numerator + decimal)
                    denominator = 10**len(decimal)
                else:
                    # The literal is an integer or fraction.
                    numerator = int(numerator)
                    # Default denominator to 1.
                    denominator = int(m.group('denom') or 1)

                if m.group('sign') == '-':
                    numerator = -numerator

            elif isinstance(numerator, Rational):
                # Handle copies from other rationals. Integrals get
                # caught here too, but it doesn't matter because
                # denominator is already 1.
                other_rational = numerator
                numerator = other_rational.numerator
                denominator = other_rational.denominator

        if denominator == 0:
            raise ZeroDivisionError('Fraction(%s, 0)' % numerator)

        numerator = numerator.__index__()
        denominator = denominator.__index__()
        g = gcd(numerator, denominator)
        self._numerator = numerator // g
        self._denominator = denominator // g
        return self

    @classmethod
    def from_float(cls, f):
        """Converts a finite float to a rational number, exactly.

        Beware that Fraction.from_float(0.3) != Fraction(3, 10).

        """
        if not isinstance(f, float):
            raise TypeError("%s.from_float() only takes floats, not %r (%s)" %
                            (cls.__name__, f, type(f).__name__))
        if math.isnan(f) or math.isinf(f):
            raise TypeError("Cannot convert %r to %s." % (f, cls.__name__))
        return cls(*f.as_integer_ratio())

    @classmethod
    def from_decimal(cls, dec):
        """Converts a finite Decimal instance to a rational number, exactly."""
        from decimal import Decimal
        if not isinstance(dec, Decimal):
            raise TypeError(
                "%s.from_decimal() only takes Decimals, not %r (%s)" %
                (cls.__name__, dec, type(dec).__name__))
        if not dec.is_finite():
            # Catches infinities and nans.
            raise TypeError("Cannot convert %s to %s." % (dec, cls.__name__))
        sign, digits, exp = dec.as_tuple()
        digits = int(''.join(map(str, digits)))
        if sign:
            digits = -digits
        if exp >= 0:
            return cls(digits * 10 ** exp)
        else:
            return cls(digits, 10 ** -exp)

    def limit_denominator(self, max_denominator=1000000):
        """Closest Fraction to self with denominator at most max_denominator.

        >>> Fraction('3.141592653589793').limit_denominator(10)
        Fraction(22, 7)
        >>> Fraction('3.141592653589793').limit_denominator(100)
        Fraction(311, 99)
        >>> Fraction(1234, 5678).limit_denominator(10000)
        Fraction(1234, 5678)

        """
        # Algorithm notes: For any real number x, define a *best upper
        # approximation* to x to be a rational number p/q such that:
        #
        #   (1) p/q >= x, and
        #   (2) if p/q > r/s >= x then s > q, for any rational r/s.
        #
        # Define *best lower approximation* similarly.  Then it can be
        # proved that a rational number is a best upper or lower
        # approximation to x if, and only if, it is a convergent or
        # semiconvergent of the (unique shortest) continued fraction
        # associated to x.
        #
        # To find a best rational approximation with denominator <= M,
        # we find the best upper and lower approximations with
        # denominator <= M and take whichever of these is closer to x.
        # In the event of a tie, the bound with smaller denominator is
        # chosen.  If both denominators are equal (which can happen
        # only when max_denominator == 1 and self is midway between
        # two integers) the lower bound---i.e., the floor of self, is
        # taken.

        if max_denominator < 1:
            raise ValueError("max_denominator should be at least 1")
        if self._denominator <= max_denominator:
            return Fraction(self)

        p0, q0, p1, q1 = 0, 1, 1, 0
        n, d = self._numerator, self._denominator
        while True:
            a = n//d
            q2 = q0+a*q1
            if q2 > max_denominator:
                break
            p0, q0, p1, q1 = p1, q1, p0+a*p1, q2
            n, d = d, n-a*d

        k = (max_denominator-q0)//q1
        bound1 = Fraction(p0+k*p1, q0+k*q1)
        bound2 = Fraction(p1, q1)
        if abs(bound2 - self) <= abs(bound1-self):
            return bound2
        else:
            return bound1

    @property
    def numerator(a):
        return a._numerator

    @property
    def denominator(a):
        return a._denominator

    def __repr__(self):
        """repr(self)"""
        return ('Fraction(%r, %r)' % (self._numerator, self._denominator))

    def __str__(self):
        """str(self)"""
        if self._denominator == 1:
            return str(self._numerator)
        else:
            return '%s/%s' % (self._numerator, self._denominator)

    def _operator_fallbacks(monomorphic_operator, fallback_operator):
        """Generates forward and reverse operators given a purely-rational
        operator and a function from the operator module.

        Use this like:
        __op__, __rop__ = _operator_fallbacks(just_rational_op, operator.op)

        In general, we want to implement the arithmetic operations so
        that mixed-mode operations either call an implementation whose
        author knew about the types of both arguments, or convert both
        to the nearest built in type and do the operation there. In
        Fraction, that means that we define __add__ and __radd__ as:

            def __add__(self, other):
                # Both types have numerators/denominator attributes,
                # so do the operation directly
                if isinstance(other, (int, long, Fraction)):
                    return Fraction(self.numerator * other.denominator +
                                    other.numerator * self.denominator,
                                    self.denominator * other.denominator)
                # float and complex don't have those operations, but we
                # know about those types, so special case them.
                elif isinstance(other, float):
                    return float(self) + other
                elif isinstance(other, complex):
                    return complex(self) + other
                # Let the other type take over.
                return NotImplemented

            def __radd__(self, other):
                # radd handles more types than add because there's
                # nothing left to fall back to.
                if isinstance(other, Rational):
                    return Fraction(self.numerator * other.denominator +
                                    other.numerator * self.denominator,
                                    self.denominator * other.denominator)
                elif isinstance(other, Real):
                    return float(other) + float(self)
                elif isinstance(other, Complex):
                    return complex(other) + complex(self)
                return NotImplemented


        There are 5 different cases for a mixed-type addition on
        Fraction. I'll refer to all of the above code that doesn't
        refer to Fraction, float, or complex as "boilerplate". 'r'
        will be an instance of Fraction, which is a subtype of
        Rational (r : Fraction <: Rational), and b : B <:
        Complex. The first three involve 'r + b':

            1. If B <: Fraction, int, float, or complex, we handle
               that specially, and all is well.
            2. If Fraction falls back to the boilerplate code, and it
               were to return a value from __add__, we'd miss the
               possibility that B defines a more intelligent __radd__,
               so the boilerplate should return NotImplemented from
               __add__. In particular, we don't handle Rational
               here, even though we could get an exact answer, in case
               the other type wants to do something special.
            3. If B <: Fraction, Python tries B.__radd__ before
               Fraction.__add__. This is ok, because it was
               implemented with knowledge of Fraction, so it can
               handle those instances before delegating to Real or
               Complex.

        The next two situations describe 'b + r'. We assume that b
        didn't know about Fraction in its implementation, and that it
        uses similar boilerplate code:

            4. If B <: Rational, then __radd_ converts both to the
               builtin rational type (hey look, that's us) and
               proceeds.
            5. Otherwise, __radd__ tries to find the nearest common
               base ABC, and fall back to its builtin type. Since this
               class doesn't subclass a concrete type, there's no
               implementation to fall back to, so we need to try as
               hard as possible to return an actual value, or the user
               will get a TypeError.

        """
        def forward(a, b):
            if isinstance(b, (int, long, Fraction)):
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
            if isinstance(a, Rational):
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
        return Fraction(a.numerator * b.denominator +
                        b.numerator * a.denominator,
                        a.denominator * b.denominator)

    __add__, __radd__ = _operator_fallbacks(_add, operator.add)

    def _sub(a, b):
        """a - b"""
        return Fraction(a.numerator * b.denominator -
                        b.numerator * a.denominator,
                        a.denominator * b.denominator)

    __sub__, __rsub__ = _operator_fallbacks(_sub, operator.sub)

    def _mul(a, b):
        """a * b"""
        return Fraction(a.numerator * b.numerator, a.denominator * b.denominator)

    __mul__, __rmul__ = _operator_fallbacks(_mul, operator.mul)

    def _div(a, b):
        """a / b"""
        return Fraction(a.numerator * b.denominator,
                        a.denominator * b.numerator)

    __truediv__, __rtruediv__ = _operator_fallbacks(_div, operator.truediv)
    __div__, __rdiv__ = _operator_fallbacks(_div, operator.div)

    def __floordiv__(a, b):
        """a // b"""
        # Will be math.floor(a / b) in 3.0.
        div = a / b
        if isinstance(div, Rational):
            # trunc(math.floor(div)) doesn't work if the rational is
            # more precise than a float because the intermediate
            # rounding may cross an integer boundary.
            return div.numerator // div.denominator
        else:
            return math.floor(div)

    def __rfloordiv__(b, a):
        """a // b"""
        # Will be math.floor(a / b) in 3.0.
        div = a / b
        if isinstance(div, Rational):
            # trunc(math.floor(div)) doesn't work if the rational is
            # more precise than a float because the intermediate
            # rounding may cross an integer boundary.
            return div.numerator // div.denominator
        else:
            return math.floor(div)

    def __mod__(a, b):
        """a % b"""
        div = a // b
        return a - b * div

    def __rmod__(b, a):
        """a % b"""
        div = a // b
        return a - b * div

    def __pow__(a, b):
        """a ** b

        If b is not an integer, the result will be a float or complex
        since roots are generally irrational. If b is an integer, the
        result will be rational.

        """
        if isinstance(b, Rational):
            if b.denominator == 1:
                power = b.numerator
                if power >= 0:
                    return Fraction(a._numerator ** power,
                                    a._denominator ** power)
                else:
                    return Fraction(a._denominator ** -power,
                                    a._numerator ** -power)
            else:
                # A fractional power will generally produce an
                # irrational number.
                return float(a) ** float(b)
        else:
            return float(a) ** b

    def __rpow__(b, a):
        """a ** b"""
        if b._denominator == 1 and b._numerator >= 0:
            # If a is an int, keep it that way if possible.
            return a ** b._numerator

        if isinstance(a, Rational):
            return Fraction(a.numerator, a.denominator) ** b

        if b._denominator == 1:
            return a ** b._numerator

        return a ** float(b)

    def __pos__(a):
        """+a: Coerces a subclass instance to Fraction"""
        return Fraction(a._numerator, a._denominator)

    def __neg__(a):
        """-a"""
        return Fraction(-a._numerator, a._denominator)

    def __abs__(a):
        """abs(a)"""
        return Fraction(abs(a._numerator), a._denominator)

    def __trunc__(a):
        """trunc(a)"""
        if a._numerator < 0:
            return -(-a._numerator // a._denominator)
        else:
            return a._numerator // a._denominator

    def __hash__(self):
        """hash(self)

        Tricky because values that are exactly representable as a
        float must have the same hash as that float.

        """
        # XXX since this method is expensive, consider caching the result
        if self._denominator == 1:
            # Get integers right.
            return hash(self._numerator)
        # Expensive check, but definitely correct.
        if self == float(self):
            return hash(float(self))
        else:
            # Use tuple's hash to avoid a high collision rate on
            # simple fractions.
            return hash((self._numerator, self._denominator))

    def __eq__(a, b):
        """a == b"""
        if isinstance(b, Rational):
            return (a._numerator == b.numerator and
                    a._denominator == b.denominator)
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
            # XXX: If b <: Real but not <: Rational, this is likely
            # to fall back to a float. If the actual values differ by
            # less than MIN_FLOAT, this could falsely call them equal,
            # which would make <= inconsistent with ==. Better ways of
            # doing this are welcome.
            diff = a - b
        except TypeError:
            return NotImplemented
        if isinstance(diff, Rational):
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

    def __nonzero__(a):
        """a != 0"""
        return a._numerator != 0

    # support for pickling, copy, and deepcopy

    def __reduce__(self):
        return (self.__class__, (str(self),))

    def __copy__(self):
        if type(self) == Fraction:
            return self     # I'm immutable; therefore I am my own clone
        return self.__class__(self._numerator, self._denominator)

    def __deepcopy__(self, memo):
        if type(self) == Fraction:
            return self     # My components are also immutable
        return self.__class__(self._numerator, self._denominator)
