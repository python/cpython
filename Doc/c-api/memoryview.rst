.. highlight:: c

.. _memoryview-objects:

.. index::
   pair: object; memoryview

MemoryView objects
------------------

A :class:`memoryview` object exposes the C level :ref:`buffer interface
<bufferobjects>` as a Python object which can then be passed around like
any other object.


.. c:function:: PyObject *PyMemoryView_FromObject(PyObject *obj)

   Create a memoryview object from an object that provides the buffer interface.
   If *obj* supports writable buffer exports, the memoryview object will be
   read/write, otherwise it may be either read-only or read/write at the
   discretion of the exporter.


.. c:macro:: PyBUF_READ

   Flag to request a readonly buffer.


.. c:macro:: PyBUF_WRITE

   Flag to request a writable buffer.


.. c:function:: PyObject *PyMemoryView_FromMemory(char *mem, Py_ssize_t size, int flags)

   Create a memoryview object using *mem* as the underlying buffer.
   *flags* can be one of :c:macro:`PyBUF_READ` or :c:macro:`PyBUF_WRITE`.

   .. versionadded:: 3.3

.. c:function:: PyObject *PyMemoryView_FromBuffer(const Py_buffer *view)

   Create a memoryview object wrapping the given buffer structure *view*.
   For simple byte buffers, :c:func:`PyMemoryView_FromMemory` is the preferred
   function.

.. c:function:: PyObject *PyMemoryView_GetContiguous(PyObject *obj, int buffertype, char order)

   Create a memoryview object to a :term:`contiguous` chunk of memory (in either
   'C' or 'F'ortran *order*) from an object that defines the buffer
   interface. If memory is contiguous, the memoryview object points to the
   original memory. Otherwise, a copy is made and the memoryview points to a
   new bytes object.

   *buffertype* can be one of :c:macro:`PyBUF_READ` or :c:macro:`PyBUF_WRITE`.


.. c:function:: int PyMemoryView_Check(PyObject *obj)

   Return true if the object *obj* is a memoryview object.  It is not
   currently allowed to create subclasses of :class:`memoryview`.  This
   function always succeeds.


.. c:function:: Py_buffer *PyMemoryView_GET_BUFFER(PyObject *mview)

   Return a pointer to the memoryview's private copy of the exporter's buffer.
   *mview* **must** be a memoryview instance; this macro doesn't check its type,
   you must do it yourself or you will risk crashes.

.. c:function:: PyObject *PyMemoryView_GET_BASE(PyObject *mview)

   Return either a pointer to the exporting object that the memoryview is based
   on or ``NULL`` if the memoryview has been created by one of the functions
   :c:func:`PyMemoryView_FromMemory` or :c:func:`PyMemoryView_FromBuffer`.
   *mview* **must** be a memoryview instance.
