:mod:`pty` --- Pseudo-terminal utilities
========================================

.. module:: pty
   :platform: Linux
   :synopsis: Pseudo-Terminal Handling for Linux.

.. moduleauthor:: Steen Lumholt
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>

**Source code:** :source:`Lib/pty.py`

--------------

The :mod:`pty` module defines operations for handling the pseudo-terminal
concept: starting another process and being able to write to and read from its
controlling terminal programmatically.

Because pseudo-terminal handling is highly platform dependent, there is code to
do it only for Linux. (The Linux code is supposed to work on other platforms,
but hasn't been tested yet.)

The :mod:`pty` module defines the following functions:


.. function:: fork()

   Fork. Connect the child's controlling terminal to a pseudo-terminal. Return
   value is ``(pid, fd)``. Note that the child  gets *pid* 0, and the *fd* is
   *invalid*. The parent's return value is the *pid* of the child, and *fd* is a
   file descriptor connected to the child's controlling terminal (and also to the
   child's standard input and output).


.. function:: openpty()

   Open a new pseudo-terminal pair, using :func:`os.openpty` if possible, or
   emulation code for generic Unix systems. Return a pair of file descriptors
   ``(master, slave)``, for the master and the slave end, respectively.


.. function:: spawn(argv[, master_read[, stdin_read]])

   Spawn a process, and connect its controlling terminal with the current
   process's standard io. This is often used to baffle programs which insist on
   reading from the controlling terminal. It is expected that the process
   spawned behind the pty will eventually terminate, and when it does *spawn*
   will return.

   The functions *master_read* and *stdin_read* are passed a file descriptor
   which they should read from, and they should always return a byte string. In
   order to force spawn to return before the child process exits an
   :exc:`OSError` should be thrown.

   The default implementation for both functions will read and return up to 1024
   bytes each time the function is called. The *master_read* callback is passed
   the pseudoterminal’s master file descriptor to read output from the child
   process, and *stdin_read* is passed file descriptor 0, to read from the
   parent process's standard input.

   Returning an empty byte string from either callback is interpreted as an
   end-of-file (EOF) condition, and that callback will not be called after
   that. If *stdin_read* signals EOF the controlling terminal can no longer
   communicate with the parent process OR the child process. Unless the child
   process will quit without any input, *spawn* will then loop forever. If
   *master_read* signals EOF the same behavior results (on linux at least).

   If both callbacks signal EOF then *spawn* will probably never return, unless
   *select* throws an error on your platform when passed three empty lists. This
   is a bug, documented in `issue 26228 <https://bugs.python.org/issue26228>`_.


   .. versionchanged:: 3.4
      :func:`spawn` now returns the status value from :func:`os.waitpid`
      on the child process.

Example
-------

.. sectionauthor:: Steen Lumholt

The following program acts like the Unix command :manpage:`script(1)`, using a
pseudo-terminal to record all input and output of a terminal session in a
"typescript". ::

    import argparse
    import os
    import pty
    import sys
    import time

    parser = argparse.ArgumentParser()
    parser.add_argument('-a', dest='append', action='store_true')
    parser.add_argument('-p', dest='use_python', action='store_true')
    parser.add_argument('filename', nargs='?', default='typescript')
    options = parser.parse_args()

    shell = sys.executable if options.use_python else os.environ.get('SHELL', 'sh')
    filename = options.filename
    mode = 'ab' if options.append else 'wb'

    with open(filename, mode) as script:
        def read(fd):
            data = os.read(fd, 1024)
            script.write(data)
            return data

        print('Script started, file is', filename)
        script.write(('Script started on %s\n' % time.asctime()).encode())

        pty.spawn(shell, read)

        script.write(('Script done on %s\n' % time.asctime()).encode())
        print('Script done, file is', filename)
