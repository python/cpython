:mod:`pty` --- Pseudo-terminal utilities
========================================

.. module:: pty
   :platform: Unix
   :synopsis: Pseudo-Terminal Handling for Unix.

.. moduleauthor:: Steen Lumholt
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>

**Source code:** :source:`Lib/pty.py`

--------------

The :mod:`pty` module defines operations for handling the pseudo-terminal
concept: starting another process and being able to write to and read from its
controlling terminal programmatically.

Pseudo-terminal handling is highly platform dependent. This code is mainly
tested on Linux, FreeBSD, and OS X (it is supposed to work on other POSIX
platforms).

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

   Spawn a child process, and connect its controlling terminal with the
   current process's standard io. This is often used to baffle programs which
   insist on reading from the controlling terminal.

   A loop copies STDIN of the current process to the child and data received
   from the child to STDOUT of the current process. It is not signaled to the
   child if STDIN of the current process closes down.

   The functions *master_read* and *stdin_read* should be functions which read from
   a file descriptor. The defaults try to read 1024 bytes each time they are
   called.

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
