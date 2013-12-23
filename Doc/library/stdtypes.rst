.. XXX: reference/datamodel and this have quite a few overlaps!


.. _bltin-types:

**************
Built-in Types
**************

The following sections describe the standard types that are built into the
interpreter.

.. index:: pair: built-in; types

The principal built-in types are numerics, sequences, mappings, classes,
instances and exceptions.

Some collection classes are mutable.  The methods that add, subtract, or
rearrange their members in place, and don't return a specific item, never return
the collection instance itself but ``None``.

Some operations are supported by several object types; in particular,
practically all objects can be compared, tested for truth value, and converted
to a string (with the :func:`repr` function or the slightly different
:func:`str` function).  The latter function is implicitly used when an object is
written by the :func:`print` function.


.. _truth:

Truth Value Testing
===================

.. index::
   statement: if
   statement: while
   pair: truth; value
   pair: Boolean; operations
   single: false

Any object can be tested for truth value, for use in an :keyword:`if` or
:keyword:`while` condition or as operand of the Boolean operations below. The
following values are considered false:

  .. index:: single: None (Built-in object)

* ``None``

  .. index:: single: False (Built-in object)

* ``False``

* zero of any numeric type, for example, ``0``, ``0.0``, ``0j``.

* any empty sequence, for example, ``''``, ``()``, ``[]``.

* any empty mapping, for example, ``{}``.

* instances of user-defined classes, if the class defines a :meth:`__bool__` or
  :meth:`__len__` method, when that method returns the integer zero or
  :class:`bool` value ``False``. [1]_

.. index:: single: true

All other values are considered true --- so objects of many types are always
true.

.. index::
   operator: or
   operator: and
   single: False
   single: True

Operations and built-in functions that have a Boolean result always return ``0``
or ``False`` for false and ``1`` or ``True`` for true, unless otherwise stated.
(Important exception: the Boolean operations ``or`` and ``and`` always return
one of their operands.)


.. _boolean:

Boolean Operations --- :keyword:`and`, :keyword:`or`, :keyword:`not`
====================================================================

.. index:: pair: Boolean; operations

These are the Boolean operations, ordered by ascending priority:

+-------------+---------------------------------+-------+
| Operation   | Result                          | Notes |
+=============+=================================+=======+
| ``x or y``  | if *x* is false, then *y*, else | \(1)  |
|             | *x*                             |       |
+-------------+---------------------------------+-------+
| ``x and y`` | if *x* is false, then *x*, else | \(2)  |
|             | *y*                             |       |
+-------------+---------------------------------+-------+
| ``not x``   | if *x* is false, then ``True``, | \(3)  |
|             | else ``False``                  |       |
+-------------+---------------------------------+-------+

.. index::
   operator: and
   operator: or
   operator: not

Notes:

(1)
   This is a short-circuit operator, so it only evaluates the second
   argument if the first one is :const:`False`.

(2)
   This is a short-circuit operator, so it only evaluates the second
   argument if the first one is :const:`True`.

(3)
   ``not`` has a lower priority than non-Boolean operators, so ``not a == b`` is
   interpreted as ``not (a == b)``, and ``a == not b`` is a syntax error.


.. _stdcomparisons:

Comparisons
===========

.. index::
   pair: chaining; comparisons
   pair: operator; comparison
   operator: ==
   operator: <
   operator: <=
   operator: >
   operator: >=
   operator: !=
   operator: is
   operator: is not

There are eight comparison operations in Python.  They all have the same
priority (which is higher than that of the Boolean operations).  Comparisons can
be chained arbitrarily; for example, ``x < y <= z`` is equivalent to ``x < y and
y <= z``, except that *y* is evaluated only once (but in both cases *z* is not
evaluated at all when ``x < y`` is found to be false).

This table summarizes the comparison operations:

+------------+-------------------------+
| Operation  | Meaning                 |
+============+=========================+
| ``<``      | strictly less than      |
+------------+-------------------------+
| ``<=``     | less than or equal      |
+------------+-------------------------+
| ``>``      | strictly greater than   |
+------------+-------------------------+
| ``>=``     | greater than or equal   |
+------------+-------------------------+
| ``==``     | equal                   |
+------------+-------------------------+
| ``!=``     | not equal               |
+------------+-------------------------+
| ``is``     | object identity         |
+------------+-------------------------+
| ``is not`` | negated object identity |
+------------+-------------------------+

.. index::
   pair: object; numeric
   pair: objects; comparing

Objects of different types, except different numeric types, never compare equal.
Furthermore, some types (for example, function objects) support only a degenerate
notion of comparison where any two objects of that type are unequal.  The ``<``,
``<=``, ``>`` and ``>=`` operators will raise a :exc:`TypeError` exception when
comparing a complex number with another built-in numeric type, when the objects
are of different types that cannot be compared, or in other cases where there is
no defined ordering.

.. index::
   single: __eq__() (instance method)
   single: __ne__() (instance method)
   single: __lt__() (instance method)
   single: __le__() (instance method)
   single: __gt__() (instance method)
   single: __ge__() (instance method)

Non-identical instances of a class normally compare as non-equal unless the
class defines the :meth:`__eq__` method.

Instances of a class cannot be ordered with respect to other instances of the
same class, or other types of object, unless the class defines enough of the
methods :meth:`__lt__`, :meth:`__le__`, :meth:`__gt__`, and :meth:`__ge__` (in
general, :meth:`__lt__` and :meth:`__eq__` are sufficient, if you want the
conventional meanings of the comparison operators).

The behavior of the :keyword:`is` and :keyword:`is not` operators cannot be
customized; also they can be applied to any two objects and never raise an
exception.

.. index::
   operator: in
   operator: not in

Two more operations with the same syntactic priority, :keyword:`in` and
:keyword:`not in`, are supported only by sequence types (below).


.. _typesnumeric:

Numeric Types --- :class:`int`, :class:`float`, :class:`complex`
================================================================

.. index::
   object: numeric
   object: Boolean
   object: integer
   object: floating point
   object: complex number
   pair: C; language

There are three distinct numeric types: :dfn:`integers`, :dfn:`floating
point numbers`, and :dfn:`complex numbers`.  In addition, Booleans are a
subtype of integers.  Integers have unlimited precision.  Floating point
numbers are usually implemented using :c:type:`double` in C; information
about the precision and internal representation of floating point
numbers for the machine on which your program is running is available
in :data:`sys.float_info`.  Complex numbers have a real and imaginary
part, which are each a floating point number.  To extract these parts
from a complex number *z*, use ``z.real`` and ``z.imag``. (The standard
library includes additional numeric types, :mod:`fractions` that hold
rationals, and :mod:`decimal` that hold floating-point numbers with
user-definable precision.)

.. index::
   pair: numeric; literals
   pair: integer; literals
   pair: floating point; literals
   pair: complex number; literals
   pair: hexadecimal; literals
   pair: octal; literals
   pair: binary; literals

Numbers are created by numeric literals or as the result of built-in functions
and operators.  Unadorned integer literals (including hex, octal and binary
numbers) yield integers.  Numeric literals containing a decimal point or an
exponent sign yield floating point numbers.  Appending ``'j'`` or ``'J'`` to a
numeric literal yields an imaginary number (a complex number with a zero real
part) which you can add to an integer or float to get a complex number with real
and imaginary parts.

.. index::
   single: arithmetic
   builtin: int
   builtin: float
   builtin: complex
   operator: +
   operator: -
   operator: *
   operator: /
   operator: //
   operator: %
   operator: **

Python fully supports mixed arithmetic: when a binary arithmetic operator has
operands of different numeric types, the operand with the "narrower" type is
widened to that of the other, where integer is narrower than floating point,
which is narrower than complex.  Comparisons between numbers of mixed type use
the same rule. [2]_ The constructors :func:`int`, :func:`float`, and
:func:`complex` can be used to produce numbers of a specific type.

All numeric types (except complex) support the following operations, sorted by
ascending priority (operations in the same box have the same priority; all
numeric operations have a higher priority than comparison operations):

+---------------------+---------------------------------+---------+--------------------+
| Operation           | Result                          | Notes   | Full documentation |
+=====================+=================================+=========+====================+
| ``x + y``           | sum of *x* and *y*              |         |                    |
+---------------------+---------------------------------+---------+--------------------+
| ``x - y``           | difference of *x* and *y*       |         |                    |
+---------------------+---------------------------------+---------+--------------------+
| ``x * y``           | product of *x* and *y*          |         |                    |
+---------------------+---------------------------------+---------+--------------------+
| ``x / y``           | quotient of *x* and *y*         |         |                    |
+---------------------+---------------------------------+---------+--------------------+
| ``x // y``          | floored quotient of *x* and     | \(1)    |                    |
|                     | *y*                             |         |                    |
+---------------------+---------------------------------+---------+--------------------+
| ``x % y``           | remainder of ``x / y``          | \(2)    |                    |
+---------------------+---------------------------------+---------+--------------------+
| ``-x``              | *x* negated                     |         |                    |
+---------------------+---------------------------------+---------+--------------------+
| ``+x``              | *x* unchanged                   |         |                    |
+---------------------+---------------------------------+---------+--------------------+
| ``abs(x)``          | absolute value or magnitude of  |         | :func:`abs`        |
|                     | *x*                             |         |                    |
+---------------------+---------------------------------+---------+--------------------+
| ``int(x)``          | *x* converted to integer        | \(3)\(6)| :func:`int`        |
+---------------------+---------------------------------+---------+--------------------+
| ``float(x)``        | *x* converted to floating point | \(4)\(6)| :func:`float`      |
+---------------------+---------------------------------+---------+--------------------+
| ``complex(re, im)`` | a complex number with real part | \(6)    | :func:`complex`    |
|                     | *re*, imaginary part *im*.      |         |                    |
|                     | *im* defaults to zero.          |         |                    |
+---------------------+---------------------------------+---------+--------------------+
|  ``c.conjugate()``  | conjugate of the complex number |         |                    |
|                     | *c*                             |         |                    |
+---------------------+---------------------------------+---------+--------------------+
| ``divmod(x, y)``    | the pair ``(x // y, x % y)``    | \(2)    | :func:`divmod`     |
+---------------------+---------------------------------+---------+--------------------+
| ``pow(x, y)``       | *x* to the power *y*            | \(5)    | :func:`pow`        |
+---------------------+---------------------------------+---------+--------------------+
| ``x ** y``          | *x* to the power *y*            | \(5)    |                    |
+---------------------+---------------------------------+---------+--------------------+

.. index::
   triple: operations on; numeric; types
   single: conjugate() (complex number method)

Notes:

(1)
   Also referred to as integer division.  The resultant value is a whole
   integer, though the result's type is not necessarily int.  The result is
   always rounded towards minus infinity: ``1//2`` is ``0``, ``(-1)//2`` is
   ``-1``, ``1//(-2)`` is ``-1``, and ``(-1)//(-2)`` is ``0``.

(2)
   Not for complex numbers.  Instead convert to floats using :func:`abs` if
   appropriate.

(3)
   .. index::
      module: math
      single: floor() (in module math)
      single: ceil() (in module math)
      single: trunc() (in module math)
      pair: numeric; conversions
      pair: C; language

   Conversion from floating point to integer may round or truncate
   as in C; see functions :func:`math.floor` and :func:`math.ceil` for
   well-defined conversions.

(4)
   float also accepts the strings "nan" and "inf" with an optional prefix "+"
   or "-" for Not a Number (NaN) and positive or negative infinity.

(5)
   Python defines ``pow(0, 0)`` and ``0 ** 0`` to be ``1``, as is common for
   programming languages.

(6)
   The numeric literals accepted include the digits ``0`` to ``9`` or any
   Unicode equivalent (code points with the ``Nd`` property).

   See http://www.unicode.org/Public/6.0.0/ucd/extracted/DerivedNumericType.txt
   for a complete list of code points with the ``Nd`` property.


All :class:`numbers.Real` types (:class:`int` and :class:`float`) also include
the following operations:

+--------------------+------------------------------------+--------+
| Operation          | Result                             | Notes  |
+====================+====================================+========+
| ``math.trunc(x)``  | *x* truncated to Integral          |        |
+--------------------+------------------------------------+--------+
| ``round(x[, n])``  | *x* rounded to n digits,           |        |
|                    | rounding half to even. If n is     |        |
|                    | omitted, it defaults to 0.         |        |
+--------------------+------------------------------------+--------+
| ``math.floor(x)``  | the greatest integral float <= *x* |        |
+--------------------+------------------------------------+--------+
| ``math.ceil(x)``   | the least integral float >= *x*    |        |
+--------------------+------------------------------------+--------+

For additional numeric operations see the :mod:`math` and :mod:`cmath`
modules.

.. XXXJH exceptions: overflow (when? what operations?) zerodivision


.. _bitstring-ops:

Bitwise Operations on Integer Types
--------------------------------------

.. index::
   triple: operations on; integer; types
   pair: bitwise; operations
   pair: shifting; operations
   pair: masking; operations
   operator: ^
   operator: &
   operator: <<
   operator: >>

Bitwise operations only make sense for integers.  Negative numbers are treated
as their 2's complement value (this assumes a sufficiently large number of bits
that no overflow occurs during the operation).

The priorities of the binary bitwise operations are all lower than the numeric
operations and higher than the comparisons; the unary operation ``~`` has the
same priority as the other unary numeric operations (``+`` and ``-``).

This table lists the bitwise operations sorted in ascending priority
(operations in the same box have the same priority):

+------------+--------------------------------+----------+
| Operation  | Result                         | Notes    |
+============+================================+==========+
| ``x | y``  | bitwise :dfn:`or` of *x* and   |          |
|            | *y*                            |          |
+------------+--------------------------------+----------+
| ``x ^ y``  | bitwise :dfn:`exclusive or` of |          |
|            | *x* and *y*                    |          |
+------------+--------------------------------+----------+
| ``x & y``  | bitwise :dfn:`and` of *x* and  |          |
|            | *y*                            |          |
+------------+--------------------------------+----------+
| ``x << n`` | *x* shifted left by *n* bits   | (1)(2)   |
+------------+--------------------------------+----------+
| ``x >> n`` | *x* shifted right by *n* bits  | (1)(3)   |
+------------+--------------------------------+----------+
| ``~x``     | the bits of *x* inverted       |          |
+------------+--------------------------------+----------+

Notes:

(1)
   Negative shift counts are illegal and cause a :exc:`ValueError` to be raised.

(2)
   A left shift by *n* bits is equivalent to multiplication by ``pow(2, n)``
   without overflow check.

(3)
   A right shift by *n* bits is equivalent to division by ``pow(2, n)`` without
   overflow check.


Additional Methods on Integer Types
-----------------------------------

The int type implements the :class:`numbers.Integral` :term:`abstract base
class`. In addition, it provides one more method:

.. method:: int.bit_length()

    Return the number of bits necessary to represent an integer in binary,
    excluding the sign and leading zeros::

        >>> n = -37
        >>> bin(n)
        '-0b100101'
        >>> n.bit_length()
        6

    More precisely, if ``x`` is nonzero, then ``x.bit_length()`` is the
    unique positive integer ``k`` such that ``2**(k-1) <= abs(x) < 2**k``.
    Equivalently, when ``abs(x)`` is small enough to have a correctly
    rounded logarithm, then ``k = 1 + int(log(abs(x), 2))``.
    If ``x`` is zero, then ``x.bit_length()`` returns ``0``.

    Equivalent to::

        def bit_length(self):
            s = bin(self)       # binary representation:  bin(-37) --> '-0b100101'
            s = s.lstrip('-0b') # remove leading zeros and minus sign
            return len(s)       # len('100101') --> 6

    .. versionadded:: 3.1

.. method:: int.to_bytes(length, byteorder, \*, signed=False)

    Return an array of bytes representing an integer.

        >>> (1024).to_bytes(2, byteorder='big')
        b'\x04\x00'
        >>> (1024).to_bytes(10, byteorder='big')
        b'\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00'
        >>> (-1024).to_bytes(10, byteorder='big', signed=True)
        b'\xff\xff\xff\xff\xff\xff\xff\xff\xfc\x00'
        >>> x = 1000
        >>> x.to_bytes((x.bit_length() // 8) + 1, byteorder='little')
        b'\xe8\x03'

    The integer is represented using *length* bytes.  An :exc:`OverflowError`
    is raised if the integer is not representable with the given number of
    bytes.

    The *byteorder* argument determines the byte order used to represent the
    integer.  If *byteorder* is ``"big"``, the most significant byte is at the
    beginning of the byte array.  If *byteorder* is ``"little"``, the most
    significant byte is at the end of the byte array.  To request the native
    byte order of the host system, use :data:`sys.byteorder` as the byte order
    value.

    The *signed* argument determines whether two's complement is used to
    represent the integer.  If *signed* is ``False`` and a negative integer is
    given, an :exc:`OverflowError` is raised. The default value for *signed*
    is ``False``.

    .. versionadded:: 3.2

.. classmethod:: int.from_bytes(bytes, byteorder, \*, signed=False)

    Return the integer represented by the given array of bytes.

        >>> int.from_bytes(b'\x00\x10', byteorder='big')
        16
        >>> int.from_bytes(b'\x00\x10', byteorder='little')
        4096
        >>> int.from_bytes(b'\xfc\x00', byteorder='big', signed=True)
        -1024
        >>> int.from_bytes(b'\xfc\x00', byteorder='big', signed=False)
        64512
        >>> int.from_bytes([255, 0, 0], byteorder='big')
        16711680

    The argument *bytes* must either be a :term:`bytes-like object` or an
    iterable producing bytes.

    The *byteorder* argument determines the byte order used to represent the
    integer.  If *byteorder* is ``"big"``, the most significant byte is at the
    beginning of the byte array.  If *byteorder* is ``"little"``, the most
    significant byte is at the end of the byte array.  To request the native
    byte order of the host system, use :data:`sys.byteorder` as the byte order
    value.

    The *signed* argument indicates whether two's complement is used to
    represent the integer.

    .. versionadded:: 3.2


Additional Methods on Float
---------------------------

The float type implements the :class:`numbers.Real` :term:`abstract base
class`. float also has the following additional methods.

.. method:: float.as_integer_ratio()

   Return a pair of integers whose ratio is exactly equal to the
   original float and with a positive denominator.  Raises
   :exc:`OverflowError` on infinities and a :exc:`ValueError` on
   NaNs.

.. method:: float.is_integer()

   Return ``True`` if the float instance is finite with integral
   value, and ``False`` otherwise::

      >>> (-2.0).is_integer()
      True
      >>> (3.2).is_integer()
      False

Two methods support conversion to
and from hexadecimal strings.  Since Python's floats are stored
internally as binary numbers, converting a float to or from a
*decimal* string usually involves a small rounding error.  In
contrast, hexadecimal strings allow exact representation and
specification of floating-point numbers.  This can be useful when
debugging, and in numerical work.


.. method:: float.hex()

   Return a representation of a floating-point number as a hexadecimal
   string.  For finite floating-point numbers, this representation
   will always include a leading ``0x`` and a trailing ``p`` and
   exponent.


.. classmethod:: float.fromhex(s)

   Class method to return the float represented by a hexadecimal
   string *s*.  The string *s* may have leading and trailing
   whitespace.


Note that :meth:`float.hex` is an instance method, while
:meth:`float.fromhex` is a class method.

A hexadecimal string takes the form::

   [sign] ['0x'] integer ['.' fraction] ['p' exponent]

where the optional ``sign`` may by either ``+`` or ``-``, ``integer``
and ``fraction`` are strings of hexadecimal digits, and ``exponent``
is a decimal integer with an optional leading sign.  Case is not
significant, and there must be at least one hexadecimal digit in
either the integer or the fraction.  This syntax is similar to the
syntax specified in section 6.4.4.2 of the C99 standard, and also to
the syntax used in Java 1.5 onwards.  In particular, the output of
:meth:`float.hex` is usable as a hexadecimal floating-point literal in
C or Java code, and hexadecimal strings produced by C's ``%a`` format
character or Java's ``Double.toHexString`` are accepted by
:meth:`float.fromhex`.


Note that the exponent is written in decimal rather than hexadecimal,
and that it gives the power of 2 by which to multiply the coefficient.
For example, the hexadecimal string ``0x3.a7p10`` represents the
floating-point number ``(3 + 10./16 + 7./16**2) * 2.0**10``, or
``3740.0``::

   >>> float.fromhex('0x3.a7p10')
   3740.0


Applying the reverse conversion to ``3740.0`` gives a different
hexadecimal string representing the same number::

   >>> float.hex(3740.0)
   '0x1.d380000000000p+11'


.. _numeric-hash:

Hashing of numeric types
------------------------

For numbers ``x`` and ``y``, possibly of different types, it's a requirement
that ``hash(x) == hash(y)`` whenever ``x == y`` (see the :meth:`__hash__`
method documentation for more details).  For ease of implementation and
efficiency across a variety of numeric types (including :class:`int`,
:class:`float`, :class:`decimal.Decimal` and :class:`fractions.Fraction`)
Python's hash for numeric types is based on a single mathematical function
that's defined for any rational number, and hence applies to all instances of
:class:`int` and :class:`fractions.Fraction`, and all finite instances of
:class:`float` and :class:`decimal.Decimal`.  Essentially, this function is
given by reduction modulo ``P`` for a fixed prime ``P``.  The value of ``P`` is
made available to Python as the :attr:`modulus` attribute of
:data:`sys.hash_info`.

.. impl-detail::

   Currently, the prime used is ``P = 2**31 - 1`` on machines with 32-bit C
   longs and ``P = 2**61 - 1`` on machines with 64-bit C longs.

Here are the rules in detail:

- If ``x = m / n`` is a nonnegative rational number and ``n`` is not divisible
  by ``P``, define ``hash(x)`` as ``m * invmod(n, P) % P``, where ``invmod(n,
  P)`` gives the inverse of ``n`` modulo ``P``.

- If ``x = m / n`` is a nonnegative rational number and ``n`` is
  divisible by ``P`` (but ``m`` is not) then ``n`` has no inverse
  modulo ``P`` and the rule above doesn't apply; in this case define
  ``hash(x)`` to be the constant value ``sys.hash_info.inf``.

- If ``x = m / n`` is a negative rational number define ``hash(x)``
  as ``-hash(-x)``.  If the resulting hash is ``-1``, replace it with
  ``-2``.

- The particular values ``sys.hash_info.inf``, ``-sys.hash_info.inf``
  and ``sys.hash_info.nan`` are used as hash values for positive
  infinity, negative infinity, or nans (respectively).  (All hashable
  nans have the same hash value.)

- For a :class:`complex` number ``z``, the hash values of the real
  and imaginary parts are combined by computing ``hash(z.real) +
  sys.hash_info.imag * hash(z.imag)``, reduced modulo
  ``2**sys.hash_info.width`` so that it lies in
  ``range(-2**(sys.hash_info.width - 1), 2**(sys.hash_info.width -
  1))``.  Again, if the result is ``-1``, it's replaced with ``-2``.


To clarify the above rules, here's some example Python code,
equivalent to the built-in hash, for computing the hash of a rational
number, :class:`float`, or :class:`complex`::


   import sys, math

   def hash_fraction(m, n):
       """Compute the hash of a rational number m / n.

       Assumes m and n are integers, with n positive.
       Equivalent to hash(fractions.Fraction(m, n)).

       """
       P = sys.hash_info.modulus
       # Remove common factors of P.  (Unnecessary if m and n already coprime.)
       while m % P == n % P == 0:
           m, n = m // P, n // P

       if n % P == 0:
           hash_ = sys.hash_info.inf
       else:
           # Fermat's Little Theorem: pow(n, P-1, P) is 1, so
           # pow(n, P-2, P) gives the inverse of n modulo P.
           hash_ = (abs(m) % P) * pow(n, P - 2, P) % P
       if m < 0:
           hash_ = -hash_
       if hash_ == -1:
           hash_ = -2
       return hash_

   def hash_float(x):
       """Compute the hash of a float x."""

       if math.isnan(x):
           return sys.hash_info.nan
       elif math.isinf(x):
           return sys.hash_info.inf if x > 0 else -sys.hash_info.inf
       else:
           return hash_fraction(*x.as_integer_ratio())

   def hash_complex(z):
       """Compute the hash of a complex number z."""

       hash_ = hash_float(z.real) + sys.hash_info.imag * hash_float(z.imag)
       # do a signed reduction modulo 2**sys.hash_info.width
       M = 2**(sys.hash_info.width - 1)
       hash_ = (hash_ & (M - 1)) - (hash & M)
       if hash_ == -1:
           hash_ == -2
       return hash_

.. _typeiter:

Iterator Types
==============

.. index::
   single: iterator protocol
   single: protocol; iterator
   single: sequence; iteration
   single: container; iteration over

Python supports a concept of iteration over containers.  This is implemented
using two distinct methods; these are used to allow user-defined classes to
support iteration.  Sequences, described below in more detail, always support
the iteration methods.

One method needs to be defined for container objects to provide iteration
support:

.. XXX duplicated in reference/datamodel!

.. method:: container.__iter__()

   Return an iterator object.  The object is required to support the iterator
   protocol described below.  If a container supports different types of
   iteration, additional methods can be provided to specifically request
   iterators for those iteration types.  (An example of an object supporting
   multiple forms of iteration would be a tree structure which supports both
   breadth-first and depth-first traversal.)  This method corresponds to the
   :c:member:`~PyTypeObject.tp_iter` slot of the type structure for Python objects in the Python/C
   API.

The iterator objects themselves are required to support the following two
methods, which together form the :dfn:`iterator protocol`:


.. method:: iterator.__iter__()

   Return the iterator object itself.  This is required to allow both containers
   and iterators to be used with the :keyword:`for` and :keyword:`in` statements.
   This method corresponds to the :c:member:`~PyTypeObject.tp_iter` slot of the type structure for
   Python objects in the Python/C API.


.. method:: iterator.__next__()

   Return the next item from the container.  If there are no further items, raise
   the :exc:`StopIteration` exception.  This method corresponds to the
   :c:member:`~PyTypeObject.tp_iternext` slot of the type structure for Python objects in the
   Python/C API.

Python defines several iterator objects to support iteration over general and
specific sequence types, dictionaries, and other more specialized forms.  The
specific types are not important beyond their implementation of the iterator
protocol.

Once an iterator's :meth:`~iterator.__next__` method raises
:exc:`StopIteration`, it must continue to do so on subsequent calls.
Implementations that do not obey this property are deemed broken.


.. _generator-types:

Generator Types
---------------

Python's :term:`generator`\s provide a convenient way to implement the iterator
protocol.  If a container object's :meth:`__iter__` method is implemented as a
generator, it will automatically return an iterator object (technically, a
generator object) supplying the :meth:`__iter__` and :meth:`~generator.__next__`
methods.
More information about generators can be found in :ref:`the documentation for
the yield expression <yieldexpr>`.


.. _typesseq:

Sequence Types --- :class:`list`, :class:`tuple`, :class:`range`
================================================================

There are three basic sequence types: lists, tuples, and range objects.
Additional sequence types tailored for processing of
:ref:`binary data <binaryseq>` and :ref:`text strings <textseq>` are
described in dedicated sections.


.. _typesseq-common:

Common Sequence Operations
--------------------------

.. index:: object: sequence

The operations in the following table are supported by most sequence types,
both mutable and immutable. The :class:`collections.abc.Sequence` ABC is
provided to make it easier to correctly implement these operations on
custom sequence types.

This table lists the sequence operations sorted in ascending priority
(operations in the same box have the same priority).  In the table, *s* and *t*
are sequences of the same type, *n*, *i*, *j* and *k* are integers and *x* is
an arbitrary object that meets any type and value restrictions imposed by *s*.

The ``in`` and ``not in`` operations have the same priorities as the
comparison operations. The ``+`` (concatenation) and ``*`` (repetition)
operations have the same priority as the corresponding numeric operations.

.. index::
   triple: operations on; sequence; types
   builtin: len
   builtin: min
   builtin: max
   pair: concatenation; operation
   pair: repetition; operation
   pair: subscript; operation
   pair: slice; operation
   operator: in
   operator: not in
   single: count() (sequence method)
   single: index() (sequence method)

+--------------------------+--------------------------------+----------+
| Operation                | Result                         | Notes    |
+==========================+================================+==========+
| ``x in s``               | ``True`` if an item of *s* is  | \(1)     |
|                          | equal to *x*, else ``False``   |          |
+--------------------------+--------------------------------+----------+
| ``x not in s``           | ``False`` if an item of *s* is | \(1)     |
|                          | equal to *x*, else ``True``    |          |
+--------------------------+--------------------------------+----------+
| ``s + t``                | the concatenation of *s* and   | (6)(7)   |
|                          | *t*                            |          |
+--------------------------+--------------------------------+----------+
| ``s * n`` or             | *n* shallow copies of *s*      | (2)(7)   |
| ``n * s``                | concatenated                   |          |
+--------------------------+--------------------------------+----------+
| ``s[i]``                 | *i*\ th item of *s*, origin 0  | \(3)     |
+--------------------------+--------------------------------+----------+
| ``s[i:j]``               | slice of *s* from *i* to *j*   | (3)(4)   |
+--------------------------+--------------------------------+----------+
| ``s[i:j:k]``             | slice of *s* from *i* to *j*   | (3)(5)   |
|                          | with step *k*                  |          |
+--------------------------+--------------------------------+----------+
| ``len(s)``               | length of *s*                  |          |
+--------------------------+--------------------------------+----------+
| ``min(s)``               | smallest item of *s*           |          |
+--------------------------+--------------------------------+----------+
| ``max(s)``               | largest item of *s*            |          |
+--------------------------+--------------------------------+----------+
| ``s.index(x[, i[, j]])`` | index of the first occurrence  | \(8)     |
|                          | of *x* in *s* (at or after     |          |
|                          | index *i* and before index *j*)|          |
+--------------------------+--------------------------------+----------+
| ``s.count(x)``           | total number of occurrences of |          |
|                          | *x* in *s*                     |          |
+--------------------------+--------------------------------+----------+

Sequences of the same type also support comparisons.  In particular, tuples
and lists are compared lexicographically by comparing corresponding elements.
This means that to compare equal, every element must compare equal and the
two sequences must be of the same type and have the same length.  (For full
details see :ref:`comparisons` in the language reference.)

Notes:

(1)
   While the ``in`` and ``not in`` operations are used only for simple
   containment testing in the general case, some specialised sequences
   (such as :class:`str`, :class:`bytes` and :class:`bytearray`) also use
   them for subsequence testing::

      >>> "gg" in "eggs"
      True

(2)
   Values of *n* less than ``0`` are treated as ``0`` (which yields an empty
   sequence of the same type as *s*).  Note also that the copies are shallow;
   nested structures are not copied.  This often haunts new Python programmers;
   consider::

      >>> lists = [[]] * 3
      >>> lists
      [[], [], []]
      >>> lists[0].append(3)
      >>> lists
      [[3], [3], [3]]

   What has happened is that ``[[]]`` is a one-element list containing an empty
   list, so all three elements of ``[[]] * 3`` are (pointers to) this single empty
   list.  Modifying any of the elements of ``lists`` modifies this single list.
   You can create a list of different lists this way::

      >>> lists = [[] for i in range(3)]
      >>> lists[0].append(3)
      >>> lists[1].append(5)
      >>> lists[2].append(7)
      >>> lists
      [[3], [5], [7]]

(3)
   If *i* or *j* is negative, the index is relative to the end of the string:
   ``len(s) + i`` or ``len(s) + j`` is substituted.  But note that ``-0`` is
   still ``0``.

(4)
   The slice of *s* from *i* to *j* is defined as the sequence of items with index
   *k* such that ``i <= k < j``.  If *i* or *j* is greater than ``len(s)``, use
   ``len(s)``.  If *i* is omitted or ``None``, use ``0``.  If *j* is omitted or
   ``None``, use ``len(s)``.  If *i* is greater than or equal to *j*, the slice is
   empty.

(5)
   The slice of *s* from *i* to *j* with step *k* is defined as the sequence of
   items with index  ``x = i + n*k`` such that ``0 <= n < (j-i)/k``.  In other words,
   the indices are ``i``, ``i+k``, ``i+2*k``, ``i+3*k`` and so on, stopping when
   *j* is reached (but never including *j*).  If *i* or *j* is greater than
   ``len(s)``, use ``len(s)``.  If *i* or *j* are omitted or ``None``, they become
   "end" values (which end depends on the sign of *k*).  Note, *k* cannot be zero.
   If *k* is ``None``, it is treated like ``1``.

(6)
   Concatenating immutable sequences always results in a new object.  This
   means that building up a sequence by repeated concatenation will have a
   quadratic runtime cost in the total sequence length.  To get a linear
   runtime cost, you must switch to one of the alternatives below:

   * if concatenating :class:`str` objects, you can build a list and use
     :meth:`str.join` at the end or else write to a :class:`io.StringIO`
     instance and retrieve its value when complete

   * if concatenating :class:`bytes` objects, you can similarly use
     :meth:`bytes.join` or :class:`io.BytesIO`, or you can do in-place
     concatenation with a :class:`bytearray` object.  :class:`bytearray`
     objects are mutable and have an efficient overallocation mechanism

   * if concatenating :class:`tuple` objects, extend a :class:`list` instead

   * for other types, investigate the relevant class documentation


(7)
  Some sequence types (such as :class:`range`) only support item sequences
  that follow specific patterns, and hence don't support sequence
  concatenation or repetition.

(8)
   ``index`` raises :exc:`ValueError` when *x* is not found in *s*.
   When supported, the additional arguments to the index method allow
   efficient searching of subsections of the sequence. Passing the extra
   arguments is roughly equivalent to using ``s[i:j].index(x)``, only
   without copying any data and with the returned index being relative to
   the start of the sequence rather than the start of the slice.


.. _typesseq-immutable:

Immutable Sequence Types
------------------------

.. index::
   triple: immutable; sequence; types
   object: tuple
   builtin: hash

The only operation that immutable sequence types generally implement that is
not also implemented by mutable sequence types is support for the :func:`hash`
built-in.

This support allows immutable sequences, such as :class:`tuple` instances, to
be used as :class:`dict` keys and stored in :class:`set` and :class:`frozenset`
instances.

Attempting to hash an immutable sequence that contains unhashable values will
result in :exc:`TypeError`.


.. _typesseq-mutable:

Mutable Sequence Types
----------------------

.. index::
   triple: mutable; sequence; types
   object: list
   object: bytearray

The operations in the following table are defined on mutable sequence types.
The :class:`collections.abc.MutableSequence` ABC is provided to make it
easier to correctly implement these operations on custom sequence types.

In the table *s* is an instance of a mutable sequence type, *t* is any
iterable object and *x* is an arbitrary object that meets any type
and value restrictions imposed by *s* (for example, :class:`bytearray` only
accepts integers that meet the value restriction ``0 <= x <= 255``).


.. index::
   triple: operations on; sequence; types
   triple: operations on; list; type
   pair: subscript; assignment
   pair: slice; assignment
   statement: del
   single: append() (sequence method)
   single: clear() (sequence method)
   single: copy() (sequence method)
   single: extend() (sequence method)
   single: insert() (sequence method)
   single: pop() (sequence method)
   single: remove() (sequence method)
   single: reverse() (sequence method)

+------------------------------+--------------------------------+---------------------+
| Operation                    | Result                         | Notes               |
+==============================+================================+=====================+
| ``s[i] = x``                 | item *i* of *s* is replaced by |                     |
|                              | *x*                            |                     |
+------------------------------+--------------------------------+---------------------+
| ``s[i:j] = t``               | slice of *s* from *i* to *j*   |                     |
|                              | is replaced by the contents of |                     |
|                              | the iterable *t*               |                     |
+------------------------------+--------------------------------+---------------------+
| ``del s[i:j]``               | same as ``s[i:j] = []``        |                     |
+------------------------------+--------------------------------+---------------------+
| ``s[i:j:k] = t``             | the elements of ``s[i:j:k]``   | \(1)                |
|                              | are replaced by those of *t*   |                     |
+------------------------------+--------------------------------+---------------------+
| ``del s[i:j:k]``             | removes the elements of        |                     |
|                              | ``s[i:j:k]`` from the list     |                     |
+------------------------------+--------------------------------+---------------------+
| ``s.append(x)``              | appends *x* to the end of the  |                     |
|                              | sequence (same as              |                     |
|                              | ``s[len(s):len(s)] = [x]``)    |                     |
+------------------------------+--------------------------------+---------------------+
| ``s.clear()``                | removes all items from ``s``   | \(5)                |
|                              | (same as ``del s[:]``)         |                     |
+------------------------------+--------------------------------+---------------------+
| ``s.copy()``                 | creates a shallow copy of ``s``| \(5)                |
|                              | (same as ``s[:]``)             |                     |
+------------------------------+--------------------------------+---------------------+
| ``s.extend(t)``              | extends *s* with the           |                     |
|                              | contents of *t* (same as       |                     |
|                              | ``s[len(s):len(s)] = t``)      |                     |
+------------------------------+--------------------------------+---------------------+
| ``s.insert(i, x)``           | inserts *x* into *s* at the    |                     |
|                              | index given by *i*             |                     |
|                              | (same as ``s[i:i] = [x]``)     |                     |
+------------------------------+--------------------------------+---------------------+
| ``s.pop([i])``               | retrieves the item at *i* and  | \(2)                |
|                              | also removes it from *s*       |                     |
+------------------------------+--------------------------------+---------------------+
| ``s.remove(x)``              | remove the first item from *s* | \(3)                |
|                              | where ``s[i] == x``            |                     |
+------------------------------+--------------------------------+---------------------+
| ``s.reverse()``              | reverses the items of *s* in   | \(4)                |
|                              | place                          |                     |
+------------------------------+--------------------------------+---------------------+


Notes:

(1)
   *t* must have the same length as the slice it is replacing.

(2)
   The optional argument *i* defaults to ``-1``, so that by default the last
   item is removed and returned.

(3)
   ``remove`` raises :exc:`ValueError` when *x* is not found in *s*.

(4)
   The :meth:`reverse` method modifies the sequence in place for economy of
   space when reversing a large sequence.  To remind users that it operates by
   side effect, it does not return the reversed sequence.

(5)
   :meth:`clear` and :meth:`!copy` are included for consistency with the
   interfaces of mutable containers that don't support slicing operations
   (such as :class:`dict` and :class:`set`)

   .. versionadded:: 3.3
      :meth:`clear` and :meth:`!copy` methods.


.. _typesseq-list:

Lists
-----

.. index:: object: list

Lists are mutable sequences, typically used to store collections of
homogeneous items (where the precise degree of similarity will vary by
application).

.. class:: list([iterable])

   Lists may be constructed in several ways:

   * Using a pair of square brackets to denote the empty list: ``[]``
   * Using square brackets, separating items with commas: ``[a]``, ``[a, b, c]``
   * Using a list comprehension: ``[x for x in iterable]``
   * Using the type constructor: ``list()`` or ``list(iterable)``

   The constructor builds a list whose items are the same and in the same
   order as *iterable*'s items.  *iterable* may be either a sequence, a
   container that supports iteration, or an iterator object.  If *iterable*
   is already a list, a copy is made and returned, similar to ``iterable[:]``.
   For example, ``list('abc')`` returns ``['a', 'b', 'c']`` and
   ``list( (1, 2, 3) )`` returns ``[1, 2, 3]``.
   If no argument is given, the constructor creates a new empty list, ``[]``.


   Many other operations also produce lists, including the :func:`sorted`
   built-in.

   Lists implement all of the :ref:`common <typesseq-common>` and
   :ref:`mutable <typesseq-mutable>` sequence operations. Lists also provide the
   following additional method:

   .. method:: list.sort(*, key=None, reverse=None)

      This method sorts the list in place, using only ``<`` comparisons
      between items. Exceptions are not suppressed - if any comparison operations
      fail, the entire sort operation will fail (and the list will likely be left
      in a partially modified state).

      :meth:`sort` accepts two arguments that can only be passed by keyword
      (:ref:`keyword-only arguments <keyword-only_parameter>`):

      *key* specifies a function of one argument that is used to extract a
      comparison key from each list element (for example, ``key=str.lower``).
      The key corresponding to each item in the list is calculated once and
      then used for the entire sorting process. The default value of ``None``
      means that list items are sorted directly without calculating a separate
      key value.

      The :func:`functools.cmp_to_key` utility is available to convert a 2.x
      style *cmp* function to a *key* function.

      *reverse* is a boolean value.  If set to ``True``, then the list elements
      are sorted as if each comparison were reversed.

      This method modifies the sequence in place for economy of space when
      sorting a large sequence.  To remind users that it operates by side
      effect, it does not return the sorted sequence (use :func:`sorted` to
      explicitly request a new sorted list instance).

      The :meth:`sort` method is guaranteed to be stable.  A sort is stable if it
      guarantees not to change the relative order of elements that compare equal
      --- this is helpful for sorting in multiple passes (for example, sort by
      department, then by salary grade).

      .. impl-detail::

         While a list is being sorted, the effect of attempting to mutate, or even
         inspect, the list is undefined.  The C implementation of Python makes the
         list appear empty for the duration, and raises :exc:`ValueError` if it can
         detect that the list has been mutated during a sort.


.. _typesseq-tuple:

Tuples
------

.. index:: object: tuple

Tuples are immutable sequences, typically used to store collections of
heterogeneous data (such as the 2-tuples produced by the :func:`enumerate`
built-in). Tuples are also used for cases where an immutable sequence of
homogeneous data is needed (such as allowing storage in a :class:`set` or
:class:`dict` instance).

.. class:: tuple([iterable])

   Tuples may be constructed in a number of ways:

   * Using a pair of parentheses to denote the empty tuple: ``()``
   * Using a trailing comma for a singleton tuple: ``a,`` or ``(a,)``
   * Separating items with commas: ``a, b, c`` or ``(a, b, c)``
   * Using the :func:`tuple` built-in: ``tuple()`` or ``tuple(iterable)``

   The constructor builds a tuple whose items are the same and in the same
   order as *iterable*'s items.  *iterable* may be either a sequence, a
   container that supports iteration, or an iterator object.  If *iterable*
   is already a tuple, it is returned unchanged. For example,
   ``tuple('abc')`` returns ``('a', 'b', 'c')`` and
   ``tuple( [1, 2, 3] )`` returns ``(1, 2, 3)``.
   If no argument is given, the constructor creates a new empty tuple, ``()``.

   Note that it is actually the comma which makes a tuple, not the parentheses.
   The parentheses are optional, except in the empty tuple case, or
   when they are needed to avoid syntactic ambiguity. For example,
   ``f(a, b, c)`` is a function call with three arguments, while
   ``f((a, b, c))`` is a function call with a 3-tuple as the sole argument.

   Tuples implement all of the :ref:`common <typesseq-common>` sequence
   operations.

For heterogeneous collections of data where access by name is clearer than
access by index, :func:`collections.namedtuple` may be a more appropriate
choice than a simple tuple object.


.. _typesseq-range:

Ranges
------

.. index:: object: range

The :class:`range` type represents an immutable sequence of numbers and is
commonly used for looping a specific number of times in :keyword:`for`
loops.

.. class:: range(stop)
           range(start, stop[, step])

   The arguments to the range constructor must be integers (either built-in
   :class:`int` or any object that implements the ``__index__`` special
   method).  If the *step* argument is omitted, it defaults to ``1``.
   If the *start* argument is omitted, it defaults to ``0``.
   If *step* is zero, :exc:`ValueError` is raised.

   For a positive *step*, the contents of a range ``r`` are determined by the
   formula ``r[i] = start + step*i`` where ``i >= 0`` and
   ``r[i] < stop``.

   For a negative *step*, the contents of the range are still determined by
   the formula ``r[i] = start + step*i``, but the constraints are ``i >= 0``
   and ``r[i] > stop``.

   A range object will be empty if ``r[0]`` does not meet the value
   constraint. Ranges do support negative indices, but these are interpreted
   as indexing from the end of the sequence determined by the positive
   indices.

   Ranges containing absolute values larger than :data:`sys.maxsize` are
   permitted but some features (such as :func:`len`) may raise
   :exc:`OverflowError`.

   Range examples::

      >>> list(range(10))
      [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
      >>> list(range(1, 11))
      [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
      >>> list(range(0, 30, 5))
      [0, 5, 10, 15, 20, 25]
      >>> list(range(0, 10, 3))
      [0, 3, 6, 9]
      >>> list(range(0, -10, -1))
      [0, -1, -2, -3, -4, -5, -6, -7, -8, -9]
      >>> list(range(0))
      []
      >>> list(range(1, 0))
      []

   Ranges implement all of the :ref:`common <typesseq-common>` sequence operations
   except concatenation and repetition (due to the fact that range objects can
   only represent sequences that follow a strict pattern and repetition and
   concatenation will usually violate that pattern).

   .. data: start

      The value of the *start* parameter (or ``0`` if the parameter was
      not supplied)

   .. data: stop

      The value of the *stop* parameter

   .. data: step

      The value of the *step* parameter (or ``1`` if the parameter was
      not supplied)

The advantage of the :class:`range` type over a regular :class:`list` or
:class:`tuple` is that a :class:`range` object will always take the same
(small) amount of memory, no matter the size of the range it represents (as it
only stores the ``start``, ``stop`` and ``step`` values, calculating individual
items and subranges as needed).

Range objects implement the :class:`collections.abc.Sequence` ABC, and provide
features such as containment tests, element index lookup, slicing and
support for negative indices (see :ref:`typesseq`):

   >>> r = range(0, 20, 2)
   >>> r
   range(0, 20, 2)
   >>> 11 in r
   False
   >>> 10 in r
   True
   >>> r.index(10)
   5
   >>> r[5]
   10
   >>> r[:5]
   range(0, 10, 2)
   >>> r[-1]
   18

Testing range objects for equality with ``==`` and ``!=`` compares
them as sequences.  That is, two range objects are considered equal if
they represent the same sequence of values.  (Note that two range
objects that compare equal might have different :attr:`~range.start`,
:attr:`~range.stop` and :attr:`~range.step` attributes, for example
``range(0) == range(2, 1, 3)`` or ``range(0, 3, 2) == range(0, 4, 2)``.)

.. versionchanged:: 3.2
   Implement the Sequence ABC.
   Support slicing and negative indices.
   Test :class:`int` objects for membership in constant time instead of
   iterating through all items.

.. versionchanged:: 3.3
   Define '==' and '!=' to compare range objects based on the
   sequence of values they define (instead of comparing based on
   object identity).

.. versionadded:: 3.3
   The :attr:`~range.start`, :attr:`~range.stop` and :attr:`~range.step`
   attributes.


.. index::
   single: string; text sequence type
   single: str (built-in class); (see also string)
   object: string

.. _textseq:

Text Sequence Type --- :class:`str`
===================================

Textual data in Python is handled with :class:`str` objects, or :dfn:`strings`.
Strings are immutable
:ref:`sequences <typesseq>` of Unicode code points.  String literals are
written in a variety of ways:

* Single quotes: ``'allows embedded "double" quotes'``
* Double quotes: ``"allows embedded 'single' quotes"``.
* Triple quoted: ``'''Three single quotes'''``, ``"""Three double quotes"""``

Triple quoted strings may span multiple lines - all associated whitespace will
be included in the string literal.

String literals that are part of a single expression and have only whitespace
between them will be implicitly converted to a single string literal. That
is, ``("spam " "eggs") == "spam eggs"``.

See :ref:`strings` for more about the various forms of string literal,
including supported escape sequences, and the ``r`` ("raw") prefix that
disables most escape sequence processing.

Strings may also be created from other objects using the :class:`str`
constructor.

Since there is no separate "character" type, indexing a string produces
strings of length 1. That is, for a non-empty string *s*, ``s[0] == s[0:1]``.

.. index::
   object: io.StringIO

There is also no mutable string type, but :meth:`str.join` or
:class:`io.StringIO` can be used to efficiently construct strings from
multiple fragments.

.. versionchanged:: 3.3
   For backwards compatibility with the Python 2 series, the ``u`` prefix is
   once again permitted on string literals. It has no effect on the meaning
   of string literals and cannot be combined with the ``r`` prefix.


.. index::
   single: string; str (built-in class)

.. class:: str(object='')
           str(object=b'', encoding='utf-8', errors='strict')

   Return a :ref:`string <textseq>` version of *object*.  If *object* is not
   provided, returns the empty string.  Otherwise, the behavior of ``str()``
   depends on whether *encoding* or *errors* is given, as follows.

   If neither *encoding* nor *errors* is given, ``str(object)`` returns
   :meth:`object.__str__() <object.__str__>`, which is the "informal" or nicely
   printable string representation of *object*.  For string objects, this is
   the string itself.  If *object* does not have a :meth:`~object.__str__`
   method, then :func:`str` falls back to returning
   :meth:`repr(object) <repr>`.

   .. index::
      single: buffer protocol; str (built-in class)
      single: bytes; str (built-in class)

   If at least one of *encoding* or *errors* is given, *object* should be a
   :term:`bytes-like object` (e.g. :class:`bytes` or :class:`bytearray`).  In
   this case, if *object* is a :class:`bytes` (or :class:`bytearray`) object,
   then ``str(bytes, encoding, errors)`` is equivalent to
   :meth:`bytes.decode(encoding, errors) <bytes.decode>`.  Otherwise, the bytes
   object underlying the buffer object is obtained before calling
   :meth:`bytes.decode`.  See :ref:`binaryseq` and
   :ref:`bufferobjects` for information on buffer objects.

   Passing a :class:`bytes` object to :func:`str` without the *encoding*
   or *errors* arguments falls under the first case of returning the informal
   string representation (see also the :option:`-b` command-line option to
   Python).  For example::

      >>> str(b'Zoot!')
      "b'Zoot!'"

   For more information on the ``str`` class and its methods, see
   :ref:`textseq` and the :ref:`string-methods` section below.  To output
   formatted strings, see the :ref:`string-formatting` section.  In addition,
   see the :ref:`stringservices` section.


.. index::
   pair: string; methods

.. _string-methods:

String Methods
--------------

.. index::
   module: re

Strings implement all of the :ref:`common <typesseq-common>` sequence
operations, along with the additional methods described below.

Strings also support two styles of string formatting, one providing a large
degree of flexibility and customization (see :meth:`str.format`,
:ref:`formatstrings` and :ref:`string-formatting`) and the other based on C
``printf`` style formatting that handles a narrower range of types and is
slightly harder to use correctly, but is often faster for the cases it can
handle (:ref:`old-string-formatting`).

The :ref:`textservices` section of the standard library covers a number of
other modules that provide various text related utilities (including regular
expression support in the :mod:`re` module).

.. method:: str.capitalize()

   Return a copy of the string with its first character capitalized and the
   rest lowercased.


.. method:: str.casefold()

   Return a casefolded copy of the string. Casefolded strings may be used for
   caseless matching.

   Casefolding is similar to lowercasing but more aggressive because it is
   intended to remove all case distinctions in a string. For example, the German
   lowercase letter ``'ß'`` is equivalent to ``"ss"``. Since it is already
   lowercase, :meth:`lower` would do nothing to ``'ß'``; :meth:`casefold`
   converts it to ``"ss"``.

   The casefolding algorithm is described in section 3.13 of the Unicode
   Standard.

   .. versionadded:: 3.3


.. method:: str.center(width[, fillchar])

   Return centered in a string of length *width*. Padding is done using the
   specified *fillchar* (default is a space).


.. method:: str.count(sub[, start[, end]])

   Return the number of non-overlapping occurrences of substring *sub* in the
   range [*start*, *end*].  Optional arguments *start* and *end* are
   interpreted as in slice notation.


.. method:: str.encode(encoding="utf-8", errors="strict")

   Return an encoded version of the string as a bytes object. Default encoding
   is ``'utf-8'``. *errors* may be given to set a different error handling scheme.
   The default for *errors* is ``'strict'``, meaning that encoding errors raise
   a :exc:`UnicodeError`. Other possible
   values are ``'ignore'``, ``'replace'``, ``'xmlcharrefreplace'``,
   ``'backslashreplace'`` and any other name registered via
   :func:`codecs.register_error`, see section :ref:`codec-base-classes`. For a
   list of possible encodings, see section :ref:`standard-encodings`.

   .. versionchanged:: 3.1
      Support for keyword arguments added.


.. method:: str.endswith(suffix[, start[, end]])

   Return ``True`` if the string ends with the specified *suffix*, otherwise return
   ``False``.  *suffix* can also be a tuple of suffixes to look for.  With optional
   *start*, test beginning at that position.  With optional *end*, stop comparing
   at that position.


.. method:: str.expandtabs(tabsize=8)

   Return a copy of the string where all tab characters are replaced by one or
   more spaces, depending on the current column and the given tab size.  Tab
   positions occur every *tabsize* characters (default is 8, giving tab
   positions at columns 0, 8, 16 and so on).  To expand the string, the current
   column is set to zero and the string is examined character by character.  If
   the character is a tab (``\t``), one or more space characters are inserted
   in the result until the current column is equal to the next tab position.
   (The tab character itself is not copied.)  If the character is a newline
   (``\n``) or return (``\r``), it is copied and the current column is reset to
   zero.  Any other character is copied unchanged and the current column is
   incremented by one regardless of how the character is represented when
   printed.

      >>> '01\t012\t0123\t01234'.expandtabs()
      '01      012     0123    01234'
      >>> '01\t012\t0123\t01234'.expandtabs(4)
      '01  012 0123    01234'


.. method:: str.find(sub[, start[, end]])

   Return the lowest index in the string where substring *sub* is found, such
   that *sub* is contained in the slice ``s[start:end]``.  Optional arguments
   *start* and *end* are interpreted as in slice notation.  Return ``-1`` if
   *sub* is not found.

   .. note::

      The :meth:`~str.find` method should be used only if you need to know the
      position of *sub*.  To check if *sub* is a substring or not, use the
      :keyword:`in` operator::

         >>> 'Py' in 'Python'
         True


.. method:: str.format(*args, **kwargs)

   Perform a string formatting operation.  The string on which this method is
   called can contain literal text or replacement fields delimited by braces
   ``{}``.  Each replacement field contains either the numeric index of a
   positional argument, or the name of a keyword argument.  Returns a copy of
   the string where each replacement field is replaced with the string value of
   the corresponding argument.

      >>> "The sum of 1 + 2 is {0}".format(1+2)
      'The sum of 1 + 2 is 3'

   See :ref:`formatstrings` for a description of the various formatting options
   that can be specified in format strings.


.. method:: str.format_map(mapping)

   Similar to ``str.format(**mapping)``, except that ``mapping`` is
   used directly and not copied to a :class:`dict`.  This is useful
   if for example ``mapping`` is a dict subclass:

   >>> class Default(dict):
   ...     def __missing__(self, key):
   ...         return key
   ...
   >>> '{name} was born in {country}'.format_map(Default(name='Guido'))
   'Guido was born in country'

   .. versionadded:: 3.2


.. method:: str.index(sub[, start[, end]])

   Like :meth:`find`, but raise :exc:`ValueError` when the substring is not found.


.. method:: str.isalnum()

   Return true if all characters in the string are alphanumeric and there is at
   least one character, false otherwise.  A character ``c`` is alphanumeric if one
   of the following returns ``True``: ``c.isalpha()``, ``c.isdecimal()``,
   ``c.isdigit()``, or ``c.isnumeric()``.


.. method:: str.isalpha()

   Return true if all characters in the string are alphabetic and there is at least
   one character, false otherwise.  Alphabetic characters are those characters defined
   in the Unicode character database as "Letter", i.e., those with general category
   property being one of "Lm", "Lt", "Lu", "Ll", or "Lo".  Note that this is different
   from the "Alphabetic" property defined in the Unicode Standard.


.. method:: str.isdecimal()

   Return true if all characters in the string are decimal
   characters and there is at least one character, false
   otherwise. Decimal characters are those from general category "Nd". This category
   includes digit characters, and all characters
   that can be used to form decimal-radix numbers, e.g. U+0660,
   ARABIC-INDIC DIGIT ZERO.


.. method:: str.isdigit()

   Return true if all characters in the string are digits and there is at least one
   character, false otherwise.  Digits include decimal characters and digits that need
   special handling, such as the compatibility superscript digits.  Formally, a digit
   is a character that has the property value Numeric_Type=Digit or Numeric_Type=Decimal.


.. method:: str.isidentifier()

   Return true if the string is a valid identifier according to the language
   definition, section :ref:`identifiers`.

   Use :func:`keyword.iskeyword` to test for reserved identifiers such as
   :keyword:`def` and :keyword:`class`.

.. method:: str.islower()

   Return true if all cased characters [4]_ in the string are lowercase and
   there is at least one cased character, false otherwise.


.. method:: str.isnumeric()

   Return true if all characters in the string are numeric
   characters, and there is at least one character, false
   otherwise. Numeric characters include digit characters, and all characters
   that have the Unicode numeric value property, e.g. U+2155,
   VULGAR FRACTION ONE FIFTH.  Formally, numeric characters are those with the property
   value Numeric_Type=Digit, Numeric_Type=Decimal or Numeric_Type=Numeric.


.. method:: str.isprintable()

   Return true if all characters in the string are printable or the string is
   empty, false otherwise.  Nonprintable characters are those characters defined
   in the Unicode character database as "Other" or "Separator", excepting the
   ASCII space (0x20) which is considered printable.  (Note that printable
   characters in this context are those which should not be escaped when
   :func:`repr` is invoked on a string.  It has no bearing on the handling of
   strings written to :data:`sys.stdout` or :data:`sys.stderr`.)


.. method:: str.isspace()

   Return true if there are only whitespace characters in the string and there is
   at least one character, false otherwise.  Whitespace characters  are those
   characters defined in the Unicode character database as "Other" or "Separator"
   and those with bidirectional property being one of "WS", "B", or "S".

.. method:: str.istitle()

   Return true if the string is a titlecased string and there is at least one
   character, for example uppercase characters may only follow uncased characters
   and lowercase characters only cased ones.  Return false otherwise.


.. method:: str.isupper()

   Return true if all cased characters [4]_ in the string are uppercase and
   there is at least one cased character, false otherwise.


.. method:: str.join(iterable)

   Return a string which is the concatenation of the strings in the
   :term:`iterable` *iterable*.  A :exc:`TypeError` will be raised if there are
   any non-string values in *iterable*, including :class:`bytes` objects.  The
   separator between elements is the string providing this method.


.. method:: str.ljust(width[, fillchar])

   Return the string left justified in a string of length *width*. Padding is done
   using the specified *fillchar* (default is a space).  The original string is
   returned if *width* is less than or equal to ``len(s)``.


.. method:: str.lower()

   Return a copy of the string with all the cased characters [4]_ converted to
   lowercase.

   The lowercasing algorithm used is described in section 3.13 of the Unicode
   Standard.


.. method:: str.lstrip([chars])

   Return a copy of the string with leading characters removed.  The *chars*
   argument is a string specifying the set of characters to be removed.  If omitted
   or ``None``, the *chars* argument defaults to removing whitespace.  The *chars*
   argument is not a prefix; rather, all combinations of its values are stripped:

      >>> '   spacious   '.lstrip()
      'spacious   '
      >>> 'www.example.com'.lstrip('cmowz.')
      'example.com'


.. staticmethod:: str.maketrans(x[, y[, z]])

   This static method returns a translation table usable for :meth:`str.translate`.

   If there is only one argument, it must be a dictionary mapping Unicode
   ordinals (integers) or characters (strings of length 1) to Unicode ordinals,
   strings (of arbitrary lengths) or None.  Character keys will then be
   converted to ordinals.

   If there are two arguments, they must be strings of equal length, and in the
   resulting dictionary, each character in x will be mapped to the character at
   the same position in y.  If there is a third argument, it must be a string,
   whose characters will be mapped to None in the result.


.. method:: str.partition(sep)

   Split the string at the first occurrence of *sep*, and return a 3-tuple
   containing the part before the separator, the separator itself, and the part
   after the separator.  If the separator is not found, return a 3-tuple containing
   the string itself, followed by two empty strings.


.. method:: str.replace(old, new[, count])

   Return a copy of the string with all occurrences of substring *old* replaced by
   *new*.  If the optional argument *count* is given, only the first *count*
   occurrences are replaced.


.. method:: str.rfind(sub[, start[, end]])

   Return the highest index in the string where substring *sub* is found, such
   that *sub* is contained within ``s[start:end]``.  Optional arguments *start*
   and *end* are interpreted as in slice notation.  Return ``-1`` on failure.


.. method:: str.rindex(sub[, start[, end]])

   Like :meth:`rfind` but raises :exc:`ValueError` when the substring *sub* is not
   found.


.. method:: str.rjust(width[, fillchar])

   Return the string right justified in a string of length *width*. Padding is done
   using the specified *fillchar* (default is a space). The original string is
   returned if *width* is less than or equal to ``len(s)``.


.. method:: str.rpartition(sep)

   Split the string at the last occurrence of *sep*, and return a 3-tuple
   containing the part before the separator, the separator itself, and the part
   after the separator.  If the separator is not found, return a 3-tuple containing
   two empty strings, followed by the string itself.


.. method:: str.rsplit(sep=None, maxsplit=-1)

   Return a list of the words in the string, using *sep* as the delimiter string.
   If *maxsplit* is given, at most *maxsplit* splits are done, the *rightmost*
   ones.  If *sep* is not specified or ``None``, any whitespace string is a
   separator.  Except for splitting from the right, :meth:`rsplit` behaves like
   :meth:`split` which is described in detail below.


.. method:: str.rstrip([chars])

   Return a copy of the string with trailing characters removed.  The *chars*
   argument is a string specifying the set of characters to be removed.  If omitted
   or ``None``, the *chars* argument defaults to removing whitespace.  The *chars*
   argument is not a suffix; rather, all combinations of its values are stripped:

      >>> '   spacious   '.rstrip()
      '   spacious'
      >>> 'mississippi'.rstrip('ipz')
      'mississ'


.. method:: str.split(sep=None, maxsplit=-1)

   Return a list of the words in the string, using *sep* as the delimiter
   string.  If *maxsplit* is given, at most *maxsplit* splits are done (thus,
   the list will have at most ``maxsplit+1`` elements).  If *maxsplit* is not
   specified or ``-1``, then there is no limit on the number of splits
   (all possible splits are made).

   If *sep* is given, consecutive delimiters are not grouped together and are
   deemed to delimit empty strings (for example, ``'1,,2'.split(',')`` returns
   ``['1', '', '2']``).  The *sep* argument may consist of multiple characters
   (for example, ``'1<>2<>3'.split('<>')`` returns ``['1', '2', '3']``).
   Splitting an empty string with a specified separator returns ``['']``.

   If *sep* is not specified or is ``None``, a different splitting algorithm is
   applied: runs of consecutive whitespace are regarded as a single separator,
   and the result will contain no empty strings at the start or end if the
   string has leading or trailing whitespace.  Consequently, splitting an empty
   string or a string consisting of just whitespace with a ``None`` separator
   returns ``[]``.

   For example, ``' 1  2   3  '.split()`` returns ``['1', '2', '3']``, and
   ``'  1  2   3  '.split(None, 1)`` returns ``['1', '2   3  ']``.


.. index::
   single: universal newlines; str.splitlines method

.. method:: str.splitlines([keepends])

   Return a list of the lines in the string, breaking at line boundaries.
   This method uses the :term:`universal newlines` approach to splitting lines.
   Line breaks are not included in the resulting list unless *keepends* is
   given and true.

   For example, ``'ab c\n\nde fg\rkl\r\n'.splitlines()`` returns
   ``['ab c', '', 'de fg', 'kl']``, while the same call with ``splitlines(True)``
   returns ``['ab c\n', '\n', 'de fg\r', 'kl\r\n']``.

   Unlike :meth:`~str.split` when a delimiter string *sep* is given, this
   method returns an empty list for the empty string, and a terminal line
   break does not result in an extra line.


.. method:: str.startswith(prefix[, start[, end]])

   Return ``True`` if string starts with the *prefix*, otherwise return ``False``.
   *prefix* can also be a tuple of prefixes to look for.  With optional *start*,
   test string beginning at that position.  With optional *end*, stop comparing
   string at that position.


.. method:: str.strip([chars])

   Return a copy of the string with the leading and trailing characters removed.
   The *chars* argument is a string specifying the set of characters to be removed.
   If omitted or ``None``, the *chars* argument defaults to removing whitespace.
   The *chars* argument is not a prefix or suffix; rather, all combinations of its
   values are stripped:

      >>> '   spacious   '.strip()
      'spacious'
      >>> 'www.example.com'.strip('cmowz.')
      'example'


.. method:: str.swapcase()

   Return a copy of the string with uppercase characters converted to lowercase and
   vice versa. Note that it is not necessarily true that
   ``s.swapcase().swapcase() == s``.


.. method:: str.title()

   Return a titlecased version of the string where words start with an uppercase
   character and the remaining characters are lowercase.

   The algorithm uses a simple language-independent definition of a word as
   groups of consecutive letters.  The definition works in many contexts but
   it means that apostrophes in contractions and possessives form word
   boundaries, which may not be the desired result::

        >>> "they're bill's friends from the UK".title()
        "They'Re Bill'S Friends From The Uk"

   A workaround for apostrophes can be constructed using regular expressions::

        >>> import re
        >>> def titlecase(s):
        ...     return re.sub(r"[A-Za-z]+('[A-Za-z]+)?",
        ...                   lambda mo: mo.group(0)[0].upper() +
        ...                              mo.group(0)[1:].lower(),
        ...                   s)
        ...
        >>> titlecase("they're bill's friends.")
        "They're Bill's Friends."


.. method:: str.translate(map)

   Return a copy of the *s* where all characters have been mapped through the
   *map* which must be a dictionary of Unicode ordinals (integers) to Unicode
   ordinals, strings or ``None``.  Unmapped characters are left untouched.
   Characters mapped to ``None`` are deleted.

   You can use :meth:`str.maketrans` to create a translation map from
   character-to-character mappings in different formats.

   .. note::

      An even more flexible approach is to create a custom character mapping
      codec using the :mod:`codecs` module (see :mod:`encodings.cp1251` for an
      example).


.. method:: str.upper()

   Return a copy of the string with all the cased characters [4]_ converted to
   uppercase.  Note that ``str.upper().isupper()`` might be ``False`` if ``s``
   contains uncased characters or if the Unicode category of the resulting
   character(s) is not "Lu" (Letter, uppercase), but e.g. "Lt" (Letter,
   titlecase).

   The uppercasing algorithm used is described in section 3.13 of the Unicode
   Standard.


.. method:: str.zfill(width)

   Return the numeric string left filled with zeros in a string of length
   *width*.  A sign prefix is handled correctly.  The original string is
   returned if *width* is less than or equal to ``len(s)``.



.. _old-string-formatting:

``printf``-style String Formatting
----------------------------------

.. index::
   single: formatting, string (%)
   single: interpolation, string (%)
   single: string; formatting
   single: string; interpolation
   single: printf-style formatting
   single: sprintf-style formatting
   single: % formatting
   single: % interpolation

.. note::

   The formatting operations described here exhibit a variety of quirks that
   lead to a number of common errors (such as failing to display tuples and
   dictionaries correctly).  Using the newer :meth:`str.format` interface
   helps avoid these errors, and also provides a generally more powerful,
   flexible and extensible approach to formatting text.

String objects have one unique built-in operation: the ``%`` operator (modulo).
This is also known as the string *formatting* or *interpolation* operator.
Given ``format % values`` (where *format* is a string), ``%`` conversion
specifications in *format* are replaced with zero or more elements of *values*.
The effect is similar to using the :c:func:`sprintf` in the C language.

If *format* requires a single argument, *values* may be a single non-tuple
object. [5]_  Otherwise, *values* must be a tuple with exactly the number of
items specified by the format string, or a single mapping object (for example, a
dictionary).

A conversion specifier contains two or more characters and has the following
components, which must occur in this order:

#. The ``'%'`` character, which marks the start of the specifier.

#. Mapping key (optional), consisting of a parenthesised sequence of characters
   (for example, ``(somename)``).

#. Conversion flags (optional), which affect the result of some conversion
   types.

#. Minimum field width (optional).  If specified as an ``'*'`` (asterisk), the
   actual width is read from the next element of the tuple in *values*, and the
   object to convert comes after the minimum field width and optional precision.

#. Precision (optional), given as a ``'.'`` (dot) followed by the precision.  If
   specified as ``'*'`` (an asterisk), the actual precision is read from the next
   element of the tuple in *values*, and the value to convert comes after the
   precision.

#. Length modifier (optional).

#. Conversion type.

When the right argument is a dictionary (or other mapping type), then the
formats in the string *must* include a parenthesised mapping key into that
dictionary inserted immediately after the ``'%'`` character. The mapping key
selects the value to be formatted from the mapping.  For example:

   >>> print('%(language)s has %(number)03d quote types.' %
   ...       {'language': "Python", "number": 2})
   Python has 002 quote types.

In this case no ``*`` specifiers may occur in a format (since they require a
sequential parameter list).

The conversion flag characters are:

+---------+---------------------------------------------------------------------+
| Flag    | Meaning                                                             |
+=========+=====================================================================+
| ``'#'`` | The value conversion will use the "alternate form" (where defined   |
|         | below).                                                             |
+---------+---------------------------------------------------------------------+
| ``'0'`` | The conversion will be zero padded for numeric values.              |
+---------+---------------------------------------------------------------------+
| ``'-'`` | The converted value is left adjusted (overrides the ``'0'``         |
|         | conversion if both are given).                                      |
+---------+---------------------------------------------------------------------+
| ``' '`` | (a space) A blank should be left before a positive number (or empty |
|         | string) produced by a signed conversion.                            |
+---------+---------------------------------------------------------------------+
| ``'+'`` | A sign character (``'+'`` or ``'-'``) will precede the conversion   |
|         | (overrides a "space" flag).                                         |
+---------+---------------------------------------------------------------------+

A length modifier (``h``, ``l``, or ``L``) may be present, but is ignored as it
is not necessary for Python -- so e.g. ``%ld`` is identical to ``%d``.

The conversion types are:

+------------+-----------------------------------------------------+-------+
| Conversion | Meaning                                             | Notes |
+============+=====================================================+=======+
| ``'d'``    | Signed integer decimal.                             |       |
+------------+-----------------------------------------------------+-------+
| ``'i'``    | Signed integer decimal.                             |       |
+------------+-----------------------------------------------------+-------+
| ``'o'``    | Signed octal value.                                 | \(1)  |
+------------+-----------------------------------------------------+-------+
| ``'u'``    | Obsolete type -- it is identical to ``'d'``.        | \(7)  |
+------------+-----------------------------------------------------+-------+
| ``'x'``    | Signed hexadecimal (lowercase).                     | \(2)  |
+------------+-----------------------------------------------------+-------+
| ``'X'``    | Signed hexadecimal (uppercase).                     | \(2)  |
+------------+-----------------------------------------------------+-------+
| ``'e'``    | Floating point exponential format (lowercase).      | \(3)  |
+------------+-----------------------------------------------------+-------+
| ``'E'``    | Floating point exponential format (uppercase).      | \(3)  |
+------------+-----------------------------------------------------+-------+
| ``'f'``    | Floating point decimal format.                      | \(3)  |
+------------+-----------------------------------------------------+-------+
| ``'F'``    | Floating point decimal format.                      | \(3)  |
+------------+-----------------------------------------------------+-------+
| ``'g'``    | Floating point format. Uses lowercase exponential   | \(4)  |
|            | format if exponent is less than -4 or not less than |       |
|            | precision, decimal format otherwise.                |       |
+------------+-----------------------------------------------------+-------+
| ``'G'``    | Floating point format. Uses uppercase exponential   | \(4)  |
|            | format if exponent is less than -4 or not less than |       |
|            | precision, decimal format otherwise.                |       |
+------------+-----------------------------------------------------+-------+
| ``'c'``    | Single character (accepts integer or single         |       |
|            | character string).                                  |       |
+------------+-----------------------------------------------------+-------+
| ``'r'``    | String (converts any Python object using            | \(5)  |
|            | :func:`repr`).                                      |       |
+------------+-----------------------------------------------------+-------+
| ``'s'``    | String (converts any Python object using            | \(5)  |
|            | :func:`str`).                                       |       |
+------------+-----------------------------------------------------+-------+
| ``'a'``    | String (converts any Python object using            | \(5)  |
|            | :func:`ascii`).                                     |       |
+------------+-----------------------------------------------------+-------+
| ``'%'``    | No argument is converted, results in a ``'%'``      |       |
|            | character in the result.                            |       |
+------------+-----------------------------------------------------+-------+

Notes:

(1)
   The alternate form causes a leading zero (``'0'``) to be inserted between
   left-hand padding and the formatting of the number if the leading character
   of the result is not already a zero.

(2)
   The alternate form causes a leading ``'0x'`` or ``'0X'`` (depending on whether
   the ``'x'`` or ``'X'`` format was used) to be inserted between left-hand padding
   and the formatting of the number if the leading character of the result is not
   already a zero.

(3)
   The alternate form causes the result to always contain a decimal point, even if
   no digits follow it.

   The precision determines the number of digits after the decimal point and
   defaults to 6.

(4)
   The alternate form causes the result to always contain a decimal point, and
   trailing zeroes are not removed as they would otherwise be.

   The precision determines the number of significant digits before and after the
   decimal point and defaults to 6.

(5)
   If precision is ``N``, the output is truncated to ``N`` characters.


(7)
   See :pep:`237`.

Since Python strings have an explicit length, ``%s`` conversions do not assume
that ``'\0'`` is the end of the string.

.. XXX Examples?

.. versionchanged:: 3.1
   ``%f`` conversions for numbers whose absolute value is over 1e50 are no
   longer replaced by ``%g`` conversions.


.. index::
   single: buffer protocol; binary sequence types

.. _binaryseq:

Binary Sequence Types --- :class:`bytes`, :class:`bytearray`, :class:`memoryview`
=================================================================================

.. index::
   object: bytes
   object: bytearray
   object: memoryview
   module: array

The core built-in types for manipulating binary data are :class:`bytes` and
:class:`bytearray`. They are supported by :class:`memoryview` which uses
the :ref:`buffer protocol <bufferobjects>` to access the memory of other
binary objects without needing to make a copy.

The :mod:`array` module supports efficient storage of basic data types like
32-bit integers and IEEE754 double-precision floating values.

.. _typebytes:

Bytes
-----

.. index:: object: bytes

Bytes objects are immutable sequences of single bytes. Since many major
binary protocols are based on the ASCII text encoding, bytes objects offer
several methods that are only valid when working with ASCII compatible
data and are closely related to string objects in a variety of other ways.

Firstly, the syntax for bytes literals is largely the same as that for string
literals, except that a ``b`` prefix is added:

* Single quotes: ``b'still allows embedded "double" quotes'``
* Double quotes: ``b"still allows embedded 'single' quotes"``.
* Triple quoted: ``b'''3 single quotes'''``, ``b"""3 double quotes"""``

Only ASCII characters are permitted in bytes literals (regardless of the
declared source code encoding). Any binary values over 127 must be entered
into bytes literals using the appropriate escape sequence.

As with string literals, bytes literals may also use a ``r`` prefix to disable
processing of escape sequences. See :ref:`strings` for more about the various
forms of bytes literal, including supported escape sequences.

While bytes literals and representations are based on ASCII text, bytes
objects actually behave like immutable sequences of integers, with each
value in the sequence restricted such that ``0 <= x < 256`` (attempts to
violate this restriction will trigger :exc:`ValueError`. This is done
deliberately to emphasise that while many binary formats include ASCII based
elements and can be usefully manipulated with some text-oriented algorithms,
this is not generally the case for arbitrary binary data (blindly applying
text processing algorithms to binary data formats that are not ASCII
compatible will usually lead to data corruption).

In addition to the literal forms, bytes objects can be created in a number of
other ways:

* A zero-filled bytes object of a specified length: ``bytes(10)``
* From an iterable of integers: ``bytes(range(20))``
* Copying existing binary data via the buffer protocol:  ``bytes(obj)``

Also see the :ref:`bytes <func-bytes>` built-in.

Since bytes objects are sequences of integers, for a bytes object *b*,
``b[0]`` will be an integer, while ``b[0:1]`` will be a bytes object of
length 1.  (This contrasts with text strings, where both indexing and
slicing will produce a string of length 1)

The representation of bytes objects uses the literal format (``b'...'``)
since it is often more useful than e.g. ``bytes([46, 46, 46])``.  You can
always convert a bytes object into a list of integers using ``list(b)``.


.. note::
   For Python 2.x users: In the Python 2.x series, a variety of implicit
   conversions between 8-bit strings (the closest thing 2.x offers to a
   built-in binary data type) and Unicode strings were permitted. This was a
   backwards compatibility workaround to account for the fact that Python
   originally only supported 8-bit text, and Unicode text was a later
   addition. In Python 3.x, those implicit conversions are gone - conversions
   between 8-bit binary data and Unicode text must be explicit, and bytes and
   string objects will always compare unequal.


.. _typebytearray:

Bytearray Objects
-----------------

.. index:: object: bytearray

:class:`bytearray` objects are a mutable counterpart to :class:`bytes`
objects. There is no dedicated literal syntax for bytearray objects, instead
they are always created by calling the constructor:

* Creating an empty instance: ``bytearray()``
* Creating a zero-filled instance with a given length: ``bytearray(10)``
* From an iterable of integers: ``bytearray(range(20))``
* Copying existing binary data via the buffer protocol:  ``bytearray(b'Hi!')``

As bytearray objects are mutable, they support the
:ref:`mutable <typesseq-mutable>` sequence operations in addition to the
common bytes and bytearray operations described in :ref:`bytes-methods`.

Also see the :ref:`bytearray <func-bytearray>` built-in.


.. _bytes-methods:

Bytes and Bytearray Operations
------------------------------

.. index:: pair: bytes; methods
           pair: bytearray; methods

Both bytes and bytearray objects support the :ref:`common <typesseq-common>`
sequence operations. They interoperate not just with operands of the same
type, but with any object that supports the
:ref:`buffer protocol <bufferobjects>`. Due to this flexibility, they can be
freely mixed in operations without causing errors. However, the return type
of the result may depend on the order of operands.

Due to the common use of ASCII text as the basis for binary protocols, bytes
and bytearray objects provide almost all methods found on text strings, with
the exceptions of:

* :meth:`str.encode` (which converts text strings to bytes objects)
* :meth:`str.format` and :meth:`str.format_map` (which are used to format
  text for display to users)
* :meth:`str.isidentifier`, :meth:`str.isnumeric`, :meth:`str.isdecimal`,
  :meth:`str.isprintable` (which are used to check various properties of
  text strings which are not typically applicable to binary protocols).

All other string methods are supported, although sometimes with slight
differences in functionality and semantics (as described below).

.. note::

   The methods on bytes and bytearray objects don't accept strings as their
   arguments, just as the methods on strings don't accept bytes as their
   arguments.  For example, you have to write::

      a = "abc"
      b = a.replace("a", "f")

   and::

      a = b"abc"
      b = a.replace(b"a", b"f")

Whenever a bytes or bytearray method needs to interpret the bytes as
characters (e.g. the :meth:`is...` methods, :meth:`split`, :meth:`strip`),
the ASCII character set is assumed (text strings use Unicode semantics).

.. note::
   Using these ASCII based methods to manipulate binary data that is not
   stored in an ASCII based format may lead to data corruption.

The search operations (:keyword:`in`, :meth:`count`, :meth:`find`,
:meth:`index`, :meth:`rfind` and :meth:`rindex`) all accept both integers
in the range 0 to 255 (inclusive) as well as bytes and byte array sequences.

.. versionchanged:: 3.3
   All of the search methods also accept an integer in the range 0 to 255
   (inclusive) as their first argument.


Each bytes and bytearray instance provides a :meth:`~bytes.decode` convenience
method that is the inverse of :meth:`str.encode`:

.. method:: bytes.decode(encoding="utf-8", errors="strict")
            bytearray.decode(encoding="utf-8", errors="strict")

   Return a string decoded from the given bytes.  Default encoding is
   ``'utf-8'``. *errors* may be given to set a different
   error handling scheme.  The default for *errors* is ``'strict'``, meaning
   that encoding errors raise a :exc:`UnicodeError`.  Other possible values are
   ``'ignore'``, ``'replace'`` and any other name registered via
   :func:`codecs.register_error`, see section :ref:`codec-base-classes`. For a
   list of possible encodings, see section :ref:`standard-encodings`.

   .. versionchanged:: 3.1
      Added support for keyword arguments.

Since 2 hexadecimal digits correspond precisely to a single byte, hexadecimal
numbers are a commonly used format for describing binary data. Accordingly,
the bytes and bytearray types have an additional class method to read data in
that format:

.. classmethod:: bytes.fromhex(string)
                 bytearray.fromhex(string)

   This :class:`bytes` class method returns a bytes or bytearray object,
   decoding the given string object.  The string must contain two hexadecimal
   digits per byte, spaces are ignored.

   >>> bytes.fromhex('2Ef0 F1f2  ')
   b'.\xf0\xf1\xf2'


The maketrans and translate methods differ in semantics from the versions
available on strings:

.. method:: bytes.translate(table[, delete])
            bytearray.translate(table[, delete])

   Return a copy of the bytes or bytearray object where all bytes occurring in
   the optional argument *delete* are removed, and the remaining bytes have been
   mapped through the given translation table, which must be a bytes object of
   length 256.

   You can use the :func:`bytes.maketrans` method to create a translation table.

   Set the *table* argument to ``None`` for translations that only delete
   characters::

      >>> b'read this short text'.translate(None, b'aeiou')
      b'rd ths shrt txt'


.. staticmethod:: bytes.maketrans(from, to)
                  bytearray.maketrans(from, to)

   This static method returns a translation table usable for
   :meth:`bytes.translate` that will map each character in *from* into the
   character at the same position in *to*; *from* and *to* must be bytes objects
   and have the same length.

   .. versionadded:: 3.1


.. _typememoryview:

Memory Views
------------

:class:`memoryview` objects allow Python code to access the internal data
of an object that supports the :ref:`buffer protocol <bufferobjects>` without
copying.

.. class:: memoryview(obj)

   Create a :class:`memoryview` that references *obj*.  *obj* must support the
   buffer protocol.  Built-in objects that support the buffer protocol include
   :class:`bytes` and :class:`bytearray`.

   A :class:`memoryview` has the notion of an *element*, which is the
   atomic memory unit handled by the originating object *obj*.  For many
   simple types such as :class:`bytes` and :class:`bytearray`, an element
   is a single byte, but other types such as :class:`array.array` may have
   bigger elements.

   ``len(view)`` is equal to the length of :class:`~memoryview.tolist`.
   If ``view.ndim = 0``, the length is 1. If ``view.ndim = 1``, the length
   is equal to the number of elements in the view. For higher dimensions,
   the length is equal to the length of the nested list representation of
   the view. The :class:`~memoryview.itemsize` attribute will give you the
   number of bytes in a single element.

   A :class:`memoryview` supports slicing to expose its data. If
   :class:`~memoryview.format` is one of the native format specifiers
   from the :mod:`struct` module, indexing will return a single element
   with the correct type. Full slicing will result in a subview::

    >>> v = memoryview(b'abcefg')
    >>> v[1]
    98
    >>> v[-1]
    103
    >>> v[1:4]
    <memory at 0x7f3ddc9f4350>
    >>> bytes(v[1:4])
    b'bce'

   Other native formats::

      >>> import array
      >>> a = array.array('l', [-11111111, 22222222, -33333333, 44444444])
      >>> a[0]
      -11111111
      >>> a[-1]
      44444444
      >>> a[2:3].tolist()
      [-33333333]
      >>> a[::2].tolist()
      [-11111111, -33333333]
      >>> a[::-1].tolist()
      [44444444, -33333333, 22222222, -11111111]

   .. versionadded:: 3.3

   If the underlying object is writable, the memoryview supports slice
   assignment. Resizing is not allowed::

      >>> data = bytearray(b'abcefg')
      >>> v = memoryview(data)
      >>> v.readonly
      False
      >>> v[0] = ord(b'z')
      >>> data
      bytearray(b'zbcefg')
      >>> v[1:4] = b'123'
      >>> data
      bytearray(b'z123fg')
      >>> v[2:3] = b'spam'
      Traceback (most recent call last):
        File "<stdin>", line 1, in <module>
      ValueError: memoryview assignment: lvalue and rvalue have different structures
      >>> v[2:6] = b'spam'
      >>> data
      bytearray(b'z1spam')

   One-dimensional memoryviews of hashable (read-only) types with formats
   'B', 'b' or 'c' are also hashable. The hash is defined as
   ``hash(m) == hash(m.tobytes())``::

      >>> v = memoryview(b'abcefg')
      >>> hash(v) == hash(b'abcefg')
      True
      >>> hash(v[2:4]) == hash(b'ce')
      True
      >>> hash(v[::-2]) == hash(b'abcefg'[::-2])
      True

   .. versionchanged:: 3.3
      One-dimensional memoryviews with formats 'B', 'b' or 'c' are now hashable.

   .. versionchanged:: 3.4
      memoryview is now registered automatically with
      :class:`collections.abc.Sequence`

   :class:`memoryview` has several methods:

   .. method:: __eq__(exporter)

      A memoryview and a :pep:`3118` exporter are equal if their shapes are
      equivalent and if all corresponding values are equal when the operands'
      respective format codes are interpreted using :mod:`struct` syntax.

      For the subset of :mod:`struct` format strings currently supported by
      :meth:`tolist`, ``v`` and ``w`` are equal if ``v.tolist() == w.tolist()``::

         >>> import array
         >>> a = array.array('I', [1, 2, 3, 4, 5])
         >>> b = array.array('d', [1.0, 2.0, 3.0, 4.0, 5.0])
         >>> c = array.array('b', [5, 3, 1])
         >>> x = memoryview(a)
         >>> y = memoryview(b)
         >>> x == a == y == b
         True
         >>> x.tolist() == a.tolist() == y.tolist() == b.tolist()
         True
         >>> z = y[::-2]
         >>> z == c
         True
         >>> z.tolist() == c.tolist()
         True

      If either format string is not supported by the :mod:`struct` module,
      then the objects will always compare as unequal (even if the format
      strings and buffer contents are identical)::

         >>> from ctypes import BigEndianStructure, c_long
         >>> class BEPoint(BigEndianStructure):
         ...     _fields_ = [("x", c_long), ("y", c_long)]
         ...
         >>> point = BEPoint(100, 200)
         >>> a = memoryview(point)
         >>> b = memoryview(point)
         >>> a == point
         False
         >>> a == b
         False

      Note that, as with floating point numbers, ``v is w`` does *not* imply
      ``v == w`` for memoryview objects.

      .. versionchanged:: 3.3
         Previous versions compared the raw memory disregarding the item format
         and the logical array structure.

   .. method:: tobytes()

      Return the data in the buffer as a bytestring.  This is equivalent to
      calling the :class:`bytes` constructor on the memoryview. ::

         >>> m = memoryview(b"abc")
         >>> m.tobytes()
         b'abc'
         >>> bytes(m)
         b'abc'

      For non-contiguous arrays the result is equal to the flattened list
      representation with all elements converted to bytes. :meth:`tobytes`
      supports all format strings, including those that are not in
      :mod:`struct` module syntax.

   .. method:: tolist()

      Return the data in the buffer as a list of elements. ::

         >>> memoryview(b'abc').tolist()
         [97, 98, 99]
         >>> import array
         >>> a = array.array('d', [1.1, 2.2, 3.3])
         >>> m = memoryview(a)
         >>> m.tolist()
         [1.1, 2.2, 3.3]

      .. versionchanged:: 3.3
         :meth:`tolist` now supports all single character native formats in
         :mod:`struct` module syntax as well as multi-dimensional
         representations.

   .. method:: release()

      Release the underlying buffer exposed by the memoryview object.  Many
      objects take special actions when a view is held on them (for example,
      a :class:`bytearray` would temporarily forbid resizing); therefore,
      calling release() is handy to remove these restrictions (and free any
      dangling resources) as soon as possible.

      After this method has been called, any further operation on the view
      raises a :class:`ValueError` (except :meth:`release()` itself which can
      be called multiple times)::

         >>> m = memoryview(b'abc')
         >>> m.release()
         >>> m[0]
         Traceback (most recent call last):
           File "<stdin>", line 1, in <module>
         ValueError: operation forbidden on released memoryview object

      The context management protocol can be used for a similar effect,
      using the ``with`` statement::

         >>> with memoryview(b'abc') as m:
         ...     m[0]
         ...
         97
         >>> m[0]
         Traceback (most recent call last):
           File "<stdin>", line 1, in <module>
         ValueError: operation forbidden on released memoryview object

      .. versionadded:: 3.2

   .. method:: cast(format[, shape])

      Cast a memoryview to a new format or shape. *shape* defaults to
      ``[byte_length//new_itemsize]``, which means that the result view
      will be one-dimensional. The return value is a new memoryview, but
      the buffer itself is not copied. Supported casts are 1D -> C-contiguous
      and C-contiguous -> 1D.

      Both formats are restricted to single element native formats in
      :mod:`struct` syntax. One of the formats must be a byte format
      ('B', 'b' or 'c'). The byte length of the result must be the same
      as the original length.

      Cast 1D/long to 1D/unsigned bytes::

         >>> import array
         >>> a = array.array('l', [1,2,3])
         >>> x = memoryview(a)
         >>> x.format
         'l'
         >>> x.itemsize
         8
         >>> len(x)
         3
         >>> x.nbytes
         24
         >>> y = x.cast('B')
         >>> y.format
         'B'
         >>> y.itemsize
         1
         >>> len(y)
         24
         >>> y.nbytes
         24

      Cast 1D/unsigned bytes to 1D/char::

         >>> b = bytearray(b'zyz')
         >>> x = memoryview(b)
         >>> x[0] = b'a'
         Traceback (most recent call last):
           File "<stdin>", line 1, in <module>
         ValueError: memoryview: invalid value for format "B"
         >>> y = x.cast('c')
         >>> y[0] = b'a'
         >>> b
         bytearray(b'ayz')

      Cast 1D/bytes to 3D/ints to 1D/signed char::

         >>> import struct
         >>> buf = struct.pack("i"*12, *list(range(12)))
         >>> x = memoryview(buf)
         >>> y = x.cast('i', shape=[2,2,3])
         >>> y.tolist()
         [[[0, 1, 2], [3, 4, 5]], [[6, 7, 8], [9, 10, 11]]]
         >>> y.format
         'i'
         >>> y.itemsize
         4
         >>> len(y)
         2
         >>> y.nbytes
         48
         >>> z = y.cast('b')
         >>> z.format
         'b'
         >>> z.itemsize
         1
         >>> len(z)
         48
         >>> z.nbytes
         48

      Cast 1D/unsigned char to 2D/unsigned long::

         >>> buf = struct.pack("L"*6, *list(range(6)))
         >>> x = memoryview(buf)
         >>> y = x.cast('L', shape=[2,3])
         >>> len(y)
         2
         >>> y.nbytes
         48
         >>> y.tolist()
         [[0, 1, 2], [3, 4, 5]]

      .. versionadded:: 3.3

   There are also several readonly attributes available:

   .. attribute:: obj

      The underlying object of the memoryview::

         >>> b  = bytearray(b'xyz')
         >>> m = memoryview(b)
         >>> m.obj is b
         True

      .. versionadded:: 3.3

   .. attribute:: nbytes

      ``nbytes == product(shape) * itemsize == len(m.tobytes())``. This is
      the amount of space in bytes that the array would use in a contiguous
      representation. It is not necessarily equal to len(m)::

         >>> import array
         >>> a = array.array('i', [1,2,3,4,5])
         >>> m = memoryview(a)
         >>> len(m)
         5
         >>> m.nbytes
         20
         >>> y = m[::2]
         >>> len(y)
         3
         >>> y.nbytes
         12
         >>> len(y.tobytes())
         12

      Multi-dimensional arrays::

         >>> import struct
         >>> buf = struct.pack("d"*12, *[1.5*x for x in range(12)])
         >>> x = memoryview(buf)
         >>> y = x.cast('d', shape=[3,4])
         >>> y.tolist()
         [[0.0, 1.5, 3.0, 4.5], [6.0, 7.5, 9.0, 10.5], [12.0, 13.5, 15.0, 16.5]]
         >>> len(y)
         3
         >>> y.nbytes
         96

      .. versionadded:: 3.3

   .. attribute:: readonly

      A bool indicating whether the memory is read only.

   .. attribute:: format

      A string containing the format (in :mod:`struct` module style) for each
      element in the view. A memoryview can be created from exporters with
      arbitrary format strings, but some methods (e.g. :meth:`tolist`) are
      restricted to native single element formats.

      .. versionchanged:: 3.3
         format ``'B'`` is now handled according to the struct module syntax.
         This means that ``memoryview(b'abc')[0] == b'abc'[0] == 97``.

   .. attribute:: itemsize

      The size in bytes of each element of the memoryview::

         >>> import array, struct
         >>> m = memoryview(array.array('H', [32000, 32001, 32002]))
         >>> m.itemsize
         2
         >>> m[0]
         32000
         >>> struct.calcsize('H') == m.itemsize
         True

   .. attribute:: ndim

      An integer indicating how many dimensions of a multi-dimensional array the
      memory represents.

   .. attribute:: shape

      A tuple of integers the length of :attr:`ndim` giving the shape of the
      memory as an N-dimensional array.

      .. versionchanged:: 3.3
         An empty tuple instead of None when ndim = 0.

   .. attribute:: strides

      A tuple of integers the length of :attr:`ndim` giving the size in bytes to
      access each element for each dimension of the array.

      .. versionchanged:: 3.3
         An empty tuple instead of None when ndim = 0.

   .. attribute:: suboffsets

      Used internally for PIL-style arrays. The value is informational only.

   .. attribute:: c_contiguous

      A bool indicating whether the memory is C-contiguous.

      .. versionadded:: 3.3

   .. attribute:: f_contiguous

      A bool indicating whether the memory is Fortran contiguous.

      .. versionadded:: 3.3

   .. attribute:: contiguous

      A bool indicating whether the memory is contiguous.

      .. versionadded:: 3.3


.. _types-set:

Set Types --- :class:`set`, :class:`frozenset`
==============================================

.. index:: object: set

A :dfn:`set` object is an unordered collection of distinct :term:`hashable` objects.
Common uses include membership testing, removing duplicates from a sequence, and
computing mathematical operations such as intersection, union, difference, and
symmetric difference.
(For other containers see the built-in :class:`dict`, :class:`list`,
and :class:`tuple` classes, and the :mod:`collections` module.)

Like other collections, sets support ``x in set``, ``len(set)``, and ``for x in
set``.  Being an unordered collection, sets do not record element position or
order of insertion.  Accordingly, sets do not support indexing, slicing, or
other sequence-like behavior.

There are currently two built-in set types, :class:`set` and :class:`frozenset`.
The :class:`set` type is mutable --- the contents can be changed using methods
like :meth:`~set.add` and :meth:`~set.remove`.  Since it is mutable, it has no
hash value and cannot be used as either a dictionary key or as an element of
another set.  The :class:`frozenset` type is immutable and :term:`hashable` ---
its contents cannot be altered after it is created; it can therefore be used as
a dictionary key or as an element of another set.

Non-empty sets (not frozensets) can be created by placing a comma-separated list
of elements within braces, for example: ``{'jack', 'sjoerd'}``, in addition to the
:class:`set` constructor.

The constructors for both classes work the same:

.. class:: set([iterable])
           frozenset([iterable])

   Return a new set or frozenset object whose elements are taken from
   *iterable*.  The elements of a set must be :term:`hashable`.  To
   represent sets of sets, the inner sets must be :class:`frozenset`
   objects.  If *iterable* is not specified, a new empty set is
   returned.

   Instances of :class:`set` and :class:`frozenset` provide the following
   operations:

   .. describe:: len(s)

      Return the cardinality of set *s*.

   .. describe:: x in s

      Test *x* for membership in *s*.

   .. describe:: x not in s

      Test *x* for non-membership in *s*.

   .. method:: isdisjoint(other)

      Return ``True`` if the set has no elements in common with *other*.  Sets are
      disjoint if and only if their intersection is the empty set.

   .. method:: issubset(other)
               set <= other

      Test whether every element in the set is in *other*.

   .. method:: set < other

      Test whether the set is a proper subset of *other*, that is,
      ``set <= other and set != other``.

   .. method:: issuperset(other)
               set >= other

      Test whether every element in *other* is in the set.

   .. method:: set > other

      Test whether the set is a proper superset of *other*, that is, ``set >=
      other and set != other``.

   .. method:: union(other, ...)
               set | other | ...

      Return a new set with elements from the set and all others.

   .. method:: intersection(other, ...)
               set & other & ...

      Return a new set with elements common to the set and all others.

   .. method:: difference(other, ...)
               set - other - ...

      Return a new set with elements in the set that are not in the others.

   .. method:: symmetric_difference(other)
               set ^ other

      Return a new set with elements in either the set or *other* but not both.

   .. method:: copy()

      Return a new set with a shallow copy of *s*.


   Note, the non-operator versions of :meth:`union`, :meth:`intersection`,
   :meth:`difference`, and :meth:`symmetric_difference`, :meth:`issubset`, and
   :meth:`issuperset` methods will accept any iterable as an argument.  In
   contrast, their operator based counterparts require their arguments to be
   sets.  This precludes error-prone constructions like ``set('abc') & 'cbs'``
   in favor of the more readable ``set('abc').intersection('cbs')``.

   Both :class:`set` and :class:`frozenset` support set to set comparisons. Two
   sets are equal if and only if every element of each set is contained in the
   other (each is a subset of the other). A set is less than another set if and
   only if the first set is a proper subset of the second set (is a subset, but
   is not equal). A set is greater than another set if and only if the first set
   is a proper superset of the second set (is a superset, but is not equal).

   Instances of :class:`set` are compared to instances of :class:`frozenset`
   based on their members.  For example, ``set('abc') == frozenset('abc')``
   returns ``True`` and so does ``set('abc') in set([frozenset('abc')])``.

   The subset and equality comparisons do not generalize to a total ordering
   function.  For example, any two nonempty disjoint sets are not equal and are not
   subsets of each other, so *all* of the following return ``False``: ``a<b``,
   ``a==b``, or ``a>b``.

   Since sets only define partial ordering (subset relationships), the output of
   the :meth:`list.sort` method is undefined for lists of sets.

   Set elements, like dictionary keys, must be :term:`hashable`.

   Binary operations that mix :class:`set` instances with :class:`frozenset`
   return the type of the first operand.  For example: ``frozenset('ab') |
   set('bc')`` returns an instance of :class:`frozenset`.

   The following table lists operations available for :class:`set` that do not
   apply to immutable instances of :class:`frozenset`:

   .. method:: update(other, ...)
               set |= other | ...

      Update the set, adding elements from all others.

   .. method:: intersection_update(other, ...)
               set &= other & ...

      Update the set, keeping only elements found in it and all others.

   .. method:: difference_update(other, ...)
               set -= other | ...

      Update the set, removing elements found in others.

   .. method:: symmetric_difference_update(other)
               set ^= other

      Update the set, keeping only elements found in either set, but not in both.

   .. method:: add(elem)

      Add element *elem* to the set.

   .. method:: remove(elem)

      Remove element *elem* from the set.  Raises :exc:`KeyError` if *elem* is
      not contained in the set.

   .. method:: discard(elem)

      Remove element *elem* from the set if it is present.

   .. method:: pop()

      Remove and return an arbitrary element from the set.  Raises
      :exc:`KeyError` if the set is empty.

   .. method:: clear()

      Remove all elements from the set.


   Note, the non-operator versions of the :meth:`update`,
   :meth:`intersection_update`, :meth:`difference_update`, and
   :meth:`symmetric_difference_update` methods will accept any iterable as an
   argument.

   Note, the *elem* argument to the :meth:`__contains__`, :meth:`remove`, and
   :meth:`discard` methods may be a set.  To support searching for an equivalent
   frozenset, the *elem* set is temporarily mutated during the search and then
   restored.  During the search, the *elem* set should not be read or mutated
   since it does not have a meaningful value.


.. _typesmapping:

Mapping Types --- :class:`dict`
===============================

.. index::
   object: mapping
   object: dictionary
   triple: operations on; mapping; types
   triple: operations on; dictionary; type
   statement: del
   builtin: len

A :term:`mapping` object maps :term:`hashable` values to arbitrary objects.
Mappings are mutable objects.  There is currently only one standard mapping
type, the :dfn:`dictionary`.  (For other containers see the built-in
:class:`list`, :class:`set`, and :class:`tuple` classes, and the
:mod:`collections` module.)

A dictionary's keys are *almost* arbitrary values.  Values that are not
:term:`hashable`, that is, values containing lists, dictionaries or other
mutable types (that are compared by value rather than by object identity) may
not be used as keys.  Numeric types used for keys obey the normal rules for
numeric comparison: if two numbers compare equal (such as ``1`` and ``1.0``)
then they can be used interchangeably to index the same dictionary entry.  (Note
however, that since computers store floating-point numbers as approximations it
is usually unwise to use them as dictionary keys.)

Dictionaries can be created by placing a comma-separated list of ``key: value``
pairs within braces, for example: ``{'jack': 4098, 'sjoerd': 4127}`` or ``{4098:
'jack', 4127: 'sjoerd'}``, or by the :class:`dict` constructor.

.. class:: dict(**kwarg)
           dict(mapping, **kwarg)
           dict(iterable, **kwarg)

   Return a new dictionary initialized from an optional positional argument
   and a possibly empty set of keyword arguments.

   If no positional argument is given, an empty dictionary is created.
   If a positional argument is given and it is a mapping object, a dictionary
   is created with the same key-value pairs as the mapping object.  Otherwise,
   the positional argument must be an :term:`iterator` object.  Each item in
   the iterable must itself be an iterator with exactly two objects.  The
   first object of each item becomes a key in the new dictionary, and the
   second object the corresponding value.  If a key occurs more than once, the
   last value for that key becomes the corresponding value in the new
   dictionary.

   If keyword arguments are given, the keyword arguments and their values are
   added to the dictionary created from the positional argument.  If a key
   being added is already present, the value from the keyword argument
   replaces the value from the positional argument.

   To illustrate, the following examples all return a dictionary equal to
   ``{"one": 1, "two": 2, "three": 3}``::

      >>> a = dict(one=1, two=2, three=3)
      >>> b = {'one': 1, 'two': 2, 'three': 3}
      >>> c = dict(zip(['one', 'two', 'three'], [1, 2, 3]))
      >>> d = dict([('two', 2), ('one', 1), ('three', 3)])
      >>> e = dict({'three': 3, 'one': 1, 'two': 2})
      >>> a == b == c == d == e
      True

   Providing keyword arguments as in the first example only works for keys that
   are valid Python identifiers.  Otherwise, any valid keys can be used.


   These are the operations that dictionaries support (and therefore, custom
   mapping types should support too):

   .. describe:: len(d)

      Return the number of items in the dictionary *d*.

   .. describe:: d[key]

      Return the item of *d* with key *key*.  Raises a :exc:`KeyError` if *key* is
      not in the map.

      If a subclass of dict defines a method :meth:`__missing__`, if the key *key*
      is not present, the ``d[key]`` operation calls that method with the key *key*
      as argument.  The ``d[key]`` operation then returns or raises whatever is
      returned or raised by the ``__missing__(key)`` call if the key is not
      present. No other operations or methods invoke :meth:`__missing__`. If
      :meth:`__missing__` is not defined, :exc:`KeyError` is raised.
      :meth:`__missing__` must be a method; it cannot be an instance variable::

          >>> class Counter(dict):
          ...     def __missing__(self, key):
          ...         return 0
          >>> c = Counter()
          >>> c['red']
          0
          >>> c['red'] += 1
          >>> c['red']
          1

      See :class:`collections.Counter` for a complete implementation including
      other methods helpful for accumulating and managing tallies.

   .. describe:: d[key] = value

      Set ``d[key]`` to *value*.

   .. describe:: del d[key]

      Remove ``d[key]`` from *d*.  Raises a :exc:`KeyError` if *key* is not in the
      map.

   .. describe:: key in d

      Return ``True`` if *d* has a key *key*, else ``False``.

   .. describe:: key not in d

      Equivalent to ``not key in d``.

   .. describe:: iter(d)

      Return an iterator over the keys of the dictionary.  This is a shortcut
      for ``iter(d.keys())``.

   .. method:: clear()

      Remove all items from the dictionary.

   .. method:: copy()

      Return a shallow copy of the dictionary.

   .. classmethod:: fromkeys(seq[, value])

      Create a new dictionary with keys from *seq* and values set to *value*.

      :meth:`fromkeys` is a class method that returns a new dictionary. *value*
      defaults to ``None``.

   .. method:: get(key[, default])

      Return the value for *key* if *key* is in the dictionary, else *default*.
      If *default* is not given, it defaults to ``None``, so that this method
      never raises a :exc:`KeyError`.

   .. method:: items()

      Return a new view of the dictionary's items (``(key, value)`` pairs).
      See the :ref:`documentation of view objects <dict-views>`.

   .. method:: keys()

      Return a new view of the dictionary's keys.  See the :ref:`documentation
      of view objects <dict-views>`.

   .. method:: pop(key[, default])

      If *key* is in the dictionary, remove it and return its value, else return
      *default*.  If *default* is not given and *key* is not in the dictionary,
      a :exc:`KeyError` is raised.

   .. method:: popitem()

      Remove and return an arbitrary ``(key, value)`` pair from the dictionary.

      :meth:`popitem` is useful to destructively iterate over a dictionary, as
      often used in set algorithms.  If the dictionary is empty, calling
      :meth:`popitem` raises a :exc:`KeyError`.

   .. method:: setdefault(key[, default])

      If *key* is in the dictionary, return its value.  If not, insert *key*
      with a value of *default* and return *default*.  *default* defaults to
      ``None``.

   .. method:: update([other])

      Update the dictionary with the key/value pairs from *other*, overwriting
      existing keys.  Return ``None``.

      :meth:`update` accepts either another dictionary object or an iterable of
      key/value pairs (as tuples or other iterables of length two).  If keyword
      arguments are specified, the dictionary is then updated with those
      key/value pairs: ``d.update(red=1, blue=2)``.

   .. method:: values()

      Return a new view of the dictionary's values.  See the
      :ref:`documentation of view objects <dict-views>`.

.. seealso::
   :class:`types.MappingProxyType` can be used to create a read-only view
   of a :class:`dict`.


.. _dict-views:

Dictionary view objects
-----------------------

The objects returned by :meth:`dict.keys`, :meth:`dict.values` and
:meth:`dict.items` are *view objects*.  They provide a dynamic view on the
dictionary's entries, which means that when the dictionary changes, the view
reflects these changes.

Dictionary views can be iterated over to yield their respective data, and
support membership tests:

.. describe:: len(dictview)

   Return the number of entries in the dictionary.

.. describe:: iter(dictview)

   Return an iterator over the keys, values or items (represented as tuples of
   ``(key, value)``) in the dictionary.

   Keys and values are iterated over in an arbitrary order which is non-random,
   varies across Python implementations, and depends on the dictionary's history
   of insertions and deletions. If keys, values and items views are iterated
   over with no intervening modifications to the dictionary, the order of items
   will directly correspond.  This allows the creation of ``(value, key)`` pairs
   using :func:`zip`: ``pairs = zip(d.values(), d.keys())``.  Another way to
   create the same list is ``pairs = [(v, k) for (k, v) in d.items()]``.

   Iterating views while adding or deleting entries in the dictionary may raise
   a :exc:`RuntimeError` or fail to iterate over all entries.

.. describe:: x in dictview

   Return ``True`` if *x* is in the underlying dictionary's keys, values or
   items (in the latter case, *x* should be a ``(key, value)`` tuple).


Keys views are set-like since their entries are unique and hashable.  If all
values are hashable, so that ``(key, value)`` pairs are unique and hashable,
then the items view is also set-like.  (Values views are not treated as set-like
since the entries are generally not unique.)  For set-like views, all of the
operations defined for the abstract base class :class:`collections.abc.Set` are
available (for example, ``==``, ``<``, or ``^``).

An example of dictionary view usage::

   >>> dishes = {'eggs': 2, 'sausage': 1, 'bacon': 1, 'spam': 500}
   >>> keys = dishes.keys()
   >>> values = dishes.values()

   >>> # iteration
   >>> n = 0
   >>> for val in values:
   ...     n += val
   >>> print(n)
   504

   >>> # keys and values are iterated over in the same order
   >>> list(keys)
   ['eggs', 'bacon', 'sausage', 'spam']
   >>> list(values)
   [2, 1, 1, 500]

   >>> # view objects are dynamic and reflect dict changes
   >>> del dishes['eggs']
   >>> del dishes['sausage']
   >>> list(keys)
   ['spam', 'bacon']

   >>> # set operations
   >>> keys & {'eggs', 'bacon', 'salad'}
   {'bacon'}
   >>> keys ^ {'sausage', 'juice'}
   {'juice', 'sausage', 'bacon', 'spam'}


.. _typecontextmanager:

Context Manager Types
=====================

.. index::
   single: context manager
   single: context management protocol
   single: protocol; context management

Python's :keyword:`with` statement supports the concept of a runtime context
defined by a context manager.  This is implemented using a pair of methods
that allow user-defined classes to define a runtime context that is entered
before the statement body is executed and exited when the statement ends:


.. method:: contextmanager.__enter__()

   Enter the runtime context and return either this object or another object
   related to the runtime context. The value returned by this method is bound to
   the identifier in the :keyword:`as` clause of :keyword:`with` statements using
   this context manager.

   An example of a context manager that returns itself is a :term:`file object`.
   File objects return themselves from __enter__() to allow :func:`open` to be
   used as the context expression in a :keyword:`with` statement.

   An example of a context manager that returns a related object is the one
   returned by :func:`decimal.localcontext`. These managers set the active
   decimal context to a copy of the original decimal context and then return the
   copy. This allows changes to be made to the current decimal context in the body
   of the :keyword:`with` statement without affecting code outside the
   :keyword:`with` statement.


.. method:: contextmanager.__exit__(exc_type, exc_val, exc_tb)

   Exit the runtime context and return a Boolean flag indicating if any exception
   that occurred should be suppressed. If an exception occurred while executing the
   body of the :keyword:`with` statement, the arguments contain the exception type,
   value and traceback information. Otherwise, all three arguments are ``None``.

   Returning a true value from this method will cause the :keyword:`with` statement
   to suppress the exception and continue execution with the statement immediately
   following the :keyword:`with` statement. Otherwise the exception continues
   propagating after this method has finished executing. Exceptions that occur
   during execution of this method will replace any exception that occurred in the
   body of the :keyword:`with` statement.

   The exception passed in should never be reraised explicitly - instead, this
   method should return a false value to indicate that the method completed
   successfully and does not want to suppress the raised exception. This allows
   context management code (such as ``contextlib.nested``) to easily detect whether
   or not an :meth:`__exit__` method has actually failed.

Python defines several context managers to support easy thread synchronisation,
prompt closure of files or other objects, and simpler manipulation of the active
decimal arithmetic context. The specific types are not treated specially beyond
their implementation of the context management protocol. See the
:mod:`contextlib` module for some examples.

Python's :term:`generator`\s and the :class:`contextlib.contextmanager` decorator
provide a convenient way to implement these protocols.  If a generator function is
decorated with the :class:`contextlib.contextmanager` decorator, it will return a
context manager implementing the necessary :meth:`__enter__` and
:meth:`__exit__` methods, rather than the iterator produced by an undecorated
generator function.

Note that there is no specific slot for any of these methods in the type
structure for Python objects in the Python/C API. Extension types wanting to
define these methods must provide them as a normal Python accessible method.
Compared to the overhead of setting up the runtime context, the overhead of a
single class dictionary lookup is negligible.


.. _typesother:

Other Built-in Types
====================

The interpreter supports several other kinds of objects. Most of these support
only one or two operations.


.. _typesmodules:

Modules
-------

The only special operation on a module is attribute access: ``m.name``, where
*m* is a module and *name* accesses a name defined in *m*'s symbol table.
Module attributes can be assigned to.  (Note that the :keyword:`import`
statement is not, strictly speaking, an operation on a module object; ``import
foo`` does not require a module object named *foo* to exist, rather it requires
an (external) *definition* for a module named *foo* somewhere.)

A special attribute of every module is :attr:`~object.__dict__`. This is the
dictionary containing the module's symbol table. Modifying this dictionary will
actually change the module's symbol table, but direct assignment to the
:attr:`__dict__` attribute is not possible (you can write
``m.__dict__['a'] = 1``, which defines ``m.a`` to be ``1``, but you can't write
``m.__dict__ = {}``).  Modifying :attr:`__dict__` directly is not recommended.

Modules built into the interpreter are written like this: ``<module 'sys'
(built-in)>``.  If loaded from a file, they are written as ``<module 'os' from
'/usr/local/lib/pythonX.Y/os.pyc'>``.


.. _typesobjects:

Classes and Class Instances
---------------------------

See :ref:`objects` and :ref:`class` for these.


.. _typesfunctions:

Functions
---------

Function objects are created by function definitions.  The only operation on a
function object is to call it: ``func(argument-list)``.

There are really two flavors of function objects: built-in functions and
user-defined functions.  Both support the same operation (to call the function),
but the implementation is different, hence the different object types.

See :ref:`function` for more information.


.. _typesmethods:

Methods
-------

.. index:: object: method

Methods are functions that are called using the attribute notation. There are
two flavors: built-in methods (such as :meth:`append` on lists) and class
instance methods.  Built-in methods are described with the types that support
them.

If you access a method (a function defined in a class namespace) through an
instance, you get a special object: a :dfn:`bound method` (also called
:dfn:`instance method`) object. When called, it will add the ``self`` argument
to the argument list.  Bound methods have two special read-only attributes:
``m.__self__`` is the object on which the method operates, and ``m.__func__`` is
the function implementing the method.  Calling ``m(arg-1, arg-2, ..., arg-n)``
is completely equivalent to calling ``m.__func__(m.__self__, arg-1, arg-2, ...,
arg-n)``.

Like function objects, bound method objects support getting arbitrary
attributes.  However, since method attributes are actually stored on the
underlying function object (``meth.__func__``), setting method attributes on
bound methods is disallowed.  Attempting to set an attribute on a method
results in an :exc:`AttributeError` being raised.  In order to set a method
attribute, you need to explicitly set it on the underlying function object::

   >>> class C:
   ...     def method(self):
   ...         pass
   ...
   >>> c = C()
   >>> c.method.whoami = 'my name is method'  # can't set on the method
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   AttributeError: 'method' object has no attribute 'whoami'
   >>> c.method.__func__.whoami = 'my name is method'
   >>> c.method.whoami
   'my name is method'

See :ref:`types` for more information.


.. _bltin-code-objects:

Code Objects
------------

.. index:: object: code

.. index::
   builtin: compile
   single: __code__ (function object attribute)

Code objects are used by the implementation to represent "pseudo-compiled"
executable Python code such as a function body. They differ from function
objects because they don't contain a reference to their global execution
environment.  Code objects are returned by the built-in :func:`compile` function
and can be extracted from function objects through their :attr:`__code__`
attribute. See also the :mod:`code` module.

.. index::
   builtin: exec
   builtin: eval

A code object can be executed or evaluated by passing it (instead of a source
string) to the :func:`exec` or :func:`eval`  built-in functions.

See :ref:`types` for more information.


.. _bltin-type-objects:

Type Objects
------------

.. index::
   builtin: type
   module: types

Type objects represent the various object types.  An object's type is accessed
by the built-in function :func:`type`.  There are no special operations on
types.  The standard module :mod:`types` defines names for all standard built-in
types.

Types are written like this: ``<class 'int'>``.


.. _bltin-null-object:

The Null Object
---------------

This object is returned by functions that don't explicitly return a value.  It
supports no special operations.  There is exactly one null object, named
``None`` (a built-in name).  ``type(None)()`` produces the same singleton.

It is written as ``None``.


.. _bltin-ellipsis-object:

The Ellipsis Object
-------------------

This object is commonly used by slicing (see :ref:`slicings`).  It supports no
special operations.  There is exactly one ellipsis object, named
:const:`Ellipsis` (a built-in name).  ``type(Ellipsis)()`` produces the
:const:`Ellipsis` singleton.

It is written as ``Ellipsis`` or ``...``.


.. _bltin-notimplemented-object:

The NotImplemented Object
-------------------------

This object is returned from comparisons and binary operations when they are
asked to operate on types they don't support. See :ref:`comparisons` for more
information.  There is exactly one ``NotImplemented`` object.
``type(NotImplemented)()`` produces the singleton instance.

It is written as ``NotImplemented``.


.. _bltin-boolean-values:

Boolean Values
--------------

Boolean values are the two constant objects ``False`` and ``True``.  They are
used to represent truth values (although other values can also be considered
false or true).  In numeric contexts (for example when used as the argument to
an arithmetic operator), they behave like the integers 0 and 1, respectively.
The built-in function :func:`bool` can be used to convert any value to a
Boolean, if the value can be interpreted as a truth value (see section
:ref:`truth` above).

.. index::
   single: False
   single: True
   pair: Boolean; values

They are written as ``False`` and ``True``, respectively.


.. _typesinternal:

Internal Objects
----------------

See :ref:`types` for this information.  It describes stack frame objects,
traceback objects, and slice objects.


.. _specialattrs:

Special Attributes
==================

The implementation adds a few special read-only attributes to several object
types, where they are relevant.  Some of these are not reported by the
:func:`dir` built-in function.


.. attribute:: object.__dict__

   A dictionary or other mapping object used to store an object's (writable)
   attributes.


.. attribute:: instance.__class__

   The class to which a class instance belongs.


.. attribute:: class.__bases__

   The tuple of base classes of a class object.


.. attribute:: class.__name__

   The name of the class or type.


.. attribute:: class.__qualname__

   The :term:`qualified name` of the class or type.

   .. versionadded:: 3.3


.. attribute:: class.__mro__

   This attribute is a tuple of classes that are considered when looking for
   base classes during method resolution.


.. method:: class.mro()

   This method can be overridden by a metaclass to customize the method
   resolution order for its instances.  It is called at class instantiation, and
   its result is stored in :attr:`~class.__mro__`.


.. method:: class.__subclasses__

   Each class keeps a list of weak references to its immediate subclasses.  This
   method returns a list of all those references still alive.
   Example::

      >>> int.__subclasses__()
      [<class 'bool'>]


.. rubric:: Footnotes

.. [1] Additional information on these special methods may be found in the Python
   Reference Manual (:ref:`customization`).

.. [2] As a consequence, the list ``[1, 2]`` is considered equal to ``[1.0, 2.0]``, and
   similarly for tuples.

.. [3] They must have since the parser can't tell the type of the operands.

.. [4] Cased characters are those with general category property being one of
   "Lu" (Letter, uppercase), "Ll" (Letter, lowercase), or "Lt" (Letter, titlecase).

.. [5] To format only a tuple you should therefore provide a singleton tuple whose only
   element is the tuple to be formatted.
