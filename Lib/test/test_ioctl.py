import array
import unittest
from test.support import run_unittest, import_module, get_attribute
import os, struct
fcntl = import_module('fcntl')
termios = import_module('termios')
get_attribute(termios, 'TIOCGPGRP') #Can't run tests without this feature

try:
    tty = open("/dev/tty", "r")
except IOError:
    raise unittest.SkipTest("Unable to open /dev/tty")
else:
    # Skip if another process is in foreground
    r = fcntl.ioctl(tty, termios.TIOCGPGRP, "    ")
    tty.close()
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
        with open("/dev/tty", "r") as tty:
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
        with open("/dev/tty", "r") as tty:
            r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf, 1)
        rpgrp = buf[0]
        self.assertEquals(r, 0)
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

    def test_ioctl_signed_unsigned_code_param(self):
        if not pty:
            raise unittest.SkipTest('pty module required')
        mfd, sfd = pty.openpty()
        try:
            if termios.TIOCSWINSZ < 0:
                set_winsz_opcode_maybe_neg = termios.TIOCSWINSZ
                set_winsz_opcode_pos = termios.TIOCSWINSZ & 0xffffffff
            else:
                set_winsz_opcode_pos = termios.TIOCSWINSZ
                set_winsz_opcode_maybe_neg, = struct.unpack("i",
                        struct.pack("I", termios.TIOCSWINSZ))

            our_winsz = struct.pack("HHHH",80,25,0,0)
            # test both with a positive and potentially negative ioctl code
            new_winsz = fcntl.ioctl(mfd, set_winsz_opcode_pos, our_winsz)
            new_winsz = fcntl.ioctl(mfd, set_winsz_opcode_maybe_neg, our_winsz)
        finally:
            os.close(mfd)
            os.close(sfd)

def test_main():
    run_unittest(IoctlTests)

if __name__ == "__main__":
    test_main()
