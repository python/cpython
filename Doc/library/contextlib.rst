:mod:`contextlib` --- Utilities for :keyword:`with`\ -statement contexts
========================================================================

.. module:: contextlib
   :synopsis: Utilities for with-statement contexts.


.. versionadded:: 2.5

**Source code:** :source:`Lib/contextlib.py`

--------------

This module provides utilities for common tasks involving the :keyword:`with`
statement. For more information see also :ref:`typecontextmanager` and
:ref:`context-managers`.

Functions provided:


.. function:: contextmanager(func)

   This function is a :term:`decorator` that can be used to define a factory
   function for :keyword:`with` statement context managers, without needing to
   create a class or separate :meth:`__enter__` and :meth:`__exit__` methods.

   A simple example (this is not recommended as a real way of generating HTML!)::

      from contextlib import contextmanager

      @contextmanager
      def tag(name):
          print "<%s>" % name
          yield
          print "</%s>" % name

      >>> with tag("h1"):
      ...    print "foo"
      ...
      <h1>
      foo
      </h1>

   The function being decorated must return a :term:`generator`-iterator when
   called. This iterator must yield exactly one value, which will be bound to
   the targets in the :keyword:`with` statement's :keyword:`as` clause, if any.

   At the point where the generator yields, the block nested in the :keyword:`with`
   statement is executed.  The generator is then resumed after the block is exited.
   If an unhandled exception occurs in the block, it is reraised inside the
   generator at the point where the yield occurred.  Thus, you can use a
   :keyword:`try`...\ :keyword:`except`...\ :keyword:`finally` statement to trap
   the error (if any), or ensure that some cleanup takes place. If an exception is
   trapped merely in order to log it or to perform some action (rather than to
   suppress it entirely), the generator must reraise that exception. Otherwise the
   generator context manager will indicate to the :keyword:`with` statement that
   the exception has been handled, and execution will resume with the statement
   immediately following the :keyword:`with` statement.


.. function:: nested(mgr1[, mgr2[, ...]])

   Combine multiple context managers into a single nested context manager.

   This function has been deprecated in favour of the multiple manager form
   of the :keyword:`with` statement.

   The one advantage of this function over the multiple manager form of the
   :keyword:`with` statement is that argument unpacking allows it to be
   used with a variable number of context managers as follows::

      from contextlib import nested

      with nested(*managers):
          do_something()

   Note that if the :meth:`__exit__` method of one of the nested context managers
   indicates an exception should be suppressed, no exception information will be
   passed to any remaining outer context managers. Similarly, if the
   :meth:`__exit__` method of one of the nested managers raises an exception, any
   previous exception state will be lost; the new exception will be passed to the
   :meth:`__exit__` methods of any remaining outer context managers. In general,
   :meth:`__exit__` methods should avoid raising exceptions, and in particular they
   should not re-raise a passed-in exception.

   This function has two major quirks that have led to it being deprecated. Firstly,
   as the context managers are all constructed before the function is invoked, the
   :meth:`__new__` and :meth:`__init__` methods of the inner context managers are
   not actually covered by the scope of the outer context managers. That means, for
   example, that using :func:`nested` to open two files is a programming error as the
   first file will not be closed promptly if an exception is thrown when opening
   the second file.

   Secondly, if the :meth:`__enter__` method of one of the inner context managers
   raises an exception that is caught and suppressed by the :meth:`__exit__` method
   of one of the outer context managers, this construct will raise
   :exc:`RuntimeError` rather than skipping the body of the :keyword:`with`
   statement.

   Developers that need to support nesting of a variable number of context managers
   can either use the :mod:`warnings` module to suppress the DeprecationWarning
   raised by this function or else use this function as a model for an application
   specific implementation.

   .. deprecated:: 2.7
      The with-statement now supports this functionality directly (without the
      confusing error prone quirks).

.. function:: closing(thing)

   Return a context manager that closes *thing* upon completion of the block.  This
   is basically equivalent to::

      from contextlib import contextmanager

      @contextmanager
      def closing(thing):
          try:
              yield thing
          finally:
              thing.close()

   And lets you write code like this::

      from contextlib import closing
      import urllib

      with closing(urllib.urlopen('http://www.python.org')) as page:
          for line in page:
              print line

   without needing to explicitly close ``page``.  Even if an error occurs,
   ``page.close()`` will be called when the :keyword:`with` block is exited.


.. seealso::

   :pep:`343` - The "with" statement
      The specification, background, and examples for the Python :keyword:`with`
      statement.

