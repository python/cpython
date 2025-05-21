.. _built-in-consts:

Built-in Constants
==================

A small number of constants live in the built-in namespace.  They are:

.. data:: False

   The false value of the :class:`bool` type. Assignments to ``False``
   are illegal and raise a :exc:`SyntaxError`.


.. data:: True

   The true value of the :class:`bool` type. Assignments to ``True``
   are illegal and raise a :exc:`SyntaxError`.


.. data:: None

   An object frequently used to represent the absence of a value, as when
   default arguments are not passed to a function. Assignments to ``None``
   are illegal and raise a :exc:`SyntaxError`.
   ``None`` is the sole instance of the :data:`~types.NoneType` type.


.. data:: NotImplemented

   A special value which should be returned by the binary special methods
   (e.g. :meth:`~object.__eq__`, :meth:`~object.__lt__`, :meth:`~object.__add__`, :meth:`~object.__rsub__`,
   etc.) to indicate that the operation is not implemented with respect to
   the other type; may be returned by the in-place binary special methods
   (e.g. :meth:`~object.__imul__`, :meth:`~object.__iand__`, etc.) for the same purpose.
   It should not be evaluated in a boolean context.
   :data:`!NotImplemented` is the sole instance of the :data:`types.NotImplementedType` type.

   .. note::

      When a binary (or in-place) method returns :data:`!NotImplemented` the
      interpreter will try the reflected operation on the other type (or some
      other fallback, depending on the operator).  If all attempts return
      :data:`!NotImplemented`, the interpreter will raise an appropriate exception.
      Incorrectly returning :data:`!NotImplemented` will result in a misleading
      error message or the :data:`!NotImplemented` value being returned to Python code.

      See :ref:`implementing-the-arithmetic-operations` for examples.

   .. caution::

      :data:`!NotImplemented` and :exc:`!NotImplementedError` are not
      interchangeable. This constant should only be used as described
      above; see :exc:`NotImplementedError` for details on correct usage
      of the exception.

   .. versionchanged:: 3.9
      Evaluating :data:`!NotImplemented` in a boolean context was deprecated.

   .. versionchanged:: 3.14
      Evaluating :data:`!NotImplemented` in a boolean context now raises a :exc:`TypeError`.
      It previously evaluated to :const:`True` and emitted a :exc:`DeprecationWarning`
      since Python 3.9.


.. index:: single: ...; ellipsis literal
.. data:: Ellipsis

   The same as the ellipsis literal "``...``". Special value used mostly in conjunction
   with extended slicing syntax for user-defined container data types.
   ``Ellipsis`` is the sole instance of the :data:`types.EllipsisType` type.


.. data:: __debug__

   This constant is true if Python was not started with an :option:`-O` option.
   See also the :keyword:`assert` statement.


.. note::

   The names :data:`None`, :data:`False`, :data:`True` and :data:`__debug__`
   cannot be reassigned (assignments to them, even as an attribute name, raise
   :exc:`SyntaxError`), so they can be considered "true" constants.


.. _site-consts:

Constants added by the :mod:`site` module
-----------------------------------------

The :mod:`site` module (which is imported automatically during startup, except
if the :option:`-S` command-line option is given) adds several constants to the
built-in namespace.  They are useful for the interactive interpreter shell and
should not be used in programs.

.. data:: quit(code=None)
          exit(code=None)

   Objects that when printed, print a message like "Use quit() or Ctrl-D
   (i.e. EOF) to exit", and when called, raise :exc:`SystemExit` with the
   specified exit code.

.. data:: help
   :noindex:

   Object that when printed, prints the message "Type help() for interactive
   help, or help(object) for help about object.", and when called,
   acts as described :func:`elsewhere <help>`.

.. data:: copyright
          credits

   Objects that when printed or called, print the text of copyright or
   credits, respectively.

.. data:: license

   Object that when printed, prints the message "Type license() to see the
   full license text", and when called, displays the full license text in a
   pager-like fashion (one screen at a time).
