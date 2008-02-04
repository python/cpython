# Originally contributed by Sjoerd Mullender.
# Significantly modified by Jeffrey Yasskin <jyasskin at gmail.com>.

"""Rational, infinite-precision, real numbers."""

from __future__ import division
import math
import numbers
import operator
import re

__all__ = ["Rational"]

RationalAbc = numbers.Rational


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


class Rational(RationalAbc):
    """This class implements rational numbers.

    Rational(8, 6) will produce a rational number equivalent to
    4/3. Both arguments must be Integral. The numerator defaults to 0
    and the denominator defaults to 1 so that Rational(3) == 3 and
    Rational() == 0.

    Rationals can also be constructed from strings of the form
    '[-+]?[0-9]+((/|.)[0-9]+)?', optionally surrounded by spaces.

    """

    __slots__ = ('_numerator', '_denominator')

    # We're immutable, so use __new__ not __init__
    def __new__(cls, numerator=0, denominator=1):
        """Constructs a Rational.

        Takes a string like '3/2' or '1.5', another Rational, or a
        numerator/denominator pair.

        """
        self = super(Rational, cls).__new__(cls)

        if denominator == 1:
            if isinstance(numerator, basestring):
                # Handle construction from strings.
                input = numerator
                m = _RATIONAL_FORMAT.match(input)
                if m is None:
                    raise ValueError('Invalid literal for Rational: ' + input)
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

            elif (not isinstance(numerator, numbers.Integral) and
                  isinstance(numerator, RationalAbc)):
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

        g = gcd(numerator, denominator)
        self._numerator = int(numerator // g)
        self._denominator = int(denominator // g)
        return self

    @classmethod
    def from_float(cls, f):
        """Converts a finite float to a rational number, exactly.

        Beware that Rational.from_float(0.3) != Rational(3, 10).

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

    @classmethod
    def from_continued_fraction(cls, seq):
        'Build a Rational from a continued fraction expessed as a sequence'
        n, d = 1, 0
        for e in reversed(seq):
            n, d = d, n
            n += e * d
        return cls(n, d) if seq else cls(0)

    def as_continued_fraction(self):
        'Return continued fraction expressed as a list'
        n = self.numerator
        d = self.denominator
        cf = []
        while d:
            e = int(n // d)
            cf.append(e)
            n -= e * d
            n, d = d, n
        return cf

    def approximate(self, max_denominator):
        'Best rational approximation with a denominator <= max_denominator'
        # XXX First cut at algorithm
        # Still needs rounding rules as specified at
        #       http://en.wikipedia.org/wiki/Continued_fraction
        if self.denominator <= max_denominator:
            return self
        cf = self.as_continued_fraction()
        result = Rational(0)
        for i in range(1, len(cf)):
            new = self.from_continued_fraction(cf[:i])
            if new.denominator > max_denominator:
                break
            result = new
        return result

    @property
    def numerator(a):
        return a._numerator

    @property
    def denominator(a):
        return a._denominator

    def __repr__(self):
        """repr(self)"""
        return ('Rational(%r,%r)' % (self.numerator, self.denominator))

    def __str__(self):
        """str(self)"""
        if self.denominator == 1:
            return str(self.numerator)
        else:
            return '%s/%s' % (self.numerator, self.denominator)

    def _operator_fallbacks(monomorphic_operator, fallback_operator):
        """Generates forward and reverse operators given a purely-rational
        operator and a function from the operator module.

        Use this like:
        __op__, __rop__ = _operator_fallbacks(just_rational_op, operator.op)

        In general, we want to implement the arithmetic operations so
        that mixed-mode operations either call an implementation whose
        author knew about the types of both arguments, or convert both
        to the nearest built in type and do the operation there. In
        Rational, that means that we define __add__ and __radd__ as:

            def __add__(self, other):
                # Both types have numerators/denominator attributes,
                # so do the operation directly
                if isinstance(other, (int, long, Rational)):
                    return Rational(self.numerator * other.denominator +
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
                if isinstance(other, RationalAbc):
                    return Rational(self.numerator * other.denominator +
                                    other.numerator * self.denominator,
                                    self.denominator * other.denominator)
                elif isinstance(other, Real):
                    return float(other) + float(self)
                elif isinstance(other, Complex):
                    return complex(other) + complex(self)
                return NotImplemented


        There are 5 different cases for a mixed-type addition on
        Rational. I'll refer to all of the above code that doesn't
        refer to Rational, float, or complex as "boilerplate". 'r'
        will be an instance of Rational, which is a subtype of
        RationalAbc (r : Rational <: RationalAbc), and b : B <:
        Complex. The first three involve 'r + b':

            1. If B <: Rational, int, float, or complex, we handle
               that specially, and all is well.
            2. If Rational falls back to the boilerplate code, and it
               were to return a value from __add__, we'd miss the
               possibility that B defines a more intelligent __radd__,
               so the boilerplate should return NotImplemented from
               __add__. In particular, we don't handle RationalAbc
               here, even though we could get an exact answer, in case
               the other type wants to do something special.
            3. If B <: Rational, Python tries B.__radd__ before
               Rational.__add__. This is ok, because it was
               implemented with knowledge of Rational, so it can
               handle those instances before delegating to Real or
               Complex.

        The next two situations describe 'b + r'. We assume that b
        didn't know about Rational in its implementation, and that it
        uses similar boilerplate code:

            4. If B <: RationalAbc, then __radd_ converts both to the
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
            if isinstance(b, (int, long, Rational)):
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
    __div__, __rdiv__ = _operator_fallbacks(_div, operator.div)

    def __floordiv__(a, b):
        """a // b"""
        # Will be math.floor(a / b) in 3.0.
        div = a / b
        if isinstance(div, RationalAbc):
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
        if isinstance(div, RationalAbc):
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

    def __hash__(self):
        """hash(self)

        Tricky because values that are exactly representable as a
        float must have the same hash as that float.

        """
        # XXX since this method is expensive, consider caching the result
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

    def __nonzero__(a):
        """a != 0"""
        return a.numerator != 0

    # support for pickling, copy, and deepcopy

    def __reduce__(self):
        return (self.__class__, (str(self),))

    def __copy__(self):
        if type(self) == Rational:
            return self     # I'm immutable; therefore I am my own clone
        return self.__class__(self.numerator, self.denominator)

    def __deepcopy__(self, memo):
        if type(self) == Rational:
            return self     # My components are also immutable
        return self.__class__(self.numerator, self.denominator)
