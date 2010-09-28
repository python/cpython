.. highlightlang:: c

Old buffer API
--------------

.. deprecated:: 3.0

These functions were part of the "old buffer protocol" API in Python 2.
In Python 3, these functions are still exposed for ease of porting code.
They act as a compatibility wrapper around the :ref:`new buffer API
<bufferobjects>`, but they don't give you control over the lifetime of
the resources acquired when a buffer is exported.

Therefore, it is recommended that you call :cfunc:`PyObject_GetBuffer`
(or the ``y*`` or ``w*`` :ref:`format codes <arg-parsing>` with the
:cfunc:`PyArg_ParseTuple` family of functions) to get a buffer view over
an object, and :cfunc:`PyBuffer_Release` when the buffer view can be released.


Buffer Protocol
===============


.. cfunction:: int PyObject_AsCharBuffer(PyObject *obj, const char **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a read-only memory location usable as character-based
   input.  The *obj* argument must support the single-segment character buffer
   interface.  On success, returns ``0``, sets *buffer* to the memory location
   and *buffer_len* to the buffer length.  Returns ``-1`` and sets a
   :exc:`TypeError` on error.


.. cfunction:: int PyObject_AsReadBuffer(PyObject *obj, const void **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a read-only memory location containing arbitrary data.
   The *obj* argument must support the single-segment readable buffer
   interface.  On success, returns ``0``, sets *buffer* to the memory location
   and *buffer_len* to the buffer length.  Returns ``-1`` and sets a
   :exc:`TypeError` on error.


.. cfunction:: int PyObject_CheckReadBuffer(PyObject *o)

   Returns ``1`` if *o* supports the single-segment readable buffer interface.
   Otherwise returns ``0``.


.. cfunction:: int PyObject_AsWriteBuffer(PyObject *obj, void **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a writable memory location.  The *obj* argument must
   support the single-segment, character buffer interface.  On success,
   returns ``0``, sets *buffer* to the memory location and *buffer_len* to the
   buffer length.  Returns ``-1`` and sets a :exc:`TypeError` on error.

