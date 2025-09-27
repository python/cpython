"""
Test random bytes: getrandom(), urandom(), etc.
"""

import errno
import os
import sys
import sysconfig
import unittest
from test.support import os_helper
from test.support.script_helper import assert_python_ok
from .utils import create_file

try:
    import resource
except ImportError:
    resource = None


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


if __name__ == "__main__":
    unittest.main()
