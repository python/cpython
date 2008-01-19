.. highlightlang:: c

.. _fileobjects:

File Objects
------------

.. index:: object: file

Python's built-in file objects are implemented entirely on the :ctype:`FILE\*`
support from the C standard library.  This is an implementation detail and may
change in future releases of Python.


.. ctype:: PyFileObject

   This subtype of :ctype:`PyObject` represents a Python file object.


.. cvar:: PyTypeObject PyFile_Type

   .. index:: single: FileType (in module types)

   This instance of :ctype:`PyTypeObject` represents the Python file type.  This is
   exposed to Python programs as ``file`` and ``types.FileType``.


.. cfunction:: int PyFile_Check(PyObject *p)

   Return true if its argument is a :ctype:`PyFileObject` or a subtype of
   :ctype:`PyFileObject`.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyFile_CheckExact(PyObject *p)

   Return true if its argument is a :ctype:`PyFileObject`, but not a subtype of
   :ctype:`PyFileObject`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyFile_FromString(char *filename, char *mode)

   .. index:: single: fopen()

   On success, return a new file object that is opened on the file given by
   *filename*, with a file mode given by *mode*, where *mode* has the same
   semantics as the standard C routine :cfunc:`fopen`.  On failure, return *NULL*.


.. cfunction:: PyObject* PyFile_FromFile(FILE *fp, char *name, char *mode, int (*close)(FILE*))

   Create a new :ctype:`PyFileObject` from the already-open standard C file
   pointer, *fp*.  The function *close* will be called when the file should be
   closed.  Return *NULL* on failure.


.. cfunction:: FILE* PyFile_AsFile(PyObject *p)

   Return the file object associated with *p* as a :ctype:`FILE\*`.


.. cfunction:: PyObject* PyFile_GetLine(PyObject *p, int n)

   .. index:: single: EOFError (built-in exception)

   Equivalent to ``p.readline([n])``, this function reads one line from the
   object *p*.  *p* may be a file object or any object with a :meth:`readline`
   method.  If *n* is ``0``, exactly one line is read, regardless of the length of
   the line.  If *n* is greater than ``0``, no more than *n* bytes will be read
   from the file; a partial line can be returned.  In both cases, an empty string
   is returned if the end of the file is reached immediately.  If *n* is less than
   ``0``, however, one line is read regardless of length, but :exc:`EOFError` is
   raised if the end of the file is reached immediately.


.. cfunction:: PyObject* PyFile_Name(PyObject *p)

   Return the name of the file specified by *p* as a string object.


.. cfunction:: void PyFile_SetBufSize(PyFileObject *p, int n)

   .. index:: single: setvbuf()

   Available on systems with :cfunc:`setvbuf` only.  This should only be called
   immediately after file object creation.


.. cfunction:: int PyFile_SetEncoding(PyFileObject *p, const char *enc)

   Set the file's encoding for Unicode output to *enc*. Return 1 on success and 0
   on failure.

   .. versionadded:: 2.3


.. cfunction:: int PyFile_SoftSpace(PyObject *p, int newflag)

   .. index:: single: softspace (file attribute)

   This function exists for internal use by the interpreter.  Set the
   :attr:`softspace` attribute of *p* to *newflag* and return the previous value.
   *p* does not have to be a file object for this function to work properly; any
   object is supported (thought its only interesting if the :attr:`softspace`
   attribute can be set).  This function clears any errors, and will return ``0``
   as the previous value if the attribute either does not exist or if there were
   errors in retrieving it.  There is no way to detect errors from this function,
   but doing so should not be needed.


.. cfunction:: int PyFile_WriteObject(PyObject *obj, PyObject *p, int flags)

   .. index:: single: Py_PRINT_RAW

   Write object *obj* to file object *p*.  The only supported flag for *flags* is
   :const:`Py_PRINT_RAW`; if given, the :func:`str` of the object is written
   instead of the :func:`repr`.  Return ``0`` on success or ``-1`` on failure; the
   appropriate exception will be set.


.. cfunction:: int PyFile_WriteString(const char *s, PyObject *p)

   Write string *s* to file object *p*.  Return ``0`` on success or ``-1`` on
   failure; the appropriate exception will be set.
