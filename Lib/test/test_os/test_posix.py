"Test posix functions"

from test import support
from test.support import os_helper
from test.support import warnings_helper
from test.support.script_helper import assert_python_ok

import errno
import os
import signal
import stat
import sys
import unittest
import warnings
from .utils import create_file

try:
    import posix
except ImportError:
    import nt as posix

try:
    import pwd
except ImportError:
    pwd = None


class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        create_file(os_helper.TESTFN, b'')
        self.enterContext(warnings_helper.check_warnings())
        warnings.filterwarnings('ignore', '.* potential security risk .*',
                                RuntimeWarning)

    def testNoArgFunctions(self):
        # test posix functions which take no arguments and have
        # no side-effects which we need to cleanup (e.g., fork, wait, abort)
        NO_ARG_FUNCTIONS = [ "ctermid", "getcwd", "getcwdb", "uname",
                             "times", "getloadavg",
                             "getegid", "geteuid", "getgid", "getgroups",
                             "getpid", "getpgrp", "getppid", "getuid", "sync",
                           ]

        for name in NO_ARG_FUNCTIONS:
            posix_func = getattr(posix, name, None)
            if posix_func is not None:
                with self.subTest(name):
                    posix_func()
                    self.assertRaises(TypeError, posix_func, 1)

    @unittest.skipUnless(hasattr(posix, 'statvfs'),
                         'test needs posix.statvfs()')
    def test_statvfs(self):
        self.assertTrue(posix.statvfs(os.curdir))

    @unittest.skipUnless(hasattr(posix, 'fstatvfs'),
                         'test needs posix.fstatvfs()')
    def test_fstatvfs(self):
        fp = open(os_helper.TESTFN)
        try:
            self.assertTrue(posix.fstatvfs(fp.fileno()))
            self.assertTrue(posix.statvfs(fp.fileno()))
        finally:
            fp.close()

    @unittest.skipUnless(hasattr(posix, 'confstr'),
                         'test needs posix.confstr()')
    def test_confstr(self):
        with self.assertRaisesRegex(
            ValueError, "unrecognized configuration name"
        ):
            posix.confstr("CS_garbage")

        with self.assertRaisesRegex(
            TypeError, "configuration names must be strings or integers"
        ):
            posix.confstr(1.23)

        path = posix.confstr("CS_PATH")
        self.assertGreater(len(path), 0)
        self.assertEqual(posix.confstr(posix.confstr_names["CS_PATH"]), path)

    @unittest.skipUnless(hasattr(posix, 'sysconf'),
                         'test needs posix.sysconf()')
    def test_sysconf(self):
        with self.assertRaisesRegex(
            ValueError, "unrecognized configuration name"
        ):
            posix.sysconf("SC_garbage")

        with self.assertRaisesRegex(
            TypeError, "configuration names must be strings or integers"
        ):
            posix.sysconf(1.23)

        arg_max = posix.sysconf("SC_ARG_MAX")
        self.assertGreater(arg_max, 0)
        self.assertEqual(
            posix.sysconf(posix.sysconf_names["SC_ARG_MAX"]), arg_max)

    @unittest.skipUnless(hasattr(posix, 'mkfifo'), "don't have mkfifo()")
    def test_mkfifo(self):
        if sys.platform == "vxworks":
            fifo_path = os.path.join("/fifos/", os_helper.TESTFN)
        else:
            fifo_path = os_helper.TESTFN
        os_helper.unlink(fifo_path)
        self.addCleanup(os_helper.unlink, fifo_path)
        try:
            posix.mkfifo(fifo_path, stat.S_IRUSR | stat.S_IWUSR)
        except PermissionError as e:
            self.skipTest('posix.mkfifo(): %s' % e)
        self.assertTrue(stat.S_ISFIFO(posix.stat(fifo_path).st_mode))

    @unittest.skipUnless(hasattr(posix, 'mknod') and hasattr(stat, 'S_IFIFO'),
                         "don't have mknod()/S_IFIFO")
    def test_mknod(self):
        # Test using mknod() to create a FIFO (the only use specified
        # by POSIX).
        os_helper.unlink(os_helper.TESTFN)
        mode = stat.S_IFIFO | stat.S_IRUSR | stat.S_IWUSR
        try:
            posix.mknod(os_helper.TESTFN, mode, 0)
        except OSError as e:
            # Some old systems don't allow unprivileged users to use
            # mknod(), or only support creating device nodes.
            self.assertIn(e.errno, (errno.EPERM, errno.EINVAL, errno.EACCES))
        else:
            self.assertTrue(stat.S_ISFIFO(posix.stat(os_helper.TESTFN).st_mode))

        # Keyword arguments are also supported
        os_helper.unlink(os_helper.TESTFN)
        try:
            posix.mknod(path=os_helper.TESTFN, mode=mode, device=0,
                dir_fd=None)
        except OSError as e:
            self.assertIn(e.errno, (errno.EPERM, errno.EINVAL, errno.EACCES))

    @unittest.skipUnless(hasattr(posix, 'makedev'), 'test needs posix.makedev()')
    def test_makedev(self):
        st = posix.stat(os_helper.TESTFN)
        dev = st.st_dev
        self.assertIsInstance(dev, int)
        self.assertGreaterEqual(dev, 0)

        major = posix.major(dev)
        self.assertIsInstance(major, int)
        self.assertGreaterEqual(major, 0)
        self.assertEqual(posix.major(dev), major)
        self.assertRaises(TypeError, posix.major, float(dev))
        self.assertRaises(TypeError, posix.major)
        for x in -2, 2**64, -2**63-1:
            self.assertRaises((ValueError, OverflowError), posix.major, x)

        minor = posix.minor(dev)
        self.assertIsInstance(minor, int)
        self.assertGreaterEqual(minor, 0)
        self.assertEqual(posix.minor(dev), minor)
        self.assertRaises(TypeError, posix.minor, float(dev))
        self.assertRaises(TypeError, posix.minor)
        for x in -2, 2**64, -2**63-1:
            self.assertRaises((ValueError, OverflowError), posix.minor, x)

        self.assertEqual(posix.makedev(major, minor), dev)
        self.assertRaises(TypeError, posix.makedev, float(major), minor)
        self.assertRaises(TypeError, posix.makedev, major, float(minor))
        self.assertRaises(TypeError, posix.makedev, major)
        self.assertRaises(TypeError, posix.makedev)
        for x in -2, 2**32, 2**64, -2**63-1:
            self.assertRaises((ValueError, OverflowError), posix.makedev, x, minor)
            self.assertRaises((ValueError, OverflowError), posix.makedev, major, x)

        # The following tests are needed to test functions accepting or
        # returning the special value NODEV (if it is defined). major(), minor()
        # and makefile() are the only easily reproducible examples, but that
        # behavior is platform specific -- on some platforms their code has
        # a special case for NODEV, on others this is just an implementation
        # artifact.
        if (hasattr(posix, 'NODEV') and
            sys.platform.startswith(('linux', 'macos', 'freebsd', 'dragonfly',
                                     'sunos'))):
            NODEV = posix.NODEV
            self.assertEqual(posix.major(NODEV), NODEV)
            self.assertEqual(posix.minor(NODEV), NODEV)
            self.assertEqual(posix.makedev(NODEV, NODEV), NODEV)

    def test_nodev(self):
        # NODEV is not a part of Posix, but is defined on many systems.
        if (not hasattr(posix, 'NODEV')
            and (not sys.platform.startswith(('linux', 'macos', 'freebsd',
                                              'dragonfly', 'netbsd', 'openbsd',
                                              'sunos'))
                 or support.linked_to_musl())):
            self.skipTest('not defined on this platform')
        self.assertHasAttr(posix, 'NODEV')

    @unittest.skipUnless(hasattr(posix, 'strerror'),
                         'test needs posix.strerror()')
    def test_strerror(self):
        self.assertTrue(posix.strerror(0))

    @unittest.skipUnless(hasattr(signal, 'SIGCHLD'), 'CLD_XXXX be placed in si_code for a SIGCHLD signal')
    @unittest.skipUnless(hasattr(os, 'waitid_result'), "test needs os.waitid_result")
    def test_cld_xxxx_constants(self):
        os.CLD_EXITED
        os.CLD_KILLED
        os.CLD_DUMPED
        os.CLD_TRAPPED
        os.CLD_STOPPED
        os.CLD_CONTINUED

    @unittest.skipIf(support.is_wasi, "No dynamic linking on WASI")
    @unittest.skipUnless(os.name == 'posix', "POSIX-only test")
    def test_rtld_constants(self):
        # check presence of major RTLD_* constants
        posix.RTLD_LAZY
        posix.RTLD_NOW
        posix.RTLD_GLOBAL
        posix.RTLD_LOCAL

    def test_path_error2(self):
        """
        Test functions that call path_error2(), providing two filenames in their exceptions.
        """
        for name in ("rename", "replace", "link"):
            function = getattr(os, name, None)
            if function is None:
                continue

            for dst in ("noodly2", os_helper.TESTFN):
                try:
                    function('doesnotexistfilename', dst)
                except OSError as e:
                    self.assertIn("'doesnotexistfilename' -> '{}'".format(dst), str(e))
                    break
            else:
                self.fail("No valid path_error2() test for os." + name)

    def test_path_with_null_character(self):
        fn = os_helper.TESTFN
        fn_with_NUL = fn + '\0'
        self.addCleanup(os_helper.unlink, fn)
        os_helper.unlink(fn)
        fd = None
        try:
            with self.assertRaises(ValueError):
                fd = os.open(fn_with_NUL, os.O_WRONLY | os.O_CREAT) # raises
        finally:
            if fd is not None:
                os.close(fd)
        self.assertFalse(os.path.exists(fn))
        self.assertRaises(ValueError, os.mkdir, fn_with_NUL)
        self.assertFalse(os.path.exists(fn))
        open(fn, 'wb').close()
        self.assertRaises(ValueError, os.stat, fn_with_NUL)

    def test_path_with_null_byte(self):
        fn = os.fsencode(os_helper.TESTFN)
        fn_with_NUL = fn + b'\0'
        self.addCleanup(os_helper.unlink, fn)
        os_helper.unlink(fn)
        fd = None
        try:
            with self.assertRaises(ValueError):
                fd = os.open(fn_with_NUL, os.O_WRONLY | os.O_CREAT) # raises
        finally:
            if fd is not None:
                os.close(fd)
        self.assertFalse(os.path.exists(fn))
        self.assertRaises(ValueError, os.mkdir, fn_with_NUL)
        self.assertFalse(os.path.exists(fn))
        open(fn, 'wb').close()
        self.assertRaises(ValueError, os.stat, fn_with_NUL)

    @unittest.skipUnless(hasattr(os, "pidfd_open"), "pidfd_open unavailable")
    def test_pidfd_open(self):
        with self.assertRaises(OSError) as cm:
            os.pidfd_open(-1)
        if cm.exception.errno == errno.ENOSYS:
            self.skipTest("system does not support pidfd_open")
        if isinstance(cm.exception, PermissionError):
            self.skipTest(f"pidfd_open syscall blocked: {cm.exception!r}")
        self.assertEqual(cm.exception.errno, errno.EINVAL)
        os.close(os.pidfd_open(os.getpid(), 0))


class NamespacesTests(unittest.TestCase):
    """Tests for os.unshare() and os.setns()."""

    @unittest.skipUnless(hasattr(os, 'unshare'), 'needs os.unshare()')
    @unittest.skipUnless(hasattr(os, 'setns'), 'needs os.setns()')
    @unittest.skipUnless(os.path.exists('/proc/self/ns/uts'), 'need /proc/self/ns/uts')
    @support.requires_linux_version(3, 0, 0)
    def test_unshare_setns(self):
        code = """if 1:
            import errno
            import os
            import sys
            fd = os.open('/proc/self/ns/uts', os.O_RDONLY)
            try:
                original = os.readlink('/proc/self/ns/uts')
                try:
                    os.unshare(os.CLONE_NEWUTS)
                except OSError as e:
                    if e.errno == errno.ENOSPC:
                        # skip test if limit is exceeded
                        sys.exit()
                    raise
                new = os.readlink('/proc/self/ns/uts')
                if original == new:
                    raise Exception('os.unshare failed')
                os.setns(fd, os.CLONE_NEWUTS)
                restored = os.readlink('/proc/self/ns/uts')
                if original != restored:
                    raise Exception('os.setns failed')
            except PermissionError:
                # The calling process did not have the required privileges
                # for this operation
                pass
            except OSError as e:
                # Skip the test on these errors:
                # - ENOSYS: syscall not available
                # - EINVAL: kernel was not configured with the CONFIG_UTS_NS option
                # - ENOMEM: not enough memory
                if e.errno not in (errno.ENOSYS, errno.EINVAL, errno.ENOMEM):
                    raise
            finally:
                os.close(fd)
            """

        assert_python_ok("-c", code)


def tearDownModule():
    support.reap_children()


if __name__ == '__main__':
    unittest.main()
