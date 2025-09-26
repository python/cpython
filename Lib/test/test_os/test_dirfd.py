"""
Tests for the posix *at functions.
"""

import errno
import os
import stat
import time
import unittest
from contextlib import contextmanager
from test import support
from test.support import os_helper

try:
    import posix
except ImportError:
    import nt as posix


class TestPosixDirFd(unittest.TestCase):
    count = 0

    @contextmanager
    def prepare(self):
        TestPosixDirFd.count += 1
        name = f'{os_helper.TESTFN}_{self.count}'
        base_dir = f'{os_helper.TESTFN}_{self.count}base'
        posix.mkdir(base_dir)
        self.addCleanup(posix.rmdir, base_dir)
        fullname = os.path.join(base_dir, name)
        assert not os.path.exists(fullname)
        with os_helper.open_dir_fd(base_dir) as dir_fd:
            yield (dir_fd, name, fullname)

    @contextmanager
    def prepare_file(self):
        with self.prepare() as (dir_fd, name, fullname):
            os_helper.create_empty_file(fullname)
            self.addCleanup(posix.unlink, fullname)
            yield (dir_fd, name, fullname)

    @unittest.skipUnless(os.access in os.supports_dir_fd, "test needs dir_fd support for os.access()")
    def test_access_dir_fd(self):
        with self.prepare_file() as (dir_fd, name, fullname):
            self.assertTrue(posix.access(name, os.R_OK, dir_fd=dir_fd))

    @unittest.skipUnless(os.chmod in os.supports_dir_fd, "test needs dir_fd support in os.chmod()")
    def test_chmod_dir_fd(self):
        with self.prepare_file() as (dir_fd, name, fullname):
            posix.chmod(fullname, stat.S_IRUSR)
            posix.chmod(name, stat.S_IRUSR | stat.S_IWUSR, dir_fd=dir_fd)
            s = posix.stat(fullname)
            self.assertEqual(s.st_mode & stat.S_IRWXU,
                             stat.S_IRUSR | stat.S_IWUSR)

    @unittest.skipUnless(hasattr(os, 'chown') and (os.chown in os.supports_dir_fd),
                         "test needs dir_fd support in os.chown()")
    @unittest.skipIf(support.is_emscripten, "getgid() is a stub")
    def test_chown_dir_fd(self):
        with self.prepare_file() as (dir_fd, name, fullname):
            posix.chown(name, os.getuid(), os.getgid(), dir_fd=dir_fd)

    @unittest.skipUnless(os.stat in os.supports_dir_fd, "test needs dir_fd support in os.stat()")
    def test_stat_dir_fd(self):
        with self.prepare() as (dir_fd, name, fullname):
            with open(fullname, 'w') as outfile:
                outfile.write("testline\n")
            self.addCleanup(posix.unlink, fullname)

            s1 = posix.stat(fullname)
            s2 = posix.stat(name, dir_fd=dir_fd)
            self.assertEqual(s1, s2)
            s2 = posix.stat(fullname, dir_fd=None)
            self.assertEqual(s1, s2)

            self.assertRaisesRegex(TypeError, 'should be integer or None, not',
                    posix.stat, name, dir_fd=posix.getcwd())
            self.assertRaisesRegex(TypeError, 'should be integer or None, not',
                    posix.stat, name, dir_fd=float(dir_fd))
            self.assertRaises(OverflowError,
                    posix.stat, name, dir_fd=10**20)

            for fd in False, True:
                with self.assertWarnsRegex(RuntimeWarning,
                        'bool is used as a file descriptor') as cm:
                    with self.assertRaises(OSError):
                        posix.stat('nonexisting', dir_fd=fd)
                self.assertEqual(cm.filename, __file__)

    @unittest.skipUnless(os.utime in os.supports_dir_fd, "test needs dir_fd support in os.utime()")
    def test_utime_dir_fd(self):
        with self.prepare_file() as (dir_fd, name, fullname):
            now = time.time()
            posix.utime(name, None, dir_fd=dir_fd)
            posix.utime(name, dir_fd=dir_fd)
            self.assertRaises(TypeError, posix.utime, name,
                              now, dir_fd=dir_fd)
            self.assertRaises(TypeError, posix.utime, name,
                              (None, None), dir_fd=dir_fd)
            self.assertRaises(TypeError, posix.utime, name,
                              (now, None), dir_fd=dir_fd)
            self.assertRaises(TypeError, posix.utime, name,
                              (None, now), dir_fd=dir_fd)
            self.assertRaises(TypeError, posix.utime, name,
                              (now, "x"), dir_fd=dir_fd)
            posix.utime(name, (int(now), int(now)), dir_fd=dir_fd)
            posix.utime(name, (now, now), dir_fd=dir_fd)
            posix.utime(name,
                    (int(now), int((now - int(now)) * 1e9)), dir_fd=dir_fd)
            posix.utime(name, dir_fd=dir_fd,
                            times=(int(now), int((now - int(now)) * 1e9)))

            # try dir_fd and follow_symlinks together
            if os.utime in os.supports_follow_symlinks:
                try:
                    posix.utime(name, follow_symlinks=False, dir_fd=dir_fd)
                except ValueError:
                    # whoops!  using both together not supported on this platform.
                    pass

    @unittest.skipIf(
        support.is_wasi,
        "WASI: symlink following on path_link is not supported"
    )
    @unittest.skipUnless(
        hasattr(os, "link") and os.link in os.supports_dir_fd,
        "test needs dir_fd support in os.link()"
    )
    def test_link_dir_fd(self):
        with self.prepare_file() as (dir_fd, name, fullname), \
             self.prepare() as (dir_fd2, linkname, fulllinkname):
            try:
                posix.link(name, linkname, src_dir_fd=dir_fd, dst_dir_fd=dir_fd2)
            except PermissionError as e:
                self.skipTest('posix.link(): %s' % e)
            self.addCleanup(posix.unlink, fulllinkname)
            # should have same inodes
            self.assertEqual(posix.stat(fullname)[1],
                posix.stat(fulllinkname)[1])

    @unittest.skipUnless(os.mkdir in os.supports_dir_fd, "test needs dir_fd support in os.mkdir()")
    def test_mkdir_dir_fd(self):
        with self.prepare() as (dir_fd, name, fullname):
            posix.mkdir(name, dir_fd=dir_fd)
            self.addCleanup(posix.rmdir, fullname)
            posix.stat(fullname) # should not raise exception

    @unittest.skipUnless(hasattr(os, 'mknod')
                         and (os.mknod in os.supports_dir_fd)
                         and hasattr(stat, 'S_IFIFO'),
                         "test requires both stat.S_IFIFO and dir_fd support for os.mknod()")
    def test_mknod_dir_fd(self):
        # Test using mknodat() to create a FIFO (the only use specified
        # by POSIX).
        with self.prepare() as (dir_fd, name, fullname):
            mode = stat.S_IFIFO | stat.S_IRUSR | stat.S_IWUSR
            try:
                posix.mknod(name, mode, 0, dir_fd=dir_fd)
            except OSError as e:
                # Some old systems don't allow unprivileged users to use
                # mknod(), or only support creating device nodes.
                self.assertIn(e.errno, (errno.EPERM, errno.EINVAL, errno.EACCES))
            else:
                self.addCleanup(posix.unlink, fullname)
                self.assertTrue(stat.S_ISFIFO(posix.stat(fullname).st_mode))

    @unittest.skipUnless(os.open in os.supports_dir_fd, "test needs dir_fd support in os.open()")
    def test_open_dir_fd(self):
        with self.prepare() as (dir_fd, name, fullname):
            with open(fullname, 'wb') as outfile:
                outfile.write(b"testline\n")
            self.addCleanup(posix.unlink, fullname)
            fd = posix.open(name, posix.O_RDONLY, dir_fd=dir_fd)
            try:
                res = posix.read(fd, 9)
                self.assertEqual(b"testline\n", res)
            finally:
                posix.close(fd)

    @unittest.skipUnless(hasattr(os, 'readlink') and (os.readlink in os.supports_dir_fd),
                         "test needs dir_fd support in os.readlink()")
    def test_readlink_dir_fd(self):
        with self.prepare() as (dir_fd, name, fullname):
            os.symlink('symlink', fullname)
            self.addCleanup(posix.unlink, fullname)
            self.assertEqual(posix.readlink(name, dir_fd=dir_fd), 'symlink')

    @unittest.skipUnless(os.rename in os.supports_dir_fd, "test needs dir_fd support in os.rename()")
    def test_rename_dir_fd(self):
        with self.prepare_file() as (dir_fd, name, fullname), \
             self.prepare() as (dir_fd2, name2, fullname2):
            posix.rename(name, name2,
                         src_dir_fd=dir_fd, dst_dir_fd=dir_fd2)
            posix.stat(fullname2) # should not raise exception
            posix.rename(fullname2, fullname)

    @unittest.skipUnless(os.symlink in os.supports_dir_fd, "test needs dir_fd support in os.symlink()")
    def test_symlink_dir_fd(self):
        with self.prepare() as (dir_fd, name, fullname):
            posix.symlink('symlink', name, dir_fd=dir_fd)
            self.addCleanup(posix.unlink, fullname)
            self.assertEqual(posix.readlink(fullname), 'symlink')

    @unittest.skipUnless(os.unlink in os.supports_dir_fd, "test needs dir_fd support in os.unlink()")
    def test_unlink_dir_fd(self):
        with self.prepare() as (dir_fd, name, fullname):
            os_helper.create_empty_file(fullname)
            posix.stat(fullname) # should not raise exception
            try:
                posix.unlink(name, dir_fd=dir_fd)
                self.assertRaises(OSError, posix.stat, fullname)
            except:
                self.addCleanup(posix.unlink, fullname)
                raise

    @unittest.skipUnless(hasattr(os, 'mkfifo') and os.mkfifo in os.supports_dir_fd, "test needs dir_fd support in os.mkfifo()")
    def test_mkfifo_dir_fd(self):
        with self.prepare() as (dir_fd, name, fullname):
            try:
                posix.mkfifo(name, stat.S_IRUSR | stat.S_IWUSR, dir_fd=dir_fd)
            except PermissionError as e:
                self.skipTest('posix.mkfifo(): %s' % e)
            self.addCleanup(posix.unlink, fullname)
            self.assertTrue(stat.S_ISFIFO(posix.stat(fullname).st_mode))


if __name__ == '__main__':
    unittest.main()
