"Test posix functions"

from test import support

# Skip these tests if there is no posix module.
posix = support.import_module('posix')

import errno
import sys
import time
import os
import fcntl
import platform
import pwd
import shutil
import stat
import tempfile
import unittest
import warnings

_DUMMY_SYMLINK = os.path.join(tempfile.gettempdir(),
                              support.TESTFN + '-dummy-symlink')

class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        fp = open(support.TESTFN, 'w+')
        fp.close()
        self.teardown_files = [ support.TESTFN ]
        self._warnings_manager = support.check_warnings()
        self._warnings_manager.__enter__()
        warnings.filterwarnings('ignore', '.* potential security risk .*',
                                RuntimeWarning)

    def tearDown(self):
        for teardown_file in self.teardown_files:
            support.unlink(teardown_file)
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

    def test_statvfs(self):
        if hasattr(posix, 'statvfs'):
            self.assertTrue(posix.statvfs(os.curdir))

    def test_fstatvfs(self):
        if hasattr(posix, 'fstatvfs'):
            fp = open(support.TESTFN)
            try:
                self.assertTrue(posix.fstatvfs(fp.fileno()))
                self.assertTrue(posix.statvfs(fp.fileno()))
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

    @unittest.skipUnless(getattr(os, 'execve', None) in os.supports_fd, "test needs execve() to support the fd parameter")
    @unittest.skipUnless(hasattr(os, 'fork'), "test needs os.fork()")
    @unittest.skipUnless(hasattr(os, 'waitpid'), "test needs os.waitpid()")
    def test_fexecve(self):
        fp = os.open(sys.executable, os.O_RDONLY)
        try:
            pid = os.fork()
            if pid == 0:
                os.chdir(os.path.split(sys.executable)[0])
                posix.execve(fp, [sys.executable, '-c', 'pass'], os.environ)
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
            # the first pread() shouldn't disturb the file offset
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

    @unittest.skipUnless(os.utime in os.supports_fd, "test needs fd support in os.utime")
    def test_utime_with_fd(self):
        now = time.time()
        fd = os.open(support.TESTFN, os.O_RDONLY)
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
        posix.utime(support.TESTFN, None, follow_symlinks=False)
        self.assertRaises(TypeError, posix.utime, support.TESTFN, (None, None), follow_symlinks=False)
        self.assertRaises(TypeError, posix.utime, support.TESTFN, (now, None), follow_symlinks=False)
        self.assertRaises(TypeError, posix.utime, support.TESTFN, (None, now), follow_symlinks=False)
        posix.utime(support.TESTFN, (int(now), int(now)), follow_symlinks=False)
        posix.utime(support.TESTFN, (now, now), follow_symlinks=False)
        posix.utime(support.TESTFN, follow_symlinks=False)

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

    @unittest.skipUnless(hasattr(os, 'O_CLOEXEC'), "needs os.O_CLOEXEC")
    @support.requires_linux_version(2, 6, 23)
    def test_oscloexec(self):
        fd = os.open(support.TESTFN, os.O_RDONLY|os.O_CLOEXEC)
        self.addCleanup(os.close, fd)
        self.assertTrue(fcntl.fcntl(fd, fcntl.F_GETFD) & fcntl.FD_CLOEXEC)

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
                self.assertTrue(posix.stat(fp.fileno()))
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
        # test a successful chown call
        chown_func(first_param, os.getuid(), os.getgid())

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
        elif platform.system() in ('HP-UX', 'SunOS'):
            # HP-UX and Solaris can allow a non-root user to chown() to root
            # (issue #5113)
            raise unittest.SkipTest("Skipping because of non-standard chown() "
                                    "behavior")
        else:
            # non-root cannot chown to root, raises OSError
            self.assertRaises(OSError, chown_func,
                              first_param, 0, 0)

    @unittest.skipUnless(hasattr(posix, 'chown'), "test needs os.chown()")
    def test_chown(self):
        # raise an OSError if the file does not exist
        os.unlink(support.TESTFN)
        self.assertRaises(OSError, posix.chown, support.TESTFN, -1, -1)

        # re-create the file
        support.create_empty_file(support.TESTFN)
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
        os.symlink(_DUMMY_SYMLINK, support.TESTFN)
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

    @unittest.skipUnless(os.listdir in os.supports_fd, "test needs fd support for os.listdir()")
    def test_flistdir(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        self.addCleanup(posix.close, f)
        self.assertEqual(
            sorted(posix.listdir('.')),
            sorted(posix.listdir(f))
            )
        # Check that the fd offset was reset (issue #13739)
        self.assertEqual(
            sorted(posix.listdir('.')),
            sorted(posix.listdir(f))
            )

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

    @unittest.skipUnless(hasattr(os, 'pipe2'), "test needs os.pipe2()")
    @support.requires_linux_version(2, 6, 27)
    def test_pipe2(self):
        self.assertRaises(TypeError, os.pipe2, 'DEADBEEF')
        self.assertRaises(TypeError, os.pipe2, 0, 0)

        # try calling with flags = 0, like os.pipe()
        r, w = os.pipe2(0)
        os.close(r)
        os.close(w)

        # test flags
        r, w = os.pipe2(os.O_CLOEXEC|os.O_NONBLOCK)
        self.addCleanup(os.close, r)
        self.addCleanup(os.close, w)
        self.assertTrue(fcntl.fcntl(r, fcntl.F_GETFD) & fcntl.FD_CLOEXEC)
        self.assertTrue(fcntl.fcntl(w, fcntl.F_GETFD) & fcntl.FD_CLOEXEC)
        # try reading from an empty pipe: this should fail, not block
        self.assertRaises(OSError, os.read, r, 1)
        # try a write big enough to fill-up the pipe: this should either
        # fail or perform a partial write, not block
        try:
            os.write(w, b'x' * support.PIPE_MAX_SIZE)
        except OSError:
            pass

    def test_utime(self):
        if hasattr(posix, 'utime'):
            now = time.time()
            posix.utime(support.TESTFN, None)
            self.assertRaises(TypeError, posix.utime, support.TESTFN, (None, None))
            self.assertRaises(TypeError, posix.utime, support.TESTFN, (now, None))
            self.assertRaises(TypeError, posix.utime, support.TESTFN, (None, now))
            posix.utime(support.TESTFN, (int(now), int(now)))
            posix.utime(support.TESTFN, (now, now))

    def _test_chflags_regular_file(self, chflags_func, target_file, **kwargs):
        st = os.stat(target_file)
        self.assertTrue(hasattr(st, 'st_flags'))
        chflags_func(target_file, st.st_flags | stat.UF_IMMUTABLE, **kwargs)
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
        self._test_chflags_regular_file(posix.chflags, support.TESTFN)

    @unittest.skipUnless(hasattr(posix, 'lchflags'), 'test needs os.lchflags()')
    def test_lchflags_regular_file(self):
        self._test_chflags_regular_file(posix.lchflags, support.TESTFN)
        self._test_chflags_regular_file(posix.chflags, support.TESTFN, follow_symlinks=False)

    @unittest.skipUnless(hasattr(posix, 'lchflags'), 'test needs os.lchflags()')
    def test_lchflags_symlink(self):
        testfn_st = os.stat(support.TESTFN)

        self.assertTrue(hasattr(testfn_st, 'st_flags'))

        os.symlink(support.TESTFN, _DUMMY_SYMLINK)
        self.teardown_files.append(_DUMMY_SYMLINK)
        dummy_symlink_st = os.lstat(_DUMMY_SYMLINK)

        def chflags_nofollow(path, flags):
            return posix.chflags(path, flags, follow_symlinks=False)

        for fn in (posix.lchflags, chflags_nofollow):
            fn(_DUMMY_SYMLINK,
                           dummy_symlink_st.st_flags | stat.UF_IMMUTABLE)
            try:
                new_testfn_st = os.stat(support.TESTFN)
                new_dummy_symlink_st = os.lstat(_DUMMY_SYMLINK)

                self.assertEqual(testfn_st.st_flags, new_testfn_st.st_flags)
                self.assertEqual(dummy_symlink_st.st_flags | stat.UF_IMMUTABLE,
                                 new_dummy_symlink_st.st_flags)
            finally:
                fn(_DUMMY_SYMLINK, dummy_symlink_st.st_flags)

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

    @unittest.skipUnless(hasattr(posix, 'getgrouplist'), "test needs posix.getgrouplist()")
    @unittest.skipUnless(hasattr(pwd, 'getpwuid'), "test needs pwd.getpwuid()")
    @unittest.skipUnless(hasattr(os, 'getuid'), "test needs os.getuid()")
    def test_getgrouplist(self):
        with os.popen('id -G') as idg:
            groups = idg.read().strip()
            ret = idg.close()

        if ret != None or not groups:
            raise unittest.SkipTest("need working 'id -G'")

        self.assertEqual(
            set([int(x) for x in groups.split()]),
            set(posix.getgrouplist(pwd.getpwuid(os.getuid())[0],
                pwd.getpwuid(os.getuid())[3])))

    @unittest.skipUnless(hasattr(os, 'getegid'), "test needs os.getegid()")
    def test_getgroups(self):
        with os.popen('id -G') as idg:
            groups = idg.read().strip()
            ret = idg.close()

        if ret != None or not groups:
            raise unittest.SkipTest("need working 'id -G'")

        # 'id -G' and 'os.getgroups()' should return the same
        # groups, ignoring order and duplicates.
        # #10822 - it is implementation defined whether posix.getgroups()
        # includes the effective gid so we include it anyway, since id -G does
        self.assertEqual(
                set([int(x) for x in groups.split()]),
                set(posix.getgroups() + [posix.getegid()]))

    # tests for the posix *at functions follow

    @unittest.skipUnless(os.access in os.supports_dir_fd, "test needs dir_fd support for os.access()")
    def test_access_dir_fd(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            self.assertTrue(posix.access(support.TESTFN, os.R_OK, dir_fd=f))
        finally:
            posix.close(f)

    @unittest.skipUnless(os.chmod in os.supports_dir_fd, "test needs dir_fd support in os.chmod()")
    def test_chmod_dir_fd(self):
        os.chmod(support.TESTFN, stat.S_IRUSR)

        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.chmod(support.TESTFN, stat.S_IRUSR | stat.S_IWUSR, dir_fd=f)

            s = posix.stat(support.TESTFN)
            self.assertEqual(s[0] & stat.S_IRWXU, stat.S_IRUSR | stat.S_IWUSR)
        finally:
            posix.close(f)

    @unittest.skipUnless(os.chown in os.supports_dir_fd, "test needs dir_fd support in os.chown()")
    def test_chown_dir_fd(self):
        support.unlink(support.TESTFN)
        support.create_empty_file(support.TESTFN)

        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.chown(support.TESTFN, os.getuid(), os.getgid(), dir_fd=f)
        finally:
            posix.close(f)

    @unittest.skipUnless(os.stat in os.supports_dir_fd, "test needs dir_fd support in os.stat()")
    def test_stat_dir_fd(self):
        support.unlink(support.TESTFN)
        with open(support.TESTFN, 'w') as outfile:
            outfile.write("testline\n")

        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            s1 = posix.stat(support.TESTFN)
            s2 = posix.stat(support.TESTFN, dir_fd=f)
            self.assertEqual(s1, s2)
        finally:
            posix.close(f)

    @unittest.skipUnless(os.utime in os.supports_dir_fd, "test needs dir_fd support in os.utime()")
    def test_utime_dir_fd(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            now = time.time()
            posix.utime(support.TESTFN, None, dir_fd=f)
            posix.utime(support.TESTFN, dir_fd=f)
            self.assertRaises(TypeError, posix.utime, support.TESTFN, now, dir_fd=f)
            self.assertRaises(TypeError, posix.utime, support.TESTFN, (None, None), dir_fd=f)
            self.assertRaises(TypeError, posix.utime, support.TESTFN, (now, None), dir_fd=f)
            self.assertRaises(TypeError, posix.utime, support.TESTFN, (None, now), dir_fd=f)
            self.assertRaises(TypeError, posix.utime, support.TESTFN, (now, "x"), dir_fd=f)
            posix.utime(support.TESTFN, (int(now), int(now)), dir_fd=f)
            posix.utime(support.TESTFN, (now, now), dir_fd=f)
            posix.utime(support.TESTFN,
                    (int(now), int((now - int(now)) * 1e9)), dir_fd=f)
            posix.utime(support.TESTFN, dir_fd=f,
                            times=(int(now), int((now - int(now)) * 1e9)))

            # try dir_fd and follow_symlinks together
            if os.utime in os.supports_follow_symlinks:
                try:
                    posix.utime(support.TESTFN, follow_symlinks=False, dir_fd=f)
                except RuntimeError:
                    # whoops!  using both together not supported on this platform.
                    pass

        finally:
            posix.close(f)

    @unittest.skipUnless(os.link in os.supports_dir_fd, "test needs dir_fd support in os.link()")
    def test_link_dir_fd(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.link(support.TESTFN, support.TESTFN + 'link', src_dir_fd=f, dst_dir_fd=f)
            # should have same inodes
            self.assertEqual(posix.stat(support.TESTFN)[1],
                posix.stat(support.TESTFN + 'link')[1])
        finally:
            posix.close(f)
            support.unlink(support.TESTFN + 'link')

    @unittest.skipUnless(os.mkdir in os.supports_dir_fd, "test needs dir_fd support in os.mkdir()")
    def test_mkdir_dir_fd(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.mkdir(support.TESTFN + 'dir', dir_fd=f)
            posix.stat(support.TESTFN + 'dir') # should not raise exception
        finally:
            posix.close(f)
            support.rmtree(support.TESTFN + 'dir')

    @unittest.skipUnless((os.mknod in os.supports_dir_fd) and hasattr(stat, 'S_IFIFO'),
                         "test requires both stat.S_IFIFO and dir_fd support for os.mknod()")
    def test_mknod_dir_fd(self):
        # Test using mknodat() to create a FIFO (the only use specified
        # by POSIX).
        support.unlink(support.TESTFN)
        mode = stat.S_IFIFO | stat.S_IRUSR | stat.S_IWUSR
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.mknod(support.TESTFN, mode, 0, dir_fd=f)
        except OSError as e:
            # Some old systems don't allow unprivileged users to use
            # mknod(), or only support creating device nodes.
            self.assertIn(e.errno, (errno.EPERM, errno.EINVAL))
        else:
            self.assertTrue(stat.S_ISFIFO(posix.stat(support.TESTFN).st_mode))
        finally:
            posix.close(f)

    @unittest.skipUnless(os.open in os.supports_dir_fd, "test needs dir_fd support in os.open()")
    def test_open_dir_fd(self):
        support.unlink(support.TESTFN)
        with open(support.TESTFN, 'w') as outfile:
            outfile.write("testline\n")
        a = posix.open(posix.getcwd(), posix.O_RDONLY)
        b = posix.open(support.TESTFN, posix.O_RDONLY, dir_fd=a)
        try:
            res = posix.read(b, 9).decode(encoding="utf-8")
            self.assertEqual("testline\n", res)
        finally:
            posix.close(a)
            posix.close(b)

    @unittest.skipUnless(os.readlink in os.supports_dir_fd, "test needs dir_fd support in os.readlink()")
    def test_readlink_dir_fd(self):
        os.symlink(support.TESTFN, support.TESTFN + 'link')
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            self.assertEqual(posix.readlink(support.TESTFN + 'link'),
                posix.readlink(support.TESTFN + 'link', dir_fd=f))
        finally:
            support.unlink(support.TESTFN + 'link')
            posix.close(f)

    @unittest.skipUnless(os.rename in os.supports_dir_fd, "test needs dir_fd support in os.rename()")
    def test_rename_dir_fd(self):
        support.unlink(support.TESTFN)
        support.create_empty_file(support.TESTFN + 'ren')
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.rename(support.TESTFN + 'ren', support.TESTFN, src_dir_fd=f, dst_dir_fd=f)
        except:
            posix.rename(support.TESTFN + 'ren', support.TESTFN)
            raise
        else:
            posix.stat(support.TESTFN) # should not throw exception
        finally:
            posix.close(f)

    @unittest.skipUnless(os.symlink in os.supports_dir_fd, "test needs dir_fd support in os.symlink()")
    def test_symlink_dir_fd(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.symlink(support.TESTFN, support.TESTFN + 'link', dir_fd=f)
            self.assertEqual(posix.readlink(support.TESTFN + 'link'), support.TESTFN)
        finally:
            posix.close(f)
            support.unlink(support.TESTFN + 'link')

    @unittest.skipUnless(os.unlink in os.supports_dir_fd, "test needs dir_fd support in os.unlink()")
    def test_unlink_dir_fd(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        support.create_empty_file(support.TESTFN + 'del')
        posix.stat(support.TESTFN + 'del') # should not throw exception
        try:
            posix.unlink(support.TESTFN + 'del', dir_fd=f)
        except:
            support.unlink(support.TESTFN + 'del')
            raise
        else:
            self.assertRaises(OSError, posix.stat, support.TESTFN + 'link')
        finally:
            posix.close(f)

    @unittest.skipUnless(os.mkfifo in os.supports_dir_fd, "test needs dir_fd support in os.mkfifo()")
    def test_mkfifo_dir_fd(self):
        support.unlink(support.TESTFN)
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        try:
            posix.mkfifo(support.TESTFN, stat.S_IRUSR | stat.S_IWUSR, dir_fd=f)
            self.assertTrue(stat.S_ISFIFO(posix.stat(support.TESTFN).st_mode))
        finally:
            posix.close(f)

    requires_sched_h = unittest.skipUnless(hasattr(posix, 'sched_yield'),
                                           "don't have scheduling support")
    requires_sched_affinity = unittest.skipUnless(hasattr(posix, 'cpu_set'),
                                                  "don't have sched affinity support")

    @requires_sched_h
    def test_sched_yield(self):
        # This has no error conditions (at least on Linux).
        posix.sched_yield()

    @requires_sched_h
    @unittest.skipUnless(hasattr(posix, 'sched_get_priority_max'),
                         "requires sched_get_priority_max()")
    def test_sched_priority(self):
        # Round-robin usually has interesting priorities.
        pol = posix.SCHED_RR
        lo = posix.sched_get_priority_min(pol)
        hi = posix.sched_get_priority_max(pol)
        self.assertIsInstance(lo, int)
        self.assertIsInstance(hi, int)
        self.assertGreaterEqual(hi, lo)
        # OSX evidently just returns 15 without checking the argument.
        if sys.platform != "darwin":
            self.assertRaises(OSError, posix.sched_get_priority_min, -23)
            self.assertRaises(OSError, posix.sched_get_priority_max, -23)

    @unittest.skipUnless(hasattr(posix, 'sched_setscheduler'), "can't change scheduler")
    def test_get_and_set_scheduler_and_param(self):
        possible_schedulers = [sched for name, sched in posix.__dict__.items()
                               if name.startswith("SCHED_")]
        mine = posix.sched_getscheduler(0)
        self.assertIn(mine, possible_schedulers)
        try:
            parent = posix.sched_getscheduler(os.getppid())
        except OSError as e:
            if e.errno != errno.EPERM:
                raise
        else:
            self.assertIn(parent, possible_schedulers)
        self.assertRaises(OSError, posix.sched_getscheduler, -1)
        self.assertRaises(OSError, posix.sched_getparam, -1)
        param = posix.sched_getparam(0)
        self.assertIsInstance(param.sched_priority, int)
        try:
            posix.sched_setscheduler(0, mine, param)
        except OSError as e:
            if e.errno != errno.EPERM:
                raise

        # POSIX states that calling sched_setparam() on a process with a
        # scheduling policy other than SCHED_FIFO or SCHED_RR is
        # implementation-defined: FreeBSD returns EINVAL.
        if not sys.platform.startswith('freebsd'):
            posix.sched_setparam(0, param)
            self.assertRaises(OSError, posix.sched_setparam, -1, param)

        self.assertRaises(OSError, posix.sched_setscheduler, -1, mine, param)
        self.assertRaises(TypeError, posix.sched_setscheduler, 0, mine, None)
        self.assertRaises(TypeError, posix.sched_setparam, 0, 43)
        param = posix.sched_param(None)
        self.assertRaises(TypeError, posix.sched_setparam, 0, param)
        large = 214748364700
        param = posix.sched_param(large)
        self.assertRaises(OverflowError, posix.sched_setparam, 0, param)
        param = posix.sched_param(sched_priority=-large)
        self.assertRaises(OverflowError, posix.sched_setparam, 0, param)

    @unittest.skipUnless(hasattr(posix, "sched_rr_get_interval"), "no function")
    def test_sched_rr_get_interval(self):
        try:
            interval = posix.sched_rr_get_interval(0)
        except OSError as e:
            # This likely means that sched_rr_get_interval is only valid for
            # processes with the SCHED_RR scheduler in effect.
            if e.errno != errno.EINVAL:
                raise
            self.skipTest("only works on SCHED_RR processes")
        self.assertIsInstance(interval, float)
        # Reasonable constraints, I think.
        self.assertGreaterEqual(interval, 0.)
        self.assertLess(interval, 1.)

    @requires_sched_affinity
    def test_sched_affinity(self):
        mask = posix.sched_getaffinity(0, 1024)
        self.assertGreaterEqual(mask.count(), 1)
        self.assertIsInstance(mask, posix.cpu_set)
        self.assertRaises(OSError, posix.sched_getaffinity, -1, 1024)
        empty = posix.cpu_set(10)
        posix.sched_setaffinity(0, mask)
        self.assertRaises(OSError, posix.sched_setaffinity, 0, empty)
        self.assertRaises(OSError, posix.sched_setaffinity, -1, mask)

    @requires_sched_affinity
    def test_cpu_set_basic(self):
        s = posix.cpu_set(10)
        self.assertEqual(len(s), 10)
        self.assertEqual(s.count(), 0)
        s.set(0)
        s.set(9)
        self.assertTrue(s.isset(0))
        self.assertTrue(s.isset(9))
        self.assertFalse(s.isset(5))
        self.assertEqual(s.count(), 2)
        s.clear(0)
        self.assertFalse(s.isset(0))
        self.assertEqual(s.count(), 1)
        s.zero()
        self.assertFalse(s.isset(0))
        self.assertFalse(s.isset(9))
        self.assertEqual(s.count(), 0)
        self.assertRaises(ValueError, s.set, -1)
        self.assertRaises(ValueError, s.set, 10)
        self.assertRaises(ValueError, s.clear, -1)
        self.assertRaises(ValueError, s.clear, 10)
        self.assertRaises(ValueError, s.isset, -1)
        self.assertRaises(ValueError, s.isset, 10)

    @requires_sched_affinity
    def test_cpu_set_cmp(self):
        self.assertNotEqual(posix.cpu_set(11), posix.cpu_set(12))
        l = posix.cpu_set(10)
        r = posix.cpu_set(10)
        self.assertEqual(l, r)
        l.set(1)
        self.assertNotEqual(l, r)
        r.set(1)
        self.assertEqual(l, r)

    @requires_sched_affinity
    def test_cpu_set_bitwise(self):
        l = posix.cpu_set(5)
        l.set(0)
        l.set(1)
        r = posix.cpu_set(5)
        r.set(1)
        r.set(2)
        b = l & r
        self.assertEqual(b.count(), 1)
        self.assertTrue(b.isset(1))
        b = l | r
        self.assertEqual(b.count(), 3)
        self.assertTrue(b.isset(0))
        self.assertTrue(b.isset(1))
        self.assertTrue(b.isset(2))
        b = l ^ r
        self.assertEqual(b.count(), 2)
        self.assertTrue(b.isset(0))
        self.assertFalse(b.isset(1))
        self.assertTrue(b.isset(2))
        b = l
        b |= r
        self.assertIs(b, l)
        self.assertEqual(l.count(), 3)

    def test_rtld_constants(self):
        # check presence of major RTLD_* constants
        posix.RTLD_LAZY
        posix.RTLD_NOW
        posix.RTLD_GLOBAL
        posix.RTLD_LOCAL

    @unittest.skipIf(sys.platform == 'freebsd9',
            "Skip test because known kernel bug - " \
            "http://lists.freebsd.org/pipermail/freebsd-amd64/2012-January/014332.html")
    @unittest.skipUnless(hasattr(os, 'SEEK_HOLE'),
                         "test needs an OS that reports file holes")
    def test_fs_holes(self) :
        # Even if the filesystem doesn't report holes,
        # if the OS supports it the SEEK_* constants
        # will be defined and will have a consistent
        # behaviour:
        # os.SEEK_DATA = current position
        # os.SEEK_HOLE = end of file position
        with open(support.TESTFN, 'r+b') as fp :
            fp.write(b"hello")
            fp.flush()
            size = fp.tell()
            fno = fp.fileno()
            for i in range(size) :
                self.assertEqual(i, os.lseek(fno, i, os.SEEK_DATA))
                self.assertLessEqual(size, os.lseek(fno, i, os.SEEK_HOLE))
            self.assertRaises(OSError, os.lseek, fno, size, os.SEEK_DATA)
            self.assertRaises(OSError, os.lseek, fno, size, os.SEEK_HOLE)

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
    try:
        support.run_unittest(PosixTester, PosixGroupsTester)
    finally:
        support.reap_children()

if __name__ == '__main__':
    test_main()
