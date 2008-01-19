
:mod:`rational` --- Rational numbers
====================================

.. module:: rational
   :synopsis: Rational numbers.
.. moduleauthor:: Jeffrey Yasskin <jyasskin at gmail.com>
.. sectionauthor:: Jeffrey Yasskin <jyasskin at gmail.com>
.. versionadded:: 2.6


The :mod:`rational` module defines an immutable, infinite-precision
Rational number class.


.. class:: Rational(numerator=0, denominator=1)
           Rational(other_rational)
           Rational(string)

   The first version requires that *numerator* and *denominator* are
   instances of :class:`numbers.Integral` and returns a new
   ``Rational`` representing ``numerator/denominator``. If
   *denominator* is :const:`0`, raises a :exc:`ZeroDivisionError`. The
   second version requires that *other_rational* is an instance of
   :class:`numbers.Rational` and returns an instance of
   :class:`Rational` with the same value. The third version expects a
   string of the form ``[-+]?[0-9]+(/[0-9]+)?``, optionally surrounded
   by spaces.

   Implements all of the methods and operations from
   :class:`numbers.Rational` and is immutable and hashable.


.. method:: Rational.from_float(flt)

   This classmethod constructs a :class:`Rational` representing the
   exact value of *flt*, which must be a :class:`float`. Beware that
   ``Rational.from_float(0.3)`` is not the same value as ``Rational(3,
   10)``


.. method:: Rational.from_decimal(dec)

   This classmethod constructs a :class:`Rational` representing the
   exact value of *dec*, which must be a
   :class:`decimal.Decimal`.


.. method:: Rational.__floor__()

   Returns the greatest :class:`int` ``<= self``. Will be accessible
   through :func:`math.floor` in Py3k.


.. method:: Rational.__ceil__()

   Returns the least :class:`int` ``>= self``. Will be accessible
   through :func:`math.ceil` in Py3k.


.. method:: Rational.__round__()
            Rational.__round__(ndigits)

   The first version returns the nearest :class:`int` to ``self``,
   rounding half to even. The second version rounds ``self`` to the
   nearest multiple of ``Rational(1, 10**ndigits)`` (logically, if
   ``ndigits`` is negative), again rounding half toward even. Will be
   accessible through :func:`round` in Py3k.


.. seealso::

   Module :mod:`numbers`
      The abstract base classes making up the numeric tower.

