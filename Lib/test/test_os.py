# As a test suite for the os module, this is woefully inadequate, but this
# does add tests for a few functions which have been determined to be more
# portable than they had been thought to be.

import asynchat
import asyncore
import codecs
import contextlib
import decimal
import errno
import fractions
import getpass
import itertools
import locale
import mmap
import os
import pickle
import re
import shutil
import signal
import socket
import stat
import subprocess
import sys
import sysconfig
import time
import unittest
import uuid
import warnings
from test import support
try:
    import threading
except ImportError:
    threading = None
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
    import grp
    groups = [g.gr_gid for g in grp.getgrall() if getpass.getuser() in g.gr_mem]
    if hasattr(os, 'getgid'):
        process_gid = os.getgid()
        if process_gid not in groups:
            groups.append(process_gid)
except ImportError:
    groups = []
try:
    import pwd
    all_users = [u.pw_uid for u in pwd.getpwall()]
except (ImportError, AttributeError):
    all_users = []
try:
    from _testcapi import INT_MAX, PY_SSIZE_T_MAX
except ImportError:
    INT_MAX = PY_SSIZE_T_MAX = sys.maxsize

from test.support.script_helper import assert_python_ok
from test.support import unix_shell


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


@contextlib.contextmanager
def ignore_deprecation_warnings(msg_regex, quiet=False):
    with support.check_warnings((msg_regex, DeprecationWarning), quiet=quiet):
        yield


def requires_os_func(name):
    return unittest.skipUnless(hasattr(os, name), 'requires os.%s' % name)


class _PathLike(os.PathLike):

    def __init__(self, path=""):
        self.path = path

    def __str__(self):
        return str(self.path)

    def __fspath__(self):
        if isinstance(self.path, BaseException):
            raise self.path
        else:
            return self.path


def create_file(filename, content=b'content'):
    with open(filename, "xb", 0) as fp:
        fp.write(content)


# Tests creating TESTFN
class FileTests(unittest.TestCase):
    def setUp(self):
        if os.path.lexists(support.TESTFN):
            os.unlink(support.TESTFN)
    tearDown = setUp

    def test_access(self):
        f = os.open(support.TESTFN, os.O_CREAT|os.O_RDWR)
        os.close(f)
        self.assertTrue(os.access(support.TESTFN, os.W_OK))

    def test_closerange(self):
        first = os.open(support.TESTFN, os.O_CREAT|os.O_RDWR)
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
        path = support.TESTFN
        old = sys.getrefcount(path)
        self.assertRaises(TypeError, os.rename, path, 0)
        new = sys.getrefcount(path)
        self.assertEqual(old, new)

    def test_read(self):
        with open(support.TESTFN, "w+b") as fobj:
            fobj.write(b"spam")
            fobj.flush()
            fd = fobj.fileno()
            os.lseek(fd, 0, 0)
            s = os.read(fd, 4)
            self.assertEqual(type(s), bytes)
            self.assertEqual(s, b"spam")

    @support.cpython_only
    # Skip the test on 32-bit platforms: the number of bytes must fit in a
    # Py_ssize_t type
    @unittest.skipUnless(INT_MAX < PY_SSIZE_T_MAX,
                         "needs INT_MAX < PY_SSIZE_T_MAX")
    @support.bigmemtest(size=INT_MAX + 10, memuse=1, dry_run=False)
    def test_large_read(self, size):
        self.addCleanup(support.unlink, support.TESTFN)
        create_file(support.TESTFN, b'test')

        # Issue #21932: Make sure that os.read() does not raise an
        # OverflowError for size larger than INT_MAX
        with open(support.TESTFN, "rb") as fp:
            data = os.read(fp.fileno(), size)

        # The test does not try to read more than 2 GB at once because the
        # operating system is free to return less bytes than requested.
        self.assertEqual(data, b'test')

    def test_write(self):
        # os.write() accepts bytes- and buffer-like objects but not strings
        fd = os.open(support.TESTFN, os.O_CREAT | os.O_WRONLY)
        self.assertRaises(TypeError, os.write, fd, "beans")
        os.write(fd, b"bacon\n")
        os.write(fd, bytearray(b"eggs\n"))
        os.write(fd, memoryview(b"spam\n"))
        os.close(fd)
        with open(support.TESTFN, "rb") as fobj:
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
        fd = os.open(support.TESTFN, os.O_RDONLY)
        f = os.fdopen(fd, *args)
        f.close()

    def test_fdopen(self):
        fd = os.open(support.TESTFN, os.O_CREAT|os.O_RDWR)
        os.close(fd)

        self.fdopen_helper()
        self.fdopen_helper('r')
        self.fdopen_helper('r', 100)

    def test_replace(self):
        TESTFN2 = support.TESTFN + ".2"
        self.addCleanup(support.unlink, support.TESTFN)
        self.addCleanup(support.unlink, TESTFN2)

        create_file(support.TESTFN, b"1")
        create_file(TESTFN2, b"2")

        os.replace(support.TESTFN, TESTFN2)
        self.assertRaises(FileNotFoundError, os.stat, support.TESTFN)
        with open(TESTFN2, 'r') as f:
            self.assertEqual(f.read(), "1")

    def test_open_keywords(self):
        f = os.open(path=__file__, flags=os.O_RDONLY, mode=0o777,
            dir_fd=None)
        os.close(f)

    def test_symlink_keywords(self):
        symlink = support.get_attribute(os, "symlink")
        try:
            symlink(src='target', dst=support.TESTFN,
                target_is_directory=False, dir_fd=None)
        except (NotImplementedError, OSError):
            pass  # No OS support or unprivileged user


# Test attributes on return values from os.*stat* family.
class StatAttributeTests(unittest.TestCase):
    def setUp(self):
        self.fname = support.TESTFN
        self.addCleanup(support.unlink, self.fname)
        create_file(self.fname, b"ABC")

    @unittest.skipUnless(hasattr(os, 'stat'), 'test needs os.stat()')
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
            p = pickle.dumps(result, proto)
            self.assertIn(b'stat_result', p)
            if proto < 4:
                self.assertIn(b'cos\nstat_result\n', p)
            unpickled = pickle.loads(p)
            self.assertEqual(result, unpickled)

    @unittest.skipUnless(hasattr(os, 'statvfs'), 'test needs os.statvfs()')
    def test_statvfs_attributes(self):
        try:
            result = os.statvfs(self.fname)
        except OSError as e:
            # On AtheOS, glibc always returns ENOSYS
            if e.errno == errno.ENOSYS:
                self.skipTest('os.statvfs() failed with ENOSYS')

        # Make sure direct access works
        self.assertEqual(result.f_bfree, result[3])

        # Make sure all the attributes are there.
        members = ('bsize', 'frsize', 'blocks', 'bfree', 'bavail', 'files',
                    'ffree', 'favail', 'flag', 'namemax')
        for value, member in enumerate(members):
            self.assertEqual(getattr(result, 'f_' + member), result[value])

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
        try:
            result = os.statvfs(self.fname)
        except OSError as e:
            # On AtheOS, glibc always returns ENOSYS
            if e.errno == errno.ENOSYS:
                self.skipTest('os.statvfs() failed with ENOSYS')

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
        self.assertTrue(hasattr(result, 'st_file_attributes'))
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
        dirname = support.TESTFN + "dir"
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
        fname = os.path.join(os.environ['TEMP'], self.fname)
        self.addCleanup(support.unlink, fname)
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


class UtimeTests(unittest.TestCase):
    def setUp(self):
        self.dirname = support.TESTFN
        self.fname = os.path.join(self.dirname, "f1")

        self.addCleanup(support.rmtree, self.dirname)
        os.mkdir(self.dirname)
        create_file(self.fname)

        def restore_float_times(state):
            with ignore_deprecation_warnings('stat_float_times'):
                os.stat_float_times(state)

        # ensure that st_atime and st_mtime are float
        with ignore_deprecation_warnings('stat_float_times'):
            old_float_times = os.stat_float_times(-1)
            self.addCleanup(restore_float_times, old_float_times)

            os.stat_float_times(True)

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
        # pass times as floating point seconds as the second indexed parameter
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
            dirfd = os.open(dirname, os.O_RDONLY)
            try:
                # pass dir_fd to test utimensat(timespec) or futimesat(timeval)
                os.utime(name, dir_fd=dirfd, ns=ns)
            finally:
                os.close(dirfd)
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
        elif os.name == 'nt':
            # On Windows, the usual resolution of time.time() is 15.6 ms.
            # bpo-30649: Tolerate 50 ms for slow Windows buildbots.
            delta = 0.050
        else:
            # bpo-30649: PPC64 Fedora 3.x buildbot requires
            # at least a delta of 14 ms
            delta = 0.020
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
    def test_update2(self):
        os.environ.clear()
        os.environ.update(HELLO="World")
        with os.popen("%s -c 'echo $HELLO'" % unix_shell) as popen:
            value = popen.read().strip()
            self.assertEqual(value, "World")

    @unittest.skipUnless(unix_shell and os.path.exists(unix_shell),
                         'requires a shell')
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
        self.assertEqual(repr(env), 'environ({{{}}})'.format(', '.join(
            '{!r}: {!r}'.format(key, value)
            for key, value in env.items())))

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

    # On FreeBSD < 7 and OS X < 10.6, unsetenv() doesn't return a value (issue
    # #13415).
    @support.requires_freebsd_version(7)
    @support.requires_mac_ver(10, 6)
    def test_unset_error(self):
        if sys.platform == "win32":
            # an environment variable is limited to 32,767 characters
            key = 'x' * 50000
            self.assertRaises(ValueError, os.environ.__delitem__, key)
        else:
            # "=" is not allowed in a variable name
            key = 'key='
            self.assertRaises(OSError, os.environ.__delitem__, key)

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


class WalkTests(unittest.TestCase):
    """Tests for os.walk()."""

    # Wrapper to hide minor differences between os.walk and os.fwalk
    # to tests both functions with the same code base
    def walk(self, top, **kwargs):
        if 'follow_symlinks' in kwargs:
            kwargs['followlinks'] = kwargs.pop('follow_symlinks')
        return os.walk(top, **kwargs)

    def setUp(self):
        join = os.path.join
        self.addCleanup(support.rmtree, support.TESTFN)

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
        self.walk_path = join(support.TESTFN, "TEST1")
        self.sub1_path = join(self.walk_path, "SUB1")
        self.sub11_path = join(self.sub1_path, "SUB11")
        sub2_path = join(self.walk_path, "SUB2")
        sub21_path = join(sub2_path, "SUB21")
        tmp1_path = join(self.walk_path, "tmp1")
        tmp2_path = join(self.sub1_path, "tmp2")
        tmp3_path = join(sub2_path, "tmp3")
        tmp5_path = join(sub21_path, "tmp3")
        self.link_path = join(sub2_path, "link")
        t2_path = join(support.TESTFN, "TEST2")
        tmp4_path = join(support.TESTFN, "TEST2", "tmp4")
        broken_link_path = join(sub2_path, "broken_link")
        broken_link2_path = join(sub2_path, "broken_link2")
        broken_link3_path = join(sub2_path, "broken_link3")

        # Create stuff.
        os.makedirs(self.sub11_path)
        os.makedirs(sub2_path)
        os.makedirs(sub21_path)
        os.makedirs(t2_path)

        for path in tmp1_path, tmp2_path, tmp3_path, tmp4_path, tmp5_path:
            with open(path, "x") as f:
                f.write("I'm " + path + " and proud of it.  Blame test_os.\n")

        if support.can_symlink():
            os.symlink(os.path.abspath(t2_path), self.link_path)
            os.symlink('broken', broken_link_path, True)
            os.symlink(join('tmp3', 'broken'), broken_link2_path, True)
            os.symlink(join('SUB21', 'tmp5'), broken_link3_path, True)
            self.sub2_tree = (sub2_path, ["SUB21", "link"],
                              ["broken_link", "broken_link2", "broken_link3",
                               "tmp3"])
        else:
            self.sub2_tree = (sub2_path, [], ["tmp3"])

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
        self.assertEqual(all[0],
                         (str(walk_path), ["SUB2"], ["tmp1"]))

        all[1][-1].sort()
        all[1][1].sort()
        self.assertEqual(all[1], self.sub2_tree)

    def test_file_like_path(self):
        self.test_walk_prune(_PathLike(self.walk_path))

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
        if not support.can_symlink():
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


@unittest.skipUnless(hasattr(os, 'fwalk'), "Test needs os.fwalk()")
class FwalkTests(WalkTests):
    """Tests for os.fwalk()."""

    def walk(self, top, **kwargs):
        for root, dirs, files, root_fd in os.fwalk(top, **kwargs):
            yield (root, dirs, files)

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

            for root, dirs, files, rootfd in os.fwalk(**fwalk_kwargs):
                self.assertIn(root, expected)
                self.assertEqual(expected[root], (set(dirs), set(files)))

    def test_compare_to_walk(self):
        kwargs = {'top': support.TESTFN}
        self._compare_to_walk(kwargs, kwargs)

    def test_dir_fd(self):
        try:
            fd = os.open(".", os.O_RDONLY)
            walk_kwargs = {'top': support.TESTFN}
            fwalk_kwargs = walk_kwargs.copy()
            fwalk_kwargs['dir_fd'] = fd
            self._compare_to_walk(walk_kwargs, fwalk_kwargs)
        finally:
            os.close(fd)

    def test_yields_correct_dir_fd(self):
        # check returned file descriptors
        for topdown, follow_symlinks in itertools.product((True, False), repeat=2):
            args = support.TESTFN, topdown, None
            for root, dirs, files, rootfd in os.fwalk(*args, follow_symlinks=follow_symlinks):
                # check that the FD is valid
                os.fstat(rootfd)
                # redundant check
                os.stat(rootfd)
                # check that listdir() returns consistent information
                self.assertEqual(set(os.listdir(rootfd)), set(dirs) | set(files))

    def test_fd_leak(self):
        # Since we're opening a lot of FDs, we must be careful to avoid leaks:
        # we both check that calling fwalk() a large number of times doesn't
        # yield EMFILE, and that the minimum allocated FD hasn't changed.
        minfd = os.dup(1)
        os.close(minfd)
        for i in range(256):
            for x in os.fwalk(support.TESTFN):
                pass
        newfd = os.dup(1)
        self.addCleanup(os.close, newfd)
        self.assertEqual(newfd, minfd)

class BytesWalkTests(WalkTests):
    """Tests for os.walk() with bytes."""
    def setUp(self):
        super().setUp()
        self.stack = contextlib.ExitStack()

    def tearDown(self):
        self.stack.close()
        super().tearDown()

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


class MakedirTests(unittest.TestCase):
    def setUp(self):
        os.mkdir(support.TESTFN)

    def test_makedir(self):
        base = support.TESTFN
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

    def test_exist_ok_existing_directory(self):
        path = os.path.join(support.TESTFN, 'dir1')
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

    def test_exist_ok_s_isgid_directory(self):
        path = os.path.join(support.TESTFN, 'dir1')
        S_ISGID = stat.S_ISGID
        mode = 0o777
        old_mask = os.umask(0o022)
        try:
            existing_testfn_mode = stat.S_IMODE(
                    os.lstat(support.TESTFN).st_mode)
            try:
                os.chmod(support.TESTFN, existing_testfn_mode | S_ISGID)
            except PermissionError:
                raise unittest.SkipTest('Cannot set S_ISGID for dir.')
            if (os.lstat(support.TESTFN).st_mode & S_ISGID != S_ISGID):
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
        base = support.TESTFN
        path = os.path.join(support.TESTFN, 'dir1')
        f = open(path, 'w')
        f.write('abc')
        f.close()
        self.assertRaises(OSError, os.makedirs, path)
        self.assertRaises(OSError, os.makedirs, path, exist_ok=False)
        self.assertRaises(OSError, os.makedirs, path, exist_ok=True)
        os.remove(path)

    def tearDown(self):
        path = os.path.join(support.TESTFN, 'dir1', 'dir2', 'dir3',
                            'dir4', 'dir5', 'dir6')
        # If the tests failed, the bottom-most directory ('../dir6')
        # may not have been created, so we look for the outermost directory
        # that exists.
        while not os.path.exists(path) and path != support.TESTFN:
            path = os.path.dirname(path)

        os.removedirs(path)


@unittest.skipUnless(hasattr(os, 'chown'), "Test needs chown")
class ChownFileTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        os.mkdir(support.TESTFN)

    def test_chown_uid_gid_arguments_must_be_index(self):
        stat = os.stat(support.TESTFN)
        uid = stat.st_uid
        gid = stat.st_gid
        for value in (-1.0, -1j, decimal.Decimal(-1), fractions.Fraction(-2, 2)):
            self.assertRaises(TypeError, os.chown, support.TESTFN, value, gid)
            self.assertRaises(TypeError, os.chown, support.TESTFN, uid, value)
        self.assertIsNone(os.chown(support.TESTFN, uid, gid))
        self.assertIsNone(os.chown(support.TESTFN, -1, -1))

    @unittest.skipUnless(len(groups) > 1, "test needs more than one group")
    def test_chown(self):
        gid_1, gid_2 = groups[:2]
        uid = os.stat(support.TESTFN).st_uid
        os.chown(support.TESTFN, uid, gid_1)
        gid = os.stat(support.TESTFN).st_gid
        self.assertEqual(gid, gid_1)
        os.chown(support.TESTFN, uid, gid_2)
        gid = os.stat(support.TESTFN).st_gid
        self.assertEqual(gid, gid_2)

    @unittest.skipUnless(root_in_posix and len(all_users) > 1,
                         "test needs root privilege and more than one user")
    def test_chown_with_root(self):
        uid_1, uid_2 = all_users[:2]
        gid = os.stat(support.TESTFN).st_gid
        os.chown(support.TESTFN, uid_1, gid)
        uid = os.stat(support.TESTFN).st_uid
        self.assertEqual(uid, uid_1)
        os.chown(support.TESTFN, uid_2, gid)
        uid = os.stat(support.TESTFN).st_uid
        self.assertEqual(uid, uid_2)

    @unittest.skipUnless(not root_in_posix and len(all_users) > 1,
                         "test needs non-root account and more than one user")
    def test_chown_without_permission(self):
        uid_1, uid_2 = all_users[:2]
        gid = os.stat(support.TESTFN).st_gid
        with self.assertRaises(PermissionError):
            os.chown(support.TESTFN, uid_1, gid)
            os.chown(support.TESTFN, uid_2, gid)

    @classmethod
    def tearDownClass(cls):
        os.rmdir(support.TESTFN)


class RemoveDirsTests(unittest.TestCase):
    def setUp(self):
        os.makedirs(support.TESTFN)

    def tearDown(self):
        support.rmtree(support.TESTFN)

    def test_remove_all(self):
        dira = os.path.join(support.TESTFN, 'dira')
        os.mkdir(dira)
        dirb = os.path.join(dira, 'dirb')
        os.mkdir(dirb)
        os.removedirs(dirb)
        self.assertFalse(os.path.exists(dirb))
        self.assertFalse(os.path.exists(dira))
        self.assertFalse(os.path.exists(support.TESTFN))

    def test_remove_partial(self):
        dira = os.path.join(support.TESTFN, 'dira')
        os.mkdir(dira)
        dirb = os.path.join(dira, 'dirb')
        os.mkdir(dirb)
        create_file(os.path.join(dira, 'file.txt'))
        os.removedirs(dirb)
        self.assertFalse(os.path.exists(dirb))
        self.assertTrue(os.path.exists(dira))
        self.assertTrue(os.path.exists(support.TESTFN))

    def test_remove_nothing(self):
        dira = os.path.join(support.TESTFN, 'dira')
        os.mkdir(dira)
        dirb = os.path.join(dira, 'dirb')
        os.mkdir(dirb)
        create_file(os.path.join(dirb, 'file.txt'))
        with self.assertRaises(OSError):
            os.removedirs(dirb)
        self.assertTrue(os.path.exists(dirb))
        self.assertTrue(os.path.exists(dira))
        self.assertTrue(os.path.exists(support.TESTFN))


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
        self.assertEqual(len(stdout), 16)
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
        self.assertTrue(hasattr(os, 'GRND_RANDOM'))

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
        self.addCleanup(support.unlink, support.TESTFN)
        create_file(support.TESTFN, b"x" * 256)

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
            """.format(TESTFN=support.TESTFN)
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

        # null character in the enviroment variable name
        newenv = os.environ.copy()
        newenv["FRUIT\0VEGETABLE"] = "cabbage"
        with self.assertRaises(ValueError):
            os.execve(args[0], args, newenv)

        # null character in the enviroment variable value
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange\0VEGETABLE=cabbage"
        with self.assertRaises(ValueError):
            os.execve(args[0], args, newenv)

        # equal character in the enviroment variable name
        newenv = os.environ.copy()
        newenv["FRUIT=ORANGE"] = "lemon"
        with self.assertRaises(ValueError):
            os.execve(args[0], args, newenv)


@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32ErrorTests(unittest.TestCase):
    def setUp(self):
        try:
            os.stat(support.TESTFN)
        except FileNotFoundError:
            exists = False
        except OSError as exc:
            exists = True
            self.fail("file %s must not exist; os.stat failed with %s"
                      % (support.TESTFN, exc))
        else:
            self.fail("file %s must not exist" % support.TESTFN)

    def test_rename(self):
        self.assertRaises(OSError, os.rename, support.TESTFN, support.TESTFN+".bak")

    def test_remove(self):
        self.assertRaises(OSError, os.remove, support.TESTFN)

    def test_chdir(self):
        self.assertRaises(OSError, os.chdir, support.TESTFN)

    def test_mkdir(self):
        self.addCleanup(support.unlink, support.TESTFN)

        with open(support.TESTFN, "x") as f:
            self.assertRaises(OSError, os.mkdir, support.TESTFN)

    def test_utime(self):
        self.assertRaises(OSError, os.utime, support.TESTFN, None)

    def test_chmod(self):
        self.assertRaises(OSError, os.chmod, support.TESTFN, 0)


class TestInvalidFD(unittest.TestCase):
    singles = ["fchdir", "dup", "fdopen", "fdatasync", "fstat",
               "fstatvfs", "fsync", "tcgetpgrp", "ttyname"]
    #singles.append("close")
    #We omit close because it doesn't raise an exception on some platforms
    def get_single(f):
        def helper(self):
            if  hasattr(os, f):
                self.check(getattr(os, f))
        return helper
    for f in singles:
        locals()["test_"+f] = get_single(f)

    def check(self, f, *args):
        try:
            f(support.make_bad_fd(), *args)
        except OSError as e:
            self.assertEqual(e.errno, errno.EBADF)
        else:
            self.fail("%r didn't raise an OSError with a bad file descriptor"
                      % f)

    @unittest.skipUnless(hasattr(os, 'isatty'), 'test needs os.isatty()')
    def test_isatty(self):
        self.assertEqual(os.isatty(support.make_bad_fd()), False)

    @unittest.skipUnless(hasattr(os, 'closerange'), 'test needs os.closerange()')
    def test_closerange(self):
        fd = support.make_bad_fd()
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

    @unittest.skipUnless(hasattr(os, 'fchmod'), 'test needs os.fchmod()')
    def test_fchmod(self):
        self.check(os.fchmod, 0)

    @unittest.skipUnless(hasattr(os, 'fchown'), 'test needs os.fchown()')
    def test_fchown(self):
        self.check(os.fchown, -1, -1)

    @unittest.skipUnless(hasattr(os, 'fpathconf'), 'test needs os.fpathconf()')
    def test_fpathconf(self):
        self.check(os.pathconf, "PC_NAME_MAX")
        self.check(os.fpathconf, "PC_NAME_MAX")

    @unittest.skipUnless(hasattr(os, 'ftruncate'), 'test needs os.ftruncate()')
    def test_ftruncate(self):
        self.check(os.truncate, 0)
        self.check(os.ftruncate, 0)

    @unittest.skipUnless(hasattr(os, 'lseek'), 'test needs os.lseek()')
    def test_lseek(self):
        self.check(os.lseek, 0, 0)

    @unittest.skipUnless(hasattr(os, 'read'), 'test needs os.read()')
    def test_read(self):
        self.check(os.read, 1)

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

    def test_inheritable(self):
        self.check(os.get_inheritable)
        self.check(os.set_inheritable, True)

    @unittest.skipUnless(hasattr(os, 'get_blocking'),
                         'needs os.get_blocking() and os.set_blocking()')
    def test_blocking(self):
        self.check(os.get_blocking)
        self.check(os.set_blocking, True)


class LinkTests(unittest.TestCase):
    def setUp(self):
        self.file1 = support.TESTFN
        self.file2 = os.path.join(support.TESTFN + "2")

    def tearDown(self):
        for file in (self.file1, self.file2):
            if os.path.exists(file):
                os.unlink(file)

    def _test_link(self, file1, file2):
        create_file(file1)

        os.link(file1, file2)
        with open(file1, "r") as f1, open(file2, "r") as f2:
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
    @unittest.skipUnless(hasattr(os, 'setuid'), 'test needs os.setuid()')
    def test_setuid(self):
        if os.getuid() != 0:
            self.assertRaises(OSError, os.setuid, 0)
        self.assertRaises(OverflowError, os.setuid, 1<<32)

    @unittest.skipUnless(hasattr(os, 'setgid'), 'test needs os.setgid()')
    def test_setgid(self):
        if os.getuid() != 0 and not HAVE_WHEEL_GROUP:
            self.assertRaises(OSError, os.setgid, 0)
        self.assertRaises(OverflowError, os.setgid, 1<<32)

    @unittest.skipUnless(hasattr(os, 'seteuid'), 'test needs os.seteuid()')
    def test_seteuid(self):
        if os.getuid() != 0:
            self.assertRaises(OSError, os.seteuid, 0)
        self.assertRaises(OverflowError, os.seteuid, 1<<32)

    @unittest.skipUnless(hasattr(os, 'setegid'), 'test needs os.setegid()')
    def test_setegid(self):
        if os.getuid() != 0 and not HAVE_WHEEL_GROUP:
            self.assertRaises(OSError, os.setegid, 0)
        self.assertRaises(OverflowError, os.setegid, 1<<32)

    @unittest.skipUnless(hasattr(os, 'setreuid'), 'test needs os.setreuid()')
    def test_setreuid(self):
        if os.getuid() != 0:
            self.assertRaises(OSError, os.setreuid, 0, 0)
        self.assertRaises(OverflowError, os.setreuid, 1<<32, 0)
        self.assertRaises(OverflowError, os.setreuid, 0, 1<<32)

    @unittest.skipUnless(hasattr(os, 'setreuid'), 'test needs os.setreuid()')
    def test_setreuid_neg1(self):
        # Needs to accept -1.  We run this in a subprocess to avoid
        # altering the test runner's process state (issue8045).
        subprocess.check_call([
                sys.executable, '-c',
                'import os,sys;os.setreuid(-1,-1);sys.exit(0)'])

    @unittest.skipUnless(hasattr(os, 'setregid'), 'test needs os.setregid()')
    def test_setregid(self):
        if os.getuid() != 0 and not HAVE_WHEEL_GROUP:
            self.assertRaises(OSError, os.setregid, 0, 0)
        self.assertRaises(OverflowError, os.setregid, 1<<32, 0)
        self.assertRaises(OverflowError, os.setregid, 0, 1<<32)

    @unittest.skipUnless(hasattr(os, 'setregid'), 'test needs os.setregid()')
    def test_setregid_neg1(self):
        # Needs to accept -1.  We run this in a subprocess to avoid
        # altering the test runner's process state (issue8045).
        subprocess.check_call([
                sys.executable, '-c',
                'import os,sys;os.setregid(-1,-1);sys.exit(0)'])

@unittest.skipIf(sys.platform == "win32", "Posix specific tests")
class Pep383Tests(unittest.TestCase):
    def setUp(self):
        if support.TESTFN_UNENCODABLE:
            self.dir = support.TESTFN_UNENCODABLE
        elif support.TESTFN_NONASCII:
            self.dir = support.TESTFN_NONASCII
        else:
            self.dir = support.TESTFN
        self.bdir = os.fsencode(self.dir)

        bytesfn = []
        def add_filename(fn):
            try:
                fn = os.fsencode(fn)
            except UnicodeEncodeError:
                return
            bytesfn.append(fn)
        add_filename(support.TESTFN_UNICODE)
        if support.TESTFN_UNENCODABLE:
            add_filename(support.TESTFN_UNENCODABLE)
        if support.TESTFN_NONASCII:
            add_filename(support.TESTFN_NONASCII)
        if not bytesfn:
            self.skipTest("couldn't create any non-ascii filename")

        self.unicodefn = set()
        os.mkdir(self.dir)
        try:
            for fn in bytesfn:
                support.create_empty_file(os.path.join(self.bdir, fn))
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
            os.chdir(os.sep)
            self.assertEqual(set(os.listdir()), set(os.listdir(os.sep)))
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

    def _kill_with_event(self, event, name):
        tagname = "test_os_%s" % uuid.uuid1()
        m = mmap.mmap(-1, 1, tagname)
        m[0] = 0
        # Run a script which has console control handling enabled.
        proc = subprocess.Popen([sys.executable,
                   os.path.join(os.path.dirname(__file__),
                                "win_console_handler.py"), tagname],
                   creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
        # Let the interpreter startup before we send signals. See #3137.
        count, max = 0, 100
        while count < max and proc.poll() is None:
            if m[0] == 1:
                break
            time.sleep(0.1)
            count += 1
        else:
            # Forcefully kill the process if we weren't able to signal it.
            os.kill(proc.pid, signal.SIGINT)
            self.fail("Subprocess didn't finish initialization")
        os.kill(proc.pid, event)
        # proc.send_signal(event) could also be done here.
        # Allow time for the signal to be passed and the process to exit.
        time.sleep(0.5)
        if not proc.poll():
            # Forcefully kill the process if we weren't able to signal it.
            os.kill(proc.pid, signal.SIGINT)
            self.fail("subprocess did not stop on {}".format(name))

    @unittest.skip("subprocesses aren't inheriting Ctrl+C property")
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

    def test_CTRL_BREAK_EVENT(self):
        self._kill_with_event(signal.CTRL_BREAK_EVENT, "CTRL_BREAK_EVENT")


@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32ListdirTests(unittest.TestCase):
    """Test listdir on Windows."""

    def setUp(self):
        self.created_paths = []
        for i in range(2):
            dir_name = 'SUB%d' % i
            dir_path = os.path.join(support.TESTFN, dir_name)
            file_name = 'FILE%d' % i
            file_path = os.path.join(support.TESTFN, file_name)
            os.makedirs(dir_path)
            with open(file_path, 'w') as f:
                f.write("I'm %s and proud of it. Blame test_os.\n" % file_path)
            self.created_paths.extend([dir_name, file_name])
        self.created_paths.sort()

    def tearDown(self):
        shutil.rmtree(support.TESTFN)

    def test_listdir_no_extended_path(self):
        """Test when the path is not an "extended" path."""
        # unicode
        self.assertEqual(
                sorted(os.listdir(support.TESTFN)),
                self.created_paths)

        # bytes
        self.assertEqual(
                sorted(os.listdir(os.fsencode(support.TESTFN))),
                [os.fsencode(path) for path in self.created_paths])

    def test_listdir_extended_path(self):
        """Test when the path starts with '\\\\?\\'."""
        # See: http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#maxpath
        # unicode
        path = '\\\\?\\' + os.path.abspath(support.TESTFN)
        self.assertEqual(
                sorted(os.listdir(path)),
                self.created_paths)

        # bytes
        path = b'\\\\?\\' + os.fsencode(os.path.abspath(support.TESTFN))
        self.assertEqual(
                sorted(os.listdir(path)),
                [os.fsencode(path) for path in self.created_paths])


@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
@support.skip_unless_symlink
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

    @unittest.skip("currently fails; consider for improvement")
    def test_isdir_on_directory_link_to_missing_target(self):
        self._create_missing_dir_link()
        # consider having isdir return true for directory links
        self.assertTrue(os.path.isdir(self.missing_link))

    @unittest.skip("currently fails; consider for improvement")
    def test_rmdir_on_directory_link_to_missing_target(self):
        self._create_missing_dir_link()
        # consider allowing rmdir to remove directory links
        os.rmdir(self.missing_link)

    def check_stat(self, link, target):
        self.assertEqual(os.stat(link), os.stat(target))
        self.assertNotEqual(os.lstat(link), os.stat(link))

        bytes_link = os.fsencode(link)
        self.assertEqual(os.stat(bytes_link), os.stat(target))
        self.assertNotEqual(os.lstat(bytes_link), os.stat(bytes_link))

    def test_12084(self):
        level1 = os.path.abspath(support.TESTFN)
        level2 = os.path.join(level1, "level2")
        level3 = os.path.join(level2, "level3")
        self.addCleanup(support.rmtree, level1)

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


@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32JunctionTests(unittest.TestCase):
    junction = 'junctiontest'
    junction_target = os.path.dirname(os.path.abspath(__file__))

    def setUp(self):
        assert os.path.exists(self.junction_target)
        assert not os.path.exists(self.junction)

    def tearDown(self):
        if os.path.exists(self.junction):
            # os.rmdir delegates to Windows' RemoveDirectoryW,
            # which removes junction points safely.
            os.rmdir(self.junction)

    def test_create_junction(self):
        _winapi.CreateJunction(self.junction_target, self.junction)
        self.assertTrue(os.path.exists(self.junction))
        self.assertTrue(os.path.isdir(self.junction))

        # Junctions are not recognized as links.
        self.assertFalse(os.path.islink(self.junction))

    def test_unlink_removes_junction(self):
        _winapi.CreateJunction(self.junction_target, self.junction)
        self.assertTrue(os.path.exists(self.junction))

        os.unlink(self.junction)
        self.assertFalse(os.path.exists(self.junction))


@support.skip_unless_symlink
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

    @unittest.skipUnless(os.isatty(0) and (sys.platform.startswith('win') or
            (hasattr(locale, 'nl_langinfo') and hasattr(locale, 'CODESET'))),
            'test requires a tty and either Windows or nl_langinfo(CODESET)')
    def test_device_encoding(self):
        encoding = os.device_encoding(0)
        self.assertIsNotNone(encoding)
        self.assertTrue(codecs.lookup(encoding))


class PidTests(unittest.TestCase):
    @unittest.skipUnless(hasattr(os, 'getppid'), "test needs os.getppid")
    def test_getppid(self):
        p = subprocess.Popen([sys.executable, '-c',
                              'import os; print(os.getppid())'],
                             stdout=subprocess.PIPE)
        stdout, _ = p.communicate()
        # We are the parent of our subprocess
        self.assertEqual(int(stdout), os.getpid())

    def test_waitpid(self):
        args = [sys.executable, '-c', 'pass']
        # Add an implicit test for PyUnicode_FSConverter().
        pid = os.spawnv(os.P_NOWAIT, _PathLike(args[0]), args)
        status = os.waitpid(pid, 0)
        self.assertEqual(status, (pid, 0))


class SpawnTests(unittest.TestCase):
    def create_args(self, *, with_env=False, use_bytes=False):
        self.exitcode = 17

        filename = support.TESTFN
        self.addCleanup(support.unlink, filename)

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

        with open(filename, "w") as fp:
            fp.write(code)

        args = [sys.executable, filename]
        if use_bytes:
            args = [os.fsencode(a) for a in args]
            self.env = {os.fsencode(k): os.fsencode(v)
                        for k, v in self.env.items()}

        return args

    @requires_os_func('spawnl')
    def test_spawnl(self):
        args = self.create_args()
        exitcode = os.spawnl(os.P_WAIT, args[0], *args)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnle')
    def test_spawnle(self):
        args = self.create_args(with_env=True)
        exitcode = os.spawnle(os.P_WAIT, args[0], *args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnlp')
    def test_spawnlp(self):
        args = self.create_args()
        exitcode = os.spawnlp(os.P_WAIT, args[0], *args)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnlpe')
    def test_spawnlpe(self):
        args = self.create_args(with_env=True)
        exitcode = os.spawnlpe(os.P_WAIT, args[0], *args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnv')
    def test_spawnv(self):
        args = self.create_args()
        exitcode = os.spawnv(os.P_WAIT, args[0], args)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnve')
    def test_spawnve(self):
        args = self.create_args(with_env=True)
        exitcode = os.spawnve(os.P_WAIT, args[0], args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnvp')
    def test_spawnvp(self):
        args = self.create_args()
        exitcode = os.spawnvp(os.P_WAIT, args[0], args)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnvpe')
    def test_spawnvpe(self):
        args = self.create_args(with_env=True)
        exitcode = os.spawnvpe(os.P_WAIT, args[0], args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnv')
    def test_nowait(self):
        args = self.create_args()
        pid = os.spawnv(os.P_NOWAIT, args[0], args)
        result = os.waitpid(pid, 0)
        self.assertEqual(result[0], pid)
        status = result[1]
        if hasattr(os, 'WIFEXITED'):
            self.assertTrue(os.WIFEXITED(status))
            self.assertEqual(os.WEXITSTATUS(status), self.exitcode)
        else:
            self.assertEqual(status, self.exitcode << 8)

    @requires_os_func('spawnve')
    def test_spawnve_bytes(self):
        # Test bytes handling in parse_arglist and parse_envlist (#28114)
        args = self.create_args(with_env=True, use_bytes=True)
        exitcode = os.spawnve(os.P_WAIT, args[0], args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @requires_os_func('spawnl')
    def test_spawnl_noargs(self):
        args = self.create_args()
        self.assertRaises(ValueError, os.spawnl, os.P_NOWAIT, args[0])
        self.assertRaises(ValueError, os.spawnl, os.P_NOWAIT, args[0], '')

    @requires_os_func('spawnle')
    def test_spawnle_noargs(self):
        args = self.create_args()
        self.assertRaises(ValueError, os.spawnle, os.P_NOWAIT, args[0], {})
        self.assertRaises(ValueError, os.spawnle, os.P_NOWAIT, args[0], '', {})

    @requires_os_func('spawnv')
    def test_spawnv_noargs(self):
        args = self.create_args()
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, args[0], ())
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, args[0], [])
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, args[0], ('',))
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, args[0], [''])

    @requires_os_func('spawnve')
    def test_spawnve_noargs(self):
        args = self.create_args()
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, args[0], (), {})
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, args[0], [], {})
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, args[0], ('',), {})
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, args[0], [''], {})

    def _test_invalid_env(self, spawn):
        args = [sys.executable, '-c', 'pass']

        # null character in the enviroment variable name
        newenv = os.environ.copy()
        newenv["FRUIT\0VEGETABLE"] = "cabbage"
        try:
            exitcode = spawn(os.P_WAIT, args[0], args, newenv)
        except ValueError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # null character in the enviroment variable value
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange\0VEGETABLE=cabbage"
        try:
            exitcode = spawn(os.P_WAIT, args[0], args, newenv)
        except ValueError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # equal character in the enviroment variable name
        newenv = os.environ.copy()
        newenv["FRUIT=ORANGE"] = "lemon"
        try:
            exitcode = spawn(os.P_WAIT, args[0], args, newenv)
        except ValueError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # equal character in the enviroment variable value
        filename = support.TESTFN
        self.addCleanup(support.unlink, filename)
        with open(filename, "w") as fp:
            fp.write('import sys, os\n'
                     'if os.getenv("FRUIT") != "orange=lemon":\n'
                     '    raise AssertionError')
        args = [sys.executable, filename]
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange=lemon"
        exitcode = spawn(os.P_WAIT, args[0], args, newenv)
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
        os.setpriority(os.PRIO_PROCESS, os.getpid(), base + 1)
        try:
            new_prio = os.getpriority(os.PRIO_PROCESS, os.getpid())
            if base >= 19 and new_prio <= 19:
                raise unittest.SkipTest("unable to reliably test setpriority "
                                        "at current nice level of %s" % base)
            else:
                self.assertEqual(new_prio, base + 1)
        finally:
            try:
                os.setpriority(os.PRIO_PROCESS, os.getpid(), base)
            except OSError as err:
                if err.errno != errno.EACCES:
                    raise


if threading is not None:
    class SendfileTestServer(asyncore.dispatcher, threading.Thread):

        class Handler(asynchat.async_chat):

            def __init__(self, conn):
                asynchat.async_chat.__init__(self, conn)
                self.in_buffer = []
                self.closed = False
                self.push(b"220 ready\r\n")

            def handle_read(self):
                data = self.recv(4096)
                self.in_buffer.append(data)

            def get_data(self):
                return b''.join(self.in_buffer)

            def handle_close(self):
                self.close()
                self.closed = True

            def handle_error(self):
                raise

        def __init__(self, address):
            threading.Thread.__init__(self)
            asyncore.dispatcher.__init__(self)
            self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
            self.bind(address)
            self.listen(5)
            self.host, self.port = self.socket.getsockname()[:2]
            self.handler_instance = None
            self._active = False
            self._active_lock = threading.Lock()

        # --- public API

        @property
        def running(self):
            return self._active

        def start(self):
            assert not self.running
            self.__flag = threading.Event()
            threading.Thread.start(self)
            self.__flag.wait()

        def stop(self):
            assert self.running
            self._active = False
            self.join()

        def wait(self):
            # wait for handler connection to be closed, then stop the server
            while not getattr(self.handler_instance, "closed", False):
                time.sleep(0.001)
            self.stop()

        # --- internals

        def run(self):
            self._active = True
            self.__flag.set()
            while self._active and asyncore.socket_map:
                self._active_lock.acquire()
                asyncore.loop(timeout=0.001, count=1)
                self._active_lock.release()
            asyncore.close_all()

        def handle_accept(self):
            conn, addr = self.accept()
            self.handler_instance = self.Handler(conn)

        def handle_connect(self):
            self.close()
        handle_read = handle_connect

        def writable(self):
            return 0

        def handle_error(self):
            raise


@unittest.skipUnless(threading is not None, "test needs threading module")
@unittest.skipUnless(hasattr(os, 'sendfile'), "test needs os.sendfile()")
class TestSendfile(unittest.TestCase):

    DATA = b"12345abcde" * 16 * 1024  # 160 KB
    SUPPORT_HEADERS_TRAILERS = not sys.platform.startswith("linux") and \
                               not sys.platform.startswith("solaris") and \
                               not sys.platform.startswith("sunos")
    requires_headers_trailers = unittest.skipUnless(SUPPORT_HEADERS_TRAILERS,
            'requires headers and trailers support')

    @classmethod
    def setUpClass(cls):
        cls.key = support.threading_setup()
        create_file(support.TESTFN, cls.DATA)

    @classmethod
    def tearDownClass(cls):
        support.threading_cleanup(*cls.key)
        support.unlink(support.TESTFN)

    def setUp(self):
        self.server = SendfileTestServer((support.HOST, 0))
        self.server.start()
        self.client = socket.socket()
        self.client.connect((self.server.host, self.server.port))
        self.client.settimeout(1)
        # synchronize by waiting for "220 ready" response
        self.client.recv(1024)
        self.sockno = self.client.fileno()
        self.file = open(support.TESTFN, 'rb')
        self.fileno = self.file.fileno()

    def tearDown(self):
        self.file.close()
        self.client.close()
        if self.server.running:
            self.server.stop()
        self.server = None

    def sendfile_wrapper(self, sock, file, offset, nbytes, headers=[], trailers=[]):
        """A higher level wrapper representing how an application is
        supposed to use sendfile().
        """
        while 1:
            try:
                if self.SUPPORT_HEADERS_TRAILERS:
                    return os.sendfile(sock, file, offset, nbytes, headers,
                                       trailers)
                else:
                    return os.sendfile(sock, file, offset, nbytes)
            except OSError as err:
                if err.errno == errno.ECONNRESET:
                    # disconnected
                    raise
                elif err.errno in (errno.EAGAIN, errno.EBUSY):
                    # we have to retry send data
                    continue
                else:
                    raise

    def test_send_whole_file(self):
        # normal send
        total_sent = 0
        offset = 0
        nbytes = 4096
        while total_sent < len(self.DATA):
            sent = self.sendfile_wrapper(self.sockno, self.fileno, offset, nbytes)
            if sent == 0:
                break
            offset += sent
            total_sent += sent
            self.assertTrue(sent <= nbytes)
            self.assertEqual(offset, total_sent)

        self.assertEqual(total_sent, len(self.DATA))
        self.client.shutdown(socket.SHUT_RDWR)
        self.client.close()
        self.server.wait()
        data = self.server.handler_instance.get_data()
        self.assertEqual(len(data), len(self.DATA))
        self.assertEqual(data, self.DATA)

    def test_send_at_certain_offset(self):
        # start sending a file at a certain offset
        total_sent = 0
        offset = len(self.DATA) // 2
        must_send = len(self.DATA) - offset
        nbytes = 4096
        while total_sent < must_send:
            sent = self.sendfile_wrapper(self.sockno, self.fileno, offset, nbytes)
            if sent == 0:
                break
            offset += sent
            total_sent += sent
            self.assertTrue(sent <= nbytes)

        self.client.shutdown(socket.SHUT_RDWR)
        self.client.close()
        self.server.wait()
        data = self.server.handler_instance.get_data()
        expected = self.DATA[len(self.DATA) // 2:]
        self.assertEqual(total_sent, len(expected))
        self.assertEqual(len(data), len(expected))
        self.assertEqual(data, expected)

    def test_offset_overflow(self):
        # specify an offset > file size
        offset = len(self.DATA) + 4096
        try:
            sent = os.sendfile(self.sockno, self.fileno, offset, 4096)
        except OSError as e:
            # Solaris can raise EINVAL if offset >= file length, ignore.
            if e.errno != errno.EINVAL:
                raise
        else:
            self.assertEqual(sent, 0)
        self.client.shutdown(socket.SHUT_RDWR)
        self.client.close()
        self.server.wait()
        data = self.server.handler_instance.get_data()
        self.assertEqual(data, b'')

    def test_invalid_offset(self):
        with self.assertRaises(OSError) as cm:
            os.sendfile(self.sockno, self.fileno, -1, 4096)
        self.assertEqual(cm.exception.errno, errno.EINVAL)

    def test_keywords(self):
        # Keyword arguments should be supported
        os.sendfile(out=self.sockno, offset=0, count=4096,
            **{'in': self.fileno})
        if self.SUPPORT_HEADERS_TRAILERS:
            os.sendfile(self.sockno, self.fileno, offset=0, count=4096,
                headers=(), trailers=(), flags=0)

    # --- headers / trailers tests

    @requires_headers_trailers
    def test_headers(self):
        total_sent = 0
        sent = os.sendfile(self.sockno, self.fileno, 0, 4096,
                            headers=[b"x" * 512])
        total_sent += sent
        offset = 4096
        nbytes = 4096
        while 1:
            sent = self.sendfile_wrapper(self.sockno, self.fileno,
                                                    offset, nbytes)
            if sent == 0:
                break
            total_sent += sent
            offset += sent

        expected_data = b"x" * 512 + self.DATA
        self.assertEqual(total_sent, len(expected_data))
        self.client.close()
        self.server.wait()
        data = self.server.handler_instance.get_data()
        self.assertEqual(hash(data), hash(expected_data))

    @requires_headers_trailers
    def test_trailers(self):
        TESTFN2 = support.TESTFN + "2"
        file_data = b"abcdef"

        self.addCleanup(support.unlink, TESTFN2)
        create_file(TESTFN2, file_data)

        with open(TESTFN2, 'rb') as f:
            os.sendfile(self.sockno, f.fileno(), 0, len(file_data),
                        trailers=[b"1234"])
            self.client.close()
            self.server.wait()
            data = self.server.handler_instance.get_data()
            self.assertEqual(data, b"abcdef1234")

    @requires_headers_trailers
    @unittest.skipUnless(hasattr(os, 'SF_NODISKIO'),
                         'test needs os.SF_NODISKIO')
    def test_flags(self):
        try:
            os.sendfile(self.sockno, self.fileno, 0, 4096,
                        flags=os.SF_NODISKIO)
        except OSError as err:
            if err.errno not in (errno.EBUSY, errno.EAGAIN):
                raise


def supports_extended_attributes():
    if not hasattr(os, "setxattr"):
        return False

    try:
        with open(support.TESTFN, "xb", 0) as fp:
            try:
                os.setxattr(fp.fileno(), b"user.test", b"")
            except OSError:
                return False
    finally:
        support.unlink(support.TESTFN)

    return True


@unittest.skipUnless(supports_extended_attributes(),
                     "no non-broken extended attribute support")
# Kernels < 2.6.39 don't respect setxattr flags.
@support.requires_linux_version(2, 6, 39)
class ExtendedAttributeTests(unittest.TestCase):

    def _check_xattrs_str(self, s, getxattr, setxattr, removexattr, listxattr, **kwargs):
        fn = support.TESTFN
        self.addCleanup(support.unlink, fn)
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
        setxattr(fn, s("user.test"), b"a"*1024, **kwargs)
        self.assertEqual(getxattr(fn, s("user.test"), **kwargs), b"a"*1024)
        removexattr(fn, s("user.test"), **kwargs)
        many = sorted("user.test{}".format(i) for i in range(100))
        for thing in many:
            setxattr(fn, thing, b"x", **kwargs)
        self.assertEqual(set(listxattr(fn)), set(init_xattr) | set(many))

    def _check_xattrs(self, *args, **kwargs):
        self._check_xattrs_str(str, *args, **kwargs)
        support.unlink(support.TESTFN)

        self._check_xattrs_str(os.fsencode, *args, **kwargs)
        support.unlink(support.TESTFN)

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
            if sys.platform == "win32" or e.errno in (errno.EINVAL, errno.ENOTTY):
                # Under win32 a generic OSError can be thrown if the
                # handle cannot be retrieved
                self.skipTest("failed to query terminal size")
            raise

        self.assertGreaterEqual(size.columns, 0)
        self.assertGreaterEqual(size.lines, 0)

    def test_stty_match(self):
        """Check if stty returns the same results

        stty actually tests stdin, so get_terminal_size is invoked on
        stdin explicitly. If stty succeeded, then get_terminal_size()
        should work too.
        """
        try:
            size = subprocess.check_output(['stty', 'size']).decode().split()
        except (FileNotFoundError, subprocess.CalledProcessError):
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


class OSErrorTests(unittest.TestCase):
    def setUp(self):
        class Str(str):
            pass

        self.bytes_filenames = []
        self.unicode_filenames = []
        if support.TESTFN_UNENCODABLE is not None:
            decoded = support.TESTFN_UNENCODABLE
        else:
            decoded = support.TESTFN
        self.unicode_filenames.append(decoded)
        self.unicode_filenames.append(Str(decoded))
        if support.TESTFN_UNDECODABLE is not None:
            encoded = support.TESTFN_UNDECODABLE
        else:
            encoded = os.fsencode(support.TESTFN)
        self.bytes_filenames.append(encoded)
        self.bytes_filenames.append(bytearray(encoded))
        self.bytes_filenames.append(memoryview(encoded))

        self.filenames = self.bytes_filenames + self.unicode_filenames

    def test_oserror_filename(self):
        funcs = [
            (self.filenames, os.chdir,),
            (self.filenames, os.chmod, 0o777),
            (self.filenames, os.lstat,),
            (self.filenames, os.open, os.O_RDONLY),
            (self.filenames, os.rmdir,),
            (self.filenames, os.stat,),
            (self.filenames, os.unlink,),
        ]
        if sys.platform == "win32":
            funcs.extend((
                (self.bytes_filenames, os.rename, b"dst"),
                (self.bytes_filenames, os.replace, b"dst"),
                (self.unicode_filenames, os.rename, "dst"),
                (self.unicode_filenames, os.replace, "dst"),
                (self.unicode_filenames, os.listdir, ),
            ))
        else:
            funcs.extend((
                (self.filenames, os.listdir,),
                (self.filenames, os.rename, "dst"),
                (self.filenames, os.replace, "dst"),
            ))
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
            if sys.platform == "win32":
                funcs.append((self.bytes_filenames, os.link, b"dst"))
                funcs.append((self.unicode_filenames, os.link, "dst"))
            else:
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
            if sys.platform == "win32":
                funcs.append((self.unicode_filenames, os.readlink,))
            else:
                funcs.append((self.filenames, os.readlink,))


        for filenames, func, *func_args in funcs:
            for name in filenames:
                try:
                    if isinstance(name, (str, bytes)):
                        func(name, *func_args)
                    else:
                        with self.assertWarnsRegex(DeprecationWarning, 'should be'):
                            func(name, *func_args)
                except OSError as err:
                    self.assertIs(err.filename, name, str(func))
                except UnicodeDecodeError:
                    pass
                else:
                    self.fail("No exception thrown by {}".format(func))

class CPUCountTests(unittest.TestCase):
    def test_cpu_count(self):
        cpus = os.cpu_count()
        if cpus is not None:
            self.assertIsInstance(cpus, int)
            self.assertGreater(cpus, 0)
        else:
            self.skipTest("Could not determine the number of CPUs")


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

    @unittest.skipUnless(hasattr(os, 'dup2'), "need os.dup2()")
    def test_dup2(self):
        fd = os.open(__file__, os.O_RDONLY)
        self.addCleanup(os.close, fd)

        # inheritable by default
        fd2 = os.open(__file__, os.O_RDONLY)
        try:
            os.dup2(fd, fd2)
            self.assertEqual(os.get_inheritable(fd2), True)
        finally:
            os.close(fd2)

        # force non-inheritable
        fd3 = os.open(__file__, os.O_RDONLY)
        try:
            os.dup2(fd, fd3, inheritable=False)
            self.assertEqual(os.get_inheritable(fd3), False)
        finally:
            os.close(fd3)

    @unittest.skipUnless(hasattr(os, 'openpty'), "need os.openpty()")
    def test_openpty(self):
        master_fd, slave_fd = os.openpty()
        self.addCleanup(os.close, master_fd)
        self.addCleanup(os.close, slave_fd)
        self.assertEqual(os.get_inheritable(master_fd), False)
        self.assertEqual(os.get_inheritable(slave_fd), False)


class PathTConverterTests(unittest.TestCase):
    # tuples of (function name, allows fd arguments, additional arguments to
    # function, cleanup function)
    functions = [
        ('stat', True, (), None),
        ('lstat', False, (), None),
        ('access', False, (os.F_OK,), None),
        ('chflags', False, (0,), None),
        ('lchflags', False, (0,), None),
        ('open', False, (0,), getattr(os, 'close', None)),
    ]

    def test_path_t_converter(self):
        str_filename = support.TESTFN
        if os.name == 'nt':
            bytes_fspath = bytes_filename = None
        else:
            bytes_filename = support.TESTFN.encode('ascii')
            bytes_fspath = _PathLike(bytes_filename)
        fd = os.open(_PathLike(str_filename), os.O_WRONLY|os.O_CREAT)
        self.addCleanup(support.unlink, support.TESTFN)
        self.addCleanup(os.close, fd)

        int_fspath = _PathLike(fd)
        str_fspath = _PathLike(str_filename)

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
                        TypeError, 'should be string, bytes'):
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


@unittest.skipUnless(hasattr(os, 'get_blocking'),
                     'needs os.get_blocking() and os.set_blocking()')
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


class TestScandir(unittest.TestCase):
    check_no_resource_warning = support.check_no_resource_warning

    def setUp(self):
        self.path = os.path.realpath(support.TESTFN)
        self.bytes_path = os.fsencode(self.path)
        self.addCleanup(support.rmtree, self.path)
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
                if attr in ("st_dev", "st_ino", "st_nlink"):
                    continue
                self.assertEqual(getattr(stat1, attr),
                                 getattr(stat2, attr),
                                 (stat1, stat2, attr))
        else:
            self.assertEqual(stat1, stat2)

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

        self.assert_stat_equal(entry.stat(),
                               entry_stat,
                               os.name == 'nt' and not is_symlink)
        self.assert_stat_equal(entry.stat(follow_symlinks=False),
                               entry_lstat,
                               os.name == 'nt')

    def test_attributes(self):
        link = hasattr(os, 'link')
        symlink = support.can_symlink()

        dirname = os.path.join(self.path, "dir")
        os.mkdir(dirname)
        filename = self.create_file("file.txt")
        if link:
            os.link(filename, os.path.join(self.path, "link_file.txt"))
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
        if not support.can_symlink():
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
            with self.assertWarns(DeprecationWarning):
                entries = list(os.scandir(path_bytes))
            self.assertEqual(len(entries), 1, entries)
            entry = entries[0]

            self.assertEqual(entry.name, b'file.txt')
            self.assertEqual(entry.path,
                             os.fsencode(os.path.join(self.path, 'file.txt')))
            self.assertIs(type(entry.name), bytes)
            self.assertIs(type(entry.path), bytes)

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
        for obj in [1234, 1.234, {}, []]:
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
            pathlike = _PathLike(p)

            self.assertEqual(p, self.fspath(pathlike))
            self.assertEqual(b"path/like/object", os.fsencode(pathlike))
            self.assertEqual("path/like/object", os.fsdecode(pathlike))

    def test_pathlike(self):
        self.assertEqual('#feelthegil', self.fspath(_PathLike('#feelthegil')))
        self.assertTrue(issubclass(_PathLike, os.PathLike))
        self.assertTrue(isinstance(_PathLike(), os.PathLike))

    def test_garbage_in_exception_out(self):
        vapor = type('blah', (), {})
        for o in int, type, os, vapor():
            self.assertRaises(TypeError, self.fspath, o)

    def test_argument_required(self):
        self.assertRaises(TypeError, self.fspath)

    def test_bad_pathlike(self):
        # __fspath__ returns a value other than str or bytes.
        self.assertRaises(TypeError, self.fspath, _PathLike(42))
        # __fspath__ attribute that is not callable.
        c = type('foo', (), {})
        c.__fspath__ = 1
        self.assertRaises(TypeError, self.fspath, c())
        # __fspath__ raises an exception.
        self.assertRaises(ZeroDivisionError, self.fspath,
                          _PathLike(ZeroDivisionError()))

# Only test if the C version is provided, otherwise TestPEP519 already tested
# the pure Python implementation.
if hasattr(os, "_fspath"):
    class TestPEP519PurePython(TestPEP519):

        """Explicitly test the pure Python implementation of os.fspath()."""

        fspath = staticmethod(os._fspath)


if __name__ == "__main__":
    unittest.main()
