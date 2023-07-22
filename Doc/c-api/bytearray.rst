.. highlight:: c

.. _bytearrayobjects:

Byte Array Objects
------------------

.. index:: pair: object; bytearray


.. c:type:: PyByteArrayObject

   This subtype of :c:type:`PyObject` represents a Python bytearray object.


.. c:var:: PyTypeObject PyByteArray_Type

   This instance of :c:type:`PyTypeObject` represents the Python bytearray type;
   it is the same object as :class:`bytearray` in the Python layer.


Type check macros
^^^^^^^^^^^^^^^^^

.. c:function:: int PyByteArray_Check(PyObject *o)

   Return true if the object *o* is a bytearray object or an instance of a
   subtype of the bytearray type.  This function always succeeds.


.. c:function:: int PyByteArray_CheckExact(PyObject *o)

   Return true if the object *o* is a bytearray object, but not an instance of a
   subtype of the bytearray type.  This function always succeeds.


Direct API functions
^^^^^^^^^^^^^^^^^^^^

.. c:function:: PyObject* PyByteArray_FromObject(PyObject *o)

   Return a new bytearray object from any object, *o*, that implements the
   :ref:`buffer protocol <bufferobjects>`.


.. c:function:: PyObject* PyByteArray_FromStringAndSize(const char *string, Py_ssize_t len)

   Create a new bytearray object from *string* and its length, *len*.  On
   failure, ``NULL`` is returned.


.. c:function:: PyObject* PyByteArray_Concat(PyObject *a, PyObject *b)

   Concat bytearrays *a* and *b* and return a new bytearray with the result.


.. c:function:: Py_ssize_t PyByteArray_Size(PyObject *bytearray)

   Return the size of *bytearray* after checking for a ``NULL`` pointer.


.. c:function:: char* PyByteArray_AsString(PyObject *bytearray)

   Return the contents of *bytearray* as a char array after checking for a
   ``NULL`` pointer.  The returned array always has an extra
   null byte appended.


.. c:function:: int PyByteArray_Resize(PyObject *bytearray, Py_ssize_t len)

   Resize the internal buffer of *bytearray* to *len*.

Macros
^^^^^^

These macros trade safety for speed and they don't check pointers.

.. c:function:: char* PyByteArray_AS_STRING(PyObject *bytearray)

   Similar to :c:func:`PyByteArray_AsString`, but without error checking.


.. c:function:: Py_ssize_t PyByteArray_GET_SIZE(PyObject *bytearray)

   Similar to :c:func:`PyByteArray_Size`, but without error checking.
