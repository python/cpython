.. highlightlang:: c

.. _bufferobjects:

Buffer Protocol
---------------

.. sectionauthor:: Greg Stein <gstein@lyra.org>
.. sectionauthor:: Benjamin Peterson


.. index::
   single: buffer interface

Certain objects available in Python wrap access to an underlying memory
array or *buffer*.  Such objects include the built-in :class:`bytes` and
:class:`bytearray`, and some extension types like :class:`array.array`.
Third-party libraries may define their own types for special purposes, such
as image processing or numeric analysis.

While each of these types have their own semantics, they share the common
characteristic of being backed by a possibly large memory buffer.  It is
then desireable, in some situations, to access that buffer directly and
without intermediate copying.

Python provides such a facility at the C level in the form of the *buffer
protocol*.  This protocol has two sides:

.. index:: single: PyBufferProcs

- on the producer side, a type can export a "buffer interface" which allows
  objects of that type to expose information about their underlying buffer.
  This interface is described in the section :ref:`buffer-structs`;

- on the consumer side, several means are available to obtain a pointer to
  the raw underlying data of an object (for example a method parameter).

Simple objects such as :class:`bytes` and :class:`bytearray` expose their
underlying buffer in byte-oriented form.  Other forms are possible; for example,
the elements exposed by a :class:`array.array` can be multi-byte values.

An example consumer of the buffer interface is the :meth:`~io.BufferedIOBase.write`
method of file objects: any object that can export a series of bytes through
the buffer interface can be written to a file.  While :meth:`write` only
needs read-only access to the internal contents of the object passed to it,
other methods such as :meth:`~io.BufferedIOBase.readinto` need write access
to the contents of their argument.  The buffer interface allows objects to
selectively allow or reject exporting of read-write and read-only buffers.

There are two ways for a consumer of the buffer interface to acquire a buffer
over a target object:

* call :c:func:`PyObject_GetBuffer` with the right parameters;

* call :c:func:`PyArg_ParseTuple` (or one of its siblings) with one of the
  ``y*``, ``w*`` or ``s*`` :ref:`format codes <arg-parsing>`.

In both cases, :c:func:`PyBuffer_Release` must be called when the buffer
isn't needed anymore.  Failure to do so could lead to various issues such as
resource leaks.


The buffer structure
====================

Buffer structures (or simply "buffers") are useful as a way to expose the
binary data from another object to the Python programmer.  They can also be
used as a zero-copy slicing mechanism.  Using their ability to reference a
block of memory, it is possible to expose any data to the Python programmer
quite easily.  The memory could be a large, constant array in a C extension,
it could be a raw block of memory for manipulation before passing to an
operating system library, or it could be used to pass around structured data
in its native, in-memory format.

Contrary to most data types exposed by the Python interpreter, buffers
are not :c:type:`PyObject` pointers but rather simple C structures.  This
allows them to be created and copied very simply.  When a generic wrapper
around a buffer is needed, a :ref:`memoryview <memoryview-objects>` object
can be created.


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


Buffer-related functions
========================


.. c:function:: int PyObject_CheckBuffer(PyObject *obj)

   Return 1 if *obj* supports the buffer interface otherwise 0.  When 1 is
   returned, it doesn't guarantee that :c:func:`PyObject_GetBuffer` will
   succeed.


.. c:function:: int PyObject_GetBuffer(PyObject *obj, Py_buffer *view, int flags)

      Export a view over some internal data from the target object *obj*.
      *obj* must not be NULL, and *view* must point to an existing
      :c:type:`Py_buffer` structure allocated by the caller (most uses of
      this function will simply declare a local variable of type
      :c:type:`Py_buffer`).  The *flags* argument is a bit field indicating
      what kind of buffer is requested.  The buffer interface allows
      for complicated memory layout possibilities; however, some callers
      won't want to handle all the complexity and instead request a simple
      view of the target object (using :c:macro:`PyBUF_SIMPLE` for a read-only
      view and :c:macro:`PyBUF_WRITABLE` for a read-write view).

      Some exporters may not be able to share memory in every possible way and
      may need to raise errors to signal to some consumers that something is
      just not possible. These errors should be a :exc:`BufferError` unless
      there is another error that is actually causing the problem. The
      exporter can use flags information to simplify how much of the
      :c:data:`Py_buffer` structure is filled in with non-default values and/or
      raise an error if the object can't support a simpler view of its memory.

      On success, 0 is returned and the *view* structure is filled with useful
      values.  On error, -1 is returned and an exception is raised; the *view*
      is left in an undefined state.

      The following are the possible values to the *flags* arguments.

      .. c:macro:: PyBUF_SIMPLE

         This is the default flag.  The returned buffer exposes a read-only
         memory area.  The format of data is assumed to be raw unsigned bytes,
         without any particular structure.  This is a "stand-alone" flag
         constant.  It never needs to be '|'d to the others.  The exporter will
         raise an error if it cannot provide such a contiguous buffer of bytes.

      .. c:macro:: PyBUF_WRITABLE

         Like :c:macro:`PyBUF_SIMPLE`, but the returned buffer is writable.  If
         the exporter doesn't support writable buffers, an error is raised.

      .. c:macro:: PyBUF_STRIDES

         This implies :c:macro:`PyBUF_ND`.  The returned buffer must provide
         strides information (i.e. the strides cannot be NULL).  This would be
         used when the consumer can handle strided, discontiguous arrays.
         Handling strides automatically assumes you can handle shape.  The
         exporter can raise an error if a strided representation of the data is
         not possible (i.e. without the suboffsets).

      .. c:macro:: PyBUF_ND

         The returned buffer must provide shape information.  The memory will be
         assumed C-style contiguous (last dimension varies the fastest).  The
         exporter may raise an error if it cannot provide this kind of
         contiguous buffer.  If this is not given then shape will be *NULL*.

      .. c:macro:: PyBUF_C_CONTIGUOUS
                  PyBUF_F_CONTIGUOUS
                  PyBUF_ANY_CONTIGUOUS

         These flags indicate that the contiguity returned buffer must be
         respectively, C-contiguous (last dimension varies the fastest), Fortran
         contiguous (first dimension varies the fastest) or either one.  All of
         these flags imply :c:macro:`PyBUF_STRIDES` and guarantee that the
         strides buffer info structure will be filled in correctly.

      .. c:macro:: PyBUF_INDIRECT

         This flag indicates the returned buffer must have suboffsets
         information (which can be NULL if no suboffsets are needed).  This can
         be used when the consumer can handle indirect array referencing implied
         by these suboffsets. This implies :c:macro:`PyBUF_STRIDES`.

      .. c:macro:: PyBUF_FORMAT

         The returned buffer must have true format information if this flag is
         provided.  This would be used when the consumer is going to be checking
         for what 'kind' of data is actually stored.  An exporter should always
         be able to provide this information if requested.  If format is not
         explicitly requested then the format must be returned as *NULL* (which
         means ``'B'``, or unsigned bytes).

      .. c:macro:: PyBUF_STRIDED

         This is equivalent to ``(PyBUF_STRIDES | PyBUF_WRITABLE)``.

      .. c:macro:: PyBUF_STRIDED_RO

         This is equivalent to ``(PyBUF_STRIDES)``.

      .. c:macro:: PyBUF_RECORDS

         This is equivalent to ``(PyBUF_STRIDES | PyBUF_FORMAT |
         PyBUF_WRITABLE)``.

      .. c:macro:: PyBUF_RECORDS_RO

         This is equivalent to ``(PyBUF_STRIDES | PyBUF_FORMAT)``.

      .. c:macro:: PyBUF_FULL

         This is equivalent to ``(PyBUF_INDIRECT | PyBUF_FORMAT |
         PyBUF_WRITABLE)``.

      .. c:macro:: PyBUF_FULL_RO

         This is equivalent to ``(PyBUF_INDIRECT | PyBUF_FORMAT)``.

      .. c:macro:: PyBUF_CONTIG

         This is equivalent to ``(PyBUF_ND | PyBUF_WRITABLE)``.

      .. c:macro:: PyBUF_CONTIG_RO

         This is equivalent to ``(PyBUF_ND)``.


.. c:function:: void PyBuffer_Release(Py_buffer *view)

   Release the buffer *view*.  This should be called when the buffer is no
   longer being used as it may free memory from it.


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

