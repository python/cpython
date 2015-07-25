:mod:`subprocess` --- Subprocess management
===========================================

.. module:: subprocess
   :synopsis: Subprocess management.
.. moduleauthor:: Peter Åstrand <astrand@lysator.liu.se>
.. sectionauthor:: Peter Åstrand <astrand@lysator.liu.se>


The :mod:`subprocess` module allows you to spawn new processes, connect to their
input/output/error pipes, and obtain their return codes.  This module intends to
replace several older modules and functions::

   os.system
   os.spawn*

Information about how the :mod:`subprocess` module can be used to replace these
modules and functions can be found in the following sections.

.. seealso::

   :pep:`324` -- PEP proposing the subprocess module


Using the :mod:`subprocess` Module
----------------------------------

The recommended approach to invoking subprocesses is to use the :func:`run`
function for all use cases it can handle. For more advanced use cases, the
underlying :class:`Popen` interface can be used directly.

The :func:`run` function was added in Python 3.5; if you need to retain
compatibility with older versions, see the :ref:`call-function-trio` section.


.. function:: run(args, *, stdin=None, input=None, stdout=None, stderr=None,\
                  shell=False, timeout=None, check=False)

   Run the command described by *args*.  Wait for command to complete, then
   return a :class:`CompletedProcess` instance.

   The arguments shown above are merely the most common ones, described below
   in :ref:`frequently-used-arguments` (hence the use of keyword-only notation
   in the abbreviated signature). The full function signature is largely the
   same as that of the :class:`Popen` constructor - apart from *timeout*,
   *input* and *check*, all the arguments to this function are passed through to
   that interface.

   This does not capture stdout or stderr by default. To do so, pass
   :data:`PIPE` for the *stdout* and/or *stderr* arguments.

   The *timeout* argument is passed to :meth:`Popen.communicate`. If the timeout
   expires, the child process will be killed and waited for.  The
   :exc:`TimeoutExpired` exception will be re-raised after the child process
   has terminated.

   The *input* argument is passed to :meth:`Popen.communicate` and thus to the
   subprocess's stdin.  If used it must be a byte sequence, or a string if
   ``universal_newlines=True``.  When used, the internal :class:`Popen` object
   is automatically created with ``stdin=PIPE``, and the *stdin* argument may
   not be used as well.

   If *check* is True, and the process exits with a non-zero exit code, a
   :exc:`CalledProcessError` exception will be raised. Attributes of that
   exception hold the arguments, the exit code, and stdout and stderr if they
   were captured.

   Examples::

      >>> subprocess.run(["ls", "-l"])  # doesn't capture output
      CompletedProcess(args=['ls', '-l'], returncode=0)

      >>> subprocess.run("exit 1", shell=True, check=True)
      Traceback (most recent call last):
        ...
      subprocess.CalledProcessError: Command 'exit 1' returned non-zero exit status 1

      >>> subprocess.run(["ls", "-l", "/dev/null"], stdout=subprocess.PIPE)
      CompletedProcess(args=['ls', '-l', '/dev/null'], returncode=0,
      stdout=b'crw-rw-rw- 1 root root 1, 3 Jan 23 16:23 /dev/null\n')

   .. versionadded:: 3.5

.. class:: CompletedProcess

   The return value from :func:`run`, representing a process that has finished.

   .. attribute:: args

      The arguments used to launch the process. This may be a list or a string.

   .. attribute:: returncode

      Exit status of the child process. Typically, an exit status of 0 indicates
      that it ran successfully.

      A negative value ``-N`` indicates that the child was terminated by signal
      ``N`` (POSIX only).

   .. attribute:: stdout

      Captured stdout from the child process. A bytes sequence, or a string if
      :func:`run` was called with ``universal_newlines=True``. None if stdout
      was not captured.

      If you ran the process with ``stderr=subprocess.STDOUT``, stdout and
      stderr will be combined in this attribute, and :attr:`stderr` will be
      None.

   .. attribute:: stderr

      Captured stderr from the child process. A bytes sequence, or a string if
      :func:`run` was called with ``universal_newlines=True``. None if stderr
      was not captured.

   .. method:: check_returncode()

      If :attr:`returncode` is non-zero, raise a :exc:`CalledProcessError`.

   .. versionadded:: 3.5

.. data:: DEVNULL

   Special value that can be used as the *stdin*, *stdout* or *stderr* argument
   to :class:`Popen` and indicates that the special file :data:`os.devnull`
   will be used.

   .. versionadded:: 3.3


.. data:: PIPE

   Special value that can be used as the *stdin*, *stdout* or *stderr* argument
   to :class:`Popen` and indicates that a pipe to the standard stream should be
   opened.  Most useful with :meth:`Popen.communicate`.


.. data:: STDOUT

   Special value that can be used as the *stderr* argument to :class:`Popen` and
   indicates that standard error should go into the same handle as standard
   output.


.. exception:: SubprocessError

    Base class for all other exceptions from this module.

    .. versionadded:: 3.3


.. exception:: TimeoutExpired

    Subclass of :exc:`SubprocessError`, raised when a timeout expires
    while waiting for a child process.

    .. attribute:: cmd

        Command that was used to spawn the child process.

    .. attribute:: timeout

        Timeout in seconds.

    .. attribute:: output

        Output of the child process if it was captured by :func:`run` or
        :func:`check_output`.  Otherwise, ``None``.

    .. attribute:: stdout

        Alias for output, for symmetry with :attr:`stderr`.

    .. attribute:: stderr

        Stderr output of the child process if it was captured by :func:`run`.
        Otherwise, ``None``.

    .. versionadded:: 3.3

    .. versionchanged:: 3.5
        *stdout* and *stderr* attributes added

.. exception:: CalledProcessError

    Subclass of :exc:`SubprocessError`, raised when a process run by
    :func:`check_call` or :func:`check_output` returns a non-zero exit status.

    .. attribute:: returncode

        Exit status of the child process.

    .. attribute:: cmd

        Command that was used to spawn the child process.

    .. attribute:: output

        Output of the child process if it was captured by :func:`run` or
        :func:`check_output`.  Otherwise, ``None``.

    .. attribute:: stdout

        Alias for output, for symmetry with :attr:`stderr`.

    .. attribute:: stderr

        Stderr output of the child process if it was captured by :func:`run`.
        Otherwise, ``None``.

    .. versionchanged:: 3.5
        *stdout* and *stderr* attributes added


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
   are :data:`PIPE`, :data:`DEVNULL`, an existing file descriptor (a positive
   integer), an existing file object, and ``None``.  :data:`PIPE` indicates
   that a new pipe to the child should be created.  :data:`DEVNULL` indicates
   that the special file :data:`os.devnull` will be used.  With the default
   settings of ``None``, no redirection will occur; the child's file handles
   will be inherited from the parent.  Additionally, *stderr* can be
   :data:`STDOUT`, which indicates that the stderr data from the child
   process should be captured into the same file handle as for *stdout*.

   .. index::
      single: universal newlines; subprocess module

   If *universal_newlines* is ``False`` the file objects *stdin*, *stdout* and
   *stderr* will be opened as binary streams, and no line ending conversion is
   done.

   If *universal_newlines* is ``True``, these file objects
   will be opened as text streams in :term:`universal newlines` mode
   using the encoding returned by :func:`locale.getpreferredencoding(False)
   <locale.getpreferredencoding>`.  For *stdin*, line ending characters
   ``'\n'`` in the input will be converted to the default line separator
   :data:`os.linesep`.  For *stdout* and *stderr*, all line endings in the
   output will be converted to ``'\n'``.  For more information see the
   documentation of the :class:`io.TextIOWrapper` class when the *newline*
   argument to its constructor is ``None``.

   .. note::

      The newlines attribute of the file objects :attr:`Popen.stdin`,
      :attr:`Popen.stdout` and :attr:`Popen.stderr` are not updated by
      the :meth:`Popen.communicate` method.

   If *shell* is ``True``, the specified command will be executed through
   the shell.  This can be useful if you are using Python primarily for the
   enhanced control flow it offers over most system shells and still want
   convenient access to other shell features such as shell pipes, filename
   wildcards, environment variable expansion, and expansion of ``~`` to a
   user's home directory.  However, note that Python itself offers
   implementations of many shell-like features (in particular, :mod:`glob`,
   :mod:`fnmatch`, :func:`os.walk`, :func:`os.path.expandvars`,
   :func:`os.path.expanduser`, and :mod:`shutil`).

   .. versionchanged:: 3.3
      When *universal_newlines* is ``True``, the class uses the encoding
      :func:`locale.getpreferredencoding(False) <locale.getpreferredencoding>`
      instead of ``locale.getpreferredencoding()``.  See the
      :class:`io.TextIOWrapper` class for more information on this change.

   .. note::

      Read the `Security Considerations`_ section before using ``shell=True``.

These options, along with all of the other options, are described in more
detail in the :class:`Popen` constructor documentation.


Popen Constructor
^^^^^^^^^^^^^^^^^

The underlying process creation and management in this module is handled by
the :class:`Popen` class. It offers a lot of flexibility so that developers
are able to handle the less common cases not covered by the convenience
functions.


.. class:: Popen(args, bufsize=-1, executable=None, stdin=None, stdout=None, \
                 stderr=None, preexec_fn=None, close_fds=True, shell=False, \
                 cwd=None, env=None, universal_newlines=False, \
                 startupinfo=None, creationflags=0, restore_signals=True, \
                 start_new_session=False, pass_fds=())

   Execute a child program in a new process.  On POSIX, the class uses
   :meth:`os.execvp`-like behavior to execute the child program.  On Windows,
   the class uses the Windows ``CreateProcess()`` function.  The arguments to
   :class:`Popen` are as follows.

   *args* should be a sequence of program arguments or else a single string.
   By default, the program to execute is the first item in *args* if *args* is
   a sequence.  If *args* is a string, the interpretation is
   platform-dependent and described below.  See the *shell* and *executable*
   arguments for additional differences from the default behavior.  Unless
   otherwise stated, it is recommended to pass *args* as a sequence.

   On POSIX, if *args* is a string, the string is interpreted as the name or
   path of the program to execute.  However, this can only be done if not
   passing arguments to the program.

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

   On Windows, if *args* is a sequence, it will be converted to a string in a
   manner described in :ref:`converting-argument-sequence`.  This is because
   the underlying ``CreateProcess()`` operates on strings.

   The *shell* argument (which defaults to *False*) specifies whether to use
   the shell as the program to execute.  If *shell* is *True*, it is
   recommended to pass *args* as a string rather than as a sequence.

   On POSIX with ``shell=True``, the shell defaults to :file:`/bin/sh`.  If
   *args* is a string, the string specifies the command
   to execute through the shell.  This means that the string must be
   formatted exactly as it would be when typed at the shell prompt.  This
   includes, for example, quoting or backslash escaping filenames with spaces in
   them.  If *args* is a sequence, the first item specifies the command string, and
   any additional items will be treated as additional arguments to the shell
   itself.  That is to say, :class:`Popen` does the equivalent of::

      Popen(['/bin/sh', '-c', args[0], args[1], ...])

   On Windows with ``shell=True``, the :envvar:`COMSPEC` environment variable
   specifies the default shell.  The only time you need to specify
   ``shell=True`` on Windows is when the command you wish to execute is built
   into the shell (e.g. :command:`dir` or :command:`copy`).  You do not need
   ``shell=True`` to run a batch file or console-based executable.

   .. note::

      Read the `Security Considerations`_ section before using ``shell=True``.

   *bufsize* will be supplied as the corresponding argument to the
   :func:`open` function when creating the stdin/stdout/stderr pipe
   file objects:

   - :const:`0` means unbuffered (read and write are one
     system call and can return short)
   - :const:`1` means line buffered
     (only usable if ``universal_newlines=True`` i.e., in a text mode)
   - any other positive value means use a buffer of approximately that
     size
   - negative bufsize (the default) means the system default of
     io.DEFAULT_BUFFER_SIZE will be used.

   .. versionchanged:: 3.3.1
      *bufsize* now defaults to -1 to enable buffering by default to match the
      behavior that most code expects.  In versions prior to Python 3.2.4 and
      3.3.1 it incorrectly defaulted to :const:`0` which was unbuffered
      and allowed short reads.  This was unintentional and did not match the
      behavior of Python 2 as most code expected.

   The *executable* argument specifies a replacement program to execute.   It
   is very seldom needed.  When ``shell=False``, *executable* replaces the
   program to execute specified by *args*.  However, the original *args* is
   still passed to the program.  Most programs treat the program specified
   by *args* as the command name, which can then be different from the program
   actually executed.  On POSIX, the *args* name
   becomes the display name for the executable in utilities such as
   :program:`ps`.  If ``shell=True``, on POSIX the *executable* argument
   specifies a replacement shell for the default :file:`/bin/sh`.

   *stdin*, *stdout* and *stderr* specify the executed program's standard input,
   standard output and standard error file handles, respectively.  Valid values
   are :data:`PIPE`, :data:`DEVNULL`, an existing file descriptor (a positive
   integer), an existing :term:`file object`, and ``None``.  :data:`PIPE`
   indicates that a new pipe to the child should be created.  :data:`DEVNULL`
   indicates that the special file :data:`os.devnull` will be used. With the
   default settings of ``None``, no redirection will occur; the child's file
   handles will be inherited from the parent.  Additionally, *stderr* can be
   :data:`STDOUT`, which indicates that the stderr data from the applications
   should be captured into the same file handle as for stdout.

   If *preexec_fn* is set to a callable object, this object will be called in the
   child process just before the child is executed.
   (POSIX only)

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
   :const:`2` will be closed before the child process is executed. (POSIX only).
   The default varies by platform:  Always true on POSIX.  On Windows it is
   true when *stdin*/*stdout*/*stderr* are :const:`None`, false otherwise.
   On Windows, if *close_fds* is true then no handles will be inherited by the
   child process.  Note that on Windows, you cannot set *close_fds* to true and
   also redirect the standard handles by setting *stdin*, *stdout* or *stderr*.

   .. versionchanged:: 3.2
      The default for *close_fds* was changed from :const:`False` to
      what is described above.

   *pass_fds* is an optional sequence of file descriptors to keep open
   between the parent and child.  Providing any *pass_fds* forces
   *close_fds* to be :const:`True`.  (POSIX only)

   .. versionadded:: 3.2
      The *pass_fds* parameter was added.

   If *cwd* is not ``None``, the function changes the working directory to
   *cwd* before executing the child.  In particular, the function looks for
   *executable* (or for the first item in *args*) relative to *cwd* if the
   executable path is a relative path.

   If *restore_signals* is true (the default) all signals that Python has set to
   SIG_IGN are restored to SIG_DFL in the child process before the exec.
   Currently this includes the SIGPIPE, SIGXFZ and SIGXFSZ signals.
   (POSIX only)

   .. versionchanged:: 3.2
      *restore_signals* was added.

   If *start_new_session* is true the setsid() system call will be made in the
   child process prior to the execution of the subprocess.  (POSIX only)

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

   If *universal_newlines* is ``True``, the file objects *stdin*, *stdout*
   and *stderr* are opened as text streams in universal newlines mode, as
   described above in :ref:`frequently-used-arguments`, otherwise they are
   opened as binary streams.

   If given, *startupinfo* will be a :class:`STARTUPINFO` object, which is
   passed to the underlying ``CreateProcess`` function.
   *creationflags*, if given, can be :data:`CREATE_NEW_CONSOLE` or
   :data:`CREATE_NEW_PROCESS_GROUP`. (Windows only)

   Popen objects are supported as context managers via the :keyword:`with` statement:
   on exit, standard file descriptors are closed, and the process is waited for.
   ::

      with Popen(["ifconfig"], stdout=PIPE) as proc:
          log.write(proc.stdout.read())

   .. versionchanged:: 3.2
      Added context manager support.


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

All of the functions and methods that accept a *timeout* parameter, such as
:func:`call` and :meth:`Popen.communicate` will raise :exc:`TimeoutExpired` if
the timeout expires before the process exits.

Exceptions defined in this module all inherit from :exc:`SubprocessError`.

   .. versionadded:: 3.3
      The :exc:`SubprocessError` base class was added.


Security Considerations
-----------------------

Unlike some other popen functions, this implementation will never
implicitly call a system shell.  This means that all characters,
including shell metacharacters, can safely be passed to child processes.
If the shell is invoked explicitly, via ``shell=True``, it is the application's
responsibility to ensure that all whitespace and metacharacters are
quoted appropriately to avoid
`shell injection <http://en.wikipedia.org/wiki/Shell_injection#Shell_injection>`_
vulnerabilities.

When using ``shell=True``, the :func:`shlex.quote` function can be
used to properly escape whitespace and shell metacharacters in strings
that are going to be used to construct shell commands.


Popen Objects
-------------

Instances of the :class:`Popen` class have the following methods:


.. method:: Popen.poll()

   Check if child process has terminated.  Set and return
   :attr:`~Popen.returncode` attribute.


.. method:: Popen.wait(timeout=None)

   Wait for child process to terminate.  Set and return
   :attr:`~Popen.returncode` attribute.

   If the process does not terminate after *timeout* seconds, raise a
   :exc:`TimeoutExpired` exception.  It is safe to catch this exception and
   retry the wait.

   .. note::

      This will deadlock when using ``stdout=PIPE`` or ``stderr=PIPE``
      and the child process generates enough output to a pipe such that
      it blocks waiting for the OS pipe buffer to accept more data.
      Use :meth:`Popen.communicate` when using pipes to avoid that.

   .. note::

      The function is implemented using a busy loop (non-blocking call and
      short sleeps). Use the :mod:`asyncio` module for an asynchronous wait:
      see :class:`asyncio.create_subprocess_exec`.

   .. versionchanged:: 3.3
      *timeout* was added.

   .. deprecated:: 3.4

      Do not use the *endtime* parameter.  It is was unintentionally
      exposed in 3.3 but was left undocumented as it was intended to be
      private for internal use.  Use *timeout* instead.

.. method:: Popen.communicate(input=None, timeout=None)

   Interact with process: Send data to stdin.  Read data from stdout and stderr,
   until end-of-file is reached.  Wait for process to terminate.  The optional
   *input* argument should be data to be sent to the child process, or
   ``None``, if no data should be sent to the child.  The type of *input*
   must be bytes or, if *universal_newlines* was ``True``, a string.

   :meth:`communicate` returns a tuple ``(stdout_data, stderr_data)``.
   The data will be bytes or, if *universal_newlines* was ``True``, strings.

   Note that if you want to send data to the process's stdin, you need to create
   the Popen object with ``stdin=PIPE``.  Similarly, to get anything other than
   ``None`` in the result tuple, you need to give ``stdout=PIPE`` and/or
   ``stderr=PIPE`` too.

   If the process does not terminate after *timeout* seconds, a
   :exc:`TimeoutExpired` exception will be raised.  Catching this exception and
   retrying communication will not lose any output.

   The child process is not killed if the timeout expires, so in order to
   cleanup properly a well-behaved application should kill the child process and
   finish communication::

      proc = subprocess.Popen(...)
      try:
          outs, errs = proc.communicate(timeout=15)
      except TimeoutExpired:
          proc.kill()
          outs, errs = proc.communicate()

   .. note::

      The data read is buffered in memory, so do not use this method if the data
      size is large or unlimited.

   .. versionchanged:: 3.3
      *timeout* was added.


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

.. attribute:: Popen.args

   The *args* argument as it was passed to :class:`Popen` -- a
   sequence of program arguments or else a single string.

   .. versionadded:: 3.3

.. attribute:: Popen.stdin

   If the *stdin* argument was :data:`PIPE`, this attribute is a writeable
   stream object as returned by :func:`open`. If the *universal_newlines*
   argument was ``True``, the stream is a text stream, otherwise it is a byte
   stream. If the *stdin* argument was not :data:`PIPE`, this attribute is
   ``None``.


.. attribute:: Popen.stdout

   If the *stdout* argument was :data:`PIPE`, this attribute is a readable
   stream object as returned by :func:`open`. Reading from the stream provides
   output from the child process. If the *universal_newlines* argument was
   ``True``, the stream is a text stream, otherwise it is a byte stream. If the
   *stdout* argument was not :data:`PIPE`, this attribute is ``None``.


.. attribute:: Popen.stderr

   If the *stderr* argument was :data:`PIPE`, this attribute is a readable
   stream object as returned by :func:`open`. Reading from the stream provides
   error output from the child process. If the *universal_newlines* argument was
   ``True``, the stream is a text stream, otherwise it is a byte stream. If the
   *stderr* argument was not :data:`PIPE`, this attribute is ``None``.

.. warning::

   Use :meth:`~Popen.communicate` rather than :attr:`.stdin.write <Popen.stdin>`,
   :attr:`.stdout.read <Popen.stdout>` or :attr:`.stderr.read <Popen.stderr>` to avoid
   deadlocks due to any of the other OS pipe buffers filling up and blocking the
   child process.


.. attribute:: Popen.pid

   The process ID of the child process.

   Note that if you set the *shell* argument to ``True``, this is the process ID
   of the spawned shell.


.. attribute:: Popen.returncode

   The child return code, set by :meth:`poll` and :meth:`wait` (and indirectly
   by :meth:`communicate`).  A ``None`` value indicates that the process
   hasn't terminated yet.

   A negative value ``-N`` indicates that the child was terminated by signal
   ``N`` (POSIX only).


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

.. data:: CREATE_NEW_PROCESS_GROUP

   A :class:`Popen` ``creationflags`` parameter to specify that a new process
   group will be created. This flag is necessary for using :func:`os.kill`
   on the subprocess.

   This flag is ignored if :data:`CREATE_NEW_CONSOLE` is specified.

.. _call-function-trio:

Older high-level API
--------------------

Prior to Python 3.5, these three functions comprised the high level API to
subprocess. You can now use :func:`run` in many cases, but lots of existing code
calls these functions.

.. function:: call(args, *, stdin=None, stdout=None, stderr=None, shell=False, timeout=None)

   Run the command described by *args*.  Wait for command to complete, then
   return the :attr:`~Popen.returncode` attribute.

   This is equivalent to::

       run(...).returncode

   (except that the *input* and *check* parameters are not supported)

   The arguments shown above are merely the most
   common ones. The full function signature is largely the
   same as that of the :class:`Popen` constructor - this function passes all
   supplied arguments other than *timeout* directly through to that interface.

   .. note::

      Do not use ``stdout=PIPE`` or ``stderr=PIPE`` with this
      function.  The child process will block if it generates enough
      output to a pipe to fill up the OS pipe buffer as the pipes are
      not being read from.

   .. versionchanged:: 3.3
      *timeout* was added.

.. function:: check_call(args, *, stdin=None, stdout=None, stderr=None, shell=False, timeout=None)

   Run command with arguments.  Wait for command to complete. If the return
   code was zero then return, otherwise raise :exc:`CalledProcessError`. The
   :exc:`CalledProcessError` object will have the return code in the
   :attr:`~CalledProcessError.returncode` attribute.

   This is equivalent to::

       run(..., check=True)

   (except that the *input* parameter is not supported)

   The arguments shown above are merely the most
   common ones. The full function signature is largely the
   same as that of the :class:`Popen` constructor - this function passes all
   supplied arguments other than *timeout* directly through to that interface.

   .. note::

      Do not use ``stdout=PIPE`` or ``stderr=PIPE`` with this
      function.  The child process will block if it generates enough
      output to a pipe to fill up the OS pipe buffer as the pipes are
      not being read from.

   .. versionchanged:: 3.3
      *timeout* was added.


.. function:: check_output(args, *, stdin=None, stderr=None, shell=False, universal_newlines=False, timeout=None)

   Run command with arguments and return its output.

   If the return code was non-zero it raises a :exc:`CalledProcessError`. The
   :exc:`CalledProcessError` object will have the return code in the
   :attr:`~CalledProcessError.returncode` attribute and any output in the
   :attr:`~CalledProcessError.output` attribute.

   This is equivalent to::

       run(..., check=True, stdout=PIPE).stdout

   The arguments shown above are merely the most common ones.
   The full function signature is largely the same as that of :func:`run` -
   most arguments are passed directly through to that interface.
   However, explicitly passing ``input=None`` to inherit the parent's
   standard input file handle is not supported.

   By default, this function will return the data as encoded bytes. The actual
   encoding of the output data may depend on the command being invoked, so the
   decoding to text will often need to be handled at the application level.

   This behaviour may be overridden by setting *universal_newlines* to
   ``True`` as described above in :ref:`frequently-used-arguments`.

   To also capture standard error in the result, use
   ``stderr=subprocess.STDOUT``::

      >>> subprocess.check_output(
      ...     "ls non_existent_file; exit 0",
      ...     stderr=subprocess.STDOUT,
      ...     shell=True)
      'ls: non_existent_file: No such file or directory\n'

   .. versionadded:: 3.1

   .. versionchanged:: 3.3
      *timeout* was added.

   .. versionchanged:: 3.4
      Support for the *input* keyword argument was added.

.. _subprocess-replacements:

Replacing Older Functions with the :mod:`subprocess` Module
-----------------------------------------------------------

In this section, "a becomes b" means that b can be used as a replacement for a.

.. note::

   All "a" functions in this section fail (more or less) silently if the
   executed program cannot be found; the "b" replacements raise :exc:`OSError`
   instead.

   In addition, the replacements using :func:`check_output` will fail with a
   :exc:`CalledProcessError` if the requested operation produces a non-zero
   return code. The output is still available as the
   :attr:`~CalledProcessError.output` attribute of the raised exception.

In the following examples, we assume that the relevant functions have already
been imported from the :mod:`subprocess` module.


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
   p = Popen("somestring", shell=True, bufsize=bufsize,
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
  ``close_fds=True`` with :class:`Popen` to guarantee this behavior on
  all platforms or past Python versions.


Legacy Shell Invocation Functions
---------------------------------

This module also provides the following legacy functions from the 2.x
``commands`` module. These operations implicitly invoke the system shell and
none of the guarantees described above regarding security and exception
handling consistency are valid for these functions.

.. function:: getstatusoutput(cmd)

   Return ``(status, output)`` of executing *cmd* in a shell.

   Execute the string *cmd* in a shell with :meth:`Popen.check_output` and
   return a 2-tuple ``(status, output)``. Universal newlines mode is used;
   see the notes on :ref:`frequently-used-arguments` for more details.

   A trailing newline is stripped from the output.
   The exit status for the command can be interpreted
   according to the rules for the C function :c:func:`wait`.  Example::

      >>> subprocess.getstatusoutput('ls /bin/ls')
      (0, '/bin/ls')
      >>> subprocess.getstatusoutput('cat /bin/junk')
      (256, 'cat: /bin/junk: No such file or directory')
      >>> subprocess.getstatusoutput('/bin/junk')
      (256, 'sh: /bin/junk: not found')

   Availability: POSIX & Windows

   .. versionchanged:: 3.3.4
      Windows support added


.. function:: getoutput(cmd)

   Return output (stdout and stderr) of executing *cmd* in a shell.

   Like :func:`getstatusoutput`, except the exit status is ignored and the return
   value is a string containing the command's output.  Example::

      >>> subprocess.getoutput('ls /bin/ls')
      '/bin/ls'

   Availability: POSIX & Windows

   .. versionchanged:: 3.3.4
      Windows support added


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


.. seealso::

   :mod:`shlex`
      Module which provides function to parse and escape command lines.
