# subprocess - Subprocesses with accessible I/O streams
#
# For more information about this module, see PEP 324.
#
# Copyright (c) 2003-2005 by Peter Astrand <astrand@lysator.liu.se>
#
# Licensed to PSF under a Contributor Agreement.
# See http://www.python.org/2.4/license for licensing details.

r"""Subprocesses with accessible I/O streams

This module allows you to spawn processes, connect to their
input/output/error pipes, and obtain their return codes.

For a complete description of this module see the Python documentation.

Main API
========
run(...): Runs a command, waits for it to complete, then returns a
          CompletedProcess instance.
Popen(...): A class for flexibly executing a command in a new process

Constants
---------
DEVNULL: Special value that indicates that os.devnull should be used
PIPE:    Special value that indicates a pipe should be created
STDOUT:  Special value that indicates that stderr should go to stdout


Older API
=========
call(...): Runs a command, waits for it to complete, then returns
    the return code.
check_call(...): Same as call() but raises CalledProcessError()
    if return code is not 0
check_output(...): Same as check_call() but returns the contents of
    stdout instead of a return code
getoutput(...): Runs a command in the shell, waits for it to complete,
    then returns the output
getstatusoutput(...): Runs a command in the shell, waits for it to complete,
    then returns a (status, output) tuple
"""

import sys
_mswindows = (sys.platform == "win32")

import io
import os
import time
import signal
import builtins
import warnings
import errno
from time import monotonic as _time

# Exception classes used by this module.
class SubprocessError(Exception): pass


class CalledProcessError(SubprocessError):
    """Raised when run() is called with check=True and the process
    returns a non-zero exit status.

    Attributes:
      cmd, returncode, stdout, stderr, output
    """
    def __init__(self, returncode, cmd, output=None, stderr=None):
        self.returncode = returncode
        self.cmd = cmd
        self.output = output
        self.stderr = stderr

    def __str__(self):
        return "Command '%s' returned non-zero exit status %d" % (self.cmd, self.returncode)

    @property
    def stdout(self):
        """Alias for output attribute, to match stderr"""
        return self.output

    @stdout.setter
    def stdout(self, value):
        # There's no obvious reason to set this, but allow it anyway so
        # .stdout is a transparent alias for .output
        self.output = value


class TimeoutExpired(SubprocessError):
    """This exception is raised when the timeout expires while waiting for a
    child process.

    Attributes:
        cmd, output, stdout, stderr, timeout
    """
    def __init__(self, cmd, timeout, output=None, stderr=None):
        self.cmd = cmd
        self.timeout = timeout
        self.output = output
        self.stderr = stderr

    def __str__(self):
        return ("Command '%s' timed out after %s seconds" %
                (self.cmd, self.timeout))

    @property
    def stdout(self):
        return self.output

    @stdout.setter
    def stdout(self, value):
        # There's no obvious reason to set this, but allow it anyway so
        # .stdout is a transparent alias for .output
        self.output = value


if _mswindows:
    import threading
    import msvcrt
    import _winapi
    class STARTUPINFO:
        dwFlags = 0
        hStdInput = None
        hStdOutput = None
        hStdError = None
        wShowWindow = 0
else:
    import _posixsubprocess
    import select
    import selectors
    try:
        import threading
    except ImportError:
        import dummy_threading as threading

    # When select or poll has indicated that the file is writable,
    # we can write up to _PIPE_BUF bytes without risk of blocking.
    # POSIX defines PIPE_BUF as >= 512.
    _PIPE_BUF = getattr(select, 'PIPE_BUF', 512)

    # poll/select have the advantage of not requiring any extra file
    # descriptor, contrarily to epoll/kqueue (also, they require a single
    # syscall).
    if hasattr(selectors, 'PollSelector'):
        _PopenSelector = selectors.PollSelector
    else:
        _PopenSelector = selectors.SelectSelector


__all__ = ["Popen", "PIPE", "STDOUT", "call", "check_call", "getstatusoutput",
           "getoutput", "check_output", "run", "CalledProcessError", "DEVNULL",
           "SubprocessError", "TimeoutExpired", "CompletedProcess"]
           # NOTE: We intentionally exclude list2cmdline as it is
           # considered an internal implementation detail.  issue10838.

if _mswindows:
    from _winapi import (CREATE_NEW_CONSOLE, CREATE_NEW_PROCESS_GROUP,
                         STD_INPUT_HANDLE, STD_OUTPUT_HANDLE,
                         STD_ERROR_HANDLE, SW_HIDE,
                         STARTF_USESTDHANDLES, STARTF_USESHOWWINDOW)

    __all__.extend(["CREATE_NEW_CONSOLE", "CREATE_NEW_PROCESS_GROUP",
                    "STD_INPUT_HANDLE", "STD_OUTPUT_HANDLE",
                    "STD_ERROR_HANDLE", "SW_HIDE",
                    "STARTF_USESTDHANDLES", "STARTF_USESHOWWINDOW"])

    class Handle(int):
        closed = False

        def Close(self, CloseHandle=_winapi.CloseHandle):
            if not self.closed:
                self.closed = True
                CloseHandle(self)

        def Detach(self):
            if not self.closed:
                self.closed = True
                return int(self)
            raise ValueError("already closed")

        def __repr__(self):
            return "%s(%d)" % (self.__class__.__name__, int(self))

        __del__ = Close
        __str__ = __repr__


# This lists holds Popen instances for which the underlying process had not
# exited at the time its __del__ method got called: those processes are wait()ed
# for synchronously from _cleanup() when a new Popen object is created, to avoid
# zombie processes.
_active = []

def _cleanup():
    for inst in _active[:]:
        res = inst._internal_poll(_deadstate=sys.maxsize)
        if res is not None:
            try:
                _active.remove(inst)
            except ValueError:
                # This can happen if two threads create a new Popen instance.
                # It's harmless that it was already removed, so ignore.
                pass

PIPE = -1
STDOUT = -2
DEVNULL = -3


# XXX This function is only used by multiprocessing and the test suite,
# but it's here so that it can be imported when Python is compiled without
# threads.

def _args_from_interpreter_flags():
    """Return a list of command-line arguments reproducing the current
    settings in sys.flags and sys.warnoptions."""
    flag_opt_map = {
        'debug': 'd',
        # 'inspect': 'i',
        # 'interactive': 'i',
        'optimize': 'O',
        'dont_write_bytecode': 'B',
        'no_user_site': 's',
        'no_site': 'S',
        'ignore_environment': 'E',
        'verbose': 'v',
        'bytes_warning': 'b',
        'quiet': 'q',
    }
    args = []
    for flag, opt in flag_opt_map.items():
        v = getattr(sys.flags, flag)
        if v > 0:
            args.append('-' + opt * v)
    for opt in sys.warnoptions:
        args.append('-W' + opt)
    return args


def call(*popenargs, timeout=None, **kwargs):
    """Run command with arguments.  Wait for command to complete or
    timeout, then return the returncode attribute.

    The arguments are the same as for the Popen constructor.  Example:

    retcode = call(["ls", "-l"])
    """
    with Popen(*popenargs, **kwargs) as p:
        try:
            return p.wait(timeout=timeout)
        except:
            p.kill()
            p.wait()
            raise


def check_call(*popenargs, **kwargs):
    """Run command with arguments.  Wait for command to complete.  If
    the exit code was zero then return, otherwise raise
    CalledProcessError.  The CalledProcessError object will have the
    return code in the returncode attribute.

    The arguments are the same as for the call function.  Example:

    check_call(["ls", "-l"])
    """
    retcode = call(*popenargs, **kwargs)
    if retcode:
        cmd = kwargs.get("args")
        if cmd is None:
            cmd = popenargs[0]
        raise CalledProcessError(retcode, cmd)
    return 0


def check_output(*popenargs, timeout=None, **kwargs):
    r"""Run command with arguments and return its output.

    If the exit code was non-zero it raises a CalledProcessError.  The
    CalledProcessError object will have the return code in the returncode
    attribute and output in the output attribute.

    The arguments are the same as for the Popen constructor.  Example:

    >>> check_output(["ls", "-l", "/dev/null"])
    b'crw-rw-rw- 1 root root 1, 3 Oct 18  2007 /dev/null\n'

    The stdout argument is not allowed as it is used internally.
    To capture standard error in the result, use stderr=STDOUT.

    >>> check_output(["/bin/sh", "-c",
    ...               "ls -l non_existent_file ; exit 0"],
    ...              stderr=STDOUT)
    b'ls: non_existent_file: No such file or directory\n'

    There is an additional optional argument, "input", allowing you to
    pass a string to the subprocess's stdin.  If you use this argument
    you may not also use the Popen constructor's "stdin" argument, as
    it too will be used internally.  Example:

    >>> check_output(["sed", "-e", "s/foo/bar/"],
    ...              input=b"when in the course of fooman events\n")
    b'when in the course of barman events\n'

    If universal_newlines=True is passed, the "input" argument must be a
    string and the return value will be a string rather than bytes.
    """
    if 'stdout' in kwargs:
        raise ValueError('stdout argument not allowed, it will be overridden.')

    if 'input' in kwargs and kwargs['input'] is None:
        # Explicitly passing input=None was previously equivalent to passing an
        # empty string. That is maintained here for backwards compatibility.
        kwargs['input'] = '' if kwargs.get('universal_newlines', False) else b''

    return run(*popenargs, stdout=PIPE, timeout=timeout, check=True,
               **kwargs).stdout


class CompletedProcess(object):
    """A process that has finished running.

    This is returned by run().

    Attributes:
      args: The list or str args passed to run().
      returncode: The exit code of the process, negative for signals.
      stdout: The standard output (None if not captured).
      stderr: The standard error (None if not captured).
    """
    def __init__(self, args, returncode, stdout=None, stderr=None):
        self.args = args
        self.returncode = returncode
        self.stdout = stdout
        self.stderr = stderr

    def __repr__(self):
        args = ['args={!r}'.format(self.args),
                'returncode={!r}'.format(self.returncode)]
        if self.stdout is not None:
            args.append('stdout={!r}'.format(self.stdout))
        if self.stderr is not None:
            args.append('stderr={!r}'.format(self.stderr))
        return "{}({})".format(type(self).__name__, ', '.join(args))

    def check_returncode(self):
        """Raise CalledProcessError if the exit code is non-zero."""
        if self.returncode:
            raise CalledProcessError(self.returncode, self.args, self.stdout,
                                     self.stderr)


def run(*popenargs, input=None, timeout=None, check=False, **kwargs):
    """Run command with arguments and return a CompletedProcess instance.

    The returned instance will have attributes args, returncode, stdout and
    stderr. By default, stdout and stderr are not captured, and those attributes
    will be None. Pass stdout=PIPE and/or stderr=PIPE in order to capture them.

    If check is True and the exit code was non-zero, it raises a
    CalledProcessError. The CalledProcessError object will have the return code
    in the returncode attribute, and output & stderr attributes if those streams
    were captured.

    If timeout is given, and the process takes too long, a TimeoutExpired
    exception will be raised.

    There is an optional argument "input", allowing you to
    pass a string to the subprocess's stdin.  If you use this argument
    you may not also use the Popen constructor's "stdin" argument, as
    it will be used internally.

    The other arguments are the same as for the Popen constructor.

    If universal_newlines=True is passed, the "input" argument must be a
    string and stdout/stderr in the returned object will be strings rather than
    bytes.
    """
    if input is not None:
        if 'stdin' in kwargs:
            raise ValueError('stdin and input arguments may not both be used.')
        kwargs['stdin'] = PIPE

    with Popen(*popenargs, **kwargs) as process:
        try:
            stdout, stderr = process.communicate(input, timeout=timeout)
        except TimeoutExpired:
            process.kill()
            stdout, stderr = process.communicate()
            raise TimeoutExpired(process.args, timeout, output=stdout,
                                 stderr=stderr)
        except:
            process.kill()
            process.wait()
            raise
        retcode = process.poll()
        if check and retcode:
            raise CalledProcessError(retcode, process.args,
                                     output=stdout, stderr=stderr)
    return CompletedProcess(process.args, retcode, stdout, stderr)


def list2cmdline(seq):
    """
    Translate a sequence of arguments into a command line
    string, using the same rules as the MS C runtime:

    1) Arguments are delimited by white space, which is either a
       space or a tab.

    2) A string surrounded by double quotation marks is
       interpreted as a single argument, regardless of white space
       contained within.  A quoted string can be embedded in an
       argument.

    3) A double quotation mark preceded by a backslash is
       interpreted as a literal double quotation mark.

    4) Backslashes are interpreted literally, unless they
       immediately precede a double quotation mark.

    5) If backslashes immediately precede a double quotation mark,
       every pair of backslashes is interpreted as a literal
       backslash.  If the number of backslashes is odd, the last
       backslash escapes the next double quotation mark as
       described in rule 3.
    """

    # See
    # http://msdn.microsoft.com/en-us/library/17w5ykft.aspx
    # or search http://msdn.microsoft.com for
    # "Parsing C++ Command-Line Arguments"
    result = []
    needquote = False
    for arg in seq:
        bs_buf = []

        # Add a space to separate this argument from the others
        if result:
            result.append(' ')

        needquote = (" " in arg) or ("\t" in arg) or not arg
        if needquote:
            result.append('"')

        for c in arg:
            if c == '\\':
                # Don't know if we need to double yet.
                bs_buf.append(c)
            elif c == '"':
                # Double backslashes.
                result.append('\\' * len(bs_buf)*2)
                bs_buf = []
                result.append('\\"')
            else:
                # Normal char
                if bs_buf:
                    result.extend(bs_buf)
                    bs_buf = []
                result.append(c)

        # Add remaining backslashes, if any.
        if bs_buf:
            result.extend(bs_buf)

        if needquote:
            result.extend(bs_buf)
            result.append('"')

    return ''.join(result)


# Various tools for executing commands and looking at their output and status.
#

def getstatusoutput(cmd):
    """    Return (status, output) of executing cmd in a shell.

    Execute the string 'cmd' in a shell with 'check_output' and
    return a 2-tuple (status, output). Universal newlines mode is used,
    meaning that the result with be decoded to a string.

    A trailing newline is stripped from the output.
    The exit status for the command can be interpreted
    according to the rules for the function 'wait'. Example:

    >>> import subprocess
    >>> subprocess.getstatusoutput('ls /bin/ls')
    (0, '/bin/ls')
    >>> subprocess.getstatusoutput('cat /bin/junk')
    (256, 'cat: /bin/junk: No such file or directory')
    >>> subprocess.getstatusoutput('/bin/junk')
    (256, 'sh: /bin/junk: not found')
    """
    try:
        data = check_output(cmd, shell=True, universal_newlines=True, stderr=STDOUT)
        status = 0
    except CalledProcessError as ex:
        data = ex.output
        status = ex.returncode
    if data[-1:] == '\n':
        data = data[:-1]
    return status, data

def getoutput(cmd):
    """Return output (stdout or stderr) of executing cmd in a shell.

    Like getstatusoutput(), except the exit status is ignored and the return
    value is a string containing the command's output.  Example:

    >>> import subprocess
    >>> subprocess.getoutput('ls /bin/ls')
    '/bin/ls'
    """
    return getstatusoutput(cmd)[1]


_PLATFORM_DEFAULT_CLOSE_FDS = object()


class Popen(object):
    """ Execute a child program in a new process.

    For a complete description of the arguments see the Python documentation.

    Arguments:
      args: A string, or a sequence of program arguments.

      bufsize: supplied as the buffering argument to the open() function when
          creating the stdin/stdout/stderr pipe file objects

      executable: A replacement program to execute.

      stdin, stdout and stderr: These specify the executed programs' standard
          input, standard output and standard error file handles, respectively.

      preexec_fn: (POSIX only) An object to be called in the child process
          just before the child is executed.

      close_fds: Controls closing or inheriting of file descriptors.

      shell: If true, the command will be executed through the shell.

      cwd: Sets the current directory before the child is executed.

      env: Defines the environment variables for the new process.

      universal_newlines: If true, use universal line endings for file
          objects stdin, stdout and stderr.

      startupinfo and creationflags (Windows only)

      restore_signals (POSIX only)

      start_new_session (POSIX only)

      pass_fds (POSIX only)

    Attributes:
        stdin, stdout, stderr, pid, returncode
    """
    _child_created = False  # Set here since __del__ checks it

    def __init__(self, args, bufsize=-1, executable=None,
                 stdin=None, stdout=None, stderr=None,
                 preexec_fn=None, close_fds=_PLATFORM_DEFAULT_CLOSE_FDS,
                 shell=False, cwd=None, env=None, universal_newlines=False,
                 startupinfo=None, creationflags=0,
                 restore_signals=True, start_new_session=False,
                 pass_fds=()):
        """Create new Popen instance."""
        _cleanup()
        # Held while anything is calling waitpid before returncode has been
        # updated to prevent clobbering returncode if wait() or poll() are
        # called from multiple threads at once.  After acquiring the lock,
        # code must re-check self.returncode to see if another thread just
        # finished a waitpid() call.
        self._waitpid_lock = threading.Lock()

        self._input = None
        self._communication_started = False
        if bufsize is None:
            bufsize = -1  # Restore default
        if not isinstance(bufsize, int):
            raise TypeError("bufsize must be an integer")

        if _mswindows:
            if preexec_fn is not None:
                raise ValueError("preexec_fn is not supported on Windows "
                                 "platforms")
            any_stdio_set = (stdin is not None or stdout is not None or
                             stderr is not None)
            if close_fds is _PLATFORM_DEFAULT_CLOSE_FDS:
                if any_stdio_set:
                    close_fds = False
                else:
                    close_fds = True
            elif close_fds and any_stdio_set:
                raise ValueError(
                        "close_fds is not supported on Windows platforms"
                        " if you redirect stdin/stdout/stderr")
        else:
            # POSIX
            if close_fds is _PLATFORM_DEFAULT_CLOSE_FDS:
                close_fds = True
            if pass_fds and not close_fds:
                warnings.warn("pass_fds overriding close_fds.", RuntimeWarning)
                close_fds = True
            if startupinfo is not None:
                raise ValueError("startupinfo is only supported on Windows "
                                 "platforms")
            if creationflags != 0:
                raise ValueError("creationflags is only supported on Windows "
                                 "platforms")

        self.args = args
        self.stdin = None
        self.stdout = None
        self.stderr = None
        self.pid = None
        self.returncode = None
        self.universal_newlines = universal_newlines

        # Input and output objects. The general principle is like
        # this:
        #
        # Parent                   Child
        # ------                   -----
        # p2cwrite   ---stdin--->  p2cread
        # c2pread    <--stdout---  c2pwrite
        # errread    <--stderr---  errwrite
        #
        # On POSIX, the child objects are file descriptors.  On
        # Windows, these are Windows file handles.  The parent objects
        # are file descriptors on both platforms.  The parent objects
        # are -1 when not using PIPEs. The child objects are -1
        # when not redirecting.

        (p2cread, p2cwrite,
         c2pread, c2pwrite,
         errread, errwrite) = self._get_handles(stdin, stdout, stderr)

        # We wrap OS handles *before* launching the child, otherwise a
        # quickly terminating child could make our fds unwrappable
        # (see #8458).

        if _mswindows:
            if p2cwrite != -1:
                p2cwrite = msvcrt.open_osfhandle(p2cwrite.Detach(), 0)
            if c2pread != -1:
                c2pread = msvcrt.open_osfhandle(c2pread.Detach(), 0)
            if errread != -1:
                errread = msvcrt.open_osfhandle(errread.Detach(), 0)

        if p2cwrite != -1:
            self.stdin = io.open(p2cwrite, 'wb', bufsize)
            if universal_newlines:
                self.stdin = io.TextIOWrapper(self.stdin, write_through=True,
                                              line_buffering=(bufsize == 1))
        if c2pread != -1:
            self.stdout = io.open(c2pread, 'rb', bufsize)
            if universal_newlines:
                self.stdout = io.TextIOWrapper(self.stdout)
        if errread != -1:
            self.stderr = io.open(errread, 'rb', bufsize)
            if universal_newlines:
                self.stderr = io.TextIOWrapper(self.stderr)

        self._closed_child_pipe_fds = False
        try:
            self._execute_child(args, executable, preexec_fn, close_fds,
                                pass_fds, cwd, env,
                                startupinfo, creationflags, shell,
                                p2cread, p2cwrite,
                                c2pread, c2pwrite,
                                errread, errwrite,
                                restore_signals, start_new_session)
        except:
            # Cleanup if the child failed starting.
            for f in filter(None, (self.stdin, self.stdout, self.stderr)):
                try:
                    f.close()
                except OSError:
                    pass  # Ignore EBADF or other errors.

            if not self._closed_child_pipe_fds:
                to_close = []
                if stdin == PIPE:
                    to_close.append(p2cread)
                if stdout == PIPE:
                    to_close.append(c2pwrite)
                if stderr == PIPE:
                    to_close.append(errwrite)
                if hasattr(self, '_devnull'):
                    to_close.append(self._devnull)
                for fd in to_close:
                    try:
                        os.close(fd)
                    except OSError:
                        pass

            raise


    def _translate_newlines(self, data, encoding):
        data = data.decode(encoding)
        return data.replace("\r\n", "\n").replace("\r", "\n")

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        if self.stdout:
            self.stdout.close()
        if self.stderr:
            self.stderr.close()
        try:  # Flushing a BufferedWriter may raise an error
            if self.stdin:
                self.stdin.close()
        finally:
            # Wait for the process to terminate, to avoid zombies.
            self.wait()

    def __del__(self, _maxsize=sys.maxsize):
        if not self._child_created:
            # We didn't get to successfully create a child process.
            return
        # In case the child hasn't been waited on, check if it's done.
        self._internal_poll(_deadstate=_maxsize)
        if self.returncode is None and _active is not None:
            # Child is still running, keep us alive until we can wait on it.
            _active.append(self)

    def _get_devnull(self):
        if not hasattr(self, '_devnull'):
            self._devnull = os.open(os.devnull, os.O_RDWR)
        return self._devnull

    def _stdin_write(self, input):
        if input:
            try:
                self.stdin.write(input)
            except BrokenPipeError:
                pass  # communicate() must ignore broken pipe errors.
            except OSError as exc:
                if exc.errno == errno.EINVAL:
                    # bpo-19612, bpo-30418: On Windows, stdin.write() fails
                    # with EINVAL if the child process exited or if the child
                    # process is still running but closed the pipe.
                    pass
                else:
                    raise

        try:
            self.stdin.close()
        except BrokenPipeError:
            pass  # communicate() must ignore broken pipe errors.
        except OSError as exc:
            if exc.errno == errno.EINVAL:
                pass
            else:
                raise

    def communicate(self, input=None, timeout=None):
        """Interact with process: Send data to stdin.  Read data from
        stdout and stderr, until end-of-file is reached.  Wait for
        process to terminate.

        The optional "input" argument should be data to be sent to the
        child process (if self.universal_newlines is True, this should
        be a string; if it is False, "input" should be bytes), or
        None, if no data should be sent to the child.

        communicate() returns a tuple (stdout, stderr).  These will be
        bytes or, if self.universal_newlines was True, a string.
        """

        if self._communication_started and input:
            raise ValueError("Cannot send input after starting communication")

        # Optimization: If we are not worried about timeouts, we haven't
        # started communicating, and we have one or zero pipes, using select()
        # or threads is unnecessary.
        if (timeout is None and not self._communication_started and
            [self.stdin, self.stdout, self.stderr].count(None) >= 2):
            stdout = None
            stderr = None
            if self.stdin:
                self._stdin_write(input)
            elif self.stdout:
                stdout = self.stdout.read()
                self.stdout.close()
            elif self.stderr:
                stderr = self.stderr.read()
                self.stderr.close()
            self.wait()
        else:
            if timeout is not None:
                endtime = _time() + timeout
            else:
                endtime = None

            try:
                stdout, stderr = self._communicate(input, endtime, timeout)
            finally:
                self._communication_started = True

            sts = self.wait(timeout=self._remaining_time(endtime))

        return (stdout, stderr)


    def poll(self):
        """Check if child process has terminated. Set and return returncode
        attribute."""
        return self._internal_poll()


    def _remaining_time(self, endtime):
        """Convenience for _communicate when computing timeouts."""
        if endtime is None:
            return None
        else:
            return endtime - _time()


    def _check_timeout(self, endtime, orig_timeout):
        """Convenience for checking if a timeout has expired."""
        if endtime is None:
            return
        if _time() > endtime:
            raise TimeoutExpired(self.args, orig_timeout)


    if _mswindows:
        #
        # Windows methods
        #
        def _get_handles(self, stdin, stdout, stderr):
            """Construct and return tuple with IO objects:
            p2cread, p2cwrite, c2pread, c2pwrite, errread, errwrite
            """
            if stdin is None and stdout is None and stderr is None:
                return (-1, -1, -1, -1, -1, -1)

            p2cread, p2cwrite = -1, -1
            c2pread, c2pwrite = -1, -1
            errread, errwrite = -1, -1

            if stdin is None:
                p2cread = _winapi.GetStdHandle(_winapi.STD_INPUT_HANDLE)
                if p2cread is None:
                    p2cread, _ = _winapi.CreatePipe(None, 0)
                    p2cread = Handle(p2cread)
                    _winapi.CloseHandle(_)
            elif stdin == PIPE:
                p2cread, p2cwrite = _winapi.CreatePipe(None, 0)
                p2cread, p2cwrite = Handle(p2cread), Handle(p2cwrite)
            elif stdin == DEVNULL:
                p2cread = msvcrt.get_osfhandle(self._get_devnull())
            elif isinstance(stdin, int):
                p2cread = msvcrt.get_osfhandle(stdin)
            else:
                # Assuming file-like object
                p2cread = msvcrt.get_osfhandle(stdin.fileno())
            p2cread = self._make_inheritable(p2cread)

            if stdout is None:
                c2pwrite = _winapi.GetStdHandle(_winapi.STD_OUTPUT_HANDLE)
                if c2pwrite is None:
                    _, c2pwrite = _winapi.CreatePipe(None, 0)
                    c2pwrite = Handle(c2pwrite)
                    _winapi.CloseHandle(_)
            elif stdout == PIPE:
                c2pread, c2pwrite = _winapi.CreatePipe(None, 0)
                c2pread, c2pwrite = Handle(c2pread), Handle(c2pwrite)
            elif stdout == DEVNULL:
                c2pwrite = msvcrt.get_osfhandle(self._get_devnull())
            elif isinstance(stdout, int):
                c2pwrite = msvcrt.get_osfhandle(stdout)
            else:
                # Assuming file-like object
                c2pwrite = msvcrt.get_osfhandle(stdout.fileno())
            c2pwrite = self._make_inheritable(c2pwrite)

            if stderr is None:
                errwrite = _winapi.GetStdHandle(_winapi.STD_ERROR_HANDLE)
                if errwrite is None:
                    _, errwrite = _winapi.CreatePipe(None, 0)
                    errwrite = Handle(errwrite)
                    _winapi.CloseHandle(_)
            elif stderr == PIPE:
                errread, errwrite = _winapi.CreatePipe(None, 0)
                errread, errwrite = Handle(errread), Handle(errwrite)
            elif stderr == STDOUT:
                errwrite = c2pwrite
            elif stderr == DEVNULL:
                errwrite = msvcrt.get_osfhandle(self._get_devnull())
            elif isinstance(stderr, int):
                errwrite = msvcrt.get_osfhandle(stderr)
            else:
                # Assuming file-like object
                errwrite = msvcrt.get_osfhandle(stderr.fileno())
            errwrite = self._make_inheritable(errwrite)

            return (p2cread, p2cwrite,
                    c2pread, c2pwrite,
                    errread, errwrite)


        def _make_inheritable(self, handle):
            """Return a duplicate of handle, which is inheritable"""
            h = _winapi.DuplicateHandle(
                _winapi.GetCurrentProcess(), handle,
                _winapi.GetCurrentProcess(), 0, 1,
                _winapi.DUPLICATE_SAME_ACCESS)
            return Handle(h)


        def _execute_child(self, args, executable, preexec_fn, close_fds,
                           pass_fds, cwd, env,
                           startupinfo, creationflags, shell,
                           p2cread, p2cwrite,
                           c2pread, c2pwrite,
                           errread, errwrite,
                           unused_restore_signals, unused_start_new_session):
            """Execute program (MS Windows version)"""

            assert not pass_fds, "pass_fds not supported on Windows."

            if not isinstance(args, str):
                args = list2cmdline(args)

            # Process startup details
            if startupinfo is None:
                startupinfo = STARTUPINFO()
            if -1 not in (p2cread, c2pwrite, errwrite):
                startupinfo.dwFlags |= _winapi.STARTF_USESTDHANDLES
                startupinfo.hStdInput = p2cread
                startupinfo.hStdOutput = c2pwrite
                startupinfo.hStdError = errwrite

            if shell:
                startupinfo.dwFlags |= _winapi.STARTF_USESHOWWINDOW
                startupinfo.wShowWindow = _winapi.SW_HIDE
                comspec = os.environ.get("COMSPEC", "cmd.exe")
                args = '{} /c "{}"'.format (comspec, args)

            # Start the process
            try:
                hp, ht, pid, tid = _winapi.CreateProcess(executable, args,
                                         # no special security
                                         None, None,
                                         int(not close_fds),
                                         creationflags,
                                         env,
                                         cwd,
                                         startupinfo)
            finally:
                # Child is launched. Close the parent's copy of those pipe
                # handles that only the child should have open.  You need
                # to make sure that no handles to the write end of the
                # output pipe are maintained in this process or else the
                # pipe will not close when the child process exits and the
                # ReadFile will hang.
                if p2cread != -1:
                    p2cread.Close()
                if c2pwrite != -1:
                    c2pwrite.Close()
                if errwrite != -1:
                    errwrite.Close()
                if hasattr(self, '_devnull'):
                    os.close(self._devnull)

            # Retain the process handle, but close the thread handle
            self._child_created = True
            self._handle = Handle(hp)
            self.pid = pid
            _winapi.CloseHandle(ht)

        def _internal_poll(self, _deadstate=None,
                _WaitForSingleObject=_winapi.WaitForSingleObject,
                _WAIT_OBJECT_0=_winapi.WAIT_OBJECT_0,
                _GetExitCodeProcess=_winapi.GetExitCodeProcess):
            """Check if child process has terminated.  Returns returncode
            attribute.

            This method is called by __del__, so it can only refer to objects
            in its local scope.

            """
            if self.returncode is None:
                if _WaitForSingleObject(self._handle, 0) == _WAIT_OBJECT_0:
                    self.returncode = _GetExitCodeProcess(self._handle)
            return self.returncode


        def wait(self, timeout=None, endtime=None):
            """Wait for child process to terminate.  Returns returncode
            attribute."""
            if endtime is not None:
                timeout = self._remaining_time(endtime)
            if timeout is None:
                timeout_millis = _winapi.INFINITE
            else:
                timeout_millis = int(timeout * 1000)
            if self.returncode is None:
                result = _winapi.WaitForSingleObject(self._handle,
                                                    timeout_millis)
                if result == _winapi.WAIT_TIMEOUT:
                    raise TimeoutExpired(self.args, timeout)
                self.returncode = _winapi.GetExitCodeProcess(self._handle)
            return self.returncode


        def _readerthread(self, fh, buffer):
            buffer.append(fh.read())
            fh.close()


        def _communicate(self, input, endtime, orig_timeout):
            # Start reader threads feeding into a list hanging off of this
            # object, unless they've already been started.
            if self.stdout and not hasattr(self, "_stdout_buff"):
                self._stdout_buff = []
                self.stdout_thread = \
                        threading.Thread(target=self._readerthread,
                                         args=(self.stdout, self._stdout_buff))
                self.stdout_thread.daemon = True
                self.stdout_thread.start()
            if self.stderr and not hasattr(self, "_stderr_buff"):
                self._stderr_buff = []
                self.stderr_thread = \
                        threading.Thread(target=self._readerthread,
                                         args=(self.stderr, self._stderr_buff))
                self.stderr_thread.daemon = True
                self.stderr_thread.start()

            if self.stdin:
                self._stdin_write(input)

            # Wait for the reader threads, or time out.  If we time out, the
            # threads remain reading and the fds left open in case the user
            # calls communicate again.
            if self.stdout is not None:
                self.stdout_thread.join(self._remaining_time(endtime))
                if self.stdout_thread.is_alive():
                    raise TimeoutExpired(self.args, orig_timeout)
            if self.stderr is not None:
                self.stderr_thread.join(self._remaining_time(endtime))
                if self.stderr_thread.is_alive():
                    raise TimeoutExpired(self.args, orig_timeout)

            # Collect the output from and close both pipes, now that we know
            # both have been read successfully.
            stdout = None
            stderr = None
            if self.stdout:
                stdout = self._stdout_buff
                self.stdout.close()
            if self.stderr:
                stderr = self._stderr_buff
                self.stderr.close()

            # All data exchanged.  Translate lists into strings.
            if stdout is not None:
                stdout = stdout[0]
            if stderr is not None:
                stderr = stderr[0]

            return (stdout, stderr)

        def send_signal(self, sig):
            """Send a signal to the process."""
            # Don't signal a process that we know has already died.
            if self.returncode is not None:
                return
            if sig == signal.SIGTERM:
                self.terminate()
            elif sig == signal.CTRL_C_EVENT:
                os.kill(self.pid, signal.CTRL_C_EVENT)
            elif sig == signal.CTRL_BREAK_EVENT:
                os.kill(self.pid, signal.CTRL_BREAK_EVENT)
            else:
                raise ValueError("Unsupported signal: {}".format(sig))

        def terminate(self):
            """Terminates the process."""
            # Don't terminate a process that we know has already died.
            if self.returncode is not None:
                return
            try:
                _winapi.TerminateProcess(self._handle, 1)
            except PermissionError:
                # ERROR_ACCESS_DENIED (winerror 5) is received when the
                # process already died.
                rc = _winapi.GetExitCodeProcess(self._handle)
                if rc == _winapi.STILL_ACTIVE:
                    raise
                self.returncode = rc

        kill = terminate

    else:
        #
        # POSIX methods
        #
        def _get_handles(self, stdin, stdout, stderr):
            """Construct and return tuple with IO objects:
            p2cread, p2cwrite, c2pread, c2pwrite, errread, errwrite
            """
            p2cread, p2cwrite = -1, -1
            c2pread, c2pwrite = -1, -1
            errread, errwrite = -1, -1

            if stdin is None:
                pass
            elif stdin == PIPE:
                p2cread, p2cwrite = os.pipe()
            elif stdin == DEVNULL:
                p2cread = self._get_devnull()
            elif isinstance(stdin, int):
                p2cread = stdin
            else:
                # Assuming file-like object
                p2cread = stdin.fileno()

            if stdout is None:
                pass
            elif stdout == PIPE:
                c2pread, c2pwrite = os.pipe()
            elif stdout == DEVNULL:
                c2pwrite = self._get_devnull()
            elif isinstance(stdout, int):
                c2pwrite = stdout
            else:
                # Assuming file-like object
                c2pwrite = stdout.fileno()

            if stderr is None:
                pass
            elif stderr == PIPE:
                errread, errwrite = os.pipe()
            elif stderr == STDOUT:
                if c2pwrite != -1:
                    errwrite = c2pwrite
                else: # child's stdout is not set, use parent's stdout
                    errwrite = sys.__stdout__.fileno()
            elif stderr == DEVNULL:
                errwrite = self._get_devnull()
            elif isinstance(stderr, int):
                errwrite = stderr
            else:
                # Assuming file-like object
                errwrite = stderr.fileno()

            return (p2cread, p2cwrite,
                    c2pread, c2pwrite,
                    errread, errwrite)


        def _execute_child(self, args, executable, preexec_fn, close_fds,
                           pass_fds, cwd, env,
                           startupinfo, creationflags, shell,
                           p2cread, p2cwrite,
                           c2pread, c2pwrite,
                           errread, errwrite,
                           restore_signals, start_new_session):
            """Execute program (POSIX version)"""

            if isinstance(args, (str, bytes)):
                args = [args]
            else:
                args = list(args)

            if shell:
                args = ["/bin/sh", "-c"] + args
                if executable:
                    args[0] = executable

            if executable is None:
                executable = args[0]
            orig_executable = executable

            # For transferring possible exec failure from child to parent.
            # Data format: "exception name:hex errno:description"
            # Pickle is not used; it is complex and involves memory allocation.
            errpipe_read, errpipe_write = os.pipe()
            # errpipe_write must not be in the standard io 0, 1, or 2 fd range.
            low_fds_to_close = []
            while errpipe_write < 3:
                low_fds_to_close.append(errpipe_write)
                errpipe_write = os.dup(errpipe_write)
            for low_fd in low_fds_to_close:
                os.close(low_fd)
            try:
                try:
                    # We must avoid complex work that could involve
                    # malloc or free in the child process to avoid
                    # potential deadlocks, thus we do all this here.
                    # and pass it to fork_exec()

                    if env is not None:
                        env_list = []
                        for k, v in env.items():
                            k = os.fsencode(k)
                            if b'=' in k:
                                raise ValueError("illegal environment variable name")
                            env_list.append(k + b'=' + os.fsencode(v))
                    else:
                        env_list = None  # Use execv instead of execve.
                    executable = os.fsencode(executable)
                    if os.path.dirname(executable):
                        executable_list = (executable,)
                    else:
                        # This matches the behavior of os._execvpe().
                        executable_list = tuple(
                            os.path.join(os.fsencode(dir), executable)
                            for dir in os.get_exec_path(env))
                    fds_to_keep = set(pass_fds)
                    fds_to_keep.add(errpipe_write)
                    self.pid = _posixsubprocess.fork_exec(
                            args, executable_list,
                            close_fds, tuple(sorted(map(int, fds_to_keep))),
                            cwd, env_list,
                            p2cread, p2cwrite, c2pread, c2pwrite,
                            errread, errwrite,
                            errpipe_read, errpipe_write,
                            restore_signals, start_new_session, preexec_fn)
                    self._child_created = True
                finally:
                    # be sure the FD is closed no matter what
                    os.close(errpipe_write)

                # self._devnull is not always defined.
                devnull_fd = getattr(self, '_devnull', None)
                if p2cread != -1 and p2cwrite != -1 and p2cread != devnull_fd:
                    os.close(p2cread)
                if c2pwrite != -1 and c2pread != -1 and c2pwrite != devnull_fd:
                    os.close(c2pwrite)
                if errwrite != -1 and errread != -1 and errwrite != devnull_fd:
                    os.close(errwrite)
                if devnull_fd is not None:
                    os.close(devnull_fd)
                # Prevent a double close of these fds from __init__ on error.
                self._closed_child_pipe_fds = True

                # Wait for exec to fail or succeed; possibly raising an
                # exception (limited in size)
                errpipe_data = bytearray()
                while True:
                    part = os.read(errpipe_read, 50000)
                    errpipe_data += part
                    if not part or len(errpipe_data) > 50000:
                        break
            finally:
                # be sure the FD is closed no matter what
                os.close(errpipe_read)

            if errpipe_data:
                try:
                    os.waitpid(self.pid, 0)
                except ChildProcessError:
                    pass
                try:
                    exception_name, hex_errno, err_msg = (
                            errpipe_data.split(b':', 2))
                except ValueError:
                    exception_name = b'SubprocessError'
                    hex_errno = b'0'
                    err_msg = (b'Bad exception data from child: ' +
                               repr(errpipe_data))
                child_exception_type = getattr(
                        builtins, exception_name.decode('ascii'),
                        SubprocessError)
                err_msg = err_msg.decode(errors="surrogatepass")
                if issubclass(child_exception_type, OSError) and hex_errno:
                    errno_num = int(hex_errno, 16)
                    child_exec_never_called = (err_msg == "noexec")
                    if child_exec_never_called:
                        err_msg = ""
                    if errno_num != 0:
                        err_msg = os.strerror(errno_num)
                        if errno_num == errno.ENOENT:
                            if child_exec_never_called:
                                # The error must be from chdir(cwd).
                                err_msg += ': ' + repr(cwd)
                            else:
                                err_msg += ': ' + repr(orig_executable)
                    raise child_exception_type(errno_num, err_msg)
                raise child_exception_type(err_msg)


        def _handle_exitstatus(self, sts, _WIFSIGNALED=os.WIFSIGNALED,
                _WTERMSIG=os.WTERMSIG, _WIFEXITED=os.WIFEXITED,
                _WEXITSTATUS=os.WEXITSTATUS, _WIFSTOPPED=os.WIFSTOPPED,
                _WSTOPSIG=os.WSTOPSIG):
            """All callers to this function MUST hold self._waitpid_lock."""
            # This method is called (indirectly) by __del__, so it cannot
            # refer to anything outside of its local scope.
            if _WIFSIGNALED(sts):
                self.returncode = -_WTERMSIG(sts)
            elif _WIFEXITED(sts):
                self.returncode = _WEXITSTATUS(sts)
            elif _WIFSTOPPED(sts):
                self.returncode = -_WSTOPSIG(sts)
            else:
                # Should never happen
                raise SubprocessError("Unknown child exit status!")


        def _internal_poll(self, _deadstate=None, _waitpid=os.waitpid,
                _WNOHANG=os.WNOHANG, _ECHILD=errno.ECHILD):
            """Check if child process has terminated.  Returns returncode
            attribute.

            This method is called by __del__, so it cannot reference anything
            outside of the local scope (nor can any methods it calls).

            """
            if self.returncode is None:
                if not self._waitpid_lock.acquire(False):
                    # Something else is busy calling waitpid.  Don't allow two
                    # at once.  We know nothing yet.
                    return None
                try:
                    if self.returncode is not None:
                        return self.returncode  # Another thread waited.
                    pid, sts = _waitpid(self.pid, _WNOHANG)
                    if pid == self.pid:
                        self._handle_exitstatus(sts)
                except OSError as e:
                    if _deadstate is not None:
                        self.returncode = _deadstate
                    elif e.errno == _ECHILD:
                        # This happens if SIGCLD is set to be ignored or
                        # waiting for child processes has otherwise been
                        # disabled for our process.  This child is dead, we
                        # can't get the status.
                        # http://bugs.python.org/issue15756
                        self.returncode = 0
                finally:
                    self._waitpid_lock.release()
            return self.returncode


        def _try_wait(self, wait_flags):
            """All callers to this function MUST hold self._waitpid_lock."""
            try:
                (pid, sts) = os.waitpid(self.pid, wait_flags)
            except ChildProcessError:
                # This happens if SIGCLD is set to be ignored or waiting
                # for child processes has otherwise been disabled for our
                # process.  This child is dead, we can't get the status.
                pid = self.pid
                sts = 0
            return (pid, sts)


        def wait(self, timeout=None, endtime=None):
            """Wait for child process to terminate.  Returns returncode
            attribute."""
            if self.returncode is not None:
                return self.returncode

            # endtime is preferred to timeout.  timeout is only used for
            # printing.
            if endtime is not None or timeout is not None:
                if endtime is None:
                    endtime = _time() + timeout
                elif timeout is None:
                    timeout = self._remaining_time(endtime)

            if endtime is not None:
                # Enter a busy loop if we have a timeout.  This busy loop was
                # cribbed from Lib/threading.py in Thread.wait() at r71065.
                delay = 0.0005 # 500 us -> initial delay of 1 ms
                while True:
                    if self._waitpid_lock.acquire(False):
                        try:
                            if self.returncode is not None:
                                break  # Another thread waited.
                            (pid, sts) = self._try_wait(os.WNOHANG)
                            assert pid == self.pid or pid == 0
                            if pid == self.pid:
                                self._handle_exitstatus(sts)
                                break
                        finally:
                            self._waitpid_lock.release()
                    remaining = self._remaining_time(endtime)
                    if remaining <= 0:
                        raise TimeoutExpired(self.args, timeout)
                    delay = min(delay * 2, remaining, .05)
                    time.sleep(delay)
            else:
                while self.returncode is None:
                    with self._waitpid_lock:
                        if self.returncode is not None:
                            break  # Another thread waited.
                        (pid, sts) = self._try_wait(0)
                        # Check the pid and loop as waitpid has been known to
                        # return 0 even without WNOHANG in odd situations.
                        # http://bugs.python.org/issue14396.
                        if pid == self.pid:
                            self._handle_exitstatus(sts)
            return self.returncode


        def _communicate(self, input, endtime, orig_timeout):
            if self.stdin and not self._communication_started:
                # Flush stdio buffer.  This might block, if the user has
                # been writing to .stdin in an uncontrolled fashion.
                try:
                    self.stdin.flush()
                except BrokenPipeError:
                    pass  # communicate() must ignore BrokenPipeError.
                if not input:
                    try:
                        self.stdin.close()
                    except BrokenPipeError:
                        pass  # communicate() must ignore BrokenPipeError.

            stdout = None
            stderr = None

            # Only create this mapping if we haven't already.
            if not self._communication_started:
                self._fileobj2output = {}
                if self.stdout:
                    self._fileobj2output[self.stdout] = []
                if self.stderr:
                    self._fileobj2output[self.stderr] = []

            if self.stdout:
                stdout = self._fileobj2output[self.stdout]
            if self.stderr:
                stderr = self._fileobj2output[self.stderr]

            self._save_input(input)

            if self._input:
                input_view = memoryview(self._input)

            with _PopenSelector() as selector:
                if self.stdin and input:
                    selector.register(self.stdin, selectors.EVENT_WRITE)
                if self.stdout:
                    selector.register(self.stdout, selectors.EVENT_READ)
                if self.stderr:
                    selector.register(self.stderr, selectors.EVENT_READ)

                while selector.get_map():
                    timeout = self._remaining_time(endtime)
                    if timeout is not None and timeout < 0:
                        raise TimeoutExpired(self.args, orig_timeout)

                    ready = selector.select(timeout)
                    self._check_timeout(endtime, orig_timeout)

                    # XXX Rewrite these to use non-blocking I/O on the file
                    # objects; they are no longer using C stdio!

                    for key, events in ready:
                        if key.fileobj is self.stdin:
                            chunk = input_view[self._input_offset :
                                               self._input_offset + _PIPE_BUF]
                            try:
                                self._input_offset += os.write(key.fd, chunk)
                            except BrokenPipeError:
                                selector.unregister(key.fileobj)
                                key.fileobj.close()
                            else:
                                if self._input_offset >= len(self._input):
                                    selector.unregister(key.fileobj)
                                    key.fileobj.close()
                        elif key.fileobj in (self.stdout, self.stderr):
                            data = os.read(key.fd, 32768)
                            if not data:
                                selector.unregister(key.fileobj)
                                key.fileobj.close()
                            self._fileobj2output[key.fileobj].append(data)

            self.wait(timeout=self._remaining_time(endtime))

            # All data exchanged.  Translate lists into strings.
            if stdout is not None:
                stdout = b''.join(stdout)
            if stderr is not None:
                stderr = b''.join(stderr)

            # Translate newlines, if requested.
            # This also turns bytes into strings.
            if self.universal_newlines:
                if stdout is not None:
                    stdout = self._translate_newlines(stdout,
                                                      self.stdout.encoding)
                if stderr is not None:
                    stderr = self._translate_newlines(stderr,
                                                      self.stderr.encoding)

            return (stdout, stderr)


        def _save_input(self, input):
            # This method is called from the _communicate_with_*() methods
            # so that if we time out while communicating, we can continue
            # sending input if we retry.
            if self.stdin and self._input is None:
                self._input_offset = 0
                self._input = input
                if self.universal_newlines and input is not None:
                    self._input = self._input.encode(self.stdin.encoding)


        def send_signal(self, sig):
            """Send a signal to the process."""
            # Skip signalling a process that we know has already died.
            if self.returncode is None:
                os.kill(self.pid, sig)

        def terminate(self):
            """Terminate the process with SIGTERM
            """
            self.send_signal(signal.SIGTERM)

        def kill(self):
            """Kill the process with SIGKILL
            """
            self.send_signal(signal.SIGKILL)
