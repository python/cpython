
:mod:`subprocess` --- Subprocess management
===========================================

.. module:: subprocess
   :synopsis: Subprocess management.
.. moduleauthor:: Peter Åstrand <astrand@lysator.liu.se>
.. sectionauthor:: Peter Åstrand <astrand@lysator.liu.se>


.. versionadded:: 2.4

The :mod:`subprocess` module allows you to spawn new processes, connect to their
input/output/error pipes, and obtain their return codes.  This module intends to
replace several other, older modules and functions, such as::

   os.system
   os.spawn*
   os.popen*
   popen2.*
   commands.*

Information about how the :mod:`subprocess` module can be used to replace these
modules and functions can be found in the following sections.


Using the subprocess Module
---------------------------

This module defines one class called :class:`Popen`:


.. class:: Popen(args, bufsize=0, executable=None, stdin=None, stdout=None, stderr=None, preexec_fn=None, close_fds=False, shell=False, cwd=None, env=None, universal_newlines=False, startupinfo=None, creationflags=0)

   Arguments are:

   *args* should be a string, or a sequence of program arguments.  The program to
   execute is normally the first item in the args sequence or string, but can be
   explicitly set by using the executable argument.

   On Unix, with *shell=False* (default): In this case, the Popen class uses
   :meth:`os.execvp` to execute the child program. *args* should normally be a
   sequence.  A string will be treated as a sequence with the string as the only
   item (the program to execute).

   On Unix, with *shell=True*: If args is a string, it specifies the command string
   to execute through the shell.  If *args* is a sequence, the first item specifies
   the command string, and any additional items will be treated as additional shell
   arguments.

   On Windows: the :class:`Popen` class uses CreateProcess() to execute the child
   program, which operates on strings.  If *args* is a sequence, it will be
   converted to a string using the :meth:`list2cmdline` method.  Please note that
   not all MS Windows applications interpret the command line the same way:
   :meth:`list2cmdline` is designed for applications using the same rules as the MS
   C runtime.

   *bufsize*, if given, has the same meaning as the corresponding argument to the
   built-in open() function: :const:`0` means unbuffered, :const:`1` means line
   buffered, any other positive value means use a buffer of (approximately) that
   size.  A negative *bufsize* means to use the system default, which usually means
   fully buffered.  The default value for *bufsize* is :const:`0` (unbuffered).

   The *executable* argument specifies the program to execute. It is very seldom
   needed: Usually, the program to execute is defined by the *args* argument. If
   ``shell=True``, the *executable* argument specifies which shell to use. On Unix,
   the default shell is :file:`/bin/sh`.  On Windows, the default shell is
   specified by the :envvar:`COMSPEC` environment variable.

   *stdin*, *stdout* and *stderr* specify the executed programs' standard input,
   standard output and standard error file handles, respectively.  Valid values are
   ``PIPE``, an existing file descriptor (a positive integer), an existing file
   object, and ``None``.  ``PIPE`` indicates that a new pipe to the child should be
   created.  With ``None``, no redirection will occur; the child's file handles
   will be inherited from the parent.  Additionally, *stderr* can be ``STDOUT``,
   which indicates that the stderr data from the applications should be captured
   into the same file handle as for stdout.

   If *preexec_fn* is set to a callable object, this object will be called in the
   child process just before the child is executed. (Unix only)

   If *close_fds* is true, all file descriptors except :const:`0`, :const:`1` and
   :const:`2` will be closed before the child process is executed. (Unix only).
   Or, on Windows, if *close_fds* is true then no handles will be inherited by the
   child process.  Note that on Windows, you cannot set *close_fds* to true and
   also redirect the standard handles by setting *stdin*, *stdout* or *stderr*.

   If *shell* is :const:`True`, the specified command will be executed through the
   shell.

   If *cwd* is not ``None``, the child's current directory will be changed to *cwd*
   before it is executed.  Note that this directory is not considered when
   searching the executable, so you can't specify the program's path relative to
   *cwd*.

   If *env* is not ``None``, it defines the environment variables for the new
   process.

   If *universal_newlines* is :const:`True`, the file objects stdout and stderr are
   opened as text files, but lines may be terminated by any of ``'\n'``, the Unix
   end-of-line convention, ``'\r'``, the Macintosh convention or ``'\r\n'``, the
   Windows convention. All of these external representations are seen as ``'\n'``
   by the Python program.

   .. note::

      This feature is only available if Python is built with universal newline support
      (the default).  Also, the newlines attribute of the file objects :attr:`stdout`,
      :attr:`stdin` and :attr:`stderr` are not updated by the communicate() method.

   The *startupinfo* and *creationflags*, if given, will be passed to the
   underlying CreateProcess() function.  They can specify things such as appearance
   of the main window and priority for the new process.  (Windows only)


Convenience Functions
^^^^^^^^^^^^^^^^^^^^^

This module also defines two shortcut functions:


.. function:: call(*popenargs, **kwargs)

   Run command with arguments.  Wait for command to complete, then return the
   :attr:`returncode` attribute.

   The arguments are the same as for the Popen constructor.  Example::

      retcode = call(["ls", "-l"])


.. function:: check_call(*popenargs, **kwargs)

   Run command with arguments.  Wait for command to complete. If the exit code was
   zero then return, otherwise raise :exc:`CalledProcessError.` The
   :exc:`CalledProcessError` object will have the return code in the
   :attr:`returncode` attribute.

   The arguments are the same as for the Popen constructor.  Example::

      check_call(["ls", "-l"])

   .. versionadded:: 2.5


Exceptions
^^^^^^^^^^

Exceptions raised in the child process, before the new program has started to
execute, will be re-raised in the parent.  Additionally, the exception object
will have one extra attribute called :attr:`child_traceback`, which is a string
containing traceback information from the childs point of view.

The most common exception raised is :exc:`OSError`.  This occurs, for example,
when trying to execute a non-existent file.  Applications should prepare for
:exc:`OSError` exceptions.

A :exc:`ValueError` will be raised if :class:`Popen` is called with invalid
arguments.

check_call() will raise :exc:`CalledProcessError`, if the called process returns
a non-zero return code.


Security
^^^^^^^^

Unlike some other popen functions, this implementation will never call /bin/sh
implicitly.  This means that all characters, including shell metacharacters, can
safely be passed to child processes.


Popen Objects
-------------

Instances of the :class:`Popen` class have the following methods:


.. method:: Popen.poll()

   Check if child process has terminated.  Returns returncode attribute.


.. method:: Popen.wait()

   Wait for child process to terminate.  Returns returncode attribute.


.. method:: Popen.communicate(input=None)

   Interact with process: Send data to stdin.  Read data from stdout and stderr,
   until end-of-file is reached.  Wait for process to terminate. The optional
   *input* argument should be a string to be sent to the child process, or
   ``None``, if no data should be sent to the child.

   communicate() returns a tuple (stdout, stderr).

   .. note::

      The data read is buffered in memory, so do not use this method if the data size
      is large or unlimited.

The following attributes are also available:


.. attribute:: Popen.stdin

   If the *stdin* argument is ``PIPE``, this attribute is a file object that
   provides input to the child process.  Otherwise, it is ``None``.


.. attribute:: Popen.stdout

   If the *stdout* argument is ``PIPE``, this attribute is a file object that
   provides output from the child process.  Otherwise, it is ``None``.


.. attribute:: Popen.stderr

   If the *stderr* argument is ``PIPE``, this attribute is file object that
   provides error output from the child process.  Otherwise, it is ``None``.


.. attribute:: Popen.pid

   The process ID of the child process.


.. attribute:: Popen.returncode

   The child return code.  A ``None`` value indicates that the process hasn't
   terminated yet.  A negative value -N indicates that the child was terminated by
   signal N (Unix only).


Replacing Older Functions with the subprocess Module
----------------------------------------------------

In this section, "a ==> b" means that b can be used as a replacement for a.

.. note::

   All functions in this section fail (more or less) silently if the executed
   program cannot be found; this module raises an :exc:`OSError` exception.

In the following examples, we assume that the subprocess module is imported with
"from subprocess import \*".


Replacing /bin/sh shell backquote
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   output=`mycmd myarg`
   ==>
   output = Popen(["mycmd", "myarg"], stdout=PIPE).communicate()[0]


Replacing shell pipe line
^^^^^^^^^^^^^^^^^^^^^^^^^

::

   output=`dmesg | grep hda`
   ==>
   p1 = Popen(["dmesg"], stdout=PIPE)
   p2 = Popen(["grep", "hda"], stdin=p1.stdout, stdout=PIPE)
   output = p2.communicate()[0]


Replacing os.system()
^^^^^^^^^^^^^^^^^^^^^

::

   sts = os.system("mycmd" + " myarg")
   ==>
   p = Popen("mycmd" + " myarg", shell=True)
   sts = os.waitpid(p.pid, 0)

Notes:

* Calling the program through the shell is usually not required.

* It's easier to look at the :attr:`returncode` attribute than the exit status.

A more realistic example would look like this::

   try:
       retcode = call("mycmd" + " myarg", shell=True)
       if retcode < 0:
           print >>sys.stderr, "Child was terminated by signal", -retcode
       else:
           print >>sys.stderr, "Child returned", retcode
   except OSError, e:
       print >>sys.stderr, "Execution failed:", e


Replacing os.spawn\*
^^^^^^^^^^^^^^^^^^^^

P_NOWAIT example::

   pid = os.spawnlp(os.P_NOWAIT, "/bin/mycmd", "mycmd", "myarg")
   ==>
   pid = Popen(["/bin/mycmd", "myarg"]).pid

P_WAIT example::

   retcode = os.spawnlp(os.P_WAIT, "/bin/mycmd", "mycmd", "myarg")
   ==>
   retcode = call(["/bin/mycmd", "myarg"])

Vector example::

   os.spawnvp(os.P_NOWAIT, path, args)
   ==>
   Popen([path] + args[1:])

Environment example::

   os.spawnlpe(os.P_NOWAIT, "/bin/mycmd", "mycmd", "myarg", env)
   ==>
   Popen(["/bin/mycmd", "myarg"], env={"PATH": "/usr/bin"})


Replacing os.popen\*
^^^^^^^^^^^^^^^^^^^^

::

   pipe = os.popen(cmd, mode='r', bufsize)
   ==>
   pipe = Popen(cmd, shell=True, bufsize=bufsize, stdout=PIPE).stdout

::

   pipe = os.popen(cmd, mode='w', bufsize)
   ==>
   pipe = Popen(cmd, shell=True, bufsize=bufsize, stdin=PIPE).stdin

::

   (child_stdin, child_stdout) = os.popen2(cmd, mode, bufsize)
   ==>
   p = Popen(cmd, shell=True, bufsize=bufsize,
             stdin=PIPE, stdout=PIPE, close_fds=True)
   (child_stdin, child_stdout) = (p.stdin, p.stdout)

::

   (child_stdin,
    child_stdout,
    child_stderr) = os.popen3(cmd, mode, bufsize)
   ==>
   p = Popen(cmd, shell=True, bufsize=bufsize,
             stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
   (child_stdin,
    child_stdout,
    child_stderr) = (p.stdin, p.stdout, p.stderr)

::

   (child_stdin, child_stdout_and_stderr) = os.popen4(cmd, mode, bufsize)
   ==>
   p = Popen(cmd, shell=True, bufsize=bufsize,
             stdin=PIPE, stdout=PIPE, stderr=STDOUT, close_fds=True)
   (child_stdin, child_stdout_and_stderr) = (p.stdin, p.stdout)


Replacing popen2.\*
^^^^^^^^^^^^^^^^^^^

.. note::

   If the cmd argument to popen2 functions is a string, the command is executed
   through /bin/sh.  If it is a list, the command is directly executed.

::

   (child_stdout, child_stdin) = popen2.popen2("somestring", bufsize, mode)
   ==>
   p = Popen(["somestring"], shell=True, bufsize=bufsize,
             stdin=PIPE, stdout=PIPE, close_fds=True)
   (child_stdout, child_stdin) = (p.stdout, p.stdin)

::

   (child_stdout, child_stdin) = popen2.popen2(["mycmd", "myarg"], bufsize, mode)
   ==>
   p = Popen(["mycmd", "myarg"], bufsize=bufsize,
             stdin=PIPE, stdout=PIPE, close_fds=True)
   (child_stdout, child_stdin) = (p.stdout, p.stdin)

The popen2.Popen3 and popen2.Popen4 basically works as subprocess.Popen, except
that:

* subprocess.Popen raises an exception if the execution fails

* the *capturestderr* argument is replaced with the *stderr* argument.

* stdin=PIPE and stdout=PIPE must be specified.

* popen2 closes all file descriptors by default, but you have to specify
  close_fds=True with subprocess.Popen.

