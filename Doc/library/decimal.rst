
:mod:`decimal` --- Decimal floating point arithmetic
====================================================

.. module:: decimal
   :synopsis: Implementation of the General Decimal Arithmetic  Specification.


.. moduleauthor:: Eric Price <eprice at tjhsst.edu>
.. moduleauthor:: Facundo Batista <facundo at taniquetil.com.ar>
.. moduleauthor:: Raymond Hettinger <python at rcn.com>
.. moduleauthor:: Aahz <aahz at pobox.com>
.. moduleauthor:: Tim Peters <tim.one at comcast.net>


.. sectionauthor:: Raymond D. Hettinger <python at rcn.com>


.. versionadded:: 2.4

The :mod:`decimal` module provides support for decimal floating point
arithmetic.  It offers several advantages over the :class:`float()` datatype:

* Decimal numbers can be represented exactly.  In contrast, numbers like
  :const:`1.1` do not have an exact representation in binary floating point. End
  users typically would not expect :const:`1.1` to display as
  :const:`1.1000000000000001` as it does with binary floating point.

* The exactness carries over into arithmetic.  In decimal floating point, ``0.1
  + 0.1 + 0.1 - 0.3`` is exactly equal to zero.  In binary floating point, result
  is :const:`5.5511151231257827e-017`.  While near to zero, the differences
  prevent reliable equality testing and differences can accumulate. For this
  reason, decimal would be preferred in accounting applications which have strict
  equality invariants.

* The decimal module incorporates a notion of significant places so that ``1.30
  + 1.20`` is :const:`2.50`.  The trailing zero is kept to indicate significance.
  This is the customary presentation for monetary applications. For
  multiplication, the "schoolbook" approach uses all the figures in the
  multiplicands.  For instance, ``1.3 * 1.2`` gives :const:`1.56` while ``1.30 *
  1.20`` gives :const:`1.5600`.

* Unlike hardware based binary floating point, the decimal module has a user
  settable precision (defaulting to 28 places) which can be as large as needed for
  a given problem::

     >>> getcontext().prec = 6
     >>> Decimal(1) / Decimal(7)
     Decimal("0.142857")
     >>> getcontext().prec = 28
     >>> Decimal(1) / Decimal(7)
     Decimal("0.1428571428571428571428571429")

* Both binary and decimal floating point are implemented in terms of published
  standards.  While the built-in float type exposes only a modest portion of its
  capabilities, the decimal module exposes all required parts of the standard.
  When needed, the programmer has full control over rounding and signal handling.

The module design is centered around three concepts:  the decimal number, the
context for arithmetic, and signals.

A decimal number is immutable.  It has a sign, coefficient digits, and an
exponent.  To preserve significance, the coefficient digits do not truncate
trailing zeroes.  Decimals also include special values such as
:const:`Infinity`, :const:`-Infinity`, and :const:`NaN`.  The standard also
differentiates :const:`-0` from :const:`+0`.

The context for arithmetic is an environment specifying precision, rounding
rules, limits on exponents, flags indicating the results of operations, and trap
enablers which determine whether signals are treated as exceptions.  Rounding
options include :const:`ROUND_CEILING`, :const:`ROUND_DOWN`,
:const:`ROUND_FLOOR`, :const:`ROUND_HALF_DOWN`, :const:`ROUND_HALF_EVEN`,
:const:`ROUND_HALF_UP`, and :const:`ROUND_UP`.

Signals are groups of exceptional conditions arising during the course of
computation.  Depending on the needs of the application, signals may be ignored,
considered as informational, or treated as exceptions. The signals in the
decimal module are: :const:`Clamped`, :const:`InvalidOperation`,
:const:`DivisionByZero`, :const:`Inexact`, :const:`Rounded`, :const:`Subnormal`,
:const:`Overflow`, and :const:`Underflow`.

For each signal there is a flag and a trap enabler.  When a signal is
encountered, its flag is incremented from zero and, then, if the trap enabler is
set to one, an exception is raised.  Flags are sticky, so the user needs to
reset them before monitoring a calculation.


.. seealso::

   IBM's General Decimal Arithmetic Specification, `The General Decimal Arithmetic
   Specification <http://www2.hursley.ibm.com/decimal/decarith.html>`_.

   IEEE standard 854-1987, `Unofficial IEEE 854 Text
   <http://www.cs.berkeley.edu/~ejr/projects/754/private/drafts/854-1987/dir.html>`_.

.. % %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


.. _decimal-tutorial:

Quick-start Tutorial
--------------------

The usual start to using decimals is importing the module, viewing the current
context with :func:`getcontext` and, if necessary, setting new values for
precision, rounding, or enabled traps::

   >>> from decimal import *
   >>> getcontext()
   Context(prec=28, rounding=ROUND_HALF_EVEN, Emin=-999999999, Emax=999999999,
           capitals=1, flags=[], traps=[Overflow, InvalidOperation,
           DivisionByZero])

   >>> getcontext().prec = 7       # Set a new precision

Decimal instances can be constructed from integers, strings, or tuples.  To
create a Decimal from a :class:`float`, first convert it to a string.  This
serves as an explicit reminder of the details of the conversion (including
representation error).  Decimal numbers include special values such as
:const:`NaN` which stands for "Not a number", positive and negative
:const:`Infinity`, and :const:`-0`.         ::

   >>> Decimal(10)
   Decimal("10")
   >>> Decimal("3.14")
   Decimal("3.14")
   >>> Decimal((0, (3, 1, 4), -2))
   Decimal("3.14")
   >>> Decimal(str(2.0 ** 0.5))
   Decimal("1.41421356237")
   >>> Decimal("NaN")
   Decimal("NaN")
   >>> Decimal("-Infinity")
   Decimal("-Infinity")

The significance of a new Decimal is determined solely by the number of digits
input.  Context precision and rounding only come into play during arithmetic
operations. ::

   >>> getcontext().prec = 6
   >>> Decimal('3.0')
   Decimal("3.0")
   >>> Decimal('3.1415926535')
   Decimal("3.1415926535")
   >>> Decimal('3.1415926535') + Decimal('2.7182818285')
   Decimal("5.85987")
   >>> getcontext().rounding = ROUND_UP
   >>> Decimal('3.1415926535') + Decimal('2.7182818285')
   Decimal("5.85988")

Decimals interact well with much of the rest of Python.  Here is a small decimal
floating point flying circus::

   >>> data = map(Decimal, '1.34 1.87 3.45 2.35 1.00 0.03 9.25'.split())
   >>> max(data)
   Decimal("9.25")
   >>> min(data)
   Decimal("0.03")
   >>> sorted(data)
   [Decimal("0.03"), Decimal("1.00"), Decimal("1.34"), Decimal("1.87"),
    Decimal("2.35"), Decimal("3.45"), Decimal("9.25")]
   >>> sum(data)
   Decimal("19.29")
   >>> a,b,c = data[:3]
   >>> str(a)
   '1.34'
   >>> float(a)
   1.3400000000000001
   >>> round(a, 1)     # round() first converts to binary floating point
   1.3
   >>> int(a)
   1
   >>> a * 5
   Decimal("6.70")
   >>> a * b
   Decimal("2.5058")
   >>> c % a
   Decimal("0.77")

The :meth:`quantize` method rounds a number to a fixed exponent.  This method is
useful for monetary applications that often round results to a fixed number of
places::

   >>> Decimal('7.325').quantize(Decimal('.01'), rounding=ROUND_DOWN)
   Decimal("7.32")
   >>> Decimal('7.325').quantize(Decimal('1.'), rounding=ROUND_UP)
   Decimal("8")

As shown above, the :func:`getcontext` function accesses the current context and
allows the settings to be changed.  This approach meets the needs of most
applications.

For more advanced work, it may be useful to create alternate contexts using the
Context() constructor.  To make an alternate active, use the :func:`setcontext`
function.

In accordance with the standard, the :mod:`Decimal` module provides two ready to
use standard contexts, :const:`BasicContext` and :const:`ExtendedContext`. The
former is especially useful for debugging because many of the traps are
enabled::

   >>> myothercontext = Context(prec=60, rounding=ROUND_HALF_DOWN)
   >>> setcontext(myothercontext)
   >>> Decimal(1) / Decimal(7)
   Decimal("0.142857142857142857142857142857142857142857142857142857142857")

   >>> ExtendedContext
   Context(prec=9, rounding=ROUND_HALF_EVEN, Emin=-999999999, Emax=999999999,
           capitals=1, flags=[], traps=[])
   >>> setcontext(ExtendedContext)
   >>> Decimal(1) / Decimal(7)
   Decimal("0.142857143")
   >>> Decimal(42) / Decimal(0)
   Decimal("Infinity")

   >>> setcontext(BasicContext)
   >>> Decimal(42) / Decimal(0)
   Traceback (most recent call last):
     File "<pyshell#143>", line 1, in -toplevel-
       Decimal(42) / Decimal(0)
   DivisionByZero: x / 0

Contexts also have signal flags for monitoring exceptional conditions
encountered during computations.  The flags remain set until explicitly cleared,
so it is best to clear the flags before each set of monitored computations by
using the :meth:`clear_flags` method. ::

   >>> setcontext(ExtendedContext)
   >>> getcontext().clear_flags()
   >>> Decimal(355) / Decimal(113)
   Decimal("3.14159292")
   >>> getcontext()
   Context(prec=9, rounding=ROUND_HALF_EVEN, Emin=-999999999, Emax=999999999,
           capitals=1, flags=[Inexact, Rounded], traps=[])

The *flags* entry shows that the rational approximation to :const:`Pi` was
rounded (digits beyond the context precision were thrown away) and that the
result is inexact (some of the discarded digits were non-zero).

Individual traps are set using the dictionary in the :attr:`traps` field of a
context::

   >>> Decimal(1) / Decimal(0)
   Decimal("Infinity")
   >>> getcontext().traps[DivisionByZero] = 1
   >>> Decimal(1) / Decimal(0)
   Traceback (most recent call last):
     File "<pyshell#112>", line 1, in -toplevel-
       Decimal(1) / Decimal(0)
   DivisionByZero: x / 0

Most programs adjust the current context only once, at the beginning of the
program.  And, in many applications, data is converted to :class:`Decimal` with
a single cast inside a loop.  With context set and decimals created, the bulk of
the program manipulates the data no differently than with other Python numeric
types.

.. % %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


.. _decimal-decimal:

Decimal objects
---------------


.. class:: Decimal([value [, context]])

   Constructs a new :class:`Decimal` object based from *value*.

   *value* can be an integer, string, tuple, or another :class:`Decimal` object. If
   no *value* is given, returns ``Decimal("0")``.  If *value* is a string, it
   should conform to the decimal numeric string syntax::

      sign           ::=  '+' | '-'
      digit          ::=  '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
      indicator      ::=  'e' | 'E'
      digits         ::=  digit [digit]...
      decimal-part   ::=  digits '.' [digits] | ['.'] digits
      exponent-part  ::=  indicator [sign] digits
      infinity       ::=  'Infinity' | 'Inf'
      nan            ::=  'NaN' [digits] | 'sNaN' [digits]
      numeric-value  ::=  decimal-part [exponent-part] | infinity
      numeric-string ::=  [sign] numeric-value | [sign] nan  

   If *value* is a :class:`tuple`, it should have three components, a sign
   (:const:`0` for positive or :const:`1` for negative), a :class:`tuple` of
   digits, and an integer exponent. For example, ``Decimal((0, (1, 4, 1, 4), -3))``
   returns ``Decimal("1.414")``.

   The *context* precision does not affect how many digits are stored. That is
   determined exclusively by the number of digits in *value*. For example,
   ``Decimal("3.00000")`` records all five zeroes even if the context precision is
   only three.

   The purpose of the *context* argument is determining what to do if *value* is a
   malformed string.  If the context traps :const:`InvalidOperation`, an exception
   is raised; otherwise, the constructor returns a new Decimal with the value of
   :const:`NaN`.

   Once constructed, :class:`Decimal` objects are immutable.

Decimal floating point objects share many properties with the other builtin
numeric types such as :class:`float` and :class:`int`.  All of the usual math
operations and special methods apply.  Likewise, decimal objects can be copied,
pickled, printed, used as dictionary keys, used as set elements, compared,
sorted, and coerced to another type (such as :class:`float` or :class:`long`).

In addition to the standard numeric properties, decimal floating point objects
also have a number of specialized methods:


.. method:: Decimal.adjusted()

   Return the adjusted exponent after shifting out the coefficient's rightmost
   digits until only the lead digit remains: ``Decimal("321e+5").adjusted()``
   returns seven.  Used for determining the position of the most significant digit
   with respect to the decimal point.


.. method:: Decimal.as_tuple()

   Returns a tuple representation of the number: ``(sign, digittuple, exponent)``.


.. method:: Decimal.compare(other[, context])

   Compares like :meth:`__cmp__` but returns a decimal instance::

      a or b is a NaN ==> Decimal("NaN")
      a < b           ==> Decimal("-1")
      a == b          ==> Decimal("0")
      a > b           ==> Decimal("1")


.. method:: Decimal.max(other[, context])

   Like ``max(self, other)`` except that the context rounding rule is applied
   before returning and that :const:`NaN` values are either signalled or ignored
   (depending on the context and whether they are signaling or quiet).


.. method:: Decimal.min(other[, context])

   Like ``min(self, other)`` except that the context rounding rule is applied
   before returning and that :const:`NaN` values are either signalled or ignored
   (depending on the context and whether they are signaling or quiet).


.. method:: Decimal.normalize([context])

   Normalize the number by stripping the rightmost trailing zeroes and converting
   any result equal to :const:`Decimal("0")` to :const:`Decimal("0e0")`. Used for
   producing canonical values for members of an equivalence class. For example,
   ``Decimal("32.100")`` and ``Decimal("0.321000e+2")`` both normalize to the
   equivalent value ``Decimal("32.1")``.


.. method:: Decimal.quantize(exp [, rounding[, context[, watchexp]]])

   Quantize makes the exponent the same as *exp*.  Searches for a rounding method
   in *rounding*, then in *context*, and then in the current context.

   If *watchexp* is set (default), then an error is returned whenever the resulting
   exponent is greater than :attr:`Emax` or less than :attr:`Etiny`.


.. method:: Decimal.remainder_near(other[, context])

   Computes the modulo as either a positive or negative value depending on which is
   closest to zero.  For instance, ``Decimal(10).remainder_near(6)`` returns
   ``Decimal("-2")`` which is closer to zero than ``Decimal("4")``.

   If both are equally close, the one chosen will have the same sign as *self*.


.. method:: Decimal.same_quantum(other[, context])

   Test whether self and other have the same exponent or whether both are
   :const:`NaN`.


.. method:: Decimal.sqrt([context])

   Return the square root to full precision.


.. method:: Decimal.to_eng_string([context])

   Convert to an engineering-type string.

   Engineering notation has an exponent which is a multiple of 3, so there are up
   to 3 digits left of the decimal place.  For example, converts
   ``Decimal('123E+1')`` to ``Decimal("1.23E+3")``


.. method:: Decimal.to_integral([rounding[, context]])

   Rounds to the nearest integer without signaling :const:`Inexact` or
   :const:`Rounded`.  If given, applies *rounding*; otherwise, uses the rounding
   method in either the supplied *context* or the current context.

.. % %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


.. _decimal-context:

Context objects
---------------

Contexts are environments for arithmetic operations.  They govern precision, set
rules for rounding, determine which signals are treated as exceptions, and limit
the range for exponents.

Each thread has its own current context which is accessed or changed using the
:func:`getcontext` and :func:`setcontext` functions:


.. function:: getcontext()

   Return the current context for the active thread.


.. function:: setcontext(c)

   Set the current context for the active thread to *c*.

Beginning with Python 2.5, you can also use the :keyword:`with` statement and
the :func:`localcontext` function to temporarily change the active context.


.. function:: localcontext([c])

   Return a context manager that will set the current context for the active thread
   to a copy of *c* on entry to the with-statement and restore the previous context
   when exiting the with-statement. If no context is specified, a copy of the
   current context is used.

   .. versionadded:: 2.5

   For example, the following code sets the current decimal precision to 42 places,
   performs a calculation, and then automatically restores the previous context::

      from __future__ import with_statement
      from decimal import localcontext

      with localcontext() as ctx:
          ctx.prec = 42   # Perform a high precision calculation
          s = calculate_something()
      s = +s  # Round the final result back to the default precision

New contexts can also be created using the :class:`Context` constructor
described below. In addition, the module provides three pre-made contexts:


.. class:: BasicContext

   This is a standard context defined by the General Decimal Arithmetic
   Specification.  Precision is set to nine.  Rounding is set to
   :const:`ROUND_HALF_UP`.  All flags are cleared.  All traps are enabled (treated
   as exceptions) except :const:`Inexact`, :const:`Rounded`, and
   :const:`Subnormal`.

   Because many of the traps are enabled, this context is useful for debugging.


.. class:: ExtendedContext

   This is a standard context defined by the General Decimal Arithmetic
   Specification.  Precision is set to nine.  Rounding is set to
   :const:`ROUND_HALF_EVEN`.  All flags are cleared.  No traps are enabled (so that
   exceptions are not raised during computations).

   Because the trapped are disabled, this context is useful for applications that
   prefer to have result value of :const:`NaN` or :const:`Infinity` instead of
   raising exceptions.  This allows an application to complete a run in the
   presence of conditions that would otherwise halt the program.


.. class:: DefaultContext

   This context is used by the :class:`Context` constructor as a prototype for new
   contexts.  Changing a field (such a precision) has the effect of changing the
   default for new contexts creating by the :class:`Context` constructor.

   This context is most useful in multi-threaded environments.  Changing one of the
   fields before threads are started has the effect of setting system-wide
   defaults.  Changing the fields after threads have started is not recommended as
   it would require thread synchronization to prevent race conditions.

   In single threaded environments, it is preferable to not use this context at
   all.  Instead, simply create contexts explicitly as described below.

   The default values are precision=28, rounding=ROUND_HALF_EVEN, and enabled traps
   for Overflow, InvalidOperation, and DivisionByZero.

In addition to the three supplied contexts, new contexts can be created with the
:class:`Context` constructor.


.. class:: Context(prec=None, rounding=None, traps=None, flags=None, Emin=None, Emax=None, capitals=1)

   Creates a new context.  If a field is not specified or is :const:`None`, the
   default values are copied from the :const:`DefaultContext`.  If the *flags*
   field is not specified or is :const:`None`, all flags are cleared.

   The *prec* field is a positive integer that sets the precision for arithmetic
   operations in the context.

   The *rounding* option is one of:

   * :const:`ROUND_CEILING` (towards :const:`Infinity`),
   * :const:`ROUND_DOWN` (towards zero),
   * :const:`ROUND_FLOOR` (towards :const:`-Infinity`),
   * :const:`ROUND_HALF_DOWN` (to nearest with ties going towards zero),
   * :const:`ROUND_HALF_EVEN` (to nearest with ties going to nearest even integer),
   * :const:`ROUND_HALF_UP` (to nearest with ties going away from zero), or
   * :const:`ROUND_UP` (away from zero).

   The *traps* and *flags* fields list any signals to be set. Generally, new
   contexts should only set traps and leave the flags clear.

   The *Emin* and *Emax* fields are integers specifying the outer limits allowable
   for exponents.

   The *capitals* field is either :const:`0` or :const:`1` (the default). If set to
   :const:`1`, exponents are printed with a capital :const:`E`; otherwise, a
   lowercase :const:`e` is used: :const:`Decimal('6.02e+23')`.

The :class:`Context` class defines several general purpose methods as well as a
large number of methods for doing arithmetic directly in a given context.


.. method:: Context.clear_flags()

   Resets all of the flags to :const:`0`.


.. method:: Context.copy()

   Return a duplicate of the context.


.. method:: Context.create_decimal(num)

   Creates a new Decimal instance from *num* but using *self* as context. Unlike
   the :class:`Decimal` constructor, the context precision, rounding method, flags,
   and traps are applied to the conversion.

   This is useful because constants are often given to a greater precision than is
   needed by the application.  Another benefit is that rounding immediately
   eliminates unintended effects from digits beyond the current precision. In the
   following example, using unrounded inputs means that adding zero to a sum can
   change the result::

      >>> getcontext().prec = 3
      >>> Decimal("3.4445") + Decimal("1.0023")
      Decimal("4.45")
      >>> Decimal("3.4445") + Decimal(0) + Decimal("1.0023")
      Decimal("4.44")


.. method:: Context.Etiny()

   Returns a value equal to ``Emin - prec + 1`` which is the minimum exponent value
   for subnormal results.  When underflow occurs, the exponent is set to
   :const:`Etiny`.


.. method:: Context.Etop()

   Returns a value equal to ``Emax - prec + 1``.

The usual approach to working with decimals is to create :class:`Decimal`
instances and then apply arithmetic operations which take place within the
current context for the active thread.  An alternate approach is to use context
methods for calculating within a specific context.  The methods are similar to
those for the :class:`Decimal` class and are only briefly recounted here.


.. method:: Context.abs(x)

   Returns the absolute value of *x*.


.. method:: Context.add(x, y)

   Return the sum of *x* and *y*.


.. method:: Context.compare(x, y)

   Compares values numerically.

   Like :meth:`__cmp__` but returns a decimal instance::

      a or b is a NaN ==> Decimal("NaN")
      a < b           ==> Decimal("-1")
      a == b          ==> Decimal("0")
      a > b           ==> Decimal("1")


.. method:: Context.divide(x, y)

   Return *x* divided by *y*.


.. method:: Context.divmod(x, y)

   Divides two numbers and returns the integer part of the result.


.. method:: Context.max(x, y)

   Compare two values numerically and return the maximum.

   If they are numerically equal then the left-hand operand is chosen as the
   result.


.. method:: Context.min(x, y)

   Compare two values numerically and return the minimum.

   If they are numerically equal then the left-hand operand is chosen as the
   result.


.. method:: Context.minus(x)

   Minus corresponds to the unary prefix minus operator in Python.


.. method:: Context.multiply(x, y)

   Return the product of *x* and *y*.


.. method:: Context.normalize(x)

   Normalize reduces an operand to its simplest form.

   Essentially a :meth:`plus` operation with all trailing zeros removed from the
   result.


.. method:: Context.plus(x)

   Plus corresponds to the unary prefix plus operator in Python.  This operation
   applies the context precision and rounding, so it is *not* an identity
   operation.


.. method:: Context.power(x, y[, modulo])

   Return ``x ** y`` to the *modulo* if given.

   The right-hand operand must be a whole number whose integer part (after any
   exponent has been applied) has no more than 9 digits and whose fractional part
   (if any) is all zeros before any rounding. The operand may be positive,
   negative, or zero; if negative, the absolute value of the power is used, and the
   left-hand operand is inverted (divided into 1) before use.

   If the increased precision needed for the intermediate calculations exceeds the
   capabilities of the implementation then an :const:`InvalidOperation` condition
   is signaled.

   If, when raising to a negative power, an underflow occurs during the division
   into 1, the operation is not halted at that point but continues.


.. method:: Context.quantize(x, y)

   Returns a value equal to *x* after rounding and having the exponent of *y*.

   Unlike other operations, if the length of the coefficient after the quantize
   operation would be greater than precision, then an :const:`InvalidOperation` is
   signaled. This guarantees that, unless there is an error condition, the
   quantized exponent is always equal to that of the right-hand operand.

   Also unlike other operations, quantize never signals Underflow, even if the
   result is subnormal and inexact.


.. method:: Context.remainder(x, y)

   Returns the remainder from integer division.

   The sign of the result, if non-zero, is the same as that of the original
   dividend.


.. method:: Context.remainder_near(x, y)

   Computed the modulo as either a positive or negative value depending on which is
   closest to zero.  For instance, ``Decimal(10).remainder_near(6)`` returns
   ``Decimal("-2")`` which is closer to zero than ``Decimal("4")``.

   If both are equally close, the one chosen will have the same sign as *self*.


.. method:: Context.same_quantum(x, y)

   Test whether *x* and *y* have the same exponent or whether both are
   :const:`NaN`.


.. method:: Context.sqrt(x)

   Return the square root of *x* to full precision.


.. method:: Context.subtract(x, y)

   Return the difference between *x* and *y*.


.. method:: Context.to_eng_string()

   Convert to engineering-type string.

   Engineering notation has an exponent which is a multiple of 3, so there are up
   to 3 digits left of the decimal place.  For example, converts
   ``Decimal('123E+1')`` to ``Decimal("1.23E+3")``


.. method:: Context.to_integral(x)

   Rounds to the nearest integer without signaling :const:`Inexact` or
   :const:`Rounded`.


.. method:: Context.to_sci_string(x)

   Converts a number to a string using scientific notation.

.. % %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


.. _decimal-signals:

Signals
-------

Signals represent conditions that arise during computation. Each corresponds to
one context flag and one context trap enabler.

The context flag is incremented whenever the condition is encountered. After the
computation, flags may be checked for informational purposes (for instance, to
determine whether a computation was exact). After checking the flags, be sure to
clear all flags before starting the next computation.

If the context's trap enabler is set for the signal, then the condition causes a
Python exception to be raised.  For example, if the :class:`DivisionByZero` trap
is set, then a :exc:`DivisionByZero` exception is raised upon encountering the
condition.


.. class:: Clamped

   Altered an exponent to fit representation constraints.

   Typically, clamping occurs when an exponent falls outside the context's
   :attr:`Emin` and :attr:`Emax` limits.  If possible, the exponent is reduced to
   fit by adding zeroes to the coefficient.


.. class:: DecimalException

   Base class for other signals and a subclass of :exc:`ArithmeticError`.


.. class:: DivisionByZero

   Signals the division of a non-infinite number by zero.

   Can occur with division, modulo division, or when raising a number to a negative
   power.  If this signal is not trapped, returns :const:`Infinity` or
   :const:`-Infinity` with the sign determined by the inputs to the calculation.


.. class:: Inexact

   Indicates that rounding occurred and the result is not exact.

   Signals when non-zero digits were discarded during rounding. The rounded result
   is returned.  The signal flag or trap is used to detect when results are
   inexact.


.. class:: InvalidOperation

   An invalid operation was performed.

   Indicates that an operation was requested that does not make sense. If not
   trapped, returns :const:`NaN`.  Possible causes include::

      Infinity - Infinity
      0 * Infinity
      Infinity / Infinity
      x % 0
      Infinity % x
      x._rescale( non-integer )
      sqrt(-x) and x > 0
      0 ** 0
      x ** (non-integer)
      x ** Infinity      


.. class:: Overflow

   Numerical overflow.

   Indicates the exponent is larger than :attr:`Emax` after rounding has occurred.
   If not trapped, the result depends on the rounding mode, either pulling inward
   to the largest representable finite number or rounding outward to
   :const:`Infinity`.  In either case, :class:`Inexact` and :class:`Rounded` are
   also signaled.


.. class:: Rounded

   Rounding occurred though possibly no information was lost.

   Signaled whenever rounding discards digits; even if those digits are zero (such
   as rounding :const:`5.00` to :const:`5.0`).   If not trapped, returns the result
   unchanged.  This signal is used to detect loss of significant digits.


.. class:: Subnormal

   Exponent was lower than :attr:`Emin` prior to rounding.

   Occurs when an operation result is subnormal (the exponent is too small). If not
   trapped, returns the result unchanged.


.. class:: Underflow

   Numerical underflow with result rounded to zero.

   Occurs when a subnormal result is pushed to zero by rounding. :class:`Inexact`
   and :class:`Subnormal` are also signaled.

The following table summarizes the hierarchy of signals::

   exceptions.ArithmeticError(exceptions.Exception)
       DecimalException
           Clamped
           DivisionByZero(DecimalException, exceptions.ZeroDivisionError)
           Inexact
               Overflow(Inexact, Rounded)
               Underflow(Inexact, Rounded, Subnormal)
           InvalidOperation
           Rounded
           Subnormal

.. % %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


.. _decimal-notes:

Floating Point Notes
--------------------


Mitigating round-off error with increased precision
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The use of decimal floating point eliminates decimal representation error
(making it possible to represent :const:`0.1` exactly); however, some operations
can still incur round-off error when non-zero digits exceed the fixed precision.

The effects of round-off error can be amplified by the addition or subtraction
of nearly offsetting quantities resulting in loss of significance.  Knuth
provides two instructive examples where rounded floating point arithmetic with
insufficient precision causes the breakdown of the associative and distributive
properties of addition::

   # Examples from Seminumerical Algorithms, Section 4.2.2.
   >>> from decimal import Decimal, getcontext
   >>> getcontext().prec = 8

   >>> u, v, w = Decimal(11111113), Decimal(-11111111), Decimal('7.51111111')
   >>> (u + v) + w
   Decimal("9.5111111")
   >>> u + (v + w)
   Decimal("10")

   >>> u, v, w = Decimal(20000), Decimal(-6), Decimal('6.0000003')
   >>> (u*v) + (u*w)
   Decimal("0.01")
   >>> u * (v+w)
   Decimal("0.0060000")

The :mod:`decimal` module makes it possible to restore the identities by
expanding the precision sufficiently to avoid loss of significance::

   >>> getcontext().prec = 20
   >>> u, v, w = Decimal(11111113), Decimal(-11111111), Decimal('7.51111111')
   >>> (u + v) + w
   Decimal("9.51111111")
   >>> u + (v + w)
   Decimal("9.51111111")
   >>> 
   >>> u, v, w = Decimal(20000), Decimal(-6), Decimal('6.0000003')
   >>> (u*v) + (u*w)
   Decimal("0.0060000")
   >>> u * (v+w)
   Decimal("0.0060000")


Special values
^^^^^^^^^^^^^^

The number system for the :mod:`decimal` module provides special values
including :const:`NaN`, :const:`sNaN`, :const:`-Infinity`, :const:`Infinity`,
and two zeroes, :const:`+0` and :const:`-0`.

Infinities can be constructed directly with:  ``Decimal('Infinity')``. Also,
they can arise from dividing by zero when the :exc:`DivisionByZero` signal is
not trapped.  Likewise, when the :exc:`Overflow` signal is not trapped, infinity
can result from rounding beyond the limits of the largest representable number.

The infinities are signed (affine) and can be used in arithmetic operations
where they get treated as very large, indeterminate numbers.  For instance,
adding a constant to infinity gives another infinite result.

Some operations are indeterminate and return :const:`NaN`, or if the
:exc:`InvalidOperation` signal is trapped, raise an exception.  For example,
``0/0`` returns :const:`NaN` which means "not a number".  This variety of
:const:`NaN` is quiet and, once created, will flow through other computations
always resulting in another :const:`NaN`.  This behavior can be useful for a
series of computations that occasionally have missing inputs --- it allows the
calculation to proceed while flagging specific results as invalid.

A variant is :const:`sNaN` which signals rather than remaining quiet after every
operation.  This is a useful return value when an invalid result needs to
interrupt a calculation for special handling.

The signed zeros can result from calculations that underflow. They keep the sign
that would have resulted if the calculation had been carried out to greater
precision.  Since their magnitude is zero, both positive and negative zeros are
treated as equal and their sign is informational.

In addition to the two signed zeros which are distinct yet equal, there are
various representations of zero with differing precisions yet equivalent in
value.  This takes a bit of getting used to.  For an eye accustomed to
normalized floating point representations, it is not immediately obvious that
the following calculation returns a value equal to zero::

   >>> 1 / Decimal('Infinity')
   Decimal("0E-1000000026")

.. % %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


.. _decimal-threads:

Working with threads
--------------------

The :func:`getcontext` function accesses a different :class:`Context` object for
each thread.  Having separate thread contexts means that threads may make
changes (such as ``getcontext.prec=10``) without interfering with other threads.

Likewise, the :func:`setcontext` function automatically assigns its target to
the current thread.

If :func:`setcontext` has not been called before :func:`getcontext`, then
:func:`getcontext` will automatically create a new context for use in the
current thread.

The new context is copied from a prototype context called *DefaultContext*. To
control the defaults so that each thread will use the same values throughout the
application, directly modify the *DefaultContext* object. This should be done
*before* any threads are started so that there won't be a race condition between
threads calling :func:`getcontext`. For example::

   # Set applicationwide defaults for all threads about to be launched
   DefaultContext.prec = 12
   DefaultContext.rounding = ROUND_DOWN
   DefaultContext.traps = ExtendedContext.traps.copy()
   DefaultContext.traps[InvalidOperation] = 1
   setcontext(DefaultContext)

   # Afterwards, the threads can be started
   t1.start()
   t2.start()
   t3.start()
    . . .

.. % %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


.. _decimal-recipes:

Recipes
-------

Here are a few recipes that serve as utility functions and that demonstrate ways
to work with the :class:`Decimal` class::

   def moneyfmt(value, places=2, curr='', sep=',', dp='.',
                pos='', neg='-', trailneg=''):
       """Convert Decimal to a money formatted string.

       places:  required number of places after the decimal point
       curr:    optional currency symbol before the sign (may be blank)
       sep:     optional grouping separator (comma, period, space, or blank)
       dp:      decimal point indicator (comma or period)
                only specify as blank when places is zero
       pos:     optional sign for positive numbers: '+', space or blank
       neg:     optional sign for negative numbers: '-', '(', space or blank
       trailneg:optional trailing minus indicator:  '-', ')', space or blank

       >>> d = Decimal('-1234567.8901')
       >>> moneyfmt(d, curr='$')
       '-$1,234,567.89'
       >>> moneyfmt(d, places=0, sep='.', dp='', neg='', trailneg='-')
       '1.234.568-'
       >>> moneyfmt(d, curr='$', neg='(', trailneg=')')
       '($1,234,567.89)'
       >>> moneyfmt(Decimal(123456789), sep=' ')
       '123 456 789.00'
       >>> moneyfmt(Decimal('-0.02'), neg='<', trailneg='>')
       '<.02>'

       """
       q = Decimal((0, (1,), -places))    # 2 places --> '0.01'
       sign, digits, exp = value.quantize(q).as_tuple()
       assert exp == -places    
       result = []
       digits = map(str, digits)
       build, next = result.append, digits.pop
       if sign:
           build(trailneg)
       for i in range(places):
           if digits:
               build(next())
           else:
               build('0')
       build(dp)
       i = 0
       while digits:
           build(next())
           i += 1
           if i == 3 and digits:
               i = 0
               build(sep)
       build(curr)
       if sign:
           build(neg)
       else:
           build(pos)
       result.reverse()
       return ''.join(result)

   def pi():
       """Compute Pi to the current precision.

       >>> print pi()
       3.141592653589793238462643383

       """
       getcontext().prec += 2  # extra digits for intermediate steps
       three = Decimal(3)      # substitute "three=3.0" for regular floats
       lasts, t, s, n, na, d, da = 0, three, 3, 1, 0, 0, 24
       while s != lasts:
           lasts = s
           n, na = n+na, na+8
           d, da = d+da, da+32
           t = (t * n) / d
           s += t
       getcontext().prec -= 2
       return +s               # unary plus applies the new precision

   def exp(x):
       """Return e raised to the power of x.  Result type matches input type.

       >>> print exp(Decimal(1))
       2.718281828459045235360287471
       >>> print exp(Decimal(2))
       7.389056098930650227230427461
       >>> print exp(2.0)
       7.38905609893
       >>> print exp(2+0j)
       (7.38905609893+0j)

       """
       getcontext().prec += 2
       i, lasts, s, fact, num = 0, 0, 1, 1, 1
       while s != lasts:
           lasts = s    
           i += 1
           fact *= i
           num *= x     
           s += num / fact   
       getcontext().prec -= 2        
       return +s

   def cos(x):
       """Return the cosine of x as measured in radians.

       >>> print cos(Decimal('0.5'))
       0.8775825618903727161162815826
       >>> print cos(0.5)
       0.87758256189
       >>> print cos(0.5+0j)
       (0.87758256189+0j)

       """
       getcontext().prec += 2
       i, lasts, s, fact, num, sign = 0, 0, 1, 1, 1, 1
       while s != lasts:
           lasts = s    
           i += 2
           fact *= i * (i-1)
           num *= x * x
           sign *= -1
           s += num / fact * sign 
       getcontext().prec -= 2        
       return +s

   def sin(x):
       """Return the sine of x as measured in radians.

       >>> print sin(Decimal('0.5'))
       0.4794255386042030002732879352
       >>> print sin(0.5)
       0.479425538604
       >>> print sin(0.5+0j)
       (0.479425538604+0j)

       """
       getcontext().prec += 2
       i, lasts, s, fact, num, sign = 1, 0, x, 1, x, 1
       while s != lasts:
           lasts = s    
           i += 2
           fact *= i * (i-1)
           num *= x * x
           sign *= -1
           s += num / fact * sign 
       getcontext().prec -= 2        
       return +s


.. % %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


.. _decimal-faq:

Decimal FAQ
-----------

Q. It is cumbersome to type ``decimal.Decimal('1234.5')``.  Is there a way to
minimize typing when using the interactive interpreter?

\A. Some users abbreviate the constructor to just a single letter::

   >>> D = decimal.Decimal
   >>> D('1.23') + D('3.45')
   Decimal("4.68")

Q. In a fixed-point application with two decimal places, some inputs have many
places and need to be rounded.  Others are not supposed to have excess digits
and need to be validated.  What methods should be used?

A. The :meth:`quantize` method rounds to a fixed number of decimal places. If
the :const:`Inexact` trap is set, it is also useful for validation::

   >>> TWOPLACES = Decimal(10) ** -2       # same as Decimal('0.01')

   >>> # Round to two places
   >>> Decimal("3.214").quantize(TWOPLACES)
   Decimal("3.21")

   >>> # Validate that a number does not exceed two places 
   >>> Decimal("3.21").quantize(TWOPLACES, context=Context(traps=[Inexact]))
   Decimal("3.21")

   >>> Decimal("3.214").quantize(TWOPLACES, context=Context(traps=[Inexact]))
   Traceback (most recent call last):
      ...
   Inexact: Changed in rounding

Q. Once I have valid two place inputs, how do I maintain that invariant
throughout an application?

A. Some operations like addition and subtraction automatically preserve fixed
point.  Others, like multiplication and division, change the number of decimal
places and need to be followed-up with a :meth:`quantize` step.

Q. There are many ways to express the same value.  The numbers :const:`200`,
:const:`200.000`, :const:`2E2`, and :const:`.02E+4` all have the same value at
various precisions. Is there a way to transform them to a single recognizable
canonical value?

A. The :meth:`normalize` method maps all equivalent values to a single
representative::

   >>> values = map(Decimal, '200 200.000 2E2 .02E+4'.split())
   >>> [v.normalize() for v in values]
   [Decimal("2E+2"), Decimal("2E+2"), Decimal("2E+2"), Decimal("2E+2")]

Q. Some decimal values always print with exponential notation.  Is there a way
to get a non-exponential representation?

A. For some values, exponential notation is the only way to express the number
of significant places in the coefficient.  For example, expressing
:const:`5.0E+3` as :const:`5000` keeps the value constant but cannot show the
original's two-place significance.

Q. Is there a way to convert a regular float to a :class:`Decimal`?

A. Yes, all binary floating point numbers can be exactly expressed as a
Decimal.  An exact conversion may take more precision than intuition would
suggest, so trapping :const:`Inexact` will signal a need for more precision::

   def floatToDecimal(f):
       "Convert a floating point number to a Decimal with no loss of information"
       # Transform (exactly) a float to a mantissa (0.5 <= abs(m) < 1.0) and an
       # exponent.  Double the mantissa until it is an integer.  Use the integer
       # mantissa and exponent to compute an equivalent Decimal.  If this cannot
       # be done exactly, then retry with more precision.

       mantissa, exponent = math.frexp(f)
       while mantissa != int(mantissa):
           mantissa *= 2.0
           exponent -= 1
       mantissa = int(mantissa)

       oldcontext = getcontext()
       setcontext(Context(traps=[Inexact]))
       try:
           while True:
               try:
                  return mantissa * Decimal(2) ** exponent
               except Inexact:
                   getcontext().prec += 1
       finally:
           setcontext(oldcontext)

Q. Why isn't the :func:`floatToDecimal` routine included in the module?

A. There is some question about whether it is advisable to mix binary and
decimal floating point.  Also, its use requires some care to avoid the
representation issues associated with binary floating point::

   >>> floatToDecimal(1.1)
   Decimal("1.100000000000000088817841970012523233890533447265625")

Q. Within a complex calculation, how can I make sure that I haven't gotten a
spurious result because of insufficient precision or rounding anomalies.

A. The decimal module makes it easy to test results.  A best practice is to
re-run calculations using greater precision and with various rounding modes.
Widely differing results indicate insufficient precision, rounding mode issues,
ill-conditioned inputs, or a numerically unstable algorithm.

Q. I noticed that context precision is applied to the results of operations but
not to the inputs.  Is there anything to watch out for when mixing values of
different precisions?

A. Yes.  The principle is that all values are considered to be exact and so is
the arithmetic on those values.  Only the results are rounded.  The advantage
for inputs is that "what you type is what you get".  A disadvantage is that the
results can look odd if you forget that the inputs haven't been rounded::

   >>> getcontext().prec = 3
   >>> Decimal('3.104') + D('2.104')
   Decimal("5.21")
   >>> Decimal('3.104') + D('0.000') + D('2.104')
   Decimal("5.20")

The solution is either to increase precision or to force rounding of inputs
using the unary plus operation::

   >>> getcontext().prec = 3
   >>> +Decimal('1.23456789')      # unary plus triggers rounding
   Decimal("1.23")

Alternatively, inputs can be rounded upon creation using the
:meth:`Context.create_decimal` method::

   >>> Context(prec=5, rounding=ROUND_DOWN).create_decimal('1.2345678')
   Decimal("1.2345")

