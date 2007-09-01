
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

The functions are:


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

   Return the hyperbolic arc sine of *x*. There are two branch cuts, extending
   left from ``±1j`` to ``±∞j``, both continuous from above. These branch cuts
   should be considered a bug to be corrected in a future release. The correct
   branch cuts should extend along the imaginary axis, one from ``1j`` up to
   ``∞j`` and continuous from the right, and one from ``-1j`` down to ``-∞j``
   and continuous from the left.


.. function:: atan(x)

   Return the arc tangent of *x*. There are two branch cuts: One extends from
   ``1j`` along the imaginary axis to ``∞j``, continuous from the left. The
   other extends from ``-1j`` along the imaginary axis to ``-∞j``, continuous
   from the left. (This should probably be changed so the upper cut becomes
   continuous from the other side.)


.. function:: atanh(x)

   Return the hyperbolic arc tangent of *x*. There are two branch cuts: One
   extends from ``1`` along the real axis to ``∞``, continuous from above. The
   other extends from ``-1`` along the real axis to ``-∞``, continuous from
   above. (This should probably be changed so the right cut becomes continuous
   from the other side.)


.. function:: cos(x)

   Return the cosine of *x*.


.. function:: cosh(x)

   Return the hyperbolic cosine of *x*.


.. function:: exp(x)

   Return the exponential value ``e**x``.


.. function:: log(x[, base])

   Returns the logarithm of *x* to the given *base*. If the *base* is not
   specified, returns the natural logarithm of *x*. There is one branch cut, from 0
   along the negative real axis to -∞, continuous from above.


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

