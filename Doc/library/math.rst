
:mod:`math` --- Mathematical functions
======================================

.. module:: math
   :synopsis: Mathematical functions (sin() etc.).

.. testsetup::

   from math import fsum

This module is always available.  It provides access to the mathematical
functions defined by the C standard.

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


Number-theoretic and representation functions
---------------------------------------------

.. function:: ceil(x)

   Return the ceiling of *x* as a float, the smallest integer value greater than or
   equal to *x*.


.. function:: copysign(x, y)

   Return *x* with the sign of *y*.  On a platform that supports
   signed zeros, ``copysign(1.0, -0.0)`` returns *-1.0*.

   .. versionadded:: 2.6


.. function:: fabs(x)

   Return the absolute value of *x*.


.. function:: factorial(x)

   Return *x* factorial.  Raises :exc:`ValueError` if *x* is not integral or
   is negative.

   .. versionadded:: 2.6


.. function:: floor(x)

   Return the floor of *x* as a float, the largest integer value less than or equal
   to *x*.


.. function:: fmod(x, y)

   Return ``fmod(x, y)``, as defined by the platform C library. Note that the
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


.. function:: frexp(x)

   Return the mantissa and exponent of *x* as the pair ``(m, e)``.  *m* is a float
   and *e* is an integer such that ``x == m * 2**e`` exactly. If *x* is zero,
   returns ``(0.0, 0)``, otherwise ``0.5 <= abs(m) < 1``.  This is used to "pick
   apart" the internal representation of a float in a portable way.


.. function:: fsum(iterable)

   Return an accurate floating point sum of values in the iterable.  Avoids
   loss of precision by tracking multiple intermediate partial sums::

        >>> sum([.1, .1, .1, .1, .1, .1, .1, .1, .1, .1])
        0.9999999999999999
        >>> fsum([.1, .1, .1, .1, .1, .1, .1, .1, .1, .1])
        1.0

   The algorithm's accuracy depends on IEEE-754 arithmetic guarantees and the
   typical case where the rounding mode is half-even.  On some non-Windows
   builds, the underlying C library uses extended precision addition and may
   occasionally double-round an intermediate sum causing it to be off in its
   least significant bit.

   For further discussion and two alternative approaches, see the `ASPN cookbook
   recipes for accurate floating point summation
   <http://code.activestate.com/recipes/393090/>`_\.

   .. versionadded:: 2.6


.. function:: isinf(x)

   Check if the float *x* is positive or negative infinity.

   .. versionadded:: 2.6


.. function:: isnan(x)

   Check if the float *x* is a NaN (not a number).  For more information
   on NaNs, see the IEEE 754 standards.

   .. versionadded:: 2.6


.. function:: ldexp(x, i)

   Return ``x * (2**i)``.  This is essentially the inverse of function
   :func:`frexp`.


.. function:: modf(x)

   Return the fractional and integer parts of *x*.  Both results carry the sign
   of *x* and are floats.


.. function:: trunc(x)

   Return the :class:`Real` value *x* truncated to an :class:`Integral` (usually
   a long integer).  Uses the ``__trunc__`` method.

   .. versionadded:: 2.6


Note that :func:`frexp` and :func:`modf` have a different call/return pattern
than their C equivalents: they take a single argument and return a pair of
values, rather than returning their second return value through an 'output
parameter' (there is no such thing in Python).

For the :func:`ceil`, :func:`floor`, and :func:`modf` functions, note that *all*
floating-point numbers of sufficiently large magnitude are exact integers.
Python floats typically carry no more than 53 bits of precision (the same as the
platform C double type), in which case any float *x* with ``abs(x) >= 2**52``
necessarily has no fractional bits.


Power and logarithmic functions
-------------------------------

.. function:: exp(x)

   Return ``e**x``.


.. function:: expm1(x)

   Return ``e**x - 1``.  For small floats *x*, the subtraction in
   ``exp(x) - 1`` can result in a significant loss of precision; the
   :func:`expm1` function provides a way to compute this quantity to
   full precision::

      >>> from math import exp, expm1
      >>> exp(1e-5) - 1  # gives result accurate to 11 places
      1.0000050000069649e-05
      >>> expm1(1e-5)    # result accurate to full precision
      1.0000050000166668e-05

   .. versionadded:: 2.7


.. function:: log(x[, base])

   With one argument, return the natural logarithm of *x* (to base *e*).

   With two arguments, return the logarithm of *x* to the given *base*,
   calculated as ``log(x)/log(base)``.

   .. versionchanged:: 2.3
      *base* argument added.


.. function:: log1p(x)

   Return the natural logarithm of *1+x* (base *e*). The
   result is calculated in a way which is accurate for *x* near zero.

   .. versionadded:: 2.6


.. function:: log10(x)

   Return the base-10 logarithm of *x*.  This is usually more accurate
   than ``log(x, 10)``.


.. function:: pow(x, y)

   Return ``x`` raised to the power ``y``.  Exceptional cases follow
   Annex 'F' of the C99 standard as far as possible.  In particular,
   ``pow(1.0, x)`` and ``pow(x, 0.0)`` always return ``1.0``, even
   when ``x`` is a zero or a NaN.  If both ``x`` and ``y`` are finite,
   ``x`` is negative, and ``y`` is not an integer then ``pow(x, y)``
   is undefined, and raises :exc:`ValueError`.

   Unlike the built-in ``**`` operator, :func:`math.pow` converts both
   its arguments to type :class:`float`.  Use ``**`` or the built-in
   :func:`pow` function for computing exact integer powers.

   .. versionchanged:: 2.6
      The outcome of ``1**nan`` and ``nan**0`` was undefined.


.. function:: sqrt(x)

   Return the square root of *x*.


Trigonometric functions
-----------------------

.. function:: acos(x)

   Return the arc cosine of *x*, in radians.


.. function:: asin(x)

   Return the arc sine of *x*, in radians.


.. function:: atan(x)

   Return the arc tangent of *x*, in radians.


.. function:: atan2(y, x)

   Return ``atan(y / x)``, in radians. The result is between ``-pi`` and ``pi``.
   The vector in the plane from the origin to point ``(x, y)`` makes this angle
   with the positive X axis. The point of :func:`atan2` is that the signs of both
   inputs are known to it, so it can compute the correct quadrant for the angle.
   For example, ``atan(1)`` and ``atan2(1, 1)`` are both ``pi/4``, but ``atan2(-1,
   -1)`` is ``-3*pi/4``.


.. function:: cos(x)

   Return the cosine of *x* radians.


.. function:: hypot(x, y)

   Return the Euclidean norm, ``sqrt(x*x + y*y)``. This is the length of the vector
   from the origin to point ``(x, y)``.


.. function:: sin(x)

   Return the sine of *x* radians.


.. function:: tan(x)

   Return the tangent of *x* radians.


Angular conversion
------------------

.. function:: degrees(x)

   Converts angle *x* from radians to degrees.


.. function:: radians(x)

   Converts angle *x* from degrees to radians.


Hyperbolic functions
--------------------

.. function:: acosh(x)

   Return the inverse hyperbolic cosine of *x*.

   .. versionadded:: 2.6


.. function:: asinh(x)

   Return the inverse hyperbolic sine of *x*.

   .. versionadded:: 2.6


.. function:: atanh(x)

   Return the inverse hyperbolic tangent of *x*.

   .. versionadded:: 2.6


.. function:: cosh(x)

   Return the hyperbolic cosine of *x*.


.. function:: sinh(x)

   Return the hyperbolic sine of *x*.


.. function:: tanh(x)

   Return the hyperbolic tangent of *x*.


Special functions
-----------------

.. function:: erf(x)

   Return the error function at *x*.

   .. versionadded:: 2.7


.. function:: erfc(x)

   Return the complementary error function at *x*.

   .. versionadded:: 2.7


.. function:: gamma(x)

   Return the Gamma function at *x*.

   .. versionadded:: 2.7


.. function:: lgamma(x)

   Return the natural logarithm of the absolute value of the Gamma
   function at *x*.

   .. versionadded:: 2.7


Constants
---------

.. data:: pi

   The mathematical constant π = 3.141592..., to available precision.


.. data:: e

   The mathematical constant e = 2.718281..., to available precision.


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

   .. versionchanged:: 2.6
      Behavior in special cases now aims to follow C99 Annex F.  In earlier
      versions of Python the behavior in special cases was loosely specified.


.. seealso::

   Module :mod:`cmath`
      Complex number versions of many of these functions.
