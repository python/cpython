"""Pseudo terminal utilities."""

# Author: Steen Lumholt -- with additions by Guido.
#
# Tested on Linux, FreeBSD, NetBSD, OpenBSD, and macOS.
#
# See:  W. Richard Stevens. 1992.  Advanced Programming in the
#       UNIX Environment.  Chapter 19.


from select import select
import os
import sys
import tty
import signal

# names imported directly for test mocking purposes
from os import close, waitpid
from tty import setraw, tcgetattr, tcsetattr

__all__ = ["openpty", "fork", "spawn"]

STDIN_FILENO = 0
STDOUT_FILENO = 1
STDERR_FILENO = 2

CHILD = 0

ALL_SIGNALS = signal.valid_signals()
HAVE_WINSZ = hasattr(tty, "TIOCGWINSZ") and hasattr(tty, "TIOCSWINSZ")
HAVE_WINCH = HAVE_WINSZ and hasattr(signal, "SIGWINCH")

def openpty(mode=None, winsz=None, name=False):
    """Open a pty master/slave pair, using os.openpty() if possible."""

    try:
        master_fd, slave_fd = os.openpty()
    except (AttributeError, OSError):
        master_fd, slave_name = _open_terminal()
        slave_fd = slave_open(slave_name)
    else:
        if name:
            slave_name = os.ttyname(slave_fd)

    if mode:
        tty.tcsetattr(slave_fd, tty.TCSAFLUSH, mode)

    if HAVE_WINSZ and winsz:
        tty.tcsetwinsize(slave_fd, winsz)

    if name:
        return master_fd, slave_fd, slave_name
    else:
        return master_fd, slave_fd

def master_open():
    """master_open() -> (master_fd, slave_name)
    Open a pty master and return the fd, and the filename of the slave end.
    Deprecated, use openpty() instead."""

    import warnings
    warnings.warn("Use pty.openpty() instead.", DeprecationWarning, stacklevel=2)  # Remove API in 3.14

    try:
        master_fd, slave_fd = os.openpty()
    except (AttributeError, OSError):
        pass
    else:
        slave_name = os.ttyname(slave_fd)
        os.close(slave_fd)
        return master_fd, slave_name

    return _open_terminal()

def _open_terminal():
    """Open pty master and return (master_fd, tty_name)."""
    for x in 'pqrstuvwxyzPQRST':
        for y in '0123456789abcdef':
            pty_name = '/dev/pty' + x + y
            try:
                fd = os.open(pty_name, os.O_RDWR)
            except OSError:
                continue
            return (fd, '/dev/tty' + x + y)
    raise OSError('out of pty devices')

def slave_open(tty_name):
    """slave_open(tty_name) -> slave_fd
    Open the pty slave and acquire the controlling terminal, returning
    opened filedescriptor.
    Deprecated, use openpty() instead."""

    import warnings
    warnings.warn("Use pty.openpty() instead.", DeprecationWarning, stacklevel=2)  # Remove API in 3.14

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

def fork(mode=None, winsz=None):
    """Fork and make the child a session leader with controlling terminal."""
    try:
        pid, master_fd = os.forkpty()
    except (AttributeError, OSError):
        master_fd, slave_fd = openpty(mode, winsz)
        pid = os.fork()
        if pid == CHILD:
            os.close(master_fd)
            os.login_tty(slave_fd)
        else:
            os.close(slave_fd)
    else:
        if pid == CHILD:
            try:
                os.setsid()
            except OSError:
                # os.forkpty() already set us session leader
                pass

            # os.forkpty() makes sure that the slave end of
            # the pty becomes the stdin of the child; this
            # is usually done via a dup2() call
            if mode:
                tty.tcsetattr(STDIN_FILENO, tty.TCSAFLUSH, mode)

            if HAVE_WINSZ and winsz:
                tty.tcsetwinsize(STDIN_FILENO, winsz)

    # Parent and child process.
    return pid, master_fd

def _writen(fd, data):
    """Write all the data to a descriptor."""
    while data:
        n = os.write(fd, data)
        data = data[n:]

def _read(fd):
    """Default read function."""
    return os.read(fd, 1024)

def _getmask():
    """Get signal mask of current thread."""
    return signal.pthread_sigmask(signal.SIG_BLOCK, [])

def _sigblock():
    """Block all signals."""
    signal.pthread_sigmask(signal.SIG_BLOCK, ALL_SIGNALS)

def _sigreset(saved_mask):
    """Restore signal mask."""
    signal.pthread_sigmask(signal.SIG_SETMASK, saved_mask)

def _setup_pty(slave_echo):
    """Open and setup a pty pair.

    If current stdin is a tty, then apply current stdin's
    termios and winsize to the slave and set current stdin to
    raw mode. Return (master, slave, original stdin mode/None,
    stdin winsize/None)."""
    stdin_mode = None
    stdin_winsz = None
    try:
        mode = tty.tcgetattr(STDIN_FILENO)
    except tty.error:
        stdin_tty = False
        fd = slave_fd

        master_fd, slave_fd, slave_path = openpty(name=True)
        mode = tty.tcgetattr(slave_fd)
    else:
        stdin_tty = True
        fd = STDIN_FILENO

        stdin_mode = list(mode)
        if HAVE_WINSZ:
            stdin_winsz = tty.tcgetwinsize(STDIN_FILENO)

    if slave_echo:
        mode[tty.LFLAG] |= tty.ECHO
    else:
        mode[tty.LFLAG] &= ~tty.ECHO

    if stdin_tty:
        master_fd, slave_fd, slave_path = openpty(mode, winsz, True)
        tty.cfmakeraw(mode)

    tty.tcsetattr(fd, tty.TCSAFLUSH, mode)

    return master_fd, slave_fd, slave_path, stdin_mode, stdin_winsz

def _setup_winch(slave_path, saved_mask, handle_winch):
    """Install SIGWINCH handler.

    Returns old SIGWINCH handler if relevant; returns
    None otherwise."""
    old_hwinch = None
    if handle_winch:
        def hwinch(signum, frame):
            """Handle SIGWINCH."""
            _sigblock()
            new_slave_fd = os.open(slave_path, os.O_RDWR)
            tty.setwinsize(new_slave_fd, tty.getwinsize(STDIN_FILENO))
            os.close(new_slave_fd)
            _sigreset(saved_mask)

        try:
            # Raises ValueError if not called from main thread.
            old_hwinch = signal.signal(signal.SIGWINCH, hwinch)
        except ValueError:
            pass

    return old_hwinch

def _copy(master_fd, master_read=_read, stdin_read=_read, \
          saved_mask=set()):
    """Parent copy loop for pty.spawn().
    Copies
            pty master -> standard output   (master_read)
            standard input -> pty master    (stdin_read)"""
    fds = [master_fd, STDIN_FILENO]
    while True:
        _sigreset(saved_mask)
        rfds = select(fds, [], [])[0]
        _sigblock()

        if master_fd in rfds:
            # Some OSes signal EOF by returning an empty byte string,
            # some throw OSErrors.
            try:
                data = master_read(master_fd)
            except OSError:
                data = b""
            if not data:  # Reached EOF.
                return    # Assume the child process has exited and is
                          # unreachable, so we clean up.
            else:
                os.write(STDOUT_FILENO, data)

        if STDIN_FILENO in rfds:
            data = stdin_read(STDIN_FILENO)
            if not data:
                fds.remove(STDIN_FILENO)
            else:
                _writen(master_fd, data)

def spawn(argv, master_read=_read, stdin_read=_read, slave_echo=True, \
          handle_winch=False):
    """Create a spawned process."""
    if isinstance(argv, str):
        argv = (argv,)
    sys.audit('pty.spawn', argv)

    saved_mask = _getmask()
    _sigblock() # Reset during select() in _copy.

    master_fd, slave_fd, slave_path, mode, winsz = _setup_pty(slave_echo)
    handle_winch = handle_winch and (winsz != None) and HAVE_WINCH
    old_hwinch = _setup_winch(slave_path, saved_mask, handle_winch)

    pid = os.fork()
    if pid == CHILD:
        os.close(master_fd)
        os.login_tty(slave_fd)
        _sigreset(saved_mask)
        os.execlp(argv[0], *argv)

    os.close(slave_fd)

   try:
        _copy(master_fd, master_read, stdin_read, saved_mask)
    finally:
        if mode:
            tcsetattr(STDIN_FILENO, tty.TCSAFLUSH, mode)

    if old_hwinch:
        signal.signal(signal.SIGWINCH, old_hwinch)
    close(master_fd)
    _sigreset(saved_mask)

    return waitpid(pid, 0)[1]
