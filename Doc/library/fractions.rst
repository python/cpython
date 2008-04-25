
:mod:`fractions` --- Rational numbers
=====================================

.. module:: fractions
   :synopsis: Rational numbers.
.. moduleauthor:: Jeffrey Yasskin <jyasskin at gmail.com>
.. sectionauthor:: Jeffrey Yasskin <jyasskin at gmail.com>
.. versionadded:: 2.6


The :mod:`fractions` module defines an immutable, infinite-precision
Fraction number class.


.. class:: Fraction(numerator=0, denominator=1)
           Fraction(other_fraction)
           Fraction(string)

   The first version requires that *numerator* and *denominator* are
   instances of :class:`numbers.Integral` and returns a new
   ``Fraction`` representing ``numerator/denominator``. If
   *denominator* is :const:`0`, raises a :exc:`ZeroDivisionError`. The
   second version requires that *other_fraction* is an instance of
   :class:`numbers.Rational` and returns an instance of
   :class:`Fraction` with the same value. The third version expects a
   string of the form ``[-+]?[0-9]+(/[0-9]+)?``, optionally surrounded
   by spaces.

   Implements all of the methods and operations from
   :class:`numbers.Rational` and is immutable and hashable.


   .. method:: from_float(flt)

      This classmethod constructs a :class:`Fraction` representing the exact
      value of *flt*, which must be a :class:`float`. Beware that
      ``Fraction.from_float(0.3)`` is not the same value as ``Fraction(3, 10)``


   .. method:: from_decimal(dec)

      This classmethod constructs a :class:`Fraction` representing the exact
      value of *dec*, which must be a :class:`decimal.Decimal`.


   .. method:: limit_denominator(max_denominator=1000000)

      Finds and returns the closest :class:`Fraction` to ``self`` that has
      denominator at most max_denominator.  This method is useful for finding
      rational approximations to a given floating-point number:

         >>> from fractions import Fraction
         >>> Fraction('3.1415926535897932').limit_denominator(1000)
         Fraction(355L, 113L)

      or for recovering a rational number that's represented as a float:

         >>> from math import pi, cos
         >>> Fraction.from_float(cos(pi/3))
         Fraction(4503599627370497L, 9007199254740992L)
         >>> Fraction.from_float(cos(pi/3)).limit_denominator()
         Fraction(1L, 2L)


   .. method:: __floor__()

      Returns the greatest :class:`int` ``<= self``. Will be accessible through
      :func:`math.floor` in Py3k.


   .. method:: __ceil__()

      Returns the least :class:`int` ``>= self``. Will be accessible through
      :func:`math.ceil` in Py3k.


   .. method:: __round__()
               __round__(ndigits)

      The first version returns the nearest :class:`int` to ``self``, rounding
      half to even. The second version rounds ``self`` to the nearest multiple
      of ``Fraction(1, 10**ndigits)`` (logically, if ``ndigits`` is negative),
      again rounding half toward even. Will be accessible through :func:`round`
      in Py3k.


.. seealso::

   Module :mod:`numbers`
      The abstract base classes making up the numeric tower.
