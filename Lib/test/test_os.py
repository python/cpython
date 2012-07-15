# As a test suite for the os module, this is woefully inadequate, but this
# does add tests for a few functions which have been determined to be more
# portable than they had been thought to be.

import os
import errno
import unittest
import warnings
import sys
import signal
import subprocess
import time
import shutil
from test import support
import contextlib
import mmap
import platform
import re
import uuid
import asyncore
import asynchat
import socket
import itertools
import stat
import locale
import codecs
try:
    import threading
except ImportError:
    threading = None
from test.script_helper import assert_python_ok

with warnings.catch_warnings():
    warnings.simplefilter("ignore", DeprecationWarning)
    os.stat_float_times(True)
st = os.stat(__file__)
stat_supports_subsecond = (
    # check if float and int timestamps are different
    (st.st_atime != st[7])
    or (st.st_mtime != st[8])
    or (st.st_ctime != st[9]))

# Detect whether we're on a Linux system that uses the (now outdated
# and unmaintained) linuxthreads threading library.  There's an issue
# when combining linuxthreads with a failed execv call: see
# http://bugs.python.org/issue4970.
if hasattr(sys, 'thread_info') and sys.thread_info.version:
    USING_LINUXTHREADS = sys.thread_info.version.startswith("linuxthreads")
else:
    USING_LINUXTHREADS = False

# Tests creating TESTFN
class FileTests(unittest.TestCase):
    def setUp(self):
        if os.path.exists(support.TESTFN):
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
        with open(support.TESTFN, 'w') as f:
            f.write("1")
        with open(TESTFN2, 'w') as f:
            f.write("2")
        self.addCleanup(os.unlink, TESTFN2)
        os.replace(support.TESTFN, TESTFN2)
        self.assertRaises(FileNotFoundError, os.stat, support.TESTFN)
        with open(TESTFN2, 'r') as f:
            self.assertEqual(f.read(), "1")


# Test attributes on return values from os.*stat* family.
class StatAttributeTests(unittest.TestCase):
    def setUp(self):
        os.mkdir(support.TESTFN)
        self.fname = os.path.join(support.TESTFN, "f1")
        f = open(self.fname, 'wb')
        f.write(b"ABC")
        f.close()

    def tearDown(self):
        os.unlink(self.fname)
        os.rmdir(support.TESTFN)

    def check_stat_attributes(self, fname):
        if not hasattr(os, "stat"):
            return

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
            self.fail("No exception thrown")
        except IndexError:
            pass

        # Make sure that assignment fails
        try:
            result.st_mode = 1
            self.fail("No exception thrown")
        except AttributeError:
            pass

        try:
            result.st_rdev = 1
            self.fail("No exception thrown")
        except (AttributeError, TypeError):
            pass

        try:
            result.parrot = 1
            self.fail("No exception thrown")
        except AttributeError:
            pass

        # Use the stat_result constructor with a too-short tuple.
        try:
            result2 = os.stat_result((10,))
            self.fail("No exception thrown")
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
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.check_stat_attributes(fname)

    def test_statvfs_attributes(self):
        if not hasattr(os, "statvfs"):
            return

        try:
            result = os.statvfs(self.fname)
        except OSError as e:
            # On AtheOS, glibc always returns ENOSYS
            if e.errno == errno.ENOSYS:
                return

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
            self.fail("No exception thrown")
        except AttributeError:
            pass

        try:
            result.parrot = 1
            self.fail("No exception thrown")
        except AttributeError:
            pass

        # Use the constructor with a too-short tuple.
        try:
            result2 = os.statvfs_result((10,))
            self.fail("No exception thrown")
        except TypeError:
            pass

        # Use the constructor with a too-long tuple.
        try:
            result2 = os.statvfs_result((0,1,2,3,4,5,6,7,8,9,10,11,12,13,14))
        except TypeError:
            pass

    def test_utime_dir(self):
        delta = 1000000
        st = os.stat(support.TESTFN)
        # round to int, because some systems may support sub-second
        # time stamps in stat, but not in utime.
        os.utime(support.TESTFN, (st.st_atime, int(st.st_mtime-delta)))
        st2 = os.stat(support.TESTFN)
        self.assertEqual(st2.st_mtime, int(st.st_mtime-delta))

    def _test_utime(self, filename, attr, utime, delta):
        # Issue #13327 removed the requirement to pass None as the
        # second argument. Check that the previous methods of passing
        # a time tuple or None work in addition to no argument.
        st0 = os.stat(filename)
        # Doesn't set anything new, but sets the time tuple way
        utime(filename, (attr(st0, "st_atime"), attr(st0, "st_mtime")))
        # Setting the time to the time you just read, then reading again,
        # should always return exactly the same times.
        st1 = os.stat(filename)
        self.assertEqual(attr(st0, "st_mtime"), attr(st1, "st_mtime"))
        self.assertEqual(attr(st0, "st_atime"), attr(st1, "st_atime"))
        # Set to the current time in the old explicit way.
        os.utime(filename, None)
        st2 = os.stat(support.TESTFN)
        # Set to the current time in the new way
        os.utime(filename)
        st3 = os.stat(filename)
        self.assertAlmostEqual(attr(st2, "st_mtime"), attr(st3, "st_mtime"), delta=delta)

    def test_utime(self):
        def utime(file, times):
            return os.utime(file, times)
        self._test_utime(self.fname, getattr, utime, 10)
        self._test_utime(support.TESTFN, getattr, utime, 10)


    def _test_utime_ns(self, set_times_ns, test_dir=True):
        def getattr_ns(o, attr):
            return getattr(o, attr + "_ns")
        ten_s = 10 * 1000 * 1000 * 1000
        self._test_utime(self.fname, getattr_ns, set_times_ns, ten_s)
        if test_dir:
            self._test_utime(support.TESTFN, getattr_ns, set_times_ns, ten_s)

    def test_utime_ns(self):
        def utime_ns(file, times):
            return os.utime(file, ns=times)
        self._test_utime_ns(utime_ns)

    requires_utime_dir_fd = unittest.skipUnless(
                                os.utime in os.supports_dir_fd,
                                "dir_fd support for utime required for this test.")
    requires_utime_fd = unittest.skipUnless(
                                os.utime in os.supports_fd,
                                "fd support for utime required for this test.")
    requires_utime_nofollow_symlinks = unittest.skipUnless(
                                os.utime in os.supports_follow_symlinks,
                                "follow_symlinks support for utime required for this test.")

    @requires_utime_nofollow_symlinks
    def test_lutimes_ns(self):
        def lutimes_ns(file, times):
            return os.utime(file, ns=times, follow_symlinks=False)
        self._test_utime_ns(lutimes_ns)

    @requires_utime_fd
    def test_futimes_ns(self):
        def futimes_ns(file, times):
            with open(file, "wb") as f:
                os.utime(f.fileno(), ns=times)
        self._test_utime_ns(futimes_ns, test_dir=False)

    def _utime_invalid_arguments(self, name, arg):
        with self.assertRaises(ValueError):
            getattr(os, name)(arg, (5, 5), ns=(5, 5))

    def test_utime_invalid_arguments(self):
        self._utime_invalid_arguments('utime', self.fname)


    @unittest.skipUnless(stat_supports_subsecond,
                         "os.stat() doesn't has a subsecond resolution")
    def _test_utime_subsecond(self, set_time_func):
        asec, amsec = 1, 901
        atime = asec + amsec * 1e-3
        msec, mmsec = 2, 901
        mtime = msec + mmsec * 1e-3
        filename = self.fname
        os.utime(filename, (0, 0))
        set_time_func(filename, atime, mtime)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            os.stat_float_times(True)
        st = os.stat(filename)
        self.assertAlmostEqual(st.st_atime, atime, places=3)
        self.assertAlmostEqual(st.st_mtime, mtime, places=3)

    def test_utime_subsecond(self):
        def set_time(filename, atime, mtime):
            os.utime(filename, (atime, mtime))
        self._test_utime_subsecond(set_time)

    @requires_utime_fd
    def test_futimes_subsecond(self):
        def set_time(filename, atime, mtime):
            with open(filename, "wb") as f:
                os.utime(f.fileno(), times=(atime, mtime))
        self._test_utime_subsecond(set_time)

    @requires_utime_fd
    def test_futimens_subsecond(self):
        def set_time(filename, atime, mtime):
            with open(filename, "wb") as f:
                os.utime(f.fileno(), times=(atime, mtime))
        self._test_utime_subsecond(set_time)

    @requires_utime_dir_fd
    def test_futimesat_subsecond(self):
        def set_time(filename, atime, mtime):
            dirname = os.path.dirname(filename)
            dirfd = os.open(dirname, os.O_RDONLY)
            try:
                os.utime(os.path.basename(filename), dir_fd=dirfd,
                             times=(atime, mtime))
            finally:
                os.close(dirfd)
        self._test_utime_subsecond(set_time)

    @requires_utime_nofollow_symlinks
    def test_lutimes_subsecond(self):
        def set_time(filename, atime, mtime):
            os.utime(filename, (atime, mtime), follow_symlinks=False)
        self._test_utime_subsecond(set_time)

    @requires_utime_dir_fd
    def test_utimensat_subsecond(self):
        def set_time(filename, atime, mtime):
            dirname = os.path.dirname(filename)
            dirfd = os.open(dirname, os.O_RDONLY)
            try:
                os.utime(os.path.basename(filename), dir_fd=dirfd,
                             times=(atime, mtime))
            finally:
                os.close(dirfd)
        self._test_utime_subsecond(set_time)

    # Restrict test to Win32, since there is no guarantee other
    # systems support centiseconds
    if sys.platform == 'win32':
        def get_file_system(path):
            root = os.path.splitdrive(os.path.abspath(path))[0] + '\\'
            import ctypes
            kernel32 = ctypes.windll.kernel32
            buf = ctypes.create_unicode_buffer("", 100)
            if kernel32.GetVolumeInformationW(root, None, 0, None, None, None, buf, len(buf)):
                return buf.value

        if get_file_system(support.TESTFN) == "NTFS":
            def test_1565150(self):
                t1 = 1159195039.25
                os.utime(self.fname, (t1, t1))
                self.assertEqual(os.stat(self.fname).st_mtime, t1)

            def test_large_time(self):
                t1 = 5000000000 # some day in 2128
                os.utime(self.fname, (t1, t1))
                self.assertEqual(os.stat(self.fname).st_mtime, t1)

        def test_1686475(self):
            # Verify that an open file can be stat'ed
            try:
                os.stat(r"c:\pagefile.sys")
            except WindowsError as e:
                if e.errno == 2: # file does not exist; cannot run test
                    return
                self.fail("Could not stat pagefile.sys")

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
    def test_update2(self):
        os.environ.clear()
        if os.path.exists("/bin/sh"):
            os.environ.update(HELLO="World")
            with os.popen("/bin/sh -c 'echo $HELLO'") as popen:
                value = popen.read().strip()
                self.assertEqual(value, "World")

    def test_os_popen_iter(self):
        if os.path.exists("/bin/sh"):
            with os.popen(
                "/bin/sh -c 'echo \"line1\nline2\nline3\"'") as popen:
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

class WalkTests(unittest.TestCase):
    """Tests for os.walk()."""

    def setUp(self):
        import os
        from os.path import join

        # Build:
        #     TESTFN/
        #       TEST1/              a file kid and two directory kids
        #         tmp1
        #         SUB1/             a file kid and a directory kid
        #           tmp2
        #           SUB11/          no kids
        #         SUB2/             a file kid and a dirsymlink kid
        #           tmp3
        #           link/           a symlink to TESTFN.2
        #           broken_link
        #       TEST2/
        #         tmp4              a lone file
        walk_path = join(support.TESTFN, "TEST1")
        sub1_path = join(walk_path, "SUB1")
        sub11_path = join(sub1_path, "SUB11")
        sub2_path = join(walk_path, "SUB2")
        tmp1_path = join(walk_path, "tmp1")
        tmp2_path = join(sub1_path, "tmp2")
        tmp3_path = join(sub2_path, "tmp3")
        link_path = join(sub2_path, "link")
        t2_path = join(support.TESTFN, "TEST2")
        tmp4_path = join(support.TESTFN, "TEST2", "tmp4")
        link_path = join(sub2_path, "link")
        broken_link_path = join(sub2_path, "broken_link")

        # Create stuff.
        os.makedirs(sub11_path)
        os.makedirs(sub2_path)
        os.makedirs(t2_path)
        for path in tmp1_path, tmp2_path, tmp3_path, tmp4_path:
            f = open(path, "w")
            f.write("I'm " + path + " and proud of it.  Blame test_os.\n")
            f.close()
        if support.can_symlink():
            if os.name == 'nt':
                def symlink_to_dir(src, dest):
                    os.symlink(src, dest, True)
            else:
                symlink_to_dir = os.symlink
            symlink_to_dir(os.path.abspath(t2_path), link_path)
            symlink_to_dir('broken', broken_link_path)
            sub2_tree = (sub2_path, ["link"], ["broken_link", "tmp3"])
        else:
            sub2_tree = (sub2_path, [], ["tmp3"])

        # Walk top-down.
        all = list(os.walk(walk_path))
        self.assertEqual(len(all), 4)
        # We can't know which order SUB1 and SUB2 will appear in.
        # Not flipped:  TESTFN, SUB1, SUB11, SUB2
        #     flipped:  TESTFN, SUB2, SUB1, SUB11
        flipped = all[0][1][0] != "SUB1"
        all[0][1].sort()
        all[3 - 2 * flipped][-1].sort()
        self.assertEqual(all[0], (walk_path, ["SUB1", "SUB2"], ["tmp1"]))
        self.assertEqual(all[1 + flipped], (sub1_path, ["SUB11"], ["tmp2"]))
        self.assertEqual(all[2 + flipped], (sub11_path, [], []))
        self.assertEqual(all[3 - 2 * flipped], sub2_tree)

        # Prune the search.
        all = []
        for root, dirs, files in os.walk(walk_path):
            all.append((root, dirs, files))
            # Don't descend into SUB1.
            if 'SUB1' in dirs:
                # Note that this also mutates the dirs we appended to all!
                dirs.remove('SUB1')
        self.assertEqual(len(all), 2)
        self.assertEqual(all[0], (walk_path, ["SUB2"], ["tmp1"]))
        all[1][-1].sort()
        self.assertEqual(all[1], sub2_tree)

        # Walk bottom-up.
        all = list(os.walk(walk_path, topdown=False))
        self.assertEqual(len(all), 4)
        # We can't know which order SUB1 and SUB2 will appear in.
        # Not flipped:  SUB11, SUB1, SUB2, TESTFN
        #     flipped:  SUB2, SUB11, SUB1, TESTFN
        flipped = all[3][1][0] != "SUB1"
        all[3][1].sort()
        all[2 - 2 * flipped][-1].sort()
        self.assertEqual(all[3], (walk_path, ["SUB1", "SUB2"], ["tmp1"]))
        self.assertEqual(all[flipped], (sub11_path, [], []))
        self.assertEqual(all[flipped + 1], (sub1_path, ["SUB11"], ["tmp2"]))
        self.assertEqual(all[2 - 2 * flipped], sub2_tree)

        if support.can_symlink():
            # Walk, following symlinks.
            for root, dirs, files in os.walk(walk_path, followlinks=True):
                if root == link_path:
                    self.assertEqual(dirs, [])
                    self.assertEqual(files, ["tmp4"])
                    break
            else:
                self.fail("Didn't follow symlink with followlinks=True")

    def tearDown(self):
        # Tear everything down.  This is a decent use for bottom-up on
        # Windows, which doesn't have a recursive delete command.  The
        # (not so) subtlety is that rmdir will fail unless the dir's
        # kids are removed first, so bottom up is essential.
        for root, dirs, files in os.walk(support.TESTFN, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                dirname = os.path.join(root, name)
                if not os.path.islink(dirname):
                    os.rmdir(dirname)
                else:
                    os.remove(dirname)
        os.rmdir(support.TESTFN)


@unittest.skipUnless(hasattr(os, 'fwalk'), "Test needs os.fwalk()")
class FwalkTests(WalkTests):
    """Tests for os.fwalk()."""

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

    def tearDown(self):
        # cleanup
        for root, dirs, files, rootfd in os.fwalk(support.TESTFN, topdown=False):
            for name in files:
                os.unlink(name, dir_fd=rootfd)
            for name in dirs:
                st = os.stat(name, dir_fd=rootfd, follow_symlinks=False)
                if stat.S_ISDIR(st.st_mode):
                    os.rmdir(name, dir_fd=rootfd)
                else:
                    os.unlink(name, dir_fd=rootfd)
        os.rmdir(support.TESTFN)


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
        self.assertRaises(OSError, os.makedirs, path, 0o776, exist_ok=True)
        os.makedirs(path, mode=mode, exist_ok=True)
        os.umask(old_mask)

    def test_exist_ok_s_isgid_directory(self):
        path = os.path.join(support.TESTFN, 'dir1')
        S_ISGID = stat.S_ISGID
        mode = 0o777
        old_mask = os.umask(0o022)
        try:
            existing_testfn_mode = stat.S_IMODE(
                    os.lstat(support.TESTFN).st_mode)
            os.chmod(support.TESTFN, existing_testfn_mode | S_ISGID)
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
            with self.assertRaises(OSError):
                # Should fail when the bit is not already set when demanded.
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

class DevNullTests(unittest.TestCase):
    def test_devnull(self):
        with open(os.devnull, 'wb') as f:
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

    def test_execvpe_with_bad_arglist(self):
        self.assertRaises(ValueError, os.execvpe, 'notepad', [], None)

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


class Win32ErrorTests(unittest.TestCase):
    def test_rename(self):
        self.assertRaises(WindowsError, os.rename, support.TESTFN, support.TESTFN+".bak")

    def test_remove(self):
        self.assertRaises(WindowsError, os.remove, support.TESTFN)

    def test_chdir(self):
        self.assertRaises(WindowsError, os.chdir, support.TESTFN)

    def test_mkdir(self):
        f = open(support.TESTFN, "w")
        try:
            self.assertRaises(WindowsError, os.mkdir, support.TESTFN)
        finally:
            f.close()
            os.unlink(support.TESTFN)

    def test_utime(self):
        self.assertRaises(WindowsError, os.utime, support.TESTFN, None)

    def test_chmod(self):
        self.assertRaises(WindowsError, os.chmod, support.TESTFN, 0)

class TestInvalidFD(unittest.TestCase):
    singles = ["fchdir", "dup", "fdopen", "fdatasync", "fstat",
               "fstatvfs", "fsync", "tcgetpgrp", "ttyname"]
    #singles.append("close")
    #We omit close because it doesn'r raise an exception on some platforms
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
            self.fail("%r didn't raise a OSError with a bad file descriptor"
                      % f)

    def test_isatty(self):
        if hasattr(os, "isatty"):
            self.assertEqual(os.isatty(support.make_bad_fd()), False)

    def test_closerange(self):
        if hasattr(os, "closerange"):
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

    def test_dup2(self):
        if hasattr(os, "dup2"):
            self.check(os.dup2, 20)

    def test_fchmod(self):
        if hasattr(os, "fchmod"):
            self.check(os.fchmod, 0)

    def test_fchown(self):
        if hasattr(os, "fchown"):
            self.check(os.fchown, -1, -1)

    def test_fpathconf(self):
        if hasattr(os, "fpathconf"):
            self.check(os.pathconf, "PC_NAME_MAX")
            self.check(os.fpathconf, "PC_NAME_MAX")

    def test_ftruncate(self):
        if hasattr(os, "ftruncate"):
            self.check(os.truncate, 0)
            self.check(os.ftruncate, 0)

    def test_lseek(self):
        if hasattr(os, "lseek"):
            self.check(os.lseek, 0, 0)

    def test_read(self):
        if hasattr(os, "read"):
            self.check(os.read, 1)

    def test_tcsetpgrpt(self):
        if hasattr(os, "tcsetpgrp"):
            self.check(os.tcsetpgrp, 0)

    def test_write(self):
        if hasattr(os, "write"):
            self.check(os.write, b" ")


class LinkTests(unittest.TestCase):
    def setUp(self):
        self.file1 = support.TESTFN
        self.file2 = os.path.join(support.TESTFN + "2")

    def tearDown(self):
        for file in (self.file1, self.file2):
            if os.path.exists(file):
                os.unlink(file)

    def _test_link(self, file1, file2):
        with open(file1, "w") as f1:
            f1.write("test")

        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
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

if sys.platform != 'win32':
    class Win32ErrorTests(unittest.TestCase):
        pass

    class PosixUidGidTests(unittest.TestCase):
        if hasattr(os, 'setuid'):
            def test_setuid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.setuid, 0)
                self.assertRaises(OverflowError, os.setuid, 1<<32)

        if hasattr(os, 'setgid'):
            def test_setgid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.setgid, 0)
                self.assertRaises(OverflowError, os.setgid, 1<<32)

        if hasattr(os, 'seteuid'):
            def test_seteuid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.seteuid, 0)
                self.assertRaises(OverflowError, os.seteuid, 1<<32)

        if hasattr(os, 'setegid'):
            def test_setegid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.setegid, 0)
                self.assertRaises(OverflowError, os.setegid, 1<<32)

        if hasattr(os, 'setreuid'):
            def test_setreuid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.setreuid, 0, 0)
                self.assertRaises(OverflowError, os.setreuid, 1<<32, 0)
                self.assertRaises(OverflowError, os.setreuid, 0, 1<<32)

            def test_setreuid_neg1(self):
                # Needs to accept -1.  We run this in a subprocess to avoid
                # altering the test runner's process state (issue8045).
                subprocess.check_call([
                        sys.executable, '-c',
                        'import os,sys;os.setreuid(-1,-1);sys.exit(0)'])

        if hasattr(os, 'setregid'):
            def test_setregid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.setregid, 0, 0)
                self.assertRaises(OverflowError, os.setregid, 1<<32, 0)
                self.assertRaises(OverflowError, os.setregid, 0, 1<<32)

            def test_setregid_neg1(self):
                # Needs to accept -1.  We run this in a subprocess to avoid
                # altering the test runner's process state (issue8045).
                subprocess.check_call([
                        sys.executable, '-c',
                        'import os,sys;os.setregid(-1,-1);sys.exit(0)'])

    class Pep383Tests(unittest.TestCase):
        def setUp(self):
            if support.TESTFN_UNENCODABLE:
                self.dir = support.TESTFN_UNENCODABLE
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

        def test_stat(self):
            for fn in self.unicodefn:
                os.stat(os.path.join(self.dir, fn))
else:
    class PosixUidGidTests(unittest.TestCase):
        pass
    class Pep383Tests(unittest.TestCase):
        pass

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

    @unittest.skip("subprocesses aren't inheriting CTRL+C property")
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
        # handle CTRL+C, rather than ignore it. This property is inherited
        # by subprocesses.
        SetConsoleCtrlHandler(NULL, 0)

        self._kill_with_event(signal.CTRL_C_EVENT, "CTRL_C_EVENT")

    def test_CTRL_BREAK_EVENT(self):
        self._kill_with_event(signal.CTRL_BREAK_EVENT, "CTRL_BREAK_EVENT")


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
        os.symlink(self.dirlink_target, self.dirlink, True)
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
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            self.assertEqual(os.stat(bytes_link), os.stat(target))
            self.assertNotEqual(os.lstat(bytes_link), os.stat(bytes_link))

    def test_12084(self):
        level1 = os.path.abspath(support.TESTFN)
        level2 = os.path.join(level1, "level2")
        level3 = os.path.join(level2, "level3")
        try:
            os.mkdir(level1)
            os.mkdir(level2)
            os.mkdir(level3)

            file1 = os.path.abspath(os.path.join(level1, "file1"))

            with open(file1, "w") as f:
                f.write("file1")

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
        except OSError as err:
            self.fail(err)
        finally:
            os.remove(file1)
            shutil.rmtree(level1)


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
                raise unittest.SkipTest(
      "unable to reliably test setpriority at current nice level of %s" % base)
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

    @classmethod
    def setUpClass(cls):
        with open(support.TESTFN, "wb") as f:
            f.write(cls.DATA)

    @classmethod
    def tearDownClass(cls):
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

    # --- headers / trailers tests

    if SUPPORT_HEADERS_TRAILERS:

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

        def test_trailers(self):
            TESTFN2 = support.TESTFN + "2"
            with open(TESTFN2, 'wb') as f:
                f.write(b"abcde")
            with open(TESTFN2, 'rb')as f:
                self.addCleanup(os.remove, TESTFN2)
                os.sendfile(self.sockno, f.fileno(), 0, 4096,
                            trailers=[b"12345"])
                self.client.close()
                self.server.wait()
                data = self.server.handler_instance.get_data()
                self.assertEqual(data, b"abcde12345")

        if hasattr(os, "SF_NODISKIO"):
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
        with open(support.TESTFN, "wb") as fp:
            try:
                os.setxattr(fp.fileno(), b"user.test", b"")
            except OSError:
                return False
    finally:
        support.unlink(support.TESTFN)
    # Kernels < 2.6.39 don't respect setxattr flags.
    kernel_version = platform.release()
    m = re.match("2.6.(\d{1,2})", kernel_version)
    return m is None or int(m.group(1)) >= 39


@unittest.skipUnless(supports_extended_attributes(),
                     "no non-broken extended attribute support")
class ExtendedAttributeTests(unittest.TestCase):

    def tearDown(self):
        support.unlink(support.TESTFN)

    def _check_xattrs_str(self, s, getxattr, setxattr, removexattr, listxattr, **kwargs):
        fn = support.TESTFN
        open(fn, "wb").close()
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
        def make_bytes(s):
            return bytes(s, "ascii")
        self._check_xattrs_str(str, *args, **kwargs)
        support.unlink(support.TESTFN)
        self._check_xattrs_str(make_bytes, *args, **kwargs)

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
            with open(path, "wb") as fp:
                os.setxattr(fp.fileno(), *args)
        def removexattr(path, *args):
            with open(path, "wb") as fp:
                os.removexattr(fp.fileno(), *args)
        def listxattr(path, *args):
            with open(path, "rb") as fp:
                return os.listxattr(fp.fileno(), *args)
        self._check_xattrs(getxattr, setxattr, removexattr, listxattr)


@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32DeprecatedBytesAPI(unittest.TestCase):
    def test_deprecated(self):
        import nt
        filename = os.fsencode(support.TESTFN)
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            for func, *args in (
                (nt._getfullpathname, filename),
                (nt._isdir, filename),
                (os.access, filename, os.R_OK),
                (os.chdir, filename),
                (os.chmod, filename, 0o777),
                (os.getcwdb,),
                (os.link, filename, filename),
                (os.listdir, filename),
                (os.lstat, filename),
                (os.mkdir, filename),
                (os.open, filename, os.O_RDONLY),
                (os.rename, filename, filename),
                (os.rmdir, filename),
                (os.startfile, filename),
                (os.stat, filename),
                (os.unlink, filename),
                (os.utime, filename),
            ):
                self.assertRaises(DeprecationWarning, func, *args)

    @support.skip_unless_symlink
    def test_symlink(self):
        filename = os.fsencode(support.TESTFN)
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            self.assertRaises(DeprecationWarning,
                              os.symlink, filename, filename)


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


@support.reap_threads
def test_main():
    support.run_unittest(
        FileTests,
        StatAttributeTests,
        EnvironTests,
        WalkTests,
        FwalkTests,
        MakedirTests,
        DevNullTests,
        URandomTests,
        ExecTests,
        Win32ErrorTests,
        TestInvalidFD,
        PosixUidGidTests,
        Pep383Tests,
        Win32KillTests,
        Win32SymlinkTests,
        FSEncodingTests,
        DeviceEncodingTests,
        PidTests,
        LoginTests,
        LinkTests,
        TestSendfile,
        ProgramPriorityTests,
        ExtendedAttributeTests,
        Win32DeprecatedBytesAPI,
        TermsizeTests,
    )

if __name__ == "__main__":
    test_main()
