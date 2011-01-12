"Test posix functions"

from test import support

# Skip these tests if there is no posix module.
posix = support.import_module('posix')

import sys
import time
import os
import pwd
import shutil
import unittest
import warnings

warnings.filterwarnings('ignore', '.* potential security risk .*',
                        RuntimeWarning)

class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        fp = open(support.TESTFN, 'w+')
        fp.close()

    def tearDown(self):
        support.unlink(support.TESTFN)

    def testNoArgFunctions(self):
        # test posix functions which take no arguments and have
        # no side-effects which we need to cleanup (e.g., fork, wait, abort)
        NO_ARG_FUNCTIONS = [ "ctermid", "getcwd", "getcwdb", "uname",
                             "times", "getloadavg",
                             "getegid", "geteuid", "getgid", "getgroups",
                             "getpid", "getpgrp", "getppid", "getuid",
                           ]

        for name in NO_ARG_FUNCTIONS:
            posix_func = getattr(posix, name, None)
            if posix_func is not None:
                posix_func()
                self.assertRaises(TypeError, posix_func, 1)

    def test_statvfs(self):
        if hasattr(posix, 'statvfs'):
            self.assertTrue(posix.statvfs(os.curdir))

    def test_fstatvfs(self):
        if hasattr(posix, 'fstatvfs'):
            fp = open(support.TESTFN)
            try:
                self.assertTrue(posix.fstatvfs(fp.fileno()))
            finally:
                fp.close()

    def test_ftruncate(self):
        if hasattr(posix, 'ftruncate'):
            fp = open(support.TESTFN, 'w+')
            try:
                # we need to have some data to truncate
                fp.write('test')
                fp.flush()
                posix.ftruncate(fp.fileno(), 0)
            finally:
                fp.close()

    def test_dup(self):
        if hasattr(posix, 'dup'):
            fp = open(support.TESTFN)
            try:
                fd = posix.dup(fp.fileno())
                self.assertTrue(isinstance(fd, int))
                os.close(fd)
            finally:
                fp.close()

    def test_confstr(self):
        if hasattr(posix, 'confstr'):
            self.assertRaises(ValueError, posix.confstr, "CS_garbage")
            self.assertEqual(len(posix.confstr("CS_PATH")) > 0, True)

    def test_dup2(self):
        if hasattr(posix, 'dup2'):
            fp1 = open(support.TESTFN)
            fp2 = open(support.TESTFN)
            try:
                posix.dup2(fp1.fileno(), fp2.fileno())
            finally:
                fp1.close()
                fp2.close()

    def test_osexlock(self):
        if hasattr(posix, "O_EXLOCK"):
            fd = os.open(support.TESTFN,
                         os.O_WRONLY|os.O_EXLOCK|os.O_CREAT)
            self.assertRaises(OSError, os.open, support.TESTFN,
                              os.O_WRONLY|os.O_EXLOCK|os.O_NONBLOCK)
            os.close(fd)

            if hasattr(posix, "O_SHLOCK"):
                fd = os.open(support.TESTFN,
                             os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
                self.assertRaises(OSError, os.open, support.TESTFN,
                                  os.O_WRONLY|os.O_EXLOCK|os.O_NONBLOCK)
                os.close(fd)

    def test_osshlock(self):
        if hasattr(posix, "O_SHLOCK"):
            fd1 = os.open(support.TESTFN,
                         os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
            fd2 = os.open(support.TESTFN,
                          os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
            os.close(fd2)
            os.close(fd1)

            if hasattr(posix, "O_EXLOCK"):
                fd = os.open(support.TESTFN,
                             os.O_WRONLY|os.O_SHLOCK|os.O_CREAT)
                self.assertRaises(OSError, os.open, support.TESTFN,
                                  os.O_RDONLY|os.O_EXLOCK|os.O_NONBLOCK)
                os.close(fd)

    def test_fstat(self):
        if hasattr(posix, 'fstat'):
            fp = open(support.TESTFN)
            try:
                self.assertTrue(posix.fstat(fp.fileno()))
            finally:
                fp.close()

    def test_stat(self):
        if hasattr(posix, 'stat'):
            self.assertTrue(posix.stat(support.TESTFN))

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
        os.unlink(support.TESTFN)
        self.assertRaises(OSError, posix.chown, support.TESTFN, -1, -1)

        # re-create the file
        open(support.TESTFN, 'w').close()
        self._test_all_chown_common(posix.chown, support.TESTFN)

    @unittest.skipUnless(hasattr(posix, 'fchown'), "test needs os.fchown()")
    def test_fchown(self):
        os.unlink(support.TESTFN)

        # re-create the file
        test_file = open(support.TESTFN, 'w')
        try:
            fd = test_file.fileno()
            self._test_all_chown_common(posix.fchown, fd)
        finally:
            test_file.close()

    @unittest.skipUnless(hasattr(posix, 'lchown'), "test needs os.lchown()")
    def test_lchown(self):
        os.unlink(support.TESTFN)
        # create a symlink
        os.symlink('/tmp/dummy-symlink-target', support.TESTFN)
        self._test_all_chown_common(posix.lchown, support.TESTFN)

    def test_chdir(self):
        if hasattr(posix, 'chdir'):
            posix.chdir(os.curdir)
            self.assertRaises(OSError, posix.chdir, support.TESTFN)

    def test_lsdir(self):
        if hasattr(posix, 'lsdir'):
            self.assertTrue(support.TESTFN in posix.lsdir(os.curdir))

    def test_access(self):
        if hasattr(posix, 'access'):
            self.assertTrue(posix.access(support.TESTFN, os.R_OK))

    def test_umask(self):
        if hasattr(posix, 'umask'):
            old_mask = posix.umask(0)
            self.assertTrue(isinstance(old_mask, int))
            posix.umask(old_mask)

    def test_strerror(self):
        if hasattr(posix, 'strerror'):
            self.assertTrue(posix.strerror(0))

    def test_pipe(self):
        if hasattr(posix, 'pipe'):
            reader, writer = posix.pipe()
            os.close(reader)
            os.close(writer)

    def test_utime(self):
        if hasattr(posix, 'utime'):
            now = time.time()
            posix.utime(support.TESTFN, None)
            self.assertRaises(TypeError, posix.utime, support.TESTFN, (None, None))
            self.assertRaises(TypeError, posix.utime, support.TESTFN, (now, None))
            self.assertRaises(TypeError, posix.utime, support.TESTFN, (None, now))
            posix.utime(support.TESTFN, (int(now), int(now)))
            posix.utime(support.TESTFN, (now, now))

    def test_chflags(self):
        if hasattr(posix, 'chflags'):
            st = os.stat(support.TESTFN)
            if hasattr(st, 'st_flags'):
                posix.chflags(support.TESTFN, st.st_flags)

    def test_lchflags(self):
        if hasattr(posix, 'lchflags'):
            st = os.stat(support.TESTFN)
            if hasattr(st, 'st_flags'):
                posix.lchflags(support.TESTFN, st.st_flags)

    def test_environ(self):
        for k, v in posix.environ.items():
            self.assertEqual(type(k), str)
            self.assertEqual(type(v), str)

    def test_getcwd_long_pathnames(self):
        if hasattr(posix, 'getcwd'):
            dirname = 'getcwd-test-directory-0123456789abcdef-01234567890abcdef'
            curdir = os.getcwd()
            base_path = os.path.abspath(support.TESTFN) + '.getcwd'

            try:
                os.mkdir(base_path)
                os.chdir(base_path)
            except:
#               Just returning nothing instead of the SkipTest exception,
#               because the test results in Error in that case.
#               Is that ok?
#                raise unittest.SkipTest("cannot create directory for testing")
                return

                def _create_and_do_getcwd(dirname, current_path_length = 0):
                    try:
                        os.mkdir(dirname)
                    except:
                        raise unittest.SkipTest("mkdir cannot create directory sufficiently deep for getcwd test")

                    os.chdir(dirname)
                    try:
                        os.getcwd()
                        if current_path_length < 1027:
                            _create_and_do_getcwd(dirname, current_path_length + len(dirname) + 1)
                    finally:
                        os.chdir('..')
                        os.rmdir(dirname)

                _create_and_do_getcwd(dirname)

            finally:
                os.chdir(curdir)
                support.rmtree(base_path)

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
        for groups in [[0], list(range(16))]:
            posix.setgroups(groups)
            self.assertListEqual(groups, posix.getgroups())


def test_main():
    support.run_unittest(PosixTester, PosixGroupsTester)

if __name__ == '__main__':
    test_main()
