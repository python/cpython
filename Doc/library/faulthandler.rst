:mod:`faulthandler` --- Dump the Python traceback
=================================================

.. module:: faulthandler
   :synopsis: Dump the Python traceback.

This module contains functions to dump the Python traceback explicitly, on a
fault, after a timeout or on a user signal. Call :func:`faulthandler.enable` to
install fault handlers for :const:`SIGSEGV`, :const:`SIGFPE`, :const:`SIGBUS`
and :const:`SIGILL` signals. You can also enable them at startup by setting the
:envvar:`PYTHONFAULTHANDLER` environment variable or by using :option:`-X`
``faulthandler`` command line option.

The fault handler is compatible with system fault handlers like Apport or
the Windows fault handler. The module uses an alternative stack for signal
handlers, if the :c:func:`sigaltstack` function is available, to be able to
dump the traceback even on a stack overflow.

The fault handler is called on catastrophic cases and so can only use
signal-safe functions (e.g. it cannot allocate memory on the heap). That's why
the traceback is limited: only support ASCII encoding (use the
``backslashreplace`` error handler), limit each string to 100 characters, don't
print the source code (only the filename, the function name and the line
number), limit to 100 frames and 100 threads.

By default, the Python traceback is written to :data:`sys.stderr`. Start your
graphical applications in a terminal and run your server in foreground to see
the traceback, or specify a log file to :func:`faulthandler.enable()`.

The module is implemented in C to be able to dump a traceback on a crash or
when Python is blocked (e.g. deadlock).

.. versionadded:: 3.3


Dump the traceback
------------------

.. function:: dump_traceback(file=sys.stderr, all_threads=False)

   Dump the traceback of the current thread, or of all threads if *all_threads*
   is ``True``, into *file*.


Fault handler state
-------------------

.. function:: enable(file=sys.stderr, all_threads=False)

   Enable the fault handler: install handlers for :const:`SIGSEGV`,
   :const:`SIGFPE`, :const:`SIGBUS` and :const:`SIGILL` signals to dump the
   Python traceback. It dumps the traceback of the current thread, or all
   threads if *all_threads* is ``True``, into *file*.

.. function:: disable()

   Disable the fault handler: uninstall the signal handlers installed by
   :func:`enable`.

.. function:: is_enabled()

   Check if the fault handler is enabled.


Dump the tracebacks after a timeout
-----------------------------------

.. function:: dump_tracebacks_later(timeout, repeat=False, file=sys.stderr, exit=False)

   Dump the tracebacks of all threads, after a timeout of *timeout* seconds, or
   each *timeout* seconds if *repeat* is ``True``.  If *exit* is True, call
   :cfunc:`_exit` with status=1 after dumping the tracebacks to terminate
   immediatly the process, which is not safe.  For example, :cfunc:`_exit`
   doesn't flush file buffers.  If the function is called twice, the new call
   replaces previous parameters (resets the timeout). The timer has a
   sub-second resolution.

   This function is implemented using a watchdog thread, and therefore is
   not available if Python is compiled with threads disabled.

.. function:: cancel_dump_traceback_later()

   Cancel the last call to :func:`dump_traceback_later`.


Dump the traceback on a user signal
-----------------------------------

.. function:: register(signum, file=sys.stderr, all_threads=False)

   Register a user signal: install a handler for the *signum* signal to dump
   the traceback of the current thread, or of all threads if *all_threads* is
   ``True``, into *file*.

   Not available on Windows.

.. function:: unregister(signum)

   Unregister a user signal: uninstall the handler of the *signum* signal
   installed by :func:`register`.

   Not available on Windows.


File descriptor issue
---------------------

:func:`enable`, :func:`dump_traceback_later` and :func:`register` keep the
file descriptor of their *file* argument. If the file is closed and its file
descriptor is reused by a new file, or if :func:`os.dup2` is used to replace
the file descriptor, the traceback will be written into a different file. Call
these functions again each time that the file is replaced.


Example
-------

Example of a segmentation fault on Linux: ::

    $ python -q -X faulthandler
    >>> import ctypes
    >>> ctypes.string_at(0)
    Fatal Python error: Segmentation fault

    Traceback (most recent call first):
      File "/home/python/cpython/Lib/ctypes/__init__.py", line 486 in string_at
      File "<stdin>", line 1 in <module>
    Segmentation fault

