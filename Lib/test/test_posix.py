"Test posix functions"

from test import test_support

# Skip these tests if there is no posix module.
posix = test_support.import_module('posix')

import errno
import sys
import time
import os
import pwd
import shutil
import sys
import unittest
import warnings


warnings.filterwarnings('ignore', '.* potential security risk .*',
                        RuntimeWarning)

class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        fp = open(test_support.TESTFN, 'w+')
        fp.close()

    def tearDown(self):
        os.unlink(test_support.TESTFN)

    def testNoArgFunctions(self):
        # test posix functions which take no arguments and have
        # no side-effects which we need to cleanup (e.g., fork, wait, abort)
        NO_ARG_FUNCTIONS = [ "ctermid", "getcwd", "getcwdu", "uname",
                             "times", "getloadavg", "tmpnam",
                             "getegid", "geteuid", "getgid", "getgroups",
                             "getpid", "getpgrp", "getppid", "getuid",
                           ]

        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", "", DeprecationWarning)
            for name in NO_ARG_FUNCTIONS:
                posix_func = getattr(posix, name, None)
                if posix_func is not None:
                    posix_func()
                    self.assertRaises(TypeError, posix_func, 1)

    if hasattr(posix, 'getresuid'):
        def test_getresuid(self):
            user_ids = posix.getresuid()
            self.assertEqual(len(user_ids), 3)
            for val in user_ids:
                self.assertGreaterEqual(val, 0)

    if hasattr(posix, 'getresgid'):
        def test_getresgid(self):
            group_ids = posix.getresgid()
            self.assertEqual(len(group_ids), 3)
            for val in group_ids:
                self.assertGreaterEqual(val, 0)

    if hasattr(posix, 'setresuid'):
        def test_setresuid(self):
            current_user_ids = posix.getresuid()
            self.assertIsNone(posix.setresuid(*current_user_ids))
            # -1 means don't change that value.
            self.assertIsNone(posix.setresuid(-1, -1, -1))

        def test_setresuid_exception(self):
            # Don't do this test if someone is silly enough to run us as root.
            current_user_ids = posix.getresuid()
            if 0 not in current_user_ids:
                new_user_ids = (current_user_ids[0]+1, -1, -1)
                self.assertRaises(OSError, posix.setresuid, *new_user_ids)

    if hasattr(posix, 'setresgid'):
        def test_setresgid(self):
            current_group_ids = posix.getresgid()
            self.assertIsNone(posix.setresgid(*current_group_ids))
            # -1 means don't change that value.
            self.assertIsNone(posix.setresgid(-1, -1, -1))

        def test_setresgid_exception(self):
            # Don't do this test if someone is silly enough to run us as root.
            current_group_ids = posix.getresgid()
            if 0 not in current_group_ids:
                new_group_ids = (current_group_ids[0]+1, -1, -1)
                self.assertRaises(OSError, posix.setresgid, *new_group_ids)

    @unittest.skipUnless(hasattr(posix, 'initgroups'),
                         "test needs os.initgroups()")
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
            name = pwd.getpwuid(posix.getuid()).pw_name
            try:
                posix.initgroups(name, 13)
            except OSError as e:
                self.assertEqual(e.errno, errno.EPERM)
            else:
                self.fail("Expected OSError to be raised by initgroups")

    def test_statvfs(self):
        if hasattr(posix, 'statvfs'):
            self.assertTrue(posix.statvfs(os.curdir))

    def test_fstatvfs(self):
        if hasattr(posix, 'fstatvfs'):
            fp = open(test_support.TESTFN)
            try:
                self.assertTrue(posix.fstatvfs(fp.fileno()))
            finally:
                fp.close()

    def test_ftruncate(self):
        if hasattr(posix, 'ftruncate'):
            fp = open(test_support.TESTFN, 'w+')
            try:
                # we need to have some data to truncate
                fp.write('test')
                fp.flush()
                posix.ftruncate(fp.fileno(), 0)
            finally:
                fp.close()

    def test_dup(self):
        if hasattr(posix, 'dup'):
            fp = open(test_support.TESTFN)
            try:
                fd = posix.dup(fp.fileno())
                self.assertIsInstance(fd, int)
                os.close(fd)
            finally:
                fp.close()

    def test_confstr(self):
        if hasattr(posix, 'confstr'):
            self.assertRaises(ValueError, posix.confstr, "CS_garbage")
            self.assertEqual(len(posix.confstr("CS_PATH")) > 0, True)

    def test_dup2(self):
        if hasattr(posix, 'dup2'):
            fp1 = open(test_support.TESTFN)
            fp2 = open(test_support.TESTFN)
            try:
                posix.dup2(fp1.fileno(), fp2.fileno())
            finally:
                fp1.close()
                fp2.close()

    def fdopen_helper(self, *args):
        fd = os.open(test_support.TESTFN, os.O_RDONLY)
        fp2 = posix.fdopen(fd, *args)
        fp2.close()

    def test_fdopen(self):
        if hasattr(posix, 'fdopen'):
            self.fdopen_helper()
            self.fdopen_helper('r')
            self.fdopen_helper('r', 100)

    def test_osexlock(self):
        if hasattr(posix, "O_EXLOCK"):
            fd = os.open(test_support.TESTFN,
                         os.O_WRONLY|os.O_EXLOCK|os.O_CREAT)
            self.assertRaises(OSError, os.open, test_support.TESTFN,
                              os.O_WRONLY|os.O_EXLOCK|os.O_NONBLOCK)
            os.close(fd)

            if hasattr(posix, "O_SHLOCK"):
                fd = os.open(test_support.TESTFN,
                             os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
                self.assertRaises(OSError, os.open, test_support.TESTFN,
                                  os.O_WRONLY|os.O_EXLOCK|os.O_NONBLOCK)
                os.close(fd)

    def test_osshlock(self):
        if hasattr(posix, "O_SHLOCK"):
            fd1 = os.open(test_support.TESTFN,
                         os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
            fd2 = os.open(test_support.TESTFN,
                          os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
            os.close(fd2)
            os.close(fd1)

            if hasattr(posix, "O_EXLOCK"):
                fd = os.open(test_support.TESTFN,
                             os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
                self.assertRaises(OSError, os.open, test_support.TESTFN,
                                  os.O_RDONLY|os.O_EXLOCK|os.O_NONBLOCK)
                os.close(fd)

    def test_fstat(self):
        if hasattr(posix, 'fstat'):
            fp = open(test_support.TESTFN)
            try:
                self.assertTrue(posix.fstat(fp.fileno()))
            finally:
                fp.close()

    def test_stat(self):
        if hasattr(posix, 'stat'):
            self.assertTrue(posix.stat(test_support.TESTFN))

    def _test_all_chown_common(self, chown_func, first_param):
        """Common code for chown, fchown and lchown tests."""
        if os.getuid() == 0:
            try:
                # Many linux distros have a nfsnobody user as MAX_UID-2
                # that makes a good test case for signedness issues.
                #   http://bugs.python.org/issue1747858
                # This part of the test only runs when run as root.
                # Only scary people run their tests as root.
                ent = pwd.getpwnam('nfsnobody')
                chown_func(first_param, ent.pw_uid, ent.pw_gid)
            except KeyError:
                pass
        else:
            # non-root cannot chown to root, raises OSError
            self.assertRaises(OSError, chown_func,
                              first_param, 0, 0)

        # test a successful chown call
        chown_func(first_param, os.getuid(), os.getgid())

    @unittest.skipUnless(hasattr(posix, 'chown'), "test needs os.chown()")
    def test_chown(self):
        # raise an OSError if the file does not exist
        os.unlink(test_support.TESTFN)
        self.assertRaises(OSError, posix.chown, test_support.TESTFN, -1, -1)

        # re-create the file
        open(test_support.TESTFN, 'w').close()
        self._test_all_chown_common(posix.chown, test_support.TESTFN)

    @unittest.skipUnless(hasattr(posix, 'fchown'), "test needs os.fchown()")
    def test_fchown(self):
        os.unlink(test_support.TESTFN)

        # re-create the file
        test_file = open(test_support.TESTFN, 'w')
        try:
            fd = test_file.fileno()
            self._test_all_chown_common(posix.fchown, fd)
        finally:
            test_file.close()

    @unittest.skipUnless(hasattr(posix, 'lchown'), "test needs os.lchown()")
    def test_lchown(self):
        os.unlink(test_support.TESTFN)
        # create a symlink
        os.symlink('/tmp/dummy-symlink-target', test_support.TESTFN)
        self._test_all_chown_common(posix.lchown, test_support.TESTFN)

    def test_chdir(self):
        if hasattr(posix, 'chdir'):
            posix.chdir(os.curdir)
            self.assertRaises(OSError, posix.chdir, test_support.TESTFN)

    def test_lsdir(self):
        if hasattr(posix, 'lsdir'):
            self.assertIn(test_support.TESTFN, posix.lsdir(os.curdir))

    def test_access(self):
        if hasattr(posix, 'access'):
            self.assertTrue(posix.access(test_support.TESTFN, os.R_OK))

    def test_umask(self):
        if hasattr(posix, 'umask'):
            old_mask = posix.umask(0)
            self.assertIsInstance(old_mask, int)
            posix.umask(old_mask)

    def test_strerror(self):
        if hasattr(posix, 'strerror'):
            self.assertTrue(posix.strerror(0))

    def test_pipe(self):
        if hasattr(posix, 'pipe'):
            reader, writer = posix.pipe()
            os.close(reader)
            os.close(writer)

    def test_tempnam(self):
        if hasattr(posix, 'tempnam'):
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", "tempnam", DeprecationWarning)
                self.assertTrue(posix.tempnam())
                self.assertTrue(posix.tempnam(os.curdir))
                self.assertTrue(posix.tempnam(os.curdir, 'blah'))

    def test_tmpfile(self):
        if hasattr(posix, 'tmpfile'):
            with warnings.catch_warnings():
                warnings.filterwarnings("ignore", "tmpfile", DeprecationWarning)
                fp = posix.tmpfile()
                fp.close()

    def test_utime(self):
        if hasattr(posix, 'utime'):
            now = time.time()
            posix.utime(test_support.TESTFN, None)
            self.assertRaises(TypeError, posix.utime, test_support.TESTFN, (None, None))
            self.assertRaises(TypeError, posix.utime, test_support.TESTFN, (now, None))
            self.assertRaises(TypeError, posix.utime, test_support.TESTFN, (None, now))
            posix.utime(test_support.TESTFN, (int(now), int(now)))
            posix.utime(test_support.TESTFN, (now, now))

    def test_chflags(self):
        if hasattr(posix, 'chflags'):
            st = os.stat(test_support.TESTFN)
            if hasattr(st, 'st_flags'):
                posix.chflags(test_support.TESTFN, st.st_flags)

    def test_lchflags(self):
        if hasattr(posix, 'lchflags'):
            st = os.stat(test_support.TESTFN)
            if hasattr(st, 'st_flags'):
                posix.lchflags(test_support.TESTFN, st.st_flags)

    def test_getcwd_long_pathnames(self):
        if hasattr(posix, 'getcwd'):
            dirname = 'getcwd-test-directory-0123456789abcdef-01234567890abcdef'
            curdir = os.getcwd()
            base_path = os.path.abspath(test_support.TESTFN) + '.getcwd'

            try:
                os.mkdir(base_path)
                os.chdir(base_path)
            except:
#               Just returning nothing instead of the SkipTest exception,
#               because the test results in Error in that case.
#               Is that ok?
#                raise unittest.SkipTest, "cannot create directory for testing"
                return

            try:
                def _create_and_do_getcwd(dirname, current_path_length = 0):
                    try:
                        os.mkdir(dirname)
                    except:
                        raise unittest.SkipTest, "mkdir cannot create directory sufficiently deep for getcwd test"

                    os.chdir(dirname)
                    try:
                        os.getcwd()
                        if current_path_length < 4099:
                            _create_and_do_getcwd(dirname, current_path_length + len(dirname) + 1)
                    except OSError as e:
                        expected_errno = errno.ENAMETOOLONG
                        if 'sunos' in sys.platform or 'openbsd' in sys.platform:
                            expected_errno = errno.ERANGE # Issue 9185
                        self.assertEqual(e.errno, expected_errno)
                    finally:
                        os.chdir('..')
                        os.rmdir(dirname)

                _create_and_do_getcwd(dirname)

            finally:
                os.chdir(curdir)
                shutil.rmtree(base_path)

    @unittest.skipUnless(hasattr(os, 'getegid'), "test needs os.getegid()")
    def test_getgroups(self):
        with os.popen('id -G') as idg:
            groups = idg.read().strip()

        if not groups:
            raise unittest.SkipTest("need working 'id -G'")

        # 'id -G' and 'os.getgroups()' should return the same
        # groups, ignoring order and duplicates.
        # #10822 - it is implementation defined whether posix.getgroups()
        # includes the effective gid so we include it anyway, since id -G does
        self.assertEqual(
                set([int(x) for x in groups.split()]),
                set(posix.getgroups() + [posix.getegid()]))

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

        g = max(self.saved_groups) + 1
        name = pwd.getpwuid(posix.getuid()).pw_name
        posix.initgroups(name, g)
        self.assertIn(g, posix.getgroups())

    @unittest.skipUnless(hasattr(posix, 'setgroups'),
                         "test needs posix.setgroups()")
    def test_setgroups(self):
        for groups in [[0], range(16)]:
            posix.setgroups(groups)
            self.assertListEqual(groups, posix.getgroups())


def test_main():
    test_support.run_unittest(PosixTester, PosixGroupsTester)

if __name__ == '__main__':
    test_main()
