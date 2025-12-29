import errno
import os
import sys
import tempfile
import threading
import unittest
from test import support
from test.support import threading_helper
from test.support.import_helper import import_module

termios = import_module('termios')


@unittest.skipUnless(hasattr(os, 'openpty'), "need os.openpty()")
class TestFunctions(unittest.TestCase):

    def setUp(self):
        self.master_fd, self.fd = os.openpty()
        self.addCleanup(os.close, self.master_fd)
        self.stream = self.enterContext(open(self.fd, 'wb', buffering=0))
        tmp = self.enterContext(tempfile.TemporaryFile(mode='wb', buffering=0))
        self.bad_fd = tmp.fileno()

    def assertRaisesTermiosError(self, err, callable, *args):
        # Some versions of Android return EACCES when calling termios functions
        # on a regular file.
        errs = [err]
        if sys.platform == 'android' and err == errno.ENOTTY:
            errs.append(errno.EACCES)

        with self.assertRaises(termios.error) as cm:
            callable(*args)
        self.assertIn(cm.exception.args[0], errs)

    def test_tcgetattr(self):
        attrs = termios.tcgetattr(self.fd)
        self.assertIsInstance(attrs, list)
        self.assertEqual(len(attrs), 7)
        for i in range(6):
            self.assertIsInstance(attrs[i], int)
        iflag, oflag, cflag, lflag, ispeed, ospeed, cc = attrs
        self.assertIsInstance(cc, list)
        self.assertEqual(len(cc), termios.NCCS)
        for i, x in enumerate(cc):
            if ((lflag & termios.ICANON) == 0 and
                (i == termios.VMIN or i == termios.VTIME)):
                self.assertIsInstance(x, int)
            else:
                self.assertIsInstance(x, bytes)
                self.assertEqual(len(x), 1)
        self.assertEqual(termios.tcgetattr(self.stream), attrs)

    def test_tcgetattr_errors(self):
        self.assertRaisesTermiosError(errno.ENOTTY, termios.tcgetattr, self.bad_fd)
        self.assertRaises(ValueError, termios.tcgetattr, -1)
        self.assertRaises(OverflowError, termios.tcgetattr, 2**1000)
        self.assertRaises(TypeError, termios.tcgetattr, object())
        self.assertRaises(TypeError, termios.tcgetattr)

    def test_tcsetattr(self):
        attrs = termios.tcgetattr(self.fd)
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        termios.tcsetattr(self.fd, termios.TCSADRAIN, attrs)
        termios.tcsetattr(self.fd, termios.TCSAFLUSH, attrs)
        termios.tcsetattr(self.stream, termios.TCSANOW, attrs)

    def test_tcsetattr_errors(self):
        attrs = termios.tcgetattr(self.fd)
        self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW, tuple(attrs))
        self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW, attrs[:-1])
        self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW, attrs + [0])
        for i in range(6):
            attrs2 = attrs[:]
            attrs2[i] = 2**1000
            self.assertRaises(OverflowError, termios.tcsetattr, self.fd, termios.TCSANOW, attrs2)
            attrs2[i] = object()
            self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW, attrs2)
        self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW, attrs[:-1] + [attrs[-1][:-1]])
        self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW, attrs[:-1] + [attrs[-1] + [b'\0']])
        for i in range(len(attrs[-1])):
            attrs2 = attrs[:]
            attrs2[-1] = attrs2[-1][:]
            attrs2[-1][i] = 2**1000
            self.assertRaises(OverflowError, termios.tcsetattr, self.fd, termios.TCSANOW, attrs2)
            attrs2[-1][i] = object()
            self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW, attrs2)
            attrs2[-1][i] = b''
            self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW, attrs2)
            attrs2[-1][i] = b'\0\0'
            self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW, attrs2)
        self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW, object())
        self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW)
        self.assertRaisesTermiosError(errno.EINVAL, termios.tcsetattr, self.fd, -1, attrs)
        self.assertRaises(OverflowError, termios.tcsetattr, self.fd, 2**1000, attrs)
        self.assertRaises(TypeError, termios.tcsetattr, self.fd, object(), attrs)
        self.assertRaisesTermiosError(errno.ENOTTY, termios.tcsetattr, self.bad_fd, termios.TCSANOW, attrs)
        self.assertRaises(ValueError, termios.tcsetattr, -1, termios.TCSANOW, attrs)
        self.assertRaises(OverflowError, termios.tcsetattr, 2**1000, termios.TCSANOW, attrs)
        self.assertRaises(TypeError, termios.tcsetattr, object(), termios.TCSANOW, attrs)
        self.assertRaises(TypeError, termios.tcsetattr, self.fd, termios.TCSANOW)

    @support.skip_android_selinux('tcsendbreak')
    def test_tcsendbreak(self):
        try:
            termios.tcsendbreak(self.fd, 1)
        except termios.error as exc:
            if exc.args[0] == errno.ENOTTY and sys.platform.startswith(('freebsd', "netbsd")):
                self.skipTest('termios.tcsendbreak() is not supported '
                              'with pseudo-terminals (?) on this platform')
            raise
        termios.tcsendbreak(self.stream, 1)

    @support.skip_android_selinux('tcsendbreak')
    def test_tcsendbreak_errors(self):
        self.assertRaises(OverflowError, termios.tcsendbreak, self.fd, 2**1000)
        self.assertRaises(TypeError, termios.tcsendbreak, self.fd, 0.0)
        self.assertRaises(TypeError, termios.tcsendbreak, self.fd, object())
        self.assertRaisesTermiosError(errno.ENOTTY, termios.tcsendbreak, self.bad_fd, 0)
        self.assertRaises(ValueError, termios.tcsendbreak, -1, 0)
        self.assertRaises(OverflowError, termios.tcsendbreak, 2**1000, 0)
        self.assertRaises(TypeError, termios.tcsendbreak, object(), 0)
        self.assertRaises(TypeError, termios.tcsendbreak, self.fd)

    @support.skip_android_selinux('tcdrain')
    def test_tcdrain(self):
        termios.tcdrain(self.fd)
        termios.tcdrain(self.stream)

    @support.skip_android_selinux('tcdrain')
    def test_tcdrain_errors(self):
        self.assertRaisesTermiosError(errno.ENOTTY, termios.tcdrain, self.bad_fd)
        self.assertRaises(ValueError, termios.tcdrain, -1)
        self.assertRaises(OverflowError, termios.tcdrain, 2**1000)
        self.assertRaises(TypeError, termios.tcdrain, object())
        self.assertRaises(TypeError, termios.tcdrain)

    def test_tcflush(self):
        termios.tcflush(self.fd, termios.TCIFLUSH)
        termios.tcflush(self.fd, termios.TCOFLUSH)
        termios.tcflush(self.fd, termios.TCIOFLUSH)

    def test_tcflush_errors(self):
        self.assertRaisesTermiosError(errno.EINVAL, termios.tcflush, self.fd, -1)
        self.assertRaises(OverflowError, termios.tcflush, self.fd, 2**1000)
        self.assertRaises(TypeError, termios.tcflush, self.fd, object())
        self.assertRaisesTermiosError(errno.ENOTTY, termios.tcflush, self.bad_fd, termios.TCIFLUSH)
        self.assertRaises(ValueError, termios.tcflush, -1, termios.TCIFLUSH)
        self.assertRaises(OverflowError, termios.tcflush, 2**1000, termios.TCIFLUSH)
        self.assertRaises(TypeError, termios.tcflush, object(), termios.TCIFLUSH)
        self.assertRaises(TypeError, termios.tcflush, self.fd)

    def test_tcflush_clear_input_or_output(self):
        wfd = self.fd
        rfd = self.master_fd
        # The data is buffered in the input buffer on Linux, and in
        # the output buffer on other platforms.
        inbuf = sys.platform in ('linux', 'android')

        os.write(wfd, b'abcdef')
        self.assertEqual(os.read(rfd, 2), b'ab')
        if inbuf:
            # don't flush input
            termios.tcflush(rfd, termios.TCOFLUSH)
        else:
            # don't flush output
            termios.tcflush(wfd, termios.TCIFLUSH)
        self.assertEqual(os.read(rfd, 2), b'cd')
        if inbuf:
            # flush input
            termios.tcflush(rfd, termios.TCIFLUSH)
        else:
            # flush output
            termios.tcflush(wfd, termios.TCOFLUSH)
        os.write(wfd, b'ABCDEF')
        self.assertEqual(os.read(rfd, 1024), b'ABCDEF')

    @support.skip_android_selinux('tcflow')
    def test_tcflow(self):
        termios.tcflow(self.fd, termios.TCOOFF)
        termios.tcflow(self.fd, termios.TCOON)
        termios.tcflow(self.fd, termios.TCIOFF)
        termios.tcflow(self.fd, termios.TCION)

    @support.skip_android_selinux('tcflow')
    def test_tcflow_errors(self):
        self.assertRaisesTermiosError(errno.EINVAL, termios.tcflow, self.fd, -1)
        self.assertRaises(OverflowError, termios.tcflow, self.fd, 2**1000)
        self.assertRaises(TypeError, termios.tcflow, self.fd, object())
        self.assertRaisesTermiosError(errno.ENOTTY, termios.tcflow, self.bad_fd, termios.TCOON)
        self.assertRaises(ValueError, termios.tcflow, -1, termios.TCOON)
        self.assertRaises(OverflowError, termios.tcflow, 2**1000, termios.TCOON)
        self.assertRaises(TypeError, termios.tcflow, object(), termios.TCOON)
        self.assertRaises(TypeError, termios.tcflow, self.fd)

    @support.skip_android_selinux('tcflow')
    @unittest.skipUnless(sys.platform in ('linux', 'android'), 'only works on Linux')
    def test_tcflow_suspend_and_resume_output(self):
        wfd = self.fd
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
                    termios.tcflow(wfd, termios.TCOOFF)
                finally:
                    write_suspended.set()
                self.assertFalse(write_finished.wait(0.5),
                                 'output was not suspended')
            finally:
                termios.tcflow(wfd, termios.TCOON)
            self.assertTrue(write_finished.wait(support.SHORT_TIMEOUT),
                            'output was not resumed')
            self.assertEqual(os.read(rfd, 1024), b'def')

    def test_tcgetwinsize(self):
        size = termios.tcgetwinsize(self.fd)
        self.assertIsInstance(size, tuple)
        self.assertEqual(len(size), 2)
        self.assertIsInstance(size[0], int)
        self.assertIsInstance(size[1], int)
        self.assertEqual(termios.tcgetwinsize(self.stream), size)

    def test_tcgetwinsize_errors(self):
        self.assertRaisesTermiosError(errno.ENOTTY, termios.tcgetwinsize, self.bad_fd)
        self.assertRaises(ValueError, termios.tcgetwinsize, -1)
        self.assertRaises(OverflowError, termios.tcgetwinsize, 2**1000)
        self.assertRaises(TypeError, termios.tcgetwinsize, object())
        self.assertRaises(TypeError, termios.tcgetwinsize)

    def test_tcsetwinsize(self):
        size = termios.tcgetwinsize(self.fd)
        termios.tcsetwinsize(self.fd, size)
        termios.tcsetwinsize(self.fd, list(size))
        termios.tcsetwinsize(self.stream, size)

    def test_tcsetwinsize_errors(self):
        size = termios.tcgetwinsize(self.fd)
        self.assertRaises(TypeError, termios.tcsetwinsize, self.fd, size[:-1])
        self.assertRaises(TypeError, termios.tcsetwinsize, self.fd, size + (0,))
        self.assertRaises(TypeError, termios.tcsetwinsize, self.fd, object())
        self.assertRaises(OverflowError, termios.tcsetwinsize, self.fd, (size[0], 2**1000))
        self.assertRaises(TypeError, termios.tcsetwinsize, self.fd, (size[0], float(size[1])))
        self.assertRaises(TypeError, termios.tcsetwinsize, self.fd, (size[0], object()))
        self.assertRaises(OverflowError, termios.tcsetwinsize, self.fd, (2**1000, size[1]))
        self.assertRaises(TypeError, termios.tcsetwinsize, self.fd, (float(size[0]), size[1]))
        self.assertRaises(TypeError, termios.tcsetwinsize, self.fd, (object(), size[1]))
        self.assertRaisesTermiosError(errno.ENOTTY, termios.tcsetwinsize, self.bad_fd, size)
        self.assertRaises(ValueError, termios.tcsetwinsize, -1, size)
        self.assertRaises(OverflowError, termios.tcsetwinsize, 2**1000, size)
        self.assertRaises(TypeError, termios.tcsetwinsize, object(), size)
        self.assertRaises(TypeError, termios.tcsetwinsize, self.fd)


class TestModule(unittest.TestCase):
    def test_constants(self):
        self.assertIsInstance(termios.B0, int)
        self.assertIsInstance(termios.B38400, int)
        self.assertIsInstance(termios.TCSANOW, int)
        self.assertIsInstance(termios.TCSADRAIN, int)
        self.assertIsInstance(termios.TCSAFLUSH, int)
        self.assertIsInstance(termios.TCIFLUSH, int)
        self.assertIsInstance(termios.TCOFLUSH, int)
        self.assertIsInstance(termios.TCIOFLUSH, int)
        self.assertIsInstance(termios.TCOOFF, int)
        self.assertIsInstance(termios.TCOON, int)
        self.assertIsInstance(termios.TCIOFF, int)
        self.assertIsInstance(termios.TCION, int)
        self.assertIsInstance(termios.VTIME, int)
        self.assertIsInstance(termios.VMIN, int)
        self.assertIsInstance(termios.NCCS, int)
        self.assertLess(termios.VTIME, termios.NCCS)
        self.assertLess(termios.VMIN, termios.NCCS)

    def test_ioctl_constants(self):
        # gh-119770: ioctl() constants must be positive
        for name in dir(termios):
            if not name.startswith('TIO'):
                continue
            value = getattr(termios, name)
            with self.subTest(name=name):
                self.assertGreaterEqual(value, 0)

    def test_exception(self):
        self.assertIsSubclass(termios.error, Exception)
        self.assertNotIsSubclass(termios.error, OSError)


if __name__ == '__main__':
    unittest.main()
