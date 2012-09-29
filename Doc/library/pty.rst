:mod:`pty` --- Pseudo-terminal utilities
========================================

.. module:: pty
   :platform: Linux
   :synopsis: Pseudo-Terminal Handling for Linux.
.. moduleauthor:: Steen Lumholt
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


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
   reading from the controlling terminal.

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

   import sys, os, time, getopt
   import pty

   mode = 'wb'
   shell = 'sh'
   filename = 'typescript'
   if 'SHELL' in os.environ:
       shell = os.environ['SHELL']

   try:
       opts, args = getopt.getopt(sys.argv[1:], 'ap')
   except getopt.error as msg:
       print('%s: %s' % (sys.argv[0], msg))
       sys.exit(2)

   for opt, arg in opts:
       # option -a: append to typescript file
       if opt == '-a':
           mode = 'ab'
       # option -p: use a Python shell as the terminal command
       elif opt == '-p':
           shell = sys.executable
   if args:
       filename = args[0]

   script = open(filename, mode)

   def read(fd):
       data = os.read(fd, 1024)
       script.write(data)
       return data

   sys.stdout.write('Script started, file is %s\n' % filename)
   script.write(('Script started on %s\n' % time.asctime()).encode())
   pty.spawn(shell, read)
   script.write(('Script done on %s\n' % time.asctime()).encode())
   sys.stdout.write('Script done, file is %s\n' % filename)
