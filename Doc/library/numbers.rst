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


Notes for type implementors
---------------------------

Implementors should be careful to make equal numbers equal and hash
them to the same values. This may be subtle if there are two different
extensions of the real numbers. For example, :class:`fractions.Fraction`
implements :func:`hash` as follows::

    def __hash__(self):
        if self.denominator == 1:
            # Get integers right.
            return hash(self.numerator)
        # Expensive check, but definitely correct.
        if self == float(self):
            return hash(float(self))
        else:
            # Use tuple's hash to avoid a high collision rate on
            # simple fractions.
            return hash((self.numerator, self.denominator))


Adding More Numeric ABCs
~~~~~~~~~~~~~~~~~~~~~~~~

There are, of course, more possible ABCs for numbers, and this would
be a poor hierarchy if it precluded the possibility of adding
those. You can add ``MyFoo`` between :class:`Complex` and
:class:`Real` with::

    class MyFoo(Complex): ...
    MyFoo.register(Real)


Implementing the arithmetic operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We want to implement the arithmetic operations so that mixed-mode
operations either call an implementation whose author knew about the
types of both arguments, or convert both to the nearest built in type
and do the operation there. For subtypes of :class:`Integral`, this
means that :meth:`__add__` and :meth:`__radd__` should be defined as::

    class MyIntegral(Integral):

        def __add__(self, other):
            if isinstance(other, MyIntegral):
                return do_my_adding_stuff(self, other)
            elif isinstance(other, OtherTypeIKnowAbout):
                return do_my_other_adding_stuff(self, other)
            else:
                return NotImplemented

        def __radd__(self, other):
            if isinstance(other, MyIntegral):
                return do_my_adding_stuff(other, self)
            elif isinstance(other, OtherTypeIKnowAbout):
                return do_my_other_adding_stuff(other, self)
            elif isinstance(other, Integral):
                return int(other) + int(self)
            elif isinstance(other, Real):
                return float(other) + float(self)
            elif isinstance(other, Complex):
                return complex(other) + complex(self)
            else:
                return NotImplemented


There are 5 different cases for a mixed-type operation on subclasses
of :class:`Complex`. I'll refer to all of the above code that doesn't
refer to ``MyIntegral`` and ``OtherTypeIKnowAbout`` as
"boilerplate". ``a`` will be an instance of ``A``, which is a subtype
of :class:`Complex` (``a : A <: Complex``), and ``b : B <:
Complex``. I'll consider ``a + b``:

    1. If ``A`` defines an :meth:`__add__` which accepts ``b``, all is
       well.
    2. If ``A`` falls back to the boilerplate code, and it were to
       return a value from :meth:`__add__`, we'd miss the possibility
       that ``B`` defines a more intelligent :meth:`__radd__`, so the
       boilerplate should return :const:`NotImplemented` from
       :meth:`__add__`. (Or ``A`` may not implement :meth:`__add__` at
       all.)
    3. Then ``B``'s :meth:`__radd__` gets a chance. If it accepts
       ``a``, all is well.
    4. If it falls back to the boilerplate, there are no more possible
       methods to try, so this is where the default implementation
       should live.
    5. If ``B <: A``, Python tries ``B.__radd__`` before
       ``A.__add__``. This is ok, because it was implemented with
       knowledge of ``A``, so it can handle those instances before
       delegating to :class:`Complex`.

If ``A<:Complex`` and ``B<:Real`` without sharing any other knowledge,
then the appropriate shared operation is the one involving the built
in :class:`complex`, and both :meth:`__radd__` s land there, so ``a+b
== b+a``.

Because most of the operations on any given type will be very similar,
it can be useful to define a helper function which generates the
forward and reverse instances of any given operator. For example,
:class:`fractions.Fraction` uses::

    def _operator_fallbacks(monomorphic_operator, fallback_operator):
        def forward(a, b):
            if isinstance(b, (int, long, Fraction)):
                return monomorphic_operator(a, b)
            elif isinstance(b, float):
                return fallback_operator(float(a), b)
            elif isinstance(b, complex):
                return fallback_operator(complex(a), b)
            else:
                return NotImplemented
        forward.__name__ = '__' + fallback_operator.__name__ + '__'
        forward.__doc__ = monomorphic_operator.__doc__

        def reverse(b, a):
            if isinstance(a, Rational):
                # Includes ints.
                return monomorphic_operator(a, b)
            elif isinstance(a, numbers.Real):
                return fallback_operator(float(a), float(b))
            elif isinstance(a, numbers.Complex):
                return fallback_operator(complex(a), complex(b))
            else:
                return NotImplemented
        reverse.__name__ = '__r' + fallback_operator.__name__ + '__'
        reverse.__doc__ = monomorphic_operator.__doc__

        return forward, reverse

    def _add(a, b):
        """a + b"""
        return Fraction(a.numerator * b.denominator +
                        b.numerator * a.denominator,
                        a.denominator * b.denominator)

    __add__, __radd__ = _operator_fallbacks(_add, operator.add)

    # ...