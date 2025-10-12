.. currentmodule:: asyncio


.. _asyncio-exceptions:

==========
Exceptions
==========

**Source code:** :source:`Lib/asyncio/exceptions.py`

----------------------------------------------------

.. exception:: TimeoutError

   A deprecated alias of :exc:`TimeoutError`,
   raised when the operation has exceeded the given deadline.

   .. versionchanged:: 3.11

      This class was made an alias of :exc:`TimeoutError`.


.. exception:: CancelledError

   The operation has been cancelled.

   This exception can be caught to perform custom operations
   when asyncio Tasks are cancelled.  In almost all situations the
   exception must be re-raised.

   .. versionchanged:: 3.8

      :exc:`CancelledError` is now a subclass of :class:`BaseException` rather than :class:`Exception`.


.. exception:: InvalidStateError

   Invalid internal state of :class:`Task` or :class:`Future`.

   Can be raised in situations like setting a result value for a
   *Future* object that already has a result value set.


.. exception:: SendfileNotAvailableError

   The "sendfile" syscall is not available for the given
   socket or file type.

   A subclass of :exc:`RuntimeError`.


.. exception:: IncompleteReadError

   The requested read operation did not complete fully.

   Raised by the :ref:`asyncio stream APIs<asyncio-streams>`.

   This exception is a subclass of :exc:`EOFError`.

   .. attribute:: expected

      The total number (:class:`int`) of expected bytes.

   .. attribute:: partial

      A string of :class:`bytes` read before the end of stream was reached.


.. exception:: LimitOverrunError

   Reached the buffer size limit while looking for a separator.

   Raised by the :ref:`asyncio stream APIs <asyncio-streams>`.

   .. attribute:: consumed

      The total number of to be consumed bytes.
