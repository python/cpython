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


.. data:: __debug__

   This constant is true if Python was not started with an :option:`-O` option.
   See also the :keyword:`assert` statement.


.. note::

   The names :data:`None` and :data:`__debug__` cannot be reassigned
   (assignments to them, even as an attribute name, raise :exc:`SyntaxError`),
   so they can be considered "true" constants.

   .. versionchanged:: 2.7
      Assignments to ``__debug__`` as an attribute became illegal.


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
   specified exit code.

.. data:: copyright
          license
          credits

   Objects that when printed, print a message like "Type license() to see the
   full license text", and when called, display the corresponding text in a
   pager-like fashion (one screen at a time).
