.. highlightlang:: c

.. _abstract-buffer:

Buffer Protocol
===============


.. cfunction:: int PyObject_AsCharBuffer(PyObject *obj, const char **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a read-only memory location usable as character-based
   input.  The *obj* argument must support the single-segment character buffer
   interface.  On success, returns ``0``, sets *buffer* to the memory location and
   *buffer_len* to the buffer length.  Returns ``-1`` and sets a :exc:`TypeError`
   on error.

   .. versionadded:: 1.6


.. cfunction:: int PyObject_AsReadBuffer(PyObject *obj, const void **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a read-only memory location containing arbitrary data.  The
   *obj* argument must support the single-segment readable buffer interface.  On
   success, returns ``0``, sets *buffer* to the memory location and *buffer_len* to
   the buffer length.  Returns ``-1`` and sets a :exc:`TypeError` on error.

   .. versionadded:: 1.6


.. cfunction:: int PyObject_CheckReadBuffer(PyObject *o)

   Returns ``1`` if *o* supports the single-segment readable buffer interface.
   Otherwise returns ``0``.

   .. versionadded:: 2.2


.. cfunction:: int PyObject_AsWriteBuffer(PyObject *obj, void **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a writeable memory location.  The *obj* argument must
   support the single-segment, character buffer interface.  On success, returns
   ``0``, sets *buffer* to the memory location and *buffer_len* to the buffer
   length.  Returns ``-1`` and sets a :exc:`TypeError` on error.

   .. versionadded:: 1.6

