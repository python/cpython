import unittest
from test.test_support import TestSkipped, run_unittest
import os, struct
try:
    import fcntl, termios
except ImportError:
    raise TestSkipped("No fcntl or termios module")
if not hasattr(termios,'TIOCGPGRP'):
    raise TestSkipped("termios module doesn't have TIOCGPGRP")

try:
    tty = open("/dev/tty", "r")
    tty.close()
except IOError:
    raise TestSkipped("Unable to open /dev/tty")

class IoctlTests(unittest.TestCase):
    def test_ioctl(self):
        # If this process has been put into the background, TIOCGPGRP returns
        # the session ID instead of the process group id.
        ids = (os.getpgrp(), os.getsid(0))
        tty = open("/dev/tty", "r")
        r = fcntl.ioctl(tty, termios.TIOCGPGRP, "    ")
        rpgrp = struct.unpack("i", r)[0]
        self.assert_(rpgrp in ids, "%s not in %s" % (rpgrp, ids))

    def test_ioctl_mutate(self):
        import array
        buf = array.array('i', [0])
        ids = (os.getpgrp(), os.getsid(0))
        tty = open("/dev/tty", "r")
        r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf, 1)
        rpgrp = buf[0]
        self.assertEquals(r, 0)
        self.assert_(rpgrp in ids, "%s not in %s" % (rpgrp, ids))

def test_main():
    run_unittest(IoctlTests)

if __name__ == "__main__":
    test_main()
