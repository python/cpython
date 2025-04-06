:mod:`!numbers` --- Numeric abstract base classes
=================================================

.. module:: numbers
   :synopsis: Numeric abstract base classes (Complex, Real, Integral, etc.).

**Source code:** :source:`Lib/numbers.py`

--------------

The :mod:`!numbers` module (:pep:`3141`) defines a hierarchy of numeric
:term:`abstract base classes <abstract base class>` which progressively define
more operations.  None of the types defined in this module are intended to be instantiated.


.. class:: Number

   The root of the numeric hierarchy. If you just want to check if an argument
   *x* is a number, without caring what kind, use ``isinstance(x, Number)``.


The numeric tower
-----------------

.. class:: Complex

   Subclasses of this type describe complex numbers and include the operations
   that work on the built-in :class:`complex` type. These are: conversions to
   :class:`complex` and :class:`bool`, :attr:`.real`, :attr:`.imag`, ``+``,
   ``-``, ``*``, ``/``, ``**``, :func:`abs`, :meth:`conjugate`, ``==``, and
   ``!=``. All except ``-`` and ``!=`` are abstract.

   .. attribute:: real

      Abstract. Retrieves the real component of this number.

   .. attribute:: imag

      Abstract. Retrieves the imaginary component of this number.

   .. method:: conjugate()
      :abstractmethod:

      Abstract. Returns the complex conjugate. For example, ``(1+3j).conjugate()
      == (1-3j)``.

.. class:: Real

   To :class:`Complex`, :class:`!Real` adds the operations that work on real
   numbers.

   In short, those are: a conversion to :class:`float`, :func:`math.trunc`,
   :func:`round`, :func:`math.floor`, :func:`math.ceil`, :func:`divmod`, ``//``,
   ``%``, ``<``, ``<=``, ``>``, and ``>=``.

   Real also provides defaults for :func:`complex`, :attr:`~Complex.real`,
   :attr:`~Complex.imag`, and :meth:`~Complex.conjugate`.


.. class:: Rational

   Subtypes :class:`Real` and adds :attr:`~Rational.numerator` and
   :attr:`~Rational.denominator` properties. It also provides a default for
   :func:`float`.

   The :attr:`~Rational.numerator` and :attr:`~Rational.denominator` values
   should be instances of :class:`Integral` and should be in lowest terms with
   :attr:`~Rational.denominator` positive.

   .. attribute:: numerator

      Abstract.

   .. attribute:: denominator

      Abstract.


.. class:: Integral

   Subtypes :class:`Rational` and adds a conversion to :class:`int`.  Provides
   defaults for :func:`float`, :attr:`~Rational.numerator`, and
   :attr:`~Rational.denominator`.  Adds abstract methods for :func:`pow` with
   modulus and bit-string operations: ``<<``, ``>>``, ``&``, ``^``, ``|``,
   ``~``.


Notes for type implementers
---------------------------

Implementers should be careful to make equal numbers equal and hash
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


.. _implementing-the-arithmetic-operations:

Implementing the arithmetic operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We want to implement the arithmetic operations so that mixed-mode
operations either call an implementation whose author knew about the
types of both arguments, or convert both to the nearest built in type
and do the operation there. For subtypes of :class:`Integral`, this
means that :meth:`~object.__add__` and :meth:`~object.__radd__` should be
defined as::

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

1. If ``A`` defines an :meth:`~object.__add__` which accepts ``b``, all is
   well.
2. If ``A`` falls back to the boilerplate code, and it were to
   return a value from :meth:`~object.__add__`, we'd miss the possibility
   that ``B`` defines a more intelligent :meth:`~object.__radd__`, so the
   boilerplate should return :data:`NotImplemented` from
   :meth:`!__add__`. (Or ``A`` may not implement :meth:`!__add__` at
   all.)
3. Then ``B``'s :meth:`~object.__radd__` gets a chance. If it accepts
   ``a``, all is well.
4. If it falls back to the boilerplate, there are no more possible
   methods to try, so this is where the default implementation
   should live.
5. If ``B <: A``, Python tries ``B.__radd__`` before
   ``A.__add__``. This is ok, because it was implemented with
   knowledge of ``A``, so it can handle those instances before
   delegating to :class:`Complex`.

If ``A <: Complex`` and ``B <: Real`` without sharing any other knowledge,
then the appropriate shared operation is the one involving the built
in :class:`complex`, and both :meth:`~object.__radd__` s land there, so ``a+b
== b+a``.

Because most of the operations on any given type will be very similar,
it can be useful to define a helper function which generates the
forward and reverse instances of any given operator. For example,
:class:`fractions.Fraction` uses::

    def _operator_fallbacks(monomorphic_operator, fallback_operator):
        def forward(a, b):
            if isinstance(b, (int, Fraction)):
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
            elif isinstance(a, Real):
                return fallback_operator(float(a), float(b))
            elif isinstance(a, Complex):
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
