
:mod:`cmath` --- Mathematical functions for complex numbers
===========================================================

.. module:: cmath
   :synopsis: Mathematical functions for complex numbers.


This module is always available.  It provides access to mathematical functions
for complex numbers.  The functions in this module accept integers,
floating-point numbers or complex numbers as arguments. They will also accept
any Python object that has either a :meth:`__complex__` or a :meth:`__float__`
method: these methods are used to convert the object to a complex or
floating-point number, respectively, and the function is then applied to the
result of the conversion.

.. note::

   On platforms with hardware and system-level support for signed
   zeros, functions involving branch cuts are continuous on *both*
   sides of the branch cut: the sign of the zero distinguishes one
   side of the branch cut from the other.  On platforms that do not
   support signed zeros the continuity is as specified below.


Complex coordinates
-------------------

Complex numbers can be expressed by two important coordinate systems.
Python's :class:`complex` type uses rectangular coordinates where a number
on the complex plain is defined by two floats, the real part and the imaginary
part.

Definition::

   z = x + 1j * y

   x := real(z)
   y := imag(z)

In engineering the polar coordinate system is popular for complex numbers. In
polar coordinates a complex number is defined by the radius *r* and the phase
angle *φ*. The radius *r* is the absolute value of the complex, which can be
viewed as distance from (0, 0). The radius *r* is always 0 or a positive float.
The phase angle *φ* is the counter clockwise angle from the positive x axis,
e.g. *1* has the angle *0*, *1j* has the angle *π/2* and *-1* the angle *-π*.

.. note::
   While :func:`phase` and func:`polar` return *+π* for a negative real they
   may return *-π* for a complex with a very small negative imaginary
   part, e.g. *-1-1E-300j*.


Definition::

   z = r * exp(1j * φ)
   z = r * cis(φ)

   r := abs(z) := sqrt(real(z)**2 + imag(z)**2)
   phi := phase(z) := atan2(imag(z), real(z))
   cis(φ) := cos(φ) + 1j * sin(φ)


.. function:: phase(x)

   Return phase, also known as the argument, of a complex.

   .. versionadded:: 2.6


.. function:: polar(x)

   Convert a :class:`complex` from rectangular coordinates to polar 
   coordinates. The function returns a tuple with the two elements
   *r* and *phi*. *r* is the distance from 0 and *phi* the phase 
   angle.

   .. versionadded:: 2.6


.. function:: rect(r, phi)

   Convert from polar coordinates to rectangular coordinates and return
   a :class:`complex`.

   .. versionadded:: 2.6



cmath functions
---------------

.. function:: acos(x)

   Return the arc cosine of *x*. There are two branch cuts: One extends right from
   1 along the real axis to ∞, continuous from below. The other extends left from
   -1 along the real axis to -∞, continuous from above.


.. function:: acosh(x)

   Return the hyperbolic arc cosine of *x*. There is one branch cut, extending left
   from 1 along the real axis to -∞, continuous from above.


.. function:: asin(x)

   Return the arc sine of *x*. This has the same branch cuts as :func:`acos`.


.. function:: asinh(x)

   Return the hyperbolic arc sine of *x*. There are two branch cuts:
   One extends from ``1j`` along the imaginary axis to ``∞j``,
   continuous from the right.  The other extends from ``-1j`` along
   the imaginary axis to ``-∞j``, continuous from the left.

   .. versionchanged:: 2.6
      branch cuts moved to match those recommended by the C99 standard


.. function:: atan(x)

   Return the arc tangent of *x*. There are two branch cuts: One extends from
   ``1j`` along the imaginary axis to ``∞j``, continuous from the right. The
   other extends from ``-1j`` along the imaginary axis to ``-∞j``, continuous
   from the left.

   .. versionchanged:: 2.6
      direction of continuity of upper cut reversed


.. function:: atanh(x)

   Return the hyperbolic arc tangent of *x*. There are two branch cuts: One
   extends from ``1`` along the real axis to ``∞``, continuous from below. The
   other extends from ``-1`` along the real axis to ``-∞``, continuous from
   above.

   .. versionchanged:: 2.6
      direction of continuity of right cut reversed


.. function:: cos(x)

   Return the cosine of *x*.


.. function:: cosh(x)

   Return the hyperbolic cosine of *x*.


.. function:: exp(x)

   Return the exponential value ``e**x``.


.. function:: isinf(x)

   Return *True* if the real or the imaginary part of x is positive
   or negative infinity.

   .. versionadded:: 2.6


.. function:: isnan(x)

   Return *True* if the real or imaginary part of x is not a number (NaN).

   .. versionadded:: 2.6


.. function:: log(x[, base])

   Returns the logarithm of *x* to the given *base*. If the *base* is not
   specified, returns the natural logarithm of *x*. There is one branch cut, from 0
   along the negative real axis to -∞, continuous from above.

   .. versionchanged:: 2.4
      *base* argument added.


.. function:: log10(x)

   Return the base-10 logarithm of *x*. This has the same branch cut as
   :func:`log`.


.. function:: sin(x)

   Return the sine of *x*.


.. function:: sinh(x)

   Return the hyperbolic sine of *x*.


.. function:: sqrt(x)

   Return the square root of *x*. This has the same branch cut as :func:`log`.


.. function:: tan(x)

   Return the tangent of *x*.


.. function:: tanh(x)

   Return the hyperbolic tangent of *x*.

The module also defines two mathematical constants:


.. data:: pi

   The mathematical constant *pi*, as a float.


.. data:: e

   The mathematical constant *e*, as a float.

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
   in numerical analysis. Clarendon Press (1987) pp165-211.


