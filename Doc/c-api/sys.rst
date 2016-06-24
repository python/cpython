.. highlightlang:: c

.. _os:

Operating System Utilities
==========================

.. c:function:: PyObject* PyOS_FSPath(PyObject *path)

   Return the file system representation for *path*. If the object is a
   :class:`str` or :class:`bytes` object, then its reference count is
   incremented. If the object implements the :class:`os.PathLike` interface,
   then :meth:`~os.PathLike.__fspath__` is returned as long as it is a
   :class:`str` or :class:`bytes` object. Otherwise :exc:`TypeError` is raised
   and ``NULL`` is returned.

   .. versionadded:: 3.6


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

.. c:function:: wchar_t* Py_DecodeLocale(const char* arg, size_t *size)

   Decode a byte string from the locale encoding with the :ref:`surrogateescape
   error handler <surrogateescape>`: undecodable bytes are decoded as
   characters in range U+DC80..U+DCFF. If a byte sequence can be decoded as a
   surrogate character, escape the bytes using the surrogateescape error
   handler instead of decoding them.

   Return a pointer to a newly allocated wide character string, use
   :c:func:`PyMem_RawFree` to free the memory. If size is not ``NULL``, write
   the number of wide characters excluding the null character into ``*size``

   Return ``NULL`` on decoding error or memory allocation error. If *size* is
   not ``NULL``, ``*size`` is set to ``(size_t)-1`` on memory error or set to
   ``(size_t)-2`` on decoding error.

   Decoding errors should never happen, unless there is a bug in the C
   library.

   Use the :c:func:`Py_EncodeLocale` function to encode the character string
   back to a byte string.

   .. seealso::

      The :c:func:`PyUnicode_DecodeFSDefaultAndSize` and
      :c:func:`PyUnicode_DecodeLocaleAndSize` functions.

   .. versionadded:: 3.5


.. c:function:: char* Py_EncodeLocale(const wchar_t *text, size_t *error_pos)

   Encode a wide character string to the locale encoding with the
   :ref:`surrogateescape error handler <surrogateescape>`: surrogate characters
   in the range U+DC80..U+DCFF are converted to bytes 0x80..0xFF.

   Return a pointer to a newly allocated byte string, use :c:func:`PyMem_Free`
   to free the memory. Return ``NULL`` on encoding error or memory allocation
   error

   If error_pos is not ``NULL``, ``*error_pos`` is set to the index of the
   invalid character on encoding error, or set to ``(size_t)-1`` otherwise.

   Use the :c:func:`Py_DecodeLocale` function to decode the bytes string back
   to a wide character string.

   .. seealso::

      The :c:func:`PyUnicode_EncodeFSDefault` and
      :c:func:`PyUnicode_EncodeLocale` functions.

   .. versionadded:: 3.5


.. _systemfunctions:

System Functions
================

These are utility functions that make functionality from the :mod:`sys` module
accessible to C code.  They all work with the current interpreter thread's
:mod:`sys` module's dict, which is contained in the internal thread state structure.

.. c:function:: PyObject *PySys_GetObject(const char *name)

   Return the object *name* from the :mod:`sys` module or *NULL* if it does
   not exist, without setting an exception.

.. c:function:: int PySys_SetObject(const char *name, PyObject *v)

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
      single: Py_FinalizeEx()
      single: exit()

   Exit the current process.  This calls :c:func:`Py_FinalizeEx` and then calls the
   standard C library function ``exit(status)``.  If :c:func:`Py_FinalizeEx`
   indicates an error, the exit status is set to 120.

   .. versionchanged:: 3.6
      Errors from finalization no longer ignored.


.. c:function:: int Py_AtExit(void (*func) ())

   .. index::
      single: Py_FinalizeEx()
      single: cleanup functions

   Register a cleanup function to be called by :c:func:`Py_FinalizeEx`.  The cleanup
   function will be called with no arguments and should return no value.  At most
   32 cleanup functions can be registered.  When the registration is successful,
   :c:func:`Py_AtExit` returns ``0``; on failure, it returns ``-1``.  The cleanup
   function registered last is called first. Each cleanup function will be called
   at most once.  Since Python's internal finalization will have completed before
   the cleanup function, no Python APIs should be called by *func*.
