# Copyright (C) 2003 Python Software Foundation

import unittest
import shutil
import tempfile
import sys
import stat
import os
import os.path
import errno
import functools
import subprocess
from test import support
from test.support import TESTFN
from os.path import splitdrive
from distutils.spawn import find_executable, spawn
from shutil import (_make_tarball, _make_zipfile, make_archive,
                    register_archive_format, unregister_archive_format,
                    get_archive_formats, Error, unpack_archive,
                    register_unpack_format, RegistryError,
                    unregister_unpack_format, get_unpack_formats)
import tarfile
import warnings

from test import support
from test.support import TESTFN, check_warnings, captured_stdout, requires_zlib

try:
    import bz2
    BZ2_SUPPORTED = True
except ImportError:
    BZ2_SUPPORTED = False

TESTFN2 = TESTFN + "2"

try:
    import grp
    import pwd
    UID_GID_SUPPORT = True
except ImportError:
    UID_GID_SUPPORT = False

try:
    import zipfile
    ZIP_SUPPORT = True
except ImportError:
    ZIP_SUPPORT = find_executable('zip')

def _fake_rename(*args, **kwargs):
    # Pretend the destination path is on a different filesystem.
    raise OSError(getattr(errno, 'EXDEV', 18), "Invalid cross-device link")

def mock_rename(func):
    @functools.wraps(func)
    def wrap(*args, **kwargs):
        try:
            builtin_rename = os.rename
            os.rename = _fake_rename
            return func(*args, **kwargs)
        finally:
            os.rename = builtin_rename
    return wrap

def write_file(path, content, binary=False):
    """Write *content* to a file located at *path*.

    If *path* is a tuple instead of a string, os.path.join will be used to
    make a path.  If *binary* is true, the file will be opened in binary
    mode.
    """
    if isinstance(path, tuple):
        path = os.path.join(*path)
    with open(path, 'wb' if binary else 'w') as fp:
        fp.write(content)

def read_file(path, binary=False):
    """Return contents from a file located at *path*.

    If *path* is a tuple instead of a string, os.path.join will be used to
    make a path.  If *binary* is true, the file will be opened in binary
    mode.
    """
    if isinstance(path, tuple):
        path = os.path.join(*path)
    with open(path, 'rb' if binary else 'r') as fp:
        return fp.read()


class TestShutil(unittest.TestCase):

    def setUp(self):
        super(TestShutil, self).setUp()
        self.tempdirs = []

    def tearDown(self):
        super(TestShutil, self).tearDown()
        while self.tempdirs:
            d = self.tempdirs.pop()
            shutil.rmtree(d, os.name in ('nt', 'cygwin'))


    def mkdtemp(self):
        """Create a temporary directory that will be cleaned up.

        Returns the path of the directory.
        """
        d = tempfile.mkdtemp()
        self.tempdirs.append(d)
        return d

    def test_rmtree_errors(self):
        # filename is guaranteed not to exist
        filename = tempfile.mktemp()
        self.assertRaises(OSError, shutil.rmtree, filename)

    # See bug #1071513 for why we don't run this on cygwin
    # and bug #1076467 for why we don't run this as root.
    if (hasattr(os, 'chmod') and sys.platform[:6] != 'cygwin'
        and not (hasattr(os, 'geteuid') and os.geteuid() == 0)):
        def test_on_error(self):
            self.errorState = 0
            os.mkdir(TESTFN)
            self.addCleanup(shutil.rmtree, TESTFN)

            self.child_file_path = os.path.join(TESTFN, 'a')
            self.child_dir_path = os.path.join(TESTFN, 'b')
            support.create_empty_file(self.child_file_path)
            os.mkdir(self.child_dir_path)
            old_dir_mode = os.stat(TESTFN).st_mode
            old_child_file_mode = os.stat(self.child_file_path).st_mode
            old_child_dir_mode = os.stat(self.child_dir_path).st_mode
            # Make unwritable.
            new_mode = stat.S_IREAD|stat.S_IEXEC
            os.chmod(self.child_file_path, new_mode)
            os.chmod(self.child_dir_path, new_mode)
            os.chmod(TESTFN, new_mode)

            self.addCleanup(os.chmod, TESTFN, old_dir_mode)
            self.addCleanup(os.chmod, self.child_file_path, old_child_file_mode)
            self.addCleanup(os.chmod, self.child_dir_path, old_child_dir_mode)

            shutil.rmtree(TESTFN, onerror=self.check_args_to_onerror)
            # Test whether onerror has actually been called.
            self.assertEqual(self.errorState, 3,
                             "Expected call to onerror function did not "
                             "happen.")

    def check_args_to_onerror(self, func, arg, exc):
        # test_rmtree_errors deliberately runs rmtree
        # on a directory that is chmod 500, which will fail.
        # This function is run when shutil.rmtree fails.
        # 99.9% of the time it initially fails to remove
        # a file in the directory, so the first time through
        # func is os.remove.
        # However, some Linux machines running ZFS on
        # FUSE experienced a failure earlier in the process
        # at os.listdir.  The first failure may legally
        # be either.
        if support.verbose:
            print("onerror [%d]: %r" % (self.errorState, (func, arg, exc[1])))
        if 0 <= self.errorState < 2:
            if func is os.unlink:
                self.assertIn(arg, [self.child_file_path, self.child_dir_path])
            else:
                if self.errorState == 1:
                    self.assertEqual(func, os.rmdir)
                else:
                    self.assertIs(func, os.listdir, "func must be os.listdir")
                self.assertIn(arg, [TESTFN, self.child_dir_path])
            self.assertTrue(issubclass(exc[0], OSError))
            self.errorState += 1
        else:
            self.assertEqual(func, os.rmdir)
            self.assertEqual(arg, TESTFN)
            self.assertTrue(issubclass(exc[0], OSError))
            self.errorState = 3

    def test_rmtree_does_not_choke_on_failing_lstat(self):
        try:
            orig_lstat = os.lstat
            def raiser(fn):
                if fn != TESTFN:
                    raise OSError()
                else:
                    return orig_lstat(fn)
            os.lstat = raiser

            os.mkdir(TESTFN)
            write_file((TESTFN, 'foo'), 'foo')
            shutil.rmtree(TESTFN)
        finally:
            os.lstat = orig_lstat

    @unittest.skipUnless(hasattr(os, 'chmod'), 'requires os.chmod')
    @support.skip_unless_symlink
    def test_copymode_follow_symlinks(self):
        tmp_dir = self.mkdtemp()
        src = os.path.join(tmp_dir, 'foo')
        dst = os.path.join(tmp_dir, 'bar')
        src_link = os.path.join(tmp_dir, 'baz')
        dst_link = os.path.join(tmp_dir, 'quux')
        write_file(src, 'foo')
        write_file(dst, 'foo')
        os.symlink(src, src_link)
        os.symlink(dst, dst_link)
        os.chmod(src, stat.S_IRWXU|stat.S_IRWXG)
        # file to file
        os.chmod(dst, stat.S_IRWXO)
        self.assertNotEqual(os.stat(src).st_mode, os.stat(dst).st_mode)
        shutil.copymode(src, dst)
        self.assertEqual(os.stat(src).st_mode, os.stat(dst).st_mode)
        # follow src link
        os.chmod(dst, stat.S_IRWXO)
        shutil.copymode(src_link, dst)
        self.assertEqual(os.stat(src).st_mode, os.stat(dst).st_mode)
        # follow dst link
        os.chmod(dst, stat.S_IRWXO)
        shutil.copymode(src, dst_link)
        self.assertEqual(os.stat(src).st_mode, os.stat(dst).st_mode)
        # follow both links
        os.chmod(dst, stat.S_IRWXO)
        shutil.copymode(src_link, dst)
        self.assertEqual(os.stat(src).st_mode, os.stat(dst).st_mode)

    @unittest.skipUnless(hasattr(os, 'lchmod'), 'requires os.lchmod')
    @support.skip_unless_symlink
    def test_copymode_symlink_to_symlink(self):
        tmp_dir = self.mkdtemp()
        src = os.path.join(tmp_dir, 'foo')
        dst = os.path.join(tmp_dir, 'bar')
        src_link = os.path.join(tmp_dir, 'baz')
        dst_link = os.path.join(tmp_dir, 'quux')
        write_file(src, 'foo')
        write_file(dst, 'foo')
        os.symlink(src, src_link)
        os.symlink(dst, dst_link)
        os.chmod(src, stat.S_IRWXU|stat.S_IRWXG)
        os.chmod(dst, stat.S_IRWXU)
        os.lchmod(src_link, stat.S_IRWXO|stat.S_IRWXG)
        # link to link
        os.lchmod(dst_link, stat.S_IRWXO)
        shutil.copymode(src_link, dst_link, symlinks=True)
        self.assertEqual(os.lstat(src_link).st_mode,
                         os.lstat(dst_link).st_mode)
        self.assertNotEqual(os.stat(src).st_mode, os.stat(dst).st_mode)
        # src link - use chmod
        os.lchmod(dst_link, stat.S_IRWXO)
        shutil.copymode(src_link, dst, symlinks=True)
        self.assertEqual(os.stat(src).st_mode, os.stat(dst).st_mode)
        # dst link - use chmod
        os.lchmod(dst_link, stat.S_IRWXO)
        shutil.copymode(src, dst_link, symlinks=True)
        self.assertEqual(os.stat(src).st_mode, os.stat(dst).st_mode)

    @unittest.skipIf(hasattr(os, 'lchmod'), 'requires os.lchmod to be missing')
    @support.skip_unless_symlink
    def test_copymode_symlink_to_symlink_wo_lchmod(self):
        tmp_dir = self.mkdtemp()
        src = os.path.join(tmp_dir, 'foo')
        dst = os.path.join(tmp_dir, 'bar')
        src_link = os.path.join(tmp_dir, 'baz')
        dst_link = os.path.join(tmp_dir, 'quux')
        write_file(src, 'foo')
        write_file(dst, 'foo')
        os.symlink(src, src_link)
        os.symlink(dst, dst_link)
        shutil.copymode(src_link, dst_link, symlinks=True)  # silent fail

    @support.skip_unless_symlink
    def test_copystat_symlinks(self):
        tmp_dir = self.mkdtemp()
        src = os.path.join(tmp_dir, 'foo')
        dst = os.path.join(tmp_dir, 'bar')
        src_link = os.path.join(tmp_dir, 'baz')
        dst_link = os.path.join(tmp_dir, 'qux')
        write_file(src, 'foo')
        src_stat = os.stat(src)
        os.utime(src, (src_stat.st_atime,
                       src_stat.st_mtime - 42.0))  # ensure different mtimes
        write_file(dst, 'bar')
        self.assertNotEqual(os.stat(src).st_mtime, os.stat(dst).st_mtime)
        os.symlink(src, src_link)
        os.symlink(dst, dst_link)
        if hasattr(os, 'lchmod'):
            os.lchmod(src_link, stat.S_IRWXO)
        if hasattr(os, 'lchflags') and hasattr(stat, 'UF_NODUMP'):
            os.lchflags(src_link, stat.UF_NODUMP)
        src_link_stat = os.lstat(src_link)
        # follow
        if hasattr(os, 'lchmod'):
            shutil.copystat(src_link, dst_link, symlinks=False)
            self.assertNotEqual(src_link_stat.st_mode, os.stat(dst).st_mode)
        # don't follow
        shutil.copystat(src_link, dst_link, symlinks=True)
        dst_link_stat = os.lstat(dst_link)
        if os.utime in os.supports_follow_symlinks:
            for attr in 'st_atime', 'st_mtime':
                # The modification times may be truncated in the new file.
                self.assertLessEqual(getattr(src_link_stat, attr),
                                     getattr(dst_link_stat, attr) + 1)
        if hasattr(os, 'lchmod'):
            self.assertEqual(src_link_stat.st_mode, dst_link_stat.st_mode)
        if hasattr(os, 'lchflags') and hasattr(src_link_stat, 'st_flags'):
            self.assertEqual(src_link_stat.st_flags, dst_link_stat.st_flags)
        # tell to follow but dst is not a link
        shutil.copystat(src_link, dst, symlinks=True)
        self.assertTrue(abs(os.stat(src).st_mtime - os.stat(dst).st_mtime) <
                        00000.1)

    @unittest.skipUnless(hasattr(os, 'chflags') and
                         hasattr(errno, 'EOPNOTSUPP') and
                         hasattr(errno, 'ENOTSUP'),
                         "requires os.chflags, EOPNOTSUPP & ENOTSUP")
    def test_copystat_handles_harmless_chflags_errors(self):
        tmpdir = self.mkdtemp()
        file1 = os.path.join(tmpdir, 'file1')
        file2 = os.path.join(tmpdir, 'file2')
        write_file(file1, 'xxx')
        write_file(file2, 'xxx')

        def make_chflags_raiser(err):
            ex = OSError()

            def _chflags_raiser(path, flags, *, follow_symlinks=True):
                ex.errno = err
                raise ex
            return _chflags_raiser
        old_chflags = os.chflags
        try:
            for err in errno.EOPNOTSUPP, errno.ENOTSUP:
                os.chflags = make_chflags_raiser(err)
                shutil.copystat(file1, file2)
            # assert others errors break it
            os.chflags = make_chflags_raiser(errno.EOPNOTSUPP + errno.ENOTSUP)
            self.assertRaises(OSError, shutil.copystat, file1, file2)
        finally:
            os.chflags = old_chflags

    @support.skip_unless_xattr
    def test_copyxattr(self):
        tmp_dir = self.mkdtemp()
        src = os.path.join(tmp_dir, 'foo')
        write_file(src, 'foo')
        dst = os.path.join(tmp_dir, 'bar')
        write_file(dst, 'bar')

        # no xattr == no problem
        shutil._copyxattr(src, dst)
        # common case
        os.setxattr(src, 'user.foo', b'42')
        os.setxattr(src, 'user.bar', b'43')
        shutil._copyxattr(src, dst)
        self.assertEqual(os.listxattr(src), os.listxattr(dst))
        self.assertEqual(
                os.getxattr(src, 'user.foo'),
                os.getxattr(dst, 'user.foo'))
        # check errors don't affect other attrs
        os.remove(dst)
        write_file(dst, 'bar')
        os_error = OSError(errno.EPERM, 'EPERM')

        def _raise_on_user_foo(fname, attr, val, **kwargs):
            if attr == 'user.foo':
                raise os_error
            else:
                orig_setxattr(fname, attr, val, **kwargs)
        try:
            orig_setxattr = os.setxattr
            os.setxattr = _raise_on_user_foo
            shutil._copyxattr(src, dst)
            self.assertIn('user.bar', os.listxattr(dst))
        finally:
            os.setxattr = orig_setxattr

    @support.skip_unless_symlink
    @support.skip_unless_xattr
    @unittest.skipUnless(hasattr(os, 'geteuid') and os.geteuid() == 0,
                         'root privileges required')
    def test_copyxattr_symlinks(self):
        # On Linux, it's only possible to access non-user xattr for symlinks;
        # which in turn require root privileges. This test should be expanded
        # as soon as other platforms gain support for extended attributes.
        tmp_dir = self.mkdtemp()
        src = os.path.join(tmp_dir, 'foo')
        src_link = os.path.join(tmp_dir, 'baz')
        write_file(src, 'foo')
        os.symlink(src, src_link)
        os.setxattr(src, 'trusted.foo', b'42')
        os.setxattr(src_link, 'trusted.foo', b'43', follow_symlinks=False)
        dst = os.path.join(tmp_dir, 'bar')
        dst_link = os.path.join(tmp_dir, 'qux')
        write_file(dst, 'bar')
        os.symlink(dst, dst_link)
        shutil._copyxattr(src_link, dst_link, symlinks=True)
        self.assertEqual(os.getxattr(dst_link, 'trusted.foo', follow_symlinks=False), b'43')
        self.assertRaises(OSError, os.getxattr, dst, 'trusted.foo')
        shutil._copyxattr(src_link, dst, symlinks=True)
        self.assertEqual(os.getxattr(dst, 'trusted.foo'), b'43')

    @support.skip_unless_symlink
    def test_copy_symlinks(self):
        tmp_dir = self.mkdtemp()
        src = os.path.join(tmp_dir, 'foo')
        dst = os.path.join(tmp_dir, 'bar')
        src_link = os.path.join(tmp_dir, 'baz')
        write_file(src, 'foo')
        os.symlink(src, src_link)
        if hasattr(os, 'lchmod'):
            os.lchmod(src_link, stat.S_IRWXU | stat.S_IRWXO)
        # don't follow
        shutil.copy(src_link, dst, symlinks=False)
        self.assertFalse(os.path.islink(dst))
        self.assertEqual(read_file(src), read_file(dst))
        os.remove(dst)
        # follow
        shutil.copy(src_link, dst, symlinks=True)
        self.assertTrue(os.path.islink(dst))
        self.assertEqual(os.readlink(dst), os.readlink(src_link))
        if hasattr(os, 'lchmod'):
            self.assertEqual(os.lstat(src_link).st_mode,
                             os.lstat(dst).st_mode)

    @support.skip_unless_symlink
    def test_copy2_symlinks(self):
        tmp_dir = self.mkdtemp()
        src = os.path.join(tmp_dir, 'foo')
        dst = os.path.join(tmp_dir, 'bar')
        src_link = os.path.join(tmp_dir, 'baz')
        write_file(src, 'foo')
        os.symlink(src, src_link)
        if hasattr(os, 'lchmod'):
            os.lchmod(src_link, stat.S_IRWXU | stat.S_IRWXO)
        if hasattr(os, 'lchflags') and hasattr(stat, 'UF_NODUMP'):
            os.lchflags(src_link, stat.UF_NODUMP)
        src_stat = os.stat(src)
        src_link_stat = os.lstat(src_link)
        # follow
        shutil.copy2(src_link, dst, symlinks=False)
        self.assertFalse(os.path.islink(dst))
        self.assertEqual(read_file(src), read_file(dst))
        os.remove(dst)
        # don't follow
        shutil.copy2(src_link, dst, symlinks=True)
        self.assertTrue(os.path.islink(dst))
        self.assertEqual(os.readlink(dst), os.readlink(src_link))
        dst_stat = os.lstat(dst)
        if os.utime in os.supports_follow_symlinks:
            for attr in 'st_atime', 'st_mtime':
                # The modification times may be truncated in the new file.
                self.assertLessEqual(getattr(src_link_stat, attr),
                                     getattr(dst_stat, attr) + 1)
        if hasattr(os, 'lchmod'):
            self.assertEqual(src_link_stat.st_mode, dst_stat.st_mode)
            self.assertNotEqual(src_stat.st_mode, dst_stat.st_mode)
        if hasattr(os, 'lchflags') and hasattr(src_link_stat, 'st_flags'):
            self.assertEqual(src_link_stat.st_flags, dst_stat.st_flags)

    @support.skip_unless_xattr
    def test_copy2_xattr(self):
        tmp_dir = self.mkdtemp()
        src = os.path.join(tmp_dir, 'foo')
        dst = os.path.join(tmp_dir, 'bar')
        write_file(src, 'foo')
        os.setxattr(src, 'user.foo', b'42')
        shutil.copy2(src, dst)
        self.assertEqual(
                os.getxattr(src, 'user.foo'),
                os.getxattr(dst, 'user.foo'))
        os.remove(dst)

    @support.skip_unless_symlink
    def test_copyfile_symlinks(self):
        tmp_dir = self.mkdtemp()
        src = os.path.join(tmp_dir, 'src')
        dst = os.path.join(tmp_dir, 'dst')
        dst_link = os.path.join(tmp_dir, 'dst_link')
        link = os.path.join(tmp_dir, 'link')
        write_file(src, 'foo')
        os.symlink(src, link)
        # don't follow
        shutil.copyfile(link, dst_link, symlinks=True)
        self.assertTrue(os.path.islink(dst_link))
        self.assertEqual(os.readlink(link), os.readlink(dst_link))
        # follow
        shutil.copyfile(link, dst)
        self.assertFalse(os.path.islink(dst))

    def test_rmtree_uses_safe_fd_version_if_available(self):
        if os.unlink in os.supports_dir_fd and os.open in os.supports_dir_fd:
            self.assertTrue(shutil._use_fd_functions)
            self.assertTrue(shutil.rmtree_is_safe)
            tmp_dir = self.mkdtemp()
            d = os.path.join(tmp_dir, 'a')
            os.mkdir(d)
            try:
                real_rmtree = shutil._rmtree_safe_fd
                class Called(Exception): pass
                def _raiser(*args, **kwargs):
                    raise Called
                shutil._rmtree_safe_fd = _raiser
                self.assertRaises(Called, shutil.rmtree, d)
            finally:
                shutil._rmtree_safe_fd = real_rmtree
        else:
            self.assertFalse(shutil._use_fd_functions)
            self.assertFalse(shutil.rmtree_is_safe)

    def test_rmtree_dont_delete_file(self):
        # When called on a file instead of a directory, don't delete it.
        handle, path = tempfile.mkstemp()
        os.close(handle)
        self.assertRaises(NotADirectoryError, shutil.rmtree, path)
        os.remove(path)

    def test_copytree_simple(self):
        src_dir = tempfile.mkdtemp()
        dst_dir = os.path.join(tempfile.mkdtemp(), 'destination')
        self.addCleanup(shutil.rmtree, src_dir)
        self.addCleanup(shutil.rmtree, os.path.dirname(dst_dir))
        write_file((src_dir, 'test.txt'), '123')
        os.mkdir(os.path.join(src_dir, 'test_dir'))
        write_file((src_dir, 'test_dir', 'test.txt'), '456')

        shutil.copytree(src_dir, dst_dir)
        self.assertTrue(os.path.isfile(os.path.join(dst_dir, 'test.txt')))
        self.assertTrue(os.path.isdir(os.path.join(dst_dir, 'test_dir')))
        self.assertTrue(os.path.isfile(os.path.join(dst_dir, 'test_dir',
                                                    'test.txt')))
        actual = read_file((dst_dir, 'test.txt'))
        self.assertEqual(actual, '123')
        actual = read_file((dst_dir, 'test_dir', 'test.txt'))
        self.assertEqual(actual, '456')

    @support.skip_unless_symlink
    def test_copytree_symlinks(self):
        tmp_dir = self.mkdtemp()
        src_dir = os.path.join(tmp_dir, 'src')
        dst_dir = os.path.join(tmp_dir, 'dst')
        sub_dir = os.path.join(src_dir, 'sub')
        os.mkdir(src_dir)
        os.mkdir(sub_dir)
        write_file((src_dir, 'file.txt'), 'foo')
        src_link = os.path.join(sub_dir, 'link')
        dst_link = os.path.join(dst_dir, 'sub/link')
        os.symlink(os.path.join(src_dir, 'file.txt'),
                   src_link)
        if hasattr(os, 'lchmod'):
            os.lchmod(src_link, stat.S_IRWXU | stat.S_IRWXO)
        if hasattr(os, 'lchflags') and hasattr(stat, 'UF_NODUMP'):
            os.lchflags(src_link, stat.UF_NODUMP)
        src_stat = os.lstat(src_link)
        shutil.copytree(src_dir, dst_dir, symlinks=True)
        self.assertTrue(os.path.islink(os.path.join(dst_dir, 'sub', 'link')))
        self.assertEqual(os.readlink(os.path.join(dst_dir, 'sub', 'link')),
                         os.path.join(src_dir, 'file.txt'))
        dst_stat = os.lstat(dst_link)
        if hasattr(os, 'lchmod'):
            self.assertEqual(dst_stat.st_mode, src_stat.st_mode)
        if hasattr(os, 'lchflags'):
            self.assertEqual(dst_stat.st_flags, src_stat.st_flags)

    def test_copytree_with_exclude(self):
        # creating data
        join = os.path.join
        exists = os.path.exists
        src_dir = tempfile.mkdtemp()
        try:
            dst_dir = join(tempfile.mkdtemp(), 'destination')
            write_file((src_dir, 'test.txt'), '123')
            write_file((src_dir, 'test.tmp'), '123')
            os.mkdir(join(src_dir, 'test_dir'))
            write_file((src_dir, 'test_dir', 'test.txt'), '456')
            os.mkdir(join(src_dir, 'test_dir2'))
            write_file((src_dir, 'test_dir2', 'test.txt'), '456')
            os.mkdir(join(src_dir, 'test_dir2', 'subdir'))
            os.mkdir(join(src_dir, 'test_dir2', 'subdir2'))
            write_file((src_dir, 'test_dir2', 'subdir', 'test.txt'), '456')
            write_file((src_dir, 'test_dir2', 'subdir2', 'test.py'), '456')

            # testing glob-like patterns
            try:
                patterns = shutil.ignore_patterns('*.tmp', 'test_dir2')
                shutil.copytree(src_dir, dst_dir, ignore=patterns)
                # checking the result: some elements should not be copied
                self.assertTrue(exists(join(dst_dir, 'test.txt')))
                self.assertFalse(exists(join(dst_dir, 'test.tmp')))
                self.assertFalse(exists(join(dst_dir, 'test_dir2')))
            finally:
                shutil.rmtree(dst_dir)
            try:
                patterns = shutil.ignore_patterns('*.tmp', 'subdir*')
                shutil.copytree(src_dir, dst_dir, ignore=patterns)
                # checking the result: some elements should not be copied
                self.assertFalse(exists(join(dst_dir, 'test.tmp')))
                self.assertFalse(exists(join(dst_dir, 'test_dir2', 'subdir2')))
                self.assertFalse(exists(join(dst_dir, 'test_dir2', 'subdir')))
            finally:
                shutil.rmtree(dst_dir)

            # testing callable-style
            try:
                def _filter(src, names):
                    res = []
                    for name in names:
                        path = os.path.join(src, name)

                        if (os.path.isdir(path) and
                            path.split()[-1] == 'subdir'):
                            res.append(name)
                        elif os.path.splitext(path)[-1] in ('.py'):
                            res.append(name)
                    return res

                shutil.copytree(src_dir, dst_dir, ignore=_filter)

                # checking the result: some elements should not be copied
                self.assertFalse(exists(join(dst_dir, 'test_dir2', 'subdir2',
                                             'test.py')))
                self.assertFalse(exists(join(dst_dir, 'test_dir2', 'subdir')))

            finally:
                shutil.rmtree(dst_dir)
        finally:
            shutil.rmtree(src_dir)
            shutil.rmtree(os.path.dirname(dst_dir))

    @unittest.skipUnless(hasattr(os, 'link'), 'requires os.link')
    def test_dont_copy_file_onto_link_to_itself(self):
        # Temporarily disable test on Windows.
        if os.name == 'nt':
            return
        # bug 851123.
        os.mkdir(TESTFN)
        src = os.path.join(TESTFN, 'cheese')
        dst = os.path.join(TESTFN, 'shop')
        try:
            with open(src, 'w') as f:
                f.write('cheddar')
            os.link(src, dst)
            self.assertRaises(shutil.Error, shutil.copyfile, src, dst)
            with open(src, 'r') as f:
                self.assertEqual(f.read(), 'cheddar')
            os.remove(dst)
        finally:
            shutil.rmtree(TESTFN, ignore_errors=True)

    @support.skip_unless_symlink
    def test_dont_copy_file_onto_symlink_to_itself(self):
        # bug 851123.
        os.mkdir(TESTFN)
        src = os.path.join(TESTFN, 'cheese')
        dst = os.path.join(TESTFN, 'shop')
        try:
            with open(src, 'w') as f:
                f.write('cheddar')
            # Using `src` here would mean we end up with a symlink pointing
            # to TESTFN/TESTFN/cheese, while it should point at
            # TESTFN/cheese.
            os.symlink('cheese', dst)
            self.assertRaises(shutil.Error, shutil.copyfile, src, dst)
            with open(src, 'r') as f:
                self.assertEqual(f.read(), 'cheddar')
            os.remove(dst)
        finally:
            shutil.rmtree(TESTFN, ignore_errors=True)

    @support.skip_unless_symlink
    def test_rmtree_on_symlink(self):
        # bug 1669.
        os.mkdir(TESTFN)
        try:
            src = os.path.join(TESTFN, 'cheese')
            dst = os.path.join(TESTFN, 'shop')
            os.mkdir(src)
            os.symlink(src, dst)
            self.assertRaises(OSError, shutil.rmtree, dst)
            shutil.rmtree(dst, ignore_errors=True)
        finally:
            shutil.rmtree(TESTFN, ignore_errors=True)

    if hasattr(os, "mkfifo"):
        # Issue #3002: copyfile and copytree block indefinitely on named pipes
        def test_copyfile_named_pipe(self):
            os.mkfifo(TESTFN)
            try:
                self.assertRaises(shutil.SpecialFileError,
                                  shutil.copyfile, TESTFN, TESTFN2)
                self.assertRaises(shutil.SpecialFileError,
                                  shutil.copyfile, __file__, TESTFN)
            finally:
                os.remove(TESTFN)

        @support.skip_unless_symlink
        def test_copytree_named_pipe(self):
            os.mkdir(TESTFN)
            try:
                subdir = os.path.join(TESTFN, "subdir")
                os.mkdir(subdir)
                pipe = os.path.join(subdir, "mypipe")
                os.mkfifo(pipe)
                try:
                    shutil.copytree(TESTFN, TESTFN2)
                except shutil.Error as e:
                    errors = e.args[0]
                    self.assertEqual(len(errors), 1)
                    src, dst, error_msg = errors[0]
                    self.assertEqual("`%s` is a named pipe" % pipe, error_msg)
                else:
                    self.fail("shutil.Error should have been raised")
            finally:
                shutil.rmtree(TESTFN, ignore_errors=True)
                shutil.rmtree(TESTFN2, ignore_errors=True)

    def test_copytree_special_func(self):

        src_dir = self.mkdtemp()
        dst_dir = os.path.join(self.mkdtemp(), 'destination')
        write_file((src_dir, 'test.txt'), '123')
        os.mkdir(os.path.join(src_dir, 'test_dir'))
        write_file((src_dir, 'test_dir', 'test.txt'), '456')

        copied = []
        def _copy(src, dst):
            copied.append((src, dst))

        shutil.copytree(src_dir, dst_dir, copy_function=_copy)
        self.assertEqual(len(copied), 2)

    @support.skip_unless_symlink
    def test_copytree_dangling_symlinks(self):

        # a dangling symlink raises an error at the end
        src_dir = self.mkdtemp()
        dst_dir = os.path.join(self.mkdtemp(), 'destination')
        os.symlink('IDONTEXIST', os.path.join(src_dir, 'test.txt'))
        os.mkdir(os.path.join(src_dir, 'test_dir'))
        write_file((src_dir, 'test_dir', 'test.txt'), '456')
        self.assertRaises(Error, shutil.copytree, src_dir, dst_dir)

        # a dangling symlink is ignored with the proper flag
        dst_dir = os.path.join(self.mkdtemp(), 'destination2')
        shutil.copytree(src_dir, dst_dir, ignore_dangling_symlinks=True)
        self.assertNotIn('test.txt', os.listdir(dst_dir))

        # a dangling symlink is copied if symlinks=True
        dst_dir = os.path.join(self.mkdtemp(), 'destination3')
        shutil.copytree(src_dir, dst_dir, symlinks=True)
        self.assertIn('test.txt', os.listdir(dst_dir))

    def _copy_file(self, method):
        fname = 'test.txt'
        tmpdir = self.mkdtemp()
        write_file((tmpdir, fname), 'xxx')
        file1 = os.path.join(tmpdir, fname)
        tmpdir2 = self.mkdtemp()
        method(file1, tmpdir2)
        file2 = os.path.join(tmpdir2, fname)
        return (file1, file2)

    @unittest.skipUnless(hasattr(os, 'chmod'), 'requires os.chmod')
    def test_copy(self):
        # Ensure that the copied file exists and has the same mode bits.
        file1, file2 = self._copy_file(shutil.copy)
        self.assertTrue(os.path.exists(file2))
        self.assertEqual(os.stat(file1).st_mode, os.stat(file2).st_mode)

    @unittest.skipUnless(hasattr(os, 'chmod'), 'requires os.chmod')
    @unittest.skipUnless(hasattr(os, 'utime'), 'requires os.utime')
    def test_copy2(self):
        # Ensure that the copied file exists and has the same mode and
        # modification time bits.
        file1, file2 = self._copy_file(shutil.copy2)
        self.assertTrue(os.path.exists(file2))
        file1_stat = os.stat(file1)
        file2_stat = os.stat(file2)
        self.assertEqual(file1_stat.st_mode, file2_stat.st_mode)
        for attr in 'st_atime', 'st_mtime':
            # The modification times may be truncated in the new file.
            self.assertLessEqual(getattr(file1_stat, attr),
                                 getattr(file2_stat, attr) + 1)
        if hasattr(os, 'chflags') and hasattr(file1_stat, 'st_flags'):
            self.assertEqual(getattr(file1_stat, 'st_flags'),
                             getattr(file2_stat, 'st_flags'))

    @requires_zlib
    def test_make_tarball(self):
        # creating something to tar
        tmpdir = self.mkdtemp()
        write_file((tmpdir, 'file1'), 'xxx')
        write_file((tmpdir, 'file2'), 'xxx')
        os.mkdir(os.path.join(tmpdir, 'sub'))
        write_file((tmpdir, 'sub', 'file3'), 'xxx')

        tmpdir2 = self.mkdtemp()
        # force shutil to create the directory
        os.rmdir(tmpdir2)
        unittest.skipUnless(splitdrive(tmpdir)[0] == splitdrive(tmpdir2)[0],
                            "source and target should be on same drive")

        base_name = os.path.join(tmpdir2, 'archive')

        # working with relative paths to avoid tar warnings
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            _make_tarball(splitdrive(base_name)[1], '.')
        finally:
            os.chdir(old_dir)

        # check if the compressed tarball was created
        tarball = base_name + '.tar.gz'
        self.assertTrue(os.path.exists(tarball))

        # trying an uncompressed one
        base_name = os.path.join(tmpdir2, 'archive')
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            _make_tarball(splitdrive(base_name)[1], '.', compress=None)
        finally:
            os.chdir(old_dir)
        tarball = base_name + '.tar'
        self.assertTrue(os.path.exists(tarball))

    def _tarinfo(self, path):
        tar = tarfile.open(path)
        try:
            names = tar.getnames()
            names.sort()
            return tuple(names)
        finally:
            tar.close()

    def _create_files(self):
        # creating something to tar
        tmpdir = self.mkdtemp()
        dist = os.path.join(tmpdir, 'dist')
        os.mkdir(dist)
        write_file((dist, 'file1'), 'xxx')
        write_file((dist, 'file2'), 'xxx')
        os.mkdir(os.path.join(dist, 'sub'))
        write_file((dist, 'sub', 'file3'), 'xxx')
        os.mkdir(os.path.join(dist, 'sub2'))
        tmpdir2 = self.mkdtemp()
        base_name = os.path.join(tmpdir2, 'archive')
        return tmpdir, tmpdir2, base_name

    @requires_zlib
    @unittest.skipUnless(find_executable('tar') and find_executable('gzip'),
                         'Need the tar command to run')
    def test_tarfile_vs_tar(self):
        tmpdir, tmpdir2, base_name =  self._create_files()
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            _make_tarball(base_name, 'dist')
        finally:
            os.chdir(old_dir)

        # check if the compressed tarball was created
        tarball = base_name + '.tar.gz'
        self.assertTrue(os.path.exists(tarball))

        # now create another tarball using `tar`
        tarball2 = os.path.join(tmpdir, 'archive2.tar.gz')
        tar_cmd = ['tar', '-cf', 'archive2.tar', 'dist']
        gzip_cmd = ['gzip', '-f9', 'archive2.tar']
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            with captured_stdout() as s:
                spawn(tar_cmd)
                spawn(gzip_cmd)
        finally:
            os.chdir(old_dir)

        self.assertTrue(os.path.exists(tarball2))
        # let's compare both tarballs
        self.assertEqual(self._tarinfo(tarball), self._tarinfo(tarball2))

        # trying an uncompressed one
        base_name = os.path.join(tmpdir2, 'archive')
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            _make_tarball(base_name, 'dist', compress=None)
        finally:
            os.chdir(old_dir)
        tarball = base_name + '.tar'
        self.assertTrue(os.path.exists(tarball))

        # now for a dry_run
        base_name = os.path.join(tmpdir2, 'archive')
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            _make_tarball(base_name, 'dist', compress=None, dry_run=True)
        finally:
            os.chdir(old_dir)
        tarball = base_name + '.tar'
        self.assertTrue(os.path.exists(tarball))

    @requires_zlib
    @unittest.skipUnless(ZIP_SUPPORT, 'Need zip support to run')
    def test_make_zipfile(self):
        # creating something to tar
        tmpdir = self.mkdtemp()
        write_file((tmpdir, 'file1'), 'xxx')
        write_file((tmpdir, 'file2'), 'xxx')

        tmpdir2 = self.mkdtemp()
        # force shutil to create the directory
        os.rmdir(tmpdir2)
        base_name = os.path.join(tmpdir2, 'archive')
        _make_zipfile(base_name, tmpdir)

        # check if the compressed tarball was created
        tarball = base_name + '.zip'
        self.assertTrue(os.path.exists(tarball))


    def test_make_archive(self):
        tmpdir = self.mkdtemp()
        base_name = os.path.join(tmpdir, 'archive')
        self.assertRaises(ValueError, make_archive, base_name, 'xxx')

    @requires_zlib
    def test_make_archive_owner_group(self):
        # testing make_archive with owner and group, with various combinations
        # this works even if there's not gid/uid support
        if UID_GID_SUPPORT:
            group = grp.getgrgid(0)[0]
            owner = pwd.getpwuid(0)[0]
        else:
            group = owner = 'root'

        base_dir, root_dir, base_name =  self._create_files()
        base_name = os.path.join(self.mkdtemp() , 'archive')
        res = make_archive(base_name, 'zip', root_dir, base_dir, owner=owner,
                           group=group)
        self.assertTrue(os.path.exists(res))

        res = make_archive(base_name, 'zip', root_dir, base_dir)
        self.assertTrue(os.path.exists(res))

        res = make_archive(base_name, 'tar', root_dir, base_dir,
                           owner=owner, group=group)
        self.assertTrue(os.path.exists(res))

        res = make_archive(base_name, 'tar', root_dir, base_dir,
                           owner='kjhkjhkjg', group='oihohoh')
        self.assertTrue(os.path.exists(res))


    @requires_zlib
    @unittest.skipUnless(UID_GID_SUPPORT, "Requires grp and pwd support")
    def test_tarfile_root_owner(self):
        tmpdir, tmpdir2, base_name =  self._create_files()
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        group = grp.getgrgid(0)[0]
        owner = pwd.getpwuid(0)[0]
        try:
            archive_name = _make_tarball(base_name, 'dist', compress=None,
                                         owner=owner, group=group)
        finally:
            os.chdir(old_dir)

        # check if the compressed tarball was created
        self.assertTrue(os.path.exists(archive_name))

        # now checks the rights
        archive = tarfile.open(archive_name)
        try:
            for member in archive.getmembers():
                self.assertEqual(member.uid, 0)
                self.assertEqual(member.gid, 0)
        finally:
            archive.close()

    def test_make_archive_cwd(self):
        current_dir = os.getcwd()
        def _breaks(*args, **kw):
            raise RuntimeError()

        register_archive_format('xxx', _breaks, [], 'xxx file')
        try:
            try:
                make_archive('xxx', 'xxx', root_dir=self.mkdtemp())
            except Exception:
                pass
            self.assertEqual(os.getcwd(), current_dir)
        finally:
            unregister_archive_format('xxx')

    def test_register_archive_format(self):

        self.assertRaises(TypeError, register_archive_format, 'xxx', 1)
        self.assertRaises(TypeError, register_archive_format, 'xxx', lambda: x,
                          1)
        self.assertRaises(TypeError, register_archive_format, 'xxx', lambda: x,
                          [(1, 2), (1, 2, 3)])

        register_archive_format('xxx', lambda: x, [(1, 2)], 'xxx file')
        formats = [name for name, params in get_archive_formats()]
        self.assertIn('xxx', formats)

        unregister_archive_format('xxx')
        formats = [name for name, params in get_archive_formats()]
        self.assertNotIn('xxx', formats)

    def _compare_dirs(self, dir1, dir2):
        # check that dir1 and dir2 are equivalent,
        # return the diff
        diff = []
        for root, dirs, files in os.walk(dir1):
            for file_ in files:
                path = os.path.join(root, file_)
                target_path = os.path.join(dir2, os.path.split(path)[-1])
                if not os.path.exists(target_path):
                    diff.append(file_)
        return diff

    @requires_zlib
    def test_unpack_archive(self):
        formats = ['tar', 'gztar', 'zip']
        if BZ2_SUPPORTED:
            formats.append('bztar')

        for format in formats:
            tmpdir = self.mkdtemp()
            base_dir, root_dir, base_name =  self._create_files()
            tmpdir2 = self.mkdtemp()
            filename = make_archive(base_name, format, root_dir, base_dir)

            # let's try to unpack it now
            unpack_archive(filename, tmpdir2)
            diff = self._compare_dirs(tmpdir, tmpdir2)
            self.assertEqual(diff, [])

            # and again, this time with the format specified
            tmpdir3 = self.mkdtemp()
            unpack_archive(filename, tmpdir3, format=format)
            diff = self._compare_dirs(tmpdir, tmpdir3)
            self.assertEqual(diff, [])
        self.assertRaises(shutil.ReadError, unpack_archive, TESTFN)
        self.assertRaises(ValueError, unpack_archive, TESTFN, format='xxx')

    def test_unpack_registery(self):

        formats = get_unpack_formats()

        def _boo(filename, extract_dir, extra):
            self.assertEqual(extra, 1)
            self.assertEqual(filename, 'stuff.boo')
            self.assertEqual(extract_dir, 'xx')

        register_unpack_format('Boo', ['.boo', '.b2'], _boo, [('extra', 1)])
        unpack_archive('stuff.boo', 'xx')

        # trying to register a .boo unpacker again
        self.assertRaises(RegistryError, register_unpack_format, 'Boo2',
                          ['.boo'], _boo)

        # should work now
        unregister_unpack_format('Boo')
        register_unpack_format('Boo2', ['.boo'], _boo)
        self.assertIn(('Boo2', ['.boo'], ''), get_unpack_formats())
        self.assertNotIn(('Boo', ['.boo'], ''), get_unpack_formats())

        # let's leave a clean state
        unregister_unpack_format('Boo2')
        self.assertEqual(get_unpack_formats(), formats)

    @unittest.skipUnless(hasattr(shutil, 'disk_usage'),
                         "disk_usage not available on this platform")
    def test_disk_usage(self):
        usage = shutil.disk_usage(os.getcwd())
        self.assertGreater(usage.total, 0)
        self.assertGreater(usage.used, 0)
        self.assertGreaterEqual(usage.free, 0)
        self.assertGreaterEqual(usage.total, usage.used)
        self.assertGreater(usage.total, usage.free)

    @unittest.skipUnless(UID_GID_SUPPORT, "Requires grp and pwd support")
    @unittest.skipUnless(hasattr(os, 'chown'), 'requires os.chown')
    def test_chown(self):

        # cleaned-up automatically by TestShutil.tearDown method
        dirname = self.mkdtemp()
        filename = tempfile.mktemp(dir=dirname)
        write_file(filename, 'testing chown function')

        with self.assertRaises(ValueError):
            shutil.chown(filename)

        with self.assertRaises(LookupError):
            shutil.chown(filename, user='non-exising username')

        with self.assertRaises(LookupError):
            shutil.chown(filename, group='non-exising groupname')

        with self.assertRaises(TypeError):
            shutil.chown(filename, b'spam')

        with self.assertRaises(TypeError):
            shutil.chown(filename, 3.14)

        uid = os.getuid()
        gid = os.getgid()

        def check_chown(path, uid=None, gid=None):
            s = os.stat(filename)
            if uid is not None:
                self.assertEqual(uid, s.st_uid)
            if gid is not None:
                self.assertEqual(gid, s.st_gid)

        shutil.chown(filename, uid, gid)
        check_chown(filename, uid, gid)
        shutil.chown(filename, uid)
        check_chown(filename, uid)
        shutil.chown(filename, user=uid)
        check_chown(filename, uid)
        shutil.chown(filename, group=gid)
        check_chown(filename, gid=gid)

        shutil.chown(dirname, uid, gid)
        check_chown(dirname, uid, gid)
        shutil.chown(dirname, uid)
        check_chown(dirname, uid)
        shutil.chown(dirname, user=uid)
        check_chown(dirname, uid)
        shutil.chown(dirname, group=gid)
        check_chown(dirname, gid=gid)

        user = pwd.getpwuid(uid)[0]
        group = grp.getgrgid(gid)[0]
        shutil.chown(filename, user, group)
        check_chown(filename, uid, gid)
        shutil.chown(dirname, user, group)
        check_chown(dirname, uid, gid)

    def test_copy_return_value(self):
        # copy and copy2 both return their destination path.
        for fn in (shutil.copy, shutil.copy2):
            src_dir = self.mkdtemp()
            dst_dir = self.mkdtemp()
            src = os.path.join(src_dir, 'foo')
            write_file(src, 'foo')
            rv = fn(src, dst_dir)
            self.assertEqual(rv, os.path.join(dst_dir, 'foo'))
            rv = fn(src, os.path.join(dst_dir, 'bar'))
            self.assertEqual(rv, os.path.join(dst_dir, 'bar'))

    def test_copyfile_return_value(self):
        # copytree returns its destination path.
        src_dir = self.mkdtemp()
        dst_dir = self.mkdtemp()
        dst_file = os.path.join(dst_dir, 'bar')
        src_file = os.path.join(src_dir, 'foo')
        write_file(src_file, 'foo')
        rv = shutil.copyfile(src_file, dst_file)
        self.assertTrue(os.path.exists(rv))
        self.assertEqual(read_file(src_file), read_file(dst_file))

    def test_copytree_return_value(self):
        # copytree returns its destination path.
        src_dir = self.mkdtemp()
        dst_dir = src_dir + "dest"
        src = os.path.join(src_dir, 'foo')
        write_file(src, 'foo')
        rv = shutil.copytree(src_dir, dst_dir)
        self.assertEqual(['foo'], os.listdir(rv))


class TestWhich(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        # Give the temp_file an ".exe" suffix for all.
        # It's needed on Windows and not harmful on other platforms.
        self.temp_file = tempfile.NamedTemporaryFile(dir=self.temp_dir,
                                                     suffix=".exe")
        os.chmod(self.temp_file.name, stat.S_IXUSR)
        self.addCleanup(self.temp_file.close)
        self.dir, self.file = os.path.split(self.temp_file.name)

    def test_basic(self):
        # Given an EXE in a directory, it should be returned.
        rv = shutil.which(self.file, path=self.dir)
        self.assertEqual(rv, self.temp_file.name)

    def test_full_path_short_circuit(self):
        # When given the fully qualified path to an executable that exists,
        # it should be returned.
        rv = shutil.which(self.temp_file.name, path=self.temp_dir)
        self.assertEqual(self.temp_file.name, rv)

    def test_non_matching_mode(self):
        # Set the file read-only and ask for writeable files.
        os.chmod(self.temp_file.name, stat.S_IREAD)
        rv = shutil.which(self.file, path=self.dir, mode=os.W_OK)
        self.assertIsNone(rv)

    def test_relative(self):
        old_cwd = os.getcwd()
        base_dir, tail_dir = os.path.split(self.dir)
        os.chdir(base_dir)
        try:
            rv = shutil.which(self.file, path=tail_dir)
            self.assertEqual(rv, os.path.join(tail_dir, self.file))
        finally:
            os.chdir(old_cwd)

    def test_nonexistent_file(self):
        # Return None when no matching executable file is found on the path.
        rv = shutil.which("foo.exe", path=self.dir)
        self.assertIsNone(rv)

    @unittest.skipUnless(sys.platform == "win32",
                         "pathext check is Windows-only")
    def test_pathext_checking(self):
        # Ask for the file without the ".exe" extension, then ensure that
        # it gets found properly with the extension.
        rv = shutil.which(self.temp_file.name[:-4], path=self.dir)
        self.assertEqual(self.temp_file.name, rv)


class TestMove(unittest.TestCase):

    def setUp(self):
        filename = "foo"
        self.src_dir = tempfile.mkdtemp()
        self.dst_dir = tempfile.mkdtemp()
        self.src_file = os.path.join(self.src_dir, filename)
        self.dst_file = os.path.join(self.dst_dir, filename)
        with open(self.src_file, "wb") as f:
            f.write(b"spam")

    def tearDown(self):
        for d in (self.src_dir, self.dst_dir):
            try:
                if d:
                    shutil.rmtree(d)
            except:
                pass

    def _check_move_file(self, src, dst, real_dst):
        with open(src, "rb") as f:
            contents = f.read()
        shutil.move(src, dst)
        with open(real_dst, "rb") as f:
            self.assertEqual(contents, f.read())
        self.assertFalse(os.path.exists(src))

    def _check_move_dir(self, src, dst, real_dst):
        contents = sorted(os.listdir(src))
        shutil.move(src, dst)
        self.assertEqual(contents, sorted(os.listdir(real_dst)))
        self.assertFalse(os.path.exists(src))

    def test_move_file(self):
        # Move a file to another location on the same filesystem.
        self._check_move_file(self.src_file, self.dst_file, self.dst_file)

    def test_move_file_to_dir(self):
        # Move a file inside an existing dir on the same filesystem.
        self._check_move_file(self.src_file, self.dst_dir, self.dst_file)

    @mock_rename
    def test_move_file_other_fs(self):
        # Move a file to an existing dir on another filesystem.
        self.test_move_file()

    @mock_rename
    def test_move_file_to_dir_other_fs(self):
        # Move a file to another location on another filesystem.
        self.test_move_file_to_dir()

    def test_move_dir(self):
        # Move a dir to another location on the same filesystem.
        dst_dir = tempfile.mktemp()
        try:
            self._check_move_dir(self.src_dir, dst_dir, dst_dir)
        finally:
            try:
                shutil.rmtree(dst_dir)
            except:
                pass

    @mock_rename
    def test_move_dir_other_fs(self):
        # Move a dir to another location on another filesystem.
        self.test_move_dir()

    def test_move_dir_to_dir(self):
        # Move a dir inside an existing dir on the same filesystem.
        self._check_move_dir(self.src_dir, self.dst_dir,
            os.path.join(self.dst_dir, os.path.basename(self.src_dir)))

    @mock_rename
    def test_move_dir_to_dir_other_fs(self):
        # Move a dir inside an existing dir on another filesystem.
        self.test_move_dir_to_dir()

    def test_existing_file_inside_dest_dir(self):
        # A file with the same name inside the destination dir already exists.
        with open(self.dst_file, "wb"):
            pass
        self.assertRaises(shutil.Error, shutil.move, self.src_file, self.dst_dir)

    def test_dont_move_dir_in_itself(self):
        # Moving a dir inside itself raises an Error.
        dst = os.path.join(self.src_dir, "bar")
        self.assertRaises(shutil.Error, shutil.move, self.src_dir, dst)

    def test_destinsrc_false_negative(self):
        os.mkdir(TESTFN)
        try:
            for src, dst in [('srcdir', 'srcdir/dest')]:
                src = os.path.join(TESTFN, src)
                dst = os.path.join(TESTFN, dst)
                self.assertTrue(shutil._destinsrc(src, dst),
                             msg='_destinsrc() wrongly concluded that '
                             'dst (%s) is not in src (%s)' % (dst, src))
        finally:
            shutil.rmtree(TESTFN, ignore_errors=True)

    def test_destinsrc_false_positive(self):
        os.mkdir(TESTFN)
        try:
            for src, dst in [('srcdir', 'src/dest'), ('srcdir', 'srcdir.new')]:
                src = os.path.join(TESTFN, src)
                dst = os.path.join(TESTFN, dst)
                self.assertFalse(shutil._destinsrc(src, dst),
                            msg='_destinsrc() wrongly concluded that '
                            'dst (%s) is in src (%s)' % (dst, src))
        finally:
            shutil.rmtree(TESTFN, ignore_errors=True)

    @support.skip_unless_symlink
    @mock_rename
    def test_move_file_symlink(self):
        dst = os.path.join(self.src_dir, 'bar')
        os.symlink(self.src_file, dst)
        shutil.move(dst, self.dst_file)
        self.assertTrue(os.path.islink(self.dst_file))
        self.assertTrue(os.path.samefile(self.src_file, self.dst_file))

    @support.skip_unless_symlink
    @mock_rename
    def test_move_file_symlink_to_dir(self):
        filename = "bar"
        dst = os.path.join(self.src_dir, filename)
        os.symlink(self.src_file, dst)
        shutil.move(dst, self.dst_dir)
        final_link = os.path.join(self.dst_dir, filename)
        self.assertTrue(os.path.islink(final_link))
        self.assertTrue(os.path.samefile(self.src_file, final_link))

    @support.skip_unless_symlink
    @mock_rename
    def test_move_dangling_symlink(self):
        src = os.path.join(self.src_dir, 'baz')
        dst = os.path.join(self.src_dir, 'bar')
        os.symlink(src, dst)
        dst_link = os.path.join(self.dst_dir, 'quux')
        shutil.move(dst, dst_link)
        self.assertTrue(os.path.islink(dst_link))
        self.assertEqual(os.path.realpath(src), os.path.realpath(dst_link))

    @support.skip_unless_symlink
    @mock_rename
    def test_move_dir_symlink(self):
        src = os.path.join(self.src_dir, 'baz')
        dst = os.path.join(self.src_dir, 'bar')
        os.mkdir(src)
        os.symlink(src, dst)
        dst_link = os.path.join(self.dst_dir, 'quux')
        shutil.move(dst, dst_link)
        self.assertTrue(os.path.islink(dst_link))
        self.assertTrue(os.path.samefile(src, dst_link))

    def test_move_return_value(self):
        rv = shutil.move(self.src_file, self.dst_dir)
        self.assertEqual(rv,
                os.path.join(self.dst_dir, os.path.basename(self.src_file)))

    def test_move_as_rename_return_value(self):
        rv = shutil.move(self.src_file, os.path.join(self.dst_dir, 'bar'))
        self.assertEqual(rv, os.path.join(self.dst_dir, 'bar'))


class TestCopyFile(unittest.TestCase):

    _delete = False

    class Faux(object):
        _entered = False
        _exited_with = None
        _raised = False
        def __init__(self, raise_in_exit=False, suppress_at_exit=True):
            self._raise_in_exit = raise_in_exit
            self._suppress_at_exit = suppress_at_exit
        def read(self, *args):
            return ''
        def __enter__(self):
            self._entered = True
        def __exit__(self, exc_type, exc_val, exc_tb):
            self._exited_with = exc_type, exc_val, exc_tb
            if self._raise_in_exit:
                self._raised = True
                raise IOError("Cannot close")
            return self._suppress_at_exit

    def tearDown(self):
        if self._delete:
            del shutil.open

    def _set_shutil_open(self, func):
        shutil.open = func
        self._delete = True

    def test_w_source_open_fails(self):
        def _open(filename, mode='r'):
            if filename == 'srcfile':
                raise IOError('Cannot open "srcfile"')
            assert 0  # shouldn't reach here.

        self._set_shutil_open(_open)

        self.assertRaises(IOError, shutil.copyfile, 'srcfile', 'destfile')

    def test_w_dest_open_fails(self):

        srcfile = self.Faux()

        def _open(filename, mode='r'):
            if filename == 'srcfile':
                return srcfile
            if filename == 'destfile':
                raise IOError('Cannot open "destfile"')
            assert 0  # shouldn't reach here.

        self._set_shutil_open(_open)

        shutil.copyfile('srcfile', 'destfile')
        self.assertTrue(srcfile._entered)
        self.assertTrue(srcfile._exited_with[0] is IOError)
        self.assertEqual(srcfile._exited_with[1].args,
                         ('Cannot open "destfile"',))

    def test_w_dest_close_fails(self):

        srcfile = self.Faux()
        destfile = self.Faux(True)

        def _open(filename, mode='r'):
            if filename == 'srcfile':
                return srcfile
            if filename == 'destfile':
                return destfile
            assert 0  # shouldn't reach here.

        self._set_shutil_open(_open)

        shutil.copyfile('srcfile', 'destfile')
        self.assertTrue(srcfile._entered)
        self.assertTrue(destfile._entered)
        self.assertTrue(destfile._raised)
        self.assertTrue(srcfile._exited_with[0] is IOError)
        self.assertEqual(srcfile._exited_with[1].args,
                         ('Cannot close',))

    def test_w_source_close_fails(self):

        srcfile = self.Faux(True)
        destfile = self.Faux()

        def _open(filename, mode='r'):
            if filename == 'srcfile':
                return srcfile
            if filename == 'destfile':
                return destfile
            assert 0  # shouldn't reach here.

        self._set_shutil_open(_open)

        self.assertRaises(IOError,
                          shutil.copyfile, 'srcfile', 'destfile')
        self.assertTrue(srcfile._entered)
        self.assertTrue(destfile._entered)
        self.assertFalse(destfile._raised)
        self.assertTrue(srcfile._exited_with[0] is None)
        self.assertTrue(srcfile._raised)

    def test_move_dir_caseinsensitive(self):
        # Renames a folder to the same name
        # but a different case.

        self.src_dir = tempfile.mkdtemp()
        dst_dir = os.path.join(
                os.path.dirname(self.src_dir),
                os.path.basename(self.src_dir).upper())
        self.assertNotEqual(self.src_dir, dst_dir)

        try:
            shutil.move(self.src_dir, dst_dir)
            self.assertTrue(os.path.isdir(dst_dir))
        finally:
            os.rmdir(dst_dir)

class TermsizeTests(unittest.TestCase):
    def test_does_not_crash(self):
        """Check if get_terminal_size() returns a meaningful value.

        There's no easy portable way to actually check the size of the
        terminal, so let's check if it returns something sensible instead.
        """
        size = shutil.get_terminal_size()
        self.assertGreaterEqual(size.columns, 0)
        self.assertGreaterEqual(size.lines, 0)

    def test_os_environ_first(self):
        "Check if environment variables have precedence"

        with support.EnvironmentVarGuard() as env:
            env['COLUMNS'] = '777'
            size = shutil.get_terminal_size()
        self.assertEqual(size.columns, 777)

        with support.EnvironmentVarGuard() as env:
            env['LINES'] = '888'
            size = shutil.get_terminal_size()
        self.assertEqual(size.lines, 888)

    @unittest.skipUnless(os.isatty(sys.__stdout__.fileno()), "not on tty")
    def test_stty_match(self):
        """Check if stty returns the same results ignoring env

        This test will fail if stdin and stdout are connected to
        different terminals with different sizes. Nevertheless, such
        situations should be pretty rare.
        """
        try:
            size = subprocess.check_output(['stty', 'size']).decode().split()
        except (FileNotFoundError, subprocess.CalledProcessError):
            self.skipTest("stty invocation failed")
        expected = (int(size[1]), int(size[0])) # reversed order

        with support.EnvironmentVarGuard() as env:
            del env['LINES']
            del env['COLUMNS']
            actual = shutil.get_terminal_size()

        self.assertEqual(expected, actual)


def test_main():
    support.run_unittest(TestShutil, TestMove, TestCopyFile,
                         TermsizeTests, TestWhich)

if __name__ == '__main__':
    test_main()
