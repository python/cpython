:mod:`fractions` --- Rational numbers
=====================================

.. module:: fractions
   :synopsis: Rational numbers.
.. moduleauthor:: Jeffrey Yasskin <jyasskin at gmail.com>
.. sectionauthor:: Jeffrey Yasskin <jyasskin at gmail.com>
.. versionadded:: 2.6

**Source code:** :source:`Lib/fractions.py`

--------------

The :mod:`fractions` module provides support for rational number arithmetic.


A Fraction instance can be constructed from a pair of integers, from
another rational number, or from a string.

.. class:: Fraction(numerator=0, denominator=1)
           Fraction(other_fraction)
           Fraction(float)
           Fraction(decimal)
           Fraction(string)

   The first version requires that *numerator* and *denominator* are instances
   of :class:`numbers.Rational` and returns a new :class:`Fraction` instance
   with value ``numerator/denominator``. If *denominator* is :const:`0`, it
   raises a :exc:`ZeroDivisionError`. The second version requires that
   *other_fraction* is an instance of :class:`numbers.Rational` and returns a
   :class:`Fraction` instance with the same value.  The next two versions accept
   either a :class:`float` or a :class:`decimal.Decimal` instance, and return a
   :class:`Fraction` instance with exactly the same value.  Note that due to the
   usual issues with binary floating-point (see :ref:`tut-fp-issues`), the
   argument to ``Fraction(1.1)`` is not exactly equal to 11/10, and so
   ``Fraction(1.1)`` does *not* return ``Fraction(11, 10)`` as one might expect.
   (But see the documentation for the :meth:`limit_denominator` method below.)
   The last version of the constructor expects a string or unicode instance.
   The usual form for this instance is::

      [sign] numerator ['/' denominator]

   where the optional ``sign`` may be either '+' or '-' and
   ``numerator`` and ``denominator`` (if present) are strings of
   decimal digits.  In addition, any string that represents a finite
   value and is accepted by the :class:`float` constructor is also
   accepted by the :class:`Fraction` constructor.  In either form the
   input string may also have leading and/or trailing whitespace.
   Here are some examples::

      >>> from fractions import Fraction
      >>> Fraction(16, -10)
      Fraction(-8, 5)
      >>> Fraction(123)
      Fraction(123, 1)
      >>> Fraction()
      Fraction(0, 1)
      >>> Fraction('3/7')
      Fraction(3, 7)
      >>> Fraction(' -3/7 ')
      Fraction(-3, 7)
      >>> Fraction('1.414213 \t\n')
      Fraction(1414213, 1000000)
      >>> Fraction('-.125')
      Fraction(-1, 8)
      >>> Fraction('7e-6')
      Fraction(7, 1000000)
      >>> Fraction(2.25)
      Fraction(9, 4)
      >>> Fraction(1.1)
      Fraction(2476979795053773, 2251799813685248)
      >>> from decimal import Decimal
      >>> Fraction(Decimal('1.1'))
      Fraction(11, 10)


   The :class:`Fraction` class inherits from the abstract base class
   :class:`numbers.Rational`, and implements all of the methods and
   operations from that class.  :class:`Fraction` instances are hashable,
   and should be treated as immutable.  In addition,
   :class:`Fraction` has the following methods:

   .. versionchanged:: 2.7
      The :class:`Fraction` constructor now accepts :class:`float` and
      :class:`decimal.Decimal` instances.


   .. method:: from_float(flt)

      This class method constructs a :class:`Fraction` representing the exact
      value of *flt*, which must be a :class:`float`. Beware that
      ``Fraction.from_float(0.3)`` is not the same value as ``Fraction(3, 10)``.

      .. note:: From Python 2.7 onwards, you can also construct a
         :class:`Fraction` instance directly from a :class:`float`.


   .. method:: from_decimal(dec)

      This class method constructs a :class:`Fraction` representing the exact
      value of *dec*, which must be a :class:`decimal.Decimal`.

      .. note:: From Python 2.7 onwards, you can also construct a
         :class:`Fraction` instance directly from a :class:`decimal.Decimal`
         instance.


   .. method:: limit_denominator(max_denominator=1000000)

      Finds and returns the closest :class:`Fraction` to ``self`` that has
      denominator at most max_denominator.  This method is useful for finding
      rational approximations to a given floating-point number:

         >>> from fractions import Fraction
         >>> Fraction('3.1415926535897932').limit_denominator(1000)
         Fraction(355, 113)

      or for recovering a rational number that's represented as a float:

         >>> from math import pi, cos
         >>> Fraction(cos(pi/3))
         Fraction(4503599627370497, 9007199254740992)
         >>> Fraction(cos(pi/3)).limit_denominator()
         Fraction(1, 2)
         >>> Fraction(1.1).limit_denominator()
         Fraction(11, 10)


.. function:: gcd(a, b)

   Return the greatest common divisor of the integers *a* and *b*.  If either
   *a* or *b* is nonzero, then the absolute value of ``gcd(a, b)`` is the
   largest integer that divides both *a* and *b*.  ``gcd(a,b)`` has the same
   sign as *b* if *b* is nonzero; otherwise it takes the sign of *a*.  ``gcd(0,
   0)`` returns ``0``.


.. seealso::

   Module :mod:`numbers`
      The abstract base classes making up the numeric tower.
