"""Similar to fd_status.py, but only checks file descriptors passed on the
command line."""

import errno
import os
import sys
import stat

if __name__ == "__main__":
    fds = map(int, sys.argv[1:])
    inherited = []
    for fd in fds:
        try:
            st = os.fstat(fd)
        except OSError as e:
            if e.errno == errno.EBADF:
                continue
            raise
        # Ignore Solaris door files
        if not stat.S_ISDOOR(st.st_mode):
            inherited.append(fd)
    print(','.join(map(str, inherited)))
