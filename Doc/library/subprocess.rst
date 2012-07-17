
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

.. seealso::

   :pep:`324` -- PEP proposing the subprocess module


Using the subprocess Module
---------------------------

The recommended approach to invoking subprocesses is to use the following
convenience functions for all use cases they can handle. For more advanced
use cases, the underlying :class:`Popen` interface can be used directly.


.. function:: call(args, *, stdin=None, stdout=None, stderr=None, shell=False)

   Run the command described by *args*.  Wait for command to complete, then
   return the :attr:`returncode` attribute.

   The arguments shown above are merely the most common ones, described below
   in :ref:`frequently-used-arguments` (hence the slightly odd notation in
   the abbreviated signature). The full function signature is the same as
   that of the :class:`Popen` constructor - this functions passes all
   supplied arguments directly through to that interface.

   Examples::

      >>> subprocess.call(["ls", "-l"])
      0

      >>> subprocess.call("exit 1", shell=True)
      1

   .. warning::

      Invoking the system shell with ``shell=True`` can be a security hazard
      if combined with untrusted input. See the warning under
      :ref:`frequently-used-arguments` for details.

   .. note::

      Do not use ``stdout=PIPE`` or ``stderr=PIPE`` with this function. As
      the pipes are not being read in the current process, the child
      process may block if it generates enough output to a pipe to fill up
      the OS pipe buffer.


.. function:: check_call(args, *, stdin=None, stdout=None, stderr=None, shell=False)

   Run command with arguments.  Wait for command to complete. If the return
   code was zero then return, otherwise raise :exc:`CalledProcessError`. The
   :exc:`CalledProcessError` object will have the return code in the
   :attr:`returncode` attribute.

   The arguments shown above are merely the most common ones, described below
   in :ref:`frequently-used-arguments` (hence the slightly odd notation in
   the abbreviated signature). The full function signature is the same as
   that of the :class:`Popen` constructor - this functions passes all
   supplied arguments directly through to that interface.

   Examples::

      >>> subprocess.check_call(["ls", "-l"])
      0

      >>> subprocess.check_call("exit 1", shell=True)
      Traceback (most recent call last):
         ...
      subprocess.CalledProcessError: Command 'exit 1' returned non-zero exit status 1

   .. versionadded:: 2.5

   .. warning::

      Invoking the system shell with ``shell=True`` can be a security hazard
      if combined with untrusted input. See the warning under
      :ref:`frequently-used-arguments` for details.

   .. note::

      Do not use ``stdout=PIPE`` or ``stderr=PIPE`` with this function. As
      the pipes are not being read in the current process, the child
      process may block if it generates enough output to a pipe to fill up
      the OS pipe buffer.


.. function:: check_output(args, *, stdin=None, stderr=None, shell=False, universal_newlines=False)

   Run command with arguments and return its output as a byte string.

   If the return code was non-zero it raises a :exc:`CalledProcessError`. The
   :exc:`CalledProcessError` object will have the return code in the
   :attr:`returncode` attribute and any output in the :attr:`output`
   attribute.

   The arguments shown above are merely the most common ones, described below
   in :ref:`frequently-used-arguments` (hence the slightly odd notation in
   the abbreviated signature). The full function signature is largely the
   same as that of the :class:`Popen` constructor, except that *stdout* is
   not permitted as it is used internally. All other supplied arguments are
   passed directly through to the :class:`Popen` constructor.

   Examples::

      >>> subprocess.check_output(["echo", "Hello World!"])
      'Hello World!\n'

      >>> subprocess.check_output("exit 1", shell=True)
      Traceback (most recent call last):
         ...
      subprocess.CalledProcessError: Command 'exit 1' returned non-zero exit status 1

   To also capture standard error in the result, use
   ``stderr=subprocess.STDOUT``::

      >>> subprocess.check_output(
      ...     "ls non_existent_file; exit 0",
      ...     stderr=subprocess.STDOUT,
      ...     shell=True)
      'ls: non_existent_file: No such file or directory\n'

   .. versionadded:: 2.7

   .. warning::

      Invoking the system shell with ``shell=True`` can be a security hazard
      if combined with untrusted input. See the warning under
      :ref:`frequently-used-arguments` for details.

   .. note::

      Do not use ``stderr=PIPE`` with this function. As the pipe is not being
      read in the current process, the child process may block if it
      generates enough output to the pipe to fill up the OS pipe buffer.


.. data:: PIPE

   Special value that can be used as the *stdin*, *stdout* or *stderr* argument
   to :class:`Popen` and indicates that a pipe to the standard stream should be
   opened.


.. data:: STDOUT

   Special value that can be used as the *stderr* argument to :class:`Popen` and
   indicates that standard error should go into the same handle as standard
   output.


.. _frequently-used-arguments:

Frequently Used Arguments
^^^^^^^^^^^^^^^^^^^^^^^^^

To support a wide variety of use cases, the :class:`Popen` constructor (and
the convenience functions) accept a large number of optional arguments. For
most typical use cases, many of these arguments can be safely left at their
default values. The arguments that are most commonly needed are:

   *args* is required for all calls and should be a string, or a sequence of
   program arguments. Providing a sequence of arguments is generally
   preferred, as it allows the module to take care of any required escaping
   and quoting of arguments (e.g. to permit spaces in file names). If passing
   a single string, either *shell* must be :const:`True` (see below) or else
   the string must simply name the program to be executed without specifying
   any arguments.

   *stdin*, *stdout* and *stderr* specify the executed program's standard input,
   standard output and standard error file handles, respectively.  Valid values
   are :data:`PIPE`, an existing file descriptor (a positive integer), an
   existing file object, and ``None``.  :data:`PIPE` indicates that a new pipe
   to the child should be created.  With the default settings of ``None``, no
   redirection will occur; the child's file handles will be inherited from the
   parent.  Additionally, *stderr* can be :data:`STDOUT`, which indicates that
   the stderr data from the child process should be captured into the same file
   handle as for stdout.

   When *stdout* or *stderr* are pipes and *universal_newlines* is
   :const:`True` then all line endings will be converted to ``'\n'`` as
   described for the universal newlines `'U'`` mode argument to :func:`open`.

   If *shell* is :const:`True`, the specified command will be executed through
   the shell. This can be useful if you are using Python primarily for the
   enhanced control flow it offers over most system shells and still want
   access to other shell features such as filename wildcards, shell pipes and
   environment variable expansion.

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

      ``shell=False`` disables all shell based features, but does not suffer
      from this vulnerability; see the Note in the :class:`Popen` constructor
      documentation for helpful hints in getting ``shell=False`` to work.

These options, along with all of the other options, are described in more
detail in the :class:`Popen` constructor documentation.


Popen Constructor
^^^^^^^^^^^^^^^^^

The underlying process creation and management in this module is handled by
the :class:`Popen` class. It offers a lot of flexibility so that developers
are able to handle the less common cases not covered by the convenience
functions.


.. class:: Popen(args, bufsize=0, executable=None, stdin=None, stdout=None, stderr=None, preexec_fn=None, close_fds=False, shell=False, cwd=None, env=None, universal_newlines=False, startupinfo=None, creationflags=0)

   Arguments are:

   *args* should be a string, or a sequence of program arguments.  The program
   to execute is normally the first item in the args sequence or the string if
   a string is given, but can be explicitly set by using the *executable*
   argument.  When *executable* is given, the first item in the args sequence
   is still treated by most programs as the command name, which can then be
   different from the actual executable name.  On Unix, it becomes the display
   name for the executing program in utilities such as :program:`ps`.

   On Unix, with *shell=False* (default): In this case, the Popen class uses
   :meth:`os.execvp` to execute the child program. *args* should normally be a
   sequence.  If a string is specified for *args*, it will be used as the name
   or path of the program to execute; this will only work if the program is
   being given no arguments.

   .. note::

      :meth:`shlex.split` can be useful when determining the correct
      tokenization for *args*, especially in complex cases::

         >>> import shlex, subprocess
         >>> command_line = raw_input()
         /bin/vikings -input eggs.txt -output "spam spam.txt" -cmd "echo '$MONEY'"
         >>> args = shlex.split(command_line)
         >>> print args
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

   On Windows: the :class:`Popen` class uses CreateProcess() to execute the
   child program, which operates on strings.  If *args* is a sequence, it will
   be converted to a string in a manner described in
   :ref:`converting-argument-sequence`.

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

   *stdin*, *stdout* and *stderr* specify the executed program's standard input,
   standard output and standard error file handles, respectively.  Valid values
   are :data:`PIPE`, an existing file descriptor (a positive integer), an
   existing file object, and ``None``.  :data:`PIPE` indicates that a new pipe
   to the child should be created.  With the default settings of ``None``, no
   redirection will occur; the child's file handles will be inherited from the
   parent.  Additionally, *stderr* can be :data:`STDOUT`, which indicates that
   the stderr data from the child process should be captured into the same file
   handle as for stdout.

   If *preexec_fn* is set to a callable object, this object will be called in the
   child process just before the child is executed. (Unix only)

   If *close_fds* is true, all file descriptors except :const:`0`, :const:`1` and
   :const:`2` will be closed before the child process is executed. (Unix only).
   Or, on Windows, if *close_fds* is true then no handles will be inherited by the
   child process.  Note that on Windows, you cannot set *close_fds* to true and
   also redirect the standard handles by setting *stdin*, *stdout* or *stderr*.

   If *shell* is :const:`True`, the specified command will be executed through the
   shell.

   .. warning::

      Enabling this option can be a security hazard if combined with untrusted
      input. See the warning under :ref:`frequently-used-arguments`
      for details.

   If *cwd* is not ``None``, the child's current directory will be changed to *cwd*
   before it is executed.  Note that this directory is not considered when
   searching the executable, so you can't specify the program's path relative to
   *cwd*.

   If *env* is not ``None``, it must be a mapping that defines the environment
   variables for the new process; these are used instead of inheriting the current
   process' environment, which is the default behavior.

   .. note::

      If specified, *env* must provide any variables required
      for the program to execute.  On Windows, in order to run a
      `side-by-side assembly`_ the specified *env* **must** include a valid
      :envvar:`SystemRoot`.

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
      communicate() method.

   If given, *startupinfo* will be a :class:`STARTUPINFO` object, which is
   passed to the underlying ``CreateProcess`` function.
   *creationflags*, if given, can be :data:`CREATE_NEW_CONSOLE` or
   :data:`CREATE_NEW_PROCESS_GROUP`. (Windows only)


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

:func:`check_call` and :func:`check_output` will raise
:exc:`CalledProcessError` if the called process returns a non-zero return
code.


Security
^^^^^^^^

Unlike some other popen functions, this implementation will never call a
system shell implicitly.  This means that all characters, including shell
metacharacters, can safely be passed to child processes. Obviously, if the
shell is invoked explicitly, then it is the application's responsibility to
ensure that all whitespace and metacharacters are quoted appropriately.


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
   *input* argument should be a string to be sent to the child process, or
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

   .. versionadded:: 2.6


.. method:: Popen.terminate()

   Stop the child. On Posix OSs the method sends SIGTERM to the
   child. On Windows the Win32 API function :c:func:`TerminateProcess` is called
   to stop the child.

   .. versionadded:: 2.6


.. method:: Popen.kill()

   Kills the child. On Posix OSs the function sends SIGKILL to the child.
   On Windows :meth:`kill` is an alias for :meth:`terminate`.

   .. versionadded:: 2.6


The following attributes are also available:

.. warning::

   Use :meth:`communicate` rather than :attr:`.stdin.write <stdin>`,
   :attr:`.stdout.read <stdout>` or :attr:`.stderr.read <stderr>` to avoid
   deadlocks due to any of the other OS pipe buffers filling up and blocking the
   child process.


.. attribute:: Popen.stdin

   If the *stdin* argument was :data:`PIPE`, this attribute is a file object
   that provides input to the child process.  Otherwise, it is ``None``.


.. attribute:: Popen.stdout

   If the *stdout* argument was :data:`PIPE`, this attribute is a file object
   that provides output from the child process.  Otherwise, it is ``None``.


.. attribute:: Popen.stderr

   If the *stderr* argument was :data:`PIPE`, this attribute is a file object
   that provides error output from the child process.  Otherwise, it is
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


Windows Popen Helpers
---------------------

The :class:`STARTUPINFO` class and following constants are only available
on Windows.

.. class:: STARTUPINFO()

   Partial support of the Windows
   `STARTUPINFO <http://msdn.microsoft.com/en-us/library/ms686331(v=vs.85).aspx>`__
   structure is used for :class:`Popen` creation.

   .. attribute:: dwFlags

      A bit field that determines whether certain :class:`STARTUPINFO`
      attributes are used when the process creates a window. ::

         si = subprocess.STARTUPINFO()
         si.dwFlags = subprocess.STARTF_USESTDHANDLES | subprocess.STARTF_USESHOWWINDOW

   .. attribute:: hStdInput

      If :attr:`dwFlags` specifies :data:`STARTF_USESTDHANDLES`, this attribute
      is the standard input handle for the process. If
      :data:`STARTF_USESTDHANDLES` is not specified, the default for standard
      input is the keyboard buffer.

   .. attribute:: hStdOutput

      If :attr:`dwFlags` specifies :data:`STARTF_USESTDHANDLES`, this attribute
      is the standard output handle for the process. Otherwise, this attribute
      is ignored and the default for standard output is the console window's
      buffer.

   .. attribute:: hStdError

      If :attr:`dwFlags` specifies :data:`STARTF_USESTDHANDLES`, this attribute
      is the standard error handle for the process. Otherwise, this attribute is
      ignored and the default for standard error is the console window's buffer.

   .. attribute:: wShowWindow

      If :attr:`dwFlags` specifies :data:`STARTF_USESHOWWINDOW`, this attribute
      can be any of the values that can be specified in the ``nCmdShow``
      parameter for the
      `ShowWindow <http://msdn.microsoft.com/en-us/library/ms633548(v=vs.85).aspx>`__
      function, except for ``SW_SHOWDEFAULT``. Otherwise, this attribute is
      ignored.

      :data:`SW_HIDE` is provided for this attribute. It is used when
      :class:`Popen` is called with ``shell=True``.


Constants
^^^^^^^^^

The :mod:`subprocess` module exposes the following constants.

.. data:: STD_INPUT_HANDLE

   The standard input device. Initially, this is the console input buffer,
   ``CONIN$``.

.. data:: STD_OUTPUT_HANDLE

   The standard output device. Initially, this is the active console screen
   buffer, ``CONOUT$``.

.. data:: STD_ERROR_HANDLE

   The standard error device. Initially, this is the active console screen
   buffer, ``CONOUT$``.

.. data:: SW_HIDE

   Hides the window. Another window will be activated.

.. data:: STARTF_USESTDHANDLES

   Specifies that the :attr:`STARTUPINFO.hStdInput`,
   :attr:`STARTUPINFO.hStdOutput`, and :attr:`STARTUPINFO.hStdError` attributes
   contain additional information.

.. data:: STARTF_USESHOWWINDOW

   Specifies that the :attr:`STARTUPINFO.wShowWindow` attribute contains
   additional information.

.. data:: CREATE_NEW_CONSOLE

   The new process has a new console, instead of inheriting its parent's
   console (the default).

   This flag is always set when :class:`Popen` is created with ``shell=True``.

.. data:: CREATE_NEW_PROCESS_GROUP

   A :class:`Popen` ``creationflags`` parameter to specify that a new process
   group will be created. This flag is necessary for using :func:`os.kill`
   on the subprocess.

   This flag is ignored if :data:`CREATE_NEW_CONSOLE` is specified.


.. _subprocess-replacements:

Replacing Older Functions with the subprocess Module
----------------------------------------------------

In this section, "a becomes b" means that b can be used as a replacement for a.

.. note::

   All "a" functions in this section fail (more or less) silently if the
   executed program cannot be found; the "b" replacements raise :exc:`OSError`
   instead.

   In addition, the replacements using :func:`check_output` will fail with a
   :exc:`CalledProcessError` if the requested operation produces a non-zero
   return code. The output is still available as the ``output`` attribute of
   the raised exception.

In the following examples, we assume that the relevant functions have already
been imported from the subprocess module.


Replacing /bin/sh shell backquote
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   output=`mycmd myarg`
   # becomes
   output = check_output(["mycmd", "myarg"])


Replacing shell pipeline
^^^^^^^^^^^^^^^^^^^^^^^^

::

   output=`dmesg | grep hda`
   # becomes
   p1 = Popen(["dmesg"], stdout=PIPE)
   p2 = Popen(["grep", "hda"], stdin=p1.stdout, stdout=PIPE)
   p1.stdout.close()  # Allow p1 to receive a SIGPIPE if p2 exits.
   output = p2.communicate()[0]

The p1.stdout.close() call after starting the p2 is important in order for p1
to receive a SIGPIPE if p2 exits before p1.

Alternatively, for trusted input, the shell's own pipeline support may still
be used directly::

   output=`dmesg | grep hda`
   # becomes
   output=check_output("dmesg | grep hda", shell=True)


Replacing :func:`os.system`
^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   sts = os.system("mycmd" + " myarg")
   # becomes
   sts = call("mycmd" + " myarg", shell=True)

Notes:

* Calling the program through the shell is usually not required.

A more realistic example would look like this::

   try:
       retcode = call("mycmd" + " myarg", shell=True)
       if retcode < 0:
           print >>sys.stderr, "Child was terminated by signal", -retcode
       else:
           print >>sys.stderr, "Child returned", retcode
   except OSError, e:
       print >>sys.stderr, "Execution failed:", e


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

   pipe = os.popen("cmd", 'r', bufsize)
   ==>
   pipe = Popen("cmd", shell=True, bufsize=bufsize, stdout=PIPE).stdout

::

   pipe = os.popen("cmd", 'w', bufsize)
   ==>
   pipe = Popen("cmd", shell=True, bufsize=bufsize, stdin=PIPE).stdin

::

   (child_stdin, child_stdout) = os.popen2("cmd", mode, bufsize)
   ==>
   p = Popen("cmd", shell=True, bufsize=bufsize,
             stdin=PIPE, stdout=PIPE, close_fds=True)
   (child_stdin, child_stdout) = (p.stdin, p.stdout)

::

   (child_stdin,
    child_stdout,
    child_stderr) = os.popen3("cmd", mode, bufsize)
   ==>
   p = Popen("cmd", shell=True, bufsize=bufsize,
             stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
   (child_stdin,
    child_stdout,
    child_stderr) = (p.stdin, p.stdout, p.stderr)

::

   (child_stdin, child_stdout_and_stderr) = os.popen4("cmd", mode,
                                                      bufsize)
   ==>
   p = Popen("cmd", shell=True, bufsize=bufsize,
             stdin=PIPE, stdout=PIPE, stderr=STDOUT, close_fds=True)
   (child_stdin, child_stdout_and_stderr) = (p.stdin, p.stdout)

On Unix, os.popen2, os.popen3 and os.popen4 also accept a sequence as
the command to execute, in which case arguments will be passed
directly to the program without shell intervention.  This usage can be
replaced as follows::

   (child_stdin, child_stdout) = os.popen2(["/bin/ls", "-l"], mode,
                                           bufsize)
   ==>
   p = Popen(["/bin/ls", "-l"], bufsize=bufsize, stdin=PIPE, stdout=PIPE)
   (child_stdin, child_stdout) = (p.stdin, p.stdout)

Return code handling translates as follows::

   pipe = os.popen("cmd", 'w')
   ...
   rc = pipe.close()
   if rc is not None and rc >> 8:
       print "There were some errors"
   ==>
   process = Popen("cmd", 'w', shell=True, stdin=PIPE)
   ...
   process.stdin.close()
   if process.wait() != 0:
       print "There were some errors"


Replacing functions from the :mod:`popen2` module
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   (child_stdout, child_stdin) = popen2.popen2("somestring", bufsize, mode)
   ==>
   p = Popen(["somestring"], shell=True, bufsize=bufsize,
             stdin=PIPE, stdout=PIPE, close_fds=True)
   (child_stdout, child_stdin) = (p.stdout, p.stdin)

On Unix, popen2 also accepts a sequence as the command to execute, in
which case arguments will be passed directly to the program without
shell intervention.  This usage can be replaced as follows::

   (child_stdout, child_stdin) = popen2.popen2(["mycmd", "myarg"], bufsize,
                                               mode)
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


Notes
-----

.. _converting-argument-sequence:

Converting an argument sequence to a string on Windows
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On Windows, an *args* sequence is converted to a string that can be parsed
using the following rules (which correspond to the rules used by the MS C
runtime):

1. Arguments are delimited by white space, which is either a
   space or a tab.

2. A string surrounded by double quotation marks is
   interpreted as a single argument, regardless of white space
   contained within.  A quoted string can be embedded in an
   argument.

3. A double quotation mark preceded by a backslash is
   interpreted as a literal double quotation mark.

4. Backslashes are interpreted literally, unless they
   immediately precede a double quotation mark.

5. If backslashes immediately precede a double quotation mark,
   every pair of backslashes is interpreted as a literal
   backslash.  If the number of backslashes is odd, the last
   backslash escapes the next double quotation mark as
   described in rule 3.

