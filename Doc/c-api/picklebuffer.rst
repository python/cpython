.. highlight:: c

.. _picklebuffer-objects:

.. index::
   pair: object; PickleBuffer

Pickle buffer objects
---------------------

.. versionadded:: 3.8

A :class:`pickle.PickleBuffer` object wraps a :ref:`buffer-providing object
<bufferobjects>` for out-of-band data transfer with the :mod:`pickle` module.


.. c:var:: PyTypeObject PyPickleBuffer_Type

   This instance of :c:type:`PyTypeObject` represents the Python pickle buffer type.
   This is the same object as :class:`pickle.PickleBuffer` in the Python layer.


.. c:function:: int PyPickleBuffer_Check(PyObject *op)

   Return true if *op* is a pickle buffer instance.
   This function always succeeds.


.. c:function:: PyObject *PyPickleBuffer_FromObject(PyObject *obj)

   Create a pickle buffer from the object *obj*.

   This function will fail if *obj* doesn't support the :ref:`buffer protocol <bufferobjects>`.

   On success, return a new pickle buffer instance.
   On failure, set an exception and return ``NULL``.

   Analogous to calling :class:`pickle.PickleBuffer` with *obj* in Python.


.. c:function:: const Py_buffer *PyPickleBuffer_GetBuffer(PyObject *picklebuf)

   Get a pointer to the underlying :c:type:`Py_buffer` that the pickle buffer wraps.

   The returned pointer is valid as long as *picklebuf* is alive and has not been
   released. The caller must not modify or free the returned :c:type:`Py_buffer`.
   If the pickle buffer has been released, raise :exc:`ValueError`.

   On success, return a pointer to the buffer view.
   On failure, set an exception and return ``NULL``.


.. c:function:: int PyPickleBuffer_Release(PyObject *picklebuf)

   Release the underlying buffer held by the pickle buffer.

   Return ``0`` on success. On failure, set an exception and return ``-1``.

   Analogous to calling :meth:`pickle.PickleBuffer.release` in Python.
