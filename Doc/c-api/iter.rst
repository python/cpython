.. highlight:: c

.. _iterator:

Iterator Protocol
=================

There are two functions specifically for working with iterators.

.. c:function:: int PyIter_Check(PyObject *o)

   Return non-zero if the object *o* can be safely passed to
   :c:func:`PyIter_NextItem` and ``0`` otherwise.
   This function always succeeds.

.. c:function:: int PyAIter_Check(PyObject *o)

   Return non-zero if the object *o* provides the :class:`AsyncIterator`
   protocol, and ``0`` otherwise.  This function always succeeds.

   .. versionadded:: 3.10

.. c:function:: int PyIter_NextItem(PyObject *iter, PyObject **item)

   Return ``1`` and set *item* to a :term:`strong reference` of the
   next value of the iterator *iter* on success.
   Return ``0`` and set *item* to ``NULL`` if there are no remaining values.
   Return ``-1``, set *item* to ``NULL`` and set an exception on error.

   .. versionadded:: 3.14

.. c:function:: PyObject* PyIter_Next(PyObject *o)

   This is an older version of :c:func:`!PyIter_NextItem`,
   which is retained for backwards compatibility.
   Prefer :c:func:`PyIter_NextItem`.

   Return the next value from the iterator *o*.  The object must be an iterator
   according to :c:func:`PyIter_Check` (it is up to the caller to check this).
   If there are no remaining values, returns ``NULL`` with no exception set.
   If an error occurs while retrieving the item, returns ``NULL`` and passes
   along the exception.

.. c:type:: PySendResult

   The enum value used to represent different results of :c:func:`PyIter_Send`.

   .. versionadded:: 3.10


.. c:function:: PySendResult PyIter_Send(PyObject *iter, PyObject *arg, PyObject **presult)

   Sends the *arg* value into the iterator *iter*. Returns:

   - ``PYGEN_RETURN`` if iterator returns. Return value is returned via *presult*.
   - ``PYGEN_NEXT`` if iterator yields. Yielded value is returned via *presult*.
   - ``PYGEN_ERROR`` if iterator has raised an exception. *presult* is set to ``NULL``.

   .. versionadded:: 3.10
