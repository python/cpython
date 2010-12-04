:mod:`subprocess` --- Subprocess management
===========================================

.. module:: subprocess
   :synopsis: Subprocess management.
.. moduleauthor:: Peter Åstrand <astrand@lysator.liu.se>
.. sectionauthor:: Peter Åstrand <astrand@lysator.liu.se>


The :mod:`subprocess` module allows you to spawn new processes, connect to their
input/output/error pipes, and obtain their return codes.  This module intends to
replace several other, older modules and functions, such as::

   os.system
   os.spawn*

Information about how the :mod:`subprocess` module can be used to replace these
modules and functions can be found in the following sections.

.. seealso::

   :pep:`324` -- PEP proposing the subprocess module


Using the subprocess Module
---------------------------

This module defines one class called :class:`Popen`:


.. class:: Popen(args, bufsize=0, executable=None, stdin=None, stdout=None, stderr=None, preexec_fn=None, close_fds=False, shell=False, cwd=None, env=None, universal_newlines=False, startupinfo=None, creationflags=0, restore_signals=True, start_new_session=False)

   Arguments are:

   *args* should be a string, or a sequence of program arguments.  The program
   to execute is normally the first item in the args sequence or the string if
   a string is given, but can be explicitly set by using the *executable*
   argument.  When *executable* is given, the first item in the args sequence
   is still treated by most programs as the command name, which can then be
   different from the actual executable name.  On Unix, it becomes the display
   name for the executing program in utilities such as :program:`ps`.

   On Unix, with *shell=False* (default): In this case, the Popen class uses
   :meth:`os.execvp` like behavior to execute the child program.
   *args* should normally be a
   sequence.  If a string is specified for *args*, it will be used as the name
   or path of the program to execute; this will only work if the program is
   being given no arguments.

   .. note::

      :meth:`shlex.split` can be useful when determining the correct
      tokenization for *args*, especially in complex cases::

         >>> import shlex, subprocess
         >>> command_line = input()
         /bin/vikings -input eggs.txt -output "spam spam.txt" -cmd "echo '$MONEY'"
         >>> args = shlex.split(command_line)
         >>> print(args)
         ['/bin/vikings', '-input', 'eggs.txt', '-output', 'spam spam.txt', '-cmd', "echo '$MONEY'"]
         >>> p = subprocess.Popen(args) # Success!

      Note in particular that options (such as *-input*) and arguments (such
      as *eggs.txt*) that are separated by whitespace in the shell go in separate
      list elements, while arguments that need quoting or backslash escaping when
      used in the shell (such as filenames containing spaces or the *echo* command
      shown above) are single list elements.

   On Unix, with *shell=True*: If args is a string, it specifies the command
   string to execute through the shell.  This means that the string must be
   formatted exactly as it would be when typed at the shell prompt.  This
   includes, for example, quoting or backslash escaping filenames with spaces in
   them.  If *args* is a sequence, the first item specifies the command string, and
   any additional items will be treated as additional arguments to the shell
   itself.  That is to say, *Popen* does the equivalent of::

      Popen(['/bin/sh', '-c', args[0], args[1], ...])

   .. warning::

      Executing shell commands that incorporate unsanitized input from an
      untrusted source makes a program vulnerable to `shell injection
      <http://en.wikipedia.org/wiki/Shell_injection#Shell_injection>`_,
      a serious security flaw which can result in arbitrary command execution.
      For this reason, the use of *shell=True* is **strongly discouraged** in cases
      where the command string is constructed from external input::

         >>> from subprocess import call
         >>> filename = input("What file would you like to display?\n")
         What file would you like to display?
         non_existent; rm -rf / #
         >>> call("cat " + filename, shell=True) # Uh-oh. This will end badly...

      *shell=False* does not suffer from this vulnerability; the above Note may be
      helpful in getting code using *shell=False* to work.

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

   .. note::

      If you experience performance issues, it is recommended that you try to
      enable buffering by setting *bufsize* to either -1 or a large enough
      positive value (such as 4096).

   The *executable* argument specifies the program to execute. It is very seldom
   needed: Usually, the program to execute is defined by the *args* argument. If
   ``shell=True``, the *executable* argument specifies which shell to use. On Unix,
   the default shell is :file:`/bin/sh`.  On Windows, the default shell is
   specified by the :envvar:`COMSPEC` environment variable. The only reason you
   would need to specify ``shell=True`` on Windows is where the command you
   wish to execute is actually built in to the shell, eg ``dir``, ``copy``.
   You don't need ``shell=True`` to run a batch file, nor to run a console-based
   executable.

   *stdin*, *stdout* and *stderr* specify the executed programs' standard input,
   standard output and standard error file handles, respectively.  Valid values
   are :data:`PIPE`, an existing file descriptor (a positive integer), an
   existing :term:`file object`, and ``None``.  :data:`PIPE` indicates that a
   new pipe to the child should be created.  With ``None``, no redirection will
   occur; the child's file handles will be inherited from the parent.  Additionally,
   *stderr* can be :data:`STDOUT`, which indicates that the stderr data from the
   applications should be captured into the same file handle as for stdout.

   If *preexec_fn* is set to a callable object, this object will be called in the
   child process just before the child is executed.
   (Unix only)

   .. warning::

      The *preexec_fn* parameter is not safe to use in the presence of threads
      in your application.  The child process could deadlock before exec is
      called.
      If you must use it, keep it trivial!  Minimize the number of libraries
      you call into.

   .. note::

      If you need to modify the environment for the child use the *env*
      parameter rather than doing it in a *preexec_fn*.
      The *start_new_session* parameter can take the place of a previously
      common use of *preexec_fn* to call os.setsid() in the child.

   If *close_fds* is true, all file descriptors except :const:`0`, :const:`1` and
   :const:`2` will be closed before the child process is executed. (Unix only).
   The recommended value for this argument is True.
   On Windows, if *close_fds* is true then no handles will be inherited by the
   child process.  Note that on Windows, you cannot set *close_fds* to true and
   also redirect the standard handles by setting *stdin*, *stdout* or *stderr*.

.. versionchanged:: 3.2
   Callers should always specify a *close_fds* to avoid a DeprecationWarning.
   The default behavior of this argument will be changing in Python 3.3.

   If *shell* is :const:`True`, the specified command will be executed through the
   shell.

   If *cwd* is not ``None``, the child's current directory will be changed to *cwd*
   before it is executed.  Note that this directory is not considered when
   searching the executable, so you can't specify the program's path relative to
   *cwd*.

   If *restore_signals* is True (the default) all signals that Python has set to
   SIG_IGN are restored to SIG_DFL in the child process before the exec.
   Currently this includes the SIGPIPE, SIGXFZ and SIGXFSZ signals.
   (Unix only)

   .. versionchanged:: 3.2
      *restore_signals* was added.

   If *start_new_session* is True the setsid() system call will be made in the
   child process prior to the execution of the subprocess.  (Unix only)

   .. versionchanged:: 3.2
      *start_new_session* was added.

   If *env* is not ``None``, it must be a mapping that defines the environment
   variables for the new process; these are used instead of the default
   behavior of inheriting the current process' environment.

   .. note::

      If specified, *env* must provide any variables required for the program to
      execute.  On Windows, in order to run a `side-by-side assembly`_ the
      specified *env* **must** include a valid :envvar:`SystemRoot`.

   .. _side-by-side assembly: http://en.wikipedia.org/wiki/Side-by-Side_Assembly

   If *universal_newlines* is :const:`True`, the file objects stdout and stderr are
   opened as text files, but lines may be terminated by any of ``'\n'``, the Unix
   end-of-line convention, ``'\r'``, the old Macintosh convention or ``'\r\n'``, the
   Windows convention. All of these external representations are seen as ``'\n'``
   by the Python program.

   .. note::

      This feature is only available if Python is built with universal newline
      support (the default).  Also, the newlines attribute of the file objects
      :attr:`stdout`, :attr:`stdin` and :attr:`stderr` are not updated by the
      :meth:`communicate` method.

   The *startupinfo* and *creationflags*, if given, will be passed to the
   underlying CreateProcess() function.  They can specify things such as appearance
   of the main window and priority for the new process.  (Windows only)

   Popen objects are supported as context managers via the :keyword:`with` statement,
   closing any open file descriptors on exit.
   ::

      with Popen(["ifconfig"], stdout=PIPE) as proc:
          log.write(proc.stdout.read())

   .. versionchanged:: 3.2
      Added context manager support.


.. data:: PIPE

   Special value that can be used as the *stdin*, *stdout* or *stderr* argument
   to :class:`Popen` and indicates that a pipe to the standard stream should be
   opened.


.. data:: STDOUT

   Special value that can be used as the *stderr* argument to :class:`Popen` and
   indicates that standard error should go into the same handle as standard
   output.


Convenience Functions
^^^^^^^^^^^^^^^^^^^^^

This module also defines four shortcut functions:


.. function:: call(*popenargs, **kwargs)

   Run command with arguments.  Wait for command to complete, then return the
   :attr:`returncode` attribute.

   The arguments are the same as for the :class:`Popen` constructor.  Example::

      >>> retcode = subprocess.call(["ls", "-l"])

   .. warning::

      Like :meth:`Popen.wait`, this will deadlock when using
      ``stdout=PIPE`` and/or ``stderr=PIPE`` and the child process
      generates enough output to a pipe such that it blocks waiting
      for the OS pipe buffer to accept more data.


.. function:: check_call(*popenargs, **kwargs)

   Run command with arguments.  Wait for command to complete. If the exit code was
   zero then return, otherwise raise :exc:`CalledProcessError`. The
   :exc:`CalledProcessError` object will have the return code in the
   :attr:`returncode` attribute.

   The arguments are the same as for the :class:`Popen` constructor.  Example::

      >>> subprocess.check_call(["ls", "-l"])
      0

   .. warning::

      See the warning for :func:`call`.


.. function:: check_output(*popenargs, **kwargs)

   Run command with arguments and return its output as a byte string.

   If the exit code was non-zero it raises a :exc:`CalledProcessError`.  The
   :exc:`CalledProcessError` object will have the return code in the
   :attr:`returncode`
   attribute and output in the :attr:`output` attribute.

   The arguments are the same as for the :class:`Popen` constructor.  Example::

      >>> subprocess.check_output(["ls", "-l", "/dev/null"])
      b'crw-rw-rw- 1 root root 1, 3 Oct 18  2007 /dev/null\n'

   The stdout argument is not allowed as it is used internally.
   To capture standard error in the result, use ``stderr=subprocess.STDOUT``::

      >>> subprocess.check_output(
      ...     ["/bin/sh", "-c", "ls non_existent_file; exit 0"],
      ...     stderr=subprocess.STDOUT)
      b'ls: non_existent_file: No such file or directory\n'

   .. versionadded:: 3.1


.. function:: getstatusoutput(cmd)

   Return ``(status, output)`` of executing *cmd* in a shell.

   Execute the string *cmd* in a shell with :func:`os.popen` and return a 2-tuple
   ``(status, output)``.  *cmd* is actually run as ``{ cmd ; } 2>&1``, so that the
   returned output will contain output or error messages.  A trailing newline is
   stripped from the output.  The exit status for the command can be interpreted
   according to the rules for the C function :c:func:`wait`.  Example::

      >>> subprocess.getstatusoutput('ls /bin/ls')
      (0, '/bin/ls')
      >>> subprocess.getstatusoutput('cat /bin/junk')
      (256, 'cat: /bin/junk: No such file or directory')
      >>> subprocess.getstatusoutput('/bin/junk')
      (256, 'sh: /bin/junk: not found')

   Availability: UNIX.


.. function:: getoutput(cmd)

   Return output (stdout and stderr) of executing *cmd* in a shell.

   Like :func:`getstatusoutput`, except the exit status is ignored and the return
   value is a string containing the command's output.  Example::

      >>> subprocess.getoutput('ls /bin/ls')
      '/bin/ls'

   Availability: UNIX.


Exceptions
^^^^^^^^^^

Exceptions raised in the child process, before the new program has started to
execute, will be re-raised in the parent.  Additionally, the exception object
will have one extra attribute called :attr:`child_traceback`, which is a string
containing traceback information from the child's point of view.

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

   Check if child process has terminated.  Set and return :attr:`returncode`
   attribute.


.. method:: Popen.wait()

   Wait for child process to terminate.  Set and return :attr:`returncode`
   attribute.

   .. warning::

      This will deadlock when using ``stdout=PIPE`` and/or
      ``stderr=PIPE`` and the child process generates enough output to
      a pipe such that it blocks waiting for the OS pipe buffer to
      accept more data.  Use :meth:`communicate` to avoid that.


.. method:: Popen.communicate(input=None)

   Interact with process: Send data to stdin.  Read data from stdout and stderr,
   until end-of-file is reached.  Wait for process to terminate. The optional
   *input* argument should be a byte string to be sent to the child process, or
   ``None``, if no data should be sent to the child.

   :meth:`communicate` returns a tuple ``(stdoutdata, stderrdata)``.

   Note that if you want to send data to the process's stdin, you need to create
   the Popen object with ``stdin=PIPE``.  Similarly, to get anything other than
   ``None`` in the result tuple, you need to give ``stdout=PIPE`` and/or
   ``stderr=PIPE`` too.

   .. note::

      The data read is buffered in memory, so do not use this method if the data
      size is large or unlimited.


.. method:: Popen.send_signal(signal)

   Sends the signal *signal* to the child.

   .. note::

      On Windows, SIGTERM is an alias for :meth:`terminate`. CTRL_C_EVENT and
      CTRL_BREAK_EVENT can be sent to processes started with a *creationflags*
      parameter which includes `CREATE_NEW_PROCESS_GROUP`.


.. method:: Popen.terminate()

   Stop the child. On Posix OSs the method sends SIGTERM to the
   child. On Windows the Win32 API function :c:func:`TerminateProcess` is called
   to stop the child.


.. method:: Popen.kill()

   Kills the child. On Posix OSs the function sends SIGKILL to the child.
   On Windows :meth:`kill` is an alias for :meth:`terminate`.


The following attributes are also available:

.. warning::

   Use :meth:`communicate` rather than :attr:`.stdin.write <stdin>`,
   :attr:`.stdout.read <stdout>` or :attr:`.stderr.read <stderr>` to avoid
   deadlocks due to any of the other OS pipe buffers filling up and blocking the
   child process.


.. attribute:: Popen.stdin

   If the *stdin* argument was :data:`PIPE`, this attribute is a :term:`file
   object` that provides input to the child process.  Otherwise, it is ``None``.


.. attribute:: Popen.stdout

   If the *stdout* argument was :data:`PIPE`, this attribute is a :term:`file
   object` that provides output from the child process.  Otherwise, it is ``None``.


.. attribute:: Popen.stderr

   If the *stderr* argument was :data:`PIPE`, this attribute is a :term:`file
   object` that provides error output from the child process.  Otherwise, it is
   ``None``.


.. attribute:: Popen.pid

   The process ID of the child process.

   Note that if you set the *shell* argument to ``True``, this is the process ID
   of the spawned shell.


.. attribute:: Popen.returncode

   The child return code, set by :meth:`poll` and :meth:`wait` (and indirectly
   by :meth:`communicate`).  A ``None`` value indicates that the process
   hasn't terminated yet.

   A negative value ``-N`` indicates that the child was terminated by signal
   ``N`` (Unix only).


.. _subprocess-replacements:

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


Replacing shell pipeline
^^^^^^^^^^^^^^^^^^^^^^^^

::

   output=`dmesg | grep hda`
   ==>
   p1 = Popen(["dmesg"], stdout=PIPE)
   p2 = Popen(["grep", "hda"], stdin=p1.stdout, stdout=PIPE)
   output = p2.communicate()[0]


Replacing :func:`os.system`
^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   sts = os.system("mycmd" + " myarg")
   ==>
   p = Popen("mycmd" + " myarg", shell=True)
   sts = os.waitpid(p.pid, 0)[1]

Notes:

* Calling the program through the shell is usually not required.

* It's easier to look at the :attr:`returncode` attribute than the exit status.

A more realistic example would look like this::

   try:
       retcode = call("mycmd" + " myarg", shell=True)
       if retcode < 0:
           print("Child was terminated by signal", -retcode, file=sys.stderr)
       else:
           print("Child returned", retcode, file=sys.stderr)
   except OSError as e:
       print("Execution failed:", e, file=sys.stderr)


Replacing the :func:`os.spawn <os.spawnl>` family
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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



Replacing :func:`os.popen`, :func:`os.popen2`, :func:`os.popen3`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

Return code handling translates as follows::

   pipe = os.popen(cmd, 'w')
   ...
   rc = pipe.close()
   if rc is not None and rc >> 8:
       print("There were some errors")
   ==>
   process = Popen(cmd, 'w', stdin=PIPE)
   ...
   process.stdin.close()
   if process.wait() != 0:
       print("There were some errors")


Replacing functions from the :mod:`popen2` module
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

:class:`popen2.Popen3` and :class:`popen2.Popen4` basically work as
:class:`subprocess.Popen`, except that:

* :class:`Popen` raises an exception if the execution fails.

* the *capturestderr* argument is replaced with the *stderr* argument.

* ``stdin=PIPE`` and ``stdout=PIPE`` must be specified.

* popen2 closes all file descriptors by default, but you have to specify
  ``close_fds=True`` with :class:`Popen`.
