
Built-in Constants
==================

A small number of constants live in the built-in namespace.  They are:


.. data:: False

   The false value of the :class:`bool` type.

   .. versionadded:: 2.3


.. data:: True

   The true value of the :class:`bool` type.

   .. versionadded:: 2.3


.. data:: None

   The sole value of :attr:`types.NoneType`.  ``None`` is frequently used to
   represent the absence of a value, as when default arguments are not passed to a
   function.

   .. versionchanged:: 2.4
      Assignments to ``None`` are illegal and raise a :exc:`SyntaxError`.


.. data:: NotImplemented

   Special value which can be returned by the "rich comparison" special methods
   (:meth:`__eq__`, :meth:`__lt__`, and friends), to indicate that the comparison
   is not implemented with respect to the other type.


.. data:: Ellipsis

   Special value used in conjunction with extended slicing syntax.

   .. XXX Someone who understands extended slicing should fill in here.


.. data:: __debug__

   This constant is true if Python was not started with an :option:`-O` option.
   Assignments to :const:`__debug__` are illegal and raise a :exc:`SyntaxError`.
   See also the :keyword:`assert` statement.