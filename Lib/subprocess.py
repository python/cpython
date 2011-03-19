# subprocess - Subprocesses with accessible I/O streams
#
# For more information about this module, see PEP 324.
#
# Copyright (c) 2003-2005 by Peter Astrand <astrand@lysator.liu.se>
#
# Licensed to PSF under a Contributor Agreement.
# See http://www.python.org/2.4/license for licensing details.

r"""subprocess - Subprocesses with accessible I/O streams

This module allows you to spawn processes, connect to their
input/output/error pipes, and obtain their return codes.  This module
intends to replace several other, older modules and functions, like:

os.system
os.spawn*

Information about how the subprocess module can be used to replace these
modules and functions can be found below.



Using the subprocess module
===========================
This module defines one class called Popen:

class Popen(args, bufsize=0, executable=None,
            stdin=None, stdout=None, stderr=None,
            preexec_fn=None, close_fds=True, shell=False,
            cwd=None, env=None, universal_newlines=False,
            startupinfo=None, creationflags=0,
            restore_signals=True, start_new_session=False, pass_fds=()):


Arguments are:

args should be a string, or a sequence of program arguments.  The
program to execute is normally the first item in the args sequence or
string, but can be explicitly set by using the executable argument.

On POSIX, with shell=False (default): In this case, the Popen class
uses os.execvp() to execute the child program.  args should normally
be a sequence.  A string will be treated as a sequence with the string
as the only item (the program to execute).

On POSIX, with shell=True: If args is a string, it specifies the
command string to execute through the shell.  If args is a sequence,
the first item specifies the command string, and any additional items
will be treated as additional shell arguments.

On Windows: the Popen class uses CreateProcess() to execute the child
program, which operates on strings.  If args is a sequence, it will be
converted to a string using the list2cmdline method.  Please note that
not all MS Windows applications interpret the command line the same
way: The list2cmdline is designed for applications using the same
rules as the MS C runtime.

bufsize, if given, has the same meaning as the corresponding argument
to the built-in open() function: 0 means unbuffered, 1 means line
buffered, any other positive value means use a buffer of
(approximately) that size.  A negative bufsize means to use the system
default, which usually means fully buffered.  The default value for
bufsize is 0 (unbuffered).

stdin, stdout and stderr specify the executed programs' standard
input, standard output and standard error file handles, respectively.
Valid values are PIPE, an existing file descriptor (a positive
integer), an existing file object, and None.  PIPE indicates that a
new pipe to the child should be created.  With None, no redirection
will occur; the child's file handles will be inherited from the
parent.  Additionally, stderr can be STDOUT, which indicates that the
stderr data from the applications should be captured into the same
file handle as for stdout.

On POSIX, if preexec_fn is set to a callable object, this object will be
called in the child process just before the child is executed.  The use
of preexec_fn is not thread safe, using it in the presence of threads
could lead to a deadlock in the child process before the new executable
is executed.

If close_fds is true, all file descriptors except 0, 1 and 2 will be
closed before the child process is executed.  The default for close_fds
varies by platform:  Always true on POSIX.  True when stdin/stdout/stderr
are None on Windows, false otherwise.

pass_fds is an optional sequence of file descriptors to keep open between the
parent and child.  Providing any pass_fds implicitly sets close_fds to true.

if shell is true, the specified command will be executed through the
shell.

If cwd is not None, the current directory will be changed to cwd
before the child is executed.

On POSIX, if restore_signals is True all signals that Python sets to
SIG_IGN are restored to SIG_DFL in the child process before the exec.
Currently this includes the SIGPIPE, SIGXFZ and SIGXFSZ signals.  This
parameter does nothing on Windows.

On POSIX, if start_new_session is True, the setsid() system call will be made
in the child process prior to executing the command.

If env is not None, it defines the environment variables for the new
process.

If universal_newlines is true, the file objects stdout and stderr are
opened as a text files, but lines may be terminated by any of '\n',
the Unix end-of-line convention, '\r', the old Macintosh convention or
'\r\n', the Windows convention.  All of these external representations
are seen as '\n' by the Python program.  Note: This feature is only
available if Python is built with universal newline support (the
default).  Also, the newlines attribute of the file objects stdout,
stdin and stderr are not updated by the communicate() method.

The startupinfo and creationflags, if given, will be passed to the
underlying CreateProcess() function.  They can specify things such as
appearance of the main window and priority for the new process.
(Windows only)


This module also defines some shortcut functions:

call(*popenargs, **kwargs):
    Run command with arguments.  Wait for command to complete, then
    return the returncode attribute.

    The arguments are the same as for the Popen constructor.  Example:

    >>> retcode = subprocess.call(["ls", "-l"])

check_call(*popenargs, **kwargs):
    Run command with arguments.  Wait for command to complete.  If the
    exit code was zero then return, otherwise raise
    CalledProcessError.  The CalledProcessError object will have the
    return code in the returncode attribute.

    The arguments are the same as for the Popen constructor.  Example:

    >>> subprocess.check_call(["ls", "-l"])
    0

getstatusoutput(cmd):
    Return (status, output) of executing cmd in a shell.

    Execute the string 'cmd' in a shell with os.popen() and return a 2-tuple
    (status, output).  cmd is actually run as '{ cmd ; } 2>&1', so that the
    returned output will contain output or error messages. A trailing newline
    is stripped from the output. The exit status for the command can be
    interpreted according to the rules for the C function wait().  Example:

    >>> subprocess.getstatusoutput('ls /bin/ls')
    (0, '/bin/ls')
    >>> subprocess.getstatusoutput('cat /bin/junk')
    (256, 'cat: /bin/junk: No such file or directory')
    >>> subprocess.getstatusoutput('/bin/junk')
    (256, 'sh: /bin/junk: not found')

getoutput(cmd):
    Return output (stdout or stderr) of executing cmd in a shell.

    Like getstatusoutput(), except the exit status is ignored and the return
    value is a string containing the command's output.  Example:

    >>> subprocess.getoutput('ls /bin/ls')
    '/bin/ls'

check_output(*popenargs, **kwargs):
    Run command with arguments and return its output as a byte string.

    If the exit code was non-zero it raises a CalledProcessError.  The
    CalledProcessError object will have the return code in the returncode
    attribute and output in the output attribute.

    The arguments are the same as for the Popen constructor.  Example:

    >>> output = subprocess.check_output(["ls", "-l", "/dev/null"])


Exceptions
----------
Exceptions raised in the child process, before the new program has
started to execute, will be re-raised in the parent.  Additionally,
the exception object will have one extra attribute called
'child_traceback', which is a string containing traceback information
from the childs point of view.

The most common exception raised is OSError.  This occurs, for
example, when trying to execute a non-existent file.  Applications
should prepare for OSErrors.

A ValueError will be raised if Popen is called with invalid arguments.

Exceptions defined within this module inherit from SubprocessError.
check_call() and check_output() will raise CalledProcessError if the
called process returns a non-zero return code.  TimeoutExpired
be raised if a timeout was specified and expired.


Security
--------
Unlike some other popen functions, this implementation will never call
/bin/sh implicitly.  This means that all characters, including shell
metacharacters, can safely be passed to child processes.


Popen objects
=============
Instances of the Popen class have the following methods:

poll()
    Check if child process has terminated.  Returns returncode
    attribute.

wait()
    Wait for child process to terminate.  Returns returncode attribute.

communicate(input=None)
    Interact with process: Send data to stdin.  Read data from stdout
    and stderr, until end-of-file is reached.  Wait for process to
    terminate.  The optional input argument should be a string to be
    sent to the child process, or None, if no data should be sent to
    the child.

    communicate() returns a tuple (stdout, stderr).

    Note: The data read is buffered in memory, so do not use this
    method if the data size is large or unlimited.

The following attributes are also available:

stdin
    If the stdin argument is PIPE, this attribute is a file object
    that provides input to the child process.  Otherwise, it is None.

stdout
    If the stdout argument is PIPE, this attribute is a file object
    that provides output from the child process.  Otherwise, it is
    None.

stderr
    If the stderr argument is PIPE, this attribute is file object that
    provides error output from the child process.  Otherwise, it is
    None.

pid
    The process ID of the child process.

returncode
    The child return code.  A None value indicates that the process
    hasn't terminated yet.  A negative value -N indicates that the
    child was terminated by signal N (POSIX only).


Replacing older functions with the subprocess module
====================================================
In this section, "a ==> b" means that b can be used as a replacement
for a.

Note: All functions in this section fail (more or less) silently if
the executed program cannot be found; this module raises an OSError
exception.

In the following examples, we assume that the subprocess module is
imported with "from subprocess import *".


Replacing /bin/sh shell backquote
---------------------------------
output=`mycmd myarg`
==>
output = Popen(["mycmd", "myarg"], stdout=PIPE).communicate()[0]


Replacing shell pipe line
-------------------------
output=`dmesg | grep hda`
==>
p1 = Popen(["dmesg"], stdout=PIPE)
p2 = Popen(["grep", "hda"], stdin=p1.stdout, stdout=PIPE)
output = p2.communicate()[0]


Replacing os.system()
---------------------
sts = os.system("mycmd" + " myarg")
==>
p = Popen("mycmd" + " myarg", shell=True)
pid, sts = os.waitpid(p.pid, 0)

Note:

* Calling the program through the shell is usually not required.

* It's easier to look at the returncode attribute than the
  exitstatus.

A more real-world example would look like this:

try:
    retcode = call("mycmd" + " myarg", shell=True)
    if retcode < 0:
        print("Child was terminated by signal", -retcode, file=sys.stderr)
    else:
        print("Child returned", retcode, file=sys.stderr)
except OSError as e:
    print("Execution failed:", e, file=sys.stderr)


Replacing os.spawn*
-------------------
P_NOWAIT example:

pid = os.spawnlp(os.P_NOWAIT, "/bin/mycmd", "mycmd", "myarg")
==>
pid = Popen(["/bin/mycmd", "myarg"]).pid


P_WAIT example:

retcode = os.spawnlp(os.P_WAIT, "/bin/mycmd", "mycmd", "myarg")
==>
retcode = call(["/bin/mycmd", "myarg"])


Vector example:

os.spawnvp(os.P_NOWAIT, path, args)
==>
Popen([path] + args[1:])


Environment example:

os.spawnlpe(os.P_NOWAIT, "/bin/mycmd", "mycmd", "myarg", env)
==>
Popen(["/bin/mycmd", "myarg"], env={"PATH": "/usr/bin"})
"""

import sys
mswindows = (sys.platform == "win32")

import io
import os
import time
import traceback
import gc
import signal
import builtins
import warnings

# Exception classes used by this module.
class SubprocessError(Exception): pass


class CalledProcessError(SubprocessError):
    """This exception is raised when a process run by check_call() or
    check_output() returns a non-zero exit status.
    The exit status will be stored in the returncode attribute;
    check_output() will also store the output in the output attribute.
    """
    def __init__(self, returncode, cmd, output=None):
        self.returncode = returncode
        self.cmd = cmd
        self.output = output
    def __str__(self):
        return "Command '%s' returned non-zero exit status %d" % (self.cmd, self.returncode)


class TimeoutExpired(SubprocessError):
    """This exception is raised when the timeout expires while waiting for a
    child process.
    """
    def __init__(self, cmd, timeout, output=None):
        self.cmd = cmd
        self.timeout = timeout
        self.output = output

    def __str__(self):
        return ("Command '%s' timed out after %s seconds" %
                (self.cmd, self.timeout))


if mswindows:
    import threading
    import msvcrt
    import _subprocess
    class STARTUPINFO:
        dwFlags = 0
        hStdInput = None
        hStdOutput = None
        hStdError = None
        wShowWindow = 0
    class pywintypes:
        error = IOError
else:
    import select
    _has_poll = hasattr(select, 'poll')
    import errno
    import fcntl
    import pickle

    try:
        import _posixsubprocess
    except ImportError:
        _posixsubprocess = None
        warnings.warn("The _posixsubprocess module is not being used. "
                      "Child process reliability may suffer if your "
                      "program uses threads.", RuntimeWarning)

    # When select or poll has indicated that the file is writable,
    # we can write up to _PIPE_BUF bytes without risk of blocking.
    # POSIX defines PIPE_BUF as >= 512.
    _PIPE_BUF = getattr(select, 'PIPE_BUF', 512)

    _FD_CLOEXEC = getattr(fcntl, 'FD_CLOEXEC', 1)

    def _set_cloexec(fd, cloexec):
        old = fcntl.fcntl(fd, fcntl.F_GETFD)
        if cloexec:
            fcntl.fcntl(fd, fcntl.F_SETFD, old | _FD_CLOEXEC)
        else:
            fcntl.fcntl(fd, fcntl.F_SETFD, old & ~_FD_CLOEXEC)

    if _posixsubprocess:
        _create_pipe = _posixsubprocess.cloexec_pipe
    else:
        def _create_pipe():
            fds = os.pipe()
            _set_cloexec(fds[0], True)
            _set_cloexec(fds[1], True)
            return fds

__all__ = ["Popen", "PIPE", "STDOUT", "call", "check_call", "getstatusoutput",
           "getoutput", "check_output", "CalledProcessError", "DEVNULL"]

if mswindows:
    from _subprocess import CREATE_NEW_CONSOLE, CREATE_NEW_PROCESS_GROUP
    __all__.extend(["CREATE_NEW_CONSOLE", "CREATE_NEW_PROCESS_GROUP"])
try:
    MAXFD = os.sysconf("SC_OPEN_MAX")
except:
    MAXFD = 256

_active = []

def _cleanup():
    for inst in _active[:]:
        res = inst._internal_poll(_deadstate=sys.maxsize)
        if res is not None and res >= 0:
            try:
                _active.remove(inst)
            except ValueError:
                # This can happen if two threads create a new Popen instance.
                # It's harmless that it was already removed, so ignore.
                pass

PIPE = -1
STDOUT = -2
DEVNULL = -3


def _eintr_retry_call(func, *args):
    while True:
        try:
            return func(*args)
        except OSError as e:
            if e.errno == errno.EINTR:
                continue
            raise


def call(*popenargs, timeout=None, **kwargs):
    """Run command with arguments.  Wait for command to complete or
    timeout, then return the returncode attribute.

    The arguments are the same as for the Popen constructor.  Example:

    retcode = call(["ls", "-l"])
    """
    p = Popen(*popenargs, **kwargs)
    try:
        return p.wait(timeout=timeout)
    except TimeoutExpired:
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
    r"""Run command with arguments and return its output as a byte string.

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
    """
    if 'stdout' in kwargs:
        raise ValueError('stdout argument not allowed, it will be overridden.')
    process = Popen(*popenargs, stdout=PIPE, **kwargs)
    try:
        output, unused_err = process.communicate(timeout=timeout)
    except TimeoutExpired:
        process.kill()
        output, unused_err = process.communicate()
        raise TimeoutExpired(process.args, timeout, output=output)
    retcode = process.poll()
    if retcode:
        raise CalledProcessError(retcode, process.args, output=output)
    return output


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
# NB This only works (and is only relevant) for POSIX.

def getstatusoutput(cmd):
    """Return (status, output) of executing cmd in a shell.

    Execute the string 'cmd' in a shell with os.popen() and return a 2-tuple
    (status, output).  cmd is actually run as '{ cmd ; } 2>&1', so that the
    returned output will contain output or error messages.  A trailing newline
    is stripped from the output.  The exit status for the command can be
    interpreted according to the rules for the C function wait().  Example:

    >>> import subprocess
    >>> subprocess.getstatusoutput('ls /bin/ls')
    (0, '/bin/ls')
    >>> subprocess.getstatusoutput('cat /bin/junk')
    (256, 'cat: /bin/junk: No such file or directory')
    >>> subprocess.getstatusoutput('/bin/junk')
    (256, 'sh: /bin/junk: not found')
    """
    pipe = os.popen('{ ' + cmd + '; } 2>&1', 'r')
    text = pipe.read()
    sts = pipe.close()
    if sts is None: sts = 0
    if text[-1:] == '\n': text = text[:-1]
    return sts, text


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
    def __init__(self, args, bufsize=0, executable=None,
                 stdin=None, stdout=None, stderr=None,
                 preexec_fn=None, close_fds=_PLATFORM_DEFAULT_CLOSE_FDS,
                 shell=False, cwd=None, env=None, universal_newlines=False,
                 startupinfo=None, creationflags=0,
                 restore_signals=True, start_new_session=False,
                 pass_fds=()):
        """Create new Popen instance."""
        _cleanup()

        self._child_created = False
        self._input = None
        self._communication_started = False
        if bufsize is None:
            bufsize = 0  # Restore default
        if not isinstance(bufsize, int):
            raise TypeError("bufsize must be an integer")

        if mswindows:
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

        if mswindows:
            if p2cwrite != -1:
                p2cwrite = msvcrt.open_osfhandle(p2cwrite.Detach(), 0)
            if c2pread != -1:
                c2pread = msvcrt.open_osfhandle(c2pread.Detach(), 0)
            if errread != -1:
                errread = msvcrt.open_osfhandle(errread.Detach(), 0)

        if p2cwrite != -1:
            self.stdin = io.open(p2cwrite, 'wb', bufsize)
            if self.universal_newlines:
                self.stdin = io.TextIOWrapper(self.stdin)
        if c2pread != -1:
            self.stdout = io.open(c2pread, 'rb', bufsize)
            if universal_newlines:
                self.stdout = io.TextIOWrapper(self.stdout)
        if errread != -1:
            self.stderr = io.open(errread, 'rb', bufsize)
            if universal_newlines:
                self.stderr = io.TextIOWrapper(self.stderr)

        try:
            self._execute_child(args, executable, preexec_fn, close_fds,
                                pass_fds, cwd, env, universal_newlines,
                                startupinfo, creationflags, shell,
                                p2cread, p2cwrite,
                                c2pread, c2pwrite,
                                errread, errwrite,
                                restore_signals, start_new_session)
        except:
            # Cleanup if the child failed starting
            for f in filter(None, [self.stdin, self.stdout, self.stderr]):
                try:
                    f.close()
                except EnvironmentError:
                    # Ignore EBADF or other errors
                    pass
            raise


    def _translate_newlines(self, data, encoding):
        data = data.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
        return data.decode(encoding)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        if self.stdout:
            self.stdout.close()
        if self.stderr:
            self.stderr.close()
        if self.stdin:
            self.stdin.close()

    def __del__(self, _maxsize=sys.maxsize, _active=_active):
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

    def communicate(self, input=None, timeout=None):
        """Interact with process: Send data to stdin.  Read data from
        stdout and stderr, until end-of-file is reached.  Wait for
        process to terminate.  The optional input argument should be a
        string to be sent to the child process, or None, if no data
        should be sent to the child.

        communicate() returns a tuple (stdout, stderr)."""

        if self._communication_started and input:
            raise ValueError("Cannot send input after starting communication")

        if timeout is not None:
            endtime = time.time() + timeout
        else:
            endtime = None

        # Optimization: If we are not worried about timeouts, we haven't
        # started communicating, and we have one or zero pipes, using select()
        # or threads is unnecessary.
        if (endtime is None and not self._communication_started and
            [self.stdin, self.stdout, self.stderr].count(None) >= 2):
            stdout = None
            stderr = None
            if self.stdin:
                if input:
                    self.stdin.write(input)
                self.stdin.close()
            elif self.stdout:
                stdout = self.stdout.read()
                self.stdout.close()
            elif self.stderr:
                stderr = self.stderr.read()
                self.stderr.close()
            self.wait()
            return (stdout, stderr)

        try:
            stdout, stderr = self._communicate(input, endtime, timeout)
        finally:
            self._communication_started = True

        sts = self.wait(timeout=self._remaining_time(endtime))

        return (stdout, stderr)


    def poll(self):
        return self._internal_poll()


    def _remaining_time(self, endtime):
        """Convenience for _communicate when computing timeouts."""
        if endtime is None:
            return None
        else:
            return endtime - time.time()


    def _check_timeout(self, endtime, orig_timeout):
        """Convenience for checking if a timeout has expired."""
        if endtime is None:
            return
        if time.time() > endtime:
            raise TimeoutExpired(self.args, orig_timeout)


    if mswindows:
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
                p2cread = _subprocess.GetStdHandle(_subprocess.STD_INPUT_HANDLE)
                if p2cread is None:
                    p2cread, _ = _subprocess.CreatePipe(None, 0)
            elif stdin == PIPE:
                p2cread, p2cwrite = _subprocess.CreatePipe(None, 0)
            elif stdin == DEVNULL:
                p2cread = msvcrt.get_osfhandle(self._get_devnull())
            elif isinstance(stdin, int):
                p2cread = msvcrt.get_osfhandle(stdin)
            else:
                # Assuming file-like object
                p2cread = msvcrt.get_osfhandle(stdin.fileno())
            p2cread = self._make_inheritable(p2cread)

            if stdout is None:
                c2pwrite = _subprocess.GetStdHandle(_subprocess.STD_OUTPUT_HANDLE)
                if c2pwrite is None:
                    _, c2pwrite = _subprocess.CreatePipe(None, 0)
            elif stdout == PIPE:
                c2pread, c2pwrite = _subprocess.CreatePipe(None, 0)
            elif stdout == DEVNULL:
                c2pwrite = msvcrt.get_osfhandle(self._get_devnull())
            elif isinstance(stdout, int):
                c2pwrite = msvcrt.get_osfhandle(stdout)
            else:
                # Assuming file-like object
                c2pwrite = msvcrt.get_osfhandle(stdout.fileno())
            c2pwrite = self._make_inheritable(c2pwrite)

            if stderr is None:
                errwrite = _subprocess.GetStdHandle(_subprocess.STD_ERROR_HANDLE)
                if errwrite is None:
                    _, errwrite = _subprocess.CreatePipe(None, 0)
            elif stderr == PIPE:
                errread, errwrite = _subprocess.CreatePipe(None, 0)
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
            return _subprocess.DuplicateHandle(_subprocess.GetCurrentProcess(),
                                handle, _subprocess.GetCurrentProcess(), 0, 1,
                                _subprocess.DUPLICATE_SAME_ACCESS)


        def _find_w9xpopen(self):
            """Find and return absolut path to w9xpopen.exe"""
            w9xpopen = os.path.join(
                            os.path.dirname(_subprocess.GetModuleFileName(0)),
                                    "w9xpopen.exe")
            if not os.path.exists(w9xpopen):
                # Eeek - file-not-found - possibly an embedding
                # situation - see if we can locate it in sys.exec_prefix
                w9xpopen = os.path.join(os.path.dirname(sys.exec_prefix),
                                        "w9xpopen.exe")
                if not os.path.exists(w9xpopen):
                    raise RuntimeError("Cannot locate w9xpopen.exe, which is "
                                       "needed for Popen to work with your "
                                       "shell or platform.")
            return w9xpopen


        def _execute_child(self, args, executable, preexec_fn, close_fds,
                           pass_fds, cwd, env, universal_newlines,
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
                startupinfo.dwFlags |= _subprocess.STARTF_USESTDHANDLES
                startupinfo.hStdInput = p2cread
                startupinfo.hStdOutput = c2pwrite
                startupinfo.hStdError = errwrite

            if shell:
                startupinfo.dwFlags |= _subprocess.STARTF_USESHOWWINDOW
                startupinfo.wShowWindow = _subprocess.SW_HIDE
                comspec = os.environ.get("COMSPEC", "cmd.exe")
                args = '{} /c "{}"'.format (comspec, args)
                if (_subprocess.GetVersion() >= 0x80000000 or
                        os.path.basename(comspec).lower() == "command.com"):
                    # Win9x, or using command.com on NT. We need to
                    # use the w9xpopen intermediate program. For more
                    # information, see KB Q150956
                    # (http://web.archive.org/web/20011105084002/http://support.microsoft.com/support/kb/articles/Q150/9/56.asp)
                    w9xpopen = self._find_w9xpopen()
                    args = '"%s" %s' % (w9xpopen, args)
                    # Not passing CREATE_NEW_CONSOLE has been known to
                    # cause random failures on win9x.  Specifically a
                    # dialog: "Your program accessed mem currently in
                    # use at xxx" and a hopeful warning about the
                    # stability of your system.  Cost is Ctrl+C won't
                    # kill children.
                    creationflags |= _subprocess.CREATE_NEW_CONSOLE

            # Start the process
            try:
                hp, ht, pid, tid = _subprocess.CreateProcess(executable, args,
                                         # no special security
                                         None, None,
                                         int(not close_fds),
                                         creationflags,
                                         env,
                                         cwd,
                                         startupinfo)
            except pywintypes.error as e:
                # Translate pywintypes.error to WindowsError, which is
                # a subclass of OSError.  FIXME: We should really
                # translate errno using _sys_errlist (or similar), but
                # how can this be done from Python?
                raise WindowsError(*e.args)
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
            self._handle = hp
            self.pid = pid
            ht.Close()

        def _internal_poll(self, _deadstate=None,
                _WaitForSingleObject=_subprocess.WaitForSingleObject,
                _WAIT_OBJECT_0=_subprocess.WAIT_OBJECT_0,
                _GetExitCodeProcess=_subprocess.GetExitCodeProcess):
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
                timeout = _subprocess.INFINITE
            else:
                timeout = int(timeout * 1000)
            if self.returncode is None:
                result = _subprocess.WaitForSingleObject(self._handle, timeout)
                if result == _subprocess.WAIT_TIMEOUT:
                    raise TimeoutExpired(self.args, timeout)
                self.returncode = _subprocess.GetExitCodeProcess(self._handle)
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
                if input is not None:
                    self.stdin.write(input)
                self.stdin.close()

            # Wait for the reader threads, or time out.  If we time out, the
            # threads remain reading and the fds left open in case the user
            # calls communicate again.
            if self.stdout is not None:
                self.stdout_thread.join(self._remaining_time(endtime))
                if self.stdout_thread.isAlive():
                    raise TimeoutExpired(self.args)
            if self.stderr is not None:
                self.stderr_thread.join(self._remaining_time(endtime))
                if self.stderr_thread.isAlive():
                    raise TimeoutExpired(self.args)

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
            """Send a signal to the process
            """
            if sig == signal.SIGTERM:
                self.terminate()
            elif sig == signal.CTRL_C_EVENT:
                os.kill(self.pid, signal.CTRL_C_EVENT)
            elif sig == signal.CTRL_BREAK_EVENT:
                os.kill(self.pid, signal.CTRL_BREAK_EVENT)
            else:
                raise ValueError("Unsupported signal: {}".format(sig))

        def terminate(self):
            """Terminates the process
            """
            _subprocess.TerminateProcess(self._handle, 1)

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
                p2cread, p2cwrite = _create_pipe()
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
                c2pread, c2pwrite = _create_pipe()
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
                errread, errwrite = _create_pipe()
            elif stderr == STDOUT:
                errwrite = c2pwrite
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


        def _close_fds(self, fds_to_keep):
            start_fd = 3
            for fd in sorted(fds_to_keep):
                if fd >= start_fd:
                    os.closerange(start_fd, fd)
                    start_fd = fd + 1
            if start_fd <= MAXFD:
                os.closerange(start_fd, MAXFD)


        def _execute_child(self, args, executable, preexec_fn, close_fds,
                           pass_fds, cwd, env, universal_newlines,
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

            # For transferring possible exec failure from child to parent.
            # Data format: "exception name:hex errno:description"
            # Pickle is not used; it is complex and involves memory allocation.
            errpipe_read, errpipe_write = _create_pipe()
            try:
                try:

                    if _posixsubprocess:
                        # We must avoid complex work that could involve
                        # malloc or free in the child process to avoid
                        # potential deadlocks, thus we do all this here.
                        # and pass it to fork_exec()

                        if env:
                            env_list = [os.fsencode(k) + b'=' + os.fsencode(v)
                                        for k, v in env.items()]
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
                                close_fds, sorted(fds_to_keep), cwd, env_list,
                                p2cread, p2cwrite, c2pread, c2pwrite,
                                errread, errwrite,
                                errpipe_read, errpipe_write,
                                restore_signals, start_new_session, preexec_fn)
                    else:
                        # Pure Python implementation: It is not thread safe.
                        # This implementation may deadlock in the child if your
                        # parent process has any other threads running.

                        gc_was_enabled = gc.isenabled()
                        # Disable gc to avoid bug where gc -> file_dealloc ->
                        # write to stderr -> hang.  See issue1336
                        gc.disable()
                        try:
                            self.pid = os.fork()
                        except:
                            if gc_was_enabled:
                                gc.enable()
                            raise
                        self._child_created = True
                        if self.pid == 0:
                            # Child
                            try:
                                # Close parent's pipe ends
                                if p2cwrite != -1:
                                    os.close(p2cwrite)
                                if c2pread != -1:
                                    os.close(c2pread)
                                if errread != -1:
                                    os.close(errread)
                                os.close(errpipe_read)

                                # Dup fds for child
                                def _dup2(a, b):
                                    # dup2() removes the CLOEXEC flag but
                                    # we must do it ourselves if dup2()
                                    # would be a no-op (issue #10806).
                                    if a == b:
                                        _set_cloexec(a, False)
                                    elif a != -1:
                                        os.dup2(a, b)
                                _dup2(p2cread, 0)
                                _dup2(c2pwrite, 1)
                                _dup2(errwrite, 2)

                                # Close pipe fds.  Make sure we don't close the
                                # same fd more than once, or standard fds.
                                closed = set()
                                for fd in [p2cread, c2pwrite, errwrite]:
                                    if fd > 2 and fd not in closed:
                                        os.close(fd)
                                        closed.add(fd)

                                # Close all other fds, if asked for
                                if close_fds:
                                    fds_to_keep = set(pass_fds)
                                    fds_to_keep.add(errpipe_write)
                                    self._close_fds(fds_to_keep)


                                if cwd is not None:
                                    os.chdir(cwd)

                                # This is a copy of Python/pythonrun.c
                                # _Py_RestoreSignals().  If that were exposed
                                # as a sys._py_restoresignals func it would be
                                # better.. but this pure python implementation
                                # isn't likely to be used much anymore.
                                if restore_signals:
                                    signals = ('SIGPIPE', 'SIGXFZ', 'SIGXFSZ')
                                    for sig in signals:
                                        if hasattr(signal, sig):
                                            signal.signal(getattr(signal, sig),
                                                          signal.SIG_DFL)

                                if start_new_session and hasattr(os, 'setsid'):
                                    os.setsid()

                                if preexec_fn:
                                    preexec_fn()

                                if env is None:
                                    os.execvp(executable, args)
                                else:
                                    os.execvpe(executable, args, env)

                            except:
                                try:
                                    exc_type, exc_value = sys.exc_info()[:2]
                                    if isinstance(exc_value, OSError):
                                        errno_num = exc_value.errno
                                    else:
                                        errno_num = 0
                                    message = '%s:%x:%s' % (exc_type.__name__,
                                                            errno_num, exc_value)
                                    message = message.encode(errors="surrogatepass")
                                    os.write(errpipe_write, message)
                                except Exception:
                                    # We MUST not allow anything odd happening
                                    # above to prevent us from exiting below.
                                    pass

                            # This exitcode won't be reported to applications
                            # so it really doesn't matter what we return.
                            os._exit(255)

                        # Parent
                        if gc_was_enabled:
                            gc.enable()
                finally:
                    # be sure the FD is closed no matter what
                    os.close(errpipe_write)

                if p2cread != -1 and p2cwrite != -1:
                    os.close(p2cread)
                if c2pwrite != -1 and c2pread != -1:
                    os.close(c2pwrite)
                if errwrite != -1 and errread != -1:
                    os.close(errwrite)
                if hasattr(self, '_devnull'):
                    os.close(self._devnull)

                # Wait for exec to fail or succeed; possibly raising an
                # exception (limited in size)
                data = bytearray()
                while True:
                    part = _eintr_retry_call(os.read, errpipe_read, 50000)
                    data += part
                    if not part or len(data) > 50000:
                        break
            finally:
                # be sure the FD is closed no matter what
                os.close(errpipe_read)

            if data:
                try:
                    _eintr_retry_call(os.waitpid, self.pid, 0)
                except OSError as e:
                    if e.errno != errno.ECHILD:
                        raise
                try:
                    exception_name, hex_errno, err_msg = data.split(b':', 2)
                except ValueError:
                    print('Bad exception data:', repr(data))
                    exception_name = b'RuntimeError'
                    hex_errno = b'0'
                    err_msg = b'Unknown'
                child_exception_type = getattr(
                        builtins, exception_name.decode('ascii'),
                        RuntimeError)
                for fd in (p2cwrite, c2pread, errread):
                    if fd != -1:
                        os.close(fd)
                err_msg = err_msg.decode(errors="surrogatepass")
                if issubclass(child_exception_type, OSError) and hex_errno:
                    errno_num = int(hex_errno, 16)
                    if errno_num != 0:
                        err_msg = os.strerror(errno_num)
                        if errno_num == errno.ENOENT:
                            err_msg += ': ' + repr(args[0])
                    raise child_exception_type(errno_num, err_msg)
                raise child_exception_type(err_msg)


        def _handle_exitstatus(self, sts, _WIFSIGNALED=os.WIFSIGNALED,
                _WTERMSIG=os.WTERMSIG, _WIFEXITED=os.WIFEXITED,
                _WEXITSTATUS=os.WEXITSTATUS):
            # This method is called (indirectly) by __del__, so it cannot
            # refer to anything outside of its local scope."""
            if _WIFSIGNALED(sts):
                self.returncode = -_WTERMSIG(sts)
            elif _WIFEXITED(sts):
                self.returncode = _WEXITSTATUS(sts)
            else:
                # Should never happen
                raise RuntimeError("Unknown child exit status!")


        def _internal_poll(self, _deadstate=None, _waitpid=os.waitpid,
                _WNOHANG=os.WNOHANG, _os_error=os.error):
            """Check if child process has terminated.  Returns returncode
            attribute.

            This method is called by __del__, so it cannot reference anything
            outside of the local scope (nor can any methods it calls).

            """
            if self.returncode is None:
                try:
                    pid, sts = _waitpid(self.pid, _WNOHANG)
                    if pid == self.pid:
                        self._handle_exitstatus(sts)
                except _os_error:
                    if _deadstate is not None:
                        self.returncode = _deadstate
            return self.returncode


        def _try_wait(self, wait_flags):
            try:
                (pid, sts) = _eintr_retry_call(os.waitpid, self.pid, wait_flags)
            except OSError as e:
                if e.errno != errno.ECHILD:
                    raise
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
                    endtime = time.time() + timeout
                elif timeout is None:
                    timeout = self._remaining_time(endtime)

            if endtime is not None:
                # Enter a busy loop if we have a timeout.  This busy loop was
                # cribbed from Lib/threading.py in Thread.wait() at r71065.
                delay = 0.0005 # 500 us -> initial delay of 1 ms
                while True:
                    (pid, sts) = self._try_wait(os.WNOHANG)
                    assert pid == self.pid or pid == 0
                    if pid == self.pid:
                        self._handle_exitstatus(sts)
                        break
                    remaining = self._remaining_time(endtime)
                    if remaining <= 0:
                        raise TimeoutExpired(self.args, timeout)
                    delay = min(delay * 2, remaining, .05)
                    time.sleep(delay)
            elif self.returncode is None:
                (pid, sts) = self._try_wait(0)
                self._handle_exitstatus(sts)
            return self.returncode


        def _communicate(self, input, endtime, orig_timeout):
            if self.stdin and not self._communication_started:
                # Flush stdio buffer.  This might block, if the user has
                # been writing to .stdin in an uncontrolled fashion.
                self.stdin.flush()
                if not input:
                    self.stdin.close()

            if _has_poll:
                stdout, stderr = self._communicate_with_poll(input, endtime,
                                                             orig_timeout)
            else:
                stdout, stderr = self._communicate_with_select(input, endtime,
                                                               orig_timeout)

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


        def _communicate_with_poll(self, input, endtime, orig_timeout):
            stdout = None # Return
            stderr = None # Return

            if not self._communication_started:
                self._fd2file = {}

            poller = select.poll()
            def register_and_append(file_obj, eventmask):
                poller.register(file_obj.fileno(), eventmask)
                self._fd2file[file_obj.fileno()] = file_obj

            def close_unregister_and_remove(fd):
                poller.unregister(fd)
                self._fd2file[fd].close()
                self._fd2file.pop(fd)

            if self.stdin and input:
                register_and_append(self.stdin, select.POLLOUT)

            # Only create this mapping if we haven't already.
            if not self._communication_started:
                self._fd2output = {}
                if self.stdout:
                    self._fd2output[self.stdout.fileno()] = []
                if self.stderr:
                    self._fd2output[self.stderr.fileno()] = []

            select_POLLIN_POLLPRI = select.POLLIN | select.POLLPRI
            if self.stdout:
                register_and_append(self.stdout, select_POLLIN_POLLPRI)
                stdout = self._fd2output[self.stdout.fileno()]
            if self.stderr:
                register_and_append(self.stderr, select_POLLIN_POLLPRI)
                stderr = self._fd2output[self.stderr.fileno()]

            # Save the input here so that if we time out while communicating,
            # we can continue sending input if we retry.
            if self.stdin and self._input is None:
                self._input_offset = 0
                self._input = input
                if self.universal_newlines:
                    self._input = self._input.encode(self.stdin.encoding)

            while self._fd2file:
                try:
                    ready = poller.poll(self._remaining_time(endtime))
                except select.error as e:
                    if e.args[0] == errno.EINTR:
                        continue
                    raise
                self._check_timeout(endtime, orig_timeout)

                # XXX Rewrite these to use non-blocking I/O on the
                # file objects; they are no longer using C stdio!

                for fd, mode in ready:
                    if mode & select.POLLOUT:
                        chunk = self._input[self._input_offset :
                                            self._input_offset + _PIPE_BUF]
                        self._input_offset += os.write(fd, chunk)
                        if self._input_offset >= len(self._input):
                            close_unregister_and_remove(fd)
                    elif mode & select_POLLIN_POLLPRI:
                        data = os.read(fd, 4096)
                        if not data:
                            close_unregister_and_remove(fd)
                        self._fd2output[fd].append(data)
                    else:
                        # Ignore hang up or errors.
                        close_unregister_and_remove(fd)

            return (stdout, stderr)


        def _communicate_with_select(self, input, endtime, orig_timeout):
            if not self._communication_started:
                self._read_set = []
                self._write_set = []
                if self.stdin and input:
                    self._write_set.append(self.stdin)
                if self.stdout:
                    self._read_set.append(self.stdout)
                if self.stderr:
                    self._read_set.append(self.stderr)

            if self.stdin and self._input is None:
                self._input_offset = 0
                self._input = input
                if self.universal_newlines:
                    self._input = self._input.encode(self.stdin.encoding)

            stdout = None # Return
            stderr = None # Return

            if self.stdout:
                if not self._communication_started:
                    self._stdout_buff = []
                stdout = self._stdout_buff
            if self.stderr:
                if not self._communication_started:
                    self._stderr_buff = []
                stderr = self._stderr_buff

            while self._read_set or self._write_set:
                try:
                    (rlist, wlist, xlist) = \
                        select.select(self._read_set, self._write_set, [],
                                      self._remaining_time(endtime))
                except select.error as e:
                    if e.args[0] == errno.EINTR:
                        continue
                    raise

                # According to the docs, returning three empty lists indicates
                # that the timeout expired.
                if not (rlist or wlist or xlist):
                    raise TimeoutExpired(self.args, orig_timeout)
                # We also check what time it is ourselves for good measure.
                self._check_timeout(endtime, orig_timeout)

                # XXX Rewrite these to use non-blocking I/O on the
                # file objects; they are no longer using C stdio!

                if self.stdin in wlist:
                    chunk = self._input[self._input_offset :
                                        self._input_offset + _PIPE_BUF]
                    bytes_written = os.write(self.stdin.fileno(), chunk)
                    self._input_offset += bytes_written
                    if self._input_offset >= len(self._input):
                        self.stdin.close()
                        self._write_set.remove(self.stdin)

                if self.stdout in rlist:
                    data = os.read(self.stdout.fileno(), 1024)
                    if not data:
                        self.stdout.close()
                        self._read_set.remove(self.stdout)
                    stdout.append(data)

                if self.stderr in rlist:
                    data = os.read(self.stderr.fileno(), 1024)
                    if not data:
                        self.stderr.close()
                        self._read_set.remove(self.stderr)
                    stderr.append(data)

            return (stdout, stderr)


        def send_signal(self, sig):
            """Send a signal to the process
            """
            os.kill(self.pid, sig)

        def terminate(self):
            """Terminate the process with SIGTERM
            """
            self.send_signal(signal.SIGTERM)

        def kill(self):
            """Kill the process with SIGKILL
            """
            self.send_signal(signal.SIGKILL)


def _demo_posix():
    #
    # Example 1: Simple redirection: Get process list
    #
    plist = Popen(["ps"], stdout=PIPE).communicate()[0]
    print("Process list:")
    print(plist)

    #
    # Example 2: Change uid before executing child
    #
    if os.getuid() == 0:
        p = Popen(["id"], preexec_fn=lambda: os.setuid(100))
        p.wait()

    #
    # Example 3: Connecting several subprocesses
    #
    print("Looking for 'hda'...")
    p1 = Popen(["dmesg"], stdout=PIPE)
    p2 = Popen(["grep", "hda"], stdin=p1.stdout, stdout=PIPE)
    print(repr(p2.communicate()[0]))

    #
    # Example 4: Catch execution error
    #
    print()
    print("Trying a weird file...")
    try:
        print(Popen(["/this/path/does/not/exist"]).communicate())
    except OSError as e:
        if e.errno == errno.ENOENT:
            print("The file didn't exist.  I thought so...")
            print("Child traceback:")
            print(e.child_traceback)
        else:
            print("Error", e.errno)
    else:
        print("Gosh.  No error.", file=sys.stderr)


def _demo_windows():
    #
    # Example 1: Connecting several subprocesses
    #
    print("Looking for 'PROMPT' in set output...")
    p1 = Popen("set", stdout=PIPE, shell=True)
    p2 = Popen('find "PROMPT"', stdin=p1.stdout, stdout=PIPE)
    print(repr(p2.communicate()[0]))

    #
    # Example 2: Simple execution of program
    #
    print("Executing calc...")
    p = Popen("calc")
    p.wait()


if __name__ == "__main__":
    if mswindows:
        _demo_windows()
    else:
        _demo_posix()
