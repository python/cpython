.. highlight:: c

.. _iterator:

Iterator Protocol
=================

There are two functions specifically for working with iterators.

.. c:function:: int PyIter_Check(PyObject *o)

   Return non-zero if the object *o* supports the iterator protocol, and ``0``
   otherwise.  This function always succeeds.

.. c:function:: int PyAIter_Check(PyObject *o)

   Returns non-zero if the object 'obj' provides :class:`AsyncIterator`
   protocols, and ``0`` otherwise.  This function always succeeds.

   .. versionadded:: 3.10

.. c:function:: PyObject* PyIter_Next(PyObject *o)

   Return the next value from the iteration *o*.  The object must be an iterator
   (it is up to the caller to check this).  If there are no remaining values,
   returns ``NULL`` with no exception set.  If an error occurs while retrieving
   the item, returns ``NULL`` and passes along the exception.

To write a loop which iterates over an iterator, the C code should look
something like this::

   PyObject *iterator = PyObject_GetIter(obj);
   PyObject *item;

   if (iterator == NULL) {
       /* propagate error */
   }

   while ((item = PyIter_Next(iterator))) {
       /* do something with item */
       ...
       /* release reference when done */
       Py_DECREF(item);
   }

   Py_DECREF(iterator);

   if (PyErr_Occurred()) {
       /* propagate error */
   }
   else {
       /* continue doing useful work */
   }


.. c:type:: PySendResult

   The enum value used to represent different results of :c:func:`PyIter_Send`.

   .. versionadded:: 3.10


.. c:function:: PySendResult PyIter_Send(PyObject *iter, PyObject *arg, PyObject **presult)

   Sends the *arg* value into the iterator *iter*. Returns:

   - ``PYGEN_RETURN`` if iterator returns. Return value is returned via *presult*.
   - ``PYGEN_NEXT`` if iterator yields. Yielded value is returned via *presult*.
   - ``PYGEN_ERROR`` if iterator has raised and exception. *presult* is set to ``NULL``.

   .. versionadded:: 3.10
