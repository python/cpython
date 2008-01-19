.. highlightlang:: c

.. _bufferobjects:

Buffer Objects
--------------

.. sectionauthor:: Greg Stein <gstein@lyra.org>


.. index::
   object: buffer
   single: buffer interface

Python objects implemented in C can export a group of functions called the
"buffer interface."  These functions can be used by an object to expose its data
in a raw, byte-oriented format. Clients of the object can use the buffer
interface to access the object data directly, without needing to copy it first.

Two examples of objects that support the buffer interface are strings and
arrays. The string object exposes the character contents in the buffer
interface's byte-oriented form. An array can also expose its contents, but it
should be noted that array elements may be multi-byte values.

An example user of the buffer interface is the file object's :meth:`write`
method. Any object that can export a series of bytes through the buffer
interface can be written to a file. There are a number of format codes to
:cfunc:`PyArg_ParseTuple` that operate against an object's buffer interface,
returning data from the target object.

.. index:: single: PyBufferProcs

More information on the buffer interface is provided in the section 
:ref:`buffer-structs`, under the description for :ctype:`PyBufferProcs`.

A "buffer object" is defined in the :file:`bufferobject.h` header (included by
:file:`Python.h`). These objects look very similar to string objects at the
Python programming level: they support slicing, indexing, concatenation, and
some other standard string operations. However, their data can come from one of
two sources: from a block of memory, or from another object which exports the
buffer interface.

Buffer objects are useful as a way to expose the data from another object's
buffer interface to the Python programmer. They can also be used as a zero-copy
slicing mechanism. Using their ability to reference a block of memory, it is
possible to expose any data to the Python programmer quite easily. The memory
could be a large, constant array in a C extension, it could be a raw block of
memory for manipulation before passing to an operating system library, or it
could be used to pass around structured data in its native, in-memory format.


.. ctype:: PyBufferObject

   This subtype of :ctype:`PyObject` represents a buffer object.


.. cvar:: PyTypeObject PyBuffer_Type

   .. index:: single: BufferType (in module types)

   The instance of :ctype:`PyTypeObject` which represents the Python buffer type;
   it is the same object as ``buffer`` and  ``types.BufferType`` in the Python
   layer. .


.. cvar:: int Py_END_OF_BUFFER

   This constant may be passed as the *size* parameter to
   :cfunc:`PyBuffer_FromObject` or :cfunc:`PyBuffer_FromReadWriteObject`.  It
   indicates that the new :ctype:`PyBufferObject` should refer to *base* object
   from the specified *offset* to the end of its exported buffer.  Using this
   enables the caller to avoid querying the *base* object for its length.


.. cfunction:: int PyBuffer_Check(PyObject *p)

   Return true if the argument has type :cdata:`PyBuffer_Type`.


.. cfunction:: PyObject* PyBuffer_FromObject(PyObject *base, Py_ssize_t offset, Py_ssize_t size)

   Return a new read-only buffer object.  This raises :exc:`TypeError` if *base*
   doesn't support the read-only buffer protocol or doesn't provide exactly one
   buffer segment, or it raises :exc:`ValueError` if *offset* is less than zero.
   The buffer will hold a reference to the *base* object, and the buffer's contents
   will refer to the *base* object's buffer interface, starting as position
   *offset* and extending for *size* bytes. If *size* is :const:`Py_END_OF_BUFFER`,
   then the new buffer's contents extend to the length of the *base* object's
   exported buffer data.


.. cfunction:: PyObject* PyBuffer_FromReadWriteObject(PyObject *base, Py_ssize_t offset, Py_ssize_t size)

   Return a new writable buffer object.  Parameters and exceptions are similar to
   those for :cfunc:`PyBuffer_FromObject`.  If the *base* object does not export
   the writeable buffer protocol, then :exc:`TypeError` is raised.


.. cfunction:: PyObject* PyBuffer_FromMemory(void *ptr, Py_ssize_t size)

   Return a new read-only buffer object that reads from a specified location in
   memory, with a specified size.  The caller is responsible for ensuring that the
   memory buffer, passed in as *ptr*, is not deallocated while the returned buffer
   object exists.  Raises :exc:`ValueError` if *size* is less than zero.  Note that
   :const:`Py_END_OF_BUFFER` may *not* be passed for the *size* parameter;
   :exc:`ValueError` will be raised in that case.


.. cfunction:: PyObject* PyBuffer_FromReadWriteMemory(void *ptr, Py_ssize_t size)

   Similar to :cfunc:`PyBuffer_FromMemory`, but the returned buffer is writable.


.. cfunction:: PyObject* PyBuffer_New(Py_ssize_t size)

   Return a new writable buffer object that maintains its own memory buffer of
   *size* bytes.  :exc:`ValueError` is returned if *size* is not zero or positive.
   Note that the memory buffer (as returned by :cfunc:`PyObject_AsWriteBuffer`) is
   not specifically aligned.
