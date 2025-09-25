"""
Test the os.utime() function.
"""

import decimal
import fractions
import os
import posix
import sys
import time
import unittest
from test import support
from test.support import os_helper
from .utils import create_file


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

    @staticmethod
    def ns_to_sec_decimal(ns):
        # Convert a number of nanosecond (int) to a number of seconds (Decimal).
        # Round towards infinity by adding 0.5 nanosecond to avoid rounding
        # issue, os.utime() rounds towards minus infinity.
        return decimal.Decimal('1e-9') * ns + decimal.Decimal('0.5e-9')

    @staticmethod
    def ns_to_sec_fraction(ns):
        # Convert a number of nanosecond (int) to a number of seconds (Fraction).
        # Round towards infinity by adding 0.5 nanosecond to avoid rounding
        # issue, os.utime() rounds towards minus infinity.
        return fractions.Fraction(ns, 10**9) + fractions.Fraction(1, 2*10**9)

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

    def test_utime_decimal(self):
        # pass times as Decimal seconds
        def set_time(filename, ns):
            atime_ns, mtime_ns = ns
            atime = self.ns_to_sec_decimal(atime_ns)
            mtime = self.ns_to_sec_decimal(mtime_ns)
            os.utime(filename, (atime, mtime))
        self._test_utime(set_time)

    def test_utime_fraction(self):
        # pass times as Fraction seconds
        def set_time(filename, ns):
            atime_ns, mtime_ns = ns
            atime = self.ns_to_sec_fraction(atime_ns)
            mtime = self.ns_to_sec_fraction(mtime_ns)
            os.utime(filename, (atime, mtime))
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

        times = (
            5000000000,  # some day in 2128
            # boundaries of the fast path cutoff in posixmodule.c:fill_time
            -9223372037, -9223372036, 9223372035, 9223372036,
        )
        for large in times:
            with self.subTest(large=large):
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


class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        create_file(os_helper.TESTFN, b'')

    @unittest.skipUnless(os.utime in os.supports_fd, "test needs fd support in os.utime")
    def test_utime_with_fd(self):
        now = time.time()
        fd = os.open(os_helper.TESTFN, os.O_RDONLY)
        try:
            posix.utime(fd)
            posix.utime(fd, None)
            self.assertRaises(TypeError, posix.utime, fd, (None, None))
            self.assertRaises(TypeError, posix.utime, fd, (now, None))
            self.assertRaises(TypeError, posix.utime, fd, (None, now))
            posix.utime(fd, (int(now), int(now)))
            posix.utime(fd, (now, now))
            self.assertRaises(ValueError, posix.utime, fd, (now, now), ns=(now, now))
            self.assertRaises(ValueError, posix.utime, fd, (now, 0), ns=(None, None))
            self.assertRaises(ValueError, posix.utime, fd, (None, None), ns=(now, 0))
            posix.utime(fd, (int(now), int((now - int(now)) * 1e9)))
            posix.utime(fd, ns=(int(now), int((now - int(now)) * 1e9)))

        finally:
            os.close(fd)

    @unittest.skipUnless(os.utime in os.supports_follow_symlinks, "test needs follow_symlinks support in os.utime")
    def test_utime_nofollow_symlinks(self):
        now = time.time()
        posix.utime(os_helper.TESTFN, None, follow_symlinks=False)
        self.assertRaises(TypeError, posix.utime, os_helper.TESTFN,
                          (None, None), follow_symlinks=False)
        self.assertRaises(TypeError, posix.utime, os_helper.TESTFN,
                          (now, None), follow_symlinks=False)
        self.assertRaises(TypeError, posix.utime, os_helper.TESTFN,
                          (None, now), follow_symlinks=False)
        posix.utime(os_helper.TESTFN, (int(now), int(now)),
                    follow_symlinks=False)
        posix.utime(os_helper.TESTFN, (now, now), follow_symlinks=False)
        posix.utime(os_helper.TESTFN, follow_symlinks=False)

    @unittest.skipUnless(hasattr(posix, 'utime'), 'test needs posix.utime()')
    def test_utime(self):
        now = time.time()
        posix.utime(os_helper.TESTFN, None)
        self.assertRaises(TypeError, posix.utime,
                          os_helper.TESTFN, (None, None))
        self.assertRaises(TypeError, posix.utime,
                          os_helper.TESTFN, (now, None))
        self.assertRaises(TypeError, posix.utime,
                          os_helper.TESTFN, (None, now))
        posix.utime(os_helper.TESTFN, (int(now), int(now)))
        posix.utime(os_helper.TESTFN, (now, now))


if __name__ == "__main__":
    unittest.main()
