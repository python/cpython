"""Spawn a command with pipes to its stdin, stdout, and optionally stderr.

The normal os.popen(cmd, mode) call spawns a shell command and provides a
file interface to just the input or output of the process depending on
whether mode is 'r' or 'w'.  This module provides the functions popen2(cmd)
and popen3(cmd) which return two or three pipes to the spawned command.
"""

import os
import sys
import string

MAXFD = 256     # Max number of file descriptors (os.getdtablesize()???)

_active = []

def _cleanup():
    for inst in _active[:]:
        inst.poll()

class Popen3:
    """Class representing a child process.  Normally instances are created
    by the factory functions popen2() and popen3()."""

    def __init__(self, cmd, capturestderr=0, bufsize=-1):
        """The parameter 'cmd' is the shell command to execute in a
        sub-process.  The 'capturestderr' flag, if true, specifies that
        the object should capture standard error output of the child process.
        The default is false.  If the 'bufsize' parameter is specified, it
        specifies the size of the I/O buffers to/from the child process."""
        if type(cmd) == type(''):
            cmd = ['/bin/sh', '-c', cmd]
        p2cread, p2cwrite = os.pipe()
        c2pread, c2pwrite = os.pipe()
        if capturestderr:
            errout, errin = os.pipe()
        self.pid = os.fork()
        if self.pid == 0:
            # Child
            os.close(0)
            os.close(1)
            if os.dup(p2cread) <> 0:
                sys.stderr.write('popen2: bad read dup\n')
            if os.dup(c2pwrite) <> 1:
                sys.stderr.write('popen2: bad write dup\n')
            if capturestderr:
                os.close(2)
                if os.dup(errin) <> 2: pass
            for i in range(3, MAXFD):
                try:
                    os.close(i)
                except: pass
            try:
                os.execvp(cmd[0], cmd)
            finally:
                os._exit(1)
            # Shouldn't come here, I guess
            os._exit(1)
        os.close(p2cread)
        self.tochild = os.fdopen(p2cwrite, 'w', bufsize)
        os.close(c2pwrite)
        self.fromchild = os.fdopen(c2pread, 'r', bufsize)
        if capturestderr:
            os.close(errin)
            self.childerr = os.fdopen(errout, 'r', bufsize)
        else:
            self.childerr = None
        self.sts = -1 # Child not completed yet
        _active.append(self)

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
        pid, sts = os.waitpid(self.pid, 0)
        if pid == self.pid:
            self.sts = sts
            _active.remove(self)
        return self.sts


if sys.platform[:3] == "win":
    def popen2(cmd, mode='t', bufsize=-1):
        """Execute the shell command 'cmd' in a sub-process.  If 'bufsize' is
        specified, it sets the buffer size for the I/O pipes.  The file objects
        (child_stdout, child_stdin) are returned."""
        w, r = os.popen2(cmd, mode, bufsize)
        return r, w
else:
    def popen2(cmd, mode='t', bufsize=-1):
        """Execute the shell command 'cmd' in a sub-process.  If 'bufsize' is
        specified, it sets the buffer size for the I/O pipes.  The file objects
        (child_stdout, child_stdin) are returned."""
        if type(mode) is type(0) and bufsize == -1:
            bufsize = mode
            mode = 't'
        assert mode in ('t', 'b')
        _cleanup()
        inst = Popen3(cmd, 0, bufsize)
        return inst.fromchild, inst.tochild

if sys.platform[:3] == "win":
    def popen3(cmd, mode='t', bufsize=-1):
        """Execute the shell command 'cmd' in a sub-process.  If 'bufsize' is
        specified, it sets the buffer size for the I/O pipes.  The file objects
        (child_stdout, child_stdin, child_stderr) are returned."""
        w, r, e = os.popen3(cmd, mode, bufsize)
        return r, w, e
else:
    def popen3(cmd, mode='t', bufsize=-1):
        """Execute the shell command 'cmd' in a sub-process.  If 'bufsize' is
        specified, it sets the buffer size for the I/O pipes.  The file objects
        (child_stdout, child_stdin, child_stderr) are returned."""
        if type(mode) is type(0) and bufsize == -1:
            bufsize = mode
            mode = 't'
        assert mode in ('t', 'b')
        _cleanup()
        inst = Popen3(cmd, 1, bufsize)
        return inst.fromchild, inst.tochild, inst.childerr

if sys.platform[:3] == "win":
    def popen4(cmd, mode='t', bufsize=-1):
        """Execute the shell command 'cmd' in a sub-process.  If 'bufsize' is
        specified, it sets the buffer size for the I/O pipes.  The file objects
        (child_stdout_stderr, child_stdin) are returned."""
        w, r = os.popen4(cmd, mode, bufsize)
        return r, w
else:
    pass # not yet on unix


def _test():
    cmd  = "cat"
    teststr = "abc\n"
    resultstr = teststr
    if os.name == "nt":
        cmd = "more"
        resultstr = "\n" + resultstr
    print "testing popen2..."
    r, w = popen2(cmd)
    w.write(teststr)
    w.close()
    assert r.read() == resultstr
    print "testing popen3..."
    try:
        r, w, e = popen3([cmd])
    except:
        r, w, e = popen3(cmd)
    w.write(teststr)
    w.close()
    assert r.read() == resultstr
    assert e.read() == ""
    for inst in _active[:]:
        inst.wait()
    assert not _active
    print "All OK"

if __name__ == '__main__':
    _test()
