.. currentmodule:: asyncio

Subprocess
==========

Operating system support
------------------------

On Windows, the default event loop uses :class:`selectors.SelectSelector`
which only supports sockets. The :class:`ProactorEventLoop` should be used to
support subprocesses.

On Mac OS X older than 10.9 (Mavericks), :class:`selectors.KqueueSelector`
does not support character devices like PTY, whereas it is used by the
default event loop. The :class:`SelectorEventLoop` can be used with
:class:`SelectSelector` or :class:`PollSelector` to handle character
devices on Mac OS X 10.6 (Snow Leopard) and later.


Create a subprocess: high-level API using Process
-------------------------------------------------

.. function:: create_subprocess_shell(cmd, stdin=None, stdout=None, stderr=None, loop=None, limit=None, \*\*kwds)

   Run the shell command *cmd* given as a string. Return a :class:`~asyncio.subprocess.Process`
   instance.

   The optional *limit* parameter sets the buffer limit passed to the
   :class:`StreamReader`.

   This function is a :ref:`coroutine <coroutine>`.

.. function:: create_subprocess_exec(\*args, stdin=None, stdout=None, stderr=None, loop=None, limit=None, \*\*kwds)

   Create a subprocess. Return a :class:`~asyncio.subprocess.Process` instance.

   The optional *limit* parameter sets the buffer limit passed to the
   :class:`StreamReader`.

   This function is a :ref:`coroutine <coroutine>`.

Use the :meth:`BaseEventLoop.connect_read_pipe` and
:meth:`BaseEventLoop.connect_write_pipe` methods to connect pipes.


Create a subprocess: low-level API using subprocess.Popen
---------------------------------------------------------

Run subprocesses asynchronously using the :mod:`subprocess` module.

.. method:: BaseEventLoop.subprocess_exec(protocol_factory, \*args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, \*\*kwargs)

   Create a subprocess from one or more string arguments, where the first string
   specifies the program to execute, and the remaining strings specify the
   program's arguments. (Thus, together the string arguments form the
   ``sys.argv`` value of the program, assuming it is a Python script.) This is
   similar to the standard library :class:`subprocess.Popen` class called with
   shell=False and the list of strings passed as the first argument;
   however, where :class:`~subprocess.Popen` takes a single argument which is
   list of strings, :func:`subprocess_exec` takes multiple string arguments.

   Other parameters:

   * *stdin*: Either a file-like object representing the pipe to be connected
     to the subprocess's standard input stream using
     :meth:`~BaseEventLoop.connect_write_pipe`, or the constant
     :const:`subprocess.PIPE` (the default). By default a new pipe will be
     created and connected.

   * *stdout*: Either a file-like object representing the pipe to be connected
     to the subprocess's standard output stream using
     :meth:`~BaseEventLoop.connect_read_pipe`, or the constant
     :const:`subprocess.PIPE` (the default). By default a new pipe will be
     created and connected.

   * *stderr*: Either a file-like object representing the pipe to be connected
     to the subprocess's standard error stream using
     :meth:`~BaseEventLoop.connect_read_pipe`, or one of the constants
     :const:`subprocess.PIPE` (the default) or :const:`subprocess.STDOUT`.
     By default a new pipe will be created and connected. When
     :const:`subprocess.STDOUT` is specified, the subprocess's standard error
     stream will be connected to the same pipe as the standard output stream.

   * All other keyword arguments are passed to :class:`subprocess.Popen`
     without interpretation, except for *bufsize*, *universal_newlines* and
     *shell*, which should not be specified at all.

   Returns a pair of ``(transport, protocol)``, where *transport* is an
   instance of :class:`BaseSubprocessTransport`.

   This method is a :ref:`coroutine <coroutine>`.

   See the constructor of the :class:`subprocess.Popen` class for parameters.

.. method:: BaseEventLoop.subprocess_shell(protocol_factory, cmd, \*, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, \*\*kwargs)

   Create a subprocess from *cmd*, which is a string using the platform's
   "shell" syntax. This is similar to the standard library
   :class:`subprocess.Popen` class called with ``shell=True``.

   See :meth:`~BaseEventLoop.subprocess_exec` for more details about
   the remaining arguments.

   Returns a pair of ``(transport, protocol)``, where *transport* is an
   instance of :class:`BaseSubprocessTransport`.

   This method is a :ref:`coroutine <coroutine>`.

   See the constructor of the :class:`subprocess.Popen` class for parameters.

.. seealso::

   The :meth:`BaseEventLoop.connect_read_pipe` and
   :meth:`BaseEventLoop.connect_write_pipe` methods.


Constants
---------

.. data:: asyncio.subprocess.PIPE

   Special value that can be used as the *stdin*, *stdout* or *stderr* argument
   to :func:`create_subprocess_shell` and :func:`create_subprocess_exec` and
   indicates that a pipe to the standard stream should be opened.

.. data:: asyncio.subprocess.STDOUT

   Special value that can be used as the *stderr* argument to
   :func:`create_subprocess_shell` and :func:`create_subprocess_exec` and
   indicates that standard error should go into the same handle as standard
   output.

.. data:: asyncio.subprocess.DEVNULL

   Special value that can be used as the *stdin*, *stdout* or *stderr* argument
   to :func:`create_subprocess_shell` and :func:`create_subprocess_exec` and
   indicates that the special file :data:`os.devnull` will be used.


Process
-------

.. class:: asyncio.subprocess.Process

   .. attribute:: pid

      The identifier of the process.

      Note that if you set the *shell* argument to ``True``, this is the
      process identifier of the spawned shell.

   .. attribute:: returncode

      Return code of the process when it exited.  A ``None`` value indicates
      that the process has not terminated yet.

      A negative value ``-N`` indicates that the child was terminated by signal
      ``N`` (Unix only).

   .. attribute:: stdin

      Standard input stream (write), ``None`` if the process was created with
      ``stdin=None``.

   .. attribute:: stdout

      Standard output stream (read), ``None`` if the process was created with
      ``stdout=None``.

   .. attribute:: stderr

      Standard error stream (read), ``None`` if the process was created with
      ``stderr=None``.

   .. method:: communicate(input=None)

      Interact with process: Send data to stdin.  Read data from stdout and
      stderr, until end-of-file is reached.  Wait for process to terminate.
      The optional *input* argument should be data to be sent to the child
      process, or ``None``, if no data should be sent to the child.  The type
      of *input* must be bytes.

      :meth:`communicate` returns a tuple ``(stdoutdata, stderrdata)``.

      Note that if you want to send data to the process's stdin, you need to
      create the Process object with ``stdin=PIPE``.  Similarly, to get anything
      other than ``None`` in the result tuple, you need to give ``stdout=PIPE``
      and/or ``stderr=PIPE`` too.

      .. note::

         The data read is buffered in memory, so do not use this method if the
         data size is large or unlimited.

      This method is a :ref:`coroutine <coroutine>`.

   .. method:: kill()

      Kills the child. On Posix OSs the function sends :py:data:`SIGKILL` to
      the child.  On Windows :meth:`kill` is an alias for :meth:`terminate`.

   .. method:: send_signal(signal)

      Sends the signal *signal* to the child process.

      .. note::

         On Windows, :py:data:`SIGTERM` is an alias for :meth:`terminate`.
         ``CTRL_C_EVENT`` and ``CTRL_BREAK_EVENT`` can be sent to processes
         started with a *creationflags* parameter which includes
         ``CREATE_NEW_PROCESS_GROUP``.

   .. method:: terminate()

      Stop the child. On Posix OSs the method sends :py:data:`signal.SIGTERM`
      to the child. On Windows the Win32 API function
      :c:func:`TerminateProcess` is called to stop the child.

   .. method:: wait():

      Wait for child process to terminate.  Set and return :attr:`returncode`
      attribute.

      This method is a :ref:`coroutine <coroutine>`.


Example
-------

Implement a function similar to :func:`subprocess.getstatusoutput`, except that
it does not use a shell. Get the output of the "python -m platform" command and
display the output::

    import asyncio
    import os
    import sys
    from asyncio import subprocess

    @asyncio.coroutine
    def getstatusoutput(*args):
        proc = yield from asyncio.create_subprocess_exec(
                                      *args,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT)
        try:
            stdout, _ = yield from proc.communicate()
        except:
            proc.kill()
            yield from proc.wait()
            raise
        exitcode = yield from proc.wait()
        return (exitcode, stdout)

    if os.name == 'nt':
        loop = asyncio.ProactorEventLoop()
        asyncio.set_event_loop(loop)
    else:
        loop = asyncio.get_event_loop()
    coro = getstatusoutput(sys.executable, '-m', 'platform')
    exitcode, stdout = loop.run_until_complete(coro)
    if not exitcode:
        stdout = stdout.decode('ascii').rstrip()
        print("Platform: %s" % stdout)
    else:
        print("Python failed with exit code %s:" % exitcode)
        sys.stdout.flush()
        sys.stdout.buffer.flush()
        sys.stdout.buffer.write(stdout)
        sys.stdout.buffer.flush()
    loop.close()
