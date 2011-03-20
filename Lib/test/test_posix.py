"Test posix functions"

from test import support

# Skip these tests if there is no posix module.
posix = support.import_module('posix')

import errno
import sys
import time
import os
import pwd
import shutil
import stat
import unittest
import warnings


class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        fp = open(support.TESTFN, 'w+')
        fp.close()
        self._warnings_manager = support.check_warnings()
        self._warnings_manager.__enter__()
        warnings.filterwarnings('ignore', '.* potential security risk .*',
                                RuntimeWarning)

    def tearDown(self):
        support.unlink(support.TESTFN)
        self._warnings_manager.__exit__(None, None, None)

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

    @unittest.skipUnless(hasattr(posix, 'truncate'), "test needs posix.truncate()")
    def test_truncate(self):
        with open(support.TESTFN, 'w') as fp:
            fp.write('test')
            fp.flush()
        posix.truncate(support.TESTFN, 0)

    @unittest.skipUnless(hasattr(posix, 'fexecve'), "test needs posix.fexecve()")
    @unittest.skipUnless(hasattr(os, 'fork'), "test needs os.fork()")
    @unittest.skipUnless(hasattr(os, 'waitpid'), "test needs os.waitpid()")
    def test_fexecve(self):
        fp = os.open(sys.executable, os.O_RDONLY)
        try:
            pid = os.fork()
            if pid == 0:
                os.chdir(os.path.split(sys.executable)[0])
                posix.fexecve(fp, [sys.executable, '-c', 'pass'], os.environ)
            else:
                self.assertEqual(os.waitpid(pid, 0), (pid, 0))
        finally:
            os.close(fp)

    @unittest.skipUnless(hasattr(posix, 'waitid'), "test needs posix.waitid()")
    @unittest.skipUnless(hasattr(os, 'fork'), "test needs os.fork()")
    def test_waitid(self):
        pid = os.fork()
        if pid == 0:
            os.chdir(os.path.split(sys.executable)[0])
            posix.execve(sys.executable, [sys.executable, '-c', 'pass'], os.environ)
        else:
            res = posix.waitid(posix.P_PID, pid, posix.WEXITED)
            self.assertEqual(pid, res.si_pid)

    @unittest.skipUnless(hasattr(posix, 'lockf'), "test needs posix.lockf()")
    def test_lockf(self):
        fd = os.open(support.TESTFN, os.O_WRONLY | os.O_CREAT)
        try:
            os.write(fd, b'test')
            os.lseek(fd, 0, os.SEEK_SET)
            posix.lockf(fd, posix.F_LOCK, 4)
            # section is locked
            posix.lockf(fd, posix.F_ULOCK, 4)
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'pread'), "test needs posix.pread()")
    def test_pread(self):
        fd = os.open(support.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            os.write(fd, b'test')
            os.lseek(fd, 0, os.SEEK_SET)
            self.assertEqual(b'es', posix.pread(fd, 2, 1))
            # the first pread() shoudn't disturb the file offset
            self.assertEqual(b'te', posix.read(fd, 2))
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'pwrite'), "test needs posix.pwrite()")
    def test_pwrite(self):
        fd = os.open(support.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            os.write(fd, b'test')
            os.lseek(fd, 0, os.SEEK_SET)
            posix.pwrite(fd, b'xx', 1)
            self.assertEqual(b'txxt', posix.read(fd, 4))
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'posix_fallocate'),
        "test needs posix.posix_fallocate()")
    def test_posix_fallocate(self):
        fd = os.open(support.TESTFN, os.O_WRONLY | os.O_CREAT)
        try:
            posix.posix_fallocate(fd, 0, 10)
        except OSError as inst:
            # issue10812, ZFS doesn't appear to support posix_fallocate,
            # so skip Solaris-based since they are likely to have ZFS.
            if inst.errno != errno.EINVAL or not sys.platform.startswith("sunos"):
                raise
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'posix_fadvise'),
        "test needs posix.posix_fadvise()")
    def test_posix_fadvise(self):
        fd = os.open(support.TESTFN, os.O_RDONLY)
        try:
            posix.posix_fadvise(fd, 0, 0, posix.POSIX_FADV_WILLNEED)
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'futimes'), "test needs posix.futimes()")
    def test_futimes(self):
        now = time.time()
        fd = os.open(support.TESTFN, os.O_RDONLY)
        try:
            posix.futimes(fd, None)
            self.assertRaises(TypeError, posix.futimes, fd, (None, None))
            self.assertRaises(TypeError, posix.futimes, fd, (now, None))
            self.assertRaises(TypeError, posix.futimes, fd, (None, now))
            posix.futimes(fd, (int(now), int(now)))
            posix.futimes(fd, (now, now))
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'lutimes'), "test needs posix.lutimes()")
    def test_lutimes(self):
        now = time.time()
        posix.lutimes(support.TESTFN, None)
        self.assertRaises(TypeError, posix.lutimes, support.TESTFN, (None, None))
        self.assertRaises(TypeError, posix.lutimes, support.TESTFN, (now, None))
        self.assertRaises(TypeError, posix.lutimes, support.TESTFN, (None, now))
        posix.lutimes(support.TESTFN, (int(now), int(now)))
        posix.lutimes(support.TESTFN, (now, now))

    @unittest.skipUnless(hasattr(posix, 'futimens'), "test needs posix.futimens()")
    def test_futimens(self):
        now = time.time()
        fd = os.open(support.TESTFN, os.O_RDONLY)
        try:
            self.assertRaises(TypeError, posix.futimens, fd, (None, None), (None, None))
            self.assertRaises(TypeError, posix.futimens, fd, (now, 0), None)
            self.assertRaises(TypeError, posix.futimens, fd, None, (now, 0))
            posix.futimens(fd, (int(now), int((now - int(now)) * 1e9)),
                    (int(now), int((now - int(now)) * 1e9)))
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'writev'), "test needs posix.writev()")
    def test_writev(self):
        fd = os.open(support.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            os.writev(fd, (b'test1', b'tt2', b't3'))
            os.lseek(fd, 0, os.SEEK_SET)
            self.assertEqual(b'test1tt2t3', posix.read(fd, 10))
        finally:
            os.close(fd)

    @unittest.skipUnless(hasattr(posix, 'readv'), "test needs posix.readv()")
    def test_readv(self):
        fd = os.open(support.TESTFN, os.O_RDWR | os.O_CREAT)
        try:
            os.write(fd, b'test1tt2t3')
            os.lseek(fd, 0, os.SEEK_SET)
            buf = [bytearray(i) for i in [5, 3, 2]]
            self.assertEqual(posix.readv(fd, buf), 10)
            self.assertEqual([b'test1', b'tt2', b't3'], [bytes(i) for i in buf])
        finally:
            os.close(fd)

    def test_dup(self):
        if hasattr(posix, 'dup'):
            fp = open(support.TESTFN)
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

    @unittest.skipUnless(hasattr(posix, 'mkfifo'), "don't have mkfifo()")
    def test_mkfifo(self):
        support.unlink(support.TESTFN)
        posix.mkfifo(support.TESTFN, stat.S_IRUSR | stat.S_IWUSR)
        self.assertTrue(stat.S_ISFIFO(posix.stat(support.TESTFN).st_mode))

    @unittest.skipUnless(hasattr(posix, 'mknod') and hasattr(stat, 'S_IFIFO'),
                         "don't have mknod()/S_IFIFO")
    def test_mknod(self):
        # Test using mknod() to create a FIFO (the only use specified
        # by POSIX).
        support.unlink(support.TESTFN)
        mode = stat.S_IFIFO | stat.S_IRUSR | stat.S_IWUSR
        try:
            posix.mknod(support.TESTFN, mode, 0)
        except OSError as e:
            # Some old systems don't allow unprivileged users to use
            # mknod(), or only support creating device nodes.
            self.assertIn(e.errno, (errno.EPERM, errno.EINVAL))
        else:
            self.assertTrue(stat.S_ISFIFO(posix.stat(support.TESTFN).st_mode))

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

    def test_listdir(self):
        if hasattr(posix, 'listdir'):
            self.assertTrue(support.TESTFN in posix.listdir(os.curdir))

    def test_listdir_default(self):
        # When listdir is called without argument, it's the same as listdir(os.curdir)
        if hasattr(posix, 'listdir'):
            self.assertTrue(support.TESTFN in posix.listdir())

    @unittest.skipUnless(hasattr(posix, 'fdlistdir'), "test needs posix.fdlistdir()")
    def test_fdlistdir(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        self.assertEqual(
            sorted(posix.listdir('.')),
            sorted(posix.fdlistdir(f))
            )
        # Check the fd was closed by fdlistdir
        with self.assertRaises(OSError) as ctx:
            posix.close(f)
        self.assertEqual(ctx.exception.errno, errno.EBADF)

    def test_access(self):
        if hasattr(posix, 'access'):
            self.assertTrue(posix.access(support.TESTFN, os.R_OK))

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
        if os.name == "nt":
            item_type = str
        else:
            item_type = bytes
        for k, v in posix.environ.items():
            self.assertEqual(type(k), item_type)
            self.assertEqual(type(v), item_type)

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

    # tests for the posix *at functions follow

    @unittest.skipUnless(hasattr(posix, 'faccessat'), "test needs posix.faccessat()")
    def test_faccessat(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            self.assertTrue(posix.faccessat(f, support.TESTFN, os.R_OK))
        finally:
            posix.close(f)

    @unittest.skipUnless(hasattr(posix, 'fchmodat'), "test needs posix.fchmodat()")
    def test_fchmodat(self):
        os.chmod(support.TESTFN, stat.S_IRUSR)

        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.fchmodat(f, support.TESTFN, stat.S_IRUSR | stat.S_IWUSR)

            s = posix.stat(support.TESTFN)
            self.assertEqual(s[0] & stat.S_IRWXU, stat.S_IRUSR | stat.S_IWUSR)
        finally:
            posix.close(f)

    @unittest.skipUnless(hasattr(posix, 'fchownat'), "test needs posix.fchownat()")
    def test_fchownat(self):
        support.unlink(support.TESTFN)
        open(support.TESTFN, 'w').close()

        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.fchownat(f, support.TESTFN, os.getuid(), os.getgid())
        finally:
            posix.close(f)

    @unittest.skipUnless(hasattr(posix, 'fstatat'), "test needs posix.fstatat()")
    def test_fstatat(self):
        support.unlink(support.TESTFN)
        with open(support.TESTFN, 'w') as outfile:
            outfile.write("testline\n")

        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            s1 = posix.stat(support.TESTFN)
            s2 = posix.fstatat(f, support.TESTFN)
            self.assertEqual(s1, s2)
        finally:
            posix.close(f)

    @unittest.skipUnless(hasattr(posix, 'futimesat'), "test needs posix.futimesat()")
    def test_futimesat(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            now = time.time()
            posix.futimesat(f, support.TESTFN, None)
            self.assertRaises(TypeError, posix.futimesat, f, support.TESTFN, (None, None))
            self.assertRaises(TypeError, posix.futimesat, f, support.TESTFN, (now, None))
            self.assertRaises(TypeError, posix.futimesat, f, support.TESTFN, (None, now))
            posix.futimesat(f, support.TESTFN, (int(now), int(now)))
            posix.futimesat(f, support.TESTFN, (now, now))
        finally:
            posix.close(f)

    @unittest.skipUnless(hasattr(posix, 'linkat'), "test needs posix.linkat()")
    def test_linkat(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.linkat(f, support.TESTFN, f, support.TESTFN + 'link')
            # should have same inodes
            self.assertEqual(posix.stat(support.TESTFN)[1],
                posix.stat(support.TESTFN + 'link')[1])
        finally:
            posix.close(f)
            support.unlink(support.TESTFN + 'link')

    @unittest.skipUnless(hasattr(posix, 'mkdirat'), "test needs posix.mkdirat()")
    def test_mkdirat(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.mkdirat(f, support.TESTFN + 'dir')
            posix.stat(support.TESTFN + 'dir') # should not raise exception
        finally:
            posix.close(f)
            support.rmtree(support.TESTFN + 'dir')

    @unittest.skipUnless(hasattr(posix, 'mknodat') and hasattr(stat, 'S_IFIFO'),
                         "don't have mknodat()/S_IFIFO")
    def test_mknodat(self):
        # Test using mknodat() to create a FIFO (the only use specified
        # by POSIX).
        support.unlink(support.TESTFN)
        mode = stat.S_IFIFO | stat.S_IRUSR | stat.S_IWUSR
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.mknodat(f, support.TESTFN, mode, 0)
        except OSError as e:
            # Some old systems don't allow unprivileged users to use
            # mknod(), or only support creating device nodes.
            self.assertIn(e.errno, (errno.EPERM, errno.EINVAL))
        else:
            self.assertTrue(stat.S_ISFIFO(posix.stat(support.TESTFN).st_mode))
        finally:
            posix.close(f)

    @unittest.skipUnless(hasattr(posix, 'openat'), "test needs posix.openat()")
    def test_openat(self):
        support.unlink(support.TESTFN)
        with open(support.TESTFN, 'w') as outfile:
            outfile.write("testline\n")
        a = posix.open(posix.getcwd(), posix.O_RDONLY)
        b = posix.openat(a, support.TESTFN, posix.O_RDONLY)
        try:
            res = posix.read(b, 9).decode(encoding="utf-8")
            self.assertEqual("testline\n", res)
        finally:
            posix.close(a)
            posix.close(b)

    @unittest.skipUnless(hasattr(posix, 'readlinkat'), "test needs posix.readlinkat()")
    def test_readlinkat(self):
        os.symlink(support.TESTFN, support.TESTFN + 'link')
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            self.assertEqual(posix.readlink(support.TESTFN + 'link'),
                posix.readlinkat(f, support.TESTFN + 'link'))
        finally:
            support.unlink(support.TESTFN + 'link')
            posix.close(f)

    @unittest.skipUnless(hasattr(posix, 'renameat'), "test needs posix.renameat()")
    def test_renameat(self):
        support.unlink(support.TESTFN)
        open(support.TESTFN + 'ren', 'w').close()
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.renameat(f, support.TESTFN + 'ren', f, support.TESTFN)
        except:
            posix.rename(support.TESTFN + 'ren', support.TESTFN)
            raise
        else:
            posix.stat(support.TESTFN) # should not throw exception
        finally:
            posix.close(f)

    @unittest.skipUnless(hasattr(posix, 'symlinkat'), "test needs posix.symlinkat()")
    def test_symlinkat(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.symlinkat(support.TESTFN, f, support.TESTFN + 'link')
            self.assertEqual(posix.readlink(support.TESTFN + 'link'), support.TESTFN)
        finally:
            posix.close(f)
            support.unlink(support.TESTFN + 'link')

    @unittest.skipUnless(hasattr(posix, 'unlinkat'), "test needs posix.unlinkat()")
    def test_unlinkat(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        open(support.TESTFN + 'del', 'w').close()
        posix.stat(support.TESTFN + 'del') # should not throw exception
        try:
            posix.unlinkat(f, support.TESTFN + 'del')
        except:
            support.unlink(support.TESTFN + 'del')
            raise
        else:
            self.assertRaises(OSError, posix.stat, support.TESTFN + 'link')
        finally:
            posix.close(f)

    @unittest.skipUnless(hasattr(posix, 'utimensat'), "test needs posix.utimensat()")
    def test_utimensat(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            now = time.time()
            posix.utimensat(f, support.TESTFN, None, None)
            self.assertRaises(TypeError, posix.utimensat, f, support.TESTFN, (None, None), (None, None))
            self.assertRaises(TypeError, posix.utimensat, f, support.TESTFN, (now, 0), None)
            self.assertRaises(TypeError, posix.utimensat, f, support.TESTFN, None, (now, 0))
            posix.utimensat(f, support.TESTFN, (int(now), int((now - int(now)) * 1e9)),
                    (int(now), int((now - int(now)) * 1e9)))
        finally:
            posix.close(f)

    @unittest.skipUnless(hasattr(posix, 'mkfifoat'), "don't have mkfifoat()")
    def test_mkfifoat(self):
        support.unlink(support.TESTFN)
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.mkfifoat(f, support.TESTFN, stat.S_IRUSR | stat.S_IWUSR)
            self.assertTrue(stat.S_ISFIFO(posix.stat(support.TESTFN).st_mode))
        finally:
            posix.close(f)

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
