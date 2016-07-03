"Test posix functions"

from test import test_support

# Skip these tests if there is no posix module.
posix = test_support.import_module('posix')

import errno
import sys
import time
import os
import platform
import pwd
import shutil
import stat
import sys
import tempfile
import unittest
import warnings

_DUMMY_SYMLINK = os.path.join(tempfile.gettempdir(),
                              test_support.TESTFN + '-dummy-symlink')

warnings.filterwarnings('ignore', '.* potential security risk .*',
                        RuntimeWarning)

class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        fp = open(test_support.TESTFN, 'w+')
        fp.close()
        self.teardown_files = [ test_support.TESTFN ]

    def tearDown(self):
        for teardown_file in self.teardown_files:
            os.unlink(teardown_file)

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

    @unittest.skipUnless(hasattr(posix, 'statvfs'),
                         'test needs posix.statvfs()')
    def test_statvfs(self):
        self.assertTrue(posix.statvfs(os.curdir))

    @unittest.skipUnless(hasattr(posix, 'fstatvfs'),
                         'test needs posix.fstatvfs()')
    def test_fstatvfs(self):
        fp = open(test_support.TESTFN)
        try:
            self.assertTrue(posix.fstatvfs(fp.fileno()))
        finally:
            fp.close()

    @unittest.skipUnless(hasattr(posix, 'ftruncate'),
                         'test needs posix.ftruncate()')
    def test_ftruncate(self):
        fp = open(test_support.TESTFN, 'w+')
        try:
            # we need to have some data to truncate
            fp.write('test')
            fp.flush()
            posix.ftruncate(fp.fileno(), 0)
        finally:
            fp.close()

    @unittest.skipUnless(hasattr(posix, 'dup'),
                         'test needs posix.dup()')
    def test_dup(self):
        fp = open(test_support.TESTFN)
        try:
            fd = posix.dup(fp.fileno())
            self.assertIsInstance(fd, int)
            os.close(fd)
        finally:
            fp.close()

    @unittest.skipUnless(hasattr(posix, 'confstr'),
                         'test needs posix.confstr()')
    def test_confstr(self):
        self.assertRaises(ValueError, posix.confstr, "CS_garbage")
        self.assertEqual(len(posix.confstr("CS_PATH")) > 0, True)

    @unittest.skipUnless(hasattr(posix, 'dup2'),
                         'test needs posix.dup2()')
    def test_dup2(self):
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

    @unittest.skipUnless(hasattr(posix, 'fdopen'),
                         'test needs posix.fdopen()')
    def test_fdopen(self):
        self.fdopen_helper()
        self.fdopen_helper('r')
        self.fdopen_helper('r', 100)

    @unittest.skipUnless(hasattr(posix, 'fdopen'),
                         'test needs posix.fdopen()')
    def test_fdopen_directory(self):
        try:
            fd = os.open('.', os.O_RDONLY)
        except OSError as e:
            self.assertEqual(e.errno, errno.EACCES)
            self.skipTest("system cannot open directories")
        with self.assertRaises(IOError) as cm:
            os.fdopen(fd, 'r')
        self.assertEqual(cm.exception.errno, errno.EISDIR)

    @unittest.skipUnless(hasattr(posix, 'fdopen') and
                         not sys.platform.startswith("sunos"),
                         'test needs posix.fdopen()')
    def test_fdopen_keeps_fd_open_on_errors(self):
        fd = os.open(test_support.TESTFN, os.O_RDONLY)
        self.assertRaises(OSError, posix.fdopen, fd, 'w')
        os.close(fd) # fd should not be closed.

    @unittest.skipUnless(hasattr(posix, 'O_EXLOCK'),
                         'test needs posix.O_EXLOCK')
    def test_osexlock(self):
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

    @unittest.skipUnless(hasattr(posix, 'O_SHLOCK'),
                         'test needs posix.O_SHLOCK')
    def test_osshlock(self):
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

    @unittest.skipUnless(hasattr(posix, 'fstat'),
                         'test needs posix.fstat()')
    def test_fstat(self):
        fp = open(test_support.TESTFN)
        try:
            self.assertTrue(posix.fstat(fp.fileno()))
        finally:
            fp.close()

    @unittest.skipUnless(hasattr(posix, 'stat'),
                         'test needs posix.stat()')
    def test_stat(self):
        self.assertTrue(posix.stat(test_support.TESTFN))

    @unittest.skipUnless(hasattr(posix, 'stat'), 'test needs posix.stat()')
    @unittest.skipUnless(hasattr(posix, 'makedev'), 'test needs posix.makedev()')
    def test_makedev(self):
        st = posix.stat(test_support.TESTFN)
        dev = st.st_dev
        self.assertIsInstance(dev, (int, long))
        self.assertGreaterEqual(dev, 0)

        major = posix.major(dev)
        self.assertIsInstance(major, (int, long))
        self.assertGreaterEqual(major, 0)
        self.assertEqual(posix.major(int(dev)), major)
        self.assertEqual(posix.major(long(dev)), major)
        self.assertRaises(TypeError, posix.major, float(dev))
        self.assertRaises(TypeError, posix.major)
        self.assertRaises((ValueError, OverflowError), posix.major, -1)

        minor = posix.minor(dev)
        self.assertIsInstance(minor, (int, long))
        self.assertGreaterEqual(minor, 0)
        self.assertEqual(posix.minor(int(dev)), minor)
        self.assertEqual(posix.minor(long(dev)), minor)
        self.assertRaises(TypeError, posix.minor, float(dev))
        self.assertRaises(TypeError, posix.minor)
        self.assertRaises((ValueError, OverflowError), posix.minor, -1)

        self.assertEqual(posix.makedev(major, minor), dev)
        self.assertEqual(posix.makedev(int(major), int(minor)), dev)
        self.assertEqual(posix.makedev(long(major), long(minor)), dev)
        self.assertRaises(TypeError, posix.makedev, float(major), minor)
        self.assertRaises(TypeError, posix.makedev, major, float(minor))
        self.assertRaises(TypeError, posix.makedev, major)
        self.assertRaises(TypeError, posix.makedev)

    def _test_all_chown_common(self, chown_func, first_param, stat_func):
        """Common code for chown, fchown and lchown tests."""
        def check_stat(uid, gid):
            if stat_func is not None:
                stat = stat_func(first_param)
                self.assertEqual(stat.st_uid, uid)
                self.assertEqual(stat.st_gid, gid)
        uid = os.getuid()
        gid = os.getgid()
        # test a successful chown call
        chown_func(first_param, uid, gid)
        check_stat(uid, gid)
        chown_func(first_param, -1, gid)
        check_stat(uid, gid)
        chown_func(first_param, uid, -1)
        check_stat(uid, gid)

        if uid == 0:
            # Try an amusingly large uid/gid to make sure we handle
            # large unsigned values.  (chown lets you use any
            # uid/gid you like, even if they aren't defined.)
            #
            # This problem keeps coming up:
            #   http://bugs.python.org/issue1747858
            #   http://bugs.python.org/issue4591
            #   http://bugs.python.org/issue15301
            # Hopefully the fix in 4591 fixes it for good!
            #
            # This part of the test only runs when run as root.
            # Only scary people run their tests as root.

            big_value = 2**31
            chown_func(first_param, big_value, big_value)
            check_stat(big_value, big_value)
            chown_func(first_param, -1, -1)
            check_stat(big_value, big_value)
            chown_func(first_param, uid, gid)
            check_stat(uid, gid)
        elif platform.system() in ('HP-UX', 'SunOS'):
            # HP-UX and Solaris can allow a non-root user to chown() to root
            # (issue #5113)
            raise unittest.SkipTest("Skipping because of non-standard chown() "
                                    "behavior")
        else:
            # non-root cannot chown to root, raises OSError
            self.assertRaises(OSError, chown_func, first_param, 0, 0)
            check_stat(uid, gid)
            self.assertRaises(OSError, chown_func, first_param, 0, -1)
            check_stat(uid, gid)
            if 0 not in os.getgroups():
                self.assertRaises(OSError, chown_func, first_param, -1, 0)
                check_stat(uid, gid)
        # test illegal types
        for t in str, float:
            self.assertRaises(TypeError, chown_func, first_param, t(uid), gid)
            check_stat(uid, gid)
            self.assertRaises(TypeError, chown_func, first_param, uid, t(gid))
            check_stat(uid, gid)

    @unittest.skipUnless(hasattr(posix, 'chown'), "test needs os.chown()")
    def test_chown(self):
        # raise an OSError if the file does not exist
        os.unlink(test_support.TESTFN)
        self.assertRaises(OSError, posix.chown, test_support.TESTFN, -1, -1)

        # re-create the file
        open(test_support.TESTFN, 'w').close()
        self._test_all_chown_common(posix.chown, test_support.TESTFN,
                                    getattr(posix, 'stat', None))

    @unittest.skipUnless(hasattr(posix, 'fchown'), "test needs os.fchown()")
    def test_fchown(self):
        os.unlink(test_support.TESTFN)

        # re-create the file
        test_file = open(test_support.TESTFN, 'w')
        try:
            fd = test_file.fileno()
            self._test_all_chown_common(posix.fchown, fd,
                                        getattr(posix, 'fstat', None))
        finally:
            test_file.close()

    @unittest.skipUnless(hasattr(posix, 'lchown'), "test needs os.lchown()")
    def test_lchown(self):
        os.unlink(test_support.TESTFN)
        # create a symlink
        os.symlink(_DUMMY_SYMLINK, test_support.TESTFN)
        self._test_all_chown_common(posix.lchown, test_support.TESTFN,
                                    getattr(posix, 'lstat', None))

    @unittest.skipUnless(hasattr(posix, 'chdir'), 'test needs posix.chdir()')
    def test_chdir(self):
        posix.chdir(os.curdir)
        self.assertRaises(OSError, posix.chdir, test_support.TESTFN)

    @unittest.skipUnless(hasattr(posix, 'lsdir'), 'test needs posix.lsdir()')
    def test_lsdir(self):
        self.assertIn(test_support.TESTFN, posix.lsdir(os.curdir))

    @unittest.skipUnless(hasattr(posix, 'access'), 'test needs posix.access()')
    def test_access(self):
        self.assertTrue(posix.access(test_support.TESTFN, os.R_OK))

    @unittest.skipUnless(hasattr(posix, 'umask'), 'test needs posix.umask()')
    def test_umask(self):
        old_mask = posix.umask(0)
        self.assertIsInstance(old_mask, int)
        posix.umask(old_mask)

    @unittest.skipUnless(hasattr(posix, 'strerror'),
                         'test needs posix.strerror()')
    def test_strerror(self):
        self.assertTrue(posix.strerror(0))

    @unittest.skipUnless(hasattr(posix, 'pipe'), 'test needs posix.pipe()')
    def test_pipe(self):
        reader, writer = posix.pipe()
        os.close(reader)
        os.close(writer)

    @unittest.skipUnless(hasattr(posix, 'tempnam'),
                         'test needs posix.tempnam()')
    def test_tempnam(self):
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", "tempnam", DeprecationWarning)
            self.assertTrue(posix.tempnam())
            self.assertTrue(posix.tempnam(os.curdir))
            self.assertTrue(posix.tempnam(os.curdir, 'blah'))

    @unittest.skipUnless(hasattr(posix, 'tmpfile'),
                         'test needs posix.tmpfile()')
    def test_tmpfile(self):
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", "tmpfile", DeprecationWarning)
            fp = posix.tmpfile()
            fp.close()

    @unittest.skipUnless(hasattr(posix, 'utime'), 'test needs posix.utime()')
    def test_utime(self):
        now = time.time()
        posix.utime(test_support.TESTFN, None)
        self.assertRaises(TypeError, posix.utime, test_support.TESTFN, (None, None))
        self.assertRaises(TypeError, posix.utime, test_support.TESTFN, (now, None))
        self.assertRaises(TypeError, posix.utime, test_support.TESTFN, (None, now))
        posix.utime(test_support.TESTFN, (int(now), int(now)))
        posix.utime(test_support.TESTFN, (now, now))

    def _test_chflags_regular_file(self, chflags_func, target_file):
        st = os.stat(target_file)
        self.assertTrue(hasattr(st, 'st_flags'))

        # ZFS returns EOPNOTSUPP when attempting to set flag UF_IMMUTABLE.
        try:
            chflags_func(target_file, st.st_flags | stat.UF_IMMUTABLE)
        except OSError as err:
            if err.errno != errno.EOPNOTSUPP:
                raise
            msg = 'chflag UF_IMMUTABLE not supported by underlying fs'
            self.skipTest(msg)

        try:
            new_st = os.stat(target_file)
            self.assertEqual(st.st_flags | stat.UF_IMMUTABLE, new_st.st_flags)
            try:
                fd = open(target_file, 'w+')
            except IOError as e:
                self.assertEqual(e.errno, errno.EPERM)
        finally:
            posix.chflags(target_file, st.st_flags)

    @unittest.skipUnless(hasattr(posix, 'chflags'), 'test needs os.chflags()')
    def test_chflags(self):
        self._test_chflags_regular_file(posix.chflags, test_support.TESTFN)

    @unittest.skipUnless(hasattr(posix, 'lchflags'), 'test needs os.lchflags()')
    def test_lchflags_regular_file(self):
        self._test_chflags_regular_file(posix.lchflags, test_support.TESTFN)

    @unittest.skipUnless(hasattr(posix, 'lchflags'), 'test needs os.lchflags()')
    def test_lchflags_symlink(self):
        testfn_st = os.stat(test_support.TESTFN)

        self.assertTrue(hasattr(testfn_st, 'st_flags'))

        os.symlink(test_support.TESTFN, _DUMMY_SYMLINK)
        self.teardown_files.append(_DUMMY_SYMLINK)
        dummy_symlink_st = os.lstat(_DUMMY_SYMLINK)

        # ZFS returns EOPNOTSUPP when attempting to set flag UF_IMMUTABLE.
        try:
            posix.lchflags(_DUMMY_SYMLINK,
                           dummy_symlink_st.st_flags | stat.UF_IMMUTABLE)
        except OSError as err:
            if err.errno != errno.EOPNOTSUPP:
                raise
            msg = 'chflag UF_IMMUTABLE not supported by underlying fs'
            self.skipTest(msg)

        try:
            new_testfn_st = os.stat(test_support.TESTFN)
            new_dummy_symlink_st = os.lstat(_DUMMY_SYMLINK)

            self.assertEqual(testfn_st.st_flags, new_testfn_st.st_flags)
            self.assertEqual(dummy_symlink_st.st_flags | stat.UF_IMMUTABLE,
                             new_dummy_symlink_st.st_flags)
        finally:
            posix.lchflags(_DUMMY_SYMLINK, dummy_symlink_st.st_flags)

    @unittest.skipUnless(hasattr(posix, 'getcwd'),
                         'test needs posix.getcwd()')
    def test_getcwd_long_pathnames(self):
        dirname = 'getcwd-test-directory-0123456789abcdef-01234567890abcdef'
        curdir = os.getcwd()
        base_path = os.path.abspath(test_support.TESTFN) + '.getcwd'

        try:
            os.mkdir(base_path)
            os.chdir(base_path)
        except:
            self.skipTest("cannot create directory for testing")

        try:
            def _create_and_do_getcwd(dirname, current_path_length = 0):
                try:
                    os.mkdir(dirname)
                except:
                    self.skipTest("mkdir cannot create directory sufficiently "
                                  "deep for getcwd test")

                os.chdir(dirname)
                try:
                    os.getcwd()
                    if current_path_length < 4099:
                        _create_and_do_getcwd(dirname, current_path_length + len(dirname) + 1)
                except OSError as e:
                    expected_errno = errno.ENAMETOOLONG
                    # The following platforms have quirky getcwd()
                    # behaviour -- see issue 9185 and 15765 for
                    # more information.
                    quirky_platform = (
                        'sunos' in sys.platform or
                        'netbsd' in sys.platform or
                        'openbsd' in sys.platform
                    )
                    if quirky_platform:
                        expected_errno = errno.ERANGE
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
        with os.popen('id -G 2>/dev/null') as idg:
            groups = idg.read().strip()
            ret = idg.close()

        if ret != None or not groups:
            raise unittest.SkipTest("need working 'id -G'")

        # Issues 16698: OS X ABIs prior to 10.6 have limits on getgroups()
        if sys.platform == 'darwin':
            import sysconfig
            dt = sysconfig.get_config_var('MACOSX_DEPLOYMENT_TARGET') or '10.0'
            if tuple(int(n) for n in dt.split('.')[0:2]) < (10, 6):
                raise unittest.SkipTest("getgroups(2) is broken prior to 10.6")

        # 'id -G' and 'os.getgroups()' should return the same
        # groups, ignoring order and duplicates.
        # #10822 - it is implementation defined whether posix.getgroups()
        # includes the effective gid so we include it anyway, since id -G does
        self.assertEqual(
                set([int(x) for x in groups.split()]),
                set(posix.getgroups() + [posix.getegid()]))

    @test_support.requires_unicode
    def test_path_with_null_unicode(self):
        fn = test_support.TESTFN_UNICODE
        try:
            fn.encode(test_support.TESTFN_ENCODING)
        except (UnicodeError, TypeError):
            self.skipTest("Requires unicode filenames support")
        fn_with_NUL = fn + u'\0'
        self.addCleanup(test_support.unlink, fn)
        test_support.unlink(fn)
        fd = None
        try:
            with self.assertRaises(TypeError):
                fd = os.open(fn_with_NUL, os.O_WRONLY | os.O_CREAT) # raises
        finally:
            if fd is not None:
                os.close(fd)
        self.assertFalse(os.path.exists(fn))
        self.assertRaises(TypeError, os.mkdir, fn_with_NUL)
        self.assertFalse(os.path.exists(fn))
        open(fn, 'wb').close()
        self.assertRaises(TypeError, os.stat, fn_with_NUL)

    def test_path_with_null_byte(self):
        fn = test_support.TESTFN
        fn_with_NUL = fn + '\0'
        self.addCleanup(test_support.unlink, fn)
        test_support.unlink(fn)
        fd = None
        try:
            with self.assertRaises(TypeError):
                fd = os.open(fn_with_NUL, os.O_WRONLY | os.O_CREAT) # raises
        finally:
            if fd is not None:
                os.close(fd)
        self.assertFalse(os.path.exists(fn))
        self.assertRaises(TypeError, os.mkdir, fn_with_NUL)
        self.assertFalse(os.path.exists(fn))
        open(fn, 'wb').close()
        self.assertRaises(TypeError, os.stat, fn_with_NUL)


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
                         'test needs posix.initgroups()')
    def test_initgroups(self):
        # find missing group

        g = max(self.saved_groups or [0]) + 1
        name = pwd.getpwuid(posix.getuid()).pw_name
        posix.initgroups(name, g)
        self.assertIn(g, posix.getgroups())

    @unittest.skipUnless(hasattr(posix, 'setgroups'),
                         'test needs posix.setgroups()')
    def test_setgroups(self):
        for groups in [[0], range(16)]:
            posix.setgroups(groups)
            self.assertListEqual(groups, posix.getgroups())


def test_main():
    test_support.run_unittest(PosixTester, PosixGroupsTester)

if __name__ == '__main__':
    test_main()
