.. highlightlang:: c

.. _fileobjects:

File Objects
------------

.. index:: object: file

Python's built-in file objects are implemented entirely on the :c:type:`FILE\*`
support from the C standard library.  This is an implementation detail and may
change in future releases of Python.


.. c:type:: PyFileObject

   This subtype of :c:type:`PyObject` represents a Python file object.


.. c:var:: PyTypeObject PyFile_Type

   .. index:: single: FileType (in module types)

   This instance of :c:type:`PyTypeObject` represents the Python file type.  This is
   exposed to Python programs as ``file`` and ``types.FileType``.


.. c:function:: int PyFile_Check(PyObject *p)

   Return true if its argument is a :c:type:`PyFileObject` or a subtype of
   :c:type:`PyFileObject`.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. c:function:: int PyFile_CheckExact(PyObject *p)

   Return true if its argument is a :c:type:`PyFileObject`, but not a subtype of
   :c:type:`PyFileObject`.

   .. versionadded:: 2.2


.. c:function:: PyObject* PyFile_FromString(char *filename, char *mode)

   .. index:: single: fopen()

   On success, return a new file object that is opened on the file given by
   *filename*, with a file mode given by *mode*, where *mode* has the same
   semantics as the standard C routine :c:func:`fopen`.  On failure, return *NULL*.


.. c:function:: PyObject* PyFile_FromFile(FILE *fp, char *name, char *mode, int (*close)(FILE*))

   Create a new :c:type:`PyFileObject` from the already-open standard C file
   pointer, *fp*.  The function *close* will be called when the file should be
   closed.  Return *NULL* and close the file using *close* on failure.
   *close* is optional and can be set to *NULL*.


.. c:function:: FILE* PyFile_AsFile(PyObject \*p)

   Return the file object associated with *p* as a :c:type:`FILE\*`.

   If the caller will ever use the returned :c:type:`FILE\*` object while
   the :term:`GIL` is released it must also call the :c:func:`PyFile_IncUseCount` and
   :c:func:`PyFile_DecUseCount` functions described below as appropriate.


.. c:function:: void PyFile_IncUseCount(PyFileObject \*p)

   Increments the PyFileObject's internal use count to indicate
   that the underlying :c:type:`FILE\*` is being used.
   This prevents Python from calling f_close() on it from another thread.
   Callers of this must call :c:func:`PyFile_DecUseCount` when they are
   finished with the :c:type:`FILE\*`.  Otherwise the file object will
   never be closed by Python.

   The :term:`GIL` must be held while calling this function.

   The suggested use is to call this after :c:func:`PyFile_AsFile` and before
   you release the GIL::

      FILE *fp = PyFile_AsFile(p);
      PyFile_IncUseCount(p);
      /* ... */
      Py_BEGIN_ALLOW_THREADS
      do_something(fp);
      Py_END_ALLOW_THREADS
      /* ... */
      PyFile_DecUseCount(p);

   .. versionadded:: 2.6


.. c:function:: void PyFile_DecUseCount(PyFileObject \*p)

   Decrements the PyFileObject's internal unlocked_count member to
   indicate that the caller is done with its own use of the :c:type:`FILE\*`.
   This may only be called to undo a prior call to :c:func:`PyFile_IncUseCount`.

   The :term:`GIL` must be held while calling this function (see the example
   above).

   .. versionadded:: 2.6


.. c:function:: PyObject* PyFile_GetLine(PyObject *p, int n)

   .. index:: single: EOFError (built-in exception)

   Equivalent to ``p.readline([n])``, this function reads one line from the
   object *p*.  *p* may be a file object or any object with a
   :meth:`~io.IOBase.readline`
   method.  If *n* is ``0``, exactly one line is read, regardless of the length of
   the line.  If *n* is greater than ``0``, no more than *n* bytes will be read
   from the file; a partial line can be returned.  In both cases, an empty string
   is returned if the end of the file is reached immediately.  If *n* is less than
   ``0``, however, one line is read regardless of length, but :exc:`EOFError` is
   raised if the end of the file is reached immediately.


.. c:function:: PyObject* PyFile_Name(PyObject *p)

   Return the name of the file specified by *p* as a string object.


.. c:function:: void PyFile_SetBufSize(PyFileObject *p, int n)

   .. index:: single: setvbuf()

   Available on systems with :c:func:`setvbuf` only.  This should only be called
   immediately after file object creation.


.. c:function:: int PyFile_SetEncoding(PyFileObject *p, const char *enc)

   Set the file's encoding for Unicode output to *enc*. Return 1 on success and 0
   on failure.

   .. versionadded:: 2.3


.. c:function:: int PyFile_SetEncodingAndErrors(PyFileObject *p, const char *enc, *errors)

   Set the file's encoding for Unicode output to *enc*, and its error
   mode to *err*. Return 1 on success and 0 on failure.

   .. versionadded:: 2.6


.. c:function:: int PyFile_SoftSpace(PyObject *p, int newflag)

   .. index:: single: softspace (file attribute)

   This function exists for internal use by the interpreter.  Set the
   :attr:`softspace` attribute of *p* to *newflag* and return the previous value.
   *p* does not have to be a file object for this function to work properly; any
   object is supported (thought its only interesting if the :attr:`softspace`
   attribute can be set).  This function clears any errors, and will return ``0``
   as the previous value if the attribute either does not exist or if there were
   errors in retrieving it.  There is no way to detect errors from this function,
   but doing so should not be needed.


.. c:function:: int PyFile_WriteObject(PyObject *obj, PyObject *p, int flags)

   .. index:: single: Py_PRINT_RAW

   Write object *obj* to file object *p*.  The only supported flag for *flags* is
   :const:`Py_PRINT_RAW`; if given, the :func:`str` of the object is written
   instead of the :func:`repr`.  Return ``0`` on success or ``-1`` on failure; the
   appropriate exception will be set.


.. c:function:: int PyFile_WriteString(const char *s, PyObject *p)

   Write string *s* to file object *p*.  Return ``0`` on success or ``-1`` on
   failure; the appropriate exception will be set.
