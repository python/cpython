.. highlightlang:: c


.. _exceptionhandling:

******************
Exception Handling
******************

The functions described in this chapter will let you handle and raise Python
exceptions.  It is important to understand some of the basics of Python
exception handling.  It works somewhat like the Unix :c:data:`errno` variable:
there is a global indicator (per thread) of the last error that occurred.  Most
functions don't clear this on success, but will set it to indicate the cause of
the error on failure.  Most functions also return an error indicator, usually
*NULL* if they are supposed to return a pointer, or ``-1`` if they return an
integer (exception: the :c:func:`PyArg_\*` functions return ``1`` for success and
``0`` for failure).

When a function must fail because some function it called failed, it generally
doesn't set the error indicator; the function it called already set it.  It is
responsible for either handling the error and clearing the exception or
returning after cleaning up any resources it holds (such as object references or
memory allocations); it should *not* continue normally if it is not prepared to
handle the error.  If returning due to an error, it is important to indicate to
the caller that an error has been set.  If the error is not handled or carefully
propagated, additional calls into the Python/C API may not behave as intended
and may fail in mysterious ways.

The error indicator consists of three Python objects corresponding to the result
of ``sys.exc_info()``.  API functions exist to interact with the error indicator
in various ways.  There is a separate error indicator for each thread.

.. XXX Order of these should be more thoughtful.
   Either alphabetical or some kind of structure.


.. c:function:: void PyErr_PrintEx(int set_sys_last_vars)

   Print a standard traceback to ``sys.stderr`` and clear the error indicator.
   Call this function only when the error indicator is set.  (Otherwise it will
   cause a fatal error!)

   If *set_sys_last_vars* is nonzero, the variables :data:`sys.last_type`,
   :data:`sys.last_value` and :data:`sys.last_traceback` will be set to the
   type, value and traceback of the printed exception, respectively.


.. c:function:: void PyErr_Print()

   Alias for ``PyErr_PrintEx(1)``.


.. c:function:: PyObject* PyErr_Occurred()

   Test whether the error indicator is set.  If set, return the exception *type*
   (the first argument to the last call to one of the :c:func:`PyErr_Set\*`
   functions or to :c:func:`PyErr_Restore`).  If not set, return *NULL*.  You do not
   own a reference to the return value, so you do not need to :c:func:`Py_DECREF`
   it.

   .. note::

      Do not compare the return value to a specific exception; use
      :c:func:`PyErr_ExceptionMatches` instead, shown below.  (The comparison could
      easily fail since the exception may be an instance instead of a class, in the
      case of a class exception, or it may the a subclass of the expected exception.)


.. c:function:: int PyErr_ExceptionMatches(PyObject *exc)

   Equivalent to ``PyErr_GivenExceptionMatches(PyErr_Occurred(), exc)``.  This
   should only be called when an exception is actually set; a memory access
   violation will occur if no exception has been raised.


.. c:function:: int PyErr_GivenExceptionMatches(PyObject *given, PyObject *exc)

   Return true if the *given* exception matches the exception in *exc*.  If
   *exc* is a class object, this also returns true when *given* is an instance
   of a subclass.  If *exc* is a tuple, all exceptions in the tuple (and
   recursively in subtuples) are searched for a match.


.. c:function:: void PyErr_NormalizeException(PyObject**exc, PyObject**val, PyObject**tb)

   Under certain circumstances, the values returned by :c:func:`PyErr_Fetch` below
   can be "unnormalized", meaning that ``*exc`` is a class object but ``*val`` is
   not an instance of the  same class.  This function can be used to instantiate
   the class in that case.  If the values are already normalized, nothing happens.
   The delayed normalization is implemented to improve performance.

   .. note::

      This function *does not* implicitly set the ``__traceback__``
      attribute on the exception value. If setting the traceback
      appropriately is desired, the following additional snippet is needed::

         if (tb != NULL) {
           PyException_SetTraceback(val, tb);
         }


.. c:function:: void PyErr_Clear()

   Clear the error indicator.  If the error indicator is not set, there is no
   effect.


.. c:function:: void PyErr_Fetch(PyObject **ptype, PyObject **pvalue, PyObject **ptraceback)

   Retrieve the error indicator into three variables whose addresses are passed.
   If the error indicator is not set, set all three variables to *NULL*.  If it is
   set, it will be cleared and you own a reference to each object retrieved.  The
   value and traceback object may be *NULL* even when the type object is not.

   .. note::

      This function is normally only used by code that needs to handle exceptions or
      by code that needs to save and restore the error indicator temporarily.


.. c:function:: void PyErr_Restore(PyObject *type, PyObject *value, PyObject *traceback)

   Set  the error indicator from the three objects.  If the error indicator is
   already set, it is cleared first.  If the objects are *NULL*, the error
   indicator is cleared.  Do not pass a *NULL* type and non-*NULL* value or
   traceback.  The exception type should be a class.  Do not pass an invalid
   exception type or value. (Violating these rules will cause subtle problems
   later.)  This call takes away a reference to each object: you must own a
   reference to each object before the call and after the call you no longer own
   these references.  (If you don't understand this, don't use this function.  I
   warned you.)

   .. note::

      This function is normally only used by code that needs to save and restore the
      error indicator temporarily; use :c:func:`PyErr_Fetch` to save the current
      exception state.


.. c:function:: void PyErr_GetExcInfo(PyObject **ptype, PyObject **pvalue, PyObject **ptraceback)

   Retrieve the exception info, as known from ``sys.exc_info()``.  This refers
   to an exception that was already caught, not to an exception that was
   freshly raised.  Returns new references for the three objects, any of which
   may be *NULL*.  Does not modify the exception info state.

   .. note::

      This function is not normally used by code that wants to handle exceptions.
      Rather, it can be used when code needs to save and restore the exception
      state temporarily.  Use :c:func:`PyErr_SetExcInfo` to restore or clear the
      exception state.

   .. versionadded:: 3.3


.. c:function:: void PyErr_SetExcInfo(PyObject *type, PyObject *value, PyObject *traceback)

   Set the exception info, as known from ``sys.exc_info()``.  This refers
   to an exception that was already caught, not to an exception that was
   freshly raised.  This function steals the references of the arguments.
   To clear the exception state, pass *NULL* for all three arguments.
   For general rules about the three arguments, see :c:func:`PyErr_Restore`.

   .. note::

      This function is not normally used by code that wants to handle exceptions.
      Rather, it can be used when code needs to save and restore the exception
      state temporarily.  Use :c:func:`PyErr_GetExcInfo` to read the exception
      state.

   .. versionadded:: 3.3


.. c:function:: void PyErr_SetString(PyObject *type, const char *message)

   This is the most common way to set the error indicator.  The first argument
   specifies the exception type; it is normally one of the standard exceptions,
   e.g. :c:data:`PyExc_RuntimeError`.  You need not increment its reference count.
   The second argument is an error message; it is decoded from ``'utf-8``'.


.. c:function:: void PyErr_SetObject(PyObject *type, PyObject *value)

   This function is similar to :c:func:`PyErr_SetString` but lets you specify an
   arbitrary Python object for the "value" of the exception.


.. c:function:: PyObject* PyErr_Format(PyObject *exception, const char *format, ...)

   This function sets the error indicator and returns *NULL*.  *exception*
   should be a Python exception class.  The *format* and subsequent
   parameters help format the error message; they have the same meaning and
   values as in :c:func:`PyUnicode_FromFormat`. *format* is an ASCII-encoded
   string.


.. c:function:: void PyErr_SetNone(PyObject *type)

   This is a shorthand for ``PyErr_SetObject(type, Py_None)``.


.. c:function:: int PyErr_BadArgument()

   This is a shorthand for ``PyErr_SetString(PyExc_TypeError, message)``, where
   *message* indicates that a built-in operation was invoked with an illegal
   argument.  It is mostly for internal use.


.. c:function:: PyObject* PyErr_NoMemory()

   This is a shorthand for ``PyErr_SetNone(PyExc_MemoryError)``; it returns *NULL*
   so an object allocation function can write ``return PyErr_NoMemory();`` when it
   runs out of memory.


.. c:function:: PyObject* PyErr_SetFromErrno(PyObject *type)

   .. index:: single: strerror()

   This is a convenience function to raise an exception when a C library function
   has returned an error and set the C variable :c:data:`errno`.  It constructs a
   tuple object whose first item is the integer :c:data:`errno` value and whose
   second item is the corresponding error message (gotten from :c:func:`strerror`),
   and then calls ``PyErr_SetObject(type, object)``.  On Unix, when the
   :c:data:`errno` value is :const:`EINTR`, indicating an interrupted system call,
   this calls :c:func:`PyErr_CheckSignals`, and if that set the error indicator,
   leaves it set to that.  The function always returns *NULL*, so a wrapper
   function around a system call can write ``return PyErr_SetFromErrno(type);``
   when the system call returns an error.


.. c:function:: PyObject* PyErr_SetFromErrnoWithFilenameObject(PyObject *type, PyObject *filenameObject)

   Similar to :c:func:`PyErr_SetFromErrno`, with the additional behavior that if
   *filenameObject* is not *NULL*, it is passed to the constructor of *type* as
   a third parameter.  In the case of exceptions such as :exc:`IOError` and
   :exc:`OSError`, this is used to define the :attr:`filename` attribute of the
   exception instance.


.. c:function:: PyObject* PyErr_SetFromErrnoWithFilenameObjects(PyObject *type, PyObject *filenameObject, PyObject *filenameObject2)

   Similar to :c:func:`PyErr_SetFromErrnoWithFilenameObject`, but takes a second
   filename object, for raising errors when a function that takes two filenames
   fails.

.. versionadded:: 3.4


.. c:function:: PyObject* PyErr_SetFromErrnoWithFilename(PyObject *type, const char *filename)

   Similar to :c:func:`PyErr_SetFromErrnoWithFilenameObject`, but the filename
   is given as a C string.  *filename* is decoded from the filesystem encoding
   (:func:`os.fsdecode`).


.. c:function:: PyObject* PyErr_SetFromWindowsErr(int ierr)

   This is a convenience function to raise :exc:`WindowsError`. If called with
   *ierr* of :c:data:`0`, the error code returned by a call to :c:func:`GetLastError`
   is used instead.  It calls the Win32 function :c:func:`FormatMessage` to retrieve
   the Windows description of error code given by *ierr* or :c:func:`GetLastError`,
   then it constructs a tuple object whose first item is the *ierr* value and whose
   second item is the corresponding error message (gotten from
   :c:func:`FormatMessage`), and then calls ``PyErr_SetObject(PyExc_WindowsError,
   object)``. This function always returns *NULL*. Availability: Windows.


.. c:function:: PyObject* PyErr_SetExcFromWindowsErr(PyObject *type, int ierr)

   Similar to :c:func:`PyErr_SetFromWindowsErr`, with an additional parameter
   specifying the exception type to be raised. Availability: Windows.


.. c:function:: PyObject* PyErr_SetFromWindowsErrWithFilename(int ierr, const char *filename)

   Similar to :c:func:`PyErr_SetFromWindowsErrWithFilenameObject`, but the
   filename is given as a C string.  *filename* is decoded from the filesystem
   encoding (:func:`os.fsdecode`).  Availability: Windows.


.. c:function:: PyObject* PyErr_SetExcFromWindowsErrWithFilenameObject(PyObject *type, int ierr, PyObject *filename)

   Similar to :c:func:`PyErr_SetFromWindowsErrWithFilenameObject`, with an
   additional parameter specifying the exception type to be raised.
   Availability: Windows.


.. c:function:: PyObject* PyErr_SetExcFromWindowsErrWithFilenameObjects(PyObject *type, int ierr, PyObject *filename, PyObject *filename2)

   Similar to :c:func:`PyErr_SetExcFromWindowsErrWithFilenameObject`,
   but accepts a second filename object.
   Availability: Windows.

.. versionadded:: 3.4


.. c:function:: PyObject* PyErr_SetExcFromWindowsErrWithFilename(PyObject *type, int ierr, const char *filename)

   Similar to :c:func:`PyErr_SetFromWindowsErrWithFilename`, with an additional
   parameter specifying the exception type to be raised. Availability: Windows.


.. c:function:: PyObject* PyErr_SetImportError(PyObject *msg, PyObject *name, PyObject *path)

   This is a convenience function to raise :exc:`ImportError`. *msg* will be
   set as the exception's message string. *name* and *path*, both of which can
   be ``NULL``, will be set as the :exc:`ImportError`'s respective ``name``
   and ``path`` attributes.

   .. versionadded:: 3.3


.. c:function:: void PyErr_SyntaxLocationObject(PyObject *filename, int lineno, int col_offset)

   Set file, line, and offset information for the current exception.  If the
   current exception is not a :exc:`SyntaxError`, then it sets additional
   attributes, which make the exception printing subsystem think the exception
   is a :exc:`SyntaxError`.

.. versionadded:: 3.4


.. c:function:: void PyErr_SyntaxLocationEx(char *filename, int lineno, int col_offset)

   Like :c:func:`PyErr_SyntaxLocationObject`, but *filename* is a byte string
   decoded from the filesystem encoding (:func:`os.fsdecode`).

.. versionadded:: 3.2


.. c:function:: void PyErr_SyntaxLocation(char *filename, int lineno)

   Like :c:func:`PyErr_SyntaxLocationEx`, but the col_offset parameter is
   omitted.


.. c:function:: void PyErr_BadInternalCall()

   This is a shorthand for ``PyErr_SetString(PyExc_SystemError, message)``,
   where *message* indicates that an internal operation (e.g. a Python/C API
   function) was invoked with an illegal argument.  It is mostly for internal
   use.


.. c:function:: int PyErr_WarnEx(PyObject *category, char *message, int stack_level)

   Issue a warning message.  The *category* argument is a warning category (see
   below) or *NULL*; the *message* argument is an UTF-8 encoded string.  *stack_level* is a
   positive number giving a number of stack frames; the warning will be issued from
   the  currently executing line of code in that stack frame.  A *stack_level* of 1
   is the function calling :c:func:`PyErr_WarnEx`, 2 is  the function above that,
   and so forth.

   This function normally prints a warning message to *sys.stderr*; however, it is
   also possible that the user has specified that warnings are to be turned into
   errors, and in that case this will raise an exception.  It is also possible that
   the function raises an exception because of a problem with the warning machinery
   (the implementation imports the :mod:`warnings` module to do the heavy lifting).
   The return value is ``0`` if no exception is raised, or ``-1`` if an exception
   is raised.  (It is not possible to determine whether a warning message is
   actually printed, nor what the reason is for the exception; this is
   intentional.)  If an exception is raised, the caller should do its normal
   exception handling (for example, :c:func:`Py_DECREF` owned references and return
   an error value).

   Warning categories must be subclasses of :c:data:`Warning`; the default warning
   category is :c:data:`RuntimeWarning`.  The standard Python warning categories are
   available as global variables whose names are ``PyExc_`` followed by the Python
   exception name. These have the type :c:type:`PyObject\*`; they are all class
   objects. Their names are :c:data:`PyExc_Warning`, :c:data:`PyExc_UserWarning`,
   :c:data:`PyExc_UnicodeWarning`, :c:data:`PyExc_DeprecationWarning`,
   :c:data:`PyExc_SyntaxWarning`, :c:data:`PyExc_RuntimeWarning`, and
   :c:data:`PyExc_FutureWarning`.  :c:data:`PyExc_Warning` is a subclass of
   :c:data:`PyExc_Exception`; the other warning categories are subclasses of
   :c:data:`PyExc_Warning`.

   For information about warning control, see the documentation for the
   :mod:`warnings` module and the :option:`-W` option in the command line
   documentation.  There is no C API for warning control.


.. c:function:: int PyErr_WarnExplicitObject(PyObject *category, PyObject *message, PyObject *filename, int lineno, PyObject *module, PyObject *registry)

   Issue a warning message with explicit control over all warning attributes.  This
   is a straightforward wrapper around the Python function
   :func:`warnings.warn_explicit`, see there for more information.  The *module*
   and *registry* arguments may be set to *NULL* to get the default effect
   described there.

   .. versionadded:: 3.4


.. c:function:: int PyErr_WarnExplicit(PyObject *category, const char *message, const char *filename, int lineno, const char *module, PyObject *registry)

   Similar to :c:func:`PyErr_WarnExplicitObject` except that *message* and
   *module* are UTF-8 encoded strings, and *filename* is decoded from the
   filesystem encoding (:func:`os.fsdecode`).


.. c:function:: int PyErr_WarnFormat(PyObject *category, Py_ssize_t stack_level, const char *format, ...)

   Function similar to :c:func:`PyErr_WarnEx`, but use
   :c:func:`PyUnicode_FromFormat` to format the warning message.  *format* is
   an ASCII-encoded string.

   .. versionadded:: 3.2


.. c:function:: int PyErr_CheckSignals()

   .. index::
      module: signal
      single: SIGINT
      single: KeyboardInterrupt (built-in exception)

   This function interacts with Python's signal handling.  It checks whether a
   signal has been sent to the processes and if so, invokes the corresponding
   signal handler.  If the :mod:`signal` module is supported, this can invoke a
   signal handler written in Python.  In all cases, the default effect for
   :const:`SIGINT` is to raise the  :exc:`KeyboardInterrupt` exception.  If an
   exception is raised the error indicator is set and the function returns ``-1``;
   otherwise the function returns ``0``.  The error indicator may or may not be
   cleared if it was previously set.


.. c:function:: void PyErr_SetInterrupt()

   .. index::
      single: SIGINT
      single: KeyboardInterrupt (built-in exception)

   This function simulates the effect of a :const:`SIGINT` signal arriving --- the
   next time :c:func:`PyErr_CheckSignals` is called,  :exc:`KeyboardInterrupt` will
   be raised.  It may be called without holding the interpreter lock.

   .. % XXX This was described as obsolete, but is used in
   .. % _thread.interrupt_main() (used from IDLE), so it's still needed.


.. c:function:: int PySignal_SetWakeupFd(int fd)

   This utility function specifies a file descriptor to which a ``'\0'`` byte will
   be written whenever a signal is received.  It returns the previous such file
   descriptor.  The value ``-1`` disables the feature; this is the initial state.
   This is equivalent to :func:`signal.set_wakeup_fd` in Python, but without any
   error checking.  *fd* should be a valid file descriptor.  The function should
   only be called from the main thread.


.. c:function:: PyObject* PyErr_NewException(char *name, PyObject *base, PyObject *dict)

   This utility function creates and returns a new exception class. The *name*
   argument must be the name of the new exception, a C string of the form
   ``module.classname``.  The *base* and *dict* arguments are normally *NULL*.
   This creates a class object derived from :exc:`Exception` (accessible in C as
   :c:data:`PyExc_Exception`).

   The :attr:`__module__` attribute of the new class is set to the first part (up
   to the last dot) of the *name* argument, and the class name is set to the last
   part (after the last dot).  The *base* argument can be used to specify alternate
   base classes; it can either be only one class or a tuple of classes. The *dict*
   argument can be used to specify a dictionary of class variables and methods.


.. c:function:: PyObject* PyErr_NewExceptionWithDoc(char *name, char *doc, PyObject *base, PyObject *dict)

   Same as :c:func:`PyErr_NewException`, except that the new exception class can
   easily be given a docstring: If *doc* is non-*NULL*, it will be used as the
   docstring for the exception class.

   .. versionadded:: 3.2


.. c:function:: void PyErr_WriteUnraisable(PyObject *obj)

   This utility function prints a warning message to ``sys.stderr`` when an
   exception has been set but it is impossible for the interpreter to actually
   raise the exception.  It is used, for example, when an exception occurs in an
   :meth:`__del__` method.

   The function is called with a single argument *obj* that identifies the context
   in which the unraisable exception occurred. The repr of *obj* will be printed in
   the warning message.


Exception Objects
=================

.. c:function:: PyObject* PyException_GetTraceback(PyObject *ex)

   Return the traceback associated with the exception as a new reference, as
   accessible from Python through :attr:`__traceback__`.  If there is no
   traceback associated, this returns *NULL*.


.. c:function:: int PyException_SetTraceback(PyObject *ex, PyObject *tb)

   Set the traceback associated with the exception to *tb*.  Use ``Py_None`` to
   clear it.


.. c:function:: PyObject* PyException_GetContext(PyObject *ex)

   Return the context (another exception instance during whose handling *ex* was
   raised) associated with the exception as a new reference, as accessible from
   Python through :attr:`__context__`.  If there is no context associated, this
   returns *NULL*.


.. c:function:: void PyException_SetContext(PyObject *ex, PyObject *ctx)

   Set the context associated with the exception to *ctx*.  Use *NULL* to clear
   it.  There is no type check to make sure that *ctx* is an exception instance.
   This steals a reference to *ctx*.


.. c:function:: PyObject* PyException_GetCause(PyObject *ex)

   Return the cause (either an exception instance, or :const:`None`,
   set by ``raise ... from ...``) associated with the exception as a new
   reference, as accessible from Python through :attr:`__cause__`.


.. c:function:: void PyException_SetCause(PyObject *ex, PyObject *cause)

   Set the cause associated with the exception to *cause*.  Use *NULL* to clear
   it.  There is no type check to make sure that *cause* is either an exception
   instance or :const:`None`.  This steals a reference to *cause*.

   :attr:`__suppress_context__` is implicitly set to ``True`` by this function.


.. _unicodeexceptions:

Unicode Exception Objects
=========================

The following functions are used to create and modify Unicode exceptions from C.

.. c:function:: PyObject* PyUnicodeDecodeError_Create(const char *encoding, const char *object, Py_ssize_t length, Py_ssize_t start, Py_ssize_t end, const char *reason)

   Create a :class:`UnicodeDecodeError` object with the attributes *encoding*,
   *object*, *length*, *start*, *end* and *reason*. *encoding* and *reason* are
   UTF-8 encoded strings.

.. c:function:: PyObject* PyUnicodeEncodeError_Create(const char *encoding, const Py_UNICODE *object, Py_ssize_t length, Py_ssize_t start, Py_ssize_t end, const char *reason)

   Create a :class:`UnicodeEncodeError` object with the attributes *encoding*,
   *object*, *length*, *start*, *end* and *reason*. *encoding* and *reason* are
   UTF-8 encoded strings.

.. c:function:: PyObject* PyUnicodeTranslateError_Create(const Py_UNICODE *object, Py_ssize_t length, Py_ssize_t start, Py_ssize_t end, const char *reason)

   Create a :class:`UnicodeTranslateError` object with the attributes *object*,
   *length*, *start*, *end* and *reason*. *reason* is an UTF-8 encoded string.

.. c:function:: PyObject* PyUnicodeDecodeError_GetEncoding(PyObject *exc)
                PyObject* PyUnicodeEncodeError_GetEncoding(PyObject *exc)

   Return the *encoding* attribute of the given exception object.

.. c:function:: PyObject* PyUnicodeDecodeError_GetObject(PyObject *exc)
                PyObject* PyUnicodeEncodeError_GetObject(PyObject *exc)
                PyObject* PyUnicodeTranslateError_GetObject(PyObject *exc)

   Return the *object* attribute of the given exception object.

.. c:function:: int PyUnicodeDecodeError_GetStart(PyObject *exc, Py_ssize_t *start)
                int PyUnicodeEncodeError_GetStart(PyObject *exc, Py_ssize_t *start)
                int PyUnicodeTranslateError_GetStart(PyObject *exc, Py_ssize_t *start)

   Get the *start* attribute of the given exception object and place it into
   *\*start*.  *start* must not be *NULL*.  Return ``0`` on success, ``-1`` on
   failure.

.. c:function:: int PyUnicodeDecodeError_SetStart(PyObject *exc, Py_ssize_t start)
                int PyUnicodeEncodeError_SetStart(PyObject *exc, Py_ssize_t start)
                int PyUnicodeTranslateError_SetStart(PyObject *exc, Py_ssize_t start)

   Set the *start* attribute of the given exception object to *start*.  Return
   ``0`` on success, ``-1`` on failure.

.. c:function:: int PyUnicodeDecodeError_GetEnd(PyObject *exc, Py_ssize_t *end)
                int PyUnicodeEncodeError_GetEnd(PyObject *exc, Py_ssize_t *end)
                int PyUnicodeTranslateError_GetEnd(PyObject *exc, Py_ssize_t *end)

   Get the *end* attribute of the given exception object and place it into
   *\*end*.  *end* must not be *NULL*.  Return ``0`` on success, ``-1`` on
   failure.

.. c:function:: int PyUnicodeDecodeError_SetEnd(PyObject *exc, Py_ssize_t end)
                int PyUnicodeEncodeError_SetEnd(PyObject *exc, Py_ssize_t end)
                int PyUnicodeTranslateError_SetEnd(PyObject *exc, Py_ssize_t end)

   Set the *end* attribute of the given exception object to *end*.  Return ``0``
   on success, ``-1`` on failure.

.. c:function:: PyObject* PyUnicodeDecodeError_GetReason(PyObject *exc)
                PyObject* PyUnicodeEncodeError_GetReason(PyObject *exc)
                PyObject* PyUnicodeTranslateError_GetReason(PyObject *exc)

   Return the *reason* attribute of the given exception object.

.. c:function:: int PyUnicodeDecodeError_SetReason(PyObject *exc, const char *reason)
                int PyUnicodeEncodeError_SetReason(PyObject *exc, const char *reason)
                int PyUnicodeTranslateError_SetReason(PyObject *exc, const char *reason)

   Set the *reason* attribute of the given exception object to *reason*.  Return
   ``0`` on success, ``-1`` on failure.


Recursion Control
=================

These two functions provide a way to perform safe recursive calls at the C
level, both in the core and in extension modules.  They are needed if the
recursive code does not necessarily invoke Python code (which tracks its
recursion depth automatically).

.. c:function:: int Py_EnterRecursiveCall(char *where)

   Marks a point where a recursive C-level call is about to be performed.

   If :const:`USE_STACKCHECK` is defined, this function checks if the OS
   stack overflowed using :c:func:`PyOS_CheckStack`.  In this is the case, it
   sets a :exc:`MemoryError` and returns a nonzero value.

   The function then checks if the recursion limit is reached.  If this is the
   case, a :exc:`RuntimeError` is set and a nonzero value is returned.
   Otherwise, zero is returned.

   *where* should be a string such as ``" in instance check"`` to be
   concatenated to the :exc:`RuntimeError` message caused by the recursion depth
   limit.

.. c:function:: void Py_LeaveRecursiveCall()

   Ends a :c:func:`Py_EnterRecursiveCall`.  Must be called once for each
   *successful* invocation of :c:func:`Py_EnterRecursiveCall`.

Properly implementing :c:member:`~PyTypeObject.tp_repr` for container types requires
special recursion handling.  In addition to protecting the stack,
:c:member:`~PyTypeObject.tp_repr` also needs to track objects to prevent cycles.  The
following two functions facilitate this functionality.  Effectively,
these are the C equivalent to :func:`reprlib.recursive_repr`.

.. c:function:: int Py_ReprEnter(PyObject *object)

   Called at the beginning of the :c:member:`~PyTypeObject.tp_repr` implementation to
   detect cycles.

   If the object has already been processed, the function returns a
   positive integer.  In that case the :c:member:`~PyTypeObject.tp_repr` implementation
   should return a string object indicating a cycle.  As examples,
   :class:`dict` objects return ``{...}`` and :class:`list` objects
   return ``[...]``.

   The function will return a negative integer if the recursion limit
   is reached.  In that case the :c:member:`~PyTypeObject.tp_repr` implementation should
   typically return ``NULL``.

   Otherwise, the function returns zero and the :c:member:`~PyTypeObject.tp_repr`
   implementation can continue normally.

.. c:function:: void Py_ReprLeave(PyObject *object)

   Ends a :c:func:`Py_ReprEnter`.  Must be called once for each
   invocation of :c:func:`Py_ReprEnter` that returns zero.


.. _standardexceptions:

Standard Exceptions
===================

All standard Python exceptions are available as global variables whose names are
``PyExc_`` followed by the Python exception name.  These have the type
:c:type:`PyObject\*`; they are all class objects.  For completeness, here are all
the variables:

+-----------------------------------------+---------------------------------+----------+
| C Name                                  | Python Name                     | Notes    |
+=========================================+=================================+==========+
| :c:data:`PyExc_BaseException`           | :exc:`BaseException`            | \(1)     |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_Exception`               | :exc:`Exception`                | \(1)     |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_ArithmeticError`         | :exc:`ArithmeticError`          | \(1)     |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_LookupError`             | :exc:`LookupError`              | \(1)     |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_AssertionError`          | :exc:`AssertionError`           |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_AttributeError`          | :exc:`AttributeError`           |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_BlockingIOError`         | :exc:`BlockingIOError`          |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_BrokenPipeError`         | :exc:`BrokenPipeError`          |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_ChildProcessError`       | :exc:`ChildProcessError`        |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_ConnectionError`         | :exc:`ConnectionError`          |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_ConnectionAbortedError`  | :exc:`ConnectionAbortedError`   |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_ConnectionRefusedError`  | :exc:`ConnectionRefusedError`   |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_ConnectionResetError`    | :exc:`ConnectionResetError`     |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_FileExistsError`         | :exc:`FileExistsError`          |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_FileNotFoundError`       | :exc:`FileNotFoundError`        |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_EOFError`                | :exc:`EOFError`                 |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_FloatingPointError`      | :exc:`FloatingPointError`       |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_ImportError`             | :exc:`ImportError`              |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_IndexError`              | :exc:`IndexError`               |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_InterruptedError`        | :exc:`InterruptedError`         |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_IsADirectoryError`       | :exc:`IsADirectoryError`        |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_KeyError`                | :exc:`KeyError`                 |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_KeyboardInterrupt`       | :exc:`KeyboardInterrupt`        |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_MemoryError`             | :exc:`MemoryError`              |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_NameError`               | :exc:`NameError`                |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_NotADirectoryError`      | :exc:`NotADirectoryError`       |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_NotImplementedError`     | :exc:`NotImplementedError`      |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_OSError`                 | :exc:`OSError`                  | \(1)     |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_OverflowError`           | :exc:`OverflowError`            |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_PermissionError`         | :exc:`PermissionError`          |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_ProcessLookupError`      | :exc:`ProcessLookupError`       |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_ReferenceError`          | :exc:`ReferenceError`           | \(2)     |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_RuntimeError`            | :exc:`RuntimeError`             |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_SyntaxError`             | :exc:`SyntaxError`              |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_SystemError`             | :exc:`SystemError`              |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_TimeoutError`            | :exc:`TimeoutError`             |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_SystemExit`              | :exc:`SystemExit`               |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_TypeError`               | :exc:`TypeError`                |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_ValueError`              | :exc:`ValueError`               |          |
+-----------------------------------------+---------------------------------+----------+
| :c:data:`PyExc_ZeroDivisionError`       | :exc:`ZeroDivisionError`        |          |
+-----------------------------------------+---------------------------------+----------+

.. versionadded:: 3.3
   :c:data:`PyExc_BlockingIOError`, :c:data:`PyExc_BrokenPipeError`,
   :c:data:`PyExc_ChildProcessError`, :c:data:`PyExc_ConnectionError`,
   :c:data:`PyExc_ConnectionAbortedError`, :c:data:`PyExc_ConnectionRefusedError`,
   :c:data:`PyExc_ConnectionResetError`, :c:data:`PyExc_FileExistsError`,
   :c:data:`PyExc_FileNotFoundError`, :c:data:`PyExc_InterruptedError`,
   :c:data:`PyExc_IsADirectoryError`, :c:data:`PyExc_NotADirectoryError`,
   :c:data:`PyExc_PermissionError`, :c:data:`PyExc_ProcessLookupError`
   and :c:data:`PyExc_TimeoutError` were introduced following :pep:`3151`.


These are compatibility aliases to :c:data:`PyExc_OSError`:

+-------------------------------------+----------+
| C Name                              | Notes    |
+=====================================+==========+
| :c:data:`PyExc_EnvironmentError`    |          |
+-------------------------------------+----------+
| :c:data:`PyExc_IOError`             |          |
+-------------------------------------+----------+
| :c:data:`PyExc_WindowsError`        | \(3)     |
+-------------------------------------+----------+

.. versionchanged:: 3.3
   These aliases used to be separate exception types.


.. index::
   single: PyExc_BaseException
   single: PyExc_Exception
   single: PyExc_ArithmeticError
   single: PyExc_LookupError
   single: PyExc_AssertionError
   single: PyExc_AttributeError
   single: PyExc_BlockingIOError
   single: PyExc_BrokenPipeError
   single: PyExc_ConnectionError
   single: PyExc_ConnectionAbortedError
   single: PyExc_ConnectionRefusedError
   single: PyExc_ConnectionResetError
   single: PyExc_EOFError
   single: PyExc_FileExistsError
   single: PyExc_FileNotFoundError
   single: PyExc_FloatingPointError
   single: PyExc_ImportError
   single: PyExc_IndexError
   single: PyExc_InterruptedError
   single: PyExc_IsADirectoryError
   single: PyExc_KeyError
   single: PyExc_KeyboardInterrupt
   single: PyExc_MemoryError
   single: PyExc_NameError
   single: PyExc_NotADirectoryError
   single: PyExc_NotImplementedError
   single: PyExc_OSError
   single: PyExc_OverflowError
   single: PyExc_PermissionError
   single: PyExc_ProcessLookupError
   single: PyExc_ReferenceError
   single: PyExc_RuntimeError
   single: PyExc_SyntaxError
   single: PyExc_SystemError
   single: PyExc_SystemExit
   single: PyExc_TimeoutError
   single: PyExc_TypeError
   single: PyExc_ValueError
   single: PyExc_ZeroDivisionError
   single: PyExc_EnvironmentError
   single: PyExc_IOError
   single: PyExc_WindowsError

Notes:

(1)
   This is a base class for other standard exceptions.

(2)
   This is the same as :exc:`weakref.ReferenceError`.

(3)
   Only defined on Windows; protect code that uses this by testing that the
   preprocessor macro ``MS_WINDOWS`` is defined.
