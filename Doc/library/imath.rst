:mod:`imath` --- Mathematical functions for integer numbers
===========================================================

.. module:: imath
   :synopsis: Mathematical functions for integer numbers.

.. versionadded:: 3.8

**Source code:** :source:`Lib/imath.py`

--------------

This module provides access to the mathematical functions for integer arguments.
These functions accept integers and objects that implement the
:meth:`__index__` method which is used to convert the object to an integer
number.  They cannot be used with floating-point numbers or complex
numbers.

The following functions are provided by this module.  All return values are
integers.


.. function:: comb(n, k)

   Return the number of ways to choose *k* items from *n* items without repetition
   and without order.

   Also called the binomial coefficient. It is mathematically equal to the expression
   ``n! / (k! (n - k)!)``. It is equivalent to the coefficient of the *k*-th term in the
   polynomial expansion of the expression ``(1 + x) ** n``.

   Raises :exc:`TypeError` if the arguments not integers.
   Raises :exc:`ValueError` if the arguments are negative or if *k* > *n*.


.. function:: gcd(a, b)

   Return the greatest common divisor of the integers *a* and *b*.  If either
   *a* or *b* is nonzero, then the value of ``gcd(a, b)`` is the largest
   positive integer that divides both *a* and *b*.  ``gcd(0, 0)`` returns
   ``0``.


.. function:: ilog2(n)

   Return the integer base 2 logarithm of the positive integer *n*. This is the
   floor of the exact base 2 logarithm root of *n*, or equivalently the
   greatest integer *k* such that
   2\ :sup:`k` |nbsp| ≤ |nbsp| *n* |nbsp| < |nbsp| 2\ :sup:`k+1`.

   It is equivalent to ``n.bit_length() - 1`` for positive *n*.


.. function:: isqrt(n)

   Return the integer square root of the nonnegative integer *n*. This is the
   floor of the exact square root of *n*, or equivalently the greatest integer
   *a* such that *a*\ ² |nbsp| ≤ |nbsp| *n*.

   For some applications, it may be more convenient to have the least integer
   *a* such that *n* |nbsp| ≤ |nbsp| *a*\ ², or in other words the ceiling of
   the exact square root of *n*. For positive *n*, this can be computed using
   ``a = 1 + isqrt(n - 1)``.


.. function:: perm(n, k)

   Return the number of ways to choose *k* items from *n* items
   without repetition and with order.

   It is mathematically equal to the expression ``n! / (n - k)!``.

   Raises :exc:`TypeError` if the arguments not integers.
   Raises :exc:`ValueError` if the arguments are negative or if *k* > *n*.

.. |nbsp| unicode:: 0xA0
   :trim:
