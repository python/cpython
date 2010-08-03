"Test posix functions"

from test import test_support

try:
    import posix
except ImportError:
    raise test_support.TestSkipped, "posix is not available"

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

        for name in NO_ARG_FUNCTIONS:
            posix_func = getattr(posix, name, None)
            if posix_func is not None:
                posix_func()
                self.assertRaises(TypeError, posix_func, 1)

    def test_statvfs(self):
        if hasattr(posix, 'statvfs'):
            self.assert_(posix.statvfs(os.curdir))

    def test_fstatvfs(self):
        if hasattr(posix, 'fstatvfs'):
            fp = open(test_support.TESTFN)
            try:
                self.assert_(posix.fstatvfs(fp.fileno()))
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
                self.assert_(isinstance(fd, int))
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
                self.assert_(posix.fstat(fp.fileno()))
            finally:
                fp.close()

    def test_stat(self):
        if hasattr(posix, 'stat'):
            self.assert_(posix.stat(test_support.TESTFN))

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

    def _test_chown(self):
        # raise an OSError if the file does not exist
        os.unlink(test_support.TESTFN)
        self.assertRaises(OSError, posix.chown, test_support.TESTFN, -1, -1)

        # re-create the file
        open(test_support.TESTFN, 'w').close()
        self._test_all_chown_common(posix.chown, test_support.TESTFN)

    if hasattr(posix, 'chown'):
        test_chown = _test_chown

    def _test_fchown(self):
        os.unlink(test_support.TESTFN)

        # re-create the file
        test_file = open(test_support.TESTFN, 'w')
        try:
            fd = test_file.fileno()
            self._test_all_chown_common(posix.fchown, fd)
        finally:
            test_file.close()

    if hasattr(posix, 'fchown'):
        test_fchown = _test_fchown

    def _test_lchown(self):
        os.unlink(test_support.TESTFN)
        # create a symlink
        os.symlink('/tmp/dummy-symlink-target', test_support.TESTFN)
        self._test_all_chown_common(posix.lchown, test_support.TESTFN)

    if hasattr(posix, 'lchown'):
        test_lchown = _test_lchown

    def test_chdir(self):
        if hasattr(posix, 'chdir'):
            posix.chdir(os.curdir)
            self.assertRaises(OSError, posix.chdir, test_support.TESTFN)

    def test_lsdir(self):
        if hasattr(posix, 'lsdir'):
            self.assert_(test_support.TESTFN in posix.lsdir(os.curdir))

    def test_access(self):
        if hasattr(posix, 'access'):
            self.assert_(posix.access(test_support.TESTFN, os.R_OK))

    def test_umask(self):
        if hasattr(posix, 'umask'):
            old_mask = posix.umask(0)
            self.assert_(isinstance(old_mask, int))
            posix.umask(old_mask)

    def test_strerror(self):
        if hasattr(posix, 'strerror'):
            self.assert_(posix.strerror(0))

    def test_pipe(self):
        if hasattr(posix, 'pipe'):
            reader, writer = posix.pipe()
            os.close(reader)
            os.close(writer)

    def test_tempnam(self):
        if hasattr(posix, 'tempnam'):
            self.assert_(posix.tempnam())
            self.assert_(posix.tempnam(os.curdir))
            self.assert_(posix.tempnam(os.curdir, 'blah'))

    def test_tmpfile(self):
        if hasattr(posix, 'tmpfile'):
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
#               Just returning nothing instead of the TestSkipped exception,
#               because the test results in Error in that case.
#               Is that ok?
#                raise test_support.TestSkipped, "cannot create directory for testing"
                return

            try:
                def _create_and_do_getcwd(dirname, current_path_length = 0):
                    try:
                        os.mkdir(dirname)
                    except:
                        raise test_support.TestSkipped, "mkdir cannot create directory sufficiently deep for getcwd test"

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

    def test_getgroups(self):
        with os.popen('id -G 2>/dev/null') as idg:
            groups = idg.read().strip()

        if not groups:
            # This test needs 'id -G'
            return

        # 'id -G' and 'os.getgroups()' should return the same
        # groups, ignoring order and duplicates.
        self.assertEqual(
                set([int(x) for x in groups.split()]),
                set(posix.getgroups()))

class PosixGroupsTester(unittest.TestCase):
    if posix.getuid() == 0 and hasattr(posix, 'getgroups') and sys.platform != 'darwin':

        def setUp(self):
            self.saved_groups = posix.getgroups()

        def tearDown(self):
            if hasattr(posix, 'setgroups'):
                posix.setgroups(self.saved_groups)
            elif hasattr(posix, 'initgroups'):
                name = pwd.getpwuid(posix.getuid()).pw_name
                posix.initgroups(name, self.saved_groups[0])

        if hasattr(posix, 'initgroups'):
            def test_initgroups(self):
                # find missing group

                groups = sorted(self.saved_groups)
                for g1,g2 in zip(groups[:-1], groups[1:]):
                    g = g1 + 1
                    if g < g2:
                        break
                else:
                    g = g2 + 1
                name = pwd.getpwuid(posix.getuid()).pw_name
                posix.initgroups(name, g)
                self.assertIn(g, posix.getgroups())

        if hasattr(posix, 'setgroups'):
            def test_setgroups(self):
                for groups in [[0], range(16)]:
                    posix.setgroups(groups)
                    self.assertListEqual(groups, posix.getgroups())


def test_main():
    test_support.run_unittest(PosixTester, PosixGroupsTester)

if __name__ == '__main__':
    test_main()
