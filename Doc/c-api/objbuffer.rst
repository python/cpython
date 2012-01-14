.. highlightlang:: c

.. _abstract-buffer:


Old Buffer Protocol
===================

This section describes the legacy buffer protocol, which has been introduced
in Python 1.6. It is still supported but deprecated in the Python 2.x series.
Python 3.0 introduces a new buffer protocol which fixes weaknesses and
shortcomings of the protocol, and has been backported to Python 2.6.  See
:ref:`bufferobjects` for more information.


.. c:function:: int PyObject_AsCharBuffer(PyObject *obj, const char **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a read-only memory location usable as character-based
   input.  The *obj* argument must support the single-segment character buffer
   interface.  On success, returns ``0``, sets *buffer* to the memory location
   and *buffer_len* to the buffer length.  Returns ``-1`` and sets a
   :exc:`TypeError` on error.

   .. versionadded:: 1.6

   .. versionchanged:: 2.5
      This function used an :c:type:`int *` type for *buffer_len*. This might
      require changes in your code for properly supporting 64-bit systems.


.. c:function:: int PyObject_AsReadBuffer(PyObject *obj, const void **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a read-only memory location containing arbitrary data.
   The *obj* argument must support the single-segment readable buffer
   interface.  On success, returns ``0``, sets *buffer* to the memory location
   and *buffer_len* to the buffer length.  Returns ``-1`` and sets a
   :exc:`TypeError` on error.

   .. versionadded:: 1.6

   .. versionchanged:: 2.5
      This function used an :c:type:`int *` type for *buffer_len*. This might
      require changes in your code for properly supporting 64-bit systems.


.. c:function:: int PyObject_CheckReadBuffer(PyObject *o)

   Returns ``1`` if *o* supports the single-segment readable buffer interface.
   Otherwise returns ``0``.

   .. versionadded:: 2.2


.. c:function:: int PyObject_AsWriteBuffer(PyObject *obj, void **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a writeable memory location.  The *obj* argument must
   support the single-segment, character buffer interface.  On success,
   returns ``0``, sets *buffer* to the memory location and *buffer_len* to the
   buffer length.  Returns ``-1`` and sets a :exc:`TypeError` on error.

   .. versionadded:: 1.6

   .. versionchanged:: 2.5
      This function used an :c:type:`int *` type for *buffer_len*. This might
      require changes in your code for properly supporting 64-bit systems.

