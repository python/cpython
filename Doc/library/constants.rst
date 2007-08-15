
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


.. data:: NotImplemented

   Special value which can be returned by the "rich comparison" special methods
   (:meth:`__eq__`, :meth:`__lt__`, and friends), to indicate that the comparison
   is not implemented with respect to the other type.


.. data:: Ellipsis

   The same as ``...``. Special value used mostly in conjunction with extended
   slicing syntax for user-defined container data types.

   .. % XXX Someone who understands extended slicing should fill in here.

