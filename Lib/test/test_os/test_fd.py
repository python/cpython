"""
Test file descriptors: pipe(), set_blocking(), memfd_create(), eventfd, etc.
"""

import errno
import itertools
import os
import select
import struct
import sys
import unittest
import warnings
from test import support
from test.support import os_helper

try:
    import posix
except ImportError:
    import nt as posix


@unittest.skipIf(support.is_wasi, "Cannot create invalid FD on WASI.")
class TestInvalidFD(unittest.TestCase):
    singles = ["fchdir", "dup", "fstat", "fstatvfs", "tcgetpgrp", "ttyname"]
    singles_fildes = {"fchdir"}
    # systemd-nspawn --suppress-sync=true does not verify fd passed
    # fdatasync() and fsync(), and always returns success
    if not support.in_systemd_nspawn_sync_suppressed():
        singles += ["fdatasync", "fsync"]
        singles_fildes |= {"fdatasync", "fsync"}
    #singles.append("close")
    #We omit close because it doesn't raise an exception on some platforms
    def get_single(f):
        def helper(self):
            if  hasattr(os, f):
                self.check(getattr(os, f))
                if f in self.singles_fildes:
                    self.check_bool(getattr(os, f))
        return helper
    for f in singles:
        locals()["test_"+f] = get_single(f)

    def check(self, f, *args, **kwargs):
        try:
            f(os_helper.make_bad_fd(), *args, **kwargs)
        except OSError as e:
            self.assertEqual(e.errno, errno.EBADF)
        else:
            self.fail("%r didn't raise an OSError with a bad file descriptor"
                      % f)

    def check_bool(self, f, *args, **kwargs):
        with warnings.catch_warnings():
            warnings.simplefilter("error", RuntimeWarning)
            for fd in False, True:
                with self.assertRaises(RuntimeWarning):
                    f(fd, *args, **kwargs)

    def test_fdopen(self):
        self.check(os.fdopen, encoding="utf-8")
        self.check_bool(os.fdopen, encoding="utf-8")

    @unittest.skipUnless(hasattr(os, 'isatty'), 'test needs os.isatty()')
    def test_isatty(self):
        self.assertEqual(os.isatty(os_helper.make_bad_fd()), False)

    @unittest.skipUnless(hasattr(os, 'closerange'), 'test needs os.closerange()')
    def test_closerange(self):
        fd = os_helper.make_bad_fd()
        # Make sure none of the descriptors we are about to close are
        # currently valid (issue 6542).
        for i in range(10):
            try: os.fstat(fd+i)
            except OSError:
                pass
            else:
                break
        if i < 2:
            raise unittest.SkipTest(
                "Unable to acquire a range of invalid file descriptors")
        self.assertEqual(os.closerange(fd, fd + i-1), None)

    @unittest.skipUnless(hasattr(os, 'dup2'), 'test needs os.dup2()')
    def test_dup2(self):
        self.check(os.dup2, 20)

    @unittest.skipUnless(hasattr(os, 'dup2'), 'test needs os.dup2()')
    def test_dup2_negative_fd(self):
        valid_fd = os.open(__file__, os.O_RDONLY)
        self.addCleanup(os.close, valid_fd)
        fds = [
            valid_fd,
            -1,
            -2**31,
        ]
        for fd, fd2 in itertools.product(fds, repeat=2):
            if fd != fd2:
                with self.subTest(fd=fd, fd2=fd2):
                    with self.assertRaises(OSError) as ctx:
                        os.dup2(fd, fd2)
                    self.assertEqual(ctx.exception.errno, errno.EBADF)

    @unittest.skipUnless(hasattr(os, 'fchmod'), 'test needs os.fchmod()')
    def test_fchmod(self):
        self.check(os.fchmod, 0)

    @unittest.skipUnless(hasattr(os, 'fchown'), 'test needs os.fchown()')
    def test_fchown(self):
        self.check(os.fchown, -1, -1)

    @unittest.skipUnless(hasattr(os, 'fpathconf'), 'test needs os.fpathconf()')
    def test_fpathconf(self):
        self.assertIn("PC_NAME_MAX", os.pathconf_names)
        self.check_bool(os.pathconf, "PC_NAME_MAX")
        self.check_bool(os.fpathconf, "PC_NAME_MAX")

    @unittest.skipUnless(hasattr(os, 'fpathconf'), 'test needs os.fpathconf()')
    @unittest.skipIf(
        support.linked_to_musl(),
        'musl pathconf ignores the file descriptor and returns a constant',
        )
    def test_fpathconf_bad_fd(self):
        self.check(os.pathconf, "PC_NAME_MAX")
        self.check(os.fpathconf, "PC_NAME_MAX")

    @unittest.skipUnless(hasattr(os, 'ftruncate'), 'test needs os.ftruncate()')
    def test_ftruncate(self):
        self.check(os.truncate, 0)
        self.check(os.ftruncate, 0)
        self.check_bool(os.truncate, 0)

    @unittest.skipUnless(hasattr(os, 'lseek'), 'test needs os.lseek()')
    def test_lseek(self):
        self.check(os.lseek, 0, 0)

    @unittest.skipUnless(hasattr(os, 'read'), 'test needs os.read()')
    def test_read(self):
        self.check(os.read, 1)

    @unittest.skipUnless(hasattr(os, 'readinto'), 'test needs os.readinto()')
    def test_readinto(self):
        self.check(os.readinto, bytearray(5))

    @unittest.skipUnless(hasattr(os, 'readv'), 'test needs os.readv()')
    def test_readv(self):
        buf = bytearray(10)
        self.check(os.readv, [buf])

    @unittest.skipUnless(hasattr(os, 'tcsetpgrp'), 'test needs os.tcsetpgrp()')
    def test_tcsetpgrpt(self):
        self.check(os.tcsetpgrp, 0)

    @unittest.skipUnless(hasattr(os, 'write'), 'test needs os.write()')
    def test_write(self):
        self.check(os.write, b" ")

    @unittest.skipUnless(hasattr(os, 'writev'), 'test needs os.writev()')
    def test_writev(self):
        self.check(os.writev, [b'abc'])

    @support.requires_subprocess()
    def test_inheritable(self):
        self.check(os.get_inheritable)
        self.check(os.set_inheritable, True)

    @unittest.skipUnless(hasattr(os, 'get_blocking'),
                         'needs os.get_blocking() and os.set_blocking()')
    def test_blocking(self):
        self.check(os.get_blocking)
        self.check(os.set_blocking, True)


@unittest.skipUnless(hasattr(os, 'memfd_create'), 'requires os.memfd_create')
@support.requires_linux_version(3, 17)
class MemfdCreateTests(unittest.TestCase):
    def test_memfd_create(self):
        fd = os.memfd_create("Hi", os.MFD_CLOEXEC)
        self.assertNotEqual(fd, -1)
        self.addCleanup(os.close, fd)
        self.assertFalse(os.get_inheritable(fd))
        with open(fd, "wb", closefd=False) as f:
            f.write(b'memfd_create')
            self.assertEqual(f.tell(), 12)

        fd2 = os.memfd_create("Hi")
        self.addCleanup(os.close, fd2)
        self.assertFalse(os.get_inheritable(fd2))


@unittest.skipUnless(hasattr(os, 'eventfd'), 'requires os.eventfd')
@support.requires_linux_version(2, 6, 30)
class EventfdTests(unittest.TestCase):
    def test_eventfd_initval(self):
        def pack(value):
            """Pack as native uint64_t
            """
            return struct.pack("@Q", value)
        size = 8  # read/write 8 bytes
        initval = 42
        fd = os.eventfd(initval)
        self.assertNotEqual(fd, -1)
        self.addCleanup(os.close, fd)
        self.assertFalse(os.get_inheritable(fd))

        # test with raw read/write
        res = os.read(fd, size)
        self.assertEqual(res, pack(initval))

        os.write(fd, pack(23))
        res = os.read(fd, size)
        self.assertEqual(res, pack(23))

        os.write(fd, pack(40))
        os.write(fd, pack(2))
        res = os.read(fd, size)
        self.assertEqual(res, pack(42))

        # test with eventfd_read/eventfd_write
        os.eventfd_write(fd, 20)
        os.eventfd_write(fd, 3)
        res = os.eventfd_read(fd)
        self.assertEqual(res, 23)

    def test_eventfd_semaphore(self):
        initval = 2
        flags = os.EFD_CLOEXEC | os.EFD_SEMAPHORE | os.EFD_NONBLOCK
        fd = os.eventfd(initval, flags)
        self.assertNotEqual(fd, -1)
        self.addCleanup(os.close, fd)

        # semaphore starts has initval 2, two reads return '1'
        res = os.eventfd_read(fd)
        self.assertEqual(res, 1)
        res = os.eventfd_read(fd)
        self.assertEqual(res, 1)
        # third read would block
        with self.assertRaises(BlockingIOError):
            os.eventfd_read(fd)
        with self.assertRaises(BlockingIOError):
            os.read(fd, 8)

        # increase semaphore counter, read one
        os.eventfd_write(fd, 1)
        res = os.eventfd_read(fd)
        self.assertEqual(res, 1)
        # next read would block, too
        with self.assertRaises(BlockingIOError):
            os.eventfd_read(fd)

    def test_eventfd_select(self):
        flags = os.EFD_CLOEXEC | os.EFD_NONBLOCK
        fd = os.eventfd(0, flags)
        self.assertNotEqual(fd, -1)
        self.addCleanup(os.close, fd)

        # counter is zero, only writeable
        rfd, wfd, xfd = select.select([fd], [fd], [fd], 0)
        self.assertEqual((rfd, wfd, xfd), ([], [fd], []))

        # counter is non-zero, read and writeable
        os.eventfd_write(fd, 23)
        rfd, wfd, xfd = select.select([fd], [fd], [fd], 0)
        self.assertEqual((rfd, wfd, xfd), ([fd], [fd], []))
        self.assertEqual(os.eventfd_read(fd), 23)

        # counter at max, only readable
        os.eventfd_write(fd, (2**64) - 2)
        rfd, wfd, xfd = select.select([fd], [fd], [fd], 0)
        self.assertEqual((rfd, wfd, xfd), ([fd], [], []))
        os.eventfd_read(fd)


@unittest.skipUnless(hasattr(os, 'get_blocking'),
                     'needs os.get_blocking() and os.set_blocking()')
@unittest.skipIf(support.is_emscripten, "Cannot unset blocking flag")
@unittest.skipIf(sys.platform == 'win32', 'Windows only supports blocking on pipes')
class BlockingTests(unittest.TestCase):
    def test_blocking(self):
        fd = os.open(__file__, os.O_RDONLY)
        self.addCleanup(os.close, fd)
        self.assertEqual(os.get_blocking(fd), True)

        os.set_blocking(fd, False)
        self.assertEqual(os.get_blocking(fd), False)

        os.set_blocking(fd, True)
        self.assertEqual(os.get_blocking(fd), True)


class PosixTester(unittest.TestCase):

    @unittest.skipUnless(hasattr(posix, 'pipe'), 'test needs posix.pipe()')
    def test_pipe(self):
        reader, writer = posix.pipe()
        os.close(reader)
        os.close(writer)

    @unittest.skipUnless(hasattr(os, 'pipe2'), "test needs os.pipe2()")
    @support.requires_linux_version(2, 6, 27)
    def test_pipe2(self):
        self.assertRaises(TypeError, os.pipe2, 'DEADBEEF')
        self.assertRaises(TypeError, os.pipe2, 0, 0)

        # try calling with flags = 0, like os.pipe()
        r, w = os.pipe2(0)
        os.close(r)
        os.close(w)

        # test flags
        r, w = os.pipe2(os.O_CLOEXEC|os.O_NONBLOCK)
        self.addCleanup(os.close, r)
        self.addCleanup(os.close, w)
        self.assertFalse(os.get_inheritable(r))
        self.assertFalse(os.get_inheritable(w))
        self.assertFalse(os.get_blocking(r))
        self.assertFalse(os.get_blocking(w))
        # try reading from an empty pipe: this should fail, not block
        self.assertRaises(OSError, os.read, r, 1)
        # try a write big enough to fill-up the pipe: this should either
        # fail or perform a partial write, not block
        try:
            os.write(w, b'x' * support.PIPE_MAX_SIZE)
        except OSError:
            pass

    @support.cpython_only
    @unittest.skipUnless(hasattr(os, 'pipe2'), "test needs os.pipe2()")
    @support.requires_linux_version(2, 6, 27)
    def test_pipe2_c_limits(self):
        # Issue 15989
        import _testcapi
        self.assertRaises(OverflowError, os.pipe2, _testcapi.INT_MAX + 1)
        self.assertRaises(OverflowError, os.pipe2, _testcapi.UINT_MAX + 1)


if __name__ == "__main__":
    unittest.main()
