import array
import os
import struct
import sys
import threading
import unittest
from test import support
from test.support import os_helper, threading_helper
from test.support.import_helper import import_module
fcntl = import_module('fcntl')
termios = import_module('termios')

class IoctlTestsTty(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        TIOCGPGRP = support.get_attribute(termios, 'TIOCGPGRP')
        try:
            tty = open("/dev/tty", "rb")
        except OSError:
            raise unittest.SkipTest("Unable to open /dev/tty")
        with tty:
            # Skip if another process is in foreground
            r = fcntl.ioctl(tty, TIOCGPGRP, struct.pack("i", 0))
        rpgrp = struct.unpack("i", r)[0]
        if rpgrp not in (os.getpgrp(), os.getsid(0)):
            raise unittest.SkipTest("Neither the process group nor the session "
                                    "are attached to /dev/tty")

    def test_ioctl_immutable_buf(self):
        # If this process has been put into the background, TIOCGPGRP returns
        # the session ID instead of the process group id.
        ids = (os.getpgrp(), os.getsid(0))
        with open("/dev/tty", "rb") as tty:
            # string
            buf = " "*8
            r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf)
            self.assertIsInstance(r, bytes)
            rpgrp = memoryview(r).cast('i')[0]
            self.assertIn(rpgrp, ids)

            # bytes
            buf = b" "*8
            r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf)
            self.assertIsInstance(r, bytes)
            rpgrp = memoryview(r).cast('i')[0]
            self.assertIn(rpgrp, ids)

            # read-only buffer
            r = fcntl.ioctl(tty, termios.TIOCGPGRP, memoryview(buf))
            self.assertIsInstance(r, bytes)
            rpgrp = memoryview(r).cast('i')[0]
            self.assertIn(rpgrp, ids)

    def test_ioctl_mutable_buf(self):
        ids = (os.getpgrp(), os.getsid(0))
        with open("/dev/tty", "rb") as tty:
            buf = bytearray(b" "*8)
            r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf)
            self.assertEqual(r, 0)
            rpgrp = memoryview(buf).cast('i')[0]
            self.assertIn(rpgrp, ids)

    def test_ioctl_no_mutate_buf(self):
        ids = (os.getpgrp(), os.getsid(0))
        with open("/dev/tty", "rb") as tty:
            buf = bytearray(b" "*8)
            save_buf = bytes(buf)
            r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf, False)
            self.assertEqual(bytes(buf), save_buf)
            self.assertIsInstance(r, bytes)
            rpgrp = memoryview(r).cast('i')[0]
            self.assertIn(rpgrp, ids)

    def _create_int_buf(self, nbytes=None):
        buf = array.array('i')
        intsize = buf.itemsize
        # A fill value unlikely to be in `ids`
        fill = -12345
        if nbytes is not None:
            # Extend the buffer so that it is exactly `nbytes` bytes long
            buf.extend([fill] * (nbytes // intsize))
            self.assertEqual(len(buf) * intsize, nbytes)   # sanity check
        else:
            buf.append(fill)
        return buf

    def _check_ioctl_mutate_len(self, nbytes=None):
        ids = (os.getpgrp(), os.getsid(0))
        buf = self._create_int_buf(nbytes)
        with open("/dev/tty", "rb") as tty:
            r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf)
        rpgrp = buf[0]
        self.assertEqual(r, 0)
        self.assertIn(rpgrp, ids)

    def _check_ioctl_not_mutate_len(self, nbytes=None):
        ids = (os.getpgrp(), os.getsid(0))
        buf = self._create_int_buf(nbytes)
        save_buf = bytes(buf)
        with open("/dev/tty", "rb") as tty:
            r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf, False)
        self.assertIsInstance(r, bytes)
        self.assertEqual(len(r), len(save_buf))
        self.assertEqual(bytes(buf), save_buf)
        rpgrp = array.array('i', r)[0]
        rpgrp = memoryview(r).cast('i')[0]
        self.assertIn(rpgrp, ids)

        buf = bytes(buf)
        with open("/dev/tty", "rb") as tty:
            r = fcntl.ioctl(tty, termios.TIOCGPGRP, buf, True)
        self.assertIsInstance(r, bytes)
        self.assertEqual(len(r), len(save_buf))
        self.assertEqual(buf, save_buf)
        rpgrp = array.array('i', r)[0]
        rpgrp = memoryview(r).cast('i')[0]
        self.assertIn(rpgrp, ids)

    def test_ioctl_mutate(self):
        self._check_ioctl_mutate_len()
        self._check_ioctl_not_mutate_len()

    def test_ioctl_mutate_1024(self):
        # Issue #9758: a mutable buffer of exactly 1024 bytes wouldn't be
        # copied back after the system call.
        self._check_ioctl_mutate_len(1024)
        self._check_ioctl_not_mutate_len(1024)

    def test_ioctl_mutate_2048(self):
        self._check_ioctl_mutate_len(2048)
        self._check_ioctl_not_mutate_len(1024)


@unittest.skipUnless(hasattr(os, 'openpty'), "need os.openpty()")
class IoctlTestsPty(unittest.TestCase):
    def setUp(self):
        self.master_fd, self.slave_fd = os.openpty()
        self.addCleanup(os.close, self.slave_fd)
        self.addCleanup(os.close, self.master_fd)

    @unittest.skipUnless(hasattr(termios, 'TCFLSH'), 'requires termios.TCFLSH')
    def test_ioctl_clear_input_or_output(self):
        wfd = self.slave_fd
        rfd = self.master_fd
        # The data is buffered in the input buffer on Linux, and in
        # the output buffer on other platforms.
        inbuf = sys.platform in ('linux', 'android')

        os.write(wfd, b'abcdef')
        self.assertEqual(os.read(rfd, 2), b'ab')
        if inbuf:
            # don't flush input
            fcntl.ioctl(rfd, termios.TCFLSH, termios.TCOFLUSH)
        else:
            # don't flush output
            fcntl.ioctl(wfd, termios.TCFLSH, termios.TCIFLUSH)
        self.assertEqual(os.read(rfd, 2), b'cd')
        if inbuf:
            # flush input
            fcntl.ioctl(rfd, termios.TCFLSH, termios.TCIFLUSH)
        else:
            # flush output
            fcntl.ioctl(wfd, termios.TCFLSH, termios.TCOFLUSH)
        os.write(wfd, b'ABCDEF')
        self.assertEqual(os.read(rfd, 1024), b'ABCDEF')

    @support.skip_android_selinux('tcflow')
    @unittest.skipUnless(sys.platform in ('linux', 'android'), 'only works on Linux')
    @unittest.skipUnless(hasattr(termios, 'TCXONC'), 'requires termios.TCXONC')
    def test_ioctl_suspend_and_resume_output(self):
        wfd = self.slave_fd
        rfd = self.master_fd
        write_suspended = threading.Event()
        write_finished = threading.Event()

        def writer():
            os.write(wfd, b'abc')
            self.assertTrue(write_suspended.wait(support.SHORT_TIMEOUT))
            os.write(wfd, b'def')
            write_finished.set()

        with threading_helper.start_threads([threading.Thread(target=writer)]):
            self.assertEqual(os.read(rfd, 3), b'abc')
            try:
                try:
                    fcntl.ioctl(wfd, termios.TCXONC, termios.TCOOFF)
                finally:
                    write_suspended.set()
                self.assertFalse(write_finished.wait(0.5),
                                 'output was not suspended')
            finally:
                fcntl.ioctl(wfd, termios.TCXONC, termios.TCOON)
            self.assertTrue(write_finished.wait(support.SHORT_TIMEOUT),
                            'output was not resumed')
            self.assertEqual(os.read(rfd, 1024), b'def')

    def test_ioctl_set_window_size(self):
        # (rows, columns, xpixel, ypixel)
        our_winsz = struct.pack("HHHH", 20, 40, 0, 0)
        result = fcntl.ioctl(self.master_fd, termios.TIOCSWINSZ, our_winsz)
        new_winsz = struct.unpack("HHHH", result)
        self.assertEqual(new_winsz[:2], (20, 40))

    @unittest.skipUnless(hasattr(fcntl, 'FICLONE'), 'need fcntl.FICLONE')
    def test_bad_fd(self):
        # gh-134744: Test error handling
        fd = os_helper.make_bad_fd()
        with self.assertRaises(OSError):
            fcntl.ioctl(fd, fcntl.FICLONE, fd)
        with self.assertRaises(OSError):
            fcntl.ioctl(fd, fcntl.FICLONE, b'\0' * 10)
        with self.assertRaises(OSError):
            fcntl.ioctl(fd, fcntl.FICLONE, b'\0' * 2048)


if __name__ == "__main__":
    unittest.main()
