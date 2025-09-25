"""
Test user functions: getuid(), setgid(), getgroups(), getlogin(), etc.
"""

import errno
import os
import posix
import pwd
import subprocess
import sys
import unittest
from test import support

# Issue #14110: Some tests fail on FreeBSD if the user is in the wheel group.
HAVE_WHEEL_GROUP = (sys.platform.startswith('freebsd') and os.getgid() == 0)


class PosixTester(unittest.TestCase):
    @unittest.skipUnless(hasattr(posix, 'getresuid'),
                         'test needs posix.getresuid()')
    def test_getresuid(self):
        user_ids = posix.getresuid()
        self.assertEqual(len(user_ids), 3)
        for val in user_ids:
            self.assertGreaterEqual(val, 0)

    @unittest.skipUnless(hasattr(posix, 'getresgid'),
                         'test needs posix.getresgid()')
    def test_getresgid(self):
        group_ids = posix.getresgid()
        self.assertEqual(len(group_ids), 3)
        for val in group_ids:
            self.assertGreaterEqual(val, 0)

    @unittest.skipUnless(hasattr(posix, 'setresuid'),
                         'test needs posix.setresuid()')
    def test_setresuid(self):
        current_user_ids = posix.getresuid()
        self.assertIsNone(posix.setresuid(*current_user_ids))
        # -1 means don't change that value.
        self.assertIsNone(posix.setresuid(-1, -1, -1))

    @unittest.skipUnless(hasattr(posix, 'setresuid'),
                         'test needs posix.setresuid()')
    def test_setresuid_exception(self):
        # Don't do this test if someone is silly enough to run us as root.
        current_user_ids = posix.getresuid()
        if 0 not in current_user_ids:
            new_user_ids = (current_user_ids[0]+1, -1, -1)
            self.assertRaises(OSError, posix.setresuid, *new_user_ids)

    @unittest.skipUnless(hasattr(posix, 'setresgid'),
                         'test needs posix.setresgid()')
    def test_setresgid(self):
        current_group_ids = posix.getresgid()
        self.assertIsNone(posix.setresgid(*current_group_ids))
        # -1 means don't change that value.
        self.assertIsNone(posix.setresgid(-1, -1, -1))

    @unittest.skipUnless(hasattr(posix, 'setresgid'),
                         'test needs posix.setresgid()')
    def test_setresgid_exception(self):
        # Don't do this test if someone is silly enough to run us as root.
        current_group_ids = posix.getresgid()
        if 0 not in current_group_ids:
            new_group_ids = (current_group_ids[0]+1, -1, -1)
            self.assertRaises(OSError, posix.setresgid, *new_group_ids)

    @unittest.skipUnless(hasattr(posix, 'initgroups'),
                         "test needs os.initgroups()")
    @unittest.skipUnless(hasattr(pwd, 'getpwuid'), "test needs pwd.getpwuid()")
    def test_initgroups(self):
        # It takes a string and an integer; check that it raises a TypeError
        # for other argument lists.
        self.assertRaises(TypeError, posix.initgroups)
        self.assertRaises(TypeError, posix.initgroups, None)
        self.assertRaises(TypeError, posix.initgroups, 3, "foo")
        self.assertRaises(TypeError, posix.initgroups, "foo", 3, object())

        # If a non-privileged user invokes it, it should fail with OSError
        # EPERM.
        if os.getuid() != 0:
            try:
                name = pwd.getpwuid(posix.getuid()).pw_name
            except KeyError:
                # the current UID may not have a pwd entry
                raise unittest.SkipTest("need a pwd entry")
            try:
                posix.initgroups(name, 13)
            except OSError as e:
                self.assertEqual(e.errno, errno.EPERM)
            else:
                self.fail("Expected OSError to be raised by initgroups")

    @unittest.skipUnless(hasattr(posix, 'getgrouplist'), "test needs posix.getgrouplist()")
    @unittest.skipUnless(hasattr(pwd, 'getpwuid'), "test needs pwd.getpwuid()")
    @unittest.skipUnless(hasattr(os, 'getuid'), "test needs os.getuid()")
    def test_getgrouplist(self):
        user = pwd.getpwuid(os.getuid())[0]
        group = pwd.getpwuid(os.getuid())[3]
        self.assertIn(group, posix.getgrouplist(user, group))

    @unittest.skipUnless(hasattr(os, 'getegid'), "test needs os.getegid()")
    @unittest.skipUnless(hasattr(os, 'popen'), "test needs os.popen()")
    @support.requires_subprocess()
    def test_getgroups(self):
        with os.popen('id -G 2>/dev/null') as idg:
            groups = idg.read().strip()
            ret = idg.close()

        try:
            idg_groups = set(int(g) for g in groups.split())
        except ValueError:
            idg_groups = set()
        if ret is not None or not idg_groups:
            raise unittest.SkipTest("need working 'id -G'")

        # Issues 16698: OS X ABIs prior to 10.6 have limits on getgroups()
        if sys.platform == 'darwin':
            import sysconfig
            dt = sysconfig.get_config_var('MACOSX_DEPLOYMENT_TARGET') or '10.3'
            if tuple(int(n) for n in dt.split('.')[0:2]) < (10, 6):
                raise unittest.SkipTest("getgroups(2) is broken prior to 10.6")

        # 'id -G' and 'os.getgroups()' should return the same
        # groups, ignoring order, duplicates, and the effective gid.
        # #10822/#26944 - It is implementation defined whether
        # posix.getgroups() includes the effective gid.
        symdiff = idg_groups.symmetric_difference(posix.getgroups())
        self.assertTrue(not symdiff or symdiff == {posix.getegid()})


class PosixGroupsTester(unittest.TestCase):

    def setUp(self):
        if posix.getuid() != 0:
            raise unittest.SkipTest("not enough privileges")
        if not hasattr(posix, 'getgroups'):
            raise unittest.SkipTest("need posix.getgroups")
        if sys.platform == 'darwin':
            raise unittest.SkipTest("getgroups(2) is broken on OSX")
        self.saved_groups = posix.getgroups()

    def tearDown(self):
        if hasattr(posix, 'setgroups'):
            posix.setgroups(self.saved_groups)
        elif hasattr(posix, 'initgroups'):
            name = pwd.getpwuid(posix.getuid()).pw_name
            posix.initgroups(name, self.saved_groups[0])

    @unittest.skipUnless(hasattr(posix, 'initgroups'),
                         "test needs posix.initgroups()")
    def test_initgroups(self):
        # find missing group

        g = max(self.saved_groups or [0]) + 1
        name = pwd.getpwuid(posix.getuid()).pw_name
        posix.initgroups(name, g)
        self.assertIn(g, posix.getgroups())

    @unittest.skipUnless(hasattr(posix, 'setgroups'),
                         "test needs posix.setgroups()")
    def test_setgroups(self):
        for groups in [[0], list(range(16))]:
            posix.setgroups(groups)
            self.assertListEqual(groups, posix.getgroups())


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


# The introduction of this TestCase caused at least two different errors on
# *nix buildbots. Temporarily skip this to let the buildbots move along.
@unittest.skip("Skip due to platform/environment differences on *NIX buildbots")
@unittest.skipUnless(hasattr(os, 'getlogin'), "test needs os.getlogin")
class LoginTests(unittest.TestCase):
    def test_getlogin(self):
        user_name = os.getlogin()
        self.assertNotEqual(len(user_name), 0)


if __name__ == '__main__':
    unittest.main()
