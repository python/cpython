Built-in Constants
==================

A small number of constants live in the built-in namespace.  They are:


.. note::

   :data:`None`, :data:`False`, :data:`True` and :data:`__debug__` cannot be
   reassigned (assignments to them raise :exc:`SyntaxError`), so they can be
   considered "true" constants.

.. XXX False, True, None are keywords too

.. data:: False

   The false value of the :class:`bool` type. Assignments to ``False`` 
   are illegal and raise a :exc:`SyntaxError`.


.. data:: True

   The true value of the :class:`bool` type. Assignments to ``True`` 
   are illegal and raise a :exc:`SyntaxError`.


.. data:: None

   The sole value of :attr:`types.NoneType`.  ``None`` is frequently used to
   represent the absence of a value, as when default arguments are not passed to a
   function. Assignments to ``None`` are illegal and raise a :exc:`SyntaxError`.


.. data:: NotImplemented

   Special value which can be returned by the "rich comparison" special methods
   (:meth:`__eq__`, :meth:`__lt__`, and friends), to indicate that the comparison
   is not implemented with respect to the other type.


.. data:: Ellipsis

   The same as ``...``. Special value used mostly in conjunction with extended
   slicing syntax for user-defined container data types, as in ::

   .. XXX Someone who understands extended slicing should fill in here.


.. data:: __debug__

   This constant is true if Python was not started with an :option:`-O` option.
   Assignments to :const:`__debug__` are illegal and raise a :exc:`SyntaxError`.
   See also the :keyword:`assert` statement.


Constants added by the :mod:`site` module
-----------------------------------------

The :mod:`site` module (which is imported automatically during startup, except
if the :option:`-S` command-line option is given) adds several constants to the
built-in namespace.  They are useful for the interactive interpreter shell and
should not be used in programs.

.. data:: quit([code=None])
          exit([code=None])

   Objects that when printed, print a message like "Use quit() or Ctrl-D
   (i.e. EOF) to exit", and when called, raise :exc:`SystemExit` with the
   specified exit code, and when .

.. data:: copyright
          license
          credits

   Objects that when printed, print a message like "Type license() to see the
   full license text", and when called, display the corresponding text in a
   pager-like fashion (one screen at a time).

