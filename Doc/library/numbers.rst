:mod:`numbers` --- Numeric abstract base classes
================================================

.. module:: numbers
   :synopsis: Numeric abstract base classes (Complex, Real, Integral, etc.).

.. versionadded:: 2.6


The :mod:`numbers` module (:pep:`3141`) defines a hierarchy of numeric abstract
base classes which progressively define more operations. These concepts also
provide a way to distinguish exact from inexact types. None of the types defined
in this module can be instantiated.


.. class:: Number

   The root of the numeric hierarchy. If you just want to check if an argument
   *x* is a number, without caring what kind, use ``isinstance(x, Number)``.


Exact and inexact operations
----------------------------

.. class:: Exact

   Subclasses of this type have exact operations.

   As long as the result of a homogenous operation is of the same type, you can
   assume that it was computed exactly, and there are no round-off errors. Laws
   like commutativity and associativity hold.


.. class:: Inexact

   Subclasses of this type have inexact operations.

   Given X, an instance of :class:`Inexact`, it is possible that ``(X + -X) + 3
   == 3``, but ``X + (-X + 3) == 0``. The exact form this error takes will vary
   by type, but it's generally unsafe to compare this type for equality.


The numeric tower
-----------------

.. class:: Complex

   Subclasses of this type describe complex numbers and include the operations
   that work on the builtin :class:`complex` type. These are: conversions to
   :class:`complex` and :class:`bool`, :attr:`.real`, :attr:`.imag`, ``+``,
   ``-``, ``*``, ``/``, :func:`abs`, :meth:`conjugate`, ``==``, and ``!=``. All
   except ``-`` and ``!=`` are abstract.

.. attribute:: Complex.real

   Abstract. Retrieves the :class:`Real` component of this number.

.. attribute:: Complex.imag

   Abstract. Retrieves the :class:`Real` component of this number.

.. method:: Complex.conjugate()

   Abstract. Returns the complex conjugate. For example, ``(1+3j).conjugate() ==
   (1-3j)``.

.. class:: Real

   To :class:`Complex`, :class:`Real` adds the operations that work on real
   numbers.

   In short, those are: a conversion to :class:`float`, :func:`trunc`,
   :func:`round`, :func:`math.floor`, :func:`math.ceil`, :func:`divmod`, ``//``,
   ``%``, ``<``, ``<=``, ``>``, and ``>=``.

   Real also provides defaults for :func:`complex`, :attr:`Complex.real`,
   :attr:`Complex.imag`, and :meth:`Complex.conjugate`.


.. class:: Rational

   Subtypes both :class:`Real` and :class:`Exact`, and adds
   :attr:`Rational.numerator` and :attr:`Rational.denominator` properties, which
   should be in lowest terms. With these, it provides a default for
   :func:`float`.

.. attribute:: Rational.numerator

   Abstract.

.. attribute:: Rational.denominator

   Abstract.


.. class:: Integral

   Subtypes :class:`Rational` and adds a conversion to :class:`long`, the
   3-argument form of :func:`pow`, and the bit-string operations: ``<<``,
   ``>>``, ``&``, ``^``, ``|``, ``~``. Provides defaults for :func:`float`,
   :attr:`Rational.numerator`, and :attr:`Rational.denominator`.
