.. currentmodule:: asyncio


==========
Exceptions
==========


.. exception:: TimeoutError

   The operation has exceeded the given deadline.

   .. note::
      This exception is different from the builtin :exc:`TimeoutError`
      exception.


.. exception:: CancelledError

   The operation has been cancelled.

   This exception can be caught to perform custom operations
   when asyncio Tasks are cancelled.  In almost all situations the
   exception must always be re-raised.

   .. note::

      This exception is a subclass of :exc:`Exception`, so it can be
      accidentally suppressed by ``try..except`` block::

        try:
            await operation
        except Exception:
            # The cancellation is broken because the *except* block
            # suppresses the CancelledError exception.
            log.log('an error has occurred')

      Instead, the following pattern should be used::

        try:
            await operation
        except asyncio.CancelledError:
            raise
        except Exception:
            log.log('an error has occurred')


.. exception:: InvalidStateError

   Invalid internal state of :class:`Task` or :class:`Future`.

   Can be raised in situations like setting a result value for a
   *Future* object that already has a result value set.


.. exception:: SendfileNotAvailableError

   The "sendfile" syscall is not available for the given
   socket or file type.

   A subclass of :exc:`RuntimeError`.


.. exception:: IncompleteReadError

    Incomplete read error.

    Raised by :ref:`asyncio streams <asyncio-streams>` APIs.

    This exception is a subclass of :exc:`EOFError`.

   .. attribute:: expected

      Total number (:class:`int`) of expected bytes.

   .. attribute:: partial

      Read :class:`bytes` string before the end of stream was reached.


.. exception:: LimitOverrunError

   Reached the buffer limit while looking for a separator.

   Raised by :ref:`asyncio streams <asyncio-streams>` APIs.

   .. attribute:: consumed

      Total number of to be consumed bytes.
