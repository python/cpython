"""Spawn a command with pipes to its stdin, stdout, and optionally stderr.

The normal os.popen(cmd, mode) call spawns a shell command and provides a
file interface to just the input or output of the process depending on
whether mode is 'r' or 'w'.  This module provides the functions popen2(cmd)
and popen3(cmd) which return two or three pipes to the spawned command.
"""

import os
import sys

__all__ = ["popen2", "popen3", "popen4"]

try:
    MAXFD = os.sysconf('SC_OPEN_MAX')
except (AttributeError, ValueError):
    MAXFD = 256

_active = []

def _cleanup():
    for inst in _active[:]:
        inst.poll()

class Popen3:
    """Class representing a child process.  Normally instances are created
    by the factory functions popen2() and popen3()."""

    sts = -1                    # Child not completed yet

    def __init__(self, cmd, capturestderr=False, bufsize=-1):
        """The parameter 'cmd' is the shell command to execute in a
        sub-process.  The 'capturestderr' flag, if true, specifies that
        the object should capture standard error output of the child process.
        The default is false.  If the 'bufsize' parameter is specified, it
        specifies the size of the I/O buffers to/from the child process."""
        _cleanup()
        p2cread, p2cwrite = os.pipe()
        c2pread, c2pwrite = os.pipe()
        if capturestderr:
            errout, errin = os.pipe()
        self.pid = os.fork()
        if self.pid == 0:
            # Child
            os.dup2(p2cread, 0)
            os.dup2(c2pwrite, 1)
            if capturestderr:
                os.dup2(errin, 2)
            self._run_child(cmd)
        os.close(p2cread)
        self.tochild = os.fdopen(p2cwrite, 'w', bufsize)
        os.close(c2pwrite)
        self.fromchild = os.fdopen(c2pread, 'r', bufsize)
        if capturestderr:
            os.close(errin)
            self.childerr = os.fdopen(errout, 'r', bufsize)
        else:
            self.childerr = None
        _active.append(self)

    def _run_child(self, cmd):
        if isinstance(cmd, basestring):
            cmd = ['/bin/sh', '-c', cmd]
        for i in range(3, MAXFD):
            try:
                os.close(i)
            except OSError:
                pass
        try:
            os.execvp(cmd[0], cmd)
        finally:
            os._exit(1)

    def poll(self):
        """Return the exit status of the child process if it has finished,
        or -1 if it hasn't finished yet."""
        if self.sts < 0:
            try:
                pid, sts = os.waitpid(self.pid, os.WNOHANG)
                if pid == self.pid:
                    self.sts = sts
                    _active.remove(self)
            except os.error:
                pass
        return self.sts

    def wait(self):
        """Wait for and return the exit status of the child process."""
        if self.sts < 0:
            pid, sts = os.waitpid(self.pid, 0)
            if pid == self.pid:
                self.sts = sts
                _active.remove(self)
        return self.sts


class Popen4(Popen3):
    childerr = None

    def __init__(self, cmd, bufsize=-1):
        _cleanup()
        p2cread, p2cwrite = os.pipe()
        c2pread, c2pwrite = os.pipe()
        self.pid = os.fork()
        if self.pid == 0:
            # Child
            os.dup2(p2cread, 0)
            os.dup2(c2pwrite, 1)
            os.dup2(c2pwrite, 2)
            self._run_child(cmd)
        os.close(p2cread)
        self.tochild = os.fdopen(p2cwrite, 'w', bufsize)
        os.close(c2pwrite)
        self.fromchild = os.fdopen(c2pread, 'r', bufsize)
        _active.append(self)


if sys.platform[:3] == "win" or sys.platform == "os2emx":
    # Some things don't make sense on non-Unix platforms.
    del Popen3, Popen4

    def popen2(cmd, bufsize=-1, mode='t'):
        """Execute the shell command 'cmd' in a sub-process.  If 'bufsize' is
        specified, it sets the buffer size for the I/O pipes.  The file objects
        (child_stdout, child_stdin) are returned."""
        w, r = os.popen2(cmd, mode, bufsize)
        return r, w

    def popen3(cmd, bufsize=-1, mode='t'):
        """Execute the shell command 'cmd' in a sub-process.  If 'bufsize' is
        specified, it sets the buffer size for the I/O pipes.  The file objects
        (child_stdout, child_stdin, child_stderr) are returned."""
        w, r, e = os.popen3(cmd, mode, bufsize)
        return r, w, e

    def popen4(cmd, bufsize=-1, mode='t'):
        """Execute the shell command 'cmd' in a sub-process.  If 'bufsize' is
        specified, it sets the buffer size for the I/O pipes.  The file objects
        (child_stdout_stderr, child_stdin) are returned."""
        w, r = os.popen4(cmd, mode, bufsize)
        return r, w
else:
    def popen2(cmd, bufsize=-1, mode='t'):
        """Execute the shell command 'cmd' in a sub-process.  If 'bufsize' is
        specified, it sets the buffer size for the I/O pipes.  The file objects
        (child_stdout, child_stdin) are returned."""
        inst = Popen3(cmd, False, bufsize)
        return inst.fromchild, inst.tochild

    def popen3(cmd, bufsize=-1, mode='t'):
        """Execute the shell command 'cmd' in a sub-process.  If 'bufsize' is
        specified, it sets the buffer size for the I/O pipes.  The file objects
        (child_stdout, child_stdin, child_stderr) are returned."""
        inst = Popen3(cmd, True, bufsize)
        return inst.fromchild, inst.tochild, inst.childerr

    def popen4(cmd, bufsize=-1, mode='t'):
        """Execute the shell command 'cmd' in a sub-process.  If 'bufsize' is
        specified, it sets the buffer size for the I/O pipes.  The file objects
        (child_stdout_stderr, child_stdin) are returned."""
        inst = Popen4(cmd, bufsize)
        return inst.fromchild, inst.tochild

    __all__.extend(["Popen3", "Popen4"])

def _test():
    cmd  = "cat"
    teststr = "ab cd\n"
    if os.name == "nt":
        cmd = "more"
    # "more" doesn't act the same way across Windows flavors,
    # sometimes adding an extra newline at the start or the
    # end.  So we strip whitespace off both ends for comparison.
    expected = teststr.strip()
    print "testing popen2..."
    r, w = popen2(cmd)
    w.write(teststr)
    w.close()
    got = r.read()
    if got.strip() != expected:
        raise ValueError("wrote %s read %s" % (`teststr`, `got`))
    print "testing popen3..."
    try:
        r, w, e = popen3([cmd])
    except:
        r, w, e = popen3(cmd)
    w.write(teststr)
    w.close()
    got = r.read()
    if got.strip() != expected:
        raise ValueError("wrote %s read %s" % (`teststr`, `got`))
    got = e.read()
    if got:
        raise ValueError("unexected %s on stderr" % `got`)
    for inst in _active[:]:
        inst.wait()
    if _active:
        raise ValueError("_active not empty")
    print "All OK"

if __name__ == '__main__':
    _test()
