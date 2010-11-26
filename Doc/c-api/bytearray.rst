.. highlightlang:: c

.. _bytearrayobjects:

Byte Array Objects
------------------

.. index:: object: bytearray


.. ctype:: PyByteArrayObject

   This subtype of :ctype:`PyObject` represents a Python bytearray object.


.. cvar:: PyTypeObject PyByteArray_Type

   This instance of :ctype:`PyTypeObject` represents the Python bytearray type;
   it is the same object as :class:`bytearray` in the Python layer.


Type check macros
^^^^^^^^^^^^^^^^^

.. cfunction:: int PyByteArray_Check(PyObject *o)

   Return true if the object *o* is a bytearray object or an instance of a
   subtype of the bytearray type.


.. cfunction:: int PyByteArray_CheckExact(PyObject *o)

   Return true if the object *o* is a bytearray object, but not an instance of a
   subtype of the bytearray type.


Direct API functions
^^^^^^^^^^^^^^^^^^^^

.. cfunction:: PyObject* PyByteArray_FromObject(PyObject *o)

   Return a new bytearray object from any object, *o*, that implements the
   buffer protocol.

   .. XXX expand about the buffer protocol, at least somewhere


.. cfunction:: PyObject* PyByteArray_FromStringAndSize(const char *string, Py_ssize_t len)

   Create a new bytearray object from *string* and its length, *len*.  On
   failure, *NULL* is returned.


.. cfunction:: PyObject* PyByteArray_Concat(PyObject *a, PyObject *b)

   Concat bytearrays *a* and *b* and return a new bytearray with the result.


.. cfunction:: Py_ssize_t PyByteArray_Size(PyObject *bytearray)

   Return the size of *bytearray* after checking for a *NULL* pointer.


.. cfunction:: char* PyByteArray_AsString(PyObject *bytearray)

   Return the contents of *bytearray* as a char array after checking for a
   *NULL* pointer.


.. cfunction:: int PyByteArray_Resize(PyObject *bytearray, Py_ssize_t len)

   Resize the internal buffer of *bytearray* to *len*.

Macros
^^^^^^

These macros trade safety for speed and they don't check pointers.

.. cfunction:: char* PyByteArray_AS_STRING(PyObject *bytearray)

   Macro version of :cfunc:`PyByteArray_AsString`.


.. cfunction:: Py_ssize_t PyByteArray_GET_SIZE(PyObject *bytearray)

   Macro version of :cfunc:`PyByteArray_Size`.
