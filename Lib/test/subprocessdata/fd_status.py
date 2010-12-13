"""When called as a script, print a comma-separated list of the open
file descriptors on stdout."""

import errno
import os
import fcntl

try:
    _MAXFD = os.sysconf("SC_OPEN_MAX")
except:
    _MAXFD = 256

def isopen(fd):
    """Return True if the fd is open, and False otherwise"""
    try:
        fcntl.fcntl(fd, fcntl.F_GETFD, 0)
    except IOError as e:
        if e.errno == errno.EBADF:
            return False
        raise
    return True

if __name__ == "__main__":
    print(','.join(str(fd) for fd in range(0, _MAXFD) if isopen(fd)))
