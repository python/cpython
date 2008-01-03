
:mod:`math` --- Mathematical functions
======================================

.. module:: math
   :synopsis: Mathematical functions (sin() etc.).


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

Number-theoretic and representation functions:


.. function:: ceil(x)

   Return the ceiling of *x* as a float, the smallest integer value greater than
   or equal to *x*. If *x* is not a float, delegates to ``x.__ceil__()``, which
   should return an :class:`Integral` value.


.. function:: fabs(x)

   Return the absolute value of *x*.


.. function:: floor(x)

   Return the floor of *x* as a float, the largest integer value less than or
   equal to *x*. If *x* is not a float, delegates to ``x.__floor__()``, which
   should return an :class:`Integral` value.


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


.. function:: ldexp(x, i)

   Return ``x * (2**i)``.  This is essentially the inverse of function
   :func:`frexp`.


.. function:: modf(x)

   Return the fractional and integer parts of *x*.  Both results carry the sign of
   *x*, and both are floats.

Note that :func:`frexp` and :func:`modf` have a different call/return pattern
than their C equivalents: they take a single argument and return a pair of
values, rather than returning their second return value through an 'output
parameter' (there is no such thing in Python).

For the :func:`ceil`, :func:`floor`, and :func:`modf` functions, note that *all*
floating-point numbers of sufficiently large magnitude are exact integers.
Python floats typically carry no more than 53 bits of precision (the same as the
platform C double type), in which case any float *x* with ``abs(x) >= 2**52``
necessarily has no fractional bits.

Power and logarithmic functions:


.. function:: exp(x)

   Return ``e**x``.


.. function:: log(x[, base])

   Return the logarithm of *x* to the given *base*. If the *base* is not specified,
   return the natural logarithm of *x* (that is, the logarithm to base *e*).

   .. versionchanged:: 2.3
      *base* argument added.


.. function:: log10(x)

   Return the base-10 logarithm of *x*.


.. function:: pow(x, y)

   Return ``x**y``.


.. function:: sqrt(x)

   Return the square root of *x*.

Trigonometric functions:


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
   For example, ``atan(1``) and ``atan2(1, 1)`` are both ``pi/4``, but ``atan2(-1,
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

Angular conversion:


.. function:: degrees(x)

   Converts angle *x* from radians to degrees.


.. function:: radians(x)

   Converts angle *x* from degrees to radians.

Hyperbolic functions:


.. function:: cosh(x)

   Return the hyperbolic cosine of *x*.


.. function:: sinh(x)

   Return the hyperbolic sine of *x*.


.. function:: tanh(x)

   Return the hyperbolic tangent of *x*.

The module also defines two mathematical constants:


.. data:: pi

   The mathematical constant *pi*.


.. data:: e

   The mathematical constant *e*.

.. note::

   The :mod:`math` module consists mostly of thin wrappers around the platform C
   math library functions.  Behavior in exceptional cases is loosely specified
   by the C standards, and Python inherits much of its math-function
   error-reporting behavior from the platform C implementation.  As a result,
   the specific exceptions raised in error cases (and even whether some
   arguments are considered to be exceptional at all) are not defined in any
   useful cross-platform or cross-release way.  For example, whether
   ``math.log(0)`` returns ``-Inf`` or raises :exc:`ValueError` or
   :exc:`OverflowError` isn't defined, and in cases where ``math.log(0)`` raises
   :exc:`OverflowError`, ``math.log(0L)`` may raise :exc:`ValueError` instead.


.. seealso::

   Module :mod:`cmath`
      Complex number versions of many of these functions.

