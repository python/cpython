"""Test program for the fcntl C module.
"""
import errno
import multiprocessing
import platform
import os
import struct
import sys
import unittest
from test.support import (
    cpython_only, get_pagesize, is_apple, requires_subprocess, verbose
)
from test.support.import_helper import import_module
from test.support.os_helper import TESTFN, unlink, make_bad_fd


# Skip test if no fcntl module.
fcntl = import_module('fcntl')



class BadFile:
    def __init__(self, fn):
        self.fn = fn
    def fileno(self):
        return self.fn

def try_lockf_on_other_process_fail(fname, cmd):
    f = open(fname, 'wb+')
    try:
        fcntl.lockf(f, cmd)
    except BlockingIOError:
        pass
    finally:
        f.close()

def try_lockf_on_other_process(fname, cmd):
    f = open(fname, 'wb+')
    fcntl.lockf(f, cmd)
    fcntl.lockf(f, fcntl.LOCK_UN)
    f.close()

class TestFcntl(unittest.TestCase):

    def setUp(self):
        self.f = None

    def tearDown(self):
        if self.f and not self.f.closed:
            self.f.close()
        unlink(TESTFN)

    @staticmethod
    def get_lockdata():
        try:
            os.O_LARGEFILE
        except AttributeError:
            start_len = "ll"
        else:
            start_len = "qq"

        if (
            sys.platform.startswith(('netbsd', 'freebsd', 'openbsd'))
            or is_apple
        ):
            if struct.calcsize('l') == 8:
                off_t = 'l'
                pid_t = 'i'
            else:
                off_t = 'lxxxx'
                pid_t = 'l'
            lockdata = struct.pack(off_t + off_t + pid_t + 'hh', 0, 0, 0,
                                   fcntl.F_WRLCK, 0)
        elif sys.platform.startswith('gnukfreebsd'):
            lockdata = struct.pack('qqihhi', 0, 0, 0, fcntl.F_WRLCK, 0, 0)
        elif sys.platform in ['hp-uxB', 'unixware7']:
            lockdata = struct.pack('hhlllii', fcntl.F_WRLCK, 0, 0, 0, 0, 0, 0)
        else:
            lockdata = struct.pack('hh'+start_len+'hh', fcntl.F_WRLCK, 0, 0, 0, 0, 0)
        if lockdata:
            if verbose:
                print('struct.pack: ', repr(lockdata))
        return lockdata

    def test_fcntl_fileno(self):
        # the example from the library docs
        self.f = open(TESTFN, 'wb')
        rv = fcntl.fcntl(self.f.fileno(), fcntl.F_SETFL, os.O_NONBLOCK)
        if verbose:
            print('Status from fcntl with O_NONBLOCK: ', rv)
        lockdata = self.get_lockdata()
        rv = fcntl.fcntl(self.f.fileno(), fcntl.F_SETLKW, lockdata)
        if verbose:
            print('String from fcntl with F_SETLKW: ', repr(rv))
        self.f.close()

    def test_fcntl_file_descriptor(self):
        # again, but pass the file rather than numeric descriptor
        self.f = open(TESTFN, 'wb')
        rv = fcntl.fcntl(self.f, fcntl.F_SETFL, os.O_NONBLOCK)
        if verbose:
            print('Status from fcntl with O_NONBLOCK: ', rv)
        lockdata = self.get_lockdata()
        rv = fcntl.fcntl(self.f, fcntl.F_SETLKW, lockdata)
        if verbose:
            print('String from fcntl with F_SETLKW: ', repr(rv))
        self.f.close()

    def test_fcntl_bad_file(self):
        with self.assertRaises(ValueError):
            fcntl.fcntl(-1, fcntl.F_SETFL, os.O_NONBLOCK)
        with self.assertRaises(ValueError):
            fcntl.fcntl(BadFile(-1), fcntl.F_SETFL, os.O_NONBLOCK)
        with self.assertRaises(TypeError):
            fcntl.fcntl('spam', fcntl.F_SETFL, os.O_NONBLOCK)
        with self.assertRaises(TypeError):
            fcntl.fcntl(BadFile('spam'), fcntl.F_SETFL, os.O_NONBLOCK)

    @cpython_only
    def test_fcntl_bad_file_overflow(self):
        _testcapi = import_module("_testcapi")
        INT_MAX = _testcapi.INT_MAX
        INT_MIN = _testcapi.INT_MIN
        # Issue 15989
        with self.assertRaises(OverflowError):
            fcntl.fcntl(INT_MAX + 1, fcntl.F_SETFL, os.O_NONBLOCK)
        with self.assertRaises(OverflowError):
            fcntl.fcntl(BadFile(INT_MAX + 1), fcntl.F_SETFL, os.O_NONBLOCK)
        with self.assertRaises(OverflowError):
            fcntl.fcntl(INT_MIN - 1, fcntl.F_SETFL, os.O_NONBLOCK)
        with self.assertRaises(OverflowError):
            fcntl.fcntl(BadFile(INT_MIN - 1), fcntl.F_SETFL, os.O_NONBLOCK)

    @unittest.skipIf(
        (platform.machine().startswith("arm") and platform.system() == "Linux")
        or platform.system() == "Android",
        "this platform returns EINVAL for F_NOTIFY DN_MULTISHOT")
    def test_fcntl_64_bit(self):
        # Issue GH-42434: fcntl shouldn't fail when the third arg fits in a
        # C 'long' but not in a C 'int'.
        try:
            cmd = fcntl.F_NOTIFY
            # DN_MULTISHOT is >= 2**31 in 64-bit builds
            flags = fcntl.DN_MULTISHOT
        except AttributeError:
            self.skipTest("F_NOTIFY or DN_MULTISHOT unavailable")
        fd = os.open(os.path.dirname(os.path.abspath(TESTFN)), os.O_RDONLY)
        try:
            try:
                fcntl.fcntl(fd, cmd, fcntl.DN_DELETE)
            except OSError as exc:
                if exc.errno == errno.EINVAL:
                    self.skipTest("F_NOTIFY not available by this environment")
            fcntl.fcntl(fd, cmd, flags)
        finally:
            os.close(fd)

    def test_flock(self):
        # Solaris needs readable file for shared lock
        self.f = open(TESTFN, 'wb+')
        fileno = self.f.fileno()
        fcntl.flock(fileno, fcntl.LOCK_SH)
        fcntl.flock(fileno, fcntl.LOCK_UN)
        fcntl.flock(self.f, fcntl.LOCK_SH | fcntl.LOCK_NB)
        fcntl.flock(self.f, fcntl.LOCK_UN)
        fcntl.flock(fileno, fcntl.LOCK_EX)
        fcntl.flock(fileno, fcntl.LOCK_UN)

        self.assertRaises(ValueError, fcntl.flock, -1, fcntl.LOCK_SH)
        self.assertRaises(TypeError, fcntl.flock, 'spam', fcntl.LOCK_SH)

    @unittest.skipIf(platform.system() == "AIX", "AIX returns PermissionError")
    @requires_subprocess()
    def test_lockf_exclusive(self):
        self.f = open(TESTFN, 'wb+')
        cmd = fcntl.LOCK_EX | fcntl.LOCK_NB
        fcntl.lockf(self.f, cmd)
        mp = multiprocessing.get_context('spawn')
        p = mp.Process(target=try_lockf_on_other_process_fail, args=(TESTFN, cmd))
        p.start()
        p.join()
        fcntl.lockf(self.f, fcntl.LOCK_UN)
        self.assertEqual(p.exitcode, 0)

    @unittest.skipIf(platform.system() == "AIX", "AIX returns PermissionError")
    @requires_subprocess()
    def test_lockf_share(self):
        self.f = open(TESTFN, 'wb+')
        cmd = fcntl.LOCK_SH | fcntl.LOCK_NB
        fcntl.lockf(self.f, cmd)
        mp = multiprocessing.get_context('spawn')
        p = mp.Process(target=try_lockf_on_other_process, args=(TESTFN, cmd))
        p.start()
        p.join()
        fcntl.lockf(self.f, fcntl.LOCK_UN)
        self.assertEqual(p.exitcode, 0)

    @cpython_only
    def test_flock_overflow(self):
        _testcapi = import_module("_testcapi")
        self.assertRaises(OverflowError, fcntl.flock, _testcapi.INT_MAX+1,
                          fcntl.LOCK_SH)

    @unittest.skipIf(sys.platform != 'darwin', "F_GETPATH is only available on macos")
    def test_fcntl_f_getpath(self):
        self.f = open(TESTFN, 'wb')
        expected = os.path.abspath(TESTFN).encode('utf-8')
        res = fcntl.fcntl(self.f.fileno(), fcntl.F_GETPATH, bytes(len(expected)))
        self.assertEqual(expected, res)

    @unittest.skipUnless(
        hasattr(fcntl, "F_SETPIPE_SZ") and hasattr(fcntl, "F_GETPIPE_SZ"),
        "F_SETPIPE_SZ and F_GETPIPE_SZ are not available on all platforms.")
    def test_fcntl_f_pipesize(self):
        test_pipe_r, test_pipe_w = os.pipe()
        try:
            # Get the default pipesize with F_GETPIPE_SZ
            pipesize_default = fcntl.fcntl(test_pipe_w, fcntl.F_GETPIPE_SZ)
            pipesize = pipesize_default // 2  # A new value to detect change.
            pagesize_default = get_pagesize()
            if pipesize < pagesize_default:  # the POSIX minimum
                raise unittest.SkipTest(
                    'default pipesize too small to perform test.')
            fcntl.fcntl(test_pipe_w, fcntl.F_SETPIPE_SZ, pipesize)
            self.assertEqual(fcntl.fcntl(test_pipe_w, fcntl.F_GETPIPE_SZ),
                             pipesize)
        finally:
            os.close(test_pipe_r)
            os.close(test_pipe_w)

    def _check_fcntl_not_mutate_len(self, nbytes=None):
        self.f = open(TESTFN, 'wb')
        buf = struct.pack('ii', fcntl.F_OWNER_PID, os.getpid())
        if nbytes is not None:
            buf += b' ' * (nbytes - len(buf))
        else:
            nbytes = len(buf)
        save_buf = bytes(buf)
        r = fcntl.fcntl(self.f, fcntl.F_SETOWN_EX, buf)
        self.assertIsInstance(r, bytes)
        self.assertEqual(len(r), len(save_buf))
        self.assertEqual(buf, save_buf)
        type, pid = memoryview(r).cast('i')[:2]
        self.assertEqual(type, fcntl.F_OWNER_PID)
        self.assertEqual(pid, os.getpid())

        buf = b' ' * nbytes
        r = fcntl.fcntl(self.f, fcntl.F_GETOWN_EX, buf)
        self.assertIsInstance(r, bytes)
        self.assertEqual(len(r), len(save_buf))
        self.assertEqual(buf, b' ' * nbytes)
        type, pid = memoryview(r).cast('i')[:2]
        self.assertEqual(type, fcntl.F_OWNER_PID)
        self.assertEqual(pid, os.getpid())

        buf = memoryview(b' ' * nbytes)
        r = fcntl.fcntl(self.f, fcntl.F_GETOWN_EX, buf)
        self.assertIsInstance(r, bytes)
        self.assertEqual(len(r), len(save_buf))
        self.assertEqual(bytes(buf), b' ' * nbytes)
        type, pid = memoryview(r).cast('i')[:2]
        self.assertEqual(type, fcntl.F_OWNER_PID)
        self.assertEqual(pid, os.getpid())

    @unittest.skipUnless(
        hasattr(fcntl, "F_SETOWN_EX") and hasattr(fcntl, "F_GETOWN_EX"),
        "requires F_SETOWN_EX and F_GETOWN_EX")
    def test_fcntl_small_buffer(self):
        self._check_fcntl_not_mutate_len()

    @unittest.skipUnless(
        hasattr(fcntl, "F_SETOWN_EX") and hasattr(fcntl, "F_GETOWN_EX"),
        "requires F_SETOWN_EX and F_GETOWN_EX")
    def test_fcntl_large_buffer(self):
        self._check_fcntl_not_mutate_len(2024)

    @unittest.skipUnless(hasattr(fcntl, 'F_DUPFD'), 'need fcntl.F_DUPFD')
    def test_bad_fd(self):
        # gh-134744: Test error handling
        fd = make_bad_fd()
        with self.assertRaises(OSError):
            fcntl.fcntl(fd, fcntl.F_DUPFD, 0)
        with self.assertRaises(OSError):
            fcntl.fcntl(fd, fcntl.F_DUPFD, b'\0' * 10)
        with self.assertRaises(OSError):
            fcntl.fcntl(fd, fcntl.F_DUPFD, b'\0' * 2048)


if __name__ == '__main__':
    unittest.main()
