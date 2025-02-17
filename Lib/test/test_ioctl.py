import array
import unittest
from test.support import get_attribute
from test.support.import_helper import import_module
import os, struct
fcntl = import_module('fcntl')
termios = import_module('termios')
get_attribute(termios, 'TIOCGPGRP') #Can't run tests without this feature

try:
    tty = open("/dev/tty", "rb")
except OSError:
    raise unittest.SkipTest("Unable to open /dev/tty")
else:
    with tty:
        # Skip if another process is in foreground
        r = fcntl.ioctl(tty, termios.TIOCGPGRP, "    ")
    rpgrp = struct.unpack("i", r)[0]
    if rpgrp not in (os.getpgrp(), os.getsid(0)):
        raise unittest.SkipTest("Neither the process group nor the session "
                                "are attached to /dev/tty")
    del tty, r, rpgrp

try:
    import pty
except ImportError:
    pty = None

class IoctlTests(unittest.TestCase):
    def test_ioctl(self):
        # If this process has been put into the background, TIOCGPGRP returns
        # the session ID instead of the process group id.
        ids = (os.getpgrp(), os.getsid(0))
        with open("/dev/tty", "rb") as tty:
            r = fcntl.ioctl(tty, termios.TIOCGPGRP, "    ")
            rpgrp = struct.unpack("i", r)[0]
            self.assertIn(rpgrp, ids)

    def _check_ioctl_mutate_len(self, nbytes=None):
        buf = array.array('i')
        intsize = buf.itemsize
        ids = (os.getpgrp(), os.getsid(0))
        # A fill value unlikely to be in `ids`
        fill = -12345
        if nbytes is not None:
            # Extend the buffer so that it is exactly `nbytes` bytes long
            buf.extend([fill] * (nbytes // intsize))
            self.assertEqual(len(buf) * intsize, nbytes)   # sanity check
        else:
            buf.append(fill)
        with open("/dev/tty", "rb") as tty:
            r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf, True)
        rpgrp = buf[0]
        self.assertEqual(r, 0)
        self.assertIn(rpgrp, ids)

    def test_ioctl_mutate(self):
        self._check_ioctl_mutate_len()

    def test_ioctl_mutate_1024(self):
        # Issue #9758: a mutable buffer of exactly 1024 bytes wouldn't be
        # copied back after the system call.
        self._check_ioctl_mutate_len(1024)

    def test_ioctl_mutate_2048(self):
        # Test with a larger buffer, just for the record.
        self._check_ioctl_mutate_len(2048)

    @unittest.skipIf(pty is None, 'pty module required')
    def test_ioctl_set_window_size(self):
        mfd, sfd = pty.openpty()
        try:
            # (rows, columns, xpixel, ypixel)
            our_winsz = struct.pack("HHHH", 20, 40, 0, 0)
            result = fcntl.ioctl(mfd, termios.TIOCSWINSZ, our_winsz)
            new_winsz = struct.unpack("HHHH", result)
            self.assertEqual(new_winsz[:2], (20, 40))
        finally:
            os.close(mfd)
            os.close(sfd)


if __name__ == "__main__":
    unittest.main()
