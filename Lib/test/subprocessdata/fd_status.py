"""When called as a script, print a comma-separated list of the open
file descriptors on stdout."""

import errno
import os

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
        if st.st_mode & 0xF000 != 0xd000:
            fds.append(fd)
    print(','.join(map(str, fds)))
