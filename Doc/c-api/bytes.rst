.. highlightlang:: c

.. _bytesobjects:

Bytes Objects
-------------

These functions raise :exc:`TypeError` when expecting a bytes parameter and are
called with a non-bytes parameter.

.. index:: object: bytes


.. c:type:: PyBytesObject

   This subtype of :c:type:`PyObject` represents a Python bytes object.


.. c:var:: PyTypeObject PyBytes_Type

   This instance of :c:type:`PyTypeObject` represents the Python bytes type; it
   is the same object as :class:`bytes` in the Python layer.


.. c:function:: int PyBytes_Check(PyObject *o)

   Return true if the object *o* is a bytes object or an instance of a subtype
   of the bytes type.


.. c:function:: int PyBytes_CheckExact(PyObject *o)

   Return true if the object *o* is a bytes object, but not an instance of a
   subtype of the bytes type.


.. c:function:: PyObject* PyBytes_FromString(const char *v)

   Return a new bytes object with a copy of the string *v* as value on success,
   and *NULL* on failure.  The parameter *v* must not be *NULL*; it will not be
   checked.


.. c:function:: PyObject* PyBytes_FromStringAndSize(const char *v, Py_ssize_t len)

   Return a new bytes object with a copy of the string *v* as value and length
   *len* on success, and *NULL* on failure.  If *v* is *NULL*, the contents of
   the bytes object are uninitialized.


.. c:function:: PyObject* PyBytes_FromFormat(const char *format, ...)

   Take a C :c:func:`printf`\ -style *format* string and a variable number of
   arguments, calculate the size of the resulting Python bytes object and return
   a bytes object with the values formatted into it.  The variable arguments
   must be C types and must correspond exactly to the format characters in the
   *format* string.  The following format characters are allowed:

   .. % XXX: This should be exactly the same as the table in PyErr_Format.
   .. % One should just refer to the other.
   .. % XXX: The descriptions for %zd and %zu are wrong, but the truth is complicated
   .. % because not all compilers support the %z width modifier -- we fake it
   .. % when necessary via interpolating PY_FORMAT_SIZE_T.

   +-------------------+---------------+--------------------------------+
   | Format Characters | Type          | Comment                        |
   +===================+===============+================================+
   | :attr:`%%`        | *n/a*         | The literal % character.       |
   +-------------------+---------------+--------------------------------+
   | :attr:`%c`        | int           | A single character,            |
   |                   |               | represented as an C int.       |
   +-------------------+---------------+--------------------------------+
   | :attr:`%d`        | int           | Exactly equivalent to          |
   |                   |               | ``printf("%d")``.              |
   +-------------------+---------------+--------------------------------+
   | :attr:`%u`        | unsigned int  | Exactly equivalent to          |
   |                   |               | ``printf("%u")``.              |
   +-------------------+---------------+--------------------------------+
   | :attr:`%ld`       | long          | Exactly equivalent to          |
   |                   |               | ``printf("%ld")``.             |
   +-------------------+---------------+--------------------------------+
   | :attr:`%lu`       | unsigned long | Exactly equivalent to          |
   |                   |               | ``printf("%lu")``.             |
   +-------------------+---------------+--------------------------------+
   | :attr:`%zd`       | Py_ssize_t    | Exactly equivalent to          |
   |                   |               | ``printf("%zd")``.             |
   +-------------------+---------------+--------------------------------+
   | :attr:`%zu`       | size_t        | Exactly equivalent to          |
   |                   |               | ``printf("%zu")``.             |
   +-------------------+---------------+--------------------------------+
   | :attr:`%i`        | int           | Exactly equivalent to          |
   |                   |               | ``printf("%i")``.              |
   +-------------------+---------------+--------------------------------+
   | :attr:`%x`        | int           | Exactly equivalent to          |
   |                   |               | ``printf("%x")``.              |
   +-------------------+---------------+--------------------------------+
   | :attr:`%s`        | char\*        | A null-terminated C character  |
   |                   |               | array.                         |
   +-------------------+---------------+--------------------------------+
   | :attr:`%p`        | void\*        | The hex representation of a C  |
   |                   |               | pointer. Mostly equivalent to  |
   |                   |               | ``printf("%p")`` except that   |
   |                   |               | it is guaranteed to start with |
   |                   |               | the literal ``0x`` regardless  |
   |                   |               | of what the platform's         |
   |                   |               | ``printf`` yields.             |
   +-------------------+---------------+--------------------------------+

   An unrecognized format character causes all the rest of the format string to be
   copied as-is to the result string, and any extra arguments discarded.


.. c:function:: PyObject* PyBytes_FromFormatV(const char *format, va_list vargs)

   Identical to :c:func:`PyBytes_FromFormat` except that it takes exactly two
   arguments.


.. c:function:: PyObject* PyBytes_FromObject(PyObject *o)

   Return the bytes representation of object *o* that implements the buffer
   protocol.


.. c:function:: Py_ssize_t PyBytes_Size(PyObject *o)

   Return the length of the bytes in bytes object *o*.


.. c:function:: Py_ssize_t PyBytes_GET_SIZE(PyObject *o)

   Macro form of :c:func:`PyBytes_Size` but without error checking.


.. c:function:: char* PyBytes_AsString(PyObject *o)

   Return a NUL-terminated representation of the contents of *o*.  The pointer
   refers to the internal buffer of *o*, not a copy.  The data must not be
   modified in any way, unless the string was just created using
   ``PyBytes_FromStringAndSize(NULL, size)``. It must not be deallocated.  If
   *o* is not a string object at all, :c:func:`PyBytes_AsString` returns *NULL*
   and raises :exc:`TypeError`.


.. c:function:: char* PyBytes_AS_STRING(PyObject *string)

   Macro form of :c:func:`PyBytes_AsString` but without error checking.


.. c:function:: int PyBytes_AsStringAndSize(PyObject *obj, char **buffer, Py_ssize_t *length)

   Return a NUL-terminated representation of the contents of the object *obj*
   through the output variables *buffer* and *length*.

   If *length* is *NULL*, the resulting buffer may not contain NUL characters;
   if it does, the function returns ``-1`` and a :exc:`TypeError` is raised.

   The buffer refers to an internal string buffer of *obj*, not a copy. The data
   must not be modified in any way, unless the string was just created using
   ``PyBytes_FromStringAndSize(NULL, size)``.  It must not be deallocated.  If
   *string* is not a string object at all, :c:func:`PyBytes_AsStringAndSize`
   returns ``-1`` and raises :exc:`TypeError`.


.. c:function:: void PyBytes_Concat(PyObject **bytes, PyObject *newpart)

   Create a new bytes object in *\*bytes* containing the contents of *newpart*
   appended to *bytes*; the caller will own the new reference.  The reference to
   the old value of *bytes* will be stolen.  If the new string cannot be
   created, the old reference to *bytes* will still be discarded and the value
   of *\*bytes* will be set to *NULL*; the appropriate exception will be set.


.. c:function:: void PyBytes_ConcatAndDel(PyObject **bytes, PyObject *newpart)

   Create a new string object in *\*bytes* containing the contents of *newpart*
   appended to *bytes*.  This version decrements the reference count of
   *newpart*.


.. c:function:: int _PyBytes_Resize(PyObject **bytes, Py_ssize_t newsize)

   A way to resize a bytes object even though it is "immutable". Only use this
   to build up a brand new bytes object; don't use this if the bytes may already
   be known in other parts of the code.  It is an error to call this function if
   the refcount on the input bytes object is not one. Pass the address of an
   existing bytes object as an lvalue (it may be written into), and the new size
   desired.  On success, *\*bytes* holds the resized bytes object and ``0`` is
   returned; the address in *\*bytes* may differ from its input value.  If the
   reallocation fails, the original bytes object at *\*bytes* is deallocated,
   *\*bytes* is set to *NULL*, a memory exception is set, and ``-1`` is
   returned.
