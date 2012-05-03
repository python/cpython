.. highlightlang:: c

.. _bufferobjects:

Buffers and Memoryview Objects
------------------------------

.. sectionauthor:: Greg Stein <gstein@lyra.org>
.. sectionauthor:: Benjamin Peterson


.. index::
   object: buffer
   single: buffer interface

Python objects implemented in C can export a group of functions called the
"buffer interface."  These functions can be used by an object to expose its
data in a raw, byte-oriented format. Clients of the object can use the buffer
interface to access the object data directly, without needing to copy it
first.

Two examples of objects that support the buffer interface are strings and
arrays. The string object exposes the character contents in the buffer
interface's byte-oriented form. An array can also expose its contents, but it
should be noted that array elements may be multi-byte values.

An example user of the buffer interface is the file object's :meth:`write`
method. Any object that can export a series of bytes through the buffer
interface can be written to a file. There are a number of format codes to
:c:func:`PyArg_ParseTuple` that operate against an object's buffer interface,
returning data from the target object.

Starting from version 1.6, Python has been providing Python-level buffer
objects and a C-level buffer API so that any built-in or used-defined type can
expose its characteristics. Both, however, have been deprecated because of
various shortcomings, and have been officially removed in Python 3 in favour
of a new C-level buffer API and a new Python-level object named
:class:`memoryview`.

The new buffer API has been backported to Python 2.6, and the
:class:`memoryview` object has been backported to Python 2.7. It is strongly
advised to use them rather than the old APIs, unless you are blocked from
doing so for compatibility reasons.


The new-style Py_buffer struct
==============================


.. c:type:: Py_buffer

   .. c:member:: void *buf

      A pointer to the start of the memory for the object.

   .. c:member:: Py_ssize_t len
      :noindex:

      The total length of the memory in bytes.

   .. c:member:: int readonly

      An indicator of whether the buffer is read only.

   .. c:member:: const char *format
      :noindex:

      A *NULL* terminated string in :mod:`struct` module style syntax giving
      the contents of the elements available through the buffer.  If this is
      *NULL*, ``"B"`` (unsigned bytes) is assumed.

   .. c:member:: int ndim

      The number of dimensions the memory represents as a multi-dimensional
      array.  If it is 0, :c:data:`strides` and :c:data:`suboffsets` must be
      *NULL*.

   .. c:member:: Py_ssize_t *shape

      An array of :c:type:`Py_ssize_t`\s the length of :c:data:`ndim` giving the
      shape of the memory as a multi-dimensional array.  Note that
      ``((*shape)[0] * ... * (*shape)[ndims-1])*itemsize`` should be equal to
      :c:data:`len`.

   .. c:member:: Py_ssize_t *strides

      An array of :c:type:`Py_ssize_t`\s the length of :c:data:`ndim` giving the
      number of bytes to skip to get to a new element in each dimension.

   .. c:member:: Py_ssize_t *suboffsets

      An array of :c:type:`Py_ssize_t`\s the length of :c:data:`ndim`.  If these
      suboffset numbers are greater than or equal to 0, then the value stored
      along the indicated dimension is a pointer and the suboffset value
      dictates how many bytes to add to the pointer after de-referencing. A
      suboffset value that it negative indicates that no de-referencing should
      occur (striding in a contiguous memory block).

      Here is a function that returns a pointer to the element in an N-D array
      pointed to by an N-dimesional index when there are both non-NULL strides
      and suboffsets::

          void *get_item_pointer(int ndim, void *buf, Py_ssize_t *strides,
              Py_ssize_t *suboffsets, Py_ssize_t *indices) {
              char *pointer = (char*)buf;
              int i;
              for (i = 0; i < ndim; i++) {
                  pointer += strides[i] * indices[i];
                  if (suboffsets[i] >=0 ) {
                      pointer = *((char**)pointer) + suboffsets[i];
                  }
              }
              return (void*)pointer;
           }


   .. c:member:: Py_ssize_t itemsize

      This is a storage for the itemsize (in bytes) of each element of the
      shared memory. It is technically un-necessary as it can be obtained
      using :c:func:`PyBuffer_SizeFromFormat`, however an exporter may know
      this information without parsing the format string and it is necessary
      to know the itemsize for proper interpretation of striding. Therefore,
      storing it is more convenient and faster.

   .. c:member:: void *internal

      This is for use internally by the exporting object. For example, this
      might be re-cast as an integer by the exporter and used to store flags
      about whether or not the shape, strides, and suboffsets arrays must be
      freed when the buffer is released. The consumer should never alter this
      value.


Buffer related functions
========================


.. c:function:: int PyObject_CheckBuffer(PyObject *obj)

   Return 1 if *obj* supports the buffer interface otherwise 0.


.. c:function:: int PyObject_GetBuffer(PyObject *obj, Py_buffer *view, int flags)

      Export *obj* into a :c:type:`Py_buffer`, *view*.  These arguments must
      never be *NULL*.  The *flags* argument is a bit field indicating what
      kind of buffer the caller is prepared to deal with and therefore what
      kind of buffer the exporter is allowed to return.  The buffer interface
      allows for complicated memory sharing possibilities, but some caller may
      not be able to handle all the complexity but may want to see if the
      exporter will let them take a simpler view to its memory.

      Some exporters may not be able to share memory in every possible way and
      may need to raise errors to signal to some consumers that something is
      just not possible. These errors should be a :exc:`BufferError` unless
      there is another error that is actually causing the problem. The
      exporter can use flags information to simplify how much of the
      :c:data:`Py_buffer` structure is filled in with non-default values and/or
      raise an error if the object can't support a simpler view of its memory.

      0 is returned on success and -1 on error.

      The following table gives possible values to the *flags* arguments.

      +-------------------------------+---------------------------------------------------+
      | Flag                          | Description                                       |
      +===============================+===================================================+
      | :c:macro:`PyBUF_SIMPLE`       | This is the default flag state.  The returned     |
      |                               | buffer may or may not have writable memory.  The  |
      |                               | format of the data will be assumed to be unsigned |
      |                               | bytes.  This is a "stand-alone" flag constant. It |
      |                               | never needs to be '|'d to the others. The exporter|
      |                               | will raise an error if it cannot provide such a   |
      |                               | contiguous buffer of bytes.                       |
      |                               |                                                   |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_WRITABLE`     | The returned buffer must be writable.  If it is   |
      |                               | not writable, then raise an error.                |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_STRIDES`      | This implies :c:macro:`PyBUF_ND`. The returned    |
      |                               | buffer must provide strides information (i.e. the |
      |                               | strides cannot be NULL). This would be used when  |
      |                               | the consumer can handle strided, discontiguous    |
      |                               | arrays.  Handling strides automatically assumes   |
      |                               | you can handle shape.  The exporter can raise an  |
      |                               | error if a strided representation of the data is  |
      |                               | not possible (i.e. without the suboffsets).       |
      |                               |                                                   |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_ND`           | The returned buffer must provide shape            |
      |                               | information. The memory will be assumed C-style   |
      |                               | contiguous (last dimension varies the             |
      |                               | fastest). The exporter may raise an error if it   |
      |                               | cannot provide this kind of contiguous buffer. If |
      |                               | this is not given then shape will be *NULL*.      |
      |                               |                                                   |
      |                               |                                                   |
      |                               |                                                   |
      +-------------------------------+---------------------------------------------------+
      |:c:macro:`PyBUF_C_CONTIGUOUS`  | These flags indicate that the contiguity returned |
      |:c:macro:`PyBUF_F_CONTIGUOUS`  | buffer must be respectively, C-contiguous (last   |
      |:c:macro:`PyBUF_ANY_CONTIGUOUS`| dimension varies the fastest), Fortran contiguous |
      |                               | (first dimension varies the fastest) or either    |
      |                               | one.  All of these flags imply                    |
      |                               | :c:macro:`PyBUF_STRIDES` and guarantee that the   |
      |                               | strides buffer info structure will be filled in   |
      |                               | correctly.                                        |
      |                               |                                                   |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_INDIRECT`     | This flag indicates the returned buffer must have |
      |                               | suboffsets information (which can be NULL if no   |
      |                               | suboffsets are needed).  This can be used when    |
      |                               | the consumer can handle indirect array            |
      |                               | referencing implied by these suboffsets. This     |
      |                               | implies :c:macro:`PyBUF_STRIDES`.                 |
      |                               |                                                   |
      |                               |                                                   |
      |                               |                                                   |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_FORMAT`       | The returned buffer must have true format         |
      |                               | information if this flag is provided. This would  |
      |                               | be used when the consumer is going to be checking |
      |                               | for what 'kind' of data is actually stored. An    |
      |                               | exporter should always be able to provide this    |
      |                               | information if requested. If format is not        |
      |                               | explicitly requested then the format must be      |
      |                               | returned as *NULL* (which means ``'B'``, or       |
      |                               | unsigned bytes)                                   |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_STRIDED`      | This is equivalent to ``(PyBUF_STRIDES |          |
      |                               | PyBUF_WRITABLE)``.                                |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_STRIDED_RO`   | This is equivalent to ``(PyBUF_STRIDES)``.        |
      |                               |                                                   |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_RECORDS`      | This is equivalent to ``(PyBUF_STRIDES |          |
      |                               | PyBUF_FORMAT | PyBUF_WRITABLE)``.                 |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_RECORDS_RO`   | This is equivalent to ``(PyBUF_STRIDES |          |
      |                               | PyBUF_FORMAT)``.                                  |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_FULL`         | This is equivalent to ``(PyBUF_INDIRECT |         |
      |                               | PyBUF_FORMAT | PyBUF_WRITABLE)``.                 |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_FULL_RO`      | This is equivalent to ``(PyBUF_INDIRECT |         |
      |                               | PyBUF_FORMAT)``.                                  |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_CONTIG`       | This is equivalent to ``(PyBUF_ND |               |
      |                               | PyBUF_WRITABLE)``.                                |
      +-------------------------------+---------------------------------------------------+
      | :c:macro:`PyBUF_CONTIG_RO`    | This is equivalent to ``(PyBUF_ND)``.             |
      |                               |                                                   |
      +-------------------------------+---------------------------------------------------+


.. c:function:: void PyBuffer_Release(Py_buffer *view)

   Release the buffer *view*.  This should be called when the buffer
   is no longer being used as it may free memory from it.


.. c:function:: Py_ssize_t PyBuffer_SizeFromFormat(const char *)

   Return the implied :c:data:`~Py_buffer.itemsize` from the struct-stype
   :c:data:`~Py_buffer.format`.


.. c:function:: int PyBuffer_IsContiguous(Py_buffer *view, char fortran)

   Return 1 if the memory defined by the *view* is C-style (*fortran* is
   ``'C'``) or Fortran-style (*fortran* is ``'F'``) contiguous or either one
   (*fortran* is ``'A'``).  Return 0 otherwise.


.. c:function:: void PyBuffer_FillContiguousStrides(int ndim, Py_ssize_t *shape, Py_ssize_t *strides, Py_ssize_t itemsize, char fortran)

   Fill the *strides* array with byte-strides of a contiguous (C-style if
   *fortran* is ``'C'`` or Fortran-style if *fortran* is ``'F'``) array of the
   given shape with the given number of bytes per element.


.. c:function:: int PyBuffer_FillInfo(Py_buffer *view, PyObject *obj, void *buf, Py_ssize_t len, int readonly, int infoflags)

   Fill in a buffer-info structure, *view*, correctly for an exporter that can
   only share a contiguous chunk of memory of "unsigned bytes" of the given
   length.  Return 0 on success and -1 (with raising an error) on error.


MemoryView objects
==================

.. versionadded:: 2.7

A :class:`memoryview` object exposes the new C level buffer interface as a
Python object which can then be passed around like any other object.

.. c:function:: PyObject *PyMemoryView_FromObject(PyObject *obj)

   Create a memoryview object from an object that defines the new buffer
   interface.


.. c:function:: PyObject *PyMemoryView_FromBuffer(Py_buffer *view)

   Create a memoryview object wrapping the given buffer-info structure *view*.
   The memoryview object then owns the buffer, which means you shouldn't
   try to release it yourself: it will be released on deallocation of the
   memoryview object.


.. c:function:: PyObject *PyMemoryView_GetContiguous(PyObject *obj, int buffertype, char order)

   Create a memoryview object to a contiguous chunk of memory (in either
   'C' or 'F'ortran *order*) from an object that defines the buffer
   interface. If memory is contiguous, the memoryview object points to the
   original memory. Otherwise copy is made and the memoryview points to a
   new bytes object.


.. c:function:: int PyMemoryView_Check(PyObject *obj)

   Return true if the object *obj* is a memoryview object.  It is not
   currently allowed to create subclasses of :class:`memoryview`.


.. c:function:: Py_buffer *PyMemoryView_GET_BUFFER(PyObject *obj)

   Return a pointer to the buffer-info structure wrapped by the given
   object.  The object **must** be a memoryview instance; this macro doesn't
   check its type, you must do it yourself or you will risk crashes.


Old-style buffer objects
========================

.. index:: single: PyBufferProcs

More information on the old buffer interface is provided in the section
:ref:`buffer-structs`, under the description for :c:type:`PyBufferProcs`.

A "buffer object" is defined in the :file:`bufferobject.h` header (included by
:file:`Python.h`). These objects look very similar to string objects at the
Python programming level: they support slicing, indexing, concatenation, and
some other standard string operations. However, their data can come from one
of two sources: from a block of memory, or from another object which exports
the buffer interface.

Buffer objects are useful as a way to expose the data from another object's
buffer interface to the Python programmer. They can also be used as a
zero-copy slicing mechanism. Using their ability to reference a block of
memory, it is possible to expose any data to the Python programmer quite
easily. The memory could be a large, constant array in a C extension, it could
be a raw block of memory for manipulation before passing to an operating
system library, or it could be used to pass around structured data in its
native, in-memory format.


.. c:type:: PyBufferObject

   This subtype of :c:type:`PyObject` represents a buffer object.


.. c:var:: PyTypeObject PyBuffer_Type

   .. index:: single: BufferType (in module types)

   The instance of :c:type:`PyTypeObject` which represents the Python buffer type;
   it is the same object as ``buffer`` and  ``types.BufferType`` in the Python
   layer. .


.. c:var:: int Py_END_OF_BUFFER

   This constant may be passed as the *size* parameter to
   :c:func:`PyBuffer_FromObject` or :c:func:`PyBuffer_FromReadWriteObject`.  It
   indicates that the new :c:type:`PyBufferObject` should refer to *base*
   object from the specified *offset* to the end of its exported buffer.
   Using this enables the caller to avoid querying the *base* object for its
   length.


.. c:function:: int PyBuffer_Check(PyObject *p)

   Return true if the argument has type :c:data:`PyBuffer_Type`.


.. c:function:: PyObject* PyBuffer_FromObject(PyObject *base, Py_ssize_t offset, Py_ssize_t size)

   Return a new read-only buffer object.  This raises :exc:`TypeError` if
   *base* doesn't support the read-only buffer protocol or doesn't provide
   exactly one buffer segment, or it raises :exc:`ValueError` if *offset* is
   less than zero.  The buffer will hold a reference to the *base* object, and
   the buffer's contents will refer to the *base* object's buffer interface,
   starting as position *offset* and extending for *size* bytes. If *size* is
   :const:`Py_END_OF_BUFFER`, then the new buffer's contents extend to the
   length of the *base* object's exported buffer data.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *offset* and *size*. This
      might require changes in your code for properly supporting 64-bit
      systems.


.. c:function:: PyObject* PyBuffer_FromReadWriteObject(PyObject *base, Py_ssize_t offset, Py_ssize_t size)

   Return a new writable buffer object.  Parameters and exceptions are similar
   to those for :c:func:`PyBuffer_FromObject`.  If the *base* object does not
   export the writeable buffer protocol, then :exc:`TypeError` is raised.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *offset* and *size*. This
      might require changes in your code for properly supporting 64-bit
      systems.


.. c:function:: PyObject* PyBuffer_FromMemory(void *ptr, Py_ssize_t size)

   Return a new read-only buffer object that reads from a specified location
   in memory, with a specified size.  The caller is responsible for ensuring
   that the memory buffer, passed in as *ptr*, is not deallocated while the
   returned buffer object exists.  Raises :exc:`ValueError` if *size* is less
   than zero.  Note that :const:`Py_END_OF_BUFFER` may *not* be passed for the
   *size* parameter; :exc:`ValueError` will be raised in that case.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *size*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: PyObject* PyBuffer_FromReadWriteMemory(void *ptr, Py_ssize_t size)

   Similar to :c:func:`PyBuffer_FromMemory`, but the returned buffer is
   writable.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *size*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: PyObject* PyBuffer_New(Py_ssize_t size)

   Return a new writable buffer object that maintains its own memory buffer of
   *size* bytes.  :exc:`ValueError` is returned if *size* is not zero or
   positive.  Note that the memory buffer (as returned by
   :c:func:`PyObject_AsWriteBuffer`) is not specifically aligned.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *size*. This might require
      changes in your code for properly supporting 64-bit systems.
