.. highlightlang:: c

Old Buffer Protocol
-------------------

.. deprecated:: 3.0

These functions were part of the "old buffer protocol" API in Python 2.
In Python 3, this protocol doesn't exist anymore but the functions are still
exposed to ease porting 2.x code.  They act as a compatibility wrapper
around the :ref:`new buffer protocol <bufferobjects>`, but they don't give
you control over the lifetime of the resources acquired when a buffer is
exported.

Therefore, it is recommended that you call :c:func:`PyObject_GetBuffer`
(or the ``y*`` or ``w*`` :ref:`format codes <arg-parsing>` with the
:c:func:`PyArg_ParseTuple` family of functions) to get a buffer view over
an object, and :c:func:`PyBuffer_Release` when the buffer view can be released.


.. c:function:: int PyObject_AsCharBuffer(PyObject *obj, const char **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a read-only memory location usable as character-based
   input.  The *obj* argument must support the single-segment character buffer
   interface.  On success, returns ``0``, sets *buffer* to the memory location
   and *buffer_len* to the buffer length.  Returns ``-1`` and sets a
   :exc:`TypeError` on error.


.. c:function:: int PyObject_AsReadBuffer(PyObject *obj, const void **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a read-only memory location containing arbitrary data.
   The *obj* argument must support the single-segment readable buffer
   interface.  On success, returns ``0``, sets *buffer* to the memory location
   and *buffer_len* to the buffer length.  Returns ``-1`` and sets a
   :exc:`TypeError` on error.


.. c:function:: int PyObject_CheckReadBuffer(PyObject *o)

   Returns ``1`` if *o* supports the single-segment readable buffer interface.
   Otherwise returns ``0``.  This function always succeeds.

   Note that this function tries to get and release a buffer, and exceptions
   which occur while calling correspoding functions will get suppressed.
   To get error reporting use :c:func:`PyObject_GetBuffer()` instead.


.. c:function:: int PyObject_AsWriteBuffer(PyObject *obj, void **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a writable memory location.  The *obj* argument must
   support the single-segment, character buffer interface.  On success,
   returns ``0``, sets *buffer* to the memory location and *buffer_len* to the
   buffer length.  Returns ``-1`` and sets a :exc:`TypeError` on error.

