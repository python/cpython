# Copyright 2007 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Abstract Base Classes (ABCs) for numbers, according to PEP 3141."""

from abc import ABCMeta, abstractmethod, abstractproperty

__all__ = ["Number", "Exact", "Inexact",
           "Complex", "Real", "Rational", "Integral",
           ]


class Number(metaclass=ABCMeta):
    """All numbers inherit from this class.

    If you just want to check if an argument x is a number, without
    caring what kind, use isinstance(x, Number).
    """


class Exact(Number):
    """Operations on instances of this type are exact.

    As long as the result of a homogenous operation is of the same
    type, you can assume that it was computed exactly, and there are
    no round-off errors. Laws like commutativity and associativity
    hold.
    """

Exact.register(int)


class Inexact(Number):
    """Operations on instances of this type are inexact.

    Given X, an instance of Inexact, it is possible that (X + -X) + 3
    == 3, but X + (-X + 3) == 0. The exact form this error takes will
    vary by type, but it's generally unsafe to compare this type for
    equality.
    """

Inexact.register(complex)
Inexact.register(float)


class Complex(Number):
    """Complex defines the operations that work on the builtin complex type.

    In short, those are: a conversion to complex, .real, .imag, +, -,
    *, /, abs(), .conjugate, ==, and !=.

    If it is given heterogenous arguments, and doesn't have special
    knowledge about them, it should fall back to the builtin complex
    type as described below.
    """

    @abstractmethod
    def __complex__(self):
        """Return a builtin complex instance."""

    def __bool__(self):
        """True if self != 0."""
        return self != 0

    @abstractproperty
    def real(self):
        """Retrieve the real component of this number.

        This should subclass Real.
        """
        raise NotImplementedError

    @abstractproperty
    def imag(self):
        """Retrieve the real component of this number.

        This should subclass Real.
        """
        raise NotImplementedError

    @abstractmethod
    def __add__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __radd__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __neg__(self):
        raise NotImplementedError

    def __pos__(self):
        return self

    def __sub__(self, other):
        return self + -other

    def __rsub__(self, other):
        return -self + other

    @abstractmethod
    def __mul__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __rmul__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __div__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __rdiv__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __pow__(self, exponent):
        """Like division, a**b should promote to complex when necessary."""
        raise NotImplementedError

    @abstractmethod
    def __rpow__(self, base):
        raise NotImplementedError

    @abstractmethod
    def __abs__(self):
        """Returns the Real distance from 0."""
        raise NotImplementedError

    @abstractmethod
    def conjugate(self):
        """(x+y*i).conjugate() returns (x-y*i)."""
        raise NotImplementedError

    @abstractmethod
    def __eq__(self, other):
        raise NotImplementedError

    def __ne__(self, other):
        return not (self == other)

Complex.register(complex)


class Real(Complex):
    """To Complex, Real adds the operations that work on real numbers.

    In short, those are: a conversion to float, trunc(), divmod,
    %, <, <=, >, and >=.

    Real also provides defaults for the derived operations.
    """

    @abstractmethod
    def __float__(self):
        """Any Real can be converted to a native float object."""
        raise NotImplementedError

    @abstractmethod
    def __trunc__(self):
        """Truncates self to an Integral.

        Returns an Integral i such that:
          * i>0 iff self>0
          * abs(i) <= abs(self).
        """
        raise NotImplementedError

    def __divmod__(self, other):
        """The pair (self // other, self % other).

        Sometimes this can be computed faster than the pair of
        operations.
        """
        return (self // other, self % other)

    def __rdivmod__(self, other):
        """The pair (self // other, self % other).

        Sometimes this can be computed faster than the pair of
        operations.
        """
        return (other // self, other % self)

    @abstractmethod
    def __floordiv__(self, other):
        """The floor() of self/other."""
        raise NotImplementedError

    @abstractmethod
    def __rfloordiv__(self, other):
        """The floor() of other/self."""
        raise NotImplementedError

    @abstractmethod
    def __mod__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __rmod__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __lt__(self, other):
        """< on Reals defines a total ordering, except perhaps for NaN."""
        raise NotImplementedError

    def __le__(self, other):
        raise NotImplementedError

    # Concrete implementations of Complex abstract methods.
    def __complex__(self):
        return complex(float(self))

    @property
    def real(self):
        return self

    @property
    def imag(self):
        return 0

    def conjugate(self):
        """Conjugate is a no-op for Reals."""
        return self

Real.register(float)


class Rational(Real, Exact):
    """.numerator and .denominator should be in lowest terms."""

    @abstractproperty
    def numerator(self):
        raise NotImplementedError

    @abstractproperty
    def denominator(self):
        raise NotImplementedError

    # Concrete implementation of Real's conversion to float.
    def __float__(self):
        return self.numerator / self.denominator


class Integral(Rational):
    """Integral adds a conversion to int and the bit-string operations."""

    @abstractmethod
    def __int__(self):
        raise NotImplementedError

    def __index__(self):
        return int(self)

    @abstractmethod
    def __pow__(self, exponent, modulus):
        """self ** exponent % modulus, but maybe faster.

        Implement this if you want to support the 3-argument version
        of pow(). Otherwise, just implement the 2-argument version
        described in Complex. Raise a TypeError if exponent < 0 or any
        argument isn't Integral.
        """
        raise NotImplementedError

    @abstractmethod
    def __lshift__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __rlshift__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __rshift__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __rrshift__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __and__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __rand__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __xor__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __rxor__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __or__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __ror__(self, other):
        raise NotImplementedError

    @abstractmethod
    def __invert__(self):
        raise NotImplementedError

    # Concrete implementations of Rational and Real abstract methods.
    def __float__(self):
        return float(int(self))

    @property
    def numerator(self):
        return self

    @property
    def denominator(self):
        return 1

Integral.register(int)
