import unittest
from test.test_support import run_unittest, import_module, get_attribute
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
        tty = open("/dev/tty", "r")
        r = fcntl.ioctl(tty, termios.TIOCGPGRP, "    ")
        rpgrp = struct.unpack("i", r)[0]
        self.assertIn(rpgrp, ids)

    def test_ioctl_mutate(self):
        import array
        buf = array.array('i', [0])
        ids = (os.getpgrp(), os.getsid(0))
        tty = open("/dev/tty", "r")
        r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf, 1)
        rpgrp = buf[0]
        self.assertEquals(r, 0)
        self.assertIn(rpgrp, ids)

    def test_ioctl_signed_unsigned_code_param(self):
        if not pty:
            raise unittest.SkipTest('pty module required')
        mfd, sfd = pty.openpty()
        try:
            if termios.TIOCSWINSZ < 0:
                set_winsz_opcode_maybe_neg = termios.TIOCSWINSZ
                set_winsz_opcode_pos = termios.TIOCSWINSZ & 0xffffffffL
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
