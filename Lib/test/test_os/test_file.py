"""
Test files: open(), read(), pwritev(), truncate(), etc.
"""

import errno
import os
import posix
import sys
import unittest
from test import support
from test.support import os_helper
from .utils import create_file

try:
    import fcntl
except ImportError:
    fcntl = None

try:
    from _testcapi import INT_MAX, PY_SSIZE_T_MAX
except ImportError:
    INT_MAX = PY_SSIZE_T_MAX = sys.maxsize


# bpo-41625: On AIX, splice() only works with a socket, not with a pipe.
requires_splice_pipe = unittest.skipIf(sys.platform.startswith("aix"),
                                       'on AIX, splice() only accepts sockets')

requires_32b = unittest.skipUnless(
    # Emscripten/WASI have 32 bits pointers, but support 64 bits syscall args.
    sys.maxsize < 2**32 and not (support.is_emscripten or support.is_wasi),
    'test is only meaningful on 32-bit builds'
)


# Tests creating TESTFN
class FileTests(unittest.TestCase):
    def setUp(self):
        if os.path.lexists(os_helper.TESTFN):
            os.unlink(os_helper.TESTFN)
    tearDown = setUp

    def test_access(self):
        f = os.open(os_helper.TESTFN, os.O_CREAT|os.O_RDWR)
        os.close(f)
        self.assertTrue(os.access(os_helper.TESTFN, os.W_OK))

    @unittest.skipIf(
        support.is_wasi, "WASI does not support dup."
    )
    def test_closerange(self):
        first = os.open(os_helper.TESTFN, os.O_CREAT|os.O_RDWR)
        # We must allocate two consecutive file descriptors, otherwise
        # it will mess up other file descriptors (perhaps even the three
        # standard ones).
        second = os.dup(first)
        try:
            retries = 0
            while second != first + 1:
                os.close(first)
                retries += 1
                if retries > 10:
                    # XXX test skipped
                    self.skipTest("couldn't allocate two consecutive fds")
                first, second = second, os.dup(second)
        finally:
            os.close(second)
        # close a fd that is open, and one that isn't
        os.closerange(first, first + 2)
        self.assertRaises(OSError, os.write, first, b"a")

    @support.cpython_only
    def test_rename(self):
        path = os_helper.TESTFN
        old = sys.getrefcount(path)
        self.assertRaises(TypeError, os.rename, path, 0)
        new = sys.getrefcount(path)
        self.assertEqual(old, new)

    def test_read(self):
        with open(os_helper.TESTFN, "w+b") as fobj:
            fobj.write(b"spam")
            fobj.flush()
            fd = fobj.fileno()
            os.lseek(fd, 0, 0)
            s = os.read(fd, 4)
            self.assertEqual(type(s), bytes)
            self.assertEqual(s, b"spam")

    def test_readinto(self):
        with open(os_helper.TESTFN, "w+b") as fobj:
            fobj.write(b"spam")
            fobj.flush()
            fd = fobj.fileno()
            os.lseek(fd, 0, 0)
            # Oversized so readinto without hitting end.
            buffer = bytearray(7)
            s = os.readinto(fd, buffer)
            self.assertEqual(type(s), int)
            self.assertEqual(s, 4)
            # Should overwrite the first 4 bytes of the buffer.
            self.assertEqual(buffer[:4], b"spam")

            # Readinto at EOF should return 0 and not touch buffer.
            buffer[:] = b"notspam"
            s = os.readinto(fd, buffer)
            self.assertEqual(type(s), int)
            self.assertEqual(s, 0)
            self.assertEqual(bytes(buffer), b"notspam")
            s = os.readinto(fd, buffer)
            self.assertEqual(s, 0)
            self.assertEqual(bytes(buffer), b"notspam")

            # Readinto a 0 length bytearray when at EOF should return 0
            self.assertEqual(os.readinto(fd, bytearray()), 0)

            # Readinto a 0 length bytearray with data available should return 0.
            os.lseek(fd, 0, 0)
            self.assertEqual(os.readinto(fd, bytearray()), 0)

    @unittest.skipUnless(hasattr(os, 'get_blocking'),
                     'needs os.get_blocking() and os.set_blocking()')
    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    @unittest.skipIf(support.is_emscripten, "set_blocking does not work correctly")
    def test_readinto_non_blocking(self):
        # Verify behavior of a readinto which would block on a non-blocking fd.
        r, w = os.pipe()
        try:
            os.set_blocking(r, False)
            with self.assertRaises(BlockingIOError):
                os.readinto(r, bytearray(5))

            # Pass some data through
            os.write(w, b"spam")
            self.assertEqual(os.readinto(r, bytearray(4)), 4)

            # Still don't block or return 0.
            with self.assertRaises(BlockingIOError):
                os.readinto(r, bytearray(5))

            # At EOF should return size 0
            os.close(w)
            w = None
            self.assertEqual(os.readinto(r, bytearray(5)), 0)
            self.assertEqual(os.readinto(r, bytearray(5)), 0)  # Still EOF

        finally:
            os.close(r)
            if w is not None:
                os.close(w)

    def test_readinto_badarg(self):
        with open(os_helper.TESTFN, "w+b") as fobj:
            fobj.write(b"spam")
            fobj.flush()
            fd = fobj.fileno()
            os.lseek(fd, 0, 0)

            for bad_arg in ("test", bytes(), 14):
                with self.subTest(f"bad buffer {type(bad_arg)}"):
                    with self.assertRaises(TypeError):
                        os.readinto(fd, bad_arg)

            with self.subTest("doesn't work on file objects"):
                with self.assertRaises(TypeError):
                    os.readinto(fobj, bytearray(5))

            # takes two args
            with self.assertRaises(TypeError):
                os.readinto(fd)

            # No data should have been read with the bad arguments.
            buffer = bytearray(4)
            s = os.readinto(fd, buffer)
            self.assertEqual(s, 4)
            self.assertEqual(buffer, b"spam")

    @support.cpython_only
    # Skip the test on 32-bit platforms: the number of bytes must fit in a
    # Py_ssize_t type
    @unittest.skipUnless(INT_MAX < PY_SSIZE_T_MAX,
                         "needs INT_MAX < PY_SSIZE_T_MAX")
    @support.bigmemtest(size=INT_MAX + 10, memuse=1, dry_run=False)
    def test_large_read(self, size):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        create_file(os_helper.TESTFN, b'test')

        # Issue #21932: Make sure that os.read() does not raise an
        # OverflowError for size larger than INT_MAX
        with open(os_helper.TESTFN, "rb") as fp:
            data = os.read(fp.fileno(), size)

        # The test does not try to read more than 2 GiB at once because the
        # operating system is free to return less bytes than requested.
        self.assertEqual(data, b'test')


    @support.cpython_only
    # Skip the test on 32-bit platforms: the number of bytes must fit in a
    # Py_ssize_t type
    @unittest.skipUnless(INT_MAX < PY_SSIZE_T_MAX,
                         "needs INT_MAX < PY_SSIZE_T_MAX")
    @support.bigmemtest(size=INT_MAX + 10, memuse=1, dry_run=False)
    def test_large_readinto(self, size):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        create_file(os_helper.TESTFN, b'test')

        # Issue #21932: For readinto the buffer contains the length rather than
        # a length being passed explicitly to read, should still get capped to a
        # valid size / not raise an OverflowError for sizes larger than INT_MAX.
        buffer = bytearray(INT_MAX + 10)
        with open(os_helper.TESTFN, "rb") as fp:
            length = os.readinto(fp.fileno(), buffer)

        # The test does not try to read more than 2 GiB at once because the
        # operating system is free to return less bytes than requested.
        self.assertEqual(length, 4)
        self.assertEqual(buffer[:4], b'test')

    def test_write(self):
        # os.write() accepts bytes- and buffer-like objects but not strings
        fd = os.open(os_helper.TESTFN, os.O_CREAT | os.O_WRONLY)
        self.assertRaises(TypeError, os.write, fd, "beans")
        os.write(fd, b"bacon\n")
        os.write(fd, bytearray(b"eggs\n"))
        os.write(fd, memoryview(b"spam\n"))
        os.close(fd)
        with open(os_helper.TESTFN, "rb") as fobj:
            self.assertEqual(fobj.read().splitlines(),
                [b"bacon", b"eggs", b"spam"])

    def fdopen_helper(self, *args):
        fd = os.open(os_helper.TESTFN, os.O_RDONLY)
        f = os.fdopen(fd, *args, encoding="utf-8")
        f.close()

    def test_fdopen(self):
        fd = os.open(os_helper.TESTFN, os.O_CREAT|os.O_RDWR)
        os.close(fd)

        self.fdopen_helper()
        self.fdopen_helper('r')
        self.fdopen_helper('r', 100)

    def test_replace(self):
        TESTFN2 = os_helper.TESTFN + ".2"
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        self.addCleanup(os_helper.unlink, TESTFN2)

        create_file(os_helper.TESTFN, b"1")
        create_file(TESTFN2, b"2")

        os.replace(os_helper.TESTFN, TESTFN2)
        self.assertRaises(FileNotFoundError, os.stat, os_helper.TESTFN)
        with open(TESTFN2, 'r', encoding='utf-8') as f:
            self.assertEqual(f.read(), "1")

    def test_open_keywords(self):
        f = os.open(path=__file__, flags=os.O_RDONLY, mode=0o777,
            dir_fd=None)
        os.close(f)

    def test_symlink_keywords(self):
        symlink = support.get_attribute(os, "symlink")
        try:
            symlink(src='target', dst=os_helper.TESTFN,
                target_is_directory=False, dir_fd=None)
        except (NotImplementedError, OSError):
            pass  # No OS support or unprivileged user

    @unittest.skipUnless(hasattr(os, 'copy_file_range'), 'test needs os.copy_file_range()')
    def test_copy_file_range_invalid_values(self):
        with self.assertRaises(ValueError):
            os.copy_file_range(0, 1, -10)

    @unittest.skipUnless(hasattr(os, 'copy_file_range'), 'test needs os.copy_file_range()')
    def test_copy_file_range(self):
        TESTFN2 = os_helper.TESTFN + ".3"
        data = b'0123456789'

        create_file(os_helper.TESTFN, data)
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)

        in_file = open(os_helper.TESTFN, 'rb')
        self.addCleanup(in_file.close)
        in_fd = in_file.fileno()

        out_file = open(TESTFN2, 'w+b')
        self.addCleanup(os_helper.unlink, TESTFN2)
        self.addCleanup(out_file.close)
        out_fd = out_file.fileno()

        try:
            i = os.copy_file_range(in_fd, out_fd, 5)
        except OSError as e:
            # Handle the case in which Python was compiled
            # in a system with the syscall but without support
            # in the kernel.
            if e.errno != errno.ENOSYS:
                raise
            self.skipTest(e)
        else:
            # The number of copied bytes can be less than
            # the number of bytes originally requested.
            self.assertIn(i, range(0, 6));

            with open(TESTFN2, 'rb') as in_file:
                self.assertEqual(in_file.read(), data[:i])

    @unittest.skipUnless(hasattr(os, 'copy_file_range'), 'test needs os.copy_file_range()')
    def test_copy_file_range_offset(self):
        TESTFN4 = os_helper.TESTFN + ".4"
        data = b'0123456789'
        bytes_to_copy = 6
        in_skip = 3
        out_seek = 5

        create_file(os_helper.TESTFN, data)
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)

        in_file = open(os_helper.TESTFN, 'rb')
        self.addCleanup(in_file.close)
        in_fd = in_file.fileno()

        out_file = open(TESTFN4, 'w+b')
        self.addCleanup(os_helper.unlink, TESTFN4)
        self.addCleanup(out_file.close)
        out_fd = out_file.fileno()

        try:
            i = os.copy_file_range(in_fd, out_fd, bytes_to_copy,
                                   offset_src=in_skip,
                                   offset_dst=out_seek)
        except OSError as e:
            # Handle the case in which Python was compiled
            # in a system with the syscall but without support
            # in the kernel.
            if e.errno != errno.ENOSYS:
                raise
            self.skipTest(e)
        else:
            # The number of copied bytes can be less than
            # the number of bytes originally requested.
            self.assertIn(i, range(0, bytes_to_copy+1));

            with open(TESTFN4, 'rb') as in_file:
                read = in_file.read()
            # seeked bytes (5) are zero'ed
            self.assertEqual(read[:out_seek], b'\x00'*out_seek)
            # 012 are skipped (in_skip)
            # 345678 are copied in the file (in_skip + bytes_to_copy)
            self.assertEqual(read[out_seek:],
                             data[in_skip:in_skip+i])

    @unittest.skipUnless(hasattr(os, 'splice'), 'test needs os.splice()')
    def test_splice_invalid_values(self):
        with self.assertRaises(ValueError):
            os.splice(0, 1, -10)

    @unittest.skipUnless(hasattr(os, 'splice'), 'test needs os.splice()')
    @requires_splice_pipe
    def test_splice(self):
        TESTFN2 = os_helper.TESTFN + ".3"
        data = b'0123456789'

        create_file(os_helper.TESTFN, data)
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)

        in_file = open(os_helper.TESTFN, 'rb')
        self.addCleanup(in_file.close)
        in_fd = in_file.fileno()

        read_fd, write_fd = os.pipe()
        self.addCleanup(lambda: os.close(read_fd))
        self.addCleanup(lambda: os.close(write_fd))

        try:
            i = os.splice(in_fd, write_fd, 5)
        except OSError as e:
            # Handle the case in which Python was compiled
            # in a system with the syscall but without support
            # in the kernel.
            if e.errno != errno.ENOSYS:
                raise
            self.skipTest(e)
        else:
            # The number of copied bytes can be less than
            # the number of bytes originally requested.
            self.assertIn(i, range(0, 6));

            self.assertEqual(os.read(read_fd, 100), data[:i])

    @unittest.skipUnless(hasattr(os, 'splice'), 'test needs os.splice()')
    @requires_splice_pipe
    def test_splice_offset_in(self):
        TESTFN4 = os_helper.TESTFN + ".4"
        data = b'0123456789'
        bytes_to_copy = 6
        in_skip = 3

        create_file(os_helper.TESTFN, data)
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)

        in_file = open(os_helper.TESTFN, 'rb')
        self.addCleanup(in_file.close)
        in_fd = in_file.fileno()

        read_fd, write_fd = os.pipe()
        self.addCleanup(lambda: os.close(read_fd))
        self.addCleanup(lambda: os.close(write_fd))

        try:
            i = os.splice(in_fd, write_fd, bytes_to_copy, offset_src=in_skip)
        except OSError as e:
            # Handle the case in which Python was compiled
            # in a system with the syscall but without support
            # in the kernel.
            if e.errno != errno.ENOSYS:
                raise
            self.skipTest(e)
        else:
            # The number of copied bytes can be less than
            # the number of bytes originally requested.
            self.assertIn(i, range(0, bytes_to_copy+1));

            read = os.read(read_fd, 100)
            # 012 are skipped (in_skip)
            # 345678 are copied in the file (in_skip + bytes_to_copy)
            self.assertEqual(read, data[in_skip:in_skip+i])

    @unittest.skipUnless(hasattr(os, 'splice'), 'test needs os.splice()')
    @requires_splice_pipe
    def test_splice_offset_out(self):
        TESTFN4 = os_helper.TESTFN + ".4"
        data = b'0123456789'
        bytes_to_copy = 6
        out_seek = 3

        create_file(os_helper.TESTFN, data)
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)

        read_fd, write_fd = os.pipe()
        self.addCleanup(lambda: os.close(read_fd))
        self.addCleanup(lambda: os.close(write_fd))
        os.write(write_fd, data)

        out_file = open(TESTFN4, 'w+b')
        self.addCleanup(os_helper.unlink, TESTFN4)
        self.addCleanup(out_file.close)
        out_fd = out_file.fileno()

        try:
            i = os.splice(read_fd, out_fd, bytes_to_copy, offset_dst=out_seek)
        except OSError as e:
            # Handle the case in which Python was compiled
            # in a system with the syscall but without support
            # in the kernel.
            if e.errno != errno.ENOSYS:
                raise
            self.skipTest(e)
        else:
            # The number of copied bytes can be less than
            # the number of bytes originally requested.
            self.assertIn(i, range(0, bytes_to_copy+1));

            with open(TESTFN4, 'rb') as in_file:
                read = in_file.read()
            # seeked bytes (5) are zero'ed
            self.assertEqual(read[:out_seek], b'\x00'*out_seek)
            # 012 are skipped (in_skip)
            # 345678 are copied in the file (in_skip + bytes_to_copy)
            self.assertEqual(read[out_seek:], data[:i])


class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        create_file(os_helper.TESTFN, b'')

    @unittest.skipUnless(hasattr(posix, 'ftruncate'),
                         'test needs posix.ftruncate()')
    def test_ftruncate(self):
        fp = open(os_helper.TESTFN, 'w+')
        try:
            # we need to have some data to truncate
            fp.write('test')
            fp.flush()
            posix.ftruncate(fp.fileno(), 0)
        finally:
            fp.close()

    @unittest.skipUnless(hasattr(posix, 'truncate'), "test needs posix.truncate()")
    def test_truncate(self):
        with open(os_helper.TESTFN, 'w') as fp:
            fp.write('test')
            fp.flush()
        posix.truncate(os_helper.TESTFN, 0)

    @unittest.skipUnless(hasattr(posix, 'pread'), "test needs posix.pread()")
    def test_pread(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            os.write(fd, b'test')
            os.lseek(fd, 0, os.SEEK_SET)
            self.assertEqual(b'es', posix.pread(fd, 2, 1))
            # the first pread() shouldn't disturb the file offset
            self.assertEqual(b'te', posix.read(fd, 2))
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'preadv'), "test needs posix.preadv()")
    def test_preadv(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            os.write(fd, b'test1tt2t3t5t6t6t8')
            buf = [bytearray(i) for i in [5, 3, 2]]
            self.assertEqual(posix.preadv(fd, buf, 3), 10)
            self.assertEqual([b't1tt2', b't3t', b'5t'], list(buf))
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'preadv'), "test needs posix.preadv()")
    @unittest.skipUnless(hasattr(posix, 'RWF_HIPRI'), "test needs posix.RWF_HIPRI")
    def test_preadv_flags(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            os.write(fd, b'test1tt2t3t5t6t6t8')
            buf = [bytearray(i) for i in [5, 3, 2]]
            self.assertEqual(posix.preadv(fd, buf, 3, os.RWF_HIPRI), 10)
            self.assertEqual([b't1tt2', b't3t', b'5t'], list(buf))
        except NotImplementedError:
            self.skipTest("preadv2 not available")
        except OSError as inst:
            # Is possible that the macro RWF_HIPRI was defined at compilation time
            # but the option is not supported by the kernel or the runtime libc shared
            # library.
            if inst.errno in {errno.EINVAL, errno.ENOTSUP}:
                raise unittest.SkipTest("RWF_HIPRI is not supported by the current system")
            else:
                raise
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'preadv'), "test needs posix.preadv()")
    @requires_32b
    def test_preadv_overflow_32bits(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            buf = [bytearray(2**16)] * 2**15
            with self.assertRaises(OSError) as cm:
                os.preadv(fd, buf, 0)
            self.assertEqual(cm.exception.errno, errno.EINVAL)
            self.assertEqual(bytes(buf[0]), b'\0'* 2**16)
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'pwrite'), "test needs posix.pwrite()")
    def test_pwrite(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            os.write(fd, b'test')
            os.lseek(fd, 0, os.SEEK_SET)
            posix.pwrite(fd, b'xx', 1)
            self.assertEqual(b'txxt', posix.read(fd, 4))
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'pwritev'), "test needs posix.pwritev()")
    def test_pwritev(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            os.write(fd, b"xx")
            os.lseek(fd, 0, os.SEEK_SET)
            n = os.pwritev(fd, [b'test1', b'tt2', b't3'], 2)
            self.assertEqual(n, 10)

            os.lseek(fd, 0, os.SEEK_SET)
            self.assertEqual(b'xxtest1tt2t3', posix.read(fd, 100))
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'pwritev'), "test needs posix.pwritev()")
    @unittest.skipUnless(hasattr(posix, 'os.RWF_SYNC'), "test needs os.RWF_SYNC")
    def test_pwritev_flags(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            os.write(fd,b"xx")
            os.lseek(fd, 0, os.SEEK_SET)
            n = os.pwritev(fd, [b'test1', b'tt2', b't3'], 2, os.RWF_SYNC)
            self.assertEqual(n, 10)

            os.lseek(fd, 0, os.SEEK_SET)
            self.assertEqual(b'xxtest1tt2', posix.read(fd, 100))
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'pwritev'), "test needs posix.pwritev()")
    @requires_32b
    def test_pwritev_overflow_32bits(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            with self.assertRaises(OSError) as cm:
                os.pwritev(fd, [b"x" * 2**16] * 2**15, 0)
            self.assertEqual(cm.exception.errno, errno.EINVAL)
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'posix_fallocate'),
        "test needs posix.posix_fallocate()")
    def test_posix_fallocate(self):
        fd = os.open(os_helper.TESTFN, os.O_WRONLY | os.O_CREAT)
        try:
            posix.posix_fallocate(fd, 0, 10)
        except OSError as inst:
            # issue10812, ZFS doesn't appear to support posix_fallocate,
            # so skip Solaris-based since they are likely to have ZFS.
            # issue33655: Also ignore EINVAL on *BSD since ZFS is also
            # often used there.
            if inst.errno == errno.EINVAL and sys.platform.startswith(
                ('sunos', 'freebsd', 'openbsd', 'gnukfreebsd')):
                raise unittest.SkipTest("test may fail on ZFS filesystems")
            elif inst.errno == errno.EOPNOTSUPP and sys.platform.startswith("netbsd"):
                raise unittest.SkipTest("test may fail on FFS filesystems")
            else:
                raise
        finally:
            os.close(fd)

    # issue31106 - posix_fallocate() does not set error in errno.
    @unittest.skipUnless(hasattr(posix, 'posix_fallocate'),
        "test needs posix.posix_fallocate()")
    def test_posix_fallocate_errno(self):
        try:
            posix.posix_fallocate(-42, 0, 10)
        except OSError as inst:
            if inst.errno != errno.EBADF:
                raise

    @unittest.skipUnless(hasattr(posix, 'posix_fadvise'),
        "test needs posix.posix_fadvise()")
    def test_posix_fadvise(self):
        fd = os.open(os_helper.TESTFN, os.O_RDONLY)
        try:
            posix.posix_fadvise(fd, 0, 0, posix.POSIX_FADV_WILLNEED)
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'posix_fadvise'),
        "test needs posix.posix_fadvise()")
    def test_posix_fadvise_errno(self):
        try:
            posix.posix_fadvise(-42, 0, 0, posix.POSIX_FADV_WILLNEED)
        except OSError as inst:
            if inst.errno != errno.EBADF:
                raise

    @unittest.skipUnless(hasattr(posix, 'writev'), "test needs posix.writev()")
    def test_writev(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            n = os.writev(fd, (b'test1', b'tt2', b't3'))
            self.assertEqual(n, 10)

            os.lseek(fd, 0, os.SEEK_SET)
            self.assertEqual(b'test1tt2t3', posix.read(fd, 10))

            # Issue #20113: empty list of buffers should not crash
            try:
                size = posix.writev(fd, [])
            except OSError:
                # writev(fd, []) raises OSError(22, "Invalid argument")
                # on OpenIndiana
                pass
            else:
                self.assertEqual(size, 0)
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'writev'), "test needs posix.writev()")
    @requires_32b
    def test_writev_overflow_32bits(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            with self.assertRaises(OSError) as cm:
                os.writev(fd, [b"x" * 2**16] * 2**15)
            self.assertEqual(cm.exception.errno, errno.EINVAL)
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'readv'), "test needs posix.readv()")
    def test_readv(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            os.write(fd, b'test1tt2t3')
            os.lseek(fd, 0, os.SEEK_SET)
            buf = [bytearray(i) for i in [5, 3, 2]]
            self.assertEqual(posix.readv(fd, buf), 10)
            self.assertEqual([b'test1', b'tt2', b't3'], [bytes(i) for i in buf])

            # Issue #20113: empty list of buffers should not crash
            try:
                size = posix.readv(fd, [])
            except OSError:
                # readv(fd, []) raises OSError(22, "Invalid argument")
                # on OpenIndiana
                pass
            else:
                self.assertEqual(size, 0)
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'readv'), "test needs posix.readv()")
    @requires_32b
    def test_readv_overflow_32bits(self):
        fd = os.open(os_helper.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            buf = [bytearray(2**16)] * 2**15
            with self.assertRaises(OSError) as cm:
                os.readv(fd, buf)
            self.assertEqual(cm.exception.errno, errno.EINVAL)
            self.assertEqual(bytes(buf[0]), b'\0'* 2**16)
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'dup'),
                         'test needs posix.dup()')
    @unittest.skipIf(support.is_wasi, "WASI does not have dup()")
    def test_dup(self):
        fp = open(os_helper.TESTFN)
        try:
            fd = posix.dup(fp.fileno())
            self.assertIsInstance(fd, int)
            os.close(fd)
        finally:
            fp.close()

    @unittest.skipUnless(hasattr(posix, 'dup2'),
                         'test needs posix.dup2()')
    @unittest.skipIf(support.is_wasi, "WASI does not have dup2()")
    def test_dup2(self):
        fp1 = open(os_helper.TESTFN)
        fp2 = open(os_helper.TESTFN)
        try:
            posix.dup2(fp1.fileno(), fp2.fileno())
        finally:
            fp1.close()
            fp2.close()

    @unittest.skipUnless(hasattr(os, 'O_CLOEXEC'), "needs os.O_CLOEXEC")
    @support.requires_linux_version(2, 6, 23)
    @support.requires_subprocess()
    def test_oscloexec(self):
        fd = os.open(os_helper.TESTFN, os.O_RDONLY|os.O_CLOEXEC)
        self.addCleanup(os.close, fd)
        self.assertFalse(os.get_inheritable(fd))

    @unittest.skipUnless(hasattr(posix, 'lockf'), "test needs posix.lockf()")
    def test_lockf(self):
        fd = os.open(os_helper.TESTFN, os.O_WRONLY | os.O_CREAT)
        try:
            os.write(fd, b'test')
            os.lseek(fd, 0, os.SEEK_SET)
            posix.lockf(fd, posix.F_LOCK, 4)
            # section is locked
            posix.lockf(fd, posix.F_ULOCK, 4)
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'O_EXLOCK'),
                         'test needs posix.O_EXLOCK')
    def test_osexlock(self):
        fd = os.open(os_helper.TESTFN,
                     os.O_WRONLY|os.O_EXLOCK|os.O_CREAT)
        self.assertRaises(OSError, os.open, os_helper.TESTFN,
                          os.O_WRONLY|os.O_EXLOCK|os.O_NONBLOCK)
        os.close(fd)

        if hasattr(posix, "O_SHLOCK"):
            fd = os.open(os_helper.TESTFN,
                         os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
            self.assertRaises(OSError, os.open, os_helper.TESTFN,
                              os.O_WRONLY|os.O_EXLOCK|os.O_NONBLOCK)
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'O_SHLOCK'),
                         'test needs posix.O_SHLOCK')
    def test_osshlock(self):
        fd1 = os.open(os_helper.TESTFN,
                     os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
        fd2 = os.open(os_helper.TESTFN,
                      os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
        os.close(fd2)
        os.close(fd1)

        if hasattr(posix, "O_EXLOCK"):
            fd = os.open(os_helper.TESTFN,
                         os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
            self.assertRaises(OSError, os.open, os_helper.TESTFN,
                              os.O_RDONLY|os.O_EXLOCK|os.O_NONBLOCK)
            os.close(fd)


# FD inheritance check is only useful for systems with process support.
@support.requires_subprocess()
class FDInheritanceTests(unittest.TestCase):
    def test_get_set_inheritable(self):
        fd = os.open(__file__, os.O_RDONLY)
        self.addCleanup(os.close, fd)
        self.assertEqual(os.get_inheritable(fd), False)

        os.set_inheritable(fd, True)
        self.assertEqual(os.get_inheritable(fd), True)

    @unittest.skipIf(fcntl is None, "need fcntl")
    def test_get_inheritable_cloexec(self):
        fd = os.open(__file__, os.O_RDONLY)
        self.addCleanup(os.close, fd)
        self.assertEqual(os.get_inheritable(fd), False)

        # clear FD_CLOEXEC flag
        flags = fcntl.fcntl(fd, fcntl.F_GETFD)
        flags &= ~fcntl.FD_CLOEXEC
        fcntl.fcntl(fd, fcntl.F_SETFD, flags)

        self.assertEqual(os.get_inheritable(fd), True)

    @unittest.skipIf(fcntl is None, "need fcntl")
    def test_set_inheritable_cloexec(self):
        fd = os.open(__file__, os.O_RDONLY)
        self.addCleanup(os.close, fd)
        self.assertEqual(fcntl.fcntl(fd, fcntl.F_GETFD) & fcntl.FD_CLOEXEC,
                         fcntl.FD_CLOEXEC)

        os.set_inheritable(fd, True)
        self.assertEqual(fcntl.fcntl(fd, fcntl.F_GETFD) & fcntl.FD_CLOEXEC,
                         0)

    @unittest.skipUnless(hasattr(os, 'O_PATH'), "need os.O_PATH")
    def test_get_set_inheritable_o_path(self):
        fd = os.open(__file__, os.O_PATH)
        self.addCleanup(os.close, fd)
        self.assertEqual(os.get_inheritable(fd), False)

        os.set_inheritable(fd, True)
        self.assertEqual(os.get_inheritable(fd), True)

        os.set_inheritable(fd, False)
        self.assertEqual(os.get_inheritable(fd), False)

    def test_get_set_inheritable_badf(self):
        fd = os_helper.make_bad_fd()

        with self.assertRaises(OSError) as ctx:
            os.get_inheritable(fd)
        self.assertEqual(ctx.exception.errno, errno.EBADF)

        with self.assertRaises(OSError) as ctx:
            os.set_inheritable(fd, True)
        self.assertEqual(ctx.exception.errno, errno.EBADF)

        with self.assertRaises(OSError) as ctx:
            os.set_inheritable(fd, False)
        self.assertEqual(ctx.exception.errno, errno.EBADF)

    def test_open(self):
        fd = os.open(__file__, os.O_RDONLY)
        self.addCleanup(os.close, fd)
        self.assertEqual(os.get_inheritable(fd), False)

    @unittest.skipUnless(hasattr(os, 'pipe'), "need os.pipe()")
    def test_pipe(self):
        rfd, wfd = os.pipe()
        self.addCleanup(os.close, rfd)
        self.addCleanup(os.close, wfd)
        self.assertEqual(os.get_inheritable(rfd), False)
        self.assertEqual(os.get_inheritable(wfd), False)

    def test_dup(self):
        fd1 = os.open(__file__, os.O_RDONLY)
        self.addCleanup(os.close, fd1)

        fd2 = os.dup(fd1)
        self.addCleanup(os.close, fd2)
        self.assertEqual(os.get_inheritable(fd2), False)

    def test_dup_standard_stream(self):
        fd = os.dup(1)
        self.addCleanup(os.close, fd)
        self.assertGreater(fd, 0)

    @unittest.skipUnless(sys.platform == 'win32', 'win32-specific test')
    def test_dup_nul(self):
        # os.dup() was creating inheritable fds for character files.
        fd1 = os.open('NUL', os.O_RDONLY)
        self.addCleanup(os.close, fd1)
        fd2 = os.dup(fd1)
        self.addCleanup(os.close, fd2)
        self.assertFalse(os.get_inheritable(fd2))

    @unittest.skipUnless(hasattr(os, 'dup2'), "need os.dup2()")
    def test_dup2(self):
        fd = os.open(__file__, os.O_RDONLY)
        self.addCleanup(os.close, fd)

        # inheritable by default
        fd2 = os.open(__file__, os.O_RDONLY)
        self.addCleanup(os.close, fd2)
        self.assertEqual(os.dup2(fd, fd2), fd2)
        self.assertTrue(os.get_inheritable(fd2))

        # force non-inheritable
        fd3 = os.open(__file__, os.O_RDONLY)
        self.addCleanup(os.close, fd3)
        self.assertEqual(os.dup2(fd, fd3, inheritable=False), fd3)
        self.assertFalse(os.get_inheritable(fd3))

    @unittest.skipUnless(hasattr(os, 'SEEK_HOLE'),
                         "test needs an OS that reports file holes")
    def test_fs_holes(self):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)

        # Even if the filesystem doesn't report holes,
        # if the OS supports it the SEEK_* constants
        # will be defined and will have a consistent
        # behaviour:
        # os.SEEK_DATA = current position
        # os.SEEK_HOLE = end of file position
        with open(os_helper.TESTFN, 'w+b') as fp:
            fp.write(b"hello")
            fp.flush()
            size = fp.tell()
            fno = fp.fileno()
            try :
                for i in range(size):
                    self.assertEqual(i, os.lseek(fno, i, os.SEEK_DATA))
                    self.assertLessEqual(size, os.lseek(fno, i, os.SEEK_HOLE))
                self.assertRaises(OSError, os.lseek, fno, size, os.SEEK_DATA)
                self.assertRaises(OSError, os.lseek, fno, size, os.SEEK_HOLE)
            except OSError :
                # Some OSs claim to support SEEK_HOLE/SEEK_DATA
                # but it is not true.
                # For instance:
                # http://lists.freebsd.org/pipermail/freebsd-amd64/2012-January/014332.html
                raise unittest.SkipTest("OSError raised!")


if __name__ == "__main__":
    unittest.main()
