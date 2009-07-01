:mod:`contextlib` --- Utilities for :keyword:`with`\ -statement contexts
========================================================================

.. module:: contextlib
   :synopsis: Utilities for with-statement contexts.


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
          print("<%s>" % name)
          yield
          print("</%s>" % name)

      >>> with tag("h1"):
      ...    print("foo")
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
      from urllib.request import urlopen

      with closing(urlopen('http://www.python.org')) as page:
          for line in page:
              print(line)

   without needing to explicitly close ``page``.  Even if an error occurs,
   ``page.close()`` will be called when the :keyword:`with` block is exited.


.. seealso::

   :pep:`0343` - The "with" statement
      The specification, background, and examples for the Python :keyword:`with`
      statement.

