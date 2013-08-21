"""When called as a script, print a comma-separated list of the open
file descriptors on stdout."""

import errno
import os
import stat

try:
    _MAXFD = os.sysconf("SC_OPEN_MAX")
except:
    _MAXFD = 256

if __name__ == "__main__":
    fds = []
    for fd in range(0, _MAXFD):
        try:
            st = os.fstat(fd)
        except OSError as e:
            if e.errno == errno.EBADF:
                continue
            raise
        # Ignore Solaris door files
        if not stat.S_ISDOOR(st.st_mode):
            fds.append(fd)
    print(','.join(map(str, fds)))
