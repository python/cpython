# As a test suite for the os module, this is woefully inadequate, but this
# does add tests for a few functions which have been determined to be more
# portable than they had been thought to be.

import asyncio
import codecs
import contextlib
import decimal
import errno
import fnmatch
import fractions
import itertools
import locale
import os
import pickle
import select
import selectors
import shutil
import signal
import socket
import stat
import struct
import subprocess
import sys
import sysconfig
import tempfile
import textwrap
import time
import types
import unittest
import uuid
import warnings
from test import support
from test.support import import_helper
from test.support import os_helper
from test.support import socket_helper
from test.support import infinite_recursion
from test.support import warnings_helper
from platform import win32_is_iot

try:
    import resource
except ImportError:
    resource = None
try:
    import fcntl
except ImportError:
    fcntl = None
try:
    import _winapi
except ImportError:
    _winapi = None
try:
    import pwd
    all_users = [u.pw_uid for u in pwd.getpwall()]
except (ImportError, AttributeError):
    all_users = []
try:
    import _testcapi
    from _testcapi import INT_MAX, PY_SSIZE_T_MAX
except ImportError:
    _testcapi = None
    INT_MAX = PY_SSIZE_T_MAX = sys.maxsize

try:
    import mmap
except ImportError:
    mmap = None

from test.support.script_helper import assert_python_ok
from test.support import unix_shell
from test.support.os_helper import FakePath


root_in_posix = False
if hasattr(os, 'geteuid'):
    root_in_posix = (os.geteuid() == 0)

# Detect whether we're on a Linux system that uses the (now outdated
# and unmaintained) linuxthreads threading library.  There's an issue
# when combining linuxthreads with a failed execv call: see
# http://bugs.python.org/issue4970.
if hasattr(sys, 'thread_info') and sys.thread_info.version:
    USING_LINUXTHREADS = sys.thread_info.version.startswith("linuxthreads")
else:
    USING_LINUXTHREADS = False

# Issue #14110: Some tests fail on FreeBSD if the user is in the wheel group.
HAVE_WHEEL_GROUP = sys.platform.startswith('freebsd') and os.getgid() == 0


def requires_os_func(name):
    return unittest.skipUnless(hasattr(os, name), 'requires os.%s' % name)


def create_file(filename, content=b'content'):
    with open(filename, "xb", 0) as fp:
        fp.write(content)


# bpo-41625: On AIX, splice() only works with a socket, not with a pipe.
requires_splice_pipe = unittest.skipIf(sys.platform.startswith("aix"),
                                       'on AIX, splice() only accepts sockets')


def tearDownModule():
    asyncio.events._set_event_loop_policy(None)


class MiscTests(unittest.TestCase):
    def test_getcwd(self):
        cwd = os.getcwd()
        self.assertIsInstance(cwd, str)

    def test_getcwd_long_path(self):
        # bpo-37412: On Linux, PATH_MAX is usually around 4096 bytes. On
        # Windows, MAX_PATH is defined as 260 characters, but Windows supports
        # longer path if longer paths support is enabled. Internally, the os
        # module uses MAXPATHLEN which is at least 1024.
        #
        # Use a directory name of 200 characters to fit into Windows MAX_PATH
        # limit.
        #
        # On Windows, the test can stop when trying to create a path longer
        # than MAX_PATH if long paths support is disabled:
        # see RtlAreLongPathsEnabled().
        min_len = 2000   # characters
        # On VxWorks, PATH_MAX is defined as 1024 bytes. Creating a path
        # longer than PATH_MAX will fail.
        if sys.platform == 'vxworks':
            min_len = 1000
        dirlen = 200     # characters
        dirname = 'python_test_dir_'
        dirname = dirname + ('a' * (dirlen - len(dirname)))

        with tempfile.TemporaryDirectory() as tmpdir:
            with os_helper.change_cwd(tmpdir) as path:
                expected = path

                while True:
                    cwd = os.getcwd()
                    self.assertEqual(cwd, expected)

                    need = min_len - (len(cwd) + len(os.path.sep))
                    if need <= 0:
                        break
                    if len(dirname) > need and need > 0:
                        dirname = dirname[:need]

                    path = os.path.join(path, dirname)
                    try:
                        os.mkdir(path)
                        # On Windows, chdir() can fail
                        # even if mkdir() succeeded
                        os.chdir(path)
                    except FileNotFoundError:
                        # On Windows, catch ERROR_PATH_NOT_FOUND (3) and
                        # ERROR_FILENAME_EXCED_RANGE (206) errors
                        # ("The filename or extension is too long")
                        break
                    except OSError as exc:
                        if exc.errno == errno.ENAMETOOLONG:
                            break
                        else:
                            raise

                    expected = path

                if support.verbose:
                    print(f"Tested current directory length: {len(cwd)}")

    def test_getcwdb(self):
        cwd = os.getcwdb()
        self.assertIsInstance(cwd, bytes)
        self.assertEqual(os.fsdecode(cwd), os.getcwd())


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

    def write_windows_console(self, *args):
        retcode = subprocess.call(args,
            # use a new console to not flood the test output
            creationflags=subprocess.CREATE_NEW_CONSOLE,
            # use a shell to hide the console window (SW_HIDE)
            shell=True)
        self.assertEqual(retcode, 0)

    @unittest.skipUnless(sys.platform == 'win32',
                         'test specific to the Windows console')
    def test_write_windows_console(self):
        # Issue #11395: the Windows console returns an error (12: not enough
        # space error) on writing into stdout if stdout mode is binary and the
        # length is greater than 66,000 bytes (or less, depending on heap
        # usage).
        code = "print('x' * 100000)"
        self.write_windows_console(sys.executable, "-c", code)
        self.write_windows_console(sys.executable, "-u", "-c", code)

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


# Test attributes on return values from os.*stat* family.
class StatAttributeTests(unittest.TestCase):
    def setUp(self):
        self.fname = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, self.fname)
        create_file(self.fname, b"ABC")

    def check_stat_attributes(self, fname):
        result = os.stat(fname)

        # Make sure direct access works
        self.assertEqual(result[stat.ST_SIZE], 3)
        self.assertEqual(result.st_size, 3)

        # Make sure all the attributes are there
        members = dir(result)
        for name in dir(stat):
            if name[:3] == 'ST_':
                attr = name.lower()
                if name.endswith("TIME"):
                    def trunc(x): return int(x)
                else:
                    def trunc(x): return x
                self.assertEqual(trunc(getattr(result, attr)),
                                  result[getattr(stat, name)])
                self.assertIn(attr, members)

        # Make sure that the st_?time and st_?time_ns fields roughly agree
        # (they should always agree up to around tens-of-microseconds)
        for name in 'st_atime st_mtime st_ctime'.split():
            floaty = int(getattr(result, name) * 100000)
            nanosecondy = getattr(result, name + "_ns") // 10000
            self.assertAlmostEqual(floaty, nanosecondy, delta=2)

        # Ensure both birthtime and birthtime_ns roughly agree, if present
        try:
            floaty = int(result.st_birthtime * 100000)
            nanosecondy = result.st_birthtime_ns // 10000
        except AttributeError:
            pass
        else:
            self.assertAlmostEqual(floaty, nanosecondy, delta=2)

        try:
            result[200]
            self.fail("No exception raised")
        except IndexError:
            pass

        # Make sure that assignment fails
        try:
            result.st_mode = 1
            self.fail("No exception raised")
        except AttributeError:
            pass

        try:
            result.st_rdev = 1
            self.fail("No exception raised")
        except (AttributeError, TypeError):
            pass

        try:
            result.parrot = 1
            self.fail("No exception raised")
        except AttributeError:
            pass

        # Use the stat_result constructor with a too-short tuple.
        try:
            result2 = os.stat_result((10,))
            self.fail("No exception raised")
        except TypeError:
            pass

        # Use the constructor with a too-long tuple.
        try:
            result2 = os.stat_result((0,1,2,3,4,5,6,7,8,9,10,11,12,13,14))
        except TypeError:
            pass

    def test_stat_attributes(self):
        self.check_stat_attributes(self.fname)

    def test_stat_attributes_bytes(self):
        try:
            fname = self.fname.encode(sys.getfilesystemencoding())
        except UnicodeEncodeError:
            self.skipTest("cannot encode %a for the filesystem" % self.fname)
        self.check_stat_attributes(fname)

    def test_stat_result_pickle(self):
        result = os.stat(self.fname)
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(f'protocol {proto}'):
                p = pickle.dumps(result, proto)
                self.assertIn(b'stat_result', p)
                if proto < 4:
                    self.assertIn(b'cos\nstat_result\n', p)
                unpickled = pickle.loads(p)
                self.assertEqual(result, unpickled)

    @unittest.skipUnless(hasattr(os, 'statvfs'), 'test needs os.statvfs()')
    def test_statvfs_attributes(self):
        result = os.statvfs(self.fname)

        # Make sure direct access works
        self.assertEqual(result.f_bfree, result[3])

        # Make sure all the attributes are there.
        members = ('bsize', 'frsize', 'blocks', 'bfree', 'bavail', 'files',
                    'ffree', 'favail', 'flag', 'namemax')
        for value, member in enumerate(members):
            self.assertEqual(getattr(result, 'f_' + member), result[value])

        self.assertTrue(isinstance(result.f_fsid, int))

        # Test that the size of the tuple doesn't change
        self.assertEqual(len(result), 10)

        # Make sure that assignment really fails
        try:
            result.f_bfree = 1
            self.fail("No exception raised")
        except AttributeError:
            pass

        try:
            result.parrot = 1
            self.fail("No exception raised")
        except AttributeError:
            pass

        # Use the constructor with a too-short tuple.
        try:
            result2 = os.statvfs_result((10,))
            self.fail("No exception raised")
        except TypeError:
            pass

        # Use the constructor with a too-long tuple.
        try:
            result2 = os.statvfs_result((0,1,2,3,4,5,6,7,8,9,10,11,12,13,14))
        except TypeError:
            pass

    @unittest.skipUnless(hasattr(os, 'statvfs'),
                         "need os.statvfs()")
    def test_statvfs_result_pickle(self):
        result = os.statvfs(self.fname)

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            p = pickle.dumps(result, proto)
            self.assertIn(b'statvfs_result', p)
            if proto < 4:
                self.assertIn(b'cos\nstatvfs_result\n', p)
            unpickled = pickle.loads(p)
            self.assertEqual(result, unpickled)

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
    def test_1686475(self):
        # Verify that an open file can be stat'ed
        try:
            os.stat(r"c:\pagefile.sys")
        except FileNotFoundError:
            self.skipTest(r'c:\pagefile.sys does not exist')
        except OSError as e:
            self.fail("Could not stat pagefile.sys")

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_15261(self):
        # Verify that stat'ing a closed fd does not cause crash
        r, w = os.pipe()
        try:
            os.stat(r)          # should not raise error
        finally:
            os.close(r)
            os.close(w)
        with self.assertRaises(OSError) as ctx:
            os.stat(r)
        self.assertEqual(ctx.exception.errno, errno.EBADF)

    def check_file_attributes(self, result):
        self.assertHasAttr(result, 'st_file_attributes')
        self.assertTrue(isinstance(result.st_file_attributes, int))
        self.assertTrue(0 <= result.st_file_attributes <= 0xFFFFFFFF)

    @unittest.skipUnless(sys.platform == "win32",
                         "st_file_attributes is Win32 specific")
    def test_file_attributes(self):
        # test file st_file_attributes (FILE_ATTRIBUTE_DIRECTORY not set)
        result = os.stat(self.fname)
        self.check_file_attributes(result)
        self.assertEqual(
            result.st_file_attributes & stat.FILE_ATTRIBUTE_DIRECTORY,
            0)

        # test directory st_file_attributes (FILE_ATTRIBUTE_DIRECTORY set)
        dirname = os_helper.TESTFN + "dir"
        os.mkdir(dirname)
        self.addCleanup(os.rmdir, dirname)

        result = os.stat(dirname)
        self.check_file_attributes(result)
        self.assertEqual(
            result.st_file_attributes & stat.FILE_ATTRIBUTE_DIRECTORY,
            stat.FILE_ATTRIBUTE_DIRECTORY)

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
    def test_access_denied(self):
        # Default to FindFirstFile WIN32_FIND_DATA when access is
        # denied. See issue 28075.
        # os.environ['TEMP'] should be located on a volume that
        # supports file ACLs.
        fname = os.path.join(os.environ['TEMP'], self.fname + "_access")
        self.addCleanup(os_helper.unlink, fname)
        create_file(fname, b'ABC')
        # Deny the right to [S]YNCHRONIZE on the file to
        # force CreateFile to fail with ERROR_ACCESS_DENIED.
        DETACHED_PROCESS = 8
        subprocess.check_call(
            # bpo-30584: Use security identifier *S-1-5-32-545 instead
            # of localized "Users" to not depend on the locale.
            ['icacls.exe', fname, '/deny', '*S-1-5-32-545:(S)'],
            creationflags=DETACHED_PROCESS
        )
        result = os.stat(fname)
        self.assertNotEqual(result.st_size, 0)
        self.assertTrue(os.path.isfile(fname))

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
    def test_stat_block_device(self):
        # bpo-38030: os.stat fails for block devices
        # Test a filename like "//./C:"
        fname = "//./" + os.path.splitdrive(os.getcwd())[0]
        result = os.stat(fname)
        self.assertEqual(result.st_mode, stat.S_IFBLK)


class UtimeTests(unittest.TestCase):
    def setUp(self):
        self.dirname = os_helper.TESTFN
        self.fname = os.path.join(self.dirname, "f1")

        self.addCleanup(os_helper.rmtree, self.dirname)
        os.mkdir(self.dirname)
        create_file(self.fname)

    def support_subsecond(self, filename):
        # Heuristic to check if the filesystem supports timestamp with
        # subsecond resolution: check if float and int timestamps are different
        st = os.stat(filename)
        return ((st.st_atime != st[7])
                or (st.st_mtime != st[8])
                or (st.st_ctime != st[9]))

    def _test_utime(self, set_time, filename=None):
        if not filename:
            filename = self.fname

        support_subsecond = self.support_subsecond(filename)
        if support_subsecond:
            # Timestamp with a resolution of 1 microsecond (10^-6).
            #
            # The resolution of the C internal function used by os.utime()
            # depends on the platform: 1 sec, 1 us, 1 ns. Writing a portable
            # test with a resolution of 1 ns requires more work:
            # see the issue #15745.
            atime_ns = 1002003000   # 1.002003 seconds
            mtime_ns = 4005006000   # 4.005006 seconds
        else:
            # use a resolution of 1 second
            atime_ns = 5 * 10**9
            mtime_ns = 8 * 10**9

        set_time(filename, (atime_ns, mtime_ns))
        st = os.stat(filename)

        if support.is_emscripten:
            # Emscripten timestamps are roundtripped through a 53 bit integer of
            # nanoseconds. If we want to represent ~50 years which is an 11
            # digits number of seconds:
            # 2*log10(60) + log10(24) + log10(365) + log10(60) + log10(50)
            # is about 11. Because 53 * log10(2) is about 16, we only have 5
            # digits worth of sub-second precision.
            # Some day it would be good to fix this upstream.
            delta=1e-5
            self.assertAlmostEqual(st.st_atime, atime_ns * 1e-9, delta=1e-5)
            self.assertAlmostEqual(st.st_mtime, mtime_ns * 1e-9, delta=1e-5)
            self.assertAlmostEqual(st.st_atime_ns, atime_ns, delta=1e9 * 1e-5)
            self.assertAlmostEqual(st.st_mtime_ns, mtime_ns, delta=1e9 * 1e-5)
        else:
            if support_subsecond:
                self.assertAlmostEqual(st.st_atime, atime_ns * 1e-9, delta=1e-6)
                self.assertAlmostEqual(st.st_mtime, mtime_ns * 1e-9, delta=1e-6)
            else:
                self.assertEqual(st.st_atime, atime_ns * 1e-9)
                self.assertEqual(st.st_mtime, mtime_ns * 1e-9)
            self.assertEqual(st.st_atime_ns, atime_ns)
            self.assertEqual(st.st_mtime_ns, mtime_ns)

    def test_utime(self):
        def set_time(filename, ns):
            # test the ns keyword parameter
            os.utime(filename, ns=ns)
        self._test_utime(set_time)

    @staticmethod
    def ns_to_sec(ns):
        # Convert a number of nanosecond (int) to a number of seconds (float).
        # Round towards infinity by adding 0.5 nanosecond to avoid rounding
        # issue, os.utime() rounds towards minus infinity.
        return (ns * 1e-9) + 0.5e-9

    def test_utime_by_indexed(self):
        # pass times as floating-point seconds as the second indexed parameter
        def set_time(filename, ns):
            atime_ns, mtime_ns = ns
            atime = self.ns_to_sec(atime_ns)
            mtime = self.ns_to_sec(mtime_ns)
            # test utimensat(timespec), utimes(timeval), utime(utimbuf)
            # or utime(time_t)
            os.utime(filename, (atime, mtime))
        self._test_utime(set_time)

    def test_utime_by_times(self):
        def set_time(filename, ns):
            atime_ns, mtime_ns = ns
            atime = self.ns_to_sec(atime_ns)
            mtime = self.ns_to_sec(mtime_ns)
            # test the times keyword parameter
            os.utime(filename, times=(atime, mtime))
        self._test_utime(set_time)

    @unittest.skipUnless(os.utime in os.supports_follow_symlinks,
                         "follow_symlinks support for utime required "
                         "for this test.")
    def test_utime_nofollow_symlinks(self):
        def set_time(filename, ns):
            # use follow_symlinks=False to test utimensat(timespec)
            # or lutimes(timeval)
            os.utime(filename, ns=ns, follow_symlinks=False)
        self._test_utime(set_time)

    @unittest.skipUnless(os.utime in os.supports_fd,
                         "fd support for utime required for this test.")
    def test_utime_fd(self):
        def set_time(filename, ns):
            with open(filename, 'wb', 0) as fp:
                # use a file descriptor to test futimens(timespec)
                # or futimes(timeval)
                os.utime(fp.fileno(), ns=ns)
        self._test_utime(set_time)

    @unittest.skipUnless(os.utime in os.supports_dir_fd,
                         "dir_fd support for utime required for this test.")
    def test_utime_dir_fd(self):
        def set_time(filename, ns):
            dirname, name = os.path.split(filename)
            with os_helper.open_dir_fd(dirname) as dirfd:
                # pass dir_fd to test utimensat(timespec) or futimesat(timeval)
                os.utime(name, dir_fd=dirfd, ns=ns)
        self._test_utime(set_time)

    def test_utime_directory(self):
        def set_time(filename, ns):
            # test calling os.utime() on a directory
            os.utime(filename, ns=ns)
        self._test_utime(set_time, filename=self.dirname)

    def _test_utime_current(self, set_time):
        # Get the system clock
        current = time.time()

        # Call os.utime() to set the timestamp to the current system clock
        set_time(self.fname)

        if not self.support_subsecond(self.fname):
            delta = 1.0
        else:
            # On Windows, the usual resolution of time.time() is 15.6 ms.
            # bpo-30649: Tolerate 50 ms for slow Windows buildbots.
            #
            # x86 Gentoo Refleaks 3.x once failed with dt=20.2 ms. So use
            # also 50 ms on other platforms.
            delta = 0.050
        st = os.stat(self.fname)
        msg = ("st_time=%r, current=%r, dt=%r"
               % (st.st_mtime, current, st.st_mtime - current))
        self.assertAlmostEqual(st.st_mtime, current,
                               delta=delta, msg=msg)

    def test_utime_current(self):
        def set_time(filename):
            # Set to the current time in the new way
            os.utime(self.fname)
        self._test_utime_current(set_time)

    def test_utime_current_old(self):
        def set_time(filename):
            # Set to the current time in the old explicit way.
            os.utime(self.fname, None)
        self._test_utime_current(set_time)

    def test_utime_nonexistent(self):
        now = time.time()
        filename = 'nonexistent'
        with self.assertRaises(FileNotFoundError) as cm:
            os.utime(filename, (now, now))
        self.assertEqual(cm.exception.filename, filename)

    def get_file_system(self, path):
        if sys.platform == 'win32':
            root = os.path.splitdrive(os.path.abspath(path))[0] + '\\'
            import ctypes
            kernel32 = ctypes.windll.kernel32
            buf = ctypes.create_unicode_buffer("", 100)
            ok = kernel32.GetVolumeInformationW(root, None, 0,
                                                None, None, None,
                                                buf, len(buf))
            if ok:
                return buf.value
        # return None if the filesystem is unknown

    def test_large_time(self):
        # Many filesystems are limited to the year 2038. At least, the test
        # pass with NTFS filesystem.
        if self.get_file_system(self.dirname) != "NTFS":
            self.skipTest("requires NTFS")

        large = 5000000000   # some day in 2128
        os.utime(self.fname, (large, large))
        self.assertEqual(os.stat(self.fname).st_mtime, large)

    def test_utime_invalid_arguments(self):
        # seconds and nanoseconds parameters are mutually exclusive
        with self.assertRaises(ValueError):
            os.utime(self.fname, (5, 5), ns=(5, 5))
        with self.assertRaises(TypeError):
            os.utime(self.fname, [5, 5])
        with self.assertRaises(TypeError):
            os.utime(self.fname, (5,))
        with self.assertRaises(TypeError):
            os.utime(self.fname, (5, 5, 5))
        with self.assertRaises(TypeError):
            os.utime(self.fname, ns=[5, 5])
        with self.assertRaises(TypeError):
            os.utime(self.fname, ns=(5,))
        with self.assertRaises(TypeError):
            os.utime(self.fname, ns=(5, 5, 5))

        if os.utime not in os.supports_follow_symlinks:
            with self.assertRaises(NotImplementedError):
                os.utime(self.fname, (5, 5), follow_symlinks=False)
        if os.utime not in os.supports_fd:
            with open(self.fname, 'wb', 0) as fp:
                with self.assertRaises(TypeError):
                    os.utime(fp.fileno(), (5, 5))
        if os.utime not in os.supports_dir_fd:
            with self.assertRaises(NotImplementedError):
                os.utime(self.fname, (5, 5), dir_fd=0)

    @support.cpython_only
    def test_issue31577(self):
        # The interpreter shouldn't crash in case utime() received a bad
        # ns argument.
        def get_bad_int(divmod_ret_val):
            class BadInt:
                def __divmod__(*args):
                    return divmod_ret_val
            return BadInt()
        with self.assertRaises(TypeError):
            os.utime(self.fname, ns=(get_bad_int(42), 1))
        with self.assertRaises(TypeError):
            os.utime(self.fname, ns=(get_bad_int(()), 1))
        with self.assertRaises(TypeError):
            os.utime(self.fname, ns=(get_bad_int((1, 2, 3)), 1))


from test import mapping_tests

class EnvironTests(mapping_tests.BasicTestMappingProtocol):
    """check that os.environ object conform to mapping protocol"""
    type2test = None

    def setUp(self):
        self.__save = dict(os.environ)
        if os.supports_bytes_environ:
            self.__saveb = dict(os.environb)
        for key, value in self._reference().items():
            os.environ[key] = value

    def tearDown(self):
        os.environ.clear()
        os.environ.update(self.__save)
        if os.supports_bytes_environ:
            os.environb.clear()
            os.environb.update(self.__saveb)

    def _reference(self):
        return {"KEY1":"VALUE1", "KEY2":"VALUE2", "KEY3":"VALUE3"}

    def _empty_mapping(self):
        os.environ.clear()
        return os.environ

    # Bug 1110478
    @unittest.skipUnless(unix_shell and os.path.exists(unix_shell),
                         'requires a shell')
    @unittest.skipUnless(hasattr(os, 'popen'), "needs os.popen()")
    @support.requires_subprocess()
    def test_update2(self):
        os.environ.clear()
        os.environ.update(HELLO="World")
        with os.popen("%s -c 'echo $HELLO'" % unix_shell) as popen:
            value = popen.read().strip()
            self.assertEqual(value, "World")

    @unittest.skipUnless(unix_shell and os.path.exists(unix_shell),
                         'requires a shell')
    @unittest.skipUnless(hasattr(os, 'popen'), "needs os.popen()")
    @support.requires_subprocess()
    def test_os_popen_iter(self):
        with os.popen("%s -c 'echo \"line1\nline2\nline3\"'"
                      % unix_shell) as popen:
            it = iter(popen)
            self.assertEqual(next(it), "line1\n")
            self.assertEqual(next(it), "line2\n")
            self.assertEqual(next(it), "line3\n")
            self.assertRaises(StopIteration, next, it)

    # Verify environ keys and values from the OS are of the
    # correct str type.
    def test_keyvalue_types(self):
        for key, val in os.environ.items():
            self.assertEqual(type(key), str)
            self.assertEqual(type(val), str)

    def test_items(self):
        for key, value in self._reference().items():
            self.assertEqual(os.environ.get(key), value)

    # Issue 7310
    def test___repr__(self):
        """Check that the repr() of os.environ looks like environ({...})."""
        env = os.environ
        formatted_items = ", ".join(
            f"{key!r}: {value!r}"
            for key, value in env.items()
        )
        self.assertEqual(repr(env), f"environ({{{formatted_items}}})")

    def test_get_exec_path(self):
        defpath_list = os.defpath.split(os.pathsep)
        test_path = ['/monty', '/python', '', '/flying/circus']
        test_env = {'PATH': os.pathsep.join(test_path)}

        saved_environ = os.environ
        try:
            os.environ = dict(test_env)
            # Test that defaulting to os.environ works.
            self.assertSequenceEqual(test_path, os.get_exec_path())
            self.assertSequenceEqual(test_path, os.get_exec_path(env=None))
        finally:
            os.environ = saved_environ

        # No PATH environment variable
        self.assertSequenceEqual(defpath_list, os.get_exec_path({}))
        # Empty PATH environment variable
        self.assertSequenceEqual(('',), os.get_exec_path({'PATH':''}))
        # Supplied PATH environment variable
        self.assertSequenceEqual(test_path, os.get_exec_path(test_env))

        if os.supports_bytes_environ:
            # env cannot contain 'PATH' and b'PATH' keys
            try:
                # ignore BytesWarning warning
                with warnings.catch_warnings(record=True):
                    mixed_env = {'PATH': '1', b'PATH': b'2'}
            except BytesWarning:
                # mixed_env cannot be created with python -bb
                pass
            else:
                self.assertRaises(ValueError, os.get_exec_path, mixed_env)

            # bytes key and/or value
            self.assertSequenceEqual(os.get_exec_path({b'PATH': b'abc'}),
                ['abc'])
            self.assertSequenceEqual(os.get_exec_path({b'PATH': 'abc'}),
                ['abc'])
            self.assertSequenceEqual(os.get_exec_path({'PATH': b'abc'}),
                ['abc'])

    @unittest.skipUnless(os.supports_bytes_environ,
                         "os.environb required for this test.")
    def test_environb(self):
        # os.environ -> os.environb
        value = 'euro\u20ac'
        try:
            value_bytes = value.encode(sys.getfilesystemencoding(),
                                       'surrogateescape')
        except UnicodeEncodeError:
            msg = "U+20AC character is not encodable to %s" % (
                sys.getfilesystemencoding(),)
            self.skipTest(msg)
        os.environ['unicode'] = value
        self.assertEqual(os.environ['unicode'], value)
        self.assertEqual(os.environb[b'unicode'], value_bytes)

        # os.environb -> os.environ
        value = b'\xff'
        os.environb[b'bytes'] = value
        self.assertEqual(os.environb[b'bytes'], value)
        value_str = value.decode(sys.getfilesystemencoding(), 'surrogateescape')
        self.assertEqual(os.environ['bytes'], value_str)

    @support.requires_subprocess()
    def test_putenv_unsetenv(self):
        name = "PYTHONTESTVAR"
        value = "testvalue"
        code = f'import os; print(repr(os.environ.get({name!r})))'

        with os_helper.EnvironmentVarGuard() as env:
            env.pop(name, None)

            os.putenv(name, value)
            proc = subprocess.run([sys.executable, '-c', code], check=True,
                                  stdout=subprocess.PIPE, text=True)
            self.assertEqual(proc.stdout.rstrip(), repr(value))

            os.unsetenv(name)
            proc = subprocess.run([sys.executable, '-c', code], check=True,
                                  stdout=subprocess.PIPE, text=True)
            self.assertEqual(proc.stdout.rstrip(), repr(None))

    # On OS X < 10.6, unsetenv() doesn't return a value (bpo-13415).
    @support.requires_mac_ver(10, 6)
    def test_putenv_unsetenv_error(self):
        # Empty variable name is invalid.
        # "=" and null character are not allowed in a variable name.
        for name in ('', '=name', 'na=me', 'name='):
            self.assertRaises((OSError, ValueError), os.putenv, name, "value")
            self.assertRaises((OSError, ValueError), os.unsetenv, name)
        for name in ('name\0', 'na\0me'):
            self.assertRaises(ValueError, os.putenv, name, "value")
            self.assertRaises(ValueError, os.unsetenv, name)

        if sys.platform == "win32":
            # On Windows, an environment variable string ("name=value" string)
            # is limited to 32,767 characters
            longstr = 'x' * 32_768
            self.assertRaises(ValueError, os.putenv, longstr, "1")
            self.assertRaises(ValueError, os.putenv, "X", longstr)
            self.assertRaises(ValueError, os.unsetenv, longstr)

    def test_key_type(self):
        missing = 'missingkey'
        self.assertNotIn(missing, os.environ)

        with self.assertRaises(KeyError) as cm:
            os.environ[missing]
        self.assertIs(cm.exception.args[0], missing)
        self.assertTrue(cm.exception.__suppress_context__)

        with self.assertRaises(KeyError) as cm:
            del os.environ[missing]
        self.assertIs(cm.exception.args[0], missing)
        self.assertTrue(cm.exception.__suppress_context__)

    def _test_environ_iteration(self, collection):
        iterator = iter(collection)
        new_key = "__new_key__"

        next(iterator)  # start iteration over os.environ.items

        # add a new key in os.environ mapping
        os.environ[new_key] = "test_environ_iteration"

        try:
            next(iterator)  # force iteration over modified mapping
            self.assertEqual(os.environ[new_key], "test_environ_iteration")
        finally:
            del os.environ[new_key]

    def test_iter_error_when_changing_os_environ(self):
        self._test_environ_iteration(os.environ)

    def test_iter_error_when_changing_os_environ_items(self):
        self._test_environ_iteration(os.environ.items())

    def test_iter_error_when_changing_os_environ_values(self):
        self._test_environ_iteration(os.environ.values())

    def _test_underlying_process_env(self, var, expected):
        if not (unix_shell and os.path.exists(unix_shell)):
            return
        elif not support.has_subprocess_support:
            return

        with os.popen(f"{unix_shell} -c 'echo ${var}'") as popen:
            value = popen.read().strip()

        self.assertEqual(expected, value)

    def test_or_operator(self):
        overridden_key = '_TEST_VAR_'
        original_value = 'original_value'
        os.environ[overridden_key] = original_value

        new_vars_dict = {'_A_': '1', '_B_': '2', overridden_key: '3'}
        expected = dict(os.environ)
        expected.update(new_vars_dict)

        actual = os.environ | new_vars_dict
        self.assertDictEqual(expected, actual)
        self.assertEqual('3', actual[overridden_key])

        new_vars_items = new_vars_dict.items()
        self.assertIs(NotImplemented, os.environ.__or__(new_vars_items))

        self._test_underlying_process_env('_A_', '')
        self._test_underlying_process_env(overridden_key, original_value)

    def test_ior_operator(self):
        overridden_key = '_TEST_VAR_'
        os.environ[overridden_key] = 'original_value'

        new_vars_dict = {'_A_': '1', '_B_': '2', overridden_key: '3'}
        expected = dict(os.environ)
        expected.update(new_vars_dict)

        os.environ |= new_vars_dict
        self.assertEqual(expected, os.environ)
        self.assertEqual('3', os.environ[overridden_key])

        self._test_underlying_process_env('_A_', '1')
        self._test_underlying_process_env(overridden_key, '3')

    def test_ior_operator_invalid_dicts(self):
        os_environ_copy = os.environ.copy()
        with self.assertRaises(TypeError):
            dict_with_bad_key = {1: '_A_'}
            os.environ |= dict_with_bad_key

        with self.assertRaises(TypeError):
            dict_with_bad_val = {'_A_': 1}
            os.environ |= dict_with_bad_val

        # Check nothing was added.
        self.assertEqual(os_environ_copy, os.environ)

    def test_ior_operator_key_value_iterable(self):
        overridden_key = '_TEST_VAR_'
        os.environ[overridden_key] = 'original_value'

        new_vars_items = (('_A_', '1'), ('_B_', '2'), (overridden_key, '3'))
        expected = dict(os.environ)
        expected.update(new_vars_items)

        os.environ |= new_vars_items
        self.assertEqual(expected, os.environ)
        self.assertEqual('3', os.environ[overridden_key])

        self._test_underlying_process_env('_A_', '1')
        self._test_underlying_process_env(overridden_key, '3')

    def test_ror_operator(self):
        overridden_key = '_TEST_VAR_'
        original_value = 'original_value'
        os.environ[overridden_key] = original_value

        new_vars_dict = {'_A_': '1', '_B_': '2', overridden_key: '3'}
        expected = dict(new_vars_dict)
        expected.update(os.environ)

        actual = new_vars_dict | os.environ
        self.assertDictEqual(expected, actual)
        self.assertEqual(original_value, actual[overridden_key])

        new_vars_items = new_vars_dict.items()
        self.assertIs(NotImplemented, os.environ.__ror__(new_vars_items))

        self._test_underlying_process_env('_A_', '')
        self._test_underlying_process_env(overridden_key, original_value)

    def test_reload_environ(self):
        # Test os.reload_environ()
        has_environb = hasattr(os, 'environb')

        # Test with putenv() which doesn't update os.environ
        os.environ['test_env'] = 'python_value'
        os.putenv("test_env", "new_value")
        self.assertEqual(os.environ['test_env'], 'python_value')
        if has_environb:
            self.assertEqual(os.environb[b'test_env'], b'python_value')

        os.reload_environ()
        self.assertEqual(os.environ['test_env'], 'new_value')
        if has_environb:
            self.assertEqual(os.environb[b'test_env'], b'new_value')

        # Test with unsetenv() which doesn't update os.environ
        os.unsetenv('test_env')
        self.assertEqual(os.environ['test_env'], 'new_value')
        if has_environb:
            self.assertEqual(os.environb[b'test_env'], b'new_value')

        os.reload_environ()
        self.assertNotIn('test_env', os.environ)
        if has_environb:
            self.assertNotIn(b'test_env', os.environb)

        if has_environb:
            # test reload_environ() on os.environb with putenv()
            os.environb[b'test_env'] = b'python_value2'
            os.putenv("test_env", "new_value2")
            self.assertEqual(os.environb[b'test_env'], b'python_value2')
            self.assertEqual(os.environ['test_env'], 'python_value2')

            os.reload_environ()
            self.assertEqual(os.environb[b'test_env'], b'new_value2')
            self.assertEqual(os.environ['test_env'], 'new_value2')

            # test reload_environ() on os.environb with unsetenv()
            os.unsetenv('test_env')
            self.assertEqual(os.environb[b'test_env'], b'new_value2')
            self.assertEqual(os.environ['test_env'], 'new_value2')

            os.reload_environ()
            self.assertNotIn(b'test_env', os.environb)
            self.assertNotIn('test_env', os.environ)

class WalkTests(unittest.TestCase):
    """Tests for os.walk()."""
    is_fwalk = False

    # Wrapper to hide minor differences between os.walk and os.fwalk
    # to tests both functions with the same code base
    def walk(self, top, **kwargs):
        if 'follow_symlinks' in kwargs:
            kwargs['followlinks'] = kwargs.pop('follow_symlinks')
        return os.walk(top, **kwargs)

    def setUp(self):
        join = os.path.join
        self.addCleanup(os_helper.rmtree, os_helper.TESTFN)

        # Build:
        #     TESTFN/
        #       TEST1/              a file kid and two directory kids
        #         tmp1
        #         SUB1/             a file kid and a directory kid
        #           tmp2
        #           SUB11/          no kids
        #         SUB2/             a file kid and a dirsymlink kid
        #           tmp3
        #           SUB21/          not readable
        #             tmp5
        #           link/           a symlink to TESTFN.2
        #           broken_link
        #           broken_link2
        #           broken_link3
        #       TEST2/
        #         tmp4              a lone file
        self.walk_path = join(os_helper.TESTFN, "TEST1")
        self.sub1_path = join(self.walk_path, "SUB1")
        self.sub11_path = join(self.sub1_path, "SUB11")
        sub2_path = join(self.walk_path, "SUB2")
        sub21_path = join(sub2_path, "SUB21")
        self.tmp1_path = join(self.walk_path, "tmp1")
        tmp2_path = join(self.sub1_path, "tmp2")
        tmp3_path = join(sub2_path, "tmp3")
        tmp5_path = join(sub21_path, "tmp3")
        self.link_path = join(sub2_path, "link")
        t2_path = join(os_helper.TESTFN, "TEST2")
        tmp4_path = join(os_helper.TESTFN, "TEST2", "tmp4")
        self.broken_link_path = join(sub2_path, "broken_link")
        broken_link2_path = join(sub2_path, "broken_link2")
        broken_link3_path = join(sub2_path, "broken_link3")

        # Create stuff.
        os.makedirs(self.sub11_path)
        os.makedirs(sub2_path)
        os.makedirs(sub21_path)
        os.makedirs(t2_path)

        for path in self.tmp1_path, tmp2_path, tmp3_path, tmp4_path, tmp5_path:
            with open(path, "x", encoding='utf-8') as f:
                f.write("I'm " + path + " and proud of it.  Blame test_os.\n")

        if os_helper.can_symlink():
            os.symlink(os.path.abspath(t2_path), self.link_path)
            os.symlink('broken', self.broken_link_path, True)
            os.symlink(join('tmp3', 'broken'), broken_link2_path, True)
            os.symlink(join('SUB21', 'tmp5'), broken_link3_path, True)
            self.sub2_tree = (sub2_path, ["SUB21", "link"],
                              ["broken_link", "broken_link2", "broken_link3",
                               "tmp3"])
        else:
            self.sub2_tree = (sub2_path, ["SUB21"], ["tmp3"])

        os.chmod(sub21_path, 0)
        try:
            os.listdir(sub21_path)
        except PermissionError:
            self.addCleanup(os.chmod, sub21_path, stat.S_IRWXU)
        else:
            os.chmod(sub21_path, stat.S_IRWXU)
            os.unlink(tmp5_path)
            os.rmdir(sub21_path)
            del self.sub2_tree[1][:1]

    def test_walk_topdown(self):
        # Walk top-down.
        all = list(self.walk(self.walk_path))

        self.assertEqual(len(all), 4)
        # We can't know which order SUB1 and SUB2 will appear in.
        # Not flipped:  TESTFN, SUB1, SUB11, SUB2
        #     flipped:  TESTFN, SUB2, SUB1, SUB11
        flipped = all[0][1][0] != "SUB1"
        all[0][1].sort()
        all[3 - 2 * flipped][-1].sort()
        all[3 - 2 * flipped][1].sort()
        self.assertEqual(all[0], (self.walk_path, ["SUB1", "SUB2"], ["tmp1"]))
        self.assertEqual(all[1 + flipped], (self.sub1_path, ["SUB11"], ["tmp2"]))
        self.assertEqual(all[2 + flipped], (self.sub11_path, [], []))
        self.assertEqual(all[3 - 2 * flipped], self.sub2_tree)

    def test_walk_prune(self, walk_path=None):
        if walk_path is None:
            walk_path = self.walk_path
        # Prune the search.
        all = []
        for root, dirs, files in self.walk(walk_path):
            all.append((root, dirs, files))
            # Don't descend into SUB1.
            if 'SUB1' in dirs:
                # Note that this also mutates the dirs we appended to all!
                dirs.remove('SUB1')

        self.assertEqual(len(all), 2)
        self.assertEqual(all[0], (self.walk_path, ["SUB2"], ["tmp1"]))

        all[1][-1].sort()
        all[1][1].sort()
        self.assertEqual(all[1], self.sub2_tree)

    def test_file_like_path(self):
        self.test_walk_prune(FakePath(self.walk_path))

    def test_walk_bottom_up(self):
        # Walk bottom-up.
        all = list(self.walk(self.walk_path, topdown=False))

        self.assertEqual(len(all), 4, all)
        # We can't know which order SUB1 and SUB2 will appear in.
        # Not flipped:  SUB11, SUB1, SUB2, TESTFN
        #     flipped:  SUB2, SUB11, SUB1, TESTFN
        flipped = all[3][1][0] != "SUB1"
        all[3][1].sort()
        all[2 - 2 * flipped][-1].sort()
        all[2 - 2 * flipped][1].sort()
        self.assertEqual(all[3],
                         (self.walk_path, ["SUB1", "SUB2"], ["tmp1"]))
        self.assertEqual(all[flipped],
                         (self.sub11_path, [], []))
        self.assertEqual(all[flipped + 1],
                         (self.sub1_path, ["SUB11"], ["tmp2"]))
        self.assertEqual(all[2 - 2 * flipped],
                         self.sub2_tree)

    def test_walk_symlink(self):
        if not os_helper.can_symlink():
            self.skipTest("need symlink support")

        # Walk, following symlinks.
        walk_it = self.walk(self.walk_path, follow_symlinks=True)
        for root, dirs, files in walk_it:
            if root == self.link_path:
                self.assertEqual(dirs, [])
                self.assertEqual(files, ["tmp4"])
                break
        else:
            self.fail("Didn't follow symlink with followlinks=True")

        walk_it = self.walk(self.broken_link_path, follow_symlinks=True)
        if self.is_fwalk:
            self.assertRaises(FileNotFoundError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)

    def test_walk_bad_dir(self):
        # Walk top-down.
        errors = []
        walk_it = self.walk(self.walk_path, onerror=errors.append)
        root, dirs, files = next(walk_it)
        self.assertEqual(errors, [])
        dir1 = 'SUB1'
        path1 = os.path.join(root, dir1)
        path1new = os.path.join(root, dir1 + '.new')
        os.rename(path1, path1new)
        try:
            roots = [r for r, d, f in walk_it]
            self.assertTrue(errors)
            self.assertNotIn(path1, roots)
            self.assertNotIn(path1new, roots)
            for dir2 in dirs:
                if dir2 != dir1:
                    self.assertIn(os.path.join(root, dir2), roots)
        finally:
            os.rename(path1new, path1)

    def test_walk_bad_dir2(self):
        walk_it = self.walk('nonexisting')
        if self.is_fwalk:
            self.assertRaises(FileNotFoundError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)

        walk_it = self.walk('nonexisting', follow_symlinks=True)
        if self.is_fwalk:
            self.assertRaises(FileNotFoundError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)

        walk_it = self.walk(self.tmp1_path)
        self.assertRaises(StopIteration, next, walk_it)

        walk_it = self.walk(self.tmp1_path, follow_symlinks=True)
        if self.is_fwalk:
            self.assertRaises(NotADirectoryError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)

    @unittest.skipUnless(hasattr(os, "mkfifo"), 'requires os.mkfifo()')
    @unittest.skipIf(sys.platform == "vxworks",
                    "fifo requires special path on VxWorks")
    def test_walk_named_pipe(self):
        path = os_helper.TESTFN + '-pipe'
        os.mkfifo(path)
        self.addCleanup(os.unlink, path)

        walk_it = self.walk(path)
        self.assertRaises(StopIteration, next, walk_it)

        walk_it = self.walk(path, follow_symlinks=True)
        if self.is_fwalk:
            self.assertRaises(NotADirectoryError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)

    @unittest.skipUnless(hasattr(os, "mkfifo"), 'requires os.mkfifo()')
    @unittest.skipIf(sys.platform == "vxworks",
                    "fifo requires special path on VxWorks")
    def test_walk_named_pipe2(self):
        path = os_helper.TESTFN + '-dir'
        os.mkdir(path)
        self.addCleanup(shutil.rmtree, path)
        os.mkfifo(os.path.join(path, 'mypipe'))

        errors = []
        walk_it = self.walk(path, onerror=errors.append)
        next(walk_it)
        self.assertRaises(StopIteration, next, walk_it)
        self.assertEqual(errors, [])

        errors = []
        walk_it = self.walk(path, onerror=errors.append)
        root, dirs, files = next(walk_it)
        self.assertEqual(root, path)
        self.assertEqual(dirs, [])
        self.assertEqual(files, ['mypipe'])
        dirs.extend(files)
        files.clear()
        if self.is_fwalk:
            self.assertRaises(NotADirectoryError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)
        if self.is_fwalk:
            self.assertEqual(errors, [])
        else:
            self.assertEqual(len(errors), 1, errors)
            self.assertIsInstance(errors[0], NotADirectoryError)

    def test_walk_many_open_files(self):
        depth = 30
        base = os.path.join(os_helper.TESTFN, 'deep')
        p = os.path.join(base, *(['d']*depth))
        os.makedirs(p)

        iters = [self.walk(base, topdown=False) for j in range(100)]
        for i in range(depth + 1):
            expected = (p, ['d'] if i else [], [])
            for it in iters:
                self.assertEqual(next(it), expected)
            p = os.path.dirname(p)

        iters = [self.walk(base, topdown=True) for j in range(100)]
        p = base
        for i in range(depth + 1):
            expected = (p, ['d'] if i < depth else [], [])
            for it in iters:
                self.assertEqual(next(it), expected)
            p = os.path.join(p, 'd')

    def test_walk_above_recursion_limit(self):
        depth = 50
        os.makedirs(os.path.join(self.walk_path, *(['d'] * depth)))
        with infinite_recursion(depth - 5):
            all = list(self.walk(self.walk_path))

        sub2_path = self.sub2_tree[0]
        for root, dirs, files in all:
            if root == sub2_path:
                dirs.sort()
                files.sort()

        d_entries = []
        d_path = self.walk_path
        for _ in range(depth):
            d_path = os.path.join(d_path, "d")
            d_entries.append((d_path, ["d"], []))
        d_entries[-1][1].clear()

        # Sub-sequences where the order is known
        sections = {
            "SUB1": [
                (self.sub1_path, ["SUB11"], ["tmp2"]),
                (self.sub11_path, [], []),
            ],
            "SUB2": [self.sub2_tree],
            "d": d_entries,
        }

        # The ordering of sub-dirs is arbitrary but determines the order in
        # which sub-sequences appear
        dirs = all[0][1]
        expected = [(self.walk_path, dirs, ["tmp1"])]
        for d in dirs:
            expected.extend(sections[d])

        self.assertEqual(len(all), depth + 4)
        self.assertEqual(sorted(dirs), ["SUB1", "SUB2", "d"])
        self.assertEqual(all, expected)


@unittest.skipUnless(hasattr(os, 'fwalk'), "Test needs os.fwalk()")
class FwalkTests(WalkTests):
    """Tests for os.fwalk()."""
    is_fwalk = True

    def walk(self, top, **kwargs):
        for root, dirs, files, root_fd in self.fwalk(top, **kwargs):
            yield (root, dirs, files)

    def fwalk(self, *args, **kwargs):
        return os.fwalk(*args, **kwargs)

    def _compare_to_walk(self, walk_kwargs, fwalk_kwargs):
        """
        compare with walk() results.
        """
        walk_kwargs = walk_kwargs.copy()
        fwalk_kwargs = fwalk_kwargs.copy()
        for topdown, follow_symlinks in itertools.product((True, False), repeat=2):
            walk_kwargs.update(topdown=topdown, followlinks=follow_symlinks)
            fwalk_kwargs.update(topdown=topdown, follow_symlinks=follow_symlinks)

            expected = {}
            for root, dirs, files in os.walk(**walk_kwargs):
                expected[root] = (set(dirs), set(files))

            for root, dirs, files, rootfd in self.fwalk(**fwalk_kwargs):
                self.assertIn(root, expected)
                self.assertEqual(expected[root], (set(dirs), set(files)))

    def test_compare_to_walk(self):
        kwargs = {'top': os_helper.TESTFN}
        self._compare_to_walk(kwargs, kwargs)

    def test_dir_fd(self):
        try:
            fd = os.open(".", os.O_RDONLY)
            walk_kwargs = {'top': os_helper.TESTFN}
            fwalk_kwargs = walk_kwargs.copy()
            fwalk_kwargs['dir_fd'] = fd
            self._compare_to_walk(walk_kwargs, fwalk_kwargs)
        finally:
            os.close(fd)

    def test_yields_correct_dir_fd(self):
        # check returned file descriptors
        for topdown, follow_symlinks in itertools.product((True, False), repeat=2):
            args = os_helper.TESTFN, topdown, None
            for root, dirs, files, rootfd in self.fwalk(*args, follow_symlinks=follow_symlinks):
                # check that the FD is valid
                os.fstat(rootfd)
                # redundant check
                os.stat(rootfd)
                # check that listdir() returns consistent information
                self.assertEqual(set(os.listdir(rootfd)), set(dirs) | set(files))

    @unittest.skipIf(
        support.is_android, "dup return value is unpredictable on Android"
    )
    def test_fd_leak(self):
        # Since we're opening a lot of FDs, we must be careful to avoid leaks:
        # we both check that calling fwalk() a large number of times doesn't
        # yield EMFILE, and that the minimum allocated FD hasn't changed.
        minfd = os.dup(1)
        os.close(minfd)
        for i in range(256):
            for x in self.fwalk(os_helper.TESTFN):
                pass
        newfd = os.dup(1)
        self.addCleanup(os.close, newfd)
        self.assertEqual(newfd, minfd)

    @unittest.skipIf(
        support.is_android, "dup return value is unpredictable on Android"
    )
    def test_fd_finalization(self):
        # Check that close()ing the fwalk() generator closes FDs
        def getfd():
            fd = os.dup(1)
            os.close(fd)
            return fd
        for topdown in (False, True):
            old_fd = getfd()
            it = self.fwalk(os_helper.TESTFN, topdown=topdown)
            self.assertEqual(getfd(), old_fd)
            next(it)
            self.assertGreater(getfd(), old_fd)
            it.close()
            self.assertEqual(getfd(), old_fd)

    # fwalk() keeps file descriptors open
    test_walk_many_open_files = None


class BytesWalkTests(WalkTests):
    """Tests for os.walk() with bytes."""
    def walk(self, top, **kwargs):
        if 'follow_symlinks' in kwargs:
            kwargs['followlinks'] = kwargs.pop('follow_symlinks')
        for broot, bdirs, bfiles in os.walk(os.fsencode(top), **kwargs):
            root = os.fsdecode(broot)
            dirs = list(map(os.fsdecode, bdirs))
            files = list(map(os.fsdecode, bfiles))
            yield (root, dirs, files)
            bdirs[:] = list(map(os.fsencode, dirs))
            bfiles[:] = list(map(os.fsencode, files))

@unittest.skipUnless(hasattr(os, 'fwalk'), "Test needs os.fwalk()")
class BytesFwalkTests(FwalkTests):
    """Tests for os.walk() with bytes."""
    def fwalk(self, top='.', *args, **kwargs):
        for broot, bdirs, bfiles, topfd in os.fwalk(os.fsencode(top), *args, **kwargs):
            root = os.fsdecode(broot)
            dirs = list(map(os.fsdecode, bdirs))
            files = list(map(os.fsdecode, bfiles))
            yield (root, dirs, files, topfd)
            bdirs[:] = list(map(os.fsencode, dirs))
            bfiles[:] = list(map(os.fsencode, files))


class MakedirTests(unittest.TestCase):
    def setUp(self):
        os.mkdir(os_helper.TESTFN)

    def test_makedir(self):
        base = os_helper.TESTFN
        path = os.path.join(base, 'dir1', 'dir2', 'dir3')
        os.makedirs(path)             # Should work
        path = os.path.join(base, 'dir1', 'dir2', 'dir3', 'dir4')
        os.makedirs(path)

        # Try paths with a '.' in them
        self.assertRaises(OSError, os.makedirs, os.curdir)
        path = os.path.join(base, 'dir1', 'dir2', 'dir3', 'dir4', 'dir5', os.curdir)
        os.makedirs(path)
        path = os.path.join(base, 'dir1', os.curdir, 'dir2', 'dir3', 'dir4',
                            'dir5', 'dir6')
        os.makedirs(path)

    @unittest.skipIf(
        support.is_wasi,
        "WASI's umask is a stub."
    )
    def test_mode(self):
        # Note: in some cases, the umask might already be 2 in which case this
        # will pass even if os.umask is actually broken.
        with os_helper.temp_umask(0o002):
            base = os_helper.TESTFN
            parent = os.path.join(base, 'dir1')
            path = os.path.join(parent, 'dir2')
            os.makedirs(path, 0o555)
            self.assertTrue(os.path.exists(path))
            self.assertTrue(os.path.isdir(path))
            if os.name != 'nt':
                self.assertEqual(os.stat(path).st_mode & 0o777, 0o555)
                self.assertEqual(os.stat(parent).st_mode & 0o777, 0o775)

    @unittest.skipIf(
        support.is_wasi,
        "WASI's umask is a stub."
    )
    def test_exist_ok_existing_directory(self):
        path = os.path.join(os_helper.TESTFN, 'dir1')
        mode = 0o777
        old_mask = os.umask(0o022)
        os.makedirs(path, mode)
        self.assertRaises(OSError, os.makedirs, path, mode)
        self.assertRaises(OSError, os.makedirs, path, mode, exist_ok=False)
        os.makedirs(path, 0o776, exist_ok=True)
        os.makedirs(path, mode=mode, exist_ok=True)
        os.umask(old_mask)

        # Issue #25583: A drive root could raise PermissionError on Windows
        os.makedirs(os.path.abspath('/'), exist_ok=True)

    @unittest.skipIf(
        support.is_wasi,
        "WASI's umask is a stub."
    )
    def test_exist_ok_s_isgid_directory(self):
        path = os.path.join(os_helper.TESTFN, 'dir1')
        S_ISGID = stat.S_ISGID
        mode = 0o777
        old_mask = os.umask(0o022)
        try:
            existing_testfn_mode = stat.S_IMODE(
                    os.lstat(os_helper.TESTFN).st_mode)
            try:
                os.chmod(os_helper.TESTFN, existing_testfn_mode | S_ISGID)
            except PermissionError:
                raise unittest.SkipTest('Cannot set S_ISGID for dir.')
            if (os.lstat(os_helper.TESTFN).st_mode & S_ISGID != S_ISGID):
                raise unittest.SkipTest('No support for S_ISGID dir mode.')
            # The os should apply S_ISGID from the parent dir for us, but
            # this test need not depend on that behavior.  Be explicit.
            os.makedirs(path, mode | S_ISGID)
            # http://bugs.python.org/issue14992
            # Should not fail when the bit is already set.
            os.makedirs(path, mode, exist_ok=True)
            # remove the bit.
            os.chmod(path, stat.S_IMODE(os.lstat(path).st_mode) & ~S_ISGID)
            # May work even when the bit is not already set when demanded.
            os.makedirs(path, mode | S_ISGID, exist_ok=True)
        finally:
            os.umask(old_mask)

    def test_exist_ok_existing_regular_file(self):
        base = os_helper.TESTFN
        path = os.path.join(os_helper.TESTFN, 'dir1')
        with open(path, 'w', encoding='utf-8') as f:
            f.write('abc')
        self.assertRaises(OSError, os.makedirs, path)
        self.assertRaises(OSError, os.makedirs, path, exist_ok=False)
        self.assertRaises(OSError, os.makedirs, path, exist_ok=True)
        os.remove(path)

    @unittest.skipUnless(os.name == 'nt', "requires Windows")
    def test_win32_mkdir_700(self):
        base = os_helper.TESTFN
        path = os.path.abspath(os.path.join(os_helper.TESTFN, 'dir'))
        os.mkdir(path, mode=0o700)
        out = subprocess.check_output(["cacls.exe", path, "/s"], encoding="oem")
        os.rmdir(path)
        out = out.strip().rsplit(" ", 1)[1]
        self.assertEqual(
            out,
            '"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;FA;;;OW)"',
        )

    def tearDown(self):
        path = os.path.join(os_helper.TESTFN, 'dir1', 'dir2', 'dir3',
                            'dir4', 'dir5', 'dir6')
        # If the tests failed, the bottom-most directory ('../dir6')
        # may not have been created, so we look for the outermost directory
        # that exists.
        while not os.path.exists(path) and path != os_helper.TESTFN:
            path = os.path.dirname(path)

        os.removedirs(path)


@unittest.skipUnless(hasattr(os, "chown"), "requires os.chown()")
class ChownFileTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        os.mkdir(os_helper.TESTFN)

    def test_chown_uid_gid_arguments_must_be_index(self):
        stat = os.stat(os_helper.TESTFN)
        uid = stat.st_uid
        gid = stat.st_gid
        for value in (-1.0, -1j, decimal.Decimal(-1), fractions.Fraction(-2, 2)):
            self.assertRaises(TypeError, os.chown, os_helper.TESTFN, value, gid)
            self.assertRaises(TypeError, os.chown, os_helper.TESTFN, uid, value)
        self.assertIsNone(os.chown(os_helper.TESTFN, uid, gid))
        self.assertIsNone(os.chown(os_helper.TESTFN, -1, -1))

    @unittest.skipUnless(hasattr(os, 'getgroups'), 'need os.getgroups')
    def test_chown_gid(self):
        groups = os.getgroups()
        if len(groups) < 2:
            self.skipTest("test needs at least 2 groups")

        gid_1, gid_2 = groups[:2]
        uid = os.stat(os_helper.TESTFN).st_uid

        os.chown(os_helper.TESTFN, uid, gid_1)
        gid = os.stat(os_helper.TESTFN).st_gid
        self.assertEqual(gid, gid_1)

        os.chown(os_helper.TESTFN, uid, gid_2)
        gid = os.stat(os_helper.TESTFN).st_gid
        self.assertEqual(gid, gid_2)

    @unittest.skipUnless(root_in_posix and len(all_users) > 1,
                         "test needs root privilege and more than one user")
    def test_chown_with_root(self):
        uid_1, uid_2 = all_users[:2]
        gid = os.stat(os_helper.TESTFN).st_gid
        os.chown(os_helper.TESTFN, uid_1, gid)
        uid = os.stat(os_helper.TESTFN).st_uid
        self.assertEqual(uid, uid_1)
        os.chown(os_helper.TESTFN, uid_2, gid)
        uid = os.stat(os_helper.TESTFN).st_uid
        self.assertEqual(uid, uid_2)

    @unittest.skipUnless(not root_in_posix and len(all_users) > 1,
                         "test needs non-root account and more than one user")
    def test_chown_without_permission(self):
        uid_1, uid_2 = all_users[:2]
        gid = os.stat(os_helper.TESTFN).st_gid
        with self.assertRaises(PermissionError):
            os.chown(os_helper.TESTFN, uid_1, gid)
            os.chown(os_helper.TESTFN, uid_2, gid)

    @classmethod
    def tearDownClass(cls):
        os.rmdir(os_helper.TESTFN)


class RemoveDirsTests(unittest.TestCase):
    def setUp(self):
        os.makedirs(os_helper.TESTFN)

    def tearDown(self):
        os_helper.rmtree(os_helper.TESTFN)

    def test_remove_all(self):
        dira = os.path.join(os_helper.TESTFN, 'dira')
        os.mkdir(dira)
        dirb = os.path.join(dira, 'dirb')
        os.mkdir(dirb)
        os.removedirs(dirb)
        self.assertFalse(os.path.exists(dirb))
        self.assertFalse(os.path.exists(dira))
        self.assertFalse(os.path.exists(os_helper.TESTFN))

    def test_remove_partial(self):
        dira = os.path.join(os_helper.TESTFN, 'dira')
        os.mkdir(dira)
        dirb = os.path.join(dira, 'dirb')
        os.mkdir(dirb)
        create_file(os.path.join(dira, 'file.txt'))
        os.removedirs(dirb)
        self.assertFalse(os.path.exists(dirb))
        self.assertTrue(os.path.exists(dira))
        self.assertTrue(os.path.exists(os_helper.TESTFN))

    def test_remove_nothing(self):
        dira = os.path.join(os_helper.TESTFN, 'dira')
        os.mkdir(dira)
        dirb = os.path.join(dira, 'dirb')
        os.mkdir(dirb)
        create_file(os.path.join(dirb, 'file.txt'))
        with self.assertRaises(OSError):
            os.removedirs(dirb)
        self.assertTrue(os.path.exists(dirb))
        self.assertTrue(os.path.exists(dira))
        self.assertTrue(os.path.exists(os_helper.TESTFN))


@unittest.skipIf(support.is_wasi, "WASI has no /dev/null")
class DevNullTests(unittest.TestCase):
    def test_devnull(self):
        with open(os.devnull, 'wb', 0) as f:
            f.write(b'hello')
            f.close()
        with open(os.devnull, 'rb') as f:
            self.assertEqual(f.read(), b'')


class URandomTests(unittest.TestCase):
    def test_urandom_length(self):
        self.assertEqual(len(os.urandom(0)), 0)
        self.assertEqual(len(os.urandom(1)), 1)
        self.assertEqual(len(os.urandom(10)), 10)
        self.assertEqual(len(os.urandom(100)), 100)
        self.assertEqual(len(os.urandom(1000)), 1000)

    def test_urandom_value(self):
        data1 = os.urandom(16)
        self.assertIsInstance(data1, bytes)
        data2 = os.urandom(16)
        self.assertNotEqual(data1, data2)

    def get_urandom_subprocess(self, count):
        code = '\n'.join((
            'import os, sys',
            'data = os.urandom(%s)' % count,
            'sys.stdout.buffer.write(data)',
            'sys.stdout.buffer.flush()'))
        out = assert_python_ok('-c', code)
        stdout = out[1]
        self.assertEqual(len(stdout), count)
        return stdout

    def test_urandom_subprocess(self):
        data1 = self.get_urandom_subprocess(16)
        data2 = self.get_urandom_subprocess(16)
        self.assertNotEqual(data1, data2)


@unittest.skipUnless(hasattr(os, 'getrandom'), 'need os.getrandom()')
class GetRandomTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        try:
            os.getrandom(1)
        except OSError as exc:
            if exc.errno == errno.ENOSYS:
                # Python compiled on a more recent Linux version
                # than the current Linux kernel
                raise unittest.SkipTest("getrandom() syscall fails with ENOSYS")
            else:
                raise

    def test_getrandom_type(self):
        data = os.getrandom(16)
        self.assertIsInstance(data, bytes)
        self.assertEqual(len(data), 16)

    def test_getrandom0(self):
        empty = os.getrandom(0)
        self.assertEqual(empty, b'')

    def test_getrandom_random(self):
        self.assertHasAttr(os, 'GRND_RANDOM')

        # Don't test os.getrandom(1, os.GRND_RANDOM) to not consume the rare
        # resource /dev/random

    def test_getrandom_nonblock(self):
        # The call must not fail. Check also that the flag exists
        try:
            os.getrandom(1, os.GRND_NONBLOCK)
        except BlockingIOError:
            # System urandom is not initialized yet
            pass

    def test_getrandom_value(self):
        data1 = os.getrandom(16)
        data2 = os.getrandom(16)
        self.assertNotEqual(data1, data2)


# os.urandom() doesn't use a file descriptor when it is implemented with the
# getentropy() function, the getrandom() function or the getrandom() syscall
OS_URANDOM_DONT_USE_FD = (
    sysconfig.get_config_var('HAVE_GETENTROPY') == 1
    or sysconfig.get_config_var('HAVE_GETRANDOM') == 1
    or sysconfig.get_config_var('HAVE_GETRANDOM_SYSCALL') == 1)

@unittest.skipIf(OS_URANDOM_DONT_USE_FD ,
                 "os.random() does not use a file descriptor")
@unittest.skipIf(sys.platform == "vxworks",
                 "VxWorks can't set RLIMIT_NOFILE to 1")
class URandomFDTests(unittest.TestCase):
    @unittest.skipUnless(resource, "test requires the resource module")
    def test_urandom_failure(self):
        # Check urandom() failing when it is not able to open /dev/random.
        # We spawn a new process to make the test more robust (if getrlimit()
        # failed to restore the file descriptor limit after this, the whole
        # test suite would crash; this actually happened on the OS X Tiger
        # buildbot).
        code = """if 1:
            import errno
            import os
            import resource

            soft_limit, hard_limit = resource.getrlimit(resource.RLIMIT_NOFILE)
            resource.setrlimit(resource.RLIMIT_NOFILE, (1, hard_limit))
            try:
                os.urandom(16)
            except OSError as e:
                assert e.errno == errno.EMFILE, e.errno
            else:
                raise AssertionError("OSError not raised")
            """
        assert_python_ok('-c', code)

    def test_urandom_fd_closed(self):
        # Issue #21207: urandom() should reopen its fd to /dev/urandom if
        # closed.
        code = """if 1:
            import os
            import sys
            import test.support
            os.urandom(4)
            with test.support.SuppressCrashReport():
                os.closerange(3, 256)
            sys.stdout.buffer.write(os.urandom(4))
            """
        rc, out, err = assert_python_ok('-Sc', code)

    def test_urandom_fd_reopened(self):
        # Issue #21207: urandom() should detect its fd to /dev/urandom
        # changed to something else, and reopen it.
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        create_file(os_helper.TESTFN, b"x" * 256)

        code = """if 1:
            import os
            import sys
            import test.support
            os.urandom(4)
            with test.support.SuppressCrashReport():
                for fd in range(3, 256):
                    try:
                        os.close(fd)
                    except OSError:
                        pass
                    else:
                        # Found the urandom fd (XXX hopefully)
                        break
                os.closerange(3, 256)
            with open({TESTFN!r}, 'rb') as f:
                new_fd = f.fileno()
                # Issue #26935: posix allows new_fd and fd to be equal but
                # some libc implementations have dup2 return an error in this
                # case.
                if new_fd != fd:
                    os.dup2(new_fd, fd)
                sys.stdout.buffer.write(os.urandom(4))
                sys.stdout.buffer.write(os.urandom(4))
            """.format(TESTFN=os_helper.TESTFN)
        rc, out, err = assert_python_ok('-Sc', code)
        self.assertEqual(len(out), 8)
        self.assertNotEqual(out[0:4], out[4:8])
        rc, out2, err2 = assert_python_ok('-Sc', code)
        self.assertEqual(len(out2), 8)
        self.assertNotEqual(out2, out)


@contextlib.contextmanager
def _execvpe_mockup(defpath=None):
    """
    Stubs out execv and execve functions when used as context manager.
    Records exec calls. The mock execv and execve functions always raise an
    exception as they would normally never return.
    """
    # A list of tuples containing (function name, first arg, args)
    # of calls to execv or execve that have been made.
    calls = []

    def mock_execv(name, *args):
        calls.append(('execv', name, args))
        raise RuntimeError("execv called")

    def mock_execve(name, *args):
        calls.append(('execve', name, args))
        raise OSError(errno.ENOTDIR, "execve called")

    try:
        orig_execv = os.execv
        orig_execve = os.execve
        orig_defpath = os.defpath
        os.execv = mock_execv
        os.execve = mock_execve
        if defpath is not None:
            os.defpath = defpath
        yield calls
    finally:
        os.execv = orig_execv
        os.execve = orig_execve
        os.defpath = orig_defpath

@unittest.skipUnless(hasattr(os, 'execv'),
                     "need os.execv()")
class ExecTests(unittest.TestCase):
    @unittest.skipIf(USING_LINUXTHREADS,
                     "avoid triggering a linuxthreads bug: see issue #4970")
    def test_execvpe_with_bad_program(self):
        self.assertRaises(OSError, os.execvpe, 'no such app-',
                          ['no such app-'], None)

    def test_execv_with_bad_arglist(self):
        self.assertRaises(ValueError, os.execv, 'notepad', ())
        self.assertRaises(ValueError, os.execv, 'notepad', [])
        self.assertRaises(ValueError, os.execv, 'notepad', ('',))
        self.assertRaises(ValueError, os.execv, 'notepad', [''])

    def test_execvpe_with_bad_arglist(self):
        self.assertRaises(ValueError, os.execvpe, 'notepad', [], None)
        self.assertRaises(ValueError, os.execvpe, 'notepad', [], {})
        self.assertRaises(ValueError, os.execvpe, 'notepad', [''], {})

    @unittest.skipUnless(hasattr(os, '_execvpe'),
                         "No internal os._execvpe function to test.")
    def _test_internal_execvpe(self, test_type):
        program_path = os.sep + 'absolutepath'
        if test_type is bytes:
            program = b'executable'
            fullpath = os.path.join(os.fsencode(program_path), program)
            native_fullpath = fullpath
            arguments = [b'progname', 'arg1', 'arg2']
        else:
            program = 'executable'
            arguments = ['progname', 'arg1', 'arg2']
            fullpath = os.path.join(program_path, program)
            if os.name != "nt":
                native_fullpath = os.fsencode(fullpath)
            else:
                native_fullpath = fullpath
        env = {'spam': 'beans'}

        # test os._execvpe() with an absolute path
        with _execvpe_mockup() as calls:
            self.assertRaises(RuntimeError,
                os._execvpe, fullpath, arguments)
            self.assertEqual(len(calls), 1)
            self.assertEqual(calls[0], ('execv', fullpath, (arguments,)))

        # test os._execvpe() with a relative path:
        # os.get_exec_path() returns defpath
        with _execvpe_mockup(defpath=program_path) as calls:
            self.assertRaises(OSError,
                os._execvpe, program, arguments, env=env)
            self.assertEqual(len(calls), 1)
            self.assertSequenceEqual(calls[0],
                ('execve', native_fullpath, (arguments, env)))

        # test os._execvpe() with a relative path:
        # os.get_exec_path() reads the 'PATH' variable
        with _execvpe_mockup() as calls:
            env_path = env.copy()
            if test_type is bytes:
                env_path[b'PATH'] = program_path
            else:
                env_path['PATH'] = program_path
            self.assertRaises(OSError,
                os._execvpe, program, arguments, env=env_path)
            self.assertEqual(len(calls), 1)
            self.assertSequenceEqual(calls[0],
                ('execve', native_fullpath, (arguments, env_path)))

    def test_internal_execvpe_str(self):
        self._test_internal_execvpe(str)
        if os.name != "nt":
            self._test_internal_execvpe(bytes)

    def test_execve_invalid_env(self):
        args = [sys.executable, '-c', 'pass']

        # null character in the environment variable name
        newenv = os.environ.copy()
        newenv["FRUIT\0VEGETABLE"] = "cabbage"
        with self.assertRaises(ValueError):
            os.execve(args[0], args, newenv)

        # null character in the environment variable value
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange\0VEGETABLE=cabbage"
        with self.assertRaises(ValueError):
            os.execve(args[0], args, newenv)

        # equal character in the environment variable name
        newenv = os.environ.copy()
        newenv["FRUIT=ORANGE"] = "lemon"
        with self.assertRaises(ValueError):
            os.execve(args[0], args, newenv)

    @unittest.skipUnless(sys.platform == "win32", "Win32-specific test")
    def test_execve_with_empty_path(self):
        # bpo-32890: Check GetLastError() misuse
        try:
            os.execve('', ['arg'], {})
        except OSError as e:
            self.assertTrue(e.winerror is None or e.winerror != 0)
        else:
            self.fail('No OSError raised')


@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32ErrorTests(unittest.TestCase):
    def setUp(self):
        try:
            os.stat(os_helper.TESTFN)
        except FileNotFoundError:
            exists = False
        except OSError as exc:
            exists = True
            self.fail("file %s must not exist; os.stat failed with %s"
                      % (os_helper.TESTFN, exc))
        else:
            self.fail("file %s must not exist" % os_helper.TESTFN)

    def test_rename(self):
        self.assertRaises(OSError, os.rename, os_helper.TESTFN, os_helper.TESTFN+".bak")

    def test_remove(self):
        self.assertRaises(OSError, os.remove, os_helper.TESTFN)

    def test_chdir(self):
        self.assertRaises(OSError, os.chdir, os_helper.TESTFN)

    def test_mkdir(self):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)

        with open(os_helper.TESTFN, "x") as f:
            self.assertRaises(OSError, os.mkdir, os_helper.TESTFN)

    def test_utime(self):
        self.assertRaises(OSError, os.utime, os_helper.TESTFN, None)

    def test_chmod(self):
        self.assertRaises(OSError, os.chmod, os_helper.TESTFN, 0)


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


@unittest.skipUnless(hasattr(os, 'link'), 'requires os.link')
class LinkTests(unittest.TestCase):
    def setUp(self):
        self.file1 = os_helper.TESTFN
        self.file2 = os.path.join(os_helper.TESTFN + "2")

    def tearDown(self):
        for file in (self.file1, self.file2):
            if os.path.exists(file):
                os.unlink(file)

    def _test_link(self, file1, file2):
        create_file(file1)

        try:
            os.link(file1, file2)
        except PermissionError as e:
            self.skipTest('os.link(): %s' % e)
        with open(file1, "rb") as f1, open(file2, "rb") as f2:
            self.assertTrue(os.path.sameopenfile(f1.fileno(), f2.fileno()))

    def test_link(self):
        self._test_link(self.file1, self.file2)

    def test_link_bytes(self):
        self._test_link(bytes(self.file1, sys.getfilesystemencoding()),
                        bytes(self.file2, sys.getfilesystemencoding()))

    def test_unicode_name(self):
        try:
            os.fsencode("\xf1")
        except UnicodeError:
            raise unittest.SkipTest("Unable to encode for this platform.")

        self.file1 += "\xf1"
        self.file2 = self.file1 + "2"
        self._test_link(self.file1, self.file2)

@unittest.skipIf(sys.platform == "win32", "Posix specific tests")
class PosixUidGidTests(unittest.TestCase):
    # uid_t and gid_t are 32-bit unsigned integers on Linux
    UID_OVERFLOW = (1 << 32)
    GID_OVERFLOW = (1 << 32)

    @unittest.skipUnless(hasattr(os, 'setuid'), 'test needs os.setuid()')
    def test_setuid(self):
        if os.getuid() != 0:
            self.assertRaises(OSError, os.setuid, 0)
        self.assertRaises(TypeError, os.setuid, 'not an int')
        self.assertRaises(OverflowError, os.setuid, self.UID_OVERFLOW)

    @unittest.skipUnless(hasattr(os, 'setgid'), 'test needs os.setgid()')
    def test_setgid(self):
        if os.getuid() != 0 and not HAVE_WHEEL_GROUP:
            self.assertRaises(OSError, os.setgid, 0)
        self.assertRaises(TypeError, os.setgid, 'not an int')
        self.assertRaises(OverflowError, os.setgid, self.GID_OVERFLOW)

    @unittest.skipUnless(hasattr(os, 'seteuid'), 'test needs os.seteuid()')
    def test_seteuid(self):
        if os.getuid() != 0:
            self.assertRaises(OSError, os.seteuid, 0)
        self.assertRaises(TypeError, os.setegid, 'not an int')
        self.assertRaises(OverflowError, os.seteuid, self.UID_OVERFLOW)

    @unittest.skipUnless(hasattr(os, 'setegid'), 'test needs os.setegid()')
    def test_setegid(self):
        if os.getuid() != 0 and not HAVE_WHEEL_GROUP:
            self.assertRaises(OSError, os.setegid, 0)
        self.assertRaises(TypeError, os.setegid, 'not an int')
        self.assertRaises(OverflowError, os.setegid, self.GID_OVERFLOW)

    @unittest.skipUnless(hasattr(os, 'setreuid'), 'test needs os.setreuid()')
    def test_setreuid(self):
        if os.getuid() != 0:
            self.assertRaises(OSError, os.setreuid, 0, 0)
        self.assertRaises(TypeError, os.setreuid, 'not an int', 0)
        self.assertRaises(TypeError, os.setreuid, 0, 'not an int')
        self.assertRaises(OverflowError, os.setreuid, self.UID_OVERFLOW, 0)
        self.assertRaises(OverflowError, os.setreuid, 0, self.UID_OVERFLOW)

    @unittest.skipUnless(hasattr(os, 'setreuid'), 'test needs os.setreuid()')
    @support.requires_subprocess()
    def test_setreuid_neg1(self):
        # Needs to accept -1.  We run this in a subprocess to avoid
        # altering the test runner's process state (issue8045).
        subprocess.check_call([
                sys.executable, '-c',
                'import os,sys;os.setreuid(-1,-1);sys.exit(0)'])

    @unittest.skipUnless(hasattr(os, 'setregid'), 'test needs os.setregid()')
    @support.requires_subprocess()
    def test_setregid(self):
        if os.getuid() != 0 and not HAVE_WHEEL_GROUP:
            self.assertRaises(OSError, os.setregid, 0, 0)
        self.assertRaises(TypeError, os.setregid, 'not an int', 0)
        self.assertRaises(TypeError, os.setregid, 0, 'not an int')
        self.assertRaises(OverflowError, os.setregid, self.GID_OVERFLOW, 0)
        self.assertRaises(OverflowError, os.setregid, 0, self.GID_OVERFLOW)

    @unittest.skipUnless(hasattr(os, 'setregid'), 'test needs os.setregid()')
    @support.requires_subprocess()
    def test_setregid_neg1(self):
        # Needs to accept -1.  We run this in a subprocess to avoid
        # altering the test runner's process state (issue8045).
        subprocess.check_call([
                sys.executable, '-c',
                'import os,sys;os.setregid(-1,-1);sys.exit(0)'])

@unittest.skipIf(sys.platform == "win32", "Posix specific tests")
class Pep383Tests(unittest.TestCase):
    def setUp(self):
        if os_helper.TESTFN_UNENCODABLE:
            self.dir = os_helper.TESTFN_UNENCODABLE
        elif os_helper.TESTFN_NONASCII:
            self.dir = os_helper.TESTFN_NONASCII
        else:
            self.dir = os_helper.TESTFN
        self.bdir = os.fsencode(self.dir)

        bytesfn = []
        def add_filename(fn):
            try:
                fn = os.fsencode(fn)
            except UnicodeEncodeError:
                return
            bytesfn.append(fn)
        add_filename(os_helper.TESTFN_UNICODE)
        if os_helper.TESTFN_UNENCODABLE:
            add_filename(os_helper.TESTFN_UNENCODABLE)
        if os_helper.TESTFN_NONASCII:
            add_filename(os_helper.TESTFN_NONASCII)
        if not bytesfn:
            self.skipTest("couldn't create any non-ascii filename")

        self.unicodefn = set()
        os.mkdir(self.dir)
        try:
            for fn in bytesfn:
                os_helper.create_empty_file(os.path.join(self.bdir, fn))
                fn = os.fsdecode(fn)
                if fn in self.unicodefn:
                    raise ValueError("duplicate filename")
                self.unicodefn.add(fn)
        except:
            shutil.rmtree(self.dir)
            raise

    def tearDown(self):
        shutil.rmtree(self.dir)

    def test_listdir(self):
        expected = self.unicodefn
        found = set(os.listdir(self.dir))
        self.assertEqual(found, expected)
        # test listdir without arguments
        current_directory = os.getcwd()
        try:
            # The root directory is not readable on Android, so use a directory
            # we created ourselves.
            os.chdir(self.dir)
            self.assertEqual(set(os.listdir()), expected)
        finally:
            os.chdir(current_directory)

    def test_open(self):
        for fn in self.unicodefn:
            f = open(os.path.join(self.dir, fn), 'rb')
            f.close()

    @unittest.skipUnless(hasattr(os, 'statvfs'),
                            "need os.statvfs()")
    def test_statvfs(self):
        # issue #9645
        for fn in self.unicodefn:
            # should not fail with file not found error
            fullname = os.path.join(self.dir, fn)
            os.statvfs(fullname)

    def test_stat(self):
        for fn in self.unicodefn:
            os.stat(os.path.join(self.dir, fn))

@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32KillTests(unittest.TestCase):
    def _kill(self, sig):
        # Start sys.executable as a subprocess and communicate from the
        # subprocess to the parent that the interpreter is ready. When it
        # becomes ready, send *sig* via os.kill to the subprocess and check
        # that the return code is equal to *sig*.
        import ctypes
        from ctypes import wintypes
        import msvcrt

        # Since we can't access the contents of the process' stdout until the
        # process has exited, use PeekNamedPipe to see what's inside stdout
        # without waiting. This is done so we can tell that the interpreter
        # is started and running at a point where it could handle a signal.
        PeekNamedPipe = ctypes.windll.kernel32.PeekNamedPipe
        PeekNamedPipe.restype = wintypes.BOOL
        PeekNamedPipe.argtypes = (wintypes.HANDLE, # Pipe handle
                                  ctypes.POINTER(ctypes.c_char), # stdout buf
                                  wintypes.DWORD, # Buffer size
                                  ctypes.POINTER(wintypes.DWORD), # bytes read
                                  ctypes.POINTER(wintypes.DWORD), # bytes avail
                                  ctypes.POINTER(wintypes.DWORD)) # bytes left
        msg = "running"
        proc = subprocess.Popen([sys.executable, "-c",
                                 "import sys;"
                                 "sys.stdout.write('{}');"
                                 "sys.stdout.flush();"
                                 "input()".format(msg)],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                stdin=subprocess.PIPE)
        self.addCleanup(proc.stdout.close)
        self.addCleanup(proc.stderr.close)
        self.addCleanup(proc.stdin.close)

        count, max = 0, 100
        while count < max and proc.poll() is None:
            # Create a string buffer to store the result of stdout from the pipe
            buf = ctypes.create_string_buffer(len(msg))
            # Obtain the text currently in proc.stdout
            # Bytes read/avail/left are left as NULL and unused
            rslt = PeekNamedPipe(msvcrt.get_osfhandle(proc.stdout.fileno()),
                                 buf, ctypes.sizeof(buf), None, None, None)
            self.assertNotEqual(rslt, 0, "PeekNamedPipe failed")
            if buf.value:
                self.assertEqual(msg, buf.value.decode())
                break
            time.sleep(0.1)
            count += 1
        else:
            self.fail("Did not receive communication from the subprocess")

        os.kill(proc.pid, sig)
        self.assertEqual(proc.wait(), sig)

    def test_kill_sigterm(self):
        # SIGTERM doesn't mean anything special, but make sure it works
        self._kill(signal.SIGTERM)

    def test_kill_int(self):
        # os.kill on Windows can take an int which gets set as the exit code
        self._kill(100)

    @unittest.skipIf(mmap is None, "requires mmap")
    def _kill_with_event(self, event, name):
        tagname = "test_os_%s" % uuid.uuid1()
        m = mmap.mmap(-1, 1, tagname)
        m[0] = 0

        # Run a script which has console control handling enabled.
        script = os.path.join(os.path.dirname(__file__),
                              "win_console_handler.py")
        cmd = [sys.executable, script, tagname]
        proc = subprocess.Popen(cmd,
                                creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)

        with proc:
            # Let the interpreter startup before we send signals. See #3137.
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if proc.poll() is None:
                    break
            else:
                # Forcefully kill the process if we weren't able to signal it.
                proc.kill()
                self.fail("Subprocess didn't finish initialization")

            os.kill(proc.pid, event)

            try:
                # proc.send_signal(event) could also be done here.
                # Allow time for the signal to be passed and the process to exit.
                proc.wait(timeout=support.SHORT_TIMEOUT)
            except subprocess.TimeoutExpired:
                # Forcefully kill the process if we weren't able to signal it.
                proc.kill()
                self.fail("subprocess did not stop on {}".format(name))

    @unittest.skip("subprocesses aren't inheriting Ctrl+C property")
    @support.requires_subprocess()
    def test_CTRL_C_EVENT(self):
        from ctypes import wintypes
        import ctypes

        # Make a NULL value by creating a pointer with no argument.
        NULL = ctypes.POINTER(ctypes.c_int)()
        SetConsoleCtrlHandler = ctypes.windll.kernel32.SetConsoleCtrlHandler
        SetConsoleCtrlHandler.argtypes = (ctypes.POINTER(ctypes.c_int),
                                          wintypes.BOOL)
        SetConsoleCtrlHandler.restype = wintypes.BOOL

        # Calling this with NULL and FALSE causes the calling process to
        # handle Ctrl+C, rather than ignore it. This property is inherited
        # by subprocesses.
        SetConsoleCtrlHandler(NULL, 0)

        self._kill_with_event(signal.CTRL_C_EVENT, "CTRL_C_EVENT")

    @support.requires_subprocess()
    def test_CTRL_BREAK_EVENT(self):
        self._kill_with_event(signal.CTRL_BREAK_EVENT, "CTRL_BREAK_EVENT")


@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32ListdirTests(unittest.TestCase):
    """Test listdir on Windows."""

    def setUp(self):
        self.created_paths = []
        for i in range(2):
            dir_name = 'SUB%d' % i
            dir_path = os.path.join(os_helper.TESTFN, dir_name)
            file_name = 'FILE%d' % i
            file_path = os.path.join(os_helper.TESTFN, file_name)
            os.makedirs(dir_path)
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write("I'm %s and proud of it. Blame test_os.\n" % file_path)
            self.created_paths.extend([dir_name, file_name])
        self.created_paths.sort()

    def tearDown(self):
        shutil.rmtree(os_helper.TESTFN)

    def test_listdir_no_extended_path(self):
        """Test when the path is not an "extended" path."""
        # unicode
        self.assertEqual(
                sorted(os.listdir(os_helper.TESTFN)),
                self.created_paths)

        # bytes
        self.assertEqual(
                sorted(os.listdir(os.fsencode(os_helper.TESTFN))),
                [os.fsencode(path) for path in self.created_paths])

    def test_listdir_extended_path(self):
        """Test when the path starts with '\\\\?\\'."""
        # See: http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#maxpath
        # unicode
        path = '\\\\?\\' + os.path.abspath(os_helper.TESTFN)
        self.assertEqual(
                sorted(os.listdir(path)),
                self.created_paths)

        # bytes
        path = b'\\\\?\\' + os.fsencode(os.path.abspath(os_helper.TESTFN))
        self.assertEqual(
                sorted(os.listdir(path)),
                [os.fsencode(path) for path in self.created_paths])


@unittest.skipUnless(os.name == "nt", "NT specific tests")
class Win32ListdriveTests(unittest.TestCase):
    """Test listdrive, listmounts and listvolume on Windows."""

    def setUp(self):
        # Get drives and volumes from fsutil
        out = subprocess.check_output(
            ["fsutil.exe", "volume", "list"],
            cwd=os.path.join(os.getenv("SystemRoot", "\\Windows"), "System32"),
            encoding="mbcs",
            errors="ignore",
        )
        lines = out.splitlines()
        self.known_volumes = {l for l in lines if l.startswith('\\\\?\\')}
        self.known_drives = {l for l in lines if l[1:] == ':\\'}
        self.known_mounts = {l for l in lines if l[1:3] == ':\\'}

    def test_listdrives(self):
        drives = os.listdrives()
        self.assertIsInstance(drives, list)
        self.assertSetEqual(
            self.known_drives,
            self.known_drives & set(drives),
        )

    def test_listvolumes(self):
        volumes = os.listvolumes()
        self.assertIsInstance(volumes, list)
        self.assertSetEqual(
            self.known_volumes,
            self.known_volumes & set(volumes),
        )

    def test_listmounts(self):
        for volume in os.listvolumes():
            try:
                mounts = os.listmounts(volume)
            except OSError as ex:
                if support.verbose:
                    print("Skipping", volume, "because of", ex)
            else:
                self.assertIsInstance(mounts, list)
                self.assertSetEqual(
                    set(mounts),
                    self.known_mounts & set(mounts),
                )


@unittest.skipUnless(hasattr(os, 'readlink'), 'needs os.readlink()')
class ReadlinkTests(unittest.TestCase):
    filelink = 'readlinktest'
    filelink_target = os.path.abspath(__file__)
    filelinkb = os.fsencode(filelink)
    filelinkb_target = os.fsencode(filelink_target)

    def assertPathEqual(self, left, right):
        left = os.path.normcase(left)
        right = os.path.normcase(right)
        if sys.platform == 'win32':
            # Bad practice to blindly strip the prefix as it may be required to
            # correctly refer to the file, but we're only comparing paths here.
            has_prefix = lambda p: p.startswith(
                b'\\\\?\\' if isinstance(p, bytes) else '\\\\?\\')
            if has_prefix(left):
                left = left[4:]
            if has_prefix(right):
                right = right[4:]
        self.assertEqual(left, right)

    def setUp(self):
        self.assertTrue(os.path.exists(self.filelink_target))
        self.assertTrue(os.path.exists(self.filelinkb_target))
        self.assertFalse(os.path.exists(self.filelink))
        self.assertFalse(os.path.exists(self.filelinkb))

    def test_not_symlink(self):
        filelink_target = FakePath(self.filelink_target)
        self.assertRaises(OSError, os.readlink, self.filelink_target)
        self.assertRaises(OSError, os.readlink, filelink_target)

    def test_missing_link(self):
        self.assertRaises(FileNotFoundError, os.readlink, 'missing-link')
        self.assertRaises(FileNotFoundError, os.readlink,
                          FakePath('missing-link'))

    @os_helper.skip_unless_symlink
    def test_pathlike(self):
        os.symlink(self.filelink_target, self.filelink)
        self.addCleanup(os_helper.unlink, self.filelink)
        filelink = FakePath(self.filelink)
        self.assertPathEqual(os.readlink(filelink), self.filelink_target)

    @os_helper.skip_unless_symlink
    def test_pathlike_bytes(self):
        os.symlink(self.filelinkb_target, self.filelinkb)
        self.addCleanup(os_helper.unlink, self.filelinkb)
        path = os.readlink(FakePath(self.filelinkb))
        self.assertPathEqual(path, self.filelinkb_target)
        self.assertIsInstance(path, bytes)

    @os_helper.skip_unless_symlink
    def test_bytes(self):
        os.symlink(self.filelinkb_target, self.filelinkb)
        self.addCleanup(os_helper.unlink, self.filelinkb)
        path = os.readlink(self.filelinkb)
        self.assertPathEqual(path, self.filelinkb_target)
        self.assertIsInstance(path, bytes)


@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
@os_helper.skip_unless_symlink
class Win32SymlinkTests(unittest.TestCase):
    filelink = 'filelinktest'
    filelink_target = os.path.abspath(__file__)
    dirlink = 'dirlinktest'
    dirlink_target = os.path.dirname(filelink_target)
    missing_link = 'missing link'

    def setUp(self):
        assert os.path.exists(self.dirlink_target)
        assert os.path.exists(self.filelink_target)
        assert not os.path.exists(self.dirlink)
        assert not os.path.exists(self.filelink)
        assert not os.path.exists(self.missing_link)

    def tearDown(self):
        if os.path.exists(self.filelink):
            os.remove(self.filelink)
        if os.path.exists(self.dirlink):
            os.rmdir(self.dirlink)
        if os.path.lexists(self.missing_link):
            os.remove(self.missing_link)

    def test_directory_link(self):
        os.symlink(self.dirlink_target, self.dirlink)
        self.assertTrue(os.path.exists(self.dirlink))
        self.assertTrue(os.path.isdir(self.dirlink))
        self.assertTrue(os.path.islink(self.dirlink))
        self.check_stat(self.dirlink, self.dirlink_target)

    def test_file_link(self):
        os.symlink(self.filelink_target, self.filelink)
        self.assertTrue(os.path.exists(self.filelink))
        self.assertTrue(os.path.isfile(self.filelink))
        self.assertTrue(os.path.islink(self.filelink))
        self.check_stat(self.filelink, self.filelink_target)

    def _create_missing_dir_link(self):
        'Create a "directory" link to a non-existent target'
        linkname = self.missing_link
        if os.path.lexists(linkname):
            os.remove(linkname)
        target = r'c:\\target does not exist.29r3c740'
        assert not os.path.exists(target)
        target_is_dir = True
        os.symlink(target, linkname, target_is_dir)

    def test_remove_directory_link_to_missing_target(self):
        self._create_missing_dir_link()
        # For compatibility with Unix, os.remove will check the
        #  directory status and call RemoveDirectory if the symlink
        #  was created with target_is_dir==True.
        os.remove(self.missing_link)

    def test_isdir_on_directory_link_to_missing_target(self):
        self._create_missing_dir_link()
        self.assertFalse(os.path.isdir(self.missing_link))

    def test_rmdir_on_directory_link_to_missing_target(self):
        self._create_missing_dir_link()
        os.rmdir(self.missing_link)

    def check_stat(self, link, target):
        self.assertEqual(os.stat(link), os.stat(target))
        self.assertNotEqual(os.lstat(link), os.stat(link))

        bytes_link = os.fsencode(link)
        self.assertEqual(os.stat(bytes_link), os.stat(target))
        self.assertNotEqual(os.lstat(bytes_link), os.stat(bytes_link))

    def test_12084(self):
        level1 = os.path.abspath(os_helper.TESTFN)
        level2 = os.path.join(level1, "level2")
        level3 = os.path.join(level2, "level3")
        self.addCleanup(os_helper.rmtree, level1)

        os.mkdir(level1)
        os.mkdir(level2)
        os.mkdir(level3)

        file1 = os.path.abspath(os.path.join(level1, "file1"))
        create_file(file1)

        orig_dir = os.getcwd()
        try:
            os.chdir(level2)
            link = os.path.join(level2, "link")
            os.symlink(os.path.relpath(file1), "link")
            self.assertIn("link", os.listdir(os.getcwd()))

            # Check os.stat calls from the same dir as the link
            self.assertEqual(os.stat(file1), os.stat("link"))

            # Check os.stat calls from a dir below the link
            os.chdir(level1)
            self.assertEqual(os.stat(file1),
                             os.stat(os.path.relpath(link)))

            # Check os.stat calls from a dir above the link
            os.chdir(level3)
            self.assertEqual(os.stat(file1),
                             os.stat(os.path.relpath(link)))
        finally:
            os.chdir(orig_dir)

    @unittest.skipUnless(os.path.lexists(r'C:\Users\All Users')
                            and os.path.exists(r'C:\ProgramData'),
                            'Test directories not found')
    def test_29248(self):
        # os.symlink() calls CreateSymbolicLink, which creates
        # the reparse data buffer with the print name stored
        # first, so the offset is always 0. CreateSymbolicLink
        # stores the "PrintName" DOS path (e.g. "C:\") first,
        # with an offset of 0, followed by the "SubstituteName"
        # NT path (e.g. "\??\C:\"). The "All Users" link, on
        # the other hand, seems to have been created manually
        # with an inverted order.
        target = os.readlink(r'C:\Users\All Users')
        self.assertTrue(os.path.samefile(target, r'C:\ProgramData'))

    def test_buffer_overflow(self):
        # Older versions would have a buffer overflow when detecting
        # whether a link source was a directory. This test ensures we
        # no longer crash, but does not otherwise validate the behavior
        segment = 'X' * 27
        path = os.path.join(*[segment] * 10)
        test_cases = [
            # overflow with absolute src
            ('\\' + path, segment),
            # overflow dest with relative src
            (segment, path),
            # overflow when joining src
            (path[:180], path[:180]),
        ]
        for src, dest in test_cases:
            try:
                os.symlink(src, dest)
            except FileNotFoundError:
                pass
            else:
                try:
                    os.remove(dest)
                except OSError:
                    pass
            # Also test with bytes, since that is a separate code path.
            try:
                os.symlink(os.fsencode(src), os.fsencode(dest))
            except FileNotFoundError:
                pass
            else:
                try:
                    os.remove(dest)
                except OSError:
                    pass

    def test_appexeclink(self):
        root = os.path.expandvars(r'%LOCALAPPDATA%\Microsoft\WindowsApps')
        if not os.path.isdir(root):
            self.skipTest("test requires a WindowsApps directory")

        aliases = [os.path.join(root, a)
                   for a in fnmatch.filter(os.listdir(root), '*.exe')]

        for alias in aliases:
            if support.verbose:
                print()
                print("Testing with", alias)
            st = os.lstat(alias)
            self.assertEqual(st, os.stat(alias))
            self.assertFalse(stat.S_ISLNK(st.st_mode))
            self.assertEqual(st.st_reparse_tag, stat.IO_REPARSE_TAG_APPEXECLINK)
            self.assertTrue(os.path.isfile(alias))
            # testing the first one we see is sufficient
            break
        else:
            self.skipTest("test requires an app execution alias")

@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32JunctionTests(unittest.TestCase):
    junction = 'junctiontest'
    junction_target = os.path.dirname(os.path.abspath(__file__))

    def setUp(self):
        assert os.path.exists(self.junction_target)
        assert not os.path.lexists(self.junction)

    def tearDown(self):
        if os.path.lexists(self.junction):
            os.unlink(self.junction)

    def test_create_junction(self):
        _winapi.CreateJunction(self.junction_target, self.junction)
        self.assertTrue(os.path.lexists(self.junction))
        self.assertTrue(os.path.exists(self.junction))
        self.assertTrue(os.path.isdir(self.junction))
        self.assertNotEqual(os.stat(self.junction), os.lstat(self.junction))
        self.assertEqual(os.stat(self.junction), os.stat(self.junction_target))

        # bpo-37834: Junctions are not recognized as links.
        self.assertFalse(os.path.islink(self.junction))
        self.assertEqual(os.path.normcase("\\\\?\\" + self.junction_target),
                         os.path.normcase(os.readlink(self.junction)))

    def test_unlink_removes_junction(self):
        _winapi.CreateJunction(self.junction_target, self.junction)
        self.assertTrue(os.path.exists(self.junction))
        self.assertTrue(os.path.lexists(self.junction))

        os.unlink(self.junction)
        self.assertFalse(os.path.exists(self.junction))

@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32NtTests(unittest.TestCase):
    def test_getfinalpathname_handles(self):
        nt = import_helper.import_module('nt')
        ctypes = import_helper.import_module('ctypes')
        # Ruff false positive -- it thinks we're redefining `ctypes` here
        import ctypes.wintypes  # noqa: F811

        kernel = ctypes.WinDLL('Kernel32.dll', use_last_error=True)
        kernel.GetCurrentProcess.restype = ctypes.wintypes.HANDLE

        kernel.GetProcessHandleCount.restype = ctypes.wintypes.BOOL
        kernel.GetProcessHandleCount.argtypes = (ctypes.wintypes.HANDLE,
                                                 ctypes.wintypes.LPDWORD)

        # This is a pseudo-handle that doesn't need to be closed
        hproc = kernel.GetCurrentProcess()

        handle_count = ctypes.wintypes.DWORD()
        ok = kernel.GetProcessHandleCount(hproc, ctypes.byref(handle_count))
        self.assertEqual(1, ok)

        before_count = handle_count.value

        # The first two test the error path, __file__ tests the success path
        filenames = [
            r'\\?\C:',
            r'\\?\NUL',
            r'\\?\CONIN',
            __file__,
        ]

        for _ in range(10):
            for name in filenames:
                try:
                    nt._getfinalpathname(name)
                except Exception:
                    # Failure is expected
                    pass
                try:
                    os.stat(name)
                except Exception:
                    pass

        ok = kernel.GetProcessHandleCount(hproc, ctypes.byref(handle_count))
        self.assertEqual(1, ok)

        handle_delta = handle_count.value - before_count

        self.assertEqual(0, handle_delta)

    @support.requires_subprocess()
    def test_stat_unlink_race(self):
        # bpo-46785: the implementation of os.stat() falls back to reading
        # the parent directory if CreateFileW() fails with a permission
        # error. If reading the parent directory fails because the file or
        # directory are subsequently unlinked, or because the volume or
        # share are no longer available, then the original permission error
        # should not be restored.
        filename =  os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)
        deadline = time.time() + 5
        command = textwrap.dedent("""\
            import os
            import sys
            import time

            filename = sys.argv[1]
            deadline = float(sys.argv[2])

            while time.time() < deadline:
                try:
                    with open(filename, "w") as f:
                        pass
                except OSError:
                    pass
                try:
                    os.remove(filename)
                except OSError:
                    pass
            """)

        with subprocess.Popen([sys.executable, '-c', command, filename, str(deadline)]) as proc:
            while time.time() < deadline:
                try:
                    os.stat(filename)
                except FileNotFoundError as e:
                    assert e.winerror == 2  # ERROR_FILE_NOT_FOUND
            try:
                proc.wait(1)
            except subprocess.TimeoutExpired:
                proc.terminate()

    @support.requires_subprocess()
    def test_stat_inaccessible_file(self):
        filename = os_helper.TESTFN
        ICACLS = os.path.expandvars(r"%SystemRoot%\System32\icacls.exe")

        with open(filename, "wb") as f:
            f.write(b'Test data')

        stat1 = os.stat(filename)

        try:
            # Remove all permissions from the file
            subprocess.check_output([ICACLS, filename, "/inheritance:r"],
                                    stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as ex:
            if support.verbose:
                print(ICACLS, filename, "/inheritance:r", "failed.")
                print(ex.stdout.decode("oem", "replace").rstrip())
            try:
                os.unlink(filename)
            except OSError:
                pass
            self.skipTest("Unable to create inaccessible file")

        def cleanup():
            # Give delete permission to the owner (us)
            subprocess.check_output([ICACLS, filename, "/grant", "*WD:(D)"],
                                    stderr=subprocess.STDOUT)
            os.unlink(filename)

        self.addCleanup(cleanup)

        if support.verbose:
            print("File:", filename)
            print("stat with access:", stat1)

        # First test - we shouldn't raise here, because we still have access to
        # the directory and can extract enough information from its metadata.
        stat2 = os.stat(filename)

        if support.verbose:
            print(" without access:", stat2)

        # We may not get st_dev/st_ino, so ensure those are 0 or match
        self.assertIn(stat2.st_dev, (0, stat1.st_dev))
        self.assertIn(stat2.st_ino, (0, stat1.st_ino))

        # st_mode and st_size should match (for a normal file, at least)
        self.assertEqual(stat1.st_mode, stat2.st_mode)
        self.assertEqual(stat1.st_size, stat2.st_size)

        # st_ctime and st_mtime should be the same
        self.assertEqual(stat1.st_ctime, stat2.st_ctime)
        self.assertEqual(stat1.st_mtime, stat2.st_mtime)

        # st_atime should be the same or later
        self.assertGreaterEqual(stat1.st_atime, stat2.st_atime)


@os_helper.skip_unless_symlink
class NonLocalSymlinkTests(unittest.TestCase):

    def setUp(self):
        r"""
        Create this structure:

        base
         \___ some_dir
        """
        os.makedirs('base/some_dir')

    def tearDown(self):
        shutil.rmtree('base')

    def test_directory_link_nonlocal(self):
        """
        The symlink target should resolve relative to the link, not relative
        to the current directory.

        Then, link base/some_link -> base/some_dir and ensure that some_link
        is resolved as a directory.

        In issue13772, it was discovered that directory detection failed if
        the symlink target was not specified relative to the current
        directory, which was a defect in the implementation.
        """
        src = os.path.join('base', 'some_link')
        os.symlink('some_dir', src)
        assert os.path.isdir(src)


class FSEncodingTests(unittest.TestCase):
    def test_nop(self):
        self.assertEqual(os.fsencode(b'abc\xff'), b'abc\xff')
        self.assertEqual(os.fsdecode('abc\u0141'), 'abc\u0141')

    def test_identity(self):
        # assert fsdecode(fsencode(x)) == x
        for fn in ('unicode\u0141', 'latin\xe9', 'ascii'):
            try:
                bytesfn = os.fsencode(fn)
            except UnicodeEncodeError:
                continue
            self.assertEqual(os.fsdecode(bytesfn), fn)



class DeviceEncodingTests(unittest.TestCase):

    def test_bad_fd(self):
        # Return None when an fd doesn't actually exist.
        self.assertIsNone(os.device_encoding(123456))

    @unittest.skipUnless(os.isatty(0) and not win32_is_iot() and (sys.platform.startswith('win') or
            (hasattr(locale, 'nl_langinfo') and hasattr(locale, 'CODESET'))),
            'test requires a tty and either Windows or nl_langinfo(CODESET)')
    def test_device_encoding(self):
        encoding = os.device_encoding(0)
        self.assertIsNotNone(encoding)
        self.assertTrue(codecs.lookup(encoding))


@support.requires_subprocess()
class PidTests(unittest.TestCase):
    @unittest.skipUnless(hasattr(os, 'getppid'), "test needs os.getppid")
    def test_getppid(self):
        p = subprocess.Popen([sys._base_executable, '-c',
                              'import os; print(os.getppid())'],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        stdout, error = p.communicate()
        # We are the parent of our subprocess
        self.assertEqual(error, b'')
        self.assertEqual(int(stdout), os.getpid())

    def check_waitpid(self, code, exitcode, callback=None):
        if sys.platform == 'win32':
            # On Windows, os.spawnv() simply joins arguments with spaces:
            # arguments need to be quoted
            args = [f'"{sys.executable}"', '-c', f'"{code}"']
        else:
            args = [sys.executable, '-c', code]
        pid = os.spawnv(os.P_NOWAIT, sys.executable, args)

        if callback is not None:
            callback(pid)

        # don't use support.wait_process() to test directly os.waitpid()
        # and os.waitstatus_to_exitcode()
        pid2, status = os.waitpid(pid, 0)
        self.assertEqual(os.waitstatus_to_exitcode(status), exitcode)
        self.assertEqual(pid2, pid)

    def test_waitpid(self):
        self.check_waitpid(code='pass', exitcode=0)

    def test_waitstatus_to_exitcode(self):
        exitcode = 23
        code = f'import sys; sys.exit({exitcode})'
        self.check_waitpid(code, exitcode=exitcode)

        with self.assertRaises(TypeError):
            os.waitstatus_to_exitcode(0.0)

    @unittest.skipUnless(sys.platform == 'win32', 'win32-specific test')
    def test_waitpid_windows(self):
        # bpo-40138: test os.waitpid() and os.waitstatus_to_exitcode()
        # with exit code larger than INT_MAX.
        STATUS_CONTROL_C_EXIT = 0xC000013A
        code = f'import _winapi; _winapi.ExitProcess({STATUS_CONTROL_C_EXIT})'
        self.check_waitpid(code, exitcode=STATUS_CONTROL_C_EXIT)

    @unittest.skipUnless(sys.platform == 'win32', 'win32-specific test')
    def test_waitstatus_to_exitcode_windows(self):
        max_exitcode = 2 ** 32 - 1
        for exitcode in (0, 1, 5, max_exitcode):
            self.assertEqual(os.waitstatus_to_exitcode(exitcode << 8),
                             exitcode)

        # invalid values
        with self.assertRaises(ValueError):
            os.waitstatus_to_exitcode((max_exitcode + 1) << 8)
        with self.assertRaises(OverflowError):
            os.waitstatus_to_exitcode(-1)

    # Skip the test on Windows
    @unittest.skipUnless(hasattr(signal, 'SIGKILL'), 'need signal.SIGKILL')
    def test_waitstatus_to_exitcode_kill(self):
        code = f'import time; time.sleep({support.LONG_TIMEOUT})'
        signum = signal.SIGKILL

        def kill_process(pid):
            os.kill(pid, signum)

        self.check_waitpid(code, exitcode=-signum, callback=kill_process)


@support.requires_subprocess()
class SpawnTests(unittest.TestCase):
    @staticmethod
    def quote_args(args):
        # On Windows, os.spawn* simply joins arguments with spaces:
        # arguments need to be quoted
        if os.name != 'nt':
            return args
        return [f'"{arg}"' if " " in arg.strip() else arg for arg in args]

    def create_args(self, *, with_env=False, use_bytes=False):
        self.exitcode = 17

        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        if not with_env:
            code = 'import sys; sys.exit(%s)' % self.exitcode
        else:
            self.env = dict(os.environ)
            # create an unique key
            self.key = str(uuid.uuid4())
            self.env[self.key] = self.key
            # read the variable from os.environ to check that it exists
            code = ('import sys, os; magic = os.environ[%r]; sys.exit(%s)'
                    % (self.key, self.exitcode))

        with open(filename, "w", encoding="utf-8") as fp:
            fp.write(code)

        program = sys.executable
        args = self.quote_args([program, filename])
        if use_bytes:
            program = os.fsencode(program)
            args = [os.fsencode(a) for a in args]
            self.env = {os.fsencode(k): os.fsencode(v)
                        for k, v in self.env.items()}

        return program, args

    @requires_os_func('spawnl')
    def test_spawnl(self):
        program, args = self.create_args()
        exitcode = os.spawnl(os.P_WAIT, program, *args)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnle')
    def test_spawnle(self):
        program, args = self.create_args(with_env=True)
        exitcode = os.spawnle(os.P_WAIT, program, *args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnlp')
    def test_spawnlp(self):
        program, args = self.create_args()
        exitcode = os.spawnlp(os.P_WAIT, program, *args)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnlpe')
    def test_spawnlpe(self):
        program, args = self.create_args(with_env=True)
        exitcode = os.spawnlpe(os.P_WAIT, program, *args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnv')
    def test_spawnv(self):
        program, args = self.create_args()
        exitcode = os.spawnv(os.P_WAIT, program, args)
        self.assertEqual(exitcode, self.exitcode)

        # Test for PyUnicode_FSConverter()
        exitcode = os.spawnv(os.P_WAIT, FakePath(program), args)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnve')
    def test_spawnve(self):
        program, args = self.create_args(with_env=True)
        exitcode = os.spawnve(os.P_WAIT, program, args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnvp')
    def test_spawnvp(self):
        program, args = self.create_args()
        exitcode = os.spawnvp(os.P_WAIT, program, args)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnvpe')
    def test_spawnvpe(self):
        program, args = self.create_args(with_env=True)
        exitcode = os.spawnvpe(os.P_WAIT, program, args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnv')
    def test_nowait(self):
        program, args = self.create_args()
        pid = os.spawnv(os.P_NOWAIT, program, args)
        support.wait_process(pid, exitcode=self.exitcode)

    @requires_os_func('spawnve')
    def test_spawnve_bytes(self):
        # Test bytes handling in parse_arglist and parse_envlist (#28114)
        program, args = self.create_args(with_env=True, use_bytes=True)
        exitcode = os.spawnve(os.P_WAIT, program, args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnl')
    def test_spawnl_noargs(self):
        program, __ = self.create_args()
        self.assertRaises(ValueError, os.spawnl, os.P_NOWAIT, program)
        self.assertRaises(ValueError, os.spawnl, os.P_NOWAIT, program, '')

    @requires_os_func('spawnle')
    def test_spawnle_noargs(self):
        program, __ = self.create_args()
        self.assertRaises(ValueError, os.spawnle, os.P_NOWAIT, program, {})
        self.assertRaises(ValueError, os.spawnle, os.P_NOWAIT, program, '', {})

    @requires_os_func('spawnv')
    def test_spawnv_noargs(self):
        program, __ = self.create_args()
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, program, ())
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, program, [])
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, program, ('',))
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, program, [''])

    @requires_os_func('spawnve')
    def test_spawnve_noargs(self):
        program, __ = self.create_args()
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, program, (), {})
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, program, [], {})
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, program, ('',), {})
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, program, [''], {})

    def _test_invalid_env(self, spawn):
        program = sys.executable
        args = self.quote_args([program, '-c', 'pass'])

        # null character in the environment variable name
        newenv = os.environ.copy()
        newenv["FRUIT\0VEGETABLE"] = "cabbage"
        try:
            exitcode = spawn(os.P_WAIT, program, args, newenv)
        except ValueError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # null character in the environment variable value
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange\0VEGETABLE=cabbage"
        try:
            exitcode = spawn(os.P_WAIT, program, args, newenv)
        except ValueError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # equal character in the environment variable name
        newenv = os.environ.copy()
        newenv["FRUIT=ORANGE"] = "lemon"
        try:
            exitcode = spawn(os.P_WAIT, program, args, newenv)
        except ValueError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # equal character in the environment variable value
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)
        with open(filename, "w", encoding="utf-8") as fp:
            fp.write('import sys, os\n'
                     'if os.getenv("FRUIT") != "orange=lemon":\n'
                     '    raise AssertionError')

        args = self.quote_args([program, filename])
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange=lemon"
        exitcode = spawn(os.P_WAIT, program, args, newenv)
        self.assertEqual(exitcode, 0)

    @requires_os_func('spawnve')
    def test_spawnve_invalid_env(self):
        self._test_invalid_env(os.spawnve)

    @requires_os_func('spawnvpe')
    def test_spawnvpe_invalid_env(self):
        self._test_invalid_env(os.spawnvpe)


# The introduction of this TestCase caused at least two different errors on
# *nix buildbots. Temporarily skip this to let the buildbots move along.
@unittest.skip("Skip due to platform/environment differences on *NIX buildbots")
@unittest.skipUnless(hasattr(os, 'getlogin'), "test needs os.getlogin")
class LoginTests(unittest.TestCase):
    def test_getlogin(self):
        user_name = os.getlogin()
        self.assertNotEqual(len(user_name), 0)


@unittest.skipUnless(hasattr(os, 'getpriority') and hasattr(os, 'setpriority'),
                     "needs os.getpriority and os.setpriority")
class ProgramPriorityTests(unittest.TestCase):
    """Tests for os.getpriority() and os.setpriority()."""

    def test_set_get_priority(self):
        base = os.getpriority(os.PRIO_PROCESS, os.getpid())
        code = f"""if 1:
        import os
        os.setpriority(os.PRIO_PROCESS, os.getpid(), {base} + 1)
        print(os.getpriority(os.PRIO_PROCESS, os.getpid()))
        """

        # Subprocess inherits the current process' priority.
        _, out, _ = assert_python_ok("-c", code)
        new_prio = int(out)
        # nice value cap is 19 for linux and 20 for FreeBSD
        if base >= 19 and new_prio <= base:
            raise unittest.SkipTest("unable to reliably test setpriority "
                                    "at current nice level of %s" % base)
        else:
            self.assertEqual(new_prio, base + 1)


@unittest.skipUnless(hasattr(os, 'sendfile'), "test needs os.sendfile()")
class TestSendfile(unittest.IsolatedAsyncioTestCase):

    DATA = b"12345abcde" * 16 * 1024  # 160 KiB
    SUPPORT_HEADERS_TRAILERS = (
        not sys.platform.startswith(("linux", "android", "solaris", "sunos")))
    requires_headers_trailers = unittest.skipUnless(SUPPORT_HEADERS_TRAILERS,
            'requires headers and trailers support')
    requires_32b = unittest.skipUnless(sys.maxsize < 2**32,
            'test is only meaningful on 32-bit builds')

    @classmethod
    def setUpClass(cls):
        create_file(os_helper.TESTFN, cls.DATA)

    @classmethod
    def tearDownClass(cls):
        os_helper.unlink(os_helper.TESTFN)

    @staticmethod
    async def chunks(reader):
        while not reader.at_eof():
            yield await reader.read()

    async def handle_new_client(self, reader, writer):
        self.server_buffer = b''.join([x async for x in self.chunks(reader)])
        writer.close()
        self.server.close()  # The test server processes a single client only

    async def asyncSetUp(self):
        self.server_buffer = b''
        self.server = await asyncio.start_server(self.handle_new_client,
                                                 socket_helper.HOSTv4)
        server_name = self.server.sockets[0].getsockname()
        self.client = socket.socket()
        self.client.setblocking(False)
        await asyncio.get_running_loop().sock_connect(self.client, server_name)
        self.sockno = self.client.fileno()
        self.file = open(os_helper.TESTFN, 'rb')
        self.fileno = self.file.fileno()

    async def asyncTearDown(self):
        self.file.close()
        self.client.close()
        await self.server.wait_closed()

    # Use the test subject instead of asyncio.loop.sendfile
    @staticmethod
    async def async_sendfile(*args, **kwargs):
        return await asyncio.to_thread(os.sendfile, *args, **kwargs)

    @staticmethod
    async def sendfile_wrapper(*args, **kwargs):
        """A higher level wrapper representing how an application is
        supposed to use sendfile().
        """
        while True:
            try:
                return await TestSendfile.async_sendfile(*args, **kwargs)
            except OSError as err:
                if err.errno == errno.ECONNRESET:
                    # disconnected
                    raise
                elif err.errno in (errno.EAGAIN, errno.EBUSY):
                    # we have to retry send data
                    continue
                else:
                    raise

    async def test_send_whole_file(self):
        # normal send
        total_sent = 0
        offset = 0
        nbytes = 4096
        while total_sent < len(self.DATA):
            sent = await self.sendfile_wrapper(self.sockno, self.fileno,
                                               offset, nbytes)
            if sent == 0:
                break
            offset += sent
            total_sent += sent
            self.assertTrue(sent <= nbytes)
            self.assertEqual(offset, total_sent)

        self.assertEqual(total_sent, len(self.DATA))
        self.client.shutdown(socket.SHUT_RDWR)
        self.client.close()
        await self.server.wait_closed()
        self.assertEqual(len(self.server_buffer), len(self.DATA))
        self.assertEqual(self.server_buffer, self.DATA)

    async def test_send_at_certain_offset(self):
        # start sending a file at a certain offset
        total_sent = 0
        offset = len(self.DATA) // 2
        must_send = len(self.DATA) - offset
        nbytes = 4096
        while total_sent < must_send:
            sent = await self.sendfile_wrapper(self.sockno, self.fileno,
                                               offset, nbytes)
            if sent == 0:
                break
            offset += sent
            total_sent += sent
            self.assertTrue(sent <= nbytes)

        self.client.shutdown(socket.SHUT_RDWR)
        self.client.close()
        await self.server.wait_closed()
        expected = self.DATA[len(self.DATA) // 2:]
        self.assertEqual(total_sent, len(expected))
        self.assertEqual(len(self.server_buffer), len(expected))
        self.assertEqual(self.server_buffer, expected)

    async def test_offset_overflow(self):
        # specify an offset > file size
        offset = len(self.DATA) + 4096
        try:
            sent = await self.async_sendfile(self.sockno, self.fileno,
                                             offset, 4096)
        except OSError as e:
            # Solaris can raise EINVAL if offset >= file length, ignore.
            if e.errno != errno.EINVAL:
                raise
        else:
            self.assertEqual(sent, 0)
        self.client.shutdown(socket.SHUT_RDWR)
        self.client.close()
        await self.server.wait_closed()
        self.assertEqual(self.server_buffer, b'')

    async def test_invalid_offset(self):
        with self.assertRaises(OSError) as cm:
            await self.async_sendfile(self.sockno, self.fileno, -1, 4096)
        self.assertEqual(cm.exception.errno, errno.EINVAL)

    async def test_keywords(self):
        # Keyword arguments should be supported
        await self.async_sendfile(out_fd=self.sockno, in_fd=self.fileno,
                                  offset=0, count=4096)
        if self.SUPPORT_HEADERS_TRAILERS:
            await self.async_sendfile(out_fd=self.sockno, in_fd=self.fileno,
                                      offset=0, count=4096,
                                      headers=(), trailers=(), flags=0)

    # --- headers / trailers tests

    @requires_headers_trailers
    async def test_headers(self):
        total_sent = 0
        expected_data = b"x" * 512 + b"y" * 256 + self.DATA[:-1]
        sent = await self.async_sendfile(self.sockno, self.fileno, 0, 4096,
                                         headers=[b"x" * 512, b"y" * 256])
        self.assertLessEqual(sent, 512 + 256 + 4096)
        total_sent += sent
        offset = 4096
        while total_sent < len(expected_data):
            nbytes = min(len(expected_data) - total_sent, 4096)
            sent = await self.sendfile_wrapper(self.sockno, self.fileno,
                                               offset, nbytes)
            if sent == 0:
                break
            self.assertLessEqual(sent, nbytes)
            total_sent += sent
            offset += sent

        self.assertEqual(total_sent, len(expected_data))
        self.client.close()
        await self.server.wait_closed()
        self.assertEqual(hash(self.server_buffer), hash(expected_data))

    @requires_headers_trailers
    async def test_trailers(self):
        TESTFN2 = os_helper.TESTFN + "2"
        file_data = b"abcdef"

        self.addCleanup(os_helper.unlink, TESTFN2)
        create_file(TESTFN2, file_data)

        with open(TESTFN2, 'rb') as f:
            await self.async_sendfile(self.sockno, f.fileno(), 0, 5,
                                      trailers=[b"123456", b"789"])
            self.client.close()
            await self.server.wait_closed()
            self.assertEqual(self.server_buffer, b"abcde123456789")

    @requires_headers_trailers
    @requires_32b
    async def test_headers_overflow_32bits(self):
        self.server.handler_instance.accumulate = False
        with self.assertRaises(OSError) as cm:
            await self.async_sendfile(self.sockno, self.fileno, 0, 0,
                                      headers=[b"x" * 2**16] * 2**15)
        self.assertEqual(cm.exception.errno, errno.EINVAL)

    @requires_headers_trailers
    @requires_32b
    async def test_trailers_overflow_32bits(self):
        self.server.handler_instance.accumulate = False
        with self.assertRaises(OSError) as cm:
            await self.async_sendfile(self.sockno, self.fileno, 0, 0,
                                      trailers=[b"x" * 2**16] * 2**15)
        self.assertEqual(cm.exception.errno, errno.EINVAL)

    @requires_headers_trailers
    @unittest.skipUnless(hasattr(os, 'SF_NODISKIO'),
                         'test needs os.SF_NODISKIO')
    async def test_flags(self):
        try:
            await self.async_sendfile(self.sockno, self.fileno, 0, 4096,
                                      flags=os.SF_NODISKIO)
        except OSError as err:
            if err.errno not in (errno.EBUSY, errno.EAGAIN):
                raise


def supports_extended_attributes():
    if not hasattr(os, "setxattr"):
        return False

    try:
        with open(os_helper.TESTFN, "xb", 0) as fp:
            try:
                os.setxattr(fp.fileno(), b"user.test", b"")
            except OSError:
                return False
    finally:
        os_helper.unlink(os_helper.TESTFN)

    return True


@unittest.skipUnless(supports_extended_attributes(),
                     "no non-broken extended attribute support")
# Kernels < 2.6.39 don't respect setxattr flags.
@support.requires_linux_version(2, 6, 39)
class ExtendedAttributeTests(unittest.TestCase):

    def _check_xattrs_str(self, s, getxattr, setxattr, removexattr, listxattr, **kwargs):
        fn = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, fn)
        create_file(fn)

        with self.assertRaises(OSError) as cm:
            getxattr(fn, s("user.test"), **kwargs)
        self.assertEqual(cm.exception.errno, errno.ENODATA)

        init_xattr = listxattr(fn)
        self.assertIsInstance(init_xattr, list)

        setxattr(fn, s("user.test"), b"", **kwargs)
        xattr = set(init_xattr)
        xattr.add("user.test")
        self.assertEqual(set(listxattr(fn)), xattr)
        self.assertEqual(getxattr(fn, b"user.test", **kwargs), b"")
        setxattr(fn, s("user.test"), b"hello", os.XATTR_REPLACE, **kwargs)
        self.assertEqual(getxattr(fn, b"user.test", **kwargs), b"hello")

        with self.assertRaises(OSError) as cm:
            setxattr(fn, s("user.test"), b"bye", os.XATTR_CREATE, **kwargs)
        self.assertEqual(cm.exception.errno, errno.EEXIST)

        with self.assertRaises(OSError) as cm:
            setxattr(fn, s("user.test2"), b"bye", os.XATTR_REPLACE, **kwargs)
        self.assertEqual(cm.exception.errno, errno.ENODATA)

        setxattr(fn, s("user.test2"), b"foo", os.XATTR_CREATE, **kwargs)
        xattr.add("user.test2")
        self.assertEqual(set(listxattr(fn)), xattr)
        removexattr(fn, s("user.test"), **kwargs)

        with self.assertRaises(OSError) as cm:
            getxattr(fn, s("user.test"), **kwargs)
        self.assertEqual(cm.exception.errno, errno.ENODATA)

        xattr.remove("user.test")
        self.assertEqual(set(listxattr(fn)), xattr)
        self.assertEqual(getxattr(fn, s("user.test2"), **kwargs), b"foo")
        setxattr(fn, s("user.test"), b"a"*256, **kwargs)
        self.assertEqual(getxattr(fn, s("user.test"), **kwargs), b"a"*256)
        removexattr(fn, s("user.test"), **kwargs)
        many = sorted("user.test{}".format(i) for i in range(32))
        for thing in many:
            setxattr(fn, thing, b"x", **kwargs)
        self.assertEqual(set(listxattr(fn)), set(init_xattr) | set(many))

    def _check_xattrs(self, *args, **kwargs):
        self._check_xattrs_str(str, *args, **kwargs)
        os_helper.unlink(os_helper.TESTFN)

        self._check_xattrs_str(os.fsencode, *args, **kwargs)
        os_helper.unlink(os_helper.TESTFN)

    def test_simple(self):
        self._check_xattrs(os.getxattr, os.setxattr, os.removexattr,
                           os.listxattr)

    def test_lpath(self):
        self._check_xattrs(os.getxattr, os.setxattr, os.removexattr,
                           os.listxattr, follow_symlinks=False)

    def test_fds(self):
        def getxattr(path, *args):
            with open(path, "rb") as fp:
                return os.getxattr(fp.fileno(), *args)
        def setxattr(path, *args):
            with open(path, "wb", 0) as fp:
                os.setxattr(fp.fileno(), *args)
        def removexattr(path, *args):
            with open(path, "wb", 0) as fp:
                os.removexattr(fp.fileno(), *args)
        def listxattr(path, *args):
            with open(path, "rb") as fp:
                return os.listxattr(fp.fileno(), *args)
        self._check_xattrs(getxattr, setxattr, removexattr, listxattr)


@unittest.skipUnless(hasattr(os, 'get_terminal_size'), "requires os.get_terminal_size")
class TermsizeTests(unittest.TestCase):
    def test_does_not_crash(self):
        """Check if get_terminal_size() returns a meaningful value.

        There's no easy portable way to actually check the size of the
        terminal, so let's check if it returns something sensible instead.
        """
        try:
            size = os.get_terminal_size()
        except OSError as e:
            known_errnos = [errno.EINVAL, errno.ENOTTY]
            if sys.platform == "android":
                # The Android testbed redirects the native stdout to a pipe,
                # which returns a different error code.
                known_errnos.append(errno.EACCES)
            if sys.platform == "win32" or e.errno in known_errnos:
                # Under win32 a generic OSError can be thrown if the
                # handle cannot be retrieved
                self.skipTest("failed to query terminal size")
            raise

        self.assertGreaterEqual(size.columns, 0)
        self.assertGreaterEqual(size.lines, 0)

    @support.requires_subprocess()
    def test_stty_match(self):
        """Check if stty returns the same results

        stty actually tests stdin, so get_terminal_size is invoked on
        stdin explicitly. If stty succeeded, then get_terminal_size()
        should work too.
        """
        try:
            size = (
                subprocess.check_output(
                    ["stty", "size"], stderr=subprocess.DEVNULL, text=True
                ).split()
            )
        except (FileNotFoundError, subprocess.CalledProcessError,
                PermissionError):
            self.skipTest("stty invocation failed")
        expected = (int(size[1]), int(size[0])) # reversed order

        try:
            actual = os.get_terminal_size(sys.__stdin__.fileno())
        except OSError as e:
            if sys.platform == "win32" or e.errno in (errno.EINVAL, errno.ENOTTY):
                # Under win32 a generic OSError can be thrown if the
                # handle cannot be retrieved
                self.skipTest("failed to query terminal size")
            raise
        self.assertEqual(expected, actual)

    @unittest.skipUnless(sys.platform == 'win32', 'Windows specific test')
    def test_windows_fd(self):
        """Check if get_terminal_size() returns a meaningful value in Windows"""
        try:
            conout = open('conout$', 'w')
        except OSError:
            self.skipTest('failed to open conout$')
        with conout:
            size = os.get_terminal_size(conout.fileno())

        self.assertGreaterEqual(size.columns, 0)
        self.assertGreaterEqual(size.lines, 0)


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

@unittest.skipUnless(hasattr(os, 'timerfd_create'), 'requires os.timerfd_create')
@unittest.skipIf(sys.platform == "android", "gh-124873: Test is flaky on Android")
@support.requires_linux_version(2, 6, 30)
class TimerfdTests(unittest.TestCase):
    # gh-126112: Use 10 ms to tolerate slow buildbots
    CLOCK_RES_PLACES = 2  # 10 ms
    CLOCK_RES = 10 ** -CLOCK_RES_PLACES
    CLOCK_RES_NS = 10 ** (9 - CLOCK_RES_PLACES)

    def timerfd_create(self, *args, **kwargs):
        fd = os.timerfd_create(*args, **kwargs)
        self.assertGreaterEqual(fd, 0)
        self.assertFalse(os.get_inheritable(fd))
        self.addCleanup(os.close, fd)
        return fd

    def read_count_signaled(self, fd):
        # read 8 bytes
        data = os.read(fd, 8)
        return int.from_bytes(data, byteorder=sys.byteorder)

    def test_timerfd_initval(self):
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        initial_expiration = 0.25
        interval = 0.125

        # 1st call
        next_expiration, interval2 = os.timerfd_settime(fd, initial=initial_expiration, interval=interval)
        self.assertAlmostEqual(interval2, 0.0, places=self.CLOCK_RES_PLACES)
        self.assertAlmostEqual(next_expiration, 0.0, places=self.CLOCK_RES_PLACES)

        # 2nd call
        next_expiration, interval2 = os.timerfd_settime(fd, initial=initial_expiration, interval=interval)
        self.assertAlmostEqual(interval2, interval, places=self.CLOCK_RES_PLACES)
        self.assertAlmostEqual(next_expiration, initial_expiration, places=self.CLOCK_RES_PLACES)

        # timerfd_gettime
        next_expiration, interval2 = os.timerfd_gettime(fd)
        self.assertAlmostEqual(interval2, interval, places=self.CLOCK_RES_PLACES)
        self.assertAlmostEqual(next_expiration, initial_expiration, places=self.CLOCK_RES_PLACES)

    def test_timerfd_non_blocking(self):
        fd = self.timerfd_create(time.CLOCK_REALTIME, flags=os.TFD_NONBLOCK)

        # 0.1 second later
        initial_expiration = 0.1
        os.timerfd_settime(fd, initial=initial_expiration, interval=0)

        # read() raises OSError with errno is EAGAIN for non-blocking timer.
        with self.assertRaises(OSError) as ctx:
            self.read_count_signaled(fd)
        self.assertEqual(ctx.exception.errno, errno.EAGAIN)

        # Wait more than 0.1 seconds
        time.sleep(initial_expiration + 0.1)

        # confirm if timerfd is readable and read() returns 1 as bytes.
        self.assertEqual(self.read_count_signaled(fd), 1)

    @unittest.skipIf(sys.platform.startswith('netbsd'),
                     "gh-131263: Skip on NetBSD due to system freeze "
                     "with negative timer values")
    def test_timerfd_negative(self):
        one_sec_in_nsec = 10**9
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        test_flags = [0, os.TFD_TIMER_ABSTIME]
        if hasattr(os, 'TFD_TIMER_CANCEL_ON_SET'):
            test_flags.append(os.TFD_TIMER_ABSTIME | os.TFD_TIMER_CANCEL_ON_SET)

        # Any of 'initial' and 'interval' is negative value.
        for initial, interval in ( (-1, 0), (1, -1), (-1, -1),  (-0.1, 0), (1, -0.1), (-0.1, -0.1)):
            for flags in test_flags:
                with self.subTest(flags=flags, initial=initial, interval=interval):
                    with self.assertRaises(OSError) as context:
                        os.timerfd_settime(fd, flags=flags, initial=initial, interval=interval)
                    self.assertEqual(context.exception.errno, errno.EINVAL)

                    with self.assertRaises(OSError) as context:
                        initial_ns = int( one_sec_in_nsec * initial )
                        interval_ns = int( one_sec_in_nsec * interval )
                        os.timerfd_settime_ns(fd, flags=flags, initial=initial_ns, interval=interval_ns)
                    self.assertEqual(context.exception.errno, errno.EINVAL)

    def test_timerfd_interval(self):
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        # 1 second
        initial_expiration = 1
        # 0.5 second
        interval = 0.5

        os.timerfd_settime(fd, initial=initial_expiration, interval=interval)

        # timerfd_gettime
        next_expiration, interval2 = os.timerfd_gettime(fd)
        self.assertAlmostEqual(interval2, interval, places=self.CLOCK_RES_PLACES)
        self.assertAlmostEqual(next_expiration, initial_expiration, places=self.CLOCK_RES_PLACES)

        count = 3
        t = time.perf_counter()
        for _ in range(count):
            self.assertEqual(self.read_count_signaled(fd), 1)
        t = time.perf_counter() - t

        total_time = initial_expiration + interval * (count - 1)
        self.assertGreater(t, total_time - self.CLOCK_RES)

        # wait 3.5 time of interval
        time.sleep( (count+0.5) * interval)
        self.assertEqual(self.read_count_signaled(fd), count)

    def test_timerfd_TFD_TIMER_ABSTIME(self):
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        now = time.clock_gettime(time.CLOCK_REALTIME)

        # 1 second later from now.
        offset = 1
        initial_expiration = now + offset
        # not interval timer
        interval = 0

        os.timerfd_settime(fd, flags=os.TFD_TIMER_ABSTIME, initial=initial_expiration, interval=interval)

        # timerfd_gettime
        # Note: timerfd_gettime returns relative values even if TFD_TIMER_ABSTIME is specified.
        next_expiration, interval2 = os.timerfd_gettime(fd)
        self.assertAlmostEqual(interval2, interval, places=self.CLOCK_RES_PLACES)
        self.assertAlmostEqual(next_expiration, offset, places=self.CLOCK_RES_PLACES)

        t = time.perf_counter()
        count_signaled = self.read_count_signaled(fd)
        t = time.perf_counter() - t
        self.assertEqual(count_signaled, 1)

        self.assertGreater(t, offset - self.CLOCK_RES)

    def test_timerfd_select(self):
        fd = self.timerfd_create(time.CLOCK_REALTIME, flags=os.TFD_NONBLOCK)

        rfd, wfd, xfd = select.select([fd], [fd], [fd], 0)
        self.assertEqual((rfd, wfd, xfd), ([], [], []))

        # 0.25 second
        initial_expiration = 0.25
        # every 0.125 second
        interval = 0.125

        os.timerfd_settime(fd, initial=initial_expiration, interval=interval)

        count = 3
        t = time.perf_counter()
        for _ in range(count):
            rfd, wfd, xfd = select.select([fd], [fd], [fd], initial_expiration + interval)
            self.assertEqual((rfd, wfd, xfd), ([fd], [], []))
            self.assertEqual(self.read_count_signaled(fd), 1)
        t = time.perf_counter() - t

        total_time = initial_expiration + interval * (count - 1)
        self.assertGreater(t, total_time - self.CLOCK_RES)

    def check_timerfd_poll(self, nanoseconds):
        fd = self.timerfd_create(time.CLOCK_REALTIME, flags=os.TFD_NONBLOCK)

        selector = selectors.DefaultSelector()
        selector.register(fd, selectors.EVENT_READ)
        self.addCleanup(selector.close)

        sec_to_nsec = 10 ** 9
        # 0.25 second
        initial_expiration_ns = sec_to_nsec // 4
        # every 0.125 second
        interval_ns = sec_to_nsec // 8

        if nanoseconds:
            os.timerfd_settime_ns(fd,
                                  initial=initial_expiration_ns,
                                  interval=interval_ns)
        else:
            os.timerfd_settime(fd,
                               initial=initial_expiration_ns / sec_to_nsec,
                               interval=interval_ns / sec_to_nsec)

        count = 3
        if nanoseconds:
            t = time.perf_counter_ns()
        else:
            t = time.perf_counter()
        for i in range(count):
            timeout_margin_ns = interval_ns
            if i == 0:
                timeout_ns = initial_expiration_ns + interval_ns + timeout_margin_ns
            else:
                timeout_ns = interval_ns + timeout_margin_ns

            ready = selector.select(timeout_ns / sec_to_nsec)
            self.assertEqual(len(ready), 1, ready)
            event = ready[0][1]
            self.assertEqual(event, selectors.EVENT_READ)

            self.assertEqual(self.read_count_signaled(fd), 1)

        total_time = initial_expiration_ns + interval_ns * (count - 1)
        if nanoseconds:
            dt = time.perf_counter_ns() - t
            self.assertGreater(dt, total_time - self.CLOCK_RES_NS)
        else:
            dt = time.perf_counter() - t
            self.assertGreater(dt, total_time / sec_to_nsec - self.CLOCK_RES)
        selector.unregister(fd)

    def test_timerfd_poll(self):
        self.check_timerfd_poll(False)

    def test_timerfd_ns_poll(self):
        self.check_timerfd_poll(True)

    def test_timerfd_ns_initval(self):
        one_sec_in_nsec = 10**9
        limit_error = one_sec_in_nsec // 10**3
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        # 1st call
        initial_expiration_ns = 0
        interval_ns = one_sec_in_nsec // 1000
        next_expiration_ns, interval_ns2  = os.timerfd_settime_ns(fd, initial=initial_expiration_ns, interval=interval_ns)
        self.assertEqual(interval_ns2, 0)
        self.assertEqual(next_expiration_ns, 0)

        # 2nd call
        next_expiration_ns, interval_ns2 = os.timerfd_settime_ns(fd, initial=initial_expiration_ns, interval=interval_ns)
        self.assertEqual(interval_ns2, interval_ns)
        self.assertEqual(next_expiration_ns, initial_expiration_ns)

        # timerfd_gettime
        next_expiration_ns, interval_ns2 = os.timerfd_gettime_ns(fd)
        self.assertEqual(interval_ns2, interval_ns)
        self.assertLessEqual(next_expiration_ns, initial_expiration_ns)

        self.assertAlmostEqual(next_expiration_ns, initial_expiration_ns, delta=limit_error)

    def test_timerfd_ns_interval(self):
        one_sec_in_nsec = 10**9
        limit_error = one_sec_in_nsec // 10**3
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        # 1 second
        initial_expiration_ns = one_sec_in_nsec
        # every 0.5 second
        interval_ns = one_sec_in_nsec // 2

        os.timerfd_settime_ns(fd, initial=initial_expiration_ns, interval=interval_ns)

        # timerfd_gettime
        next_expiration_ns, interval_ns2 = os.timerfd_gettime_ns(fd)
        self.assertEqual(interval_ns2, interval_ns)
        self.assertLessEqual(next_expiration_ns, initial_expiration_ns)

        count = 3
        t = time.perf_counter_ns()
        for _ in range(count):
            self.assertEqual(self.read_count_signaled(fd), 1)
        t = time.perf_counter_ns() - t

        total_time_ns = initial_expiration_ns + interval_ns * (count - 1)
        self.assertGreater(t, total_time_ns - self.CLOCK_RES_NS)

        # wait 3.5 time of interval
        time.sleep( (count+0.5) * interval_ns / one_sec_in_nsec)
        self.assertEqual(self.read_count_signaled(fd), count)


    def test_timerfd_ns_TFD_TIMER_ABSTIME(self):
        one_sec_in_nsec = 10**9
        limit_error = one_sec_in_nsec // 10**3
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        now_ns = time.clock_gettime_ns(time.CLOCK_REALTIME)

        # 1 second later from now.
        offset_ns = one_sec_in_nsec
        initial_expiration_ns = now_ns + offset_ns
        # not interval timer
        interval_ns = 0

        os.timerfd_settime_ns(fd, flags=os.TFD_TIMER_ABSTIME, initial=initial_expiration_ns, interval=interval_ns)

        # timerfd_gettime
        # Note: timerfd_gettime returns relative values even if TFD_TIMER_ABSTIME is specified.
        next_expiration_ns, interval_ns2 = os.timerfd_gettime_ns(fd)
        self.assertLess(abs(interval_ns2 - interval_ns),  limit_error)
        self.assertLess(abs(next_expiration_ns - offset_ns),  limit_error)

        t = time.perf_counter_ns()
        count_signaled = self.read_count_signaled(fd)
        t = time.perf_counter_ns() - t
        self.assertEqual(count_signaled, 1)

        self.assertGreater(t, offset_ns - self.CLOCK_RES_NS)

    def test_timerfd_ns_select(self):
        one_sec_in_nsec = 10**9

        fd = self.timerfd_create(time.CLOCK_REALTIME, flags=os.TFD_NONBLOCK)

        rfd, wfd, xfd = select.select([fd], [fd], [fd], 0)
        self.assertEqual((rfd, wfd, xfd), ([], [], []))

        # 0.25 second
        initial_expiration_ns = one_sec_in_nsec // 4
        # every 0.125 second
        interval_ns = one_sec_in_nsec // 8

        os.timerfd_settime_ns(fd, initial=initial_expiration_ns, interval=interval_ns)

        count = 3
        t = time.perf_counter_ns()
        for _ in range(count):
            rfd, wfd, xfd = select.select([fd], [fd], [fd], (initial_expiration_ns + interval_ns) / 1e9 )
            self.assertEqual((rfd, wfd, xfd), ([fd], [], []))
            self.assertEqual(self.read_count_signaled(fd), 1)
        t = time.perf_counter_ns() - t

        total_time_ns = initial_expiration_ns + interval_ns * (count - 1)
        self.assertGreater(t, total_time_ns - self.CLOCK_RES_NS)

class OSErrorTests(unittest.TestCase):
    def setUp(self):
        class Str(str):
            pass

        self.bytes_filenames = []
        self.unicode_filenames = []
        if os_helper.TESTFN_UNENCODABLE is not None:
            decoded = os_helper.TESTFN_UNENCODABLE
        else:
            decoded = os_helper.TESTFN
        self.unicode_filenames.append(decoded)
        self.unicode_filenames.append(Str(decoded))
        if os_helper.TESTFN_UNDECODABLE is not None:
            encoded = os_helper.TESTFN_UNDECODABLE
        else:
            encoded = os.fsencode(os_helper.TESTFN)
        self.bytes_filenames.append(encoded)

        self.filenames = self.bytes_filenames + self.unicode_filenames

    def test_oserror_filename(self):
        funcs = [
            (self.filenames, os.chdir,),
            (self.filenames, os.lstat,),
            (self.filenames, os.open, os.O_RDONLY),
            (self.filenames, os.rmdir,),
            (self.filenames, os.stat,),
            (self.filenames, os.unlink,),
            (self.filenames, os.listdir,),
            (self.filenames, os.rename, "dst"),
            (self.filenames, os.replace, "dst"),
        ]
        if os_helper.can_chmod():
            funcs.append((self.filenames, os.chmod, 0o777))
        if hasattr(os, "chown"):
            funcs.append((self.filenames, os.chown, 0, 0))
        if hasattr(os, "lchown"):
            funcs.append((self.filenames, os.lchown, 0, 0))
        if hasattr(os, "truncate"):
            funcs.append((self.filenames, os.truncate, 0))
        if hasattr(os, "chflags"):
            funcs.append((self.filenames, os.chflags, 0))
        if hasattr(os, "lchflags"):
            funcs.append((self.filenames, os.lchflags, 0))
        if hasattr(os, "chroot"):
            funcs.append((self.filenames, os.chroot,))
        if hasattr(os, "link"):
            funcs.append((self.filenames, os.link, "dst"))
        if hasattr(os, "listxattr"):
            funcs.extend((
                (self.filenames, os.listxattr,),
                (self.filenames, os.getxattr, "user.test"),
                (self.filenames, os.setxattr, "user.test", b'user'),
                (self.filenames, os.removexattr, "user.test"),
            ))
        if hasattr(os, "lchmod"):
            funcs.append((self.filenames, os.lchmod, 0o777))
        if hasattr(os, "readlink"):
            funcs.append((self.filenames, os.readlink,))

        for filenames, func, *func_args in funcs:
            for name in filenames:
                try:
                    func(name, *func_args)
                except OSError as err:
                    self.assertIs(err.filename, name, str(func))
                except UnicodeDecodeError:
                    pass
                else:
                    self.fail(f"No exception thrown by {func}")

class CPUCountTests(unittest.TestCase):
    def check_cpu_count(self, cpus):
        if cpus is None:
            self.skipTest("Could not determine the number of CPUs")

        self.assertIsInstance(cpus, int)
        self.assertGreater(cpus, 0)

    def test_cpu_count(self):
        cpus = os.cpu_count()
        self.check_cpu_count(cpus)

    def test_process_cpu_count(self):
        cpus = os.process_cpu_count()
        self.assertLessEqual(cpus, os.cpu_count())
        self.check_cpu_count(cpus)

    @unittest.skipUnless(hasattr(os, 'sched_setaffinity'),
                         "don't have sched affinity support")
    def test_process_cpu_count_affinity(self):
        affinity1 = os.process_cpu_count()
        if affinity1 is None:
            self.skipTest("Could not determine the number of CPUs")

        # Disable one CPU
        mask = os.sched_getaffinity(0)
        if len(mask) <= 1:
            self.skipTest(f"sched_getaffinity() returns less than "
                          f"2 CPUs: {sorted(mask)}")
        self.addCleanup(os.sched_setaffinity, 0, list(mask))
        mask.pop()
        os.sched_setaffinity(0, mask)

        # test process_cpu_count()
        affinity2 = os.process_cpu_count()
        self.assertEqual(affinity2, affinity1 - 1)


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

@unittest.skipUnless(hasattr(os, 'openpty'), "need os.openpty()")
class PseudoterminalTests(unittest.TestCase):
    def open_pty(self):
        """Open a pty fd-pair, and schedule cleanup for it"""
        main_fd, second_fd = os.openpty()
        self.addCleanup(os.close, main_fd)
        self.addCleanup(os.close, second_fd)
        return main_fd, second_fd

    def test_openpty(self):
        main_fd, second_fd = self.open_pty()
        self.assertEqual(os.get_inheritable(main_fd), False)
        self.assertEqual(os.get_inheritable(second_fd), False)

    @unittest.skipUnless(hasattr(os, 'ptsname'), "need os.ptsname()")
    @unittest.skipUnless(hasattr(os, 'O_RDWR'), "need os.O_RDWR")
    @unittest.skipUnless(hasattr(os, 'O_NOCTTY'), "need os.O_NOCTTY")
    def test_open_via_ptsname(self):
        main_fd, second_fd = self.open_pty()
        second_path = os.ptsname(main_fd)
        reopened_second_fd = os.open(second_path, os.O_RDWR|os.O_NOCTTY)
        self.addCleanup(os.close, reopened_second_fd)
        os.write(reopened_second_fd, b'foo')
        self.assertEqual(os.read(main_fd, 3), b'foo')

    @unittest.skipUnless(hasattr(os, 'posix_openpt'), "need os.posix_openpt()")
    @unittest.skipUnless(hasattr(os, 'grantpt'), "need os.grantpt()")
    @unittest.skipUnless(hasattr(os, 'unlockpt'), "need os.unlockpt()")
    @unittest.skipUnless(hasattr(os, 'ptsname'), "need os.ptsname()")
    @unittest.skipUnless(hasattr(os, 'O_RDWR'), "need os.O_RDWR")
    @unittest.skipUnless(hasattr(os, 'O_NOCTTY'), "need os.O_NOCTTY")
    def test_posix_pty_functions(self):
        mother_fd = os.posix_openpt(os.O_RDWR|os.O_NOCTTY)
        self.addCleanup(os.close, mother_fd)
        os.grantpt(mother_fd)
        os.unlockpt(mother_fd)
        son_path = os.ptsname(mother_fd)
        son_fd = os.open(son_path, os.O_RDWR|os.O_NOCTTY)
        self.addCleanup(os.close, son_fd)
        self.assertEqual(os.ptsname(mother_fd), os.ttyname(son_fd))

    @unittest.skipUnless(hasattr(os, 'spawnl'), "need os.spawnl()")
    @support.requires_subprocess()
    def test_pipe_spawnl(self):
        # gh-77046: On Windows, os.pipe() file descriptors must be created with
        # _O_NOINHERIT to make them non-inheritable. UCRT has no public API to
        # get (_osfile(fd) & _O_NOINHERIT), so use a functional test.
        #
        # Make sure that fd is not inherited by a child process created by
        # os.spawnl(): get_osfhandle() and dup() must fail with EBADF.

        fd, fd2 = os.pipe()
        self.addCleanup(os.close, fd)
        self.addCleanup(os.close, fd2)

        code = textwrap.dedent(f"""
            import errno
            import os
            import test.support
            try:
                import msvcrt
            except ImportError:
                msvcrt = None

            fd = {fd}

            with test.support.SuppressCrashReport():
                if msvcrt is not None:
                    try:
                        handle = msvcrt.get_osfhandle(fd)
                    except OSError as exc:
                        if exc.errno != errno.EBADF:
                            raise
                        # get_osfhandle(fd) failed with EBADF as expected
                    else:
                        raise Exception("get_osfhandle() must fail")

                try:
                    fd3 = os.dup(fd)
                except OSError as exc:
                    if exc.errno != errno.EBADF:
                        raise
                    # os.dup(fd) failed with EBADF as expected
                else:
                    os.close(fd3)
                    raise Exception("dup must fail")
        """)

        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with open(filename, "w") as fp:
            print(code, file=fp, end="")

        executable = sys.executable
        cmd = [executable, filename]
        if os.name == "nt" and " " in cmd[0]:
            cmd[0] = f'"{cmd[0]}"'
        exitcode = os.spawnl(os.P_WAIT, executable, *cmd)
        self.assertEqual(exitcode, 0)


class PathTConverterTests(unittest.TestCase):
    # tuples of (function name, allows fd arguments, additional arguments to
    # function, cleanup function)
    functions = [
        ('stat', True, (), None),
        ('lstat', False, (), None),
        ('access', False, (os.F_OK,), None),
        ('chflags', False, (0,), None),
        ('lchflags', False, (0,), None),
        ('open', False, (os.O_RDONLY,), getattr(os, 'close', None)),
    ]

    def test_path_t_converter(self):
        str_filename = os_helper.TESTFN
        if os.name == 'nt':
            bytes_fspath = bytes_filename = None
        else:
            bytes_filename = os.fsencode(os_helper.TESTFN)
            bytes_fspath = FakePath(bytes_filename)
        fd = os.open(FakePath(str_filename), os.O_WRONLY|os.O_CREAT)
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        self.addCleanup(os.close, fd)

        int_fspath = FakePath(fd)
        str_fspath = FakePath(str_filename)

        for name, allow_fd, extra_args, cleanup_fn in self.functions:
            with self.subTest(name=name):
                try:
                    fn = getattr(os, name)
                except AttributeError:
                    continue

                for path in (str_filename, bytes_filename, str_fspath,
                             bytes_fspath):
                    if path is None:
                        continue
                    with self.subTest(name=name, path=path):
                        result = fn(path, *extra_args)
                        if cleanup_fn is not None:
                            cleanup_fn(result)

                with self.assertRaisesRegex(
                        TypeError, 'to return str or bytes'):
                    fn(int_fspath, *extra_args)

                if allow_fd:
                    result = fn(fd, *extra_args)  # should not fail
                    if cleanup_fn is not None:
                        cleanup_fn(result)
                else:
                    with self.assertRaisesRegex(
                            TypeError,
                            'os.PathLike'):
                        fn(fd, *extra_args)

    def test_path_t_converter_and_custom_class(self):
        msg = r'__fspath__\(\) to return str or bytes, not %s'
        with self.assertRaisesRegex(TypeError, msg % r'int'):
            os.stat(FakePath(2))
        with self.assertRaisesRegex(TypeError, msg % r'float'):
            os.stat(FakePath(2.34))
        with self.assertRaisesRegex(TypeError, msg % r'object'):
            os.stat(FakePath(object()))


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



class ExportsTests(unittest.TestCase):
    def test_os_all(self):
        self.assertIn('open', os.__all__)
        self.assertIn('walk', os.__all__)


class TestDirEntry(unittest.TestCase):
    def setUp(self):
        self.path = os.path.realpath(os_helper.TESTFN)
        self.addCleanup(os_helper.rmtree, self.path)
        os.mkdir(self.path)

    def test_uninstantiable(self):
        self.assertRaises(TypeError, os.DirEntry)

    def test_unpickable(self):
        filename = create_file(os.path.join(self.path, "file.txt"), b'python')
        entry = [entry for entry in os.scandir(self.path)].pop()
        self.assertIsInstance(entry, os.DirEntry)
        self.assertEqual(entry.name, "file.txt")
        import pickle
        self.assertRaises(TypeError, pickle.dumps, entry, filename)


class TestScandir(unittest.TestCase):
    check_no_resource_warning = warnings_helper.check_no_resource_warning

    def setUp(self):
        self.path = os.path.realpath(os_helper.TESTFN)
        self.bytes_path = os.fsencode(self.path)
        self.addCleanup(os_helper.rmtree, self.path)
        os.mkdir(self.path)

    def create_file(self, name="file.txt"):
        path = self.bytes_path if isinstance(name, bytes) else self.path
        filename = os.path.join(path, name)
        create_file(filename, b'python')
        return filename

    def get_entries(self, names):
        entries = dict((entry.name, entry)
                       for entry in os.scandir(self.path))
        self.assertEqual(sorted(entries.keys()), names)
        return entries

    def assert_stat_equal(self, stat1, stat2, skip_fields):
        if skip_fields:
            for attr in dir(stat1):
                if not attr.startswith("st_"):
                    continue
                if attr in ("st_dev", "st_ino", "st_nlink", "st_ctime",
                            "st_ctime_ns"):
                    continue
                self.assertEqual(getattr(stat1, attr),
                                 getattr(stat2, attr),
                                 (stat1, stat2, attr))
        else:
            self.assertEqual(stat1, stat2)

    def test_uninstantiable(self):
        scandir_iter = os.scandir(self.path)
        self.assertRaises(TypeError, type(scandir_iter))
        scandir_iter.close()

    def test_unpickable(self):
        filename = self.create_file("file.txt")
        scandir_iter = os.scandir(self.path)
        import pickle
        self.assertRaises(TypeError, pickle.dumps, scandir_iter, filename)
        scandir_iter.close()

    def check_entry(self, entry, name, is_dir, is_file, is_symlink):
        self.assertIsInstance(entry, os.DirEntry)
        self.assertEqual(entry.name, name)
        self.assertEqual(entry.path, os.path.join(self.path, name))
        self.assertEqual(entry.inode(),
                         os.stat(entry.path, follow_symlinks=False).st_ino)

        entry_stat = os.stat(entry.path)
        self.assertEqual(entry.is_dir(),
                         stat.S_ISDIR(entry_stat.st_mode))
        self.assertEqual(entry.is_file(),
                         stat.S_ISREG(entry_stat.st_mode))
        self.assertEqual(entry.is_symlink(),
                         os.path.islink(entry.path))

        entry_lstat = os.stat(entry.path, follow_symlinks=False)
        self.assertEqual(entry.is_dir(follow_symlinks=False),
                         stat.S_ISDIR(entry_lstat.st_mode))
        self.assertEqual(entry.is_file(follow_symlinks=False),
                         stat.S_ISREG(entry_lstat.st_mode))

        self.assertEqual(entry.is_junction(), os.path.isjunction(entry.path))

        self.assert_stat_equal(entry.stat(),
                               entry_stat,
                               os.name == 'nt' and not is_symlink)
        self.assert_stat_equal(entry.stat(follow_symlinks=False),
                               entry_lstat,
                               os.name == 'nt')

    def test_attributes(self):
        link = os_helper.can_hardlink()
        symlink = os_helper.can_symlink()

        dirname = os.path.join(self.path, "dir")
        os.mkdir(dirname)
        filename = self.create_file("file.txt")
        if link:
            try:
                os.link(filename, os.path.join(self.path, "link_file.txt"))
            except PermissionError as e:
                self.skipTest('os.link(): %s' % e)
        if symlink:
            os.symlink(dirname, os.path.join(self.path, "symlink_dir"),
                       target_is_directory=True)
            os.symlink(filename, os.path.join(self.path, "symlink_file.txt"))

        names = ['dir', 'file.txt']
        if link:
            names.append('link_file.txt')
        if symlink:
            names.extend(('symlink_dir', 'symlink_file.txt'))
        entries = self.get_entries(names)

        entry = entries['dir']
        self.check_entry(entry, 'dir', True, False, False)

        entry = entries['file.txt']
        self.check_entry(entry, 'file.txt', False, True, False)

        if link:
            entry = entries['link_file.txt']
            self.check_entry(entry, 'link_file.txt', False, True, False)

        if symlink:
            entry = entries['symlink_dir']
            self.check_entry(entry, 'symlink_dir', True, False, True)

            entry = entries['symlink_file.txt']
            self.check_entry(entry, 'symlink_file.txt', False, True, True)

    @unittest.skipIf(sys.platform != 'win32', "Can only test junctions with creation on win32.")
    def test_attributes_junctions(self):
        dirname = os.path.join(self.path, "tgtdir")
        os.mkdir(dirname)

        import _winapi
        try:
            _winapi.CreateJunction(dirname, os.path.join(self.path, "srcjunc"))
        except OSError:
            raise unittest.SkipTest('creating the test junction failed')

        entries = self.get_entries(['srcjunc', 'tgtdir'])
        self.assertEqual(entries['srcjunc'].is_junction(), True)
        self.assertEqual(entries['tgtdir'].is_junction(), False)

    def get_entry(self, name):
        path = self.bytes_path if isinstance(name, bytes) else self.path
        entries = list(os.scandir(path))
        self.assertEqual(len(entries), 1)

        entry = entries[0]
        self.assertEqual(entry.name, name)
        return entry

    def create_file_entry(self, name='file.txt'):
        filename = self.create_file(name=name)
        return self.get_entry(os.path.basename(filename))

    def test_current_directory(self):
        filename = self.create_file()
        old_dir = os.getcwd()
        try:
            os.chdir(self.path)

            # call scandir() without parameter: it must list the content
            # of the current directory
            entries = dict((entry.name, entry) for entry in os.scandir())
            self.assertEqual(sorted(entries.keys()),
                             [os.path.basename(filename)])
        finally:
            os.chdir(old_dir)

    def test_repr(self):
        entry = self.create_file_entry()
        self.assertEqual(repr(entry), "<DirEntry 'file.txt'>")

    def test_fspath_protocol(self):
        entry = self.create_file_entry()
        self.assertEqual(os.fspath(entry), os.path.join(self.path, 'file.txt'))

    def test_fspath_protocol_bytes(self):
        bytes_filename = os.fsencode('bytesfile.txt')
        bytes_entry = self.create_file_entry(name=bytes_filename)
        fspath = os.fspath(bytes_entry)
        self.assertIsInstance(fspath, bytes)
        self.assertEqual(fspath,
                         os.path.join(os.fsencode(self.path),bytes_filename))

    def test_removed_dir(self):
        path = os.path.join(self.path, 'dir')

        os.mkdir(path)
        entry = self.get_entry('dir')
        os.rmdir(path)

        # On POSIX, is_dir() result depends if scandir() filled d_type or not
        if os.name == 'nt':
            self.assertTrue(entry.is_dir())
        self.assertFalse(entry.is_file())
        self.assertFalse(entry.is_symlink())
        if os.name == 'nt':
            self.assertRaises(FileNotFoundError, entry.inode)
            # don't fail
            entry.stat()
            entry.stat(follow_symlinks=False)
        else:
            self.assertGreater(entry.inode(), 0)
            self.assertRaises(FileNotFoundError, entry.stat)
            self.assertRaises(FileNotFoundError, entry.stat, follow_symlinks=False)

    def test_removed_file(self):
        entry = self.create_file_entry()
        os.unlink(entry.path)

        self.assertFalse(entry.is_dir())
        # On POSIX, is_dir() result depends if scandir() filled d_type or not
        if os.name == 'nt':
            self.assertTrue(entry.is_file())
        self.assertFalse(entry.is_symlink())
        if os.name == 'nt':
            self.assertRaises(FileNotFoundError, entry.inode)
            # don't fail
            entry.stat()
            entry.stat(follow_symlinks=False)
        else:
            self.assertGreater(entry.inode(), 0)
            self.assertRaises(FileNotFoundError, entry.stat)
            self.assertRaises(FileNotFoundError, entry.stat, follow_symlinks=False)

    def test_broken_symlink(self):
        if not os_helper.can_symlink():
            return self.skipTest('cannot create symbolic link')

        filename = self.create_file("file.txt")
        os.symlink(filename,
                   os.path.join(self.path, "symlink.txt"))
        entries = self.get_entries(['file.txt', 'symlink.txt'])
        entry = entries['symlink.txt']
        os.unlink(filename)

        self.assertGreater(entry.inode(), 0)
        self.assertFalse(entry.is_dir())
        self.assertFalse(entry.is_file())  # broken symlink returns False
        self.assertFalse(entry.is_dir(follow_symlinks=False))
        self.assertFalse(entry.is_file(follow_symlinks=False))
        self.assertTrue(entry.is_symlink())
        self.assertRaises(FileNotFoundError, entry.stat)
        # don't fail
        entry.stat(follow_symlinks=False)

    def test_bytes(self):
        self.create_file("file.txt")

        path_bytes = os.fsencode(self.path)
        entries = list(os.scandir(path_bytes))
        self.assertEqual(len(entries), 1, entries)
        entry = entries[0]

        self.assertEqual(entry.name, b'file.txt')
        self.assertEqual(entry.path,
                         os.fsencode(os.path.join(self.path, 'file.txt')))

    def test_bytes_like(self):
        self.create_file("file.txt")

        for cls in bytearray, memoryview:
            path_bytes = cls(os.fsencode(self.path))
            with self.assertRaises(TypeError):
                os.scandir(path_bytes)

    @unittest.skipUnless(os.listdir in os.supports_fd,
                         'fd support for listdir required for this test.')
    def test_fd(self):
        self.assertIn(os.scandir, os.supports_fd)
        self.create_file('file.txt')
        expected_names = ['file.txt']
        if os_helper.can_symlink():
            os.symlink('file.txt', os.path.join(self.path, 'link'))
            expected_names.append('link')

        with os_helper.open_dir_fd(self.path) as fd:
            with os.scandir(fd) as it:
                entries = list(it)
            names = [entry.name for entry in entries]
            self.assertEqual(sorted(names), expected_names)
            self.assertEqual(names, os.listdir(fd))
            for entry in entries:
                self.assertEqual(entry.path, entry.name)
                self.assertEqual(os.fspath(entry), entry.name)
                self.assertEqual(entry.is_symlink(), entry.name == 'link')
                if os.stat in os.supports_dir_fd:
                    st = os.stat(entry.name, dir_fd=fd)
                    self.assertEqual(entry.stat(), st)
                    st = os.stat(entry.name, dir_fd=fd, follow_symlinks=False)
                    self.assertEqual(entry.stat(follow_symlinks=False), st)

    @unittest.skipIf(support.is_wasi, "WASI maps '' to cwd")
    def test_empty_path(self):
        self.assertRaises(FileNotFoundError, os.scandir, '')

    def test_consume_iterator_twice(self):
        self.create_file("file.txt")
        iterator = os.scandir(self.path)

        entries = list(iterator)
        self.assertEqual(len(entries), 1, entries)

        # check than consuming the iterator twice doesn't raise exception
        entries2 = list(iterator)
        self.assertEqual(len(entries2), 0, entries2)

    def test_bad_path_type(self):
        for obj in [1.234, {}, []]:
            self.assertRaises(TypeError, os.scandir, obj)

    def test_close(self):
        self.create_file("file.txt")
        self.create_file("file2.txt")
        iterator = os.scandir(self.path)
        next(iterator)
        iterator.close()
        # multiple closes
        iterator.close()
        with self.check_no_resource_warning():
            del iterator

    def test_context_manager(self):
        self.create_file("file.txt")
        self.create_file("file2.txt")
        with os.scandir(self.path) as iterator:
            next(iterator)
        with self.check_no_resource_warning():
            del iterator

    def test_context_manager_close(self):
        self.create_file("file.txt")
        self.create_file("file2.txt")
        with os.scandir(self.path) as iterator:
            next(iterator)
            iterator.close()

    def test_context_manager_exception(self):
        self.create_file("file.txt")
        self.create_file("file2.txt")
        with self.assertRaises(ZeroDivisionError):
            with os.scandir(self.path) as iterator:
                next(iterator)
                1/0
        with self.check_no_resource_warning():
            del iterator

    def test_resource_warning(self):
        self.create_file("file.txt")
        self.create_file("file2.txt")
        iterator = os.scandir(self.path)
        next(iterator)
        with self.assertWarns(ResourceWarning):
            del iterator
            support.gc_collect()
        # exhausted iterator
        iterator = os.scandir(self.path)
        list(iterator)
        with self.check_no_resource_warning():
            del iterator


class TestPEP519(unittest.TestCase):

    # Abstracted so it can be overridden to test pure Python implementation
    # if a C version is provided.
    fspath = staticmethod(os.fspath)

    def test_return_bytes(self):
        for b in b'hello', b'goodbye', b'some/path/and/file':
            self.assertEqual(b, self.fspath(b))

    def test_return_string(self):
        for s in 'hello', 'goodbye', 'some/path/and/file':
            self.assertEqual(s, self.fspath(s))

    def test_fsencode_fsdecode(self):
        for p in "path/like/object", b"path/like/object":
            pathlike = FakePath(p)

            self.assertEqual(p, self.fspath(pathlike))
            self.assertEqual(b"path/like/object", os.fsencode(pathlike))
            self.assertEqual("path/like/object", os.fsdecode(pathlike))

    def test_pathlike(self):
        self.assertEqual('#feelthegil', self.fspath(FakePath('#feelthegil')))
        self.assertIsSubclass(FakePath, os.PathLike)
        self.assertIsInstance(FakePath('x'), os.PathLike)

    def test_garbage_in_exception_out(self):
        vapor = type('blah', (), {})
        for o in int, type, os, vapor():
            self.assertRaises(TypeError, self.fspath, o)

    def test_argument_required(self):
        self.assertRaises(TypeError, self.fspath)

    def test_bad_pathlike(self):
        # __fspath__ returns a value other than str or bytes.
        self.assertRaises(TypeError, self.fspath, FakePath(42))
        # __fspath__ attribute that is not callable.
        c = type('foo', (), {})
        c.__fspath__ = 1
        self.assertRaises(TypeError, self.fspath, c())
        # __fspath__ raises an exception.
        self.assertRaises(ZeroDivisionError, self.fspath,
                          FakePath(ZeroDivisionError()))

    def test_pathlike_subclasshook(self):
        # bpo-38878: subclasshook causes subclass checks
        # true on abstract implementation.
        class A(os.PathLike):
            pass
        self.assertNotIsSubclass(FakePath, A)
        self.assertIsSubclass(FakePath, os.PathLike)

    def test_pathlike_class_getitem(self):
        self.assertIsInstance(os.PathLike[bytes], types.GenericAlias)

    def test_pathlike_subclass_slots(self):
        class A(os.PathLike):
            __slots__ = ()
            def __fspath__(self):
                return ''
        self.assertNotHasAttr(A(), '__dict__')

    def test_fspath_set_to_None(self):
        class Foo:
            __fspath__ = None

        class Bar:
            def __fspath__(self):
                return 'bar'

        class Baz(Bar):
            __fspath__ = None

        good_error_msg = (
            r"expected str, bytes or os.PathLike object, not {}".format
        )

        with self.assertRaisesRegex(TypeError, good_error_msg("Foo")):
            self.fspath(Foo())

        self.assertEqual(self.fspath(Bar()), 'bar')

        with self.assertRaisesRegex(TypeError, good_error_msg("Baz")):
            self.fspath(Baz())

        with self.assertRaisesRegex(TypeError, good_error_msg("Foo")):
            open(Foo())

        with self.assertRaisesRegex(TypeError, good_error_msg("Baz")):
            open(Baz())

        other_good_error_msg = (
            r"should be string, bytes or os.PathLike, not {}".format
        )

        with self.assertRaisesRegex(TypeError, other_good_error_msg("Foo")):
            os.rename(Foo(), "foooo")

        with self.assertRaisesRegex(TypeError, other_good_error_msg("Baz")):
            os.rename(Baz(), "bazzz")

class TimesTests(unittest.TestCase):
    def test_times(self):
        times = os.times()
        self.assertIsInstance(times, os.times_result)

        for field in ('user', 'system', 'children_user', 'children_system',
                      'elapsed'):
            value = getattr(times, field)
            self.assertIsInstance(value, float)

        if os.name == 'nt':
            self.assertEqual(times.children_user, 0)
            self.assertEqual(times.children_system, 0)
            self.assertEqual(times.elapsed, 0)


@support.requires_fork()
class ForkTests(unittest.TestCase):
    def test_fork(self):
        # bpo-42540: ensure os.fork() with non-default memory allocator does
        # not crash on exit.
        code = """if 1:
            import os
            from test import support
            pid = os.fork()
            if pid != 0:
                support.wait_process(pid, exitcode=0)
        """
        assert_python_ok("-c", code)
        if support.Py_GIL_DISABLED:
            assert_python_ok("-c", code, PYTHONMALLOC="mimalloc_debug")
        else:
            assert_python_ok("-c", code, PYTHONMALLOC="malloc_debug")

    @unittest.skipUnless(sys.platform in ("linux", "android", "darwin"),
                         "Only Linux and macOS detect this today.")
    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_fork_warns_when_non_python_thread_exists(self):
        code = """if 1:
            import os, threading, warnings
            from _testcapi import _spawn_pthread_waiter, _end_spawned_pthread
            _spawn_pthread_waiter()
            try:
                with warnings.catch_warnings(record=True) as ws:
                    warnings.filterwarnings(
                            "always", category=DeprecationWarning)
                    if os.fork() == 0:
                        assert not ws, f"unexpected warnings in child: {ws}"
                        os._exit(0)  # child
                    else:
                        assert ws[0].category == DeprecationWarning, ws[0]
                        assert 'fork' in str(ws[0].message), ws[0]
                        # Waiting allows an error in the child to hit stderr.
                        exitcode = os.wait()[1]
                        assert exitcode == 0, f"child exited {exitcode}"
                assert threading.active_count() == 1, threading.enumerate()
            finally:
                _end_spawned_pthread()
        """
        _, out, err = assert_python_ok("-c", code, PYTHONOPTIMIZE='0')
        self.assertEqual(err.decode("utf-8"), "")
        self.assertEqual(out.decode("utf-8"), "")

    def test_fork_at_finalization(self):
        code = """if 1:
            import atexit
            import os

            class AtFinalization:
                def __del__(self):
                    print("OK")
                    pid = os.fork()
                    if pid != 0:
                        print("shouldn't be printed")
            at_finalization = AtFinalization()
        """
        _, out, err = assert_python_ok("-c", code)
        self.assertEqual(b"OK\n", out)
        self.assertIn(b"can't fork at interpreter shutdown", err)


# Only test if the C version is provided, otherwise TestPEP519 already tested
# the pure Python implementation.
if hasattr(os, "_fspath"):
    class TestPEP519PurePython(TestPEP519):

        """Explicitly test the pure Python implementation of os.fspath()."""

        fspath = staticmethod(os._fspath)


if __name__ == "__main__":
    unittest.main()
