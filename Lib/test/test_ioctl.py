import unittest
from test_support import TestSkipped, run_unittest
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
        pgrp = os.getpgrp()
        tty = open("/dev/tty", "r")
        r = fcntl.ioctl(tty, termios.TIOCGPGRP, "    ")
        self.assertEquals(pgrp, struct.unpack("i", r)[0])

    def test_ioctl_mutate(self):
        import array
        buf = array.array('i', [0])
        pgrp = os.getpgrp()
        tty = open("/dev/tty", "r")
        r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf, 1)
        self.assertEquals(r, 0)
        self.assertEquals(pgrp, buf[0])

def test_main():
    run_unittest(IoctlTests)

if __name__ == "__main__":
    test_main()
