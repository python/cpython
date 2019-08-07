:mod:`cmath` --- Mathematical functions for complex numbers
===========================================================

.. module:: cmath
   :synopsis: Mathematical functions for complex numbers.

--------------

This module provides access to mathematical functions for complex numbers.  The
functions in this module accept integers, floating-point numbers or complex
numbers as arguments. They will also accept any Python object that has either a
:meth:`__complex__` or a :meth:`__float__` method: these methods are used to
convert the object to a complex or floating-point number, respectively, and
the function is then applied to the result of the conversion.

.. note::

   On platforms with hardware and system-level support for signed
   zeros, functions involving branch cuts are continuous on *both*
   sides of the branch cut: the sign of the zero distinguishes one
   side of the branch cut from the other.  On platforms that do not
   support signed zeros the continuity is as specified below.


Conversions to and from polar coordinates
-----------------------------------------

A Python complex number ``z`` is stored internally using *rectangular*
or *Cartesian* coordinates.  It is completely determined by its *real
part* ``z.real`` and its *imaginary part* ``z.imag``.  In other
words::

   z == z.real + z.imag*1j

*Polar coordinates* give an alternative way to represent a complex
number.  In polar coordinates, a complex number *z* is defined by the
modulus *r* and the phase angle *phi*. The modulus *r* is the distance
from *z* to the origin, while the phase *phi* is the counterclockwise
angle, measured in radians, from the positive x-axis to the line
segment that joins the origin to *z*.

The following functions can be used to convert from the native
rectangular coordinates to polar coordinates and back.

.. function:: phase(x)

   Return the phase of *x* (also known as the *argument* of *x*), as a
   float.  ``phase(x)`` is equivalent to ``math.atan2(x.imag,
   x.real)``.  The result lies in the range [-\ *π*, *π*], and the branch
   cut for this operation lies along the negative real axis,
   continuous from above.  On systems with support for signed zeros
   (which includes most systems in current use), this means that the
   sign of the result is the same as the sign of ``x.imag``, even when
   ``x.imag`` is zero::

      >>> phase(complex(-1.0, 0.0))
      3.141592653589793
      >>> phase(complex(-1.0, -0.0))
      -3.141592653589793


.. note::

   The modulus (absolute value) of a complex number *x* can be
   computed using the built-in :func:`abs` function.  There is no
   separate :mod:`cmath` module function for this operation.


.. function:: polar(x)

   Return the representation of *x* in polar coordinates.  Returns a
   pair ``(r, phi)`` where *r* is the modulus of *x* and phi is the
   phase of *x*.  ``polar(x)`` is equivalent to ``(abs(x),
   phase(x))``.


.. function:: rect(r, phi)

   Return the complex number *x* with polar coordinates *r* and *phi*.
   Equivalent to ``r * (math.cos(phi) + math.sin(phi)*1j)``.


Power and logarithmic functions
-------------------------------

.. function:: exp(x)

   Return *e* raised to the power *x*, where *e* is the base of natural
   logarithms.


.. function:: log(x[, base])

   Returns the logarithm of *x* to the given *base*. If the *base* is not
   specified, returns the natural logarithm of *x*. There is one branch cut, from 0
   along the negative real axis to -∞, continuous from above.


.. function:: log10(x)

   Return the base-10 logarithm of *x*. This has the same branch cut as
   :func:`log`.


.. function:: sqrt(x)

   Return the square root of *x*. This has the same branch cut as :func:`log`.


Trigonometric functions
-----------------------

.. function:: acos(x)

   Return the arc cosine of *x*. There are two branch cuts: One extends right from
   1 along the real axis to ∞, continuous from below. The other extends left from
   -1 along the real axis to -∞, continuous from above.


.. function:: asin(x)

   Return the arc sine of *x*. This has the same branch cuts as :func:`acos`.


.. function:: atan(x)

   Return the arc tangent of *x*. There are two branch cuts: One extends from
   ``1j`` along the imaginary axis to ``∞j``, continuous from the right. The
   other extends from ``-1j`` along the imaginary axis to ``-∞j``, continuous
   from the left.


.. function:: cos(x)

   Return the cosine of *x*.


.. function:: sin(x)

   Return the sine of *x*.


.. function:: tan(x)

   Return the tangent of *x*.


Hyperbolic functions
--------------------

.. function:: acosh(x)

   Return the inverse hyperbolic cosine of *x*. There is one branch cut,
   extending left from 1 along the real axis to -∞, continuous from above.


.. function:: asinh(x)

   Return the inverse hyperbolic sine of *x*. There are two branch cuts:
   One extends from ``1j`` along the imaginary axis to ``∞j``,
   continuous from the right.  The other extends from ``-1j`` along
   the imaginary axis to ``-∞j``, continuous from the left.


.. function:: atanh(x)

   Return the inverse hyperbolic tangent of *x*. There are two branch cuts: One
   extends from ``1`` along the real axis to ``∞``, continuous from below. The
   other extends from ``-1`` along the real axis to ``-∞``, continuous from
   above.


.. function:: cosh(x)

   Return the hyperbolic cosine of *x*.


.. function:: sinh(x)

   Return the hyperbolic sine of *x*.


.. function:: tanh(x)

   Return the hyperbolic tangent of *x*.


Classification functions
------------------------

.. function:: isfinite(x)

   Return ``True`` if both the real and imaginary parts of *x* are finite, and
   ``False`` otherwise.

   .. versionadded:: 3.2


.. function:: isinf(x)

   Return ``True`` if either the real or the imaginary part of *x* is an
   infinity, and ``False`` otherwise.


.. function:: isnan(x)

   Return ``True`` if either the real or the imaginary part of *x* is a NaN,
   and ``False`` otherwise.


.. function:: isclose(a, b, *, rel_tol=1e-09, abs_tol=0.0)

   Return ``True`` if the values *a* and *b* are close to each other and
   ``False`` otherwise.

   Whether or not two values are considered close is determined according to
   given absolute and relative tolerances.

   *rel_tol* is the relative tolerance -- it is the maximum allowed difference
   between *a* and *b*, relative to the larger absolute value of *a* or *b*.
   For example, to set a tolerance of 5%, pass ``rel_tol=0.05``.  The default
   tolerance is ``1e-09``, which assures that the two values are the same
   within about 9 decimal digits.  *rel_tol* must be greater than zero.

   *abs_tol* is the minimum absolute tolerance -- useful for comparisons near
   zero. *abs_tol* must be at least zero.

   If no errors occur, the result will be:
   ``abs(a-b) <= max(rel_tol * max(abs(a), abs(b)), abs_tol)``.

   The IEEE 754 special values of ``NaN``, ``inf``, and ``-inf`` will be
   handled according to IEEE rules.  Specifically, ``NaN`` is not considered
   close to any other value, including ``NaN``.  ``inf`` and ``-inf`` are only
   considered close to themselves.

   .. versionadded:: 3.5

   .. seealso::

      :pep:`485` -- A function for testing approximate equality


Constants
---------

.. data:: pi

   The mathematical constant *π*, as a float.


.. data:: e

   The mathematical constant *e*, as a float.


.. data:: tau

   The mathematical constant *τ*, as a float.

   .. versionadded:: 3.6


.. data:: inf

   Floating-point positive infinity. Equivalent to ``float('inf')``.

   .. versionadded:: 3.6


.. data:: infj

   Complex number with zero real part and positive infinity imaginary
   part. Equivalent to ``complex(0.0, float('inf'))``.

   .. versionadded:: 3.6


.. data:: nan

   A floating-point "not a number" (NaN) value.  Equivalent to
   ``float('nan')``.

   .. versionadded:: 3.6


.. data:: nanj

   Complex number with zero real part and NaN imaginary part. Equivalent to
   ``complex(0.0, float('nan'))``.

   .. versionadded:: 3.6


.. index:: module: math

Note that the selection of functions is similar, but not identical, to that in
module :mod:`math`.  The reason for having two modules is that some users aren't
interested in complex numbers, and perhaps don't even know what they are.  They
would rather have ``math.sqrt(-1)`` raise an exception than return a complex
number. Also note that the functions defined in :mod:`cmath` always return a
complex number, even if the answer can be expressed as a real number (in which
case the complex number has an imaginary part of zero).

A note on branch cuts: They are curves along which the given function fails to
be continuous.  They are a necessary feature of many complex functions.  It is
assumed that if you need to compute with complex functions, you will understand
about branch cuts.  Consult almost any (not too elementary) book on complex
variables for enlightenment.  For information of the proper choice of branch
cuts for numerical purposes, a good reference should be the following:


.. seealso::

   Kahan, W:  Branch cuts for complex elementary functions; or, Much ado about
   nothing's sign bit.  In Iserles, A., and Powell, M. (eds.), The state of the art
   in numerical analysis. Clarendon Press (1987) pp165--211.
