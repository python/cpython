
Built-in Constants
==================

A small number of constants live in the built-in namespace.  They are:


.. note::

   :data:`None`, :data:`False`, :data:`True` and :data:`__debug__` cannot be
   reassigned (assignments to them raise :exc:`SyntaxError`), so they can be
   considered "true" constants.

.. XXX False, True, None are keywords too

.. data:: False

   The false value of the :class:`bool` type.


.. data:: True

   The true value of the :class:`bool` type.


.. data:: None

   The sole value of :attr:`types.NoneType`.  ``None`` is frequently used to
   represent the absence of a value, as when default arguments are not passed to a
   function.


.. data:: NotImplemented

   Special value which can be returned by the "rich comparison" special methods
   (:meth:`__eq__`, :meth:`__lt__`, and friends), to indicate that the comparison
   is not implemented with respect to the other type.


.. data:: Ellipsis

   The same as ``...``. Special value used mostly in conjunction with extended
   slicing syntax for user-defined container data types, as in ::

      val = container[1:5, 7:10, ...]


.. data:: __debug__

   A boolean value that is :data:`True` if Python was not started with the
   ``-O`` command line option.  Its value is used indirectly by the
   :keyword:`assert` statement, but it can also be used directly in code.
