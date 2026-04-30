.. highlight:: c


.. _traceback:

**********
Tracebacks
**********

The functions below collect Python stack frames into a caller-supplied array of
:c:type:`PyUnstable_FrameInfo` structs.  Because they do not acquire or release
the GIL or allocate heap memory, they can be called from signal handlers and
are suitable for low-overhead observability tools such as sampling profilers
and tracers.

.. c:function:: int PyUnstable_CollectCallStack(PyThreadState *tstate, PyUnstable_FrameInfo *frames, int max_frames)

   Collect up to *max_frames* frames from *tstate* into the caller-supplied
   *frames* array and return the number of frames written (0..*max_frames*).
   Returns ``-1`` if *frames* is NULL, *tstate* is freed, or *tstate* has no
   current Python frame.

   Filenames and function names are ASCII-encoded (non-ASCII characters are
   backslash-escaped) and truncated to 500 characters; if truncated, the
   corresponding ``filename_truncated`` or ``name_truncated`` field is set
   to ``1``.

   In crash scenarios such as signal handlers for SIGSEGV, where the
   interpreter may be in an inconsistent state, the function might produce
   incomplete output or it may even crash itself.

   The caller does not need to hold an attached thread state, nor does *tstate*
   need to be attached.

   This function does not acquire or release the GIL, modify reference counts,
   or allocate heap memory.

   .. versionadded:: next

.. c:function:: void PyUnstable_PrintCallStack(int fd, const PyUnstable_FrameInfo *frames, int n_frames, int write_header)

   Write a traceback collected by :c:func:`PyUnstable_CollectCallStack` to
   *fd*.  The format looks like::

      Stack (most recent call first):
        File "foo/bar.py", line 42 in myfunc
        File "foo/bar.py", line 99 in caller

   Pass *write_header* as ``1`` to emit the ``Stack (most recent call first):``
   header line, or ``0`` to omit it.  Truncated filenames and function names
   are followed by ``...``.

   This function only reads the caller-supplied *frames* array and does not
   access interpreter state.  It is async-signal-safe: it does not acquire or
   release the GIL, modify reference counts, or allocate heap memory, and its
   only I/O is via :c:func:`!write`.

   .. versionadded:: next

.. c:type:: PyUnstable_FrameInfo

   A plain-data struct representing a single Python stack frame, suitable for
   use in crash-handling code.  Populated by
   :c:func:`PyUnstable_CollectCallStack`.

   .. c:member:: int lineno

      The line number, or ``-1`` if unknown.

   .. c:member:: int filename_truncated

      ``1`` if :c:member:`filename` was truncated, ``0`` otherwise.

   .. c:member:: int name_truncated

      ``1`` if :c:member:`name` was truncated, ``0`` otherwise.

   .. c:member:: char filename[Py_UNSTABLE_FRAMEINFO_STRSIZE]

      The source filename, ASCII-encoded with ``backslashreplace`` and
      null-terminated.  Empty string if unavailable.

   .. c:member:: char name[Py_UNSTABLE_FRAMEINFO_STRSIZE]

      The function name, ASCII-encoded with ``backslashreplace`` and
      null-terminated.  Empty string if unavailable.

   .. versionadded:: next

.. c:macro:: Py_UNSTABLE_FRAMEINFO_STRSIZE

   The size in bytes of the :c:member:`PyUnstable_FrameInfo.filename` and
   :c:member:`PyUnstable_FrameInfo.name` character arrays (``501``): up to
   500 content bytes plus a null terminator.

   .. versionadded:: next
