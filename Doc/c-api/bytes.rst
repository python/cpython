.. highlight:: c

.. _bytesobjects:

Bytes Objects
-------------

These functions raise :exc:`TypeError` when expecting a bytes parameter and
called with a non-bytes parameter.

.. index:: pair: object; bytes


.. c:type:: PyBytesObject

   This subtype of :c:type:`PyObject` represents a Python bytes object.


.. c:var:: PyTypeObject PyBytes_Type

   This instance of :c:type:`PyTypeObject` represents the Python bytes type; it
   is the same object as :class:`bytes` in the Python layer.


.. c:function:: int PyBytes_Check(PyObject *o)

   Return true if the object *o* is a bytes object or an instance of a subtype
   of the bytes type.  This function always succeeds.


.. c:function:: int PyBytes_CheckExact(PyObject *o)

   Return true if the object *o* is a bytes object, but not an instance of a
   subtype of the bytes type.  This function always succeeds.


.. c:function:: PyObject* PyBytes_FromString(const char *v)

   Return a new bytes object with a copy of the string *v* as value on success,
   and ``NULL`` on failure.  The parameter *v* must not be ``NULL``; it will not be
   checked.


.. c:function:: PyObject* PyBytes_FromStringAndSize(const char *v, Py_ssize_t len)

   Return a new bytes object with a copy of the string *v* as value and length
   *len* on success, and ``NULL`` on failure.  If *v* is ``NULL``, the contents of
   the bytes object are uninitialized.

   .. deprecated:: 3.15
      ``PyBytes_FromStringAndSize(NULL, len)`` is :term:`soft deprecated`,
      use the :c:type:`PyBytesWriter` API instead.


.. c:function:: PyObject* PyBytes_FromFormat(const char *format, ...)

   Take a C :c:func:`printf`\ -style *format* string and a variable number of
   arguments, calculate the size of the resulting Python bytes object and return
   a bytes object with the values formatted into it.  The variable arguments
   must be C types and must correspond exactly to the format characters in the
   *format* string.  The following format characters are allowed:

   .. % XXX: This should be exactly the same as the table in PyErr_Format.
   .. % One should just refer to the other.

   .. tabularcolumns:: |l|l|L|

   +-------------------+---------------+--------------------------------+
   | Format Characters | Type          | Comment                        |
   +===================+===============+================================+
   | ``%%``            | *n/a*         | The literal % character.       |
   +-------------------+---------------+--------------------------------+
   | ``%c``            | int           | A single byte,                 |
   |                   |               | represented as a C int.        |
   +-------------------+---------------+--------------------------------+
   | ``%d``            | int           | Equivalent to                  |
   |                   |               | ``printf("%d")``. [1]_         |
   +-------------------+---------------+--------------------------------+
   | ``%u``            | unsigned int  | Equivalent to                  |
   |                   |               | ``printf("%u")``. [1]_         |
   +-------------------+---------------+--------------------------------+
   | ``%ld``           | long          | Equivalent to                  |
   |                   |               | ``printf("%ld")``. [1]_        |
   +-------------------+---------------+--------------------------------+
   | ``%lu``           | unsigned long | Equivalent to                  |
   |                   |               | ``printf("%lu")``. [1]_        |
   +-------------------+---------------+--------------------------------+
   | ``%zd``           | :c:type:`\    | Equivalent to                  |
   |                   | Py_ssize_t`   | ``printf("%zd")``. [1]_        |
   +-------------------+---------------+--------------------------------+
   | ``%zu``           | size_t        | Equivalent to                  |
   |                   |               | ``printf("%zu")``. [1]_        |
   +-------------------+---------------+--------------------------------+
   | ``%i``            | int           | Equivalent to                  |
   |                   |               | ``printf("%i")``. [1]_         |
   +-------------------+---------------+--------------------------------+
   | ``%x``            | int           | Equivalent to                  |
   |                   |               | ``printf("%x")``. [1]_         |
   +-------------------+---------------+--------------------------------+
   | ``%s``            | const char\*  | A null-terminated C character  |
   |                   |               | array.                         |
   +-------------------+---------------+--------------------------------+
   | ``%p``            | const void\*  | The hex representation of a C  |
   |                   |               | pointer. Mostly equivalent to  |
   |                   |               | ``printf("%p")`` except that   |
   |                   |               | it is guaranteed to start with |
   |                   |               | the literal ``0x`` regardless  |
   |                   |               | of what the platform's         |
   |                   |               | ``printf`` yields.             |
   +-------------------+---------------+--------------------------------+

   An unrecognized format character causes all the rest of the format string to be
   copied as-is to the result object, and any extra arguments discarded.

   .. [1] For integer specifiers (d, u, ld, lu, zd, zu, i, x): the 0-conversion
      flag has effect even when a precision is given.


.. c:function:: PyObject* PyBytes_FromFormatV(const char *format, va_list vargs)

   Identical to :c:func:`PyBytes_FromFormat` except that it takes exactly two
   arguments.


.. c:function:: PyObject* PyBytes_FromObject(PyObject *o)

   Return the bytes representation of object *o* that implements the buffer
   protocol.


.. c:function:: Py_ssize_t PyBytes_Size(PyObject *o)

   Return the length of the bytes in bytes object *o*.


.. c:function:: Py_ssize_t PyBytes_GET_SIZE(PyObject *o)

   Similar to :c:func:`PyBytes_Size`, but without error checking.


.. c:function:: char* PyBytes_AsString(PyObject *o)

   Return a pointer to the contents of *o*.  The pointer
   refers to the internal buffer of *o*, which consists of ``len(o) + 1``
   bytes.  The last byte in the buffer is always null, regardless of
   whether there are any other null bytes.  The data must not be
   modified in any way, unless the object was just created using
   ``PyBytes_FromStringAndSize(NULL, size)``. It must not be deallocated.  If
   *o* is not a bytes object at all, :c:func:`PyBytes_AsString` returns ``NULL``
   and raises :exc:`TypeError`.


.. c:function:: char* PyBytes_AS_STRING(PyObject *string)

   Similar to :c:func:`PyBytes_AsString`, but without error checking.


.. c:function:: int PyBytes_AsStringAndSize(PyObject *obj, char **buffer, Py_ssize_t *length)

   Return the null-terminated contents of the object *obj*
   through the output variables *buffer* and *length*.
   Returns ``0`` on success.

   If *length* is ``NULL``, the bytes object
   may not contain embedded null bytes;
   if it does, the function returns ``-1`` and a :exc:`ValueError` is raised.

   The buffer refers to an internal buffer of *obj*, which includes an
   additional null byte at the end (not counted in *length*).  The data
   must not be modified in any way, unless the object was just created using
   ``PyBytes_FromStringAndSize(NULL, size)``.  It must not be deallocated.  If
   *obj* is not a bytes object at all, :c:func:`PyBytes_AsStringAndSize`
   returns ``-1`` and raises :exc:`TypeError`.

   .. versionchanged:: 3.5
      Previously, :exc:`TypeError` was raised when embedded null bytes were
      encountered in the bytes object.


.. c:function:: void PyBytes_Concat(PyObject **bytes, PyObject *newpart)

   Create a new bytes object in *\*bytes* containing the contents of *newpart*
   appended to *bytes*; the caller will own the new reference.  The reference to
   the old value of *bytes* will be stolen.  If the new object cannot be
   created, the old reference to *bytes* will still be discarded and the value
   of *\*bytes* will be set to ``NULL``; the appropriate exception will be set.


.. c:function:: void PyBytes_ConcatAndDel(PyObject **bytes, PyObject *newpart)

   Create a new bytes object in *\*bytes* containing the contents of *newpart*
   appended to *bytes*.  This version releases the :term:`strong reference`
   to *newpart* (i.e. decrements its reference count).


.. c:function:: PyObject* PyBytes_Join(PyObject *sep, PyObject *iterable)

   Similar to ``sep.join(iterable)`` in Python.

   *sep* must be Python :class:`bytes` object.
   (Note that :c:func:`PyUnicode_Join` accepts ``NULL`` separator and treats
   it as a space, whereas :c:func:`PyBytes_Join` doesn't accept ``NULL``
   separator.)

   *iterable* must be an iterable object yielding objects that implement the
   :ref:`buffer protocol <bufferobjects>`.

   On success, return a new :class:`bytes` object.
   On error, set an exception and return ``NULL``.

   .. versionadded:: 3.14


.. c:function:: int _PyBytes_Resize(PyObject **bytes, Py_ssize_t newsize)

   Resize a bytes object. *newsize* will be the new length of the bytes object.
   You can think of it as creating a new bytes object and destroying the old
   one, only more efficiently.
   Pass the address of an
   existing bytes object as an lvalue (it may be written into), and the new size
   desired.  On success, *\*bytes* holds the resized bytes object and ``0`` is
   returned; the address in *\*bytes* may differ from its input value.  If the
   reallocation fails, the original bytes object at *\*bytes* is deallocated,
   *\*bytes* is set to ``NULL``, :exc:`MemoryError` is set, and ``-1`` is
   returned.

   .. deprecated:: 3.15
      The function is :term:`soft deprecated`,
      use the :c:type:`PyBytesWriter` API instead.


.. c:function:: PyObject *PyBytes_Repr(PyObject *bytes, int smartquotes)

   Get the string representation of *bytes*. This function is currently used to
   implement :meth:`!bytes.__repr__` in Python.

   This function does not do type checking; it is undefined behavior to pass
   *bytes* as a non-bytes object or ``NULL``.

   If *smartquotes* is true, the representation will use a double-quoted string
   instead of single-quoted string when single-quotes are present in *bytes*.
   For example, the byte string ``'Python'`` would be represented as
   ``b"'Python'"`` when *smartquotes* is true, or ``b'\'Python\''`` when it is
   false.

   On success, this function returns a :term:`strong reference` to a
   :class:`str` object containing the representation. On failure, this
   returns ``NULL`` with an exception set.


.. c:function:: PyObject *PyBytes_DecodeEscape(const char *s, Py_ssize_t len, const char *errors, Py_ssize_t unicode, const char *recode_encoding)

   Unescape a backslash-escaped string *s*. *s* must not be ``NULL``.
   *len* must be the size of *s*.

   *errors* must be one of ``"strict"``, ``"replace"``, or ``"ignore"``. If
   *errors* is ``NULL``, then ``"strict"`` is used by default.

   On success, this function returns a :term:`strong reference` to a Python
   :class:`bytes` object containing the unescaped string. On failure, this
   function returns ``NULL`` with an exception set.

   .. versionchanged:: 3.9
      *unicode* and *recode_encoding* are now unused.


.. _pybyteswriter:

PyBytesWriter
-------------

The :c:type:`PyBytesWriter` API can be used to create a Python :class:`bytes`
object.

.. versionadded:: 3.15

.. c:type:: PyBytesWriter

   A bytes writer instance.

   The API is **not thread safe**: a writer should only be used by a single
   thread at the same time.

   The instance must be destroyed by :c:func:`PyBytesWriter_Finish` on
   success, or :c:func:`PyBytesWriter_Discard` on error.


Create, Finish, Discard
^^^^^^^^^^^^^^^^^^^^^^^

.. c:function:: PyBytesWriter* PyBytesWriter_Create(Py_ssize_t size)

   Create a :c:type:`PyBytesWriter` to write *size* bytes.

   If *size* is greater than zero, allocate *size* bytes, and set the
   writer size to *size*. The caller is responsible to write *size*
   bytes using :c:func:`PyBytesWriter_GetData`.
   This function does not overallocate.

   On error, set an exception and return ``NULL``.

   *size* must be positive or zero.

.. c:function:: PyObject* PyBytesWriter_Finish(PyBytesWriter *writer)

   Finish a :c:type:`PyBytesWriter` created by
   :c:func:`PyBytesWriter_Create`.

   On success, return a Python :class:`bytes` object.
   On error, set an exception and return ``NULL``.

   The writer instance is invalid after the call in any case.
   No API can be called on the writer after :c:func:`PyBytesWriter_Finish`.

.. c:function:: PyObject* PyBytesWriter_FinishWithSize(PyBytesWriter *writer, Py_ssize_t size)

   Similar to :c:func:`PyBytesWriter_Finish`, but resize the writer
   to *size* bytes before creating the :class:`bytes` object.

.. c:function:: PyObject* PyBytesWriter_FinishWithPointer(PyBytesWriter *writer, void *buf)

   Similar to :c:func:`PyBytesWriter_Finish`, but resize the writer
   using *buf* pointer before creating the :class:`bytes` object.

   Set an exception and return ``NULL`` if *buf* pointer is outside the
   internal buffer bounds.

   Function pseudo-code::

       Py_ssize_t size = (char*)buf - (char*)PyBytesWriter_GetData(writer);
       return PyBytesWriter_FinishWithSize(writer, size);

.. c:function:: void PyBytesWriter_Discard(PyBytesWriter *writer)

   Discard a :c:type:`PyBytesWriter` created by :c:func:`PyBytesWriter_Create`.

   Do nothing if *writer* is ``NULL``.

   The writer instance is invalid after the call.
   No API can be called on the writer after :c:func:`PyBytesWriter_Discard`.

High-level API
^^^^^^^^^^^^^^

.. c:function:: int PyBytesWriter_WriteBytes(PyBytesWriter *writer, const void *bytes, Py_ssize_t size)

   Grow the *writer* internal buffer by *size* bytes,
   write *size* bytes of *bytes* at the *writer* end,
   and add *size* to the *writer* size.

   If *size* is equal to ``-1``, call ``strlen(bytes)`` to get the
   string length.

   On success, return ``0``.
   On error, set an exception and return ``-1``.

.. c:function:: int PyBytesWriter_Format(PyBytesWriter *writer, const char *format, ...)

   Similar to :c:func:`PyBytes_FromFormat`, but write the output directly at
   the writer end. Grow the writer internal buffer on demand. Then add the
   written size to the writer size.

   On success, return ``0``.
   On error, set an exception and return ``-1``.


Getters
^^^^^^^

.. c:function:: Py_ssize_t PyBytesWriter_GetSize(PyBytesWriter *writer)

   Get the writer size.

.. c:function:: void* PyBytesWriter_GetData(PyBytesWriter *writer)

   Get the writer data: start of the internal buffer.

   The pointer is valid until :c:func:`PyBytesWriter_Finish` or
   :c:func:`PyBytesWriter_Discard` is called on *writer*.


Low-level API
^^^^^^^^^^^^^

.. c:function:: int PyBytesWriter_Resize(PyBytesWriter *writer, Py_ssize_t size)

   Resize the writer to *size* bytes. It can be used to enlarge or to
   shrink the writer.
   This function typically overallocates to achieve amortized performance when
   resizing multiple times.

   Newly allocated bytes are left uninitialized.

   On success, return ``0``.
   On error, set an exception and return ``-1``.

   *size* must be positive or zero.

.. c:function:: int PyBytesWriter_Grow(PyBytesWriter *writer, Py_ssize_t grow)

   Resize the writer by adding *grow* bytes to the current writer size.
   This function typically overallocates to achieve amortized performance when
   resizing multiple times.

   Newly allocated bytes are left uninitialized.

   On success, return ``0``.
   On error, set an exception and return ``-1``.

   *size* can be negative to shrink the writer.

.. c:function:: void* PyBytesWriter_GrowAndUpdatePointer(PyBytesWriter *writer, Py_ssize_t size, void *buf)

   Similar to :c:func:`PyBytesWriter_Grow`, but update also the *buf*
   pointer.

   The *buf* pointer is moved if the internal buffer is moved in memory.
   The *buf* relative position within the internal buffer is left
   unchanged.

   On error, set an exception and return ``NULL``.

   *buf* must not be ``NULL``.

   Function pseudo-code::

       Py_ssize_t pos = (char*)buf - (char*)PyBytesWriter_GetData(writer);
       if (PyBytesWriter_Grow(writer, size) < 0) {
           return NULL;
       }
       return (char*)PyBytesWriter_GetData(writer) + pos;
