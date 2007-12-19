.. highlightlang:: c


.. _exceptionhandling:

******************
Exception Handling
******************

The functions described in this chapter will let you handle and raise Python
exceptions.  It is important to understand some of the basics of Python
exception handling.  It works somewhat like the Unix :cdata:`errno` variable:
there is a global indicator (per thread) of the last error that occurred.  Most
functions don't clear this on success, but will set it to indicate the cause of
the error on failure.  Most functions also return an error indicator, usually
*NULL* if they are supposed to return a pointer, or ``-1`` if they return an
integer (exception: the :cfunc:`PyArg_\*` functions return ``1`` for success and
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

.. index::
   single: exc_type (in module sys)
   single: exc_value (in module sys)
   single: exc_traceback (in module sys)

The error indicator consists of three Python objects corresponding to   the
Python variables ``sys.exc_type``, ``sys.exc_value`` and ``sys.exc_traceback``.
API functions exist to interact with the error indicator in various ways.  There
is a separate error indicator for each thread.

.. % XXX Order of these should be more thoughtful.
.. % Either alphabetical or some kind of structure.


.. cfunction:: void PyErr_Print()

   Print a standard traceback to ``sys.stderr`` and clear the error indicator.
   Call this function only when the error indicator is set.  (Otherwise it will
   cause a fatal error!)


.. cfunction:: PyObject* PyErr_Occurred()

   Test whether the error indicator is set.  If set, return the exception *type*
   (the first argument to the last call to one of the :cfunc:`PyErr_Set\*`
   functions or to :cfunc:`PyErr_Restore`).  If not set, return *NULL*.  You do not
   own a reference to the return value, so you do not need to :cfunc:`Py_DECREF`
   it.

   .. note::

      Do not compare the return value to a specific exception; use
      :cfunc:`PyErr_ExceptionMatches` instead, shown below.  (The comparison could
      easily fail since the exception may be an instance instead of a class, in the
      case of a class exception, or it may the a subclass of the expected exception.)


.. cfunction:: int PyErr_ExceptionMatches(PyObject *exc)

   Equivalent to ``PyErr_GivenExceptionMatches(PyErr_Occurred(), exc)``.  This
   should only be called when an exception is actually set; a memory access
   violation will occur if no exception has been raised.


.. cfunction:: int PyErr_GivenExceptionMatches(PyObject *given, PyObject *exc)

   Return true if the *given* exception matches the exception in *exc*.  If *exc*
   is a class object, this also returns true when *given* is an instance of a
   subclass.  If *exc* is a tuple, all exceptions in the tuple (and recursively in
   subtuples) are searched for a match.  If *given* is *NULL*, a memory access
   violation will occur.


.. cfunction:: void PyErr_NormalizeException(PyObject**exc, PyObject**val, PyObject**tb)

   Under certain circumstances, the values returned by :cfunc:`PyErr_Fetch` below
   can be "unnormalized", meaning that ``*exc`` is a class object but ``*val`` is
   not an instance of the  same class.  This function can be used to instantiate
   the class in that case.  If the values are already normalized, nothing happens.
   The delayed normalization is implemented to improve performance.


.. cfunction:: void PyErr_Clear()

   Clear the error indicator.  If the error indicator is not set, there is no
   effect.


.. cfunction:: void PyErr_Fetch(PyObject **ptype, PyObject **pvalue, PyObject **ptraceback)

   Retrieve the error indicator into three variables whose addresses are passed.
   If the error indicator is not set, set all three variables to *NULL*.  If it is
   set, it will be cleared and you own a reference to each object retrieved.  The
   value and traceback object may be *NULL* even when the type object is not.

   .. note::

      This function is normally only used by code that needs to handle exceptions or
      by code that needs to save and restore the error indicator temporarily.


.. cfunction:: void PyErr_Restore(PyObject *type, PyObject *value, PyObject *traceback)

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
      error indicator temporarily; use :cfunc:`PyErr_Fetch` to save the current
      exception state.


.. cfunction:: void PyErr_SetString(PyObject *type, const char *message)

   This is the most common way to set the error indicator.  The first argument
   specifies the exception type; it is normally one of the standard exceptions,
   e.g. :cdata:`PyExc_RuntimeError`.  You need not increment its reference count.
   The second argument is an error message; it is converted to a string object.


.. cfunction:: void PyErr_SetObject(PyObject *type, PyObject *value)

   This function is similar to :cfunc:`PyErr_SetString` but lets you specify an
   arbitrary Python object for the "value" of the exception.


.. cfunction:: PyObject* PyErr_Format(PyObject *exception, const char *format, ...)

   This function sets the error indicator and returns *NULL*. *exception* should be
   a Python exception (class, not an instance).  *format* should be a string,
   containing format codes, similar to :cfunc:`printf`. The ``width.precision``
   before a format code is parsed, but the width part is ignored.

   .. % This should be exactly the same as the table in PyString_FromFormat.
   .. % One should just refer to the other.
   .. % The descriptions for %zd and %zu are wrong, but the truth is complicated
   .. % because not all compilers support the %z width modifier -- we fake it
   .. % when necessary via interpolating PY_FORMAT_SIZE_T.
   .. % %u, %lu, %zu should have "new in Python 2.5" blurbs.

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


.. cfunction:: void PyErr_SetNone(PyObject *type)

   This is a shorthand for ``PyErr_SetObject(type, Py_None)``.


.. cfunction:: int PyErr_BadArgument()

   This is a shorthand for ``PyErr_SetString(PyExc_TypeError, message)``, where
   *message* indicates that a built-in operation was invoked with an illegal
   argument.  It is mostly for internal use.


.. cfunction:: PyObject* PyErr_NoMemory()

   This is a shorthand for ``PyErr_SetNone(PyExc_MemoryError)``; it returns *NULL*
   so an object allocation function can write ``return PyErr_NoMemory();`` when it
   runs out of memory.


.. cfunction:: PyObject* PyErr_SetFromErrno(PyObject *type)

   .. index:: single: strerror()

   This is a convenience function to raise an exception when a C library function
   has returned an error and set the C variable :cdata:`errno`.  It constructs a
   tuple object whose first item is the integer :cdata:`errno` value and whose
   second item is the corresponding error message (gotten from :cfunc:`strerror`),
   and then calls ``PyErr_SetObject(type, object)``.  On Unix, when the
   :cdata:`errno` value is :const:`EINTR`, indicating an interrupted system call,
   this calls :cfunc:`PyErr_CheckSignals`, and if that set the error indicator,
   leaves it set to that.  The function always returns *NULL*, so a wrapper
   function around a system call can write ``return PyErr_SetFromErrno(type);``
   when the system call returns an error.


.. cfunction:: PyObject* PyErr_SetFromErrnoWithFilename(PyObject *type, const char *filename)

   Similar to :cfunc:`PyErr_SetFromErrno`, with the additional behavior that if
   *filename* is not *NULL*, it is passed to the constructor of *type* as a third
   parameter.  In the case of exceptions such as :exc:`IOError` and :exc:`OSError`,
   this is used to define the :attr:`filename` attribute of the exception instance.


.. cfunction:: PyObject* PyErr_SetFromWindowsErr(int ierr)

   This is a convenience function to raise :exc:`WindowsError`. If called with
   *ierr* of :cdata:`0`, the error code returned by a call to :cfunc:`GetLastError`
   is used instead.  It calls the Win32 function :cfunc:`FormatMessage` to retrieve
   the Windows description of error code given by *ierr* or :cfunc:`GetLastError`,
   then it constructs a tuple object whose first item is the *ierr* value and whose
   second item is the corresponding error message (gotten from
   :cfunc:`FormatMessage`), and then calls ``PyErr_SetObject(PyExc_WindowsError,
   object)``. This function always returns *NULL*. Availability: Windows.


.. cfunction:: PyObject* PyErr_SetExcFromWindowsErr(PyObject *type, int ierr)

   Similar to :cfunc:`PyErr_SetFromWindowsErr`, with an additional parameter
   specifying the exception type to be raised. Availability: Windows.

   .. versionadded:: 2.3


.. cfunction:: PyObject* PyErr_SetFromWindowsErrWithFilename(int ierr, const char *filename)

   Similar to :cfunc:`PyErr_SetFromWindowsErr`, with the additional behavior that
   if *filename* is not *NULL*, it is passed to the constructor of
   :exc:`WindowsError` as a third parameter. Availability: Windows.


.. cfunction:: PyObject* PyErr_SetExcFromWindowsErrWithFilename(PyObject *type, int ierr, char *filename)

   Similar to :cfunc:`PyErr_SetFromWindowsErrWithFilename`, with an additional
   parameter specifying the exception type to be raised. Availability: Windows.

   .. versionadded:: 2.3


.. cfunction:: void PyErr_BadInternalCall()

   This is a shorthand for ``PyErr_SetString(PyExc_TypeError, message)``, where
   *message* indicates that an internal operation (e.g. a Python/C API function)
   was invoked with an illegal argument.  It is mostly for internal use.


.. cfunction:: int PyErr_WarnEx(PyObject *category, char *message, int stacklevel)

   Issue a warning message.  The *category* argument is a warning category (see
   below) or *NULL*; the *message* argument is a message string.  *stacklevel* is a
   positive number giving a number of stack frames; the warning will be issued from
   the  currently executing line of code in that stack frame.  A *stacklevel* of 1
   is the function calling :cfunc:`PyErr_WarnEx`, 2 is  the function above that,
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
   exception handling (for example, :cfunc:`Py_DECREF` owned references and return
   an error value).

   Warning categories must be subclasses of :cdata:`Warning`; the default warning
   category is :cdata:`RuntimeWarning`.  The standard Python warning categories are
   available as global variables whose names are ``PyExc_`` followed by the Python
   exception name. These have the type :ctype:`PyObject\*`; they are all class
   objects. Their names are :cdata:`PyExc_Warning`, :cdata:`PyExc_UserWarning`,
   :cdata:`PyExc_UnicodeWarning`, :cdata:`PyExc_DeprecationWarning`,
   :cdata:`PyExc_SyntaxWarning`, :cdata:`PyExc_RuntimeWarning`, and
   :cdata:`PyExc_FutureWarning`.  :cdata:`PyExc_Warning` is a subclass of
   :cdata:`PyExc_Exception`; the other warning categories are subclasses of
   :cdata:`PyExc_Warning`.

   For information about warning control, see the documentation for the
   :mod:`warnings` module and the :option:`-W` option in the command line
   documentation.  There is no C API for warning control.


.. cfunction:: int PyErr_Warn(PyObject *category, char *message)

   Issue a warning message.  The *category* argument is a warning category (see
   below) or *NULL*; the *message* argument is a message string.  The warning will
   appear to be issued from the function calling :cfunc:`PyErr_Warn`, equivalent to
   calling :cfunc:`PyErr_WarnEx` with a *stacklevel* of 1.

   Deprecated; use :cfunc:`PyErr_WarnEx` instead.


.. cfunction:: int PyErr_WarnExplicit(PyObject *category, const char *message, const char *filename, int lineno, const char *module, PyObject *registry)

   Issue a warning message with explicit control over all warning attributes.  This
   is a straightforward wrapper around the Python function
   :func:`warnings.warn_explicit`, see there for more information.  The *module*
   and *registry* arguments may be set to *NULL* to get the default effect
   described there.


.. cfunction:: int PyErr_CheckSignals()

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


.. cfunction:: void PyErr_SetInterrupt()

   .. index::
      single: SIGINT
      single: KeyboardInterrupt (built-in exception)

   This function simulates the effect of a :const:`SIGINT` signal arriving --- the
   next time :cfunc:`PyErr_CheckSignals` is called,  :exc:`KeyboardInterrupt` will
   be raised.  It may be called without holding the interpreter lock.

   .. % XXX This was described as obsolete, but is used in
   .. % thread.interrupt_main() (used from IDLE), so it's still needed.


.. cfunction:: int PySignal_SetWakeupFd(int fd)

   This utility function specifies a file descriptor to which a ``'\0'`` byte will
   be written whenever a signal is received.  It returns the previous such file
   descriptor.  The value ``-1`` disables the feature; this is the initial state.
   This is equivalent to :func:`signal.set_wakeup_fd` in Python, but without any
   error checking.  *fd* should be a valid file descriptor.  The function should
   only be called from the main thread.


.. cfunction:: PyObject* PyErr_NewException(char *name, PyObject *base, PyObject *dict)

   This utility function creates and returns a new exception object. The *name*
   argument must be the name of the new exception, a C string of the form
   ``module.class``.  The *base* and *dict* arguments are normally *NULL*.  This
   creates a class object derived from :exc:`Exception` (accessible in C as
   :cdata:`PyExc_Exception`).

   The :attr:`__module__` attribute of the new class is set to the first part (up
   to the last dot) of the *name* argument, and the class name is set to the last
   part (after the last dot).  The *base* argument can be used to specify alternate
   base classes; it can either be only one class or a tuple of classes. The *dict*
   argument can be used to specify a dictionary of class variables and methods.


.. cfunction:: void PyErr_WriteUnraisable(PyObject *obj)

   This utility function prints a warning message to ``sys.stderr`` when an
   exception has been set but it is impossible for the interpreter to actually
   raise the exception.  It is used, for example, when an exception occurs in an
   :meth:`__del__` method.

   The function is called with a single argument *obj* that identifies the context
   in which the unraisable exception occurred. The repr of *obj* will be printed in
   the warning message.


.. _standardexceptions:

Standard Exceptions
===================

All standard Python exceptions are available as global variables whose names are
``PyExc_`` followed by the Python exception name.  These have the type
:ctype:`PyObject\*`; they are all class objects.  For completeness, here are all
the variables:

+------------------------------------+----------------------------+----------+
| C Name                             | Python Name                | Notes    |
+====================================+============================+==========+
| :cdata:`PyExc_BaseException`       | :exc:`BaseException`       | (1), (4) |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_Exception`           | :exc:`Exception`           | \(1)     |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_StandardError`       | :exc:`StandardError`       | \(1)     |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_ArithmeticError`     | :exc:`ArithmeticError`     | \(1)     |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_LookupError`         | :exc:`LookupError`         | \(1)     |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_AssertionError`      | :exc:`AssertionError`      |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_AttributeError`      | :exc:`AttributeError`      |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_EOFError`            | :exc:`EOFError`            |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_EnvironmentError`    | :exc:`EnvironmentError`    | \(1)     |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_FloatingPointError`  | :exc:`FloatingPointError`  |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_IOError`             | :exc:`IOError`             |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_ImportError`         | :exc:`ImportError`         |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_IndexError`          | :exc:`IndexError`          |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_KeyError`            | :exc:`KeyError`            |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_KeyboardInterrupt`   | :exc:`KeyboardInterrupt`   |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_MemoryError`         | :exc:`MemoryError`         |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_NameError`           | :exc:`NameError`           |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_NotImplementedError` | :exc:`NotImplementedError` |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_OSError`             | :exc:`OSError`             |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_OverflowError`       | :exc:`OverflowError`       |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_ReferenceError`      | :exc:`ReferenceError`      | \(2)     |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_RuntimeError`        | :exc:`RuntimeError`        |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_SyntaxError`         | :exc:`SyntaxError`         |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_SystemError`         | :exc:`SystemError`         |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_SystemExit`          | :exc:`SystemExit`          |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_TypeError`           | :exc:`TypeError`           |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_ValueError`          | :exc:`ValueError`          |          |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_WindowsError`        | :exc:`WindowsError`        | \(3)     |
+------------------------------------+----------------------------+----------+
| :cdata:`PyExc_ZeroDivisionError`   | :exc:`ZeroDivisionError`   |          |
+------------------------------------+----------------------------+----------+

.. index::
   single: PyExc_BaseException
   single: PyExc_Exception
   single: PyExc_StandardError
   single: PyExc_ArithmeticError
   single: PyExc_LookupError
   single: PyExc_AssertionError
   single: PyExc_AttributeError
   single: PyExc_EOFError
   single: PyExc_EnvironmentError
   single: PyExc_FloatingPointError
   single: PyExc_IOError
   single: PyExc_ImportError
   single: PyExc_IndexError
   single: PyExc_KeyError
   single: PyExc_KeyboardInterrupt
   single: PyExc_MemoryError
   single: PyExc_NameError
   single: PyExc_NotImplementedError
   single: PyExc_OSError
   single: PyExc_OverflowError
   single: PyExc_ReferenceError
   single: PyExc_RuntimeError
   single: PyExc_SyntaxError
   single: PyExc_SystemError
   single: PyExc_SystemExit
   single: PyExc_TypeError
   single: PyExc_ValueError
   single: PyExc_WindowsError
   single: PyExc_ZeroDivisionError

Notes:

(1)
   This is a base class for other standard exceptions.

(2)
   This is the same as :exc:`weakref.ReferenceError`.

(3)
   Only defined on Windows; protect code that uses this by testing that the
   preprocessor macro ``MS_WINDOWS`` is defined.

(4)
   .. versionadded:: 2.5


Deprecation of String Exceptions
================================

.. index:: single: BaseException (built-in exception)

All exceptions built into Python or provided in the standard library are derived
from :exc:`BaseException`.

String exceptions are still supported in the interpreter to allow existing code
to run unmodified, but this will also change in a future release.

