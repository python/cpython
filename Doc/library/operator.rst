:mod:`operator` --- Standard operators as functions
===================================================

.. module:: operator
   :synopsis: Functions corresponding to the standard operators.
.. sectionauthor:: Skip Montanaro <skip@automatrix.com>


.. testsetup::
   
   import operator
   from operator import itemgetter


The :mod:`operator` module exports a set of functions implemented in C
corresponding to the intrinsic operators of Python.  For example,
``operator.add(x, y)`` is equivalent to the expression ``x+y``.  The function
names are those used for special class methods; variants without leading and
trailing ``__`` are also provided for convenience.

The functions fall into categories that perform object comparisons, logical
operations, mathematical operations, sequence operations, and abstract type
tests.

The object comparison functions are useful for all objects, and are named after
the rich comparison operators they support:


.. function:: lt(a, b)
              le(a, b)
              eq(a, b)
              ne(a, b)
              ge(a, b)
              gt(a, b)
              __lt__(a, b)
              __le__(a, b)
              __eq__(a, b)
              __ne__(a, b)
              __ge__(a, b)
              __gt__(a, b)

   Perform "rich comparisons" between *a* and *b*. Specifically, ``lt(a, b)`` is
   equivalent to ``a < b``, ``le(a, b)`` is equivalent to ``a <= b``, ``eq(a,
   b)`` is equivalent to ``a == b``, ``ne(a, b)`` is equivalent to ``a != b``,
   ``gt(a, b)`` is equivalent to ``a > b`` and ``ge(a, b)`` is equivalent to ``a
   >= b``.  Note that unlike the built-in :func:`cmp`, these functions can
   return any value, which may or may not be interpretable as a Boolean value.
   See :ref:`comparisons` for more information about rich comparisons.

   .. versionadded:: 2.2

The logical operations are also generally applicable to all objects, and support
truth tests, identity tests, and boolean operations:


.. function:: not_(obj)
              __not__(obj)

   Return the outcome of :keyword:`not` *obj*.  (Note that there is no
   :meth:`__not__` method for object instances; only the interpreter core defines
   this operation.  The result is affected by the :meth:`__nonzero__` and
   :meth:`__len__` methods.)


.. function:: truth(obj)

   Return :const:`True` if *obj* is true, and :const:`False` otherwise.  This is
   equivalent to using the :class:`bool` constructor.


.. function:: is_(a, b)

   Return ``a is b``.  Tests object identity.

   .. versionadded:: 2.3


.. function:: is_not(a, b)

   Return ``a is not b``.  Tests object identity.

   .. versionadded:: 2.3

The mathematical and bitwise operations are the most numerous:


.. function:: abs(obj)
              __abs__(obj)

   Return the absolute value of *obj*.


.. function:: add(a, b)
              __add__(a, b)

   Return ``a + b``, for *a* and *b* numbers.


.. function:: and_(a, b)
              __and__(a, b)

   Return the bitwise and of *a* and *b*.


.. function:: div(a, b)
              __div__(a, b)

   Return ``a / b`` when ``__future__.division`` is not in effect.  This is
   also known as "classic" division.


.. function:: floordiv(a, b)
              __floordiv__(a, b)

   Return ``a // b``.

   .. versionadded:: 2.2


.. function:: inv(obj)
              invert(obj)
              __inv__(obj)
              __invert__(obj)

   Return the bitwise inverse of the number *obj*.  This is equivalent to ``~obj``.

   .. versionadded:: 2.0
      The names :func:`invert` and :func:`__invert__`.


.. function:: lshift(a, b)
              __lshift__(a, b)

   Return *a* shifted left by *b*.


.. function:: mod(a, b)
              __mod__(a, b)

   Return ``a % b``.


.. function:: mul(a, b)
              __mul__(a, b)

   Return ``a * b``, for *a* and *b* numbers.


.. function:: neg(obj)
              __neg__(obj)

   Return *obj* negated.


.. function:: or_(a, b)
              __or__(a, b)

   Return the bitwise or of *a* and *b*.


.. function:: pos(obj)
              __pos__(obj)

   Return *obj* positive.


.. function:: pow(a, b)
              __pow__(a, b)

   Return ``a ** b``, for *a* and *b* numbers.

   .. versionadded:: 2.3


.. function:: rshift(a, b)
              __rshift__(a, b)

   Return *a* shifted right by *b*.


.. function:: sub(a, b)
              __sub__(a, b)

   Return ``a - b``.


.. function:: truediv(a, b)
              __truediv__(a, b)

   Return ``a / b`` when ``__future__.division`` is in effect.  This is also
   known as "true" division.

   .. versionadded:: 2.2


.. function:: xor(a, b)
              __xor__(a, b)

   Return the bitwise exclusive or of *a* and *b*.


.. function:: index(a)
              __index__(a)

   Return *a* converted to an integer.  Equivalent to ``a.__index__()``.

   .. versionadded:: 2.5


Operations which work with sequences include:

.. function:: concat(a, b)
              __concat__(a, b)

   Return ``a + b`` for *a* and *b* sequences.


.. function:: contains(a, b)
              __contains__(a, b)

   Return the outcome of the test ``b in a``. Note the reversed operands.

   .. versionadded:: 2.0
      The name :func:`__contains__`.


.. function:: countOf(a, b)

   Return the number of occurrences of *b* in *a*.


.. function:: delitem(a, b)
              __delitem__(a, b)

   Remove the value of *a* at index *b*.


.. function:: delslice(a, b, c)
              __delslice__(a, b, c)

   Delete the slice of *a* from index *b* to index *c-1*.


.. function:: getitem(a, b)
              __getitem__(a, b)

   Return the value of *a* at index *b*.


.. function:: getslice(a, b, c)
              __getslice__(a, b, c)

   Return the slice of *a* from index *b* to index *c-1*.


.. function:: indexOf(a, b)

   Return the index of the first of occurrence of *b* in *a*.


.. function:: repeat(a, b)
              __repeat__(a, b)

   Return ``a * b`` where *a* is a sequence and *b* is an integer.


.. function:: sequenceIncludes(...)

   .. deprecated:: 2.0
      Use :func:`contains` instead.

   Alias for :func:`contains`.


.. function:: setitem(a, b, c)
              __setitem__(a, b, c)

   Set the value of *a* at index *b* to *c*.


.. function:: setslice(a, b, c, v)
              __setslice__(a, b, c, v)

   Set the slice of *a* from index *b* to index *c-1* to the sequence *v*.

Many operations have an "in-place" version.  The following functions provide a
more primitive access to in-place operators than the usual syntax does; for
example, the :term:`statement` ``x += y`` is equivalent to
``x = operator.iadd(x, y)``.  Another way to put it is to say that
``z = operator.iadd(x, y)`` is equivalent to the compound statement
``z = x; z += y``.

.. function:: iadd(a, b)
              __iadd__(a, b)

   ``a = iadd(a, b)`` is equivalent to ``a += b``.

   .. versionadded:: 2.5


.. function:: iand(a, b)
              __iand__(a, b)

   ``a = iand(a, b)`` is equivalent to ``a &= b``.

   .. versionadded:: 2.5


.. function:: iconcat(a, b)
              __iconcat__(a, b)

   ``a = iconcat(a, b)`` is equivalent to ``a += b`` for *a* and *b* sequences.

   .. versionadded:: 2.5


.. function:: idiv(a, b)
              __idiv__(a, b)

   ``a = idiv(a, b)`` is equivalent to ``a /= b`` when ``__future__.division`` is
   not in effect.

   .. versionadded:: 2.5


.. function:: ifloordiv(a, b)
              __ifloordiv__(a, b)

   ``a = ifloordiv(a, b)`` is equivalent to ``a //= b``.

   .. versionadded:: 2.5


.. function:: ilshift(a, b)
              __ilshift__(a, b)

   ``a = ilshift(a, b)`` is equivalent to ``a <``\ ``<= b``.

   .. versionadded:: 2.5


.. function:: imod(a, b)
              __imod__(a, b)

   ``a = imod(a, b)`` is equivalent to ``a %= b``.

   .. versionadded:: 2.5


.. function:: imul(a, b)
              __imul__(a, b)

   ``a = imul(a, b)`` is equivalent to ``a *= b``.

   .. versionadded:: 2.5


.. function:: ior(a, b)
              __ior__(a, b)

   ``a = ior(a, b)`` is equivalent to ``a |= b``.

   .. versionadded:: 2.5


.. function:: ipow(a, b)
              __ipow__(a, b)

   ``a = ipow(a, b)`` is equivalent to ``a **= b``.

   .. versionadded:: 2.5


.. function:: irepeat(a, b)
              __irepeat__(a, b)

   ``a = irepeat(a, b)`` is equivalent to ``a *= b`` where *a* is a sequence and
   *b* is an integer.

   .. versionadded:: 2.5


.. function:: irshift(a, b)
              __irshift__(a, b)

   ``a = irshift(a, b)`` is equivalent to ``a >>= b``.

   .. versionadded:: 2.5


.. function:: isub(a, b)
              __isub__(a, b)

   ``a = isub(a, b)`` is equivalent to ``a -= b``.

   .. versionadded:: 2.5


.. function:: itruediv(a, b)
              __itruediv__(a, b)

   ``a = itruediv(a, b)`` is equivalent to ``a /= b`` when ``__future__.division``
   is in effect.

   .. versionadded:: 2.5


.. function:: ixor(a, b)
              __ixor__(a, b)

   ``a = ixor(a, b)`` is equivalent to ``a ^= b``.

   .. versionadded:: 2.5


The :mod:`operator` module also defines a few predicates to test the type of
objects.

.. note::

   Be careful not to misinterpret the results of these functions; only
   :func:`isCallable` has any measure of reliability with instance objects.
   For example:

      >>> class C:
      ...     pass
      ... 
      >>> import operator
      >>> obj = C()
      >>> operator.isMappingType(obj)
      True

.. note::

   Python 3 is expected to introduce abstract base classes for
   collection types, so it should be possible to write, for example,
   ``isinstance(obj, collections.Mapping)`` and ``isinstance(obj,
   collections.Sequence)``.

.. function:: isCallable(obj)

   .. deprecated:: 2.0
      Use the :func:`callable` built-in function instead.

   Returns true if the object *obj* can be called like a function, otherwise it
   returns false.  True is returned for functions, bound and unbound methods, class
   objects, and instance objects which support the :meth:`__call__` method.


.. function:: isMappingType(obj)

   Returns true if the object *obj* supports the mapping interface. This is true for
   dictionaries and all instance objects defining :meth:`__getitem__`.

   .. warning::

      There is no reliable way to test if an instance supports the complete mapping
      protocol since the interface itself is ill-defined.  This makes this test less
      useful than it otherwise might be.


.. function:: isNumberType(obj)

   Returns true if the object *obj* represents a number.  This is true for all
   numeric types implemented in C.

   .. warning::

      There is no reliable way to test if an instance supports the complete numeric
      interface since the interface itself is ill-defined.  This makes this test less
      useful than it otherwise might be.


.. function:: isSequenceType(obj)

   Returns true if the object *obj* supports the sequence protocol. This returns true
   for all objects which define sequence methods in C, and for all instance objects
   defining :meth:`__getitem__`.

   .. warning::

      There is no reliable way to test if an instance supports the complete sequence
      interface since the interface itself is ill-defined.  This makes this test less
      useful than it otherwise might be.

Example: Build a dictionary that maps the ordinals from ``0`` to ``255`` to
their character equivalents.

   >>> d = {}
   >>> keys = range(256)
   >>> vals = map(chr, keys)
   >>> map(operator.setitem, [d]*len(keys), keys, vals)   # doctest: +SKIP

.. XXX: find a better, readable, example

The :mod:`operator` module also defines tools for generalized attribute and item
lookups.  These are useful for making fast field extractors as arguments for
:func:`map`, :func:`sorted`, :meth:`itertools.groupby`, or other functions that
expect a function argument.


.. function:: attrgetter(attr[, args...])

   Return a callable object that fetches *attr* from its operand. If more than one
   attribute is requested, returns a tuple of attributes. After,
   ``f = attrgetter('name')``, the call ``f(b)`` returns ``b.name``.  After,
   ``f = attrgetter('name', 'date')``, the call ``f(b)`` returns ``(b.name,
   b.date)``.

   The attribute names can also contain dots; after ``f = attrgetter('date.month')``,
   the call ``f(b)`` returns ``b.date.month``.

   .. versionadded:: 2.4

   .. versionchanged:: 2.5
      Added support for multiple attributes.

   .. versionchanged:: 2.6
      Added support for dotted attributes.


.. function:: itemgetter(item[, args...])

   Return a callable object that fetches *item* from its operand using the
   operand's :meth:`__getitem__` method.  If multiple items are specified,
   returns a tuple of lookup values.  Equivalent to::

        def itemgetter(*items):
            if len(items) == 1:
                item = items[0]
                def g(obj):
                    return obj[item]
            else:
                def g(obj):
                    return tuple(obj[item] for item in items)
            return g
   
   The items can be any type accepted by the operand's :meth:`__getitem__` 
   method.  Dictionaries accept any hashable value.  Lists, tuples, and 
   strings accept an index or a slice:

      >>> itemgetter(1)('ABCDEFG')
      'B'
      >>> itemgetter(1,3,5)('ABCDEFG')
      ('B', 'D', 'F')
      >>> itemgetter(slice(2,None))('ABCDEFG')
      'CDEFG'

   .. versionadded:: 2.4

   .. versionchanged:: 2.5
      Added support for multiple item extraction.

   Example of using :func:`itemgetter` to retrieve specific fields from a
   tuple record:

       >>> inventory = [('apple', 3), ('banana', 2), ('pear', 5), ('orange', 1)]
       >>> getcount = itemgetter(1)
       >>> map(getcount, inventory)
       [3, 2, 5, 1]
       >>> sorted(inventory, key=getcount)
       [('orange', 1), ('banana', 2), ('apple', 3), ('pear', 5)]


.. function:: methodcaller(name[, args...])

   Return a callable object that calls the method *name* on its operand.  If
   additional arguments and/or keyword arguments are given, they will be given
   to the method as well.  After ``f = methodcaller('name')``, the call ``f(b)``
   returns ``b.name()``.  After ``f = methodcaller('name', 'foo', bar=1)``, the
   call ``f(b)`` returns ``b.name('foo', bar=1)``.

   .. versionadded:: 2.6


.. _operator-map:

Mapping Operators to Functions
------------------------------

This table shows how abstract operations correspond to operator symbols in the
Python syntax and the functions in the :mod:`operator` module.

+-----------------------+-------------------------+---------------------------------+
| Operation             | Syntax                  | Function                        |
+=======================+=========================+=================================+
| Addition              | ``a + b``               | ``add(a, b)``                   |
+-----------------------+-------------------------+---------------------------------+
| Concatenation         | ``seq1 + seq2``         | ``concat(seq1, seq2)``          |
+-----------------------+-------------------------+---------------------------------+
| Containment Test      | ``obj in seq``          | ``contains(seq, obj)``          |
+-----------------------+-------------------------+---------------------------------+
| Division              | ``a / b``               | ``div(a, b)`` (without          |
|                       |                         | ``__future__.division``)        |
+-----------------------+-------------------------+---------------------------------+
| Division              | ``a / b``               | ``truediv(a, b)`` (with         |
|                       |                         | ``__future__.division``)        |
+-----------------------+-------------------------+---------------------------------+
| Division              | ``a // b``              | ``floordiv(a, b)``              |
+-----------------------+-------------------------+---------------------------------+
| Bitwise And           | ``a & b``               | ``and_(a, b)``                  |
+-----------------------+-------------------------+---------------------------------+
| Bitwise Exclusive Or  | ``a ^ b``               | ``xor(a, b)``                   |
+-----------------------+-------------------------+---------------------------------+
| Bitwise Inversion     | ``~ a``                 | ``invert(a)``                   |
+-----------------------+-------------------------+---------------------------------+
| Bitwise Or            | ``a | b``               | ``or_(a, b)``                   |
+-----------------------+-------------------------+---------------------------------+
| Exponentiation        | ``a ** b``              | ``pow(a, b)``                   |
+-----------------------+-------------------------+---------------------------------+
| Identity              | ``a is b``              | ``is_(a, b)``                   |
+-----------------------+-------------------------+---------------------------------+
| Identity              | ``a is not b``          | ``is_not(a, b)``                |
+-----------------------+-------------------------+---------------------------------+
| Indexed Assignment    | ``obj[k] = v``          | ``setitem(obj, k, v)``          |
+-----------------------+-------------------------+---------------------------------+
| Indexed Deletion      | ``del obj[k]``          | ``delitem(obj, k)``             |
+-----------------------+-------------------------+---------------------------------+
| Indexing              | ``obj[k]``              | ``getitem(obj, k)``             |
+-----------------------+-------------------------+---------------------------------+
| Left Shift            | ``a << b``              | ``lshift(a, b)``                |
+-----------------------+-------------------------+---------------------------------+
| Modulo                | ``a % b``               | ``mod(a, b)``                   |
+-----------------------+-------------------------+---------------------------------+
| Multiplication        | ``a * b``               | ``mul(a, b)``                   |
+-----------------------+-------------------------+---------------------------------+
| Negation (Arithmetic) | ``- a``                 | ``neg(a)``                      |
+-----------------------+-------------------------+---------------------------------+
| Negation (Logical)    | ``not a``               | ``not_(a)``                     |
+-----------------------+-------------------------+---------------------------------+
| Right Shift           | ``a >> b``              | ``rshift(a, b)``                |
+-----------------------+-------------------------+---------------------------------+
| Sequence Repitition   | ``seq * i``             | ``repeat(seq, i)``              |
+-----------------------+-------------------------+---------------------------------+
| Slice Assignment      | ``seq[i:j] = values``   | ``setslice(seq, i, j, values)`` |
+-----------------------+-------------------------+---------------------------------+
| Slice Deletion        | ``del seq[i:j]``        | ``delslice(seq, i, j)``         |
+-----------------------+-------------------------+---------------------------------+
| Slicing               | ``seq[i:j]``            | ``getslice(seq, i, j)``         |
+-----------------------+-------------------------+---------------------------------+
| String Formatting     | ``s % obj``             | ``mod(s, obj)``                 |
+-----------------------+-------------------------+---------------------------------+
| Subtraction           | ``a - b``               | ``sub(a, b)``                   |
+-----------------------+-------------------------+---------------------------------+
| Truth Test            | ``obj``                 | ``truth(obj)``                  |
+-----------------------+-------------------------+---------------------------------+
| Ordering              | ``a < b``               | ``lt(a, b)``                    |
+-----------------------+-------------------------+---------------------------------+
| Ordering              | ``a <= b``              | ``le(a, b)``                    |
+-----------------------+-------------------------+---------------------------------+
| Equality              | ``a == b``              | ``eq(a, b)``                    |
+-----------------------+-------------------------+---------------------------------+
| Difference            | ``a != b``              | ``ne(a, b)``                    |
+-----------------------+-------------------------+---------------------------------+
| Ordering              | ``a >= b``              | ``ge(a, b)``                    |
+-----------------------+-------------------------+---------------------------------+
| Ordering              | ``a > b``               | ``gt(a, b)``                    |
+-----------------------+-------------------------+---------------------------------+

