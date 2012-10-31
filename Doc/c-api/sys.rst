.. highlightlang:: c

.. _os:

Operating System Utilities
==========================


.. c:function:: int Py_FdIsInteractive(FILE *fp, const char *filename)

   Return true (nonzero) if the standard I/O file *fp* with name *filename* is
   deemed interactive.  This is the case for files for which ``isatty(fileno(fp))``
   is true.  If the global flag :c:data:`Py_InteractiveFlag` is true, this function
   also returns true if the *filename* pointer is *NULL* or if the name is equal to
   one of the strings ``'<stdin>'`` or ``'???'``.


.. c:function:: void PyOS_AfterFork()

   Function to update some internal state after a process fork; this should be
   called in the new process if the Python interpreter will continue to be used.
   If a new executable is loaded into the new process, this function does not need
   to be called.


.. c:function:: int PyOS_CheckStack()

   Return true when the interpreter runs out of stack space.  This is a reliable
   check, but is only available when :const:`USE_STACKCHECK` is defined (currently
   on Windows using the Microsoft Visual C++ compiler).  :const:`USE_STACKCHECK`
   will be defined automatically; you should never change the definition in your
   own code.


.. c:function:: PyOS_sighandler_t PyOS_getsig(int i)

   Return the current signal handler for signal *i*.  This is a thin wrapper around
   either :c:func:`sigaction` or :c:func:`signal`.  Do not call those functions
   directly! :c:type:`PyOS_sighandler_t` is a typedef alias for :c:type:`void
   (\*)(int)`.


.. c:function:: PyOS_sighandler_t PyOS_setsig(int i, PyOS_sighandler_t h)

   Set the signal handler for signal *i* to be *h*; return the old signal handler.
   This is a thin wrapper around either :c:func:`sigaction` or :c:func:`signal`.  Do
   not call those functions directly!  :c:type:`PyOS_sighandler_t` is a typedef
   alias for :c:type:`void (\*)(int)`.

.. _systemfunctions:

System Functions
================

These are utility functions that make functionality from the :mod:`sys` module
accessible to C code.  They all work with the current interpreter thread's
:mod:`sys` module's dict, which is contained in the internal thread state structure.

.. c:function:: PyObject *PySys_GetObject(char *name)

   Return the object *name* from the :mod:`sys` module or *NULL* if it does
   not exist, without setting an exception.

.. c:function:: int PySys_SetObject(char *name, PyObject *v)

   Set *name* in the :mod:`sys` module to *v* unless *v* is *NULL*, in which
   case *name* is deleted from the sys module. Returns ``0`` on success, ``-1``
   on error.

.. c:function:: void PySys_ResetWarnOptions()

   Reset :data:`sys.warnoptions` to an empty list.

.. c:function:: void PySys_AddWarnOption(wchar_t *s)

   Append *s* to :data:`sys.warnoptions`.

.. c:function:: void PySys_AddWarnOptionUnicode(PyObject *unicode)

   Append *unicode* to :data:`sys.warnoptions`.

.. c:function:: void PySys_SetPath(wchar_t *path)

   Set :data:`sys.path` to a list object of paths found in *path* which should
   be a list of paths separated with the platform's search path delimiter
   (``:`` on Unix, ``;`` on Windows).

.. c:function:: void PySys_WriteStdout(const char *format, ...)

   Write the output string described by *format* to :data:`sys.stdout`.  No
   exceptions are raised, even if truncation occurs (see below).

   *format* should limit the total size of the formatted output string to
   1000 bytes or less -- after 1000 bytes, the output string is truncated.
   In particular, this means that no unrestricted "%s" formats should occur;
   these should be limited using "%.<N>s" where <N> is a decimal number
   calculated so that <N> plus the maximum size of other formatted text does not
   exceed 1000 bytes.  Also watch out for "%f", which can print hundreds of
   digits for very large numbers.

   If a problem occurs, or :data:`sys.stdout` is unset, the formatted message
   is written to the real (C level) *stdout*.

.. c:function:: void PySys_WriteStderr(const char *format, ...)

   As :c:func:`PySys_WriteStdout`, but write to :data:`sys.stderr` or *stderr*
   instead.

.. c:function:: void PySys_FormatStdout(const char *format, ...)

   Function similar to PySys_WriteStdout() but format the message using
   :c:func:`PyUnicode_FromFormatV` and don't truncate the message to an
   arbitrary length.

   .. versionadded:: 3.2

.. c:function:: void PySys_FormatStderr(const char *format, ...)

   As :c:func:`PySys_FormatStdout`, but write to :data:`sys.stderr` or *stderr*
   instead.

   .. versionadded:: 3.2

.. c:function:: void PySys_AddXOption(const wchar_t *s)

   Parse *s* as a set of :option:`-X` options and add them to the current
   options mapping as returned by :c:func:`PySys_GetXOptions`.

   .. versionadded:: 3.2

.. c:function:: PyObject *PySys_GetXOptions()

   Return the current dictionary of :option:`-X` options, similarly to
   :data:`sys._xoptions`.  On error, *NULL* is returned and an exception is
   set.

   .. versionadded:: 3.2


.. _processcontrol:

Process Control
===============


.. c:function:: void Py_FatalError(const char *message)

   .. index:: single: abort()

   Print a fatal error message and kill the process.  No cleanup is performed.
   This function should only be invoked when a condition is detected that would
   make it dangerous to continue using the Python interpreter; e.g., when the
   object administration appears to be corrupted.  On Unix, the standard C library
   function :c:func:`abort` is called which will attempt to produce a :file:`core`
   file.


.. c:function:: void Py_Exit(int status)

   .. index::
      single: Py_Finalize()
      single: exit()

   Exit the current process.  This calls :c:func:`Py_Finalize` and then calls the
   standard C library function ``exit(status)``.


.. c:function:: int Py_AtExit(void (*func) ())

   .. index::
      single: Py_Finalize()
      single: cleanup functions

   Register a cleanup function to be called by :c:func:`Py_Finalize`.  The cleanup
   function will be called with no arguments and should return no value.  At most
   32 cleanup functions can be registered.  When the registration is successful,
   :c:func:`Py_AtExit` returns ``0``; on failure, it returns ``-1``.  The cleanup
   function registered last is called first. Each cleanup function will be called
   at most once.  Since Python's internal finalization will have completed before
   the cleanup function, no Python APIs should be called by *func*.
