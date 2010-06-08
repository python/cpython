.. highlightlang:: c

.. _bufferobjects:

Buffer Objects
--------------

.. sectionauthor:: Greg Stein <gstein@lyra.org>
.. sectionauthor:: Benjamin Peterson


.. index::
   single: buffer interface

Python objects implemented in C can export a "buffer interface."  These
functions can be used by an object to expose its data in a raw, byte-oriented
format. Clients of the object can use the buffer interface to access the
object data directly, without needing to copy it first.

Two examples of objects that support the buffer interface are bytes and
arrays. The bytes object exposes the character contents in the buffer
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

Buffer objects are useful as a way to expose the data from another object's
buffer interface to the Python programmer.  They can also be used as a zero-copy
slicing mechanism.  Using their ability to reference a block of memory, it is
possible to expose any data to the Python programmer quite easily.  The memory
could be a large, constant array in a C extension, it could be a raw block of
memory for manipulation before passing to an operating system library, or it
could be used to pass around structured data in its native, in-memory format.


.. ctype:: Py_buffer

   .. cmember:: void *buf

      A pointer to the start of the memory for the object.

   .. cmember:: Py_ssize_t len
      :noindex:

      The total length of the memory in bytes.

   .. cmember:: int readonly

      An indicator of whether the buffer is read only.

   .. cmember:: const char *format
      :noindex:

      A *NULL* terminated string in :mod:`struct` module style syntax giving
      the contents of the elements available through the buffer.  If this is
      *NULL*, ``"B"`` (unsigned bytes) is assumed.

   .. cmember:: int ndim

      The number of dimensions the memory represents as a multi-dimensional
      array.  If it is 0, :cdata:`strides` and :cdata:`suboffsets` must be
      *NULL*.

   .. cmember:: Py_ssize_t *shape

      An array of :ctype:`Py_ssize_t`\s the length of :cdata:`ndim` giving the
      shape of the memory as a multi-dimensional array.  Note that
      ``((*shape)[0] * ... * (*shape)[ndims-1])*itemsize`` should be equal to
      :cdata:`len`.

   .. cmember:: Py_ssize_t *strides

      An array of :ctype:`Py_ssize_t`\s the length of :cdata:`ndim` giving the
      number of bytes to skip to get to a new element in each dimension.

   .. cmember:: Py_ssize_t *suboffsets

      An array of :ctype:`Py_ssize_t`\s the length of :cdata:`ndim`.  If these
      suboffset numbers are greater than or equal to 0, then the value stored
      along the indicated dimension is a pointer and the suboffset value
      dictates how many bytes to add to the pointer after de-referencing. A
      suboffset value that it negative indicates that no de-referencing should
      occur (striding in a contiguous memory block).

      Here is a function that returns a pointer to the element in an N-D array
      pointed to by an N-dimensional index when there are both non-NULL strides
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


   .. cmember:: Py_ssize_t itemsize

      This is a storage for the itemsize (in bytes) of each element of the
      shared memory. It is technically un-necessary as it can be obtained
      using :cfunc:`PyBuffer_SizeFromFormat`, however an exporter may know
      this information without parsing the format string and it is necessary
      to know the itemsize for proper interpretation of striding. Therefore,
      storing it is more convenient and faster.

   .. cmember:: void *internal

      This is for use internally by the exporting object. For example, this
      might be re-cast as an integer by the exporter and used to store flags
      about whether or not the shape, strides, and suboffsets arrays must be
      freed when the buffer is released. The consumer should never alter this
      value.


Buffer related functions
========================


.. cfunction:: int PyObject_CheckBuffer(PyObject *obj)

   Return 1 if *obj* supports the buffer interface otherwise 0.


.. cfunction:: int PyObject_GetBuffer(PyObject *obj, Py_buffer *view, int flags)

      Export *obj* into a :ctype:`Py_buffer`, *view*.  These arguments must
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
      :cdata:`Py_buffer` structure is filled in with non-default values and/or
      raise an error if the object can't support a simpler view of its memory.

      0 is returned on success and -1 on error.

      The following table gives possible values to the *flags* arguments.

      +------------------------------+---------------------------------------------------+
      | Flag                         | Description                                       |
      +==============================+===================================================+
      | :cmacro:`PyBUF_SIMPLE`       | This is the default flag state.  The returned     |
      |                              | buffer may or may not have writable memory.  The  |
      |                              | format of the data will be assumed to be unsigned |
      |                              | bytes.  This is a "stand-alone" flag constant. It |
      |                              | never needs to be '|'d to the others. The exporter|
      |                              | will raise an error if it cannot provide such a   |
      |                              | contiguous buffer of bytes.                       |
      |                              |                                                   |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_WRITABLE`     | The returned buffer must be writable.  If it is   |
      |                              | not writable, then raise an error.                |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_STRIDES`      | This implies :cmacro:`PyBUF_ND`. The returned     |
      |                              | buffer must provide strides information (i.e. the |
      |                              | strides cannot be NULL). This would be used when  |
      |                              | the consumer can handle strided, discontiguous    |
      |                              | arrays.  Handling strides automatically assumes   |
      |                              | you can handle shape.  The exporter can raise an  |
      |                              | error if a strided representation of the data is  |
      |                              | not possible (i.e. without the suboffsets).       |
      |                              |                                                   |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_ND`           | The returned buffer must provide shape            |
      |                              | information. The memory will be assumed C-style   |
      |                              | contiguous (last dimension varies the             |
      |                              | fastest). The exporter may raise an error if it   |
      |                              | cannot provide this kind of contiguous buffer. If |
      |                              | this is not given then shape will be *NULL*.      |
      |                              |                                                   |
      |                              |                                                   |
      |                              |                                                   |
      +------------------------------+---------------------------------------------------+
      |:cmacro:`PyBUF_C_CONTIGUOUS`  | These flags indicate that the contiguity returned |
      |:cmacro:`PyBUF_F_CONTIGUOUS`  | buffer must be respectively, C-contiguous (last   |
      |:cmacro:`PyBUF_ANY_CONTIGUOUS`| dimension varies the fastest), Fortran contiguous |
      |                              | (first dimension varies the fastest) or either    |
      |                              | one.  All of these flags imply                    |
      |                              | :cmacro:`PyBUF_STRIDES` and guarantee that the    |
      |                              | strides buffer info structure will be filled in   |
      |                              | correctly.                                        |
      |                              |                                                   |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_INDIRECT`     | This flag indicates the returned buffer must have |
      |                              | suboffsets information (which can be NULL if no   |
      |                              | suboffsets are needed).  This can be used when    |
      |                              | the consumer can handle indirect array            |
      |                              | referencing implied by these suboffsets. This     |
      |                              | implies :cmacro:`PyBUF_STRIDES`.                  |
      |                              |                                                   |
      |                              |                                                   |
      |                              |                                                   |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_FORMAT`       | The returned buffer must have true format         |
      |                              | information if this flag is provided. This would  |
      |                              | be used when the consumer is going to be checking |
      |                              | for what 'kind' of data is actually stored. An    |
      |                              | exporter should always be able to provide this    |
      |                              | information if requested. If format is not        |
      |                              | explicitly requested then the format must be      |
      |                              | returned as *NULL* (which means ``'B'``, or       |
      |                              | unsigned bytes)                                   |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_STRIDED`      | This is equivalent to ``(PyBUF_STRIDES |          |
      |                              | PyBUF_WRITABLE)``.                                |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_STRIDED_RO`   | This is equivalent to ``(PyBUF_STRIDES)``.        |
      |                              |                                                   |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_RECORDS`      | This is equivalent to ``(PyBUF_STRIDES |          |
      |                              | PyBUF_FORMAT | PyBUF_WRITABLE)``.                 |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_RECORDS_RO`   | This is equivalent to ``(PyBUF_STRIDES |          |
      |                              | PyBUF_FORMAT)``.                                  |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_FULL`         | This is equivalent to ``(PyBUF_INDIRECT |         |
      |                              | PyBUF_FORMAT | PyBUF_WRITABLE)``.                 |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_FULL_RO`      | This is equivalent to ``(PyBUF_INDIRECT |         |
      |                              | PyBUF_FORMAT)``.                                  |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_CONTIG`       | This is equivalent to ``(PyBUF_ND |               |
      |                              | PyBUF_WRITABLE)``.                                |
      +------------------------------+---------------------------------------------------+
      | :cmacro:`PyBUF_CONTIG_RO`    | This is equivalent to ``(PyBUF_ND)``.             |
      |                              |                                                   |
      +------------------------------+---------------------------------------------------+


.. cfunction:: void PyBuffer_Release(Py_buffer *view)

   Release the buffer *view*.  This should be called when the buffer is no
   longer being used as it may free memory from it.


.. cfunction:: Py_ssize_t PyBuffer_SizeFromFormat(const char *)

   Return the implied :cdata:`~Py_buffer.itemsize` from the struct-stype
   :cdata:`~Py_buffer.format`.


.. cfunction:: int PyObject_CopyToObject(PyObject *obj, void *buf, Py_ssize_t len, char fortran)

   Copy *len* bytes of data pointed to by the contiguous chunk of memory
   pointed to by *buf* into the buffer exported by obj.  The buffer must of
   course be writable.  Return 0 on success and return -1 and raise an error
   on failure.  If the object does not have a writable buffer, then an error
   is raised.  If *fortran* is ``'F'``, then if the object is
   multi-dimensional, then the data will be copied into the array in
   Fortran-style (first dimension varies the fastest).  If *fortran* is
   ``'C'``, then the data will be copied into the array in C-style (last
   dimension varies the fastest).  If *fortran* is ``'A'``, then it does not
   matter and the copy will be made in whatever way is more efficient.


.. cfunction:: int PyBuffer_IsContiguous(Py_buffer *view, char fortran)

   Return 1 if the memory defined by the *view* is C-style (*fortran* is
   ``'C'``) or Fortran-style (*fortran* is ``'F'``) contiguous or either one
   (*fortran* is ``'A'``).  Return 0 otherwise.


.. cfunction:: void PyBuffer_FillContiguousStrides(int ndim, Py_ssize_t *shape, Py_ssize_t *strides, Py_ssize_t itemsize, char fortran)

   Fill the *strides* array with byte-strides of a contiguous (C-style if
   *fortran* is ``'C'`` or Fortran-style if *fortran* is ``'F'`` array of the
   given shape with the given number of bytes per element.


.. cfunction:: int PyBuffer_FillInfo(Py_buffer *view, void *buf, Py_ssize_t len, int readonly, int infoflags)

   Fill in a buffer-info structure, *view*, correctly for an exporter that can
   only share a contiguous chunk of memory of "unsigned bytes" of the given
   length.  Return 0 on success and -1 (with raising an error) on error.


.. index::
   object: memoryview


MemoryView objects
==================

A memoryview object exposes the C level buffer interface to Python.


.. cfunction:: PyObject* PyMemoryView_FromObject(PyObject *obj)

   Return a memoryview object from an object that defines the buffer interface.


.. cfunction:: PyObject * PyMemoryView_GetContiguous(PyObject *obj,  int buffertype, char order)

   Return a memoryview object to a contiguous chunk of memory (in either
   'C' or 'F'ortran order) from an object that defines the buffer
   interface. If memory is contiguous, the memoryview object points to the
   original memory. Otherwise copy is made and the memoryview points to a
   new bytes object.
