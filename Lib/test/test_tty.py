import os
import unittest
from test.support.import_helper import import_module

termios = import_module('termios')
tty = import_module('tty')


@unittest.skipUnless(hasattr(os, 'openpty'), "need os.openpty()")
class TestTty(unittest.TestCase):

    def setUp(self):
        master_fd, self.fd = os.openpty()
        self.addCleanup(os.close, master_fd)
        self.stream = self.enterContext(open(self.fd, 'wb', buffering=0))
        self.fd = self.stream.fileno()
        self.mode = termios.tcgetattr(self.fd)
        self.addCleanup(termios.tcsetattr, self.fd, termios.TCSANOW, self.mode)
        self.addCleanup(termios.tcsetattr, self.fd, termios.TCSAFLUSH, self.mode)

    def check_cbreak(self, mode):
        self.assertEqual(mode[3] & termios.ECHO, 0)
        self.assertEqual(mode[3] & termios.ICANON, 0)
        self.assertEqual(mode[6][termios.VMIN], 1)
        self.assertEqual(mode[6][termios.VTIME], 0)

    def check_raw(self, mode):
        self.assertEqual(mode[0] & termios.ICRNL, 0)
        self.check_cbreak(mode)
        self.assertEqual(mode[0] & termios.ISTRIP, 0)
        self.assertEqual(mode[0] & termios.ICRNL, 0)
        self.assertEqual(mode[1] & termios.OPOST, 0)
        self.assertEqual(mode[2] & termios.PARENB, termios.CS8 & termios.PARENB)
        self.assertEqual(mode[2] & termios.CSIZE, termios.CS8 & termios.CSIZE)
        self.assertEqual(mode[2] & termios.CS8, termios.CS8)
        self.assertEqual(mode[3] & termios.ECHO, 0)
        self.assertEqual(mode[3] & termios.ICANON, 0)
        self.assertEqual(mode[3] & termios.ISIG, 0)
        self.assertEqual(mode[6][termios.VMIN], 1)
        self.assertEqual(mode[6][termios.VTIME], 0)

    def test_setraw(self):
        tty.setraw(self.fd)
        mode = termios.tcgetattr(self.fd)
        self.check_raw(mode)
        tty.setraw(self.fd, termios.TCSANOW)
        tty.setraw(self.stream)
        tty.setraw(fd=self.fd, when=termios.TCSANOW)

    def test_setcbreak(self):
        tty.setcbreak(self.fd)
        mode = termios.tcgetattr(self.fd)
        self.check_cbreak(mode)
        tty.setcbreak(self.fd, termios.TCSANOW)
        tty.setcbreak(self.stream)
        tty.setcbreak(fd=self.fd, when=termios.TCSANOW)


if __name__ == '__main__':
    unittest.main()
