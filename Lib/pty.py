"""Pseudo terminal utilities."""

# Bugs: No signal handling.  Doesn't set child termios and window size.
#       Only tested on Linux.
# See:  W. Richard Stevens. 1992.  Advanced Programming in the
#       UNIX Environment.  Chapter 19.
# Author: Steen Lumholt -- with additions by Guido.

from select import select
import os
import tty

__all__ = ["openpty","fork","spawn"]

STDIN_FILENO = 0
STDOUT_FILENO = 1
STDERR_FILENO = 2

CHILD = 0

def openpty():
    """openpty() -> (parent_fd, child_fd)
    Open a pty parent/child pair, using os.openpty() if possible."""

    try:
        return os.openpty()
    except (AttributeError, OSError):
        pass
    parent_fd, child_name = _open_terminal()
    child_fd = child_open(child_name)
    return parent_fd, child_fd

def parent_open():
    """parent_open() -> (parent_fd, child_name)
    Open a pty parent and return the fd, and the filename of the child end.
    Deprecated, use openpty() instead."""

    try:
        parent_fd, child_fd = os.openpty()
    except (AttributeError, OSError):
        pass
    else:
        child_name = os.ttyname(child_fd)
        os.close(child_fd)
        return parent_fd, child_name

    return _open_terminal()

def _open_terminal():
    """Open pty parent and return (parent_fd, tty_name)."""
    for x in 'pqrstuvwxyzPQRST':
        for y in '0123456789abcdef':
            pty_name = '/dev/pty' + x + y
            try:
                fd = os.open(pty_name, os.O_RDWR)
            except OSError:
                continue
            return (fd, '/dev/tty' + x + y)
    raise OSError('out of pty devices')

def child_open(tty_name):
    """child_open(tty_name) -> child_fd
    Open the pty child and acquire the controlling terminal, returning
    opened filedescriptor.
    Deprecated, use openpty() instead."""

    result = os.open(tty_name, os.O_RDWR)
    try:
        from fcntl import ioctl, I_PUSH
    except ImportError:
        return result
    try:
        ioctl(result, I_PUSH, "ptem")
        ioctl(result, I_PUSH, "ldterm")
    except OSError:
        pass
    return result

# bpo-34605: master_open()/child_open() has been renamed
# to parent_open()/child_open(), but keep master_open/slave_open aliases
# for backward compatibility.
master_open = parent_open
slave_open = child_open

def fork():
    """fork() -> (pid, parent_fd)
    Fork and make the child a session leader with a controlling terminal."""

    try:
        pid, fd = os.forkpty()
    except (AttributeError, OSError):
        pass
    else:
        if pid == CHILD:
            try:
                os.setsid()
            except OSError:
                # os.forkpty() already set us session leader
                pass
        return pid, fd

    parent_fd, child_fd = openpty()
    pid = os.fork()
    if pid == CHILD:
        # Establish a new session.
        os.setsid()
        os.close(parent_fd)

        # Child becomes stdin/stdout/stderr of child.
        os.dup2(child_fd, STDIN_FILENO)
        os.dup2(child_fd, STDOUT_FILENO)
        os.dup2(child_fd, STDERR_FILENO)
        if (child_fd > STDERR_FILENO):
            os.close (child_fd)

        # Explicitly open the tty to make it become a controlling tty.
        tmp_fd = os.open(os.ttyname(STDOUT_FILENO), os.O_RDWR)
        os.close(tmp_fd)
    else:
        os.close(child_fd)

    # Parent and child process.
    return pid, parent_fd

def _writen(fd, data):
    """Write all the data to a descriptor."""
    while data:
        n = os.write(fd, data)
        data = data[n:]

def _read(fd):
    """Default read function."""
    return os.read(fd, 1024)

def _copy(parent_fd, parent_read=_read, stdin_read=_read):
    """Parent copy loop.
    Copies
            pty parent -> standard output   (parent_read)
            standard input -> pty parent    (stdin_read)"""
    fds = [parent_fd, STDIN_FILENO]
    while True:
        rfds, wfds, xfds = select(fds, [], [])
        if parent_fd in rfds:
            data = parent_read(parent_fd)
            if not data:  # Reached EOF.
                fds.remove(parent_fd)
            else:
                os.write(STDOUT_FILENO, data)
        if STDIN_FILENO in rfds:
            data = stdin_read(STDIN_FILENO)
            if not data:
                fds.remove(STDIN_FILENO)
            else:
                _writen(parent_fd, data)

def spawn(argv, parent_read=_read, stdin_read=_read):
    """Create a spawned process."""
    if type(argv) == type(''):
        argv = (argv,)
    pid, parent_fd = fork()
    if pid == CHILD:
        os.execlp(argv[0], *argv)
    try:
        mode = tty.tcgetattr(STDIN_FILENO)
        tty.setraw(STDIN_FILENO)
        restore = 1
    except tty.error:    # This is the same as termios.error
        restore = 0
    try:
        _copy(parent_fd, parent_read, stdin_read)
    except OSError:
        if restore:
            tty.tcsetattr(STDIN_FILENO, tty.TCSAFLUSH, mode)

    os.close(parent_fd)
    return os.waitpid(pid, 0)[1]
