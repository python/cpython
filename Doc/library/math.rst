:mod:`!math` --- Mathematical functions
=======================================

.. module:: math
   :synopsis: Mathematical functions (sin() etc.).

.. testsetup::

   from math import fsum

--------------

This module provides access to the mathematical functions defined by the C
standard.

These functions cannot be used with complex numbers; use the functions of the
same name from the :mod:`cmath` module if you require support for complex
numbers.  The distinction between functions which support complex numbers and
those which don't is made since most users do not want to learn quite as much
mathematics as required to understand complex numbers.  Receiving an exception
instead of a complex result allows earlier detection of the unexpected complex
number used as a parameter, so that the programmer can determine how and why it
was generated in the first place.

The following functions are provided by this module.  Except when explicitly
noted otherwise, all return values are floats.


====================================================  ============================================
**Number-theoretic functions**
--------------------------------------------------------------------------------------------------
:func:`comb(n, k) <comb>`                             Number of ways to choose *k* items from *n* items without repetition and without order
:func:`factorial(n) <factorial>`                      *n* factorial
:func:`gcd(*integers) <gcd>`                          Greatest common divisor of the integer arguments
:func:`isqrt(n) <isqrt>`                              Integer square root of a nonnegative integer *n*
:func:`lcm(*integers) <lcm>`                          Least common multiple of the integer arguments
:func:`perm(n, k) <perm>`                             Number of ways to choose *k* items from *n* items without repetition and with order

**Floating point arithmetic**
--------------------------------------------------------------------------------------------------
:func:`ceil(x) <ceil>`                                Ceiling of *x*, the smallest integer greater than or equal to *x*
:func:`fabs(x) <fabs>`                                Absolute value of *x*
:func:`floor(x)  <floor>`                             Floor of *x*, the largest integer less than or equal to *x*
:func:`fma(x, y, z) <fma>`                            Fused multiply-add operation: ``(x * y) + z``
:func:`fmod(x, y) <fmod>`                             Remainder of division ``x / y``
:func:`modf(x) <modf>`                                Fractional and integer parts of *x*
:func:`remainder(x, y) <remainder>`                   Remainder of *x* with respect to *y*
:func:`trunc(x) <trunc>`                              Integer part of *x*

**Floating point manipulation functions**
--------------------------------------------------------------------------------------------------
:func:`copysign(x, y) <copysign>`                     Magnitude (absolute value) of *x* with the sign of *y*
:func:`frexp(x) <frexp>`                              Mantissa and exponent of *x*
:func:`isclose(a, b, rel_tol, abs_tol) <isclose>`     Check if the values *a* and *b* are close to each other
:func:`isfinite(x) <isfinite>`                        Check if *x* is neither an infinity nor a NaN
:func:`isinf(x) <isinf>`                              Check if *x* is a positive or negative infinity
:func:`isnan(x) <isnan>`                              Check if *x* is a NaN  (not a number)
:func:`ldexp(x, i) <ldexp>`                           ``x * (2**i)``, inverse of function :func:`frexp`
:func:`nextafter(x, y, steps) <nextafter>`            Floating-point value *steps* steps after *x* towards *y*
:func:`ulp(x) <ulp>`                                  Value of the least significant bit of *x*

**Power, exponential and logarithmic functions**
--------------------------------------------------------------------------------------------------
:func:`cbrt(x) <cbrt>`                                Cube root of *x*
:func:`exp(x) <exp>`                                  *e* raised to the power *x*
:func:`exp2(x) <exp2>`                                *2* raised to the power *x*
:func:`expm1(x) <expm1>`                              *e* raised to the power *x*, minus 1
:func:`log(x, base) <log>`                            Logarithm of *x* to the given base (*e* by default)
:func:`log1p(x) <log1p>`                              Natural logarithm of *1+x* (base *e*)
:func:`log2(x) <log2>`                                Base-2 logarithm of *x*
:func:`log10(x) <log10>`                              Base-10 logarithm of *x*
:func:`pow(x, y) <math.pow>`                          *x* raised to the power *y*
:func:`sqrt(x) <sqrt>`                                Square root of *x*

**Summation and product functions**
--------------------------------------------------------------------------------------------------
:func:`dist(p, q) <dist>`                             Euclidean distance between two points *p* and *q* given as an iterable of coordinates
:func:`fsum(iterable) <fsum>`                         Sum of values in the input *iterable*
:func:`hypot(*coordinates) <hypot>`                   Euclidean norm of an iterable of coordinates
:func:`prod(iterable, start) <prod>`                  Product of elements in the input *iterable* with a *start* value
:func:`sumprod(p, q) <sumprod>`                       Sum of products from two iterables *p* and *q*

**Angular conversion**
--------------------------------------------------------------------------------------------------
:func:`degrees(x) <degrees>`                          Convert angle *x* from radians to degrees
:func:`radians(x) <radians>`                          Convert angle *x* from degrees to radians

**Trigonometric functions**
--------------------------------------------------------------------------------------------------
:func:`acos(x) <acos>`                                Arc cosine of *x*
:func:`asin(x) <asin>`                                Arc sine of *x*
:func:`atan(x) <atan>`                                Arc tangent of *x*
:func:`atan2(y, x) <atan2>`                           ``atan(y / x)``
:func:`cos(x) <cos>`                                  Cosine of *x*
:func:`sin(x) <sin>`                                  Sine of *x*
:func:`tan(x) <tan>`                                  Tangent of *x*

**Hyperbolic functions**
--------------------------------------------------------------------------------------------------
:func:`acosh(x) <acosh>`                              Inverse hyperbolic cosine of *x*
:func:`asinh(x) <asinh>`                              Inverse hyperbolic sine of *x*
:func:`atanh(x) <atanh>`                              Inverse hyperbolic tangent of *x*
:func:`cosh(x) <cosh>`                                Hyperbolic cosine of *x*
:func:`sinh(x) <sinh>`                                Hyperbolic sine of *x*
:func:`tanh(x) <tanh>`                                Hyperbolic tangent of *x*

**Special functions**
--------------------------------------------------------------------------------------------------
:func:`erf(x) <erf>`                                  `Error function <https://en.wikipedia.org/wiki/Error_function>`_ at *x*
:func:`erfc(x) <erfc>`                                `Complementary error function <https://en.wikipedia.org/wiki/Error_function>`_ at *x*
:func:`gamma(x) <gamma>`                              `Gamma function <https://en.wikipedia.org/wiki/Gamma_function>`_ at *x*
:func:`lgamma(x) <lgamma>`                            Natural logarithm of the absolute value of the `Gamma function <https://en.wikipedia.org/wiki/Gamma_function>`_ at *x*

**Constants**
--------------------------------------------------------------------------------------------------
:data:`pi`                                            *π* = 3.141592...
:data:`e`                                             *e* = 2.718281...
:data:`tau`                                           *τ* = 2\ *π* = 6.283185...
:data:`inf`                                           Positive infinity
:data:`nan`                                           "Not a number" (NaN)
====================================================  ============================================


Number-theoretic functions
--------------------------

.. function:: comb(n, k)

   Return the number of ways to choose *k* items from *n* items without repetition
   and without order.

   Evaluates to ``n! / (k! * (n - k)!)`` when ``k <= n`` and evaluates
   to zero when ``k > n``.

   Also called the binomial coefficient because it is equivalent
   to the coefficient of k-th term in polynomial expansion of
   ``(1 + x)ⁿ``.

   Raises :exc:`TypeError` if either of the arguments are not integers.
   Raises :exc:`ValueError` if either of the arguments are negative.

   .. versionadded:: 3.8


.. function:: factorial(n)

   Return factorial of the nonnegative integer *n*.

   .. versionchanged:: 3.10
      Floats with integral values (like ``5.0``) are no longer accepted.


.. function:: gcd(*integers)

   Return the greatest common divisor of the specified integer arguments.
   If any of the arguments is nonzero, then the returned value is the largest
   positive integer that is a divisor of all arguments.  If all arguments
   are zero, then the returned value is ``0``.  ``gcd()`` without arguments
   returns ``0``.

   .. versionadded:: 3.5

   .. versionchanged:: 3.9
      Added support for an arbitrary number of arguments. Formerly, only two
      arguments were supported.


.. function:: isqrt(n)

   Return the integer square root of the nonnegative integer *n*. This is the
   floor of the exact square root of *n*, or equivalently the greatest integer
   *a* such that *a*\ ² |nbsp| ≤ |nbsp| *n*.

   For some applications, it may be more convenient to have the least integer
   *a* such that *n* |nbsp| ≤ |nbsp| *a*\ ², or in other words the ceiling of
   the exact square root of *n*. For positive *n*, this can be computed using
   ``a = 1 + isqrt(n - 1)``.

   .. versionadded:: 3.8


.. function:: lcm(*integers)

   Return the least common multiple of the specified integer arguments.
   If all arguments are nonzero, then the returned value is the smallest
   positive integer that is a multiple of all arguments.  If any of the arguments
   is zero, then the returned value is ``0``.  ``lcm()`` without arguments
   returns ``1``.

   .. versionadded:: 3.9


.. function:: perm(n, k=None)

   Return the number of ways to choose *k* items from *n* items
   without repetition and with order.

   Evaluates to ``n! / (n - k)!`` when ``k <= n`` and evaluates
   to zero when ``k > n``.

   If *k* is not specified or is ``None``, then *k* defaults to *n*
   and the function returns ``n!``.

   Raises :exc:`TypeError` if either of the arguments are not integers.
   Raises :exc:`ValueError` if either of the arguments are negative.

   .. versionadded:: 3.8


Floating point arithmetic
-------------------------

.. function:: ceil(x)

   Return the ceiling of *x*, the smallest integer greater than or equal to *x*.
   If *x* is not a float, delegates to :meth:`x.__ceil__ <object.__ceil__>`,
   which should return an :class:`~numbers.Integral` value.


.. function:: fabs(x)

   Return the absolute value of *x*.


.. function:: floor(x)

   Return the floor of *x*, the largest integer less than or equal to *x*.  If
   *x* is not a float, delegates to :meth:`x.__floor__ <object.__floor__>`, which
   should return an :class:`~numbers.Integral` value.


.. function:: fma(x, y, z)

   Fused multiply-add operation. Return ``(x * y) + z``, computed as though with
   infinite precision and range followed by a single round to the ``float``
   format. This operation often provides better accuracy than the direct
   expression ``(x * y) + z``.

   This function follows the specification of the fusedMultiplyAdd operation
   described in the IEEE 754 standard. The standard leaves one case
   implementation-defined, namely the result of ``fma(0, inf, nan)``
   and ``fma(inf, 0, nan)``. In these cases, ``math.fma`` returns a NaN,
   and does not raise any exception.

   .. versionadded:: 3.13


.. function:: fmod(x, y)

   Return the floating-point remainder of ``x / y``,
   as defined by the platform C library function ``fmod(x, y)``. Note that the
   Python expression ``x % y`` may not return the same result.  The intent of the C
   standard is that ``fmod(x, y)`` be exactly (mathematically; to infinite
   precision) equal to ``x - n*y`` for some integer *n* such that the result has
   the same sign as *x* and magnitude less than ``abs(y)``.  Python's ``x % y``
   returns a result with the sign of *y* instead, and may not be exactly computable
   for float arguments. For example, ``fmod(-1e-100, 1e100)`` is ``-1e-100``, but
   the result of Python's ``-1e-100 % 1e100`` is ``1e100-1e-100``, which cannot be
   represented exactly as a float, and rounds to the surprising ``1e100``.  For
   this reason, function :func:`fmod` is generally preferred when working with
   floats, while Python's ``x % y`` is preferred when working with integers.


.. function:: modf(x)

   Return the fractional and integer parts of *x*.  Both results carry the sign
   of *x* and are floats.

   Note that :func:`modf` has a different call/return pattern
   than its C equivalents: it takes a single argument and return a pair of
   values, rather than returning its second return value through an 'output
   parameter' (there is no such thing in Python).


.. function:: remainder(x, y)

   Return the IEEE 754-style remainder of *x* with respect to *y*.  For
   finite *x* and finite nonzero *y*, this is the difference ``x - n*y``,
   where ``n`` is the closest integer to the exact value of the quotient ``x /
   y``.  If ``x / y`` is exactly halfway between two consecutive integers, the
   nearest *even* integer is used for ``n``.  The remainder ``r = remainder(x,
   y)`` thus always satisfies ``abs(r) <= 0.5 * abs(y)``.

   Special cases follow IEEE 754: in particular, ``remainder(x, math.inf)`` is
   *x* for any finite *x*, and ``remainder(x, 0)`` and
   ``remainder(math.inf, x)`` raise :exc:`ValueError` for any non-NaN *x*.
   If the result of the remainder operation is zero, that zero will have
   the same sign as *x*.

   On platforms using IEEE 754 binary floating point, the result of this
   operation is always exactly representable: no rounding error is introduced.

   .. versionadded:: 3.7


.. function:: trunc(x)

   Return *x* with the fractional part
   removed, leaving the integer part.  This rounds toward 0: ``trunc()`` is
   equivalent to :func:`floor` for positive *x*, and equivalent to :func:`ceil`
   for negative *x*. If *x* is not a float, delegates to :meth:`x.__trunc__
   <object.__trunc__>`, which should return an :class:`~numbers.Integral` value.


For the :func:`ceil`, :func:`floor`, and :func:`modf` functions, note that *all*
floating-point numbers of sufficiently large magnitude are exact integers.
Python floats typically carry no more than 53 bits of precision (the same as the
platform C double type), in which case any float *x* with ``abs(x) >= 2**52``
necessarily has no fractional bits.


Floating point manipulation functions
-------------------------------------

.. function:: copysign(x, y)

   Return a float with the magnitude (absolute value) of *x* but the sign of
   *y*.  On platforms that support signed zeros, ``copysign(1.0, -0.0)``
   returns *-1.0*.


.. function:: frexp(x)

   Return the mantissa and exponent of *x* as the pair ``(m, e)``.  *m* is a float
   and *e* is an integer such that ``x == m * 2**e`` exactly. If *x* is zero,
   returns ``(0.0, 0)``, otherwise ``0.5 <= abs(m) < 1``.  This is used to "pick
   apart" the internal representation of a float in a portable way.

   Note that :func:`frexp` has a different call/return pattern
   than its C equivalents: it takes a single argument and return a pair of
   values, rather than returning its second return value through an 'output
   parameter' (there is no such thing in Python).

.. function:: isclose(a, b, *, rel_tol=1e-09, abs_tol=0.0)

   Return ``True`` if the values *a* and *b* are close to each other and
   ``False`` otherwise.

   Whether or not two values are considered close is determined according to
   given absolute and relative tolerances.  If no errors occur, the result will
   be: ``abs(a-b) <= max(rel_tol * max(abs(a), abs(b)), abs_tol)``.

   *rel_tol* is the relative tolerance -- it is the maximum allowed difference
   between *a* and *b*, relative to the larger absolute value of *a* or *b*.
   For example, to set a tolerance of 5%, pass ``rel_tol=0.05``.  The default
   tolerance is ``1e-09``, which assures that the two values are the same
   within about 9 decimal digits.  *rel_tol* must be nonnegative and less
   than ``1.0``.

   *abs_tol* is the absolute tolerance; it defaults to ``0.0`` and it must be
   nonnegative.  When comparing ``x`` to ``0.0``, ``isclose(x, 0)`` is computed
   as ``abs(x) <= rel_tol  * abs(x)``, which is ``False`` for any nonzero ``x`` and
   *rel_tol* less than ``1.0``.  So add an appropriate positive *abs_tol* argument
   to the call.

   The IEEE 754 special values of ``NaN``, ``inf``, and ``-inf`` will be
   handled according to IEEE rules.  Specifically, ``NaN`` is not considered
   close to any other value, including ``NaN``.  ``inf`` and ``-inf`` are only
   considered close to themselves.

   .. versionadded:: 3.5

   .. seealso::

      :pep:`485` -- A function for testing approximate equality


.. function:: isfinite(x)

   Return ``True`` if *x* is neither an infinity nor a NaN, and
   ``False`` otherwise.  (Note that ``0.0`` *is* considered finite.)

   .. versionadded:: 3.2


.. function:: isinf(x)

   Return ``True`` if *x* is a positive or negative infinity, and
   ``False`` otherwise.


.. function:: isnan(x)

   Return ``True`` if *x* is a NaN (not a number), and ``False`` otherwise.


.. function:: ldexp(x, i)

   Return ``x * (2**i)``.  This is essentially the inverse of function
   :func:`frexp`.


.. function:: nextafter(x, y, steps=1)

   Return the floating-point value *steps* steps after *x* towards *y*.

   If *x* is equal to *y*, return *y*, unless *steps* is zero.

   Examples:

   * ``math.nextafter(x, math.inf)`` goes up: towards positive infinity.
   * ``math.nextafter(x, -math.inf)`` goes down: towards minus infinity.
   * ``math.nextafter(x, 0.0)`` goes towards zero.
   * ``math.nextafter(x, math.copysign(math.inf, x))`` goes away from zero.

   See also :func:`math.ulp`.

   .. versionadded:: 3.9

   .. versionchanged:: 3.12
      Added the *steps* argument.


.. function:: ulp(x)

   Return the value of the least significant bit of the float *x*:

   * If *x* is a NaN (not a number), return *x*.
   * If *x* is negative, return ``ulp(-x)``.
   * If *x* is a positive infinity, return *x*.
   * If *x* is equal to zero, return the smallest positive
     *denormalized* representable float (smaller than the minimum positive
     *normalized* float, :data:`sys.float_info.min <sys.float_info>`).
   * If *x* is equal to the largest positive representable float,
     return the value of the least significant bit of *x*, such that the first
     float smaller than *x* is ``x - ulp(x)``.
   * Otherwise (*x* is a positive finite number), return the value of the least
     significant bit of *x*, such that the first float bigger than *x*
     is ``x + ulp(x)``.

   ULP stands for "Unit in the Last Place".

   See also :func:`math.nextafter` and :data:`sys.float_info.epsilon
   <sys.float_info>`.

   .. versionadded:: 3.9


Power, exponential and logarithmic functions
--------------------------------------------

.. function:: cbrt(x)

   Return the cube root of *x*.

   .. versionadded:: 3.11


.. function:: exp(x)

   Return *e* raised to the power *x*, where *e* = 2.718281... is the base
   of natural logarithms.  This is usually more accurate than ``math.e ** x``
   or ``pow(math.e, x)``.


.. function:: exp2(x)

   Return *2* raised to the power *x*.

   .. versionadded:: 3.11


.. function:: expm1(x)

   Return *e* raised to the power *x*, minus 1.  Here *e* is the base of natural
   logarithms.  For small floats *x*, the subtraction in ``exp(x) - 1``
   can result in a `significant loss of precision
   <https://en.wikipedia.org/wiki/Loss_of_significance>`_\; the :func:`expm1`
   function provides a way to compute this quantity to full precision:

      >>> from math import exp, expm1
      >>> exp(1e-5) - 1  # gives result accurate to 11 places
      1.0000050000069649e-05
      >>> expm1(1e-5)    # result accurate to full precision
      1.0000050000166668e-05

   .. versionadded:: 3.2


.. function:: log(x[, base])

   With one argument, return the natural logarithm of *x* (to base *e*).

   With two arguments, return the logarithm of *x* to the given *base*,
   calculated as ``log(x)/log(base)``.


.. function:: log1p(x)

   Return the natural logarithm of *1+x* (base *e*). The
   result is calculated in a way which is accurate for *x* near zero.


.. function:: log2(x)

   Return the base-2 logarithm of *x*. This is usually more accurate than
   ``log(x, 2)``.

   .. versionadded:: 3.3

   .. seealso::

      :meth:`int.bit_length` returns the number of bits necessary to represent
      an integer in binary, excluding the sign and leading zeros.


.. function:: log10(x)

   Return the base-10 logarithm of *x*.  This is usually more accurate
   than ``log(x, 10)``.


.. function:: pow(x, y)

   Return *x* raised to the power *y*.  Exceptional cases follow
   the IEEE 754 standard as far as possible.  In particular,
   ``pow(1.0, x)`` and ``pow(x, 0.0)`` always return ``1.0``, even
   when *x* is a zero or a NaN.  If both *x* and *y* are finite,
   *x* is negative, and *y* is not an integer then ``pow(x, y)``
   is undefined, and raises :exc:`ValueError`.

   Unlike the built-in ``**`` operator, :func:`math.pow` converts both
   its arguments to type :class:`float`.  Use ``**`` or the built-in
   :func:`pow` function for computing exact integer powers.

   .. versionchanged:: 3.11
      The special cases ``pow(0.0, -inf)`` and ``pow(-0.0, -inf)`` were
      changed to return ``inf`` instead of raising :exc:`ValueError`,
      for consistency with IEEE 754.


.. function:: sqrt(x)

   Return the square root of *x*.


Summation and product functions
-------------------------------

.. function:: dist(p, q)

   Return the Euclidean distance between two points *p* and *q*, each
   given as a sequence (or iterable) of coordinates.  The two points
   must have the same dimension.

   Roughly equivalent to::

       sqrt(sum((px - qx) ** 2.0 for px, qx in zip(p, q)))

   .. versionadded:: 3.8


.. function:: fsum(iterable)

   Return an accurate floating-point sum of values in the iterable.  Avoids
   loss of precision by tracking multiple intermediate partial sums.

   The algorithm's accuracy depends on IEEE-754 arithmetic guarantees and the
   typical case where the rounding mode is half-even.  On some non-Windows
   builds, the underlying C library uses extended precision addition and may
   occasionally double-round an intermediate sum causing it to be off in its
   least significant bit.

   For further discussion and two alternative approaches, see the `ASPN cookbook
   recipes for accurate floating-point summation
   <https://code.activestate.com/recipes/393090-binary-floating-point-summation-accurate-to-full-p/>`_\.


.. function:: hypot(*coordinates)

   Return the Euclidean norm, ``sqrt(sum(x**2 for x in coordinates))``.
   This is the length of the vector from the origin to the point
   given by the coordinates.

   For a two dimensional point ``(x, y)``, this is equivalent to computing
   the hypotenuse of a right triangle using the Pythagorean theorem,
   ``sqrt(x*x + y*y)``.

   .. versionchanged:: 3.8
      Added support for n-dimensional points. Formerly, only the two
      dimensional case was supported.

   .. versionchanged:: 3.10
      Improved the algorithm's accuracy so that the maximum error is
      under 1 ulp (unit in the last place).  More typically, the result
      is almost always correctly rounded to within 1/2 ulp.


.. function:: prod(iterable, *, start=1)

   Calculate the product of all the elements in the input *iterable*.
   The default *start* value for the product is ``1``.

   When the iterable is empty, return the start value.  This function is
   intended specifically for use with numeric values and may reject
   non-numeric types.

   .. versionadded:: 3.8


.. function:: sumprod(p, q)

   Return the sum of products of values from two iterables *p* and *q*.

   Raises :exc:`ValueError` if the inputs do not have the same length.

   Roughly equivalent to::

       sum(map(operator.mul, p, q, strict=True))

   For float and mixed int/float inputs, the intermediate products
   and sums are computed with extended precision.

   .. versionadded:: 3.12


Angular conversion
------------------

.. function:: degrees(x)

   Convert angle *x* from radians to degrees.


.. function:: radians(x)

   Convert angle *x* from degrees to radians.


Trigonometric functions
-----------------------

.. function:: acos(x)

   Return the arc cosine of *x*, in radians. The result is between ``0`` and
   ``pi``.


.. function:: asin(x)

   Return the arc sine of *x*, in radians. The result is between ``-pi/2`` and
   ``pi/2``.


.. function:: atan(x)

   Return the arc tangent of *x*, in radians. The result is between ``-pi/2`` and
   ``pi/2``.


.. function:: atan2(y, x)

   Return ``atan(y / x)``, in radians. The result is between ``-pi`` and ``pi``.
   The vector in the plane from the origin to point ``(x, y)`` makes this angle
   with the positive X axis. The point of :func:`atan2` is that the signs of both
   inputs are known to it, so it can compute the correct quadrant for the angle.
   For example, ``atan(1)`` and ``atan2(1, 1)`` are both ``pi/4``, but ``atan2(-1,
   -1)`` is ``-3*pi/4``.


.. function:: cos(x)

   Return the cosine of *x* radians.


.. function:: sin(x)

   Return the sine of *x* radians.


.. function:: tan(x)

   Return the tangent of *x* radians.


Hyperbolic functions
--------------------

`Hyperbolic functions <https://en.wikipedia.org/wiki/Hyperbolic_functions>`_
are analogs of trigonometric functions that are based on hyperbolas
instead of circles.

.. function:: acosh(x)

   Return the inverse hyperbolic cosine of *x*.


.. function:: asinh(x)

   Return the inverse hyperbolic sine of *x*.


.. function:: atanh(x)

   Return the inverse hyperbolic tangent of *x*.


.. function:: cosh(x)

   Return the hyperbolic cosine of *x*.


.. function:: sinh(x)

   Return the hyperbolic sine of *x*.


.. function:: tanh(x)

   Return the hyperbolic tangent of *x*.


Special functions
-----------------

.. function:: erf(x)

   Return the `error function <https://en.wikipedia.org/wiki/Error_function>`_ at
   *x*.

   The :func:`erf` function can be used to compute traditional statistical
   functions such as the `cumulative standard normal distribution
   <https://en.wikipedia.org/wiki/Cumulative_distribution_function>`_::

     def phi(x):
         'Cumulative distribution function for the standard normal distribution'
         return (1.0 + erf(x / sqrt(2.0))) / 2.0

   .. versionadded:: 3.2


.. function:: erfc(x)

   Return the complementary error function at *x*.  The `complementary error
   function <https://en.wikipedia.org/wiki/Error_function>`_ is defined as
   ``1.0 - erf(x)``.  It is used for large values of *x* where a subtraction
   from one would cause a `loss of significance
   <https://en.wikipedia.org/wiki/Loss_of_significance>`_\.

   .. versionadded:: 3.2


.. function:: gamma(x)

   Return the `Gamma function <https://en.wikipedia.org/wiki/Gamma_function>`_ at
   *x*.

   .. versionadded:: 3.2


.. function:: lgamma(x)

   Return the natural logarithm of the absolute value of the Gamma
   function at *x*.

   .. versionadded:: 3.2


Constants
---------

.. data:: pi

   The mathematical constant *π* = 3.141592..., to available precision.


.. data:: e

   The mathematical constant *e* = 2.718281..., to available precision.


.. data:: tau

   The mathematical constant *τ* = 6.283185..., to available precision.
   Tau is a circle constant equal to 2\ *π*, the ratio of a circle's circumference to
   its radius. To learn more about Tau, check out Vi Hart's video `Pi is (still)
   Wrong <https://www.youtube.com/watch?v=jG7vhMMXagQ>`_, and start celebrating
   `Tau day <https://tauday.com/>`_ by eating twice as much pie!

   .. versionadded:: 3.6


.. data:: inf

   A floating-point positive infinity.  (For negative infinity, use
   ``-math.inf``.)  Equivalent to the output of ``float('inf')``.

   .. versionadded:: 3.5


.. data:: nan

   A floating-point "not a number" (NaN) value. Equivalent to the output of
   ``float('nan')``. Due to the requirements of the `IEEE-754 standard
   <https://en.wikipedia.org/wiki/IEEE_754>`_, ``math.nan`` and ``float('nan')`` are
   not considered to equal to any other numeric value, including themselves. To check
   whether a number is a NaN, use the :func:`isnan` function to test
   for NaNs instead of ``is`` or ``==``.
   Example:

      >>> import math
      >>> math.nan == math.nan
      False
      >>> float('nan') == float('nan')
      False
      >>> math.isnan(math.nan)
      True
      >>> math.isnan(float('nan'))
      True

   .. versionadded:: 3.5

   .. versionchanged:: 3.11
      It is now always available.


.. impl-detail::

   The :mod:`math` module consists mostly of thin wrappers around the platform C
   math library functions.  Behavior in exceptional cases follows Annex F of
   the C99 standard where appropriate.  The current implementation will raise
   :exc:`ValueError` for invalid operations like ``sqrt(-1.0)`` or ``log(0.0)``
   (where C99 Annex F recommends signaling invalid operation or divide-by-zero),
   and :exc:`OverflowError` for results that overflow (for example,
   ``exp(1000.0)``).  A NaN will not be returned from any of the functions
   above unless one or more of the input arguments was a NaN; in that case,
   most functions will return a NaN, but (again following C99 Annex F) there
   are some exceptions to this rule, for example ``pow(float('nan'), 0.0)`` or
   ``hypot(float('nan'), float('inf'))``.

   Note that Python makes no effort to distinguish signaling NaNs from
   quiet NaNs, and behavior for signaling NaNs remains unspecified.
   Typical behavior is to treat all NaNs as though they were quiet.


.. seealso::

   Module :mod:`cmath`
      Complex number versions of many of these functions.

.. |nbsp| unicode:: 0xA0
   :trim:
