.. currentmodule:: asyncio

Subprocess
==========

Create a subprocess
-------------------

.. function:: create_subprocess_shell(cmd, stdin=None, stdout=None, stderr=None, loop=None, limit=None, \*\*kwds)

   Run the shell command *cmd* given as a string. Return a :class:`~asyncio.subprocess.Process`
   instance.

   This function is a :ref:`coroutine <coroutine>`.

.. function:: create_subprocess_exec(\*args, stdin=None, stdout=None, stderr=None, loop=None, limit=None, \*\*kwds)

   Create a subprocess. Return a :class:`~asyncio.subprocess.Process` instance.

   This function is a :ref:`coroutine <coroutine>`.

Use the :meth:`BaseEventLoop.connect_read_pipe` and
:meth:`BaseEventLoop.connect_write_pipe` methods to connect pipes.

.. seealso::

   The :meth:`BaseEventLoop.subprocess_exec` and
   :meth:`BaseEventLoop.subprocess_shell` methods.


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

   Special value that can be used as the *stderr* argument to
   :func:`create_subprocess_shell` and :func:`create_subprocess_exec` and
   indicates that standard error should go into the same handle as standard
   output.


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

   .. method:: wait(self):

      Wait for child process to terminate.  Set and return :attr:`returncode`
      attribute.


Example
-------

Implement a function similar to :func:`subprocess.getstatusoutput`, except that
it does not use a shell. Get the output of the "python -m platform" command and
display the output::

    import asyncio
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
