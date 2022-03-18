# Copyright (C) 2003 Python Software Foundation

import unittest
import unittest.mock
import shutil
import tempfile
import sys
import stat
import os
import os.path
import errno
import functools
import pathlib
import subprocess
import random
import string
import contextlib
import io
from shutil import (make_archive,
                    register_archive_format, unregister_archive_format,
                    get_archive_formats, Error, unpack_archive,
                    register_unpack_format, RegistryError,
                    unregister_unpack_format, get_unpack_formats,
                    SameFileError, _GiveupOnFastCopy)
import tarfile
import zipfile
try:
    import posix
except ImportError:
    posix = None

from test import support
from test.support import TESTFN, FakePath

TESTFN2 = TESTFN + "2"
TESTFN_SRC = TESTFN + "_SRC"
TESTFN_DST = TESTFN + "_DST"
MACOS = sys.platform.startswith("darwin")
SOLARIS = sys.platform.startswith("sunos")
AIX = sys.platform[:3] == 'aix'
try:
    import grp
    import pwd
    UID_GID_SUPPORT = True
except ImportError:
    UID_GID_SUPPORT = False

try:
    import _winapi
except ImportError:
    _winapi = None

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

def write_test_file(path, size):
    """Create a test file with an arbitrary size and random text content."""
    def chunks(total, step):
        assert total >= step
        while total > step:
            yield step
            total -= step
        if total:
            yield total

    bufsize = min(size, 8192)
    chunk = b"".join([random.choice(string.ascii_letters).encode()
                      for i in range(bufsize)])
    with open(path, 'wb') as f:
        for csize in chunks(size, bufsize):
            f.write(chunk)
    assert os.path.getsize(path) == size

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

def rlistdir(path):
    res = []
    for name in sorted(os.listdir(path)):
        p = os.path.join(path, name)
        if os.path.isdir(p) and not os.path.islink(p):
            res.append(name + '/')
            for n in rlistdir(p):
                res.append(name + '/' + n)
        else:
            res.append(name)
    return res

def supports_file2file_sendfile():
    # ...apparently Linux and Solaris are the only ones
    if not hasattr(os, "sendfile"):
        return False
    srcname = None
    dstname = None
    try:
        with tempfile.NamedTemporaryFile("wb", dir=os.getcwd(), delete=False) as f:
            srcname = f.name
            f.write(b"0123456789")

        with open(srcname, "rb") as src:
            with tempfile.NamedTemporaryFile("wb", dir=os.getcwd(), delete=False) as dst:
                dstname = dst.name
                infd = src.fileno()
                outfd = dst.fileno()
                try:
                    os.sendfile(outfd, infd, 0, 2)
                except OSError:
                    return False
                else:
                    return True
    finally:
        if srcname is not None:
            support.unlink(srcname)
        if dstname is not None:
            support.unlink(dstname)


SUPPORTS_SENDFILE = supports_file2file_sendfile()

# AIX 32-bit mode, by default, lacks enough memory for the xz/lzma compiler test
# The AIX command 'dump -o program' gives XCOFF header information
# The second word of the last line in the maxdata value
# when 32-bit maxdata must be greater than 0x1000000 for the xz test to succeed
def _maxdataOK():
    if AIX and sys.maxsize == 2147483647:
        hdrs=subprocess.getoutput("/usr/bin/dump -o %s" % sys.executable)
        maxdata=hdrs.split("\n")[-1].split()[1]
        return int(maxdata,16) >= 0x20000000
    else:
        return True


class BaseTest:

    def mkdtemp(self, prefix=None):
        """Create a temporary directory that will be cleaned up.

        Returns the path of the directory.
        """
        d = tempfile.mkdtemp(prefix=prefix, dir=os.getcwd())
        self.addCleanup(support.rmtree, d)
        return d


class TestRmTree(BaseTest, unittest.TestCase):

    def test_rmtree_works_on_bytes(self):
        tmp = self.mkdtemp()
        victim = os.path.join(tmp, 'killme')
        os.mkdir(victim)
        write_file(os.path.join(victim, 'somefile'), 'foo')
        victim = os.fsencode(victim)
        self.assertIsInstance(victim, bytes)
        shutil.rmtree(victim)

    @support.skip_unless_symlink
    def test_rmtree_fails_on_symlink(self):
        tmp = self.mkdtemp()
        dir_ = os.path.join(tmp, 'dir')
        os.mkdir(dir_)
        link = os.path.join(tmp, 'link')
        os.symlink(dir_, link)
        self.assertRaises(OSError, shutil.rmtree, link)
        self.assertTrue(os.path.exists(dir_))
        self.assertTrue(os.path.lexists(link))
        errors = []
        def onerror(*args):
            errors.append(args)
        shutil.rmtree(link, onerror=onerror)
        self.assertEqual(len(errors), 1)
        self.assertIs(errors[0][0], os.path.islink)
        self.assertEqual(errors[0][1], link)
        self.assertIsInstance(errors[0][2][1], OSError)

    @support.skip_unless_symlink
    def test_rmtree_works_on_symlinks(self):
        tmp = self.mkdtemp()
        dir1 = os.path.join(tmp, 'dir1')
        dir2 = os.path.join(dir1, 'dir2')
        dir3 = os.path.join(tmp, 'dir3')
        for d in dir1, dir2, dir3:
            os.mkdir(d)
        file1 = os.path.join(tmp, 'file1')
        write_file(file1, 'foo')
        link1 = os.path.join(dir1, 'link1')
        os.symlink(dir2, link1)
        link2 = os.path.join(dir1, 'link2')
        os.symlink(dir3, link2)
        link3 = os.path.join(dir1, 'link3')
        os.symlink(file1, link3)
        # make sure symlinks are removed but not followed
        shutil.rmtree(dir1)
        self.assertFalse(os.path.exists(dir1))
        self.assertTrue(os.path.exists(dir3))
        self.assertTrue(os.path.exists(file1))

    @unittest.skipUnless(_winapi, 'only relevant on Windows')
    def test_rmtree_fails_on_junctions(self):
        tmp = self.mkdtemp()
        dir_ = os.path.join(tmp, 'dir')
        os.mkdir(dir_)
        link = os.path.join(tmp, 'link')
        _winapi.CreateJunction(dir_, link)
        self.addCleanup(support.unlink, link)
        self.assertRaises(OSError, shutil.rmtree, link)
        self.assertTrue(os.path.exists(dir_))
        self.assertTrue(os.path.lexists(link))
        errors = []
        def onerror(*args):
            errors.append(args)
        shutil.rmtree(link, onerror=onerror)
        self.assertEqual(len(errors), 1)
        self.assertIs(errors[0][0], os.path.islink)
        self.assertEqual(errors[0][1], link)
        self.assertIsInstance(errors[0][2][1], OSError)

    @unittest.skipUnless(_winapi, 'only relevant on Windows')
    def test_rmtree_works_on_junctions(self):
        tmp = self.mkdtemp()
        dir1 = os.path.join(tmp, 'dir1')
        dir2 = os.path.join(dir1, 'dir2')
        dir3 = os.path.join(tmp, 'dir3')
        for d in dir1, dir2, dir3:
            os.mkdir(d)
        file1 = os.path.join(tmp, 'file1')
        write_file(file1, 'foo')
        link1 = os.path.join(dir1, 'link1')
        _winapi.CreateJunction(dir2, link1)
        link2 = os.path.join(dir1, 'link2')
        _winapi.CreateJunction(dir3, link2)
        link3 = os.path.join(dir1, 'link3')
        _winapi.CreateJunction(file1, link3)
        # make sure junctions are removed but not followed
        shutil.rmtree(dir1)
        self.assertFalse(os.path.exists(dir1))
        self.assertTrue(os.path.exists(dir3))
        self.assertTrue(os.path.exists(file1))

    def test_rmtree_errors(self):
        # filename is guaranteed not to exist
        filename = tempfile.mktemp(dir=self.mkdtemp())
        self.assertRaises(FileNotFoundError, shutil.rmtree, filename)
        # test that ignore_errors option is honored
        shutil.rmtree(filename, ignore_errors=True)

        # existing file
        tmpdir = self.mkdtemp()
        write_file((tmpdir, "tstfile"), "")
        filename = os.path.join(tmpdir, "tstfile")
        with self.assertRaises(NotADirectoryError) as cm:
            shutil.rmtree(filename)
        # The reason for this rather odd construct is that Windows sprinkles
        # a \*.* at the end of file names. But only sometimes on some buildbots
        possible_args = [filename, os.path.join(filename, '*.*')]
        self.assertIn(cm.exception.filename, possible_args)
        self.assertTrue(os.path.exists(filename))
        # test that ignore_errors option is honored
        shutil.rmtree(filename, ignore_errors=True)
        self.assertTrue(os.path.exists(filename))
        errors = []
        def onerror(*args):
            errors.append(args)
        shutil.rmtree(filename, onerror=onerror)
        self.assertEqual(len(errors), 2)
        self.assertIs(errors[0][0], os.scandir)
        self.assertEqual(errors[0][1], filename)
        self.assertIsInstance(errors[0][2][1], NotADirectoryError)
        self.assertIn(errors[0][2][1].filename, possible_args)
        self.assertIs(errors[1][0], os.rmdir)
        self.assertEqual(errors[1][1], filename)
        self.assertIsInstance(errors[1][2][1], NotADirectoryError)
        self.assertIn(errors[1][2][1].filename, possible_args)


    @unittest.skipIf(sys.platform[:6] == 'cygwin',
                     "This test can't be run on Cygwin (issue #1071513).")
    @unittest.skipIf(hasattr(os, 'geteuid') and os.geteuid() == 0,
                     "This test can't be run reliably as root (issue #1076467).")
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
                         "Expected call to onerror function did not happen.")

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
        if self.errorState < 2:
            if func is os.unlink:
                self.assertEqual(arg, self.child_file_path)
            elif func is os.rmdir:
                self.assertEqual(arg, self.child_dir_path)
            else:
                self.assertIs(func, os.listdir)
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
            def raiser(fn, *args, **kwargs):
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

    def test_rmtree_uses_safe_fd_version_if_available(self):
        _use_fd_functions = ({os.open, os.stat, os.unlink, os.rmdir} <=
                             os.supports_dir_fd and
                             os.listdir in os.supports_fd and
                             os.stat in os.supports_follow_symlinks)
        if _use_fd_functions:
            self.assertTrue(shutil._use_fd_functions)
            self.assertTrue(shutil.rmtree.avoids_symlink_attacks)
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
            self.assertFalse(shutil.rmtree.avoids_symlink_attacks)

    def test_rmtree_dont_delete_file(self):
        # When called on a file instead of a directory, don't delete it.
        handle, path = tempfile.mkstemp(dir=self.mkdtemp())
        os.close(handle)
        self.assertRaises(NotADirectoryError, shutil.rmtree, path)
        os.remove(path)

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

    @unittest.skipUnless(_winapi, 'only relevant on Windows')
    def test_rmtree_on_junction(self):
        os.mkdir(TESTFN)
        try:
            src = os.path.join(TESTFN, 'cheese')
            dst = os.path.join(TESTFN, 'shop')
            os.mkdir(src)
            open(os.path.join(src, 'spam'), 'wb').close()
            _winapi.CreateJunction(src, dst)
            self.assertRaises(OSError, shutil.rmtree, dst)
            shutil.rmtree(dst, ignore_errors=True)
        finally:
            shutil.rmtree(TESTFN, ignore_errors=True)


class TestCopyTree(BaseTest, unittest.TestCase):

    def test_copytree_simple(self):
        src_dir = self.mkdtemp()
        dst_dir = os.path.join(self.mkdtemp(), 'destination')
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

    def test_copytree_dirs_exist_ok(self):
        src_dir = self.mkdtemp()
        dst_dir = self.mkdtemp()
        self.addCleanup(shutil.rmtree, src_dir)
        self.addCleanup(shutil.rmtree, dst_dir)

        write_file((src_dir, 'nonexisting.txt'), '123')
        os.mkdir(os.path.join(src_dir, 'existing_dir'))
        os.mkdir(os.path.join(dst_dir, 'existing_dir'))
        write_file((dst_dir, 'existing_dir', 'existing.txt'), 'will be replaced')
        write_file((src_dir, 'existing_dir', 'existing.txt'), 'has been replaced')

        shutil.copytree(src_dir, dst_dir, dirs_exist_ok=True)
        self.assertTrue(os.path.isfile(os.path.join(dst_dir, 'nonexisting.txt')))
        self.assertTrue(os.path.isdir(os.path.join(dst_dir, 'existing_dir')))
        self.assertTrue(os.path.isfile(os.path.join(dst_dir, 'existing_dir',
                                                    'existing.txt')))
        actual = read_file((dst_dir, 'nonexisting.txt'))
        self.assertEqual(actual, '123')
        actual = read_file((dst_dir, 'existing_dir', 'existing.txt'))
        self.assertEqual(actual, 'has been replaced')

        with self.assertRaises(FileExistsError):
            shutil.copytree(src_dir, dst_dir, dirs_exist_ok=False)

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
        actual = os.readlink(os.path.join(dst_dir, 'sub', 'link'))
        # Bad practice to blindly strip the prefix as it may be required to
        # correctly refer to the file, but we're only comparing paths here.
        if os.name == 'nt' and actual.startswith('\\\\?\\'):
            actual = actual[4:]
        self.assertEqual(actual, os.path.join(src_dir, 'file.txt'))
        dst_stat = os.lstat(dst_link)
        if hasattr(os, 'lchmod'):
            self.assertEqual(dst_stat.st_mode, src_stat.st_mode)
        if hasattr(os, 'lchflags'):
            self.assertEqual(dst_stat.st_flags, src_stat.st_flags)

    def test_copytree_with_exclude(self):
        # creating data
        join = os.path.join
        exists = os.path.exists
        src_dir = self.mkdtemp()
        try:
            dst_dir = join(self.mkdtemp(), 'destination')
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

    def test_copytree_arg_types_of_ignore(self):
        join = os.path.join
        exists = os.path.exists

        tmp_dir = self.mkdtemp()
        src_dir = join(tmp_dir, "source")

        os.mkdir(join(src_dir))
        os.mkdir(join(src_dir, 'test_dir'))
        os.mkdir(os.path.join(src_dir, 'test_dir', 'subdir'))
        write_file((src_dir, 'test_dir', 'subdir', 'test.txt'), '456')

        invokations = []

        def _ignore(src, names):
            invokations.append(src)
            self.assertIsInstance(src, str)
            self.assertIsInstance(names, list)
            self.assertEqual(len(names), len(set(names)))
            for name in names:
                self.assertIsInstance(name, str)
            return []

        dst_dir = join(self.mkdtemp(), 'destination')
        shutil.copytree(src_dir, dst_dir, ignore=_ignore)
        self.assertTrue(exists(join(dst_dir, 'test_dir', 'subdir',
                                    'test.txt')))

        dst_dir = join(self.mkdtemp(), 'destination')
        shutil.copytree(pathlib.Path(src_dir), dst_dir, ignore=_ignore)
        self.assertTrue(exists(join(dst_dir, 'test_dir', 'subdir',
                                    'test.txt')))

        dst_dir = join(self.mkdtemp(), 'destination')
        src_dir_entry = list(os.scandir(tmp_dir))[0]
        self.assertIsInstance(src_dir_entry, os.DirEntry)
        shutil.copytree(src_dir_entry, dst_dir, ignore=_ignore)
        self.assertTrue(exists(join(dst_dir, 'test_dir', 'subdir',
                                    'test.txt')))

        self.assertEqual(len(invokations), 9)

    def test_copytree_retains_permissions(self):
        tmp_dir = self.mkdtemp()
        src_dir = os.path.join(tmp_dir, 'source')
        os.mkdir(src_dir)
        dst_dir = os.path.join(tmp_dir, 'destination')
        self.addCleanup(shutil.rmtree, tmp_dir)

        os.chmod(src_dir, 0o777)
        write_file((src_dir, 'permissive.txt'), '123')
        os.chmod(os.path.join(src_dir, 'permissive.txt'), 0o777)
        write_file((src_dir, 'restrictive.txt'), '456')
        os.chmod(os.path.join(src_dir, 'restrictive.txt'), 0o600)
        restrictive_subdir = tempfile.mkdtemp(dir=src_dir)
        self.addCleanup(support.rmtree, restrictive_subdir)
        os.chmod(restrictive_subdir, 0o600)

        shutil.copytree(src_dir, dst_dir)
        self.assertEqual(os.stat(src_dir).st_mode, os.stat(dst_dir).st_mode)
        self.assertEqual(os.stat(os.path.join(src_dir, 'permissive.txt')).st_mode,
                          os.stat(os.path.join(dst_dir, 'permissive.txt')).st_mode)
        self.assertEqual(os.stat(os.path.join(src_dir, 'restrictive.txt')).st_mode,
                          os.stat(os.path.join(dst_dir, 'restrictive.txt')).st_mode)
        restrictive_subdir_dst = os.path.join(dst_dir,
                                              os.path.split(restrictive_subdir)[1])
        self.assertEqual(os.stat(restrictive_subdir).st_mode,
                          os.stat(restrictive_subdir_dst).st_mode)

    @unittest.mock.patch('os.chmod')
    def test_copytree_winerror(self, mock_patch):
        # When copying to VFAT, copystat() raises OSError. On Windows, the
        # exception object has a meaningful 'winerror' attribute, but not
        # on other operating systems. Do not assume 'winerror' is set.
        src_dir = self.mkdtemp()
        dst_dir = os.path.join(self.mkdtemp(), 'destination')
        self.addCleanup(shutil.rmtree, src_dir)
        self.addCleanup(shutil.rmtree, os.path.dirname(dst_dir))

        mock_patch.side_effect = PermissionError('ka-boom')
        with self.assertRaises(shutil.Error):
            shutil.copytree(src_dir, dst_dir)

    def test_copytree_custom_copy_function(self):
        # See: https://bugs.python.org/issue35648
        def custom_cpfun(a, b):
            flag.append(None)
            self.assertIsInstance(a, str)
            self.assertIsInstance(b, str)
            self.assertEqual(a, os.path.join(src, 'foo'))
            self.assertEqual(b, os.path.join(dst, 'foo'))

        flag = []
        src = self.mkdtemp()
        dst = tempfile.mktemp(dir=self.mkdtemp())
        with open(os.path.join(src, 'foo'), 'w') as f:
            f.close()
        shutil.copytree(src, dst, copy_function=custom_cpfun)
        self.assertEqual(len(flag), 1)

    # Issue #3002: copyfile and copytree block indefinitely on named pipes
    @unittest.skipUnless(hasattr(os, "mkfifo"), 'requires os.mkfifo()')
    @support.skip_unless_symlink
    def test_copytree_named_pipe(self):
        os.mkdir(TESTFN)
        try:
            subdir = os.path.join(TESTFN, "subdir")
            os.mkdir(subdir)
            pipe = os.path.join(subdir, "mypipe")
            try:
                os.mkfifo(pipe)
            except PermissionError as e:
                self.skipTest('os.mkfifo(): %s' % e)
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

    @support.skip_unless_symlink
    def test_copytree_symlink_dir(self):
        src_dir = self.mkdtemp()
        dst_dir = os.path.join(self.mkdtemp(), 'destination')
        os.mkdir(os.path.join(src_dir, 'real_dir'))
        with open(os.path.join(src_dir, 'real_dir', 'test.txt'), 'w'):
            pass
        os.symlink(os.path.join(src_dir, 'real_dir'),
                   os.path.join(src_dir, 'link_to_dir'),
                   target_is_directory=True)

        shutil.copytree(src_dir, dst_dir, symlinks=False)
        self.assertFalse(os.path.islink(os.path.join(dst_dir, 'link_to_dir')))
        self.assertIn('test.txt', os.listdir(os.path.join(dst_dir, 'link_to_dir')))

        dst_dir = os.path.join(self.mkdtemp(), 'destination2')
        shutil.copytree(src_dir, dst_dir, symlinks=True)
        self.assertTrue(os.path.islink(os.path.join(dst_dir, 'link_to_dir')))
        self.assertIn('test.txt', os.listdir(os.path.join(dst_dir, 'link_to_dir')))

    def test_copytree_return_value(self):
        # copytree returns its destination path.
        src_dir = self.mkdtemp()
        dst_dir = src_dir + "dest"
        self.addCleanup(shutil.rmtree, dst_dir, True)
        src = os.path.join(src_dir, 'foo')
        write_file(src, 'foo')
        rv = shutil.copytree(src_dir, dst_dir)
        self.assertEqual(['foo'], os.listdir(rv))

    def test_copytree_subdirectory(self):
        # copytree where dst is a subdirectory of src, see Issue 38688
        base_dir = self.mkdtemp()
        self.addCleanup(shutil.rmtree, base_dir, ignore_errors=True)
        src_dir = os.path.join(base_dir, "t", "pg")
        dst_dir = os.path.join(src_dir, "somevendor", "1.0")
        os.makedirs(src_dir)
        src = os.path.join(src_dir, 'pol')
        write_file(src, 'pol')
        rv = shutil.copytree(src_dir, dst_dir)
        self.assertEqual(['pol'], os.listdir(rv))

class TestCopy(BaseTest, unittest.TestCase):

    ### shutil.copymode

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
        # On Windows, os.chmod does not follow symlinks (issue #15411)
        if os.name != 'nt':
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
            shutil.copymode(src_link, dst_link)
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
        shutil.copymode(src_link, dst_link, follow_symlinks=False)
        self.assertEqual(os.lstat(src_link).st_mode,
                         os.lstat(dst_link).st_mode)
        self.assertNotEqual(os.stat(src).st_mode, os.stat(dst).st_mode)
        # src link - use chmod
        os.lchmod(dst_link, stat.S_IRWXO)
        shutil.copymode(src_link, dst, follow_symlinks=False)
        self.assertEqual(os.stat(src).st_mode, os.stat(dst).st_mode)
        # dst link - use chmod
        os.lchmod(dst_link, stat.S_IRWXO)
        shutil.copymode(src, dst_link, follow_symlinks=False)
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
        shutil.copymode(src_link, dst_link, follow_symlinks=False)  # silent fail

    ### shutil.copystat

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
            shutil.copystat(src_link, dst_link, follow_symlinks=True)
            self.assertNotEqual(src_link_stat.st_mode, os.stat(dst).st_mode)
        # don't follow
        shutil.copystat(src_link, dst_link, follow_symlinks=False)
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
        shutil.copystat(src_link, dst, follow_symlinks=False)
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

    ### shutil.copyxattr

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
        self.assertEqual(sorted(os.listxattr(src)), sorted(os.listxattr(dst)))
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
        # the source filesystem not supporting xattrs should be ok, too.
        def _raise_on_src(fname, *, follow_symlinks=True):
            if fname == src:
                raise OSError(errno.ENOTSUP, 'Operation not supported')
            return orig_listxattr(fname, follow_symlinks=follow_symlinks)
        try:
            orig_listxattr = os.listxattr
            os.listxattr = _raise_on_src
            shutil._copyxattr(src, dst)
        finally:
            os.listxattr = orig_listxattr

        # test that shutil.copystat copies xattrs
        src = os.path.join(tmp_dir, 'the_original')
        srcro = os.path.join(tmp_dir, 'the_original_ro')
        write_file(src, src)
        write_file(srcro, srcro)
        os.setxattr(src, 'user.the_value', b'fiddly')
        os.setxattr(srcro, 'user.the_value', b'fiddly')
        os.chmod(srcro, 0o444)
        dst = os.path.join(tmp_dir, 'the_copy')
        dstro = os.path.join(tmp_dir, 'the_copy_ro')
        write_file(dst, dst)
        write_file(dstro, dstro)
        shutil.copystat(src, dst)
        shutil.copystat(srcro, dstro)
        self.assertEqual(os.getxattr(dst, 'user.the_value'), b'fiddly')
        self.assertEqual(os.getxattr(dstro, 'user.the_value'), b'fiddly')

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
        shutil._copyxattr(src_link, dst_link, follow_symlinks=False)
        self.assertEqual(os.getxattr(dst_link, 'trusted.foo', follow_symlinks=False), b'43')
        self.assertRaises(OSError, os.getxattr, dst, 'trusted.foo')
        shutil._copyxattr(src_link, dst, follow_symlinks=False)
        self.assertEqual(os.getxattr(dst, 'trusted.foo'), b'43')

    ### shutil.copy

    def _copy_file(self, method):
        fname = 'test.txt'
        tmpdir = self.mkdtemp()
        write_file((tmpdir, fname), 'xxx')
        file1 = os.path.join(tmpdir, fname)
        tmpdir2 = self.mkdtemp()
        method(file1, tmpdir2)
        file2 = os.path.join(tmpdir2, fname)
        return (file1, file2)

    def test_copy(self):
        # Ensure that the copied file exists and has the same mode bits.
        file1, file2 = self._copy_file(shutil.copy)
        self.assertTrue(os.path.exists(file2))
        self.assertEqual(os.stat(file1).st_mode, os.stat(file2).st_mode)

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
        shutil.copy(src_link, dst, follow_symlinks=True)
        self.assertFalse(os.path.islink(dst))
        self.assertEqual(read_file(src), read_file(dst))
        os.remove(dst)
        # follow
        shutil.copy(src_link, dst, follow_symlinks=False)
        self.assertTrue(os.path.islink(dst))
        self.assertEqual(os.readlink(dst), os.readlink(src_link))
        if hasattr(os, 'lchmod'):
            self.assertEqual(os.lstat(src_link).st_mode,
                             os.lstat(dst).st_mode)

    ### shutil.copy2

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
        shutil.copy2(src_link, dst, follow_symlinks=True)
        self.assertFalse(os.path.islink(dst))
        self.assertEqual(read_file(src), read_file(dst))
        os.remove(dst)
        # don't follow
        shutil.copy2(src_link, dst, follow_symlinks=False)
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

    def test_copy_dir(self):
        self._test_copy_dir(shutil.copy)

    def test_copy2_dir(self):
        self._test_copy_dir(shutil.copy2)

    def _test_copy_dir(self, copy_func):
        src_dir = self.mkdtemp()
        src_file = os.path.join(src_dir, 'foo')
        dir2 = self.mkdtemp()
        dst = os.path.join(src_dir, 'does_not_exist/')
        write_file(src_file, 'foo')
        if sys.platform == "win32":
            err = PermissionError
        else:
            err = IsADirectoryError
        self.assertRaises(err, copy_func, dir2, src_dir)

        # raise *err* because of src rather than FileNotFoundError because of dst
        self.assertRaises(err, copy_func, dir2, dst)
        copy_func(src_file, dir2)     # should not raise exceptions

    ### shutil.copyfile

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
        shutil.copyfile(link, dst_link, follow_symlinks=False)
        self.assertTrue(os.path.islink(dst_link))
        self.assertEqual(os.readlink(link), os.readlink(dst_link))
        # follow
        shutil.copyfile(link, dst)
        self.assertFalse(os.path.islink(dst))

    @unittest.skipUnless(hasattr(os, 'link'), 'requires os.link')
    def test_dont_copy_file_onto_link_to_itself(self):
        # bug 851123.
        os.mkdir(TESTFN)
        src = os.path.join(TESTFN, 'cheese')
        dst = os.path.join(TESTFN, 'shop')
        try:
            with open(src, 'w') as f:
                f.write('cheddar')
            try:
                os.link(src, dst)
            except PermissionError as e:
                self.skipTest('os.link(): %s' % e)
            self.assertRaises(shutil.SameFileError, shutil.copyfile, src, dst)
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
            self.assertRaises(shutil.SameFileError, shutil.copyfile, src, dst)
            with open(src, 'r') as f:
                self.assertEqual(f.read(), 'cheddar')
            os.remove(dst)
        finally:
            shutil.rmtree(TESTFN, ignore_errors=True)

    # Issue #3002: copyfile and copytree block indefinitely on named pipes
    @unittest.skipUnless(hasattr(os, "mkfifo"), 'requires os.mkfifo()')
    def test_copyfile_named_pipe(self):
        try:
            os.mkfifo(TESTFN)
        except PermissionError as e:
            self.skipTest('os.mkfifo(): %s' % e)
        try:
            self.assertRaises(shutil.SpecialFileError,
                                shutil.copyfile, TESTFN, TESTFN2)
            self.assertRaises(shutil.SpecialFileError,
                                shutil.copyfile, __file__, TESTFN)
        finally:
            os.remove(TESTFN)

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

    def test_copyfile_same_file(self):
        # copyfile() should raise SameFileError if the source and destination
        # are the same.
        src_dir = self.mkdtemp()
        src_file = os.path.join(src_dir, 'foo')
        write_file(src_file, 'foo')
        self.assertRaises(SameFileError, shutil.copyfile, src_file, src_file)
        # But Error should work too, to stay backward compatible.
        self.assertRaises(Error, shutil.copyfile, src_file, src_file)
        # Make sure file is not corrupted.
        self.assertEqual(read_file(src_file), 'foo')

    @unittest.skipIf(MACOS or SOLARIS or _winapi, 'On MACOS, Solaris and Windows the errors are not confusing (though different)')
    def test_copyfile_nonexistent_dir(self):
        # Issue 43219
        src_dir = self.mkdtemp()
        src_file = os.path.join(src_dir, 'foo')
        dst = os.path.join(src_dir, 'does_not_exist/')
        write_file(src_file, 'foo')
        self.assertRaises(FileNotFoundError, shutil.copyfile, src_file, dst)

    def test_copyfile_copy_dir(self):
        # Issue 45234
        # test copy() and copyfile() raising proper exceptions when src and/or
        # dst are directories
        src_dir = self.mkdtemp()
        src_file = os.path.join(src_dir, 'foo')
        dir2 = self.mkdtemp()
        dst = os.path.join(src_dir, 'does_not_exist/')
        write_file(src_file, 'foo')
        if sys.platform == "win32":
            err = PermissionError
        else:
            err = IsADirectoryError

        self.assertRaises(err, shutil.copyfile, src_dir, dst)
        self.assertRaises(err, shutil.copyfile, src_file, src_dir)
        self.assertRaises(err, shutil.copyfile, dir2, src_dir)


class TestArchives(BaseTest, unittest.TestCase):

    ### shutil.make_archive

    @support.requires_zlib()
    def test_make_tarball(self):
        # creating something to tar
        root_dir, base_dir = self._create_files('')

        tmpdir2 = self.mkdtemp()
        # force shutil to create the directory
        os.rmdir(tmpdir2)
        # working with relative paths
        work_dir = os.path.dirname(tmpdir2)
        rel_base_name = os.path.join(os.path.basename(tmpdir2), 'archive')

        with support.change_cwd(work_dir):
            base_name = os.path.abspath(rel_base_name)
            tarball = make_archive(rel_base_name, 'gztar', root_dir, '.')

        # check if the compressed tarball was created
        self.assertEqual(tarball, base_name + '.tar.gz')
        self.assertTrue(os.path.isfile(tarball))
        self.assertTrue(tarfile.is_tarfile(tarball))
        with tarfile.open(tarball, 'r:gz') as tf:
            self.assertCountEqual(tf.getnames(),
                                  ['.', './sub', './sub2',
                                   './file1', './file2', './sub/file3'])

        # trying an uncompressed one
        with support.change_cwd(work_dir):
            tarball = make_archive(rel_base_name, 'tar', root_dir, '.')
        self.assertEqual(tarball, base_name + '.tar')
        self.assertTrue(os.path.isfile(tarball))
        self.assertTrue(tarfile.is_tarfile(tarball))
        with tarfile.open(tarball, 'r') as tf:
            self.assertCountEqual(tf.getnames(),
                                  ['.', './sub', './sub2',
                                  './file1', './file2', './sub/file3'])

    def _tarinfo(self, path):
        with tarfile.open(path) as tar:
            names = tar.getnames()
            names.sort()
            return tuple(names)

    def _create_files(self, base_dir='dist'):
        # creating something to tar
        root_dir = self.mkdtemp()
        dist = os.path.join(root_dir, base_dir)
        os.makedirs(dist, exist_ok=True)
        write_file((dist, 'file1'), 'xxx')
        write_file((dist, 'file2'), 'xxx')
        os.mkdir(os.path.join(dist, 'sub'))
        write_file((dist, 'sub', 'file3'), 'xxx')
        os.mkdir(os.path.join(dist, 'sub2'))
        if base_dir:
            write_file((root_dir, 'outer'), 'xxx')
        return root_dir, base_dir

    @support.requires_zlib()
    @unittest.skipUnless(shutil.which('tar'),
                         'Need the tar command to run')
    def test_tarfile_vs_tar(self):
        root_dir, base_dir = self._create_files()
        base_name = os.path.join(self.mkdtemp(), 'archive')
        tarball = make_archive(base_name, 'gztar', root_dir, base_dir)

        # check if the compressed tarball was created
        self.assertEqual(tarball, base_name + '.tar.gz')
        self.assertTrue(os.path.isfile(tarball))

        # now create another tarball using `tar`
        tarball2 = os.path.join(root_dir, 'archive2.tar')
        tar_cmd = ['tar', '-cf', 'archive2.tar', base_dir]
        subprocess.check_call(tar_cmd, cwd=root_dir,
                              stdout=subprocess.DEVNULL)

        self.assertTrue(os.path.isfile(tarball2))
        # let's compare both tarballs
        self.assertEqual(self._tarinfo(tarball), self._tarinfo(tarball2))

        # trying an uncompressed one
        tarball = make_archive(base_name, 'tar', root_dir, base_dir)
        self.assertEqual(tarball, base_name + '.tar')
        self.assertTrue(os.path.isfile(tarball))

        # now for a dry_run
        tarball = make_archive(base_name, 'tar', root_dir, base_dir,
                               dry_run=True)
        self.assertEqual(tarball, base_name + '.tar')
        self.assertTrue(os.path.isfile(tarball))

    @support.requires_zlib()
    def test_make_zipfile(self):
        # creating something to zip
        root_dir, base_dir = self._create_files()

        tmpdir2 = self.mkdtemp()
        # force shutil to create the directory
        os.rmdir(tmpdir2)
        # working with relative paths
        work_dir = os.path.dirname(tmpdir2)
        rel_base_name = os.path.join(os.path.basename(tmpdir2), 'archive')

        with support.change_cwd(work_dir):
            base_name = os.path.abspath(rel_base_name)
            res = make_archive(rel_base_name, 'zip', root_dir)

        self.assertEqual(res, base_name + '.zip')
        self.assertTrue(os.path.isfile(res))
        self.assertTrue(zipfile.is_zipfile(res))
        with zipfile.ZipFile(res) as zf:
            self.assertCountEqual(zf.namelist(),
                    ['dist/', 'dist/sub/', 'dist/sub2/',
                     'dist/file1', 'dist/file2', 'dist/sub/file3',
                     'outer'])

        with support.change_cwd(work_dir):
            base_name = os.path.abspath(rel_base_name)
            res = make_archive(rel_base_name, 'zip', root_dir, base_dir)

        self.assertEqual(res, base_name + '.zip')
        self.assertTrue(os.path.isfile(res))
        self.assertTrue(zipfile.is_zipfile(res))
        with zipfile.ZipFile(res) as zf:
            self.assertCountEqual(zf.namelist(),
                    ['dist/', 'dist/sub/', 'dist/sub2/',
                     'dist/file1', 'dist/file2', 'dist/sub/file3'])

    @support.requires_zlib()
    @unittest.skipUnless(shutil.which('zip'),
                         'Need the zip command to run')
    def test_zipfile_vs_zip(self):
        root_dir, base_dir = self._create_files()
        base_name = os.path.join(self.mkdtemp(), 'archive')
        archive = make_archive(base_name, 'zip', root_dir, base_dir)

        # check if ZIP file  was created
        self.assertEqual(archive, base_name + '.zip')
        self.assertTrue(os.path.isfile(archive))

        # now create another ZIP file using `zip`
        archive2 = os.path.join(root_dir, 'archive2.zip')
        zip_cmd = ['zip', '-q', '-r', 'archive2.zip', base_dir]
        subprocess.check_call(zip_cmd, cwd=root_dir,
                              stdout=subprocess.DEVNULL)

        self.assertTrue(os.path.isfile(archive2))
        # let's compare both ZIP files
        with zipfile.ZipFile(archive) as zf:
            names = zf.namelist()
        with zipfile.ZipFile(archive2) as zf:
            names2 = zf.namelist()
        self.assertEqual(sorted(names), sorted(names2))

    @support.requires_zlib()
    @unittest.skipUnless(shutil.which('unzip'),
                         'Need the unzip command to run')
    def test_unzip_zipfile(self):
        root_dir, base_dir = self._create_files()
        base_name = os.path.join(self.mkdtemp(), 'archive')
        archive = make_archive(base_name, 'zip', root_dir, base_dir)

        # check if ZIP file  was created
        self.assertEqual(archive, base_name + '.zip')
        self.assertTrue(os.path.isfile(archive))

        # now check the ZIP file using `unzip -t`
        zip_cmd = ['unzip', '-t', archive]
        with support.change_cwd(root_dir):
            try:
                subprocess.check_output(zip_cmd, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as exc:
                details = exc.output.decode(errors="replace")
                if 'unrecognized option: t' in details:
                    self.skipTest("unzip doesn't support -t")
                msg = "{}\n\n**Unzip Output**\n{}"
                self.fail(msg.format(exc, details))

    def test_make_archive(self):
        tmpdir = self.mkdtemp()
        base_name = os.path.join(tmpdir, 'archive')
        self.assertRaises(ValueError, make_archive, base_name, 'xxx')

    @support.requires_zlib()
    def test_make_archive_owner_group(self):
        # testing make_archive with owner and group, with various combinations
        # this works even if there's not gid/uid support
        if UID_GID_SUPPORT:
            group = grp.getgrgid(0)[0]
            owner = pwd.getpwuid(0)[0]
        else:
            group = owner = 'root'

        root_dir, base_dir = self._create_files()
        base_name = os.path.join(self.mkdtemp(), 'archive')
        res = make_archive(base_name, 'zip', root_dir, base_dir, owner=owner,
                           group=group)
        self.assertTrue(os.path.isfile(res))

        res = make_archive(base_name, 'zip', root_dir, base_dir)
        self.assertTrue(os.path.isfile(res))

        res = make_archive(base_name, 'tar', root_dir, base_dir,
                           owner=owner, group=group)
        self.assertTrue(os.path.isfile(res))

        res = make_archive(base_name, 'tar', root_dir, base_dir,
                           owner='kjhkjhkjg', group='oihohoh')
        self.assertTrue(os.path.isfile(res))


    @support.requires_zlib()
    @unittest.skipUnless(UID_GID_SUPPORT, "Requires grp and pwd support")
    def test_tarfile_root_owner(self):
        root_dir, base_dir = self._create_files()
        base_name = os.path.join(self.mkdtemp(), 'archive')
        group = grp.getgrgid(0)[0]
        owner = pwd.getpwuid(0)[0]
        with support.change_cwd(root_dir):
            archive_name = make_archive(base_name, 'gztar', root_dir, 'dist',
                                        owner=owner, group=group)

        # check if the compressed tarball was created
        self.assertTrue(os.path.isfile(archive_name))

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

    def test_make_tarfile_in_curdir(self):
        # Issue #21280
        root_dir = self.mkdtemp()
        with support.change_cwd(root_dir):
            self.assertEqual(make_archive('test', 'tar'), 'test.tar')
            self.assertTrue(os.path.isfile('test.tar'))

    @support.requires_zlib()
    def test_make_zipfile_in_curdir(self):
        # Issue #21280
        root_dir = self.mkdtemp()
        with support.change_cwd(root_dir):
            self.assertEqual(make_archive('test', 'zip'), 'test.zip')
            self.assertTrue(os.path.isfile('test.zip'))

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

    ### shutil.unpack_archive

    def check_unpack_archive(self, format):
        self.check_unpack_archive_with_converter(format, lambda path: path)
        self.check_unpack_archive_with_converter(format, pathlib.Path)
        self.check_unpack_archive_with_converter(format, FakePath)

    def check_unpack_archive_with_converter(self, format, converter):
        root_dir, base_dir = self._create_files()
        expected = rlistdir(root_dir)
        expected.remove('outer')

        base_name = os.path.join(self.mkdtemp(), 'archive')
        filename = make_archive(base_name, format, root_dir, base_dir)

        # let's try to unpack it now
        tmpdir2 = self.mkdtemp()
        unpack_archive(converter(filename), converter(tmpdir2))
        self.assertEqual(rlistdir(tmpdir2), expected)

        # and again, this time with the format specified
        tmpdir3 = self.mkdtemp()
        unpack_archive(converter(filename), converter(tmpdir3), format=format)
        self.assertEqual(rlistdir(tmpdir3), expected)

        self.assertRaises(shutil.ReadError, unpack_archive, converter(TESTFN))
        self.assertRaises(ValueError, unpack_archive, converter(TESTFN), format='xxx')

    def test_unpack_archive_tar(self):
        self.check_unpack_archive('tar')

    @support.requires_zlib()
    def test_unpack_archive_gztar(self):
        self.check_unpack_archive('gztar')

    @support.requires_bz2()
    def test_unpack_archive_bztar(self):
        self.check_unpack_archive('bztar')

    @support.requires_lzma()
    @unittest.skipIf(AIX and not _maxdataOK(), "AIX MAXDATA must be 0x20000000 or larger")
    def test_unpack_archive_xztar(self):
        self.check_unpack_archive('xztar')

    @support.requires_zlib()
    def test_unpack_archive_zip(self):
        self.check_unpack_archive('zip')

    def test_unpack_registry(self):

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


class TestMisc(BaseTest, unittest.TestCase):

    @unittest.skipUnless(hasattr(shutil, 'disk_usage'),
                         "disk_usage not available on this platform")
    def test_disk_usage(self):
        usage = shutil.disk_usage(os.path.dirname(__file__))
        for attr in ('total', 'used', 'free'):
            self.assertIsInstance(getattr(usage, attr), int)
        self.assertGreater(usage.total, 0)
        self.assertGreater(usage.used, 0)
        self.assertGreaterEqual(usage.free, 0)
        self.assertGreaterEqual(usage.total, usage.used)
        self.assertGreater(usage.total, usage.free)

        # bpo-32557: Check that disk_usage() also accepts a filename
        shutil.disk_usage(__file__)

    @unittest.skipUnless(UID_GID_SUPPORT, "Requires grp and pwd support")
    @unittest.skipUnless(hasattr(os, 'chown'), 'requires os.chown')
    def test_chown(self):
        dirname = self.mkdtemp()
        filename = tempfile.mktemp(dir=dirname)
        write_file(filename, 'testing chown function')

        with self.assertRaises(ValueError):
            shutil.chown(filename)

        with self.assertRaises(LookupError):
            shutil.chown(filename, user='non-existing username')

        with self.assertRaises(LookupError):
            shutil.chown(filename, group='non-existing groupname')

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

        try:
            user = pwd.getpwuid(uid)[0]
            group = grp.getgrgid(gid)[0]
        except KeyError:
            # On some systems uid/gid cannot be resolved.
            pass
        else:
            shutil.chown(filename, user, group)
            check_chown(filename, uid, gid)
            shutil.chown(dirname, user, group)
            check_chown(dirname, uid, gid)


class TestWhich(BaseTest, unittest.TestCase):

    def setUp(self):
        self.temp_dir = self.mkdtemp(prefix="Tmp")
        # Give the temp_file an ".exe" suffix for all.
        # It's needed on Windows and not harmful on other platforms.
        self.temp_file = tempfile.NamedTemporaryFile(dir=self.temp_dir,
                                                     prefix="Tmp",
                                                     suffix=".Exe")
        os.chmod(self.temp_file.name, stat.S_IXUSR)
        self.addCleanup(self.temp_file.close)
        self.dir, self.file = os.path.split(self.temp_file.name)
        self.env_path = self.dir
        self.curdir = os.curdir
        self.ext = ".EXE"

    def test_basic(self):
        # Given an EXE in a directory, it should be returned.
        rv = shutil.which(self.file, path=self.dir)
        self.assertEqual(rv, self.temp_file.name)

    def test_absolute_cmd(self):
        # When given the fully qualified path to an executable that exists,
        # it should be returned.
        rv = shutil.which(self.temp_file.name, path=self.temp_dir)
        self.assertEqual(rv, self.temp_file.name)

    def test_relative_cmd(self):
        # When given the relative path with a directory part to an executable
        # that exists, it should be returned.
        base_dir, tail_dir = os.path.split(self.dir)
        relpath = os.path.join(tail_dir, self.file)
        with support.change_cwd(path=base_dir):
            rv = shutil.which(relpath, path=self.temp_dir)
            self.assertEqual(rv, relpath)
        # But it shouldn't be searched in PATH directories (issue #16957).
        with support.change_cwd(path=self.dir):
            rv = shutil.which(relpath, path=base_dir)
            self.assertIsNone(rv)

    def test_cwd(self):
        # Issue #16957
        base_dir = os.path.dirname(self.dir)
        with support.change_cwd(path=self.dir):
            rv = shutil.which(self.file, path=base_dir)
            if sys.platform == "win32":
                # Windows: current directory implicitly on PATH
                self.assertEqual(rv, os.path.join(self.curdir, self.file))
            else:
                # Other platforms: shouldn't match in the current directory.
                self.assertIsNone(rv)

    @unittest.skipIf(hasattr(os, 'geteuid') and os.geteuid() == 0,
                     'non-root user required')
    def test_non_matching_mode(self):
        # Set the file read-only and ask for writeable files.
        os.chmod(self.temp_file.name, stat.S_IREAD)
        if os.access(self.temp_file.name, os.W_OK):
            self.skipTest("can't set the file read-only")
        rv = shutil.which(self.file, path=self.dir, mode=os.W_OK)
        self.assertIsNone(rv)

    def test_relative_path(self):
        base_dir, tail_dir = os.path.split(self.dir)
        with support.change_cwd(path=base_dir):
            rv = shutil.which(self.file, path=tail_dir)
            self.assertEqual(rv, os.path.join(tail_dir, self.file))

    def test_nonexistent_file(self):
        # Return None when no matching executable file is found on the path.
        rv = shutil.which("foo.exe", path=self.dir)
        self.assertIsNone(rv)

    @unittest.skipUnless(sys.platform == "win32",
                         "pathext check is Windows-only")
    def test_pathext_checking(self):
        # Ask for the file without the ".exe" extension, then ensure that
        # it gets found properly with the extension.
        rv = shutil.which(self.file[:-4], path=self.dir)
        self.assertEqual(rv, self.temp_file.name[:-4] + self.ext)

    def test_environ_path(self):
        with support.EnvironmentVarGuard() as env:
            env['PATH'] = self.env_path
            rv = shutil.which(self.file)
            self.assertEqual(rv, self.temp_file.name)

    def test_environ_path_empty(self):
        # PATH='': no match
        with support.EnvironmentVarGuard() as env:
            env['PATH'] = ''
            with unittest.mock.patch('os.confstr', return_value=self.dir, \
                                     create=True), \
                 support.swap_attr(os, 'defpath', self.dir), \
                 support.change_cwd(self.dir):
                rv = shutil.which(self.file)
                self.assertIsNone(rv)

    def test_environ_path_cwd(self):
        expected_cwd = os.path.basename(self.temp_file.name)
        if sys.platform == "win32":
            curdir = os.curdir
            if isinstance(expected_cwd, bytes):
                curdir = os.fsencode(curdir)
            expected_cwd = os.path.join(curdir, expected_cwd)

        # PATH=':': explicitly looks in the current directory
        with support.EnvironmentVarGuard() as env:
            env['PATH'] = os.pathsep
            with unittest.mock.patch('os.confstr', return_value=self.dir, \
                                     create=True), \
                 support.swap_attr(os, 'defpath', self.dir):
                rv = shutil.which(self.file)
                self.assertIsNone(rv)

                # look in current directory
                with support.change_cwd(self.dir):
                    rv = shutil.which(self.file)
                    self.assertEqual(rv, expected_cwd)

    def test_environ_path_missing(self):
        with support.EnvironmentVarGuard() as env:
            env.pop('PATH', None)

            # without confstr
            with unittest.mock.patch('os.confstr', side_effect=ValueError, \
                                     create=True), \
                 support.swap_attr(os, 'defpath', self.dir):
                rv = shutil.which(self.file)
            self.assertEqual(rv, self.temp_file.name)

            # with confstr
            with unittest.mock.patch('os.confstr', return_value=self.dir, \
                                     create=True), \
                 support.swap_attr(os, 'defpath', ''):
                rv = shutil.which(self.file)
            self.assertEqual(rv, self.temp_file.name)

    def test_empty_path(self):
        base_dir = os.path.dirname(self.dir)
        with support.change_cwd(path=self.dir), \
             support.EnvironmentVarGuard() as env:
            env['PATH'] = self.env_path
            rv = shutil.which(self.file, path='')
            self.assertIsNone(rv)

    def test_empty_path_no_PATH(self):
        with support.EnvironmentVarGuard() as env:
            env.pop('PATH', None)
            rv = shutil.which(self.file)
            self.assertIsNone(rv)

    @unittest.skipUnless(sys.platform == "win32", 'test specific to Windows')
    def test_pathext(self):
        ext = ".xyz"
        temp_filexyz = tempfile.NamedTemporaryFile(dir=self.temp_dir,
                                                   prefix="Tmp2", suffix=ext)
        os.chmod(temp_filexyz.name, stat.S_IXUSR)
        self.addCleanup(temp_filexyz.close)

        # strip path and extension
        program = os.path.basename(temp_filexyz.name)
        program = os.path.splitext(program)[0]

        with support.EnvironmentVarGuard() as env:
            env['PATHEXT'] = ext
            rv = shutil.which(program, path=self.temp_dir)
            self.assertEqual(rv, temp_filexyz.name)

    # Issue 40592: See https://bugs.python.org/issue40592
    @unittest.skipUnless(sys.platform == "win32", 'test specific to Windows')
    def test_pathext_with_empty_str(self):
        ext = ".xyz"
        temp_filexyz = tempfile.NamedTemporaryFile(dir=self.temp_dir,
                                                   prefix="Tmp2", suffix=ext)
        self.addCleanup(temp_filexyz.close)

        # strip path and extension
        program = os.path.basename(temp_filexyz.name)
        program = os.path.splitext(program)[0]

        with support.EnvironmentVarGuard() as env:
            env['PATHEXT'] = f"{ext};"  # note the ;
            rv = shutil.which(program, path=self.temp_dir)
            self.assertEqual(rv, temp_filexyz.name)


class TestWhichBytes(TestWhich):
    def setUp(self):
        TestWhich.setUp(self)
        self.dir = os.fsencode(self.dir)
        self.file = os.fsencode(self.file)
        self.temp_file.name = os.fsencode(self.temp_file.name)
        self.curdir = os.fsencode(self.curdir)
        self.ext = os.fsencode(self.ext)


class TestMove(BaseTest, unittest.TestCase):

    def setUp(self):
        filename = "foo"
        self.src_dir = self.mkdtemp()
        self.dst_dir = self.mkdtemp()
        self.src_file = os.path.join(self.src_dir, filename)
        self.dst_file = os.path.join(self.dst_dir, filename)
        with open(self.src_file, "wb") as f:
            f.write(b"spam")

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

    def test_move_file_to_dir_pathlike_src(self):
        # Move a pathlike file to another location on the same filesystem.
        src = pathlib.Path(self.src_file)
        self._check_move_file(src, self.dst_dir, self.dst_file)

    def test_move_file_to_dir_pathlike_dst(self):
        # Move a file to another pathlike location on the same filesystem.
        dst = pathlib.Path(self.dst_dir)
        self._check_move_file(self.src_file, dst, self.dst_file)

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
        dst_dir = tempfile.mktemp(dir=self.mkdtemp())
        try:
            self._check_move_dir(self.src_dir, dst_dir, dst_dir)
        finally:
            support.rmtree(dst_dir)

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

    def test_move_dir_sep_to_dir(self):
        self._check_move_dir(self.src_dir + os.path.sep, self.dst_dir,
            os.path.join(self.dst_dir, os.path.basename(self.src_dir)))

    @unittest.skipUnless(os.path.altsep, 'requires os.path.altsep')
    def test_move_dir_altsep_to_dir(self):
        self._check_move_dir(self.src_dir + os.path.altsep, self.dst_dir,
            os.path.join(self.dst_dir, os.path.basename(self.src_dir)))

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
            support.rmtree(TESTFN)

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
            support.rmtree(TESTFN)

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

    @mock_rename
    def test_move_file_special_function(self):
        moved = []
        def _copy(src, dst):
            moved.append((src, dst))
        shutil.move(self.src_file, self.dst_dir, copy_function=_copy)
        self.assertEqual(len(moved), 1)

    @mock_rename
    def test_move_dir_special_function(self):
        moved = []
        def _copy(src, dst):
            moved.append((src, dst))
        support.create_empty_file(os.path.join(self.src_dir, 'child'))
        support.create_empty_file(os.path.join(self.src_dir, 'child1'))
        shutil.move(self.src_dir, self.dst_dir, copy_function=_copy)
        self.assertEqual(len(moved), 3)

    def test_move_dir_caseinsensitive(self):
        # Renames a folder to the same name
        # but a different case.

        self.src_dir = self.mkdtemp()
        dst_dir = os.path.join(
                os.path.dirname(self.src_dir),
                os.path.basename(self.src_dir).upper())
        self.assertNotEqual(self.src_dir, dst_dir)

        try:
            shutil.move(self.src_dir, dst_dir)
            self.assertTrue(os.path.isdir(dst_dir))
        finally:
            os.rmdir(dst_dir)


    @unittest.skipUnless(hasattr(os, 'geteuid') and os.geteuid() == 0
                         and hasattr(os, 'lchflags')
                         and hasattr(stat, 'SF_IMMUTABLE')
                         and hasattr(stat, 'UF_OPAQUE'),
                         'root privileges required')
    def test_move_dir_permission_denied(self):
        # bpo-42782: shutil.move should not create destination directories
        # if the source directory cannot be removed.
        try:
            os.mkdir(TESTFN_SRC)
            os.lchflags(TESTFN_SRC, stat.SF_IMMUTABLE)

            # Testing on an empty immutable directory
            # TESTFN_DST should not exist if shutil.move failed
            self.assertRaises(PermissionError, shutil.move, TESTFN_SRC, TESTFN_DST)
            self.assertFalse(TESTFN_DST in os.listdir())

            # Create a file and keep the directory immutable
            os.lchflags(TESTFN_SRC, stat.UF_OPAQUE)
            support.create_empty_file(os.path.join(TESTFN_SRC, 'child'))
            os.lchflags(TESTFN_SRC, stat.SF_IMMUTABLE)

            # Testing on a non-empty immutable directory
            # TESTFN_DST should not exist if shutil.move failed
            self.assertRaises(PermissionError, shutil.move, TESTFN_SRC, TESTFN_DST)
            self.assertFalse(TESTFN_DST in os.listdir())
        finally:
            if os.path.exists(TESTFN_SRC):
                os.lchflags(TESTFN_SRC, stat.UF_OPAQUE)
                support.rmtree(TESTFN_SRC)
            if os.path.exists(TESTFN_DST):
                os.lchflags(TESTFN_DST, stat.UF_OPAQUE)
                support.rmtree(TESTFN_DST)


class TestCopyFile(unittest.TestCase):

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
                raise OSError("Cannot close")
            return self._suppress_at_exit

    def test_w_source_open_fails(self):
        def _open(filename, mode='r'):
            if filename == 'srcfile':
                raise OSError('Cannot open "srcfile"')
            assert 0  # shouldn't reach here.

        with support.swap_attr(shutil, 'open', _open):
            with self.assertRaises(OSError):
                shutil.copyfile('srcfile', 'destfile')

    @unittest.skipIf(MACOS, "skipped on macOS")
    def test_w_dest_open_fails(self):
        srcfile = self.Faux()

        def _open(filename, mode='r'):
            if filename == 'srcfile':
                return srcfile
            if filename == 'destfile':
                raise OSError('Cannot open "destfile"')
            assert 0  # shouldn't reach here.

        with support.swap_attr(shutil, 'open', _open):
            shutil.copyfile('srcfile', 'destfile')
        self.assertTrue(srcfile._entered)
        self.assertTrue(srcfile._exited_with[0] is OSError)
        self.assertEqual(srcfile._exited_with[1].args,
                         ('Cannot open "destfile"',))

    @unittest.skipIf(MACOS, "skipped on macOS")
    def test_w_dest_close_fails(self):
        srcfile = self.Faux()
        destfile = self.Faux(True)

        def _open(filename, mode='r'):
            if filename == 'srcfile':
                return srcfile
            if filename == 'destfile':
                return destfile
            assert 0  # shouldn't reach here.

        with support.swap_attr(shutil, 'open', _open):
            shutil.copyfile('srcfile', 'destfile')
        self.assertTrue(srcfile._entered)
        self.assertTrue(destfile._entered)
        self.assertTrue(destfile._raised)
        self.assertTrue(srcfile._exited_with[0] is OSError)
        self.assertEqual(srcfile._exited_with[1].args,
                         ('Cannot close',))

    @unittest.skipIf(MACOS, "skipped on macOS")
    def test_w_source_close_fails(self):

        srcfile = self.Faux(True)
        destfile = self.Faux()

        def _open(filename, mode='r'):
            if filename == 'srcfile':
                return srcfile
            if filename == 'destfile':
                return destfile
            assert 0  # shouldn't reach here.

        with support.swap_attr(shutil, 'open', _open):
            with self.assertRaises(OSError):
                shutil.copyfile('srcfile', 'destfile')
        self.assertTrue(srcfile._entered)
        self.assertTrue(destfile._entered)
        self.assertFalse(destfile._raised)
        self.assertTrue(srcfile._exited_with[0] is None)
        self.assertTrue(srcfile._raised)


class TestCopyFileObj(unittest.TestCase):
    FILESIZE = 2 * 1024 * 1024

    @classmethod
    def setUpClass(cls):
        write_test_file(TESTFN, cls.FILESIZE)

    @classmethod
    def tearDownClass(cls):
        support.unlink(TESTFN)
        support.unlink(TESTFN2)

    def tearDown(self):
        support.unlink(TESTFN2)

    @contextlib.contextmanager
    def get_files(self):
        with open(TESTFN, "rb") as src:
            with open(TESTFN2, "wb") as dst:
                yield (src, dst)

    def assert_files_eq(self, src, dst):
        with open(src, 'rb') as fsrc:
            with open(dst, 'rb') as fdst:
                self.assertEqual(fsrc.read(), fdst.read())

    def test_content(self):
        with self.get_files() as (src, dst):
            shutil.copyfileobj(src, dst)
        self.assert_files_eq(TESTFN, TESTFN2)

    def test_file_not_closed(self):
        with self.get_files() as (src, dst):
            shutil.copyfileobj(src, dst)
            assert not src.closed
            assert not dst.closed

    def test_file_offset(self):
        with self.get_files() as (src, dst):
            shutil.copyfileobj(src, dst)
            self.assertEqual(src.tell(), self.FILESIZE)
            self.assertEqual(dst.tell(), self.FILESIZE)

    @unittest.skipIf(os.name != 'nt', "Windows only")
    def test_win_impl(self):
        # Make sure alternate Windows implementation is called.
        with unittest.mock.patch("shutil._copyfileobj_readinto") as m:
            shutil.copyfile(TESTFN, TESTFN2)
        assert m.called

        # File size is 2 MiB but max buf size should be 1 MiB.
        self.assertEqual(m.call_args[0][2], 1 * 1024 * 1024)

        # If file size < 1 MiB memoryview() length must be equal to
        # the actual file size.
        with tempfile.NamedTemporaryFile(dir=os.getcwd(), delete=False) as f:
            f.write(b'foo')
        fname = f.name
        self.addCleanup(support.unlink, fname)
        with unittest.mock.patch("shutil._copyfileobj_readinto") as m:
            shutil.copyfile(fname, TESTFN2)
        self.assertEqual(m.call_args[0][2], 3)

        # Empty files should not rely on readinto() variant.
        with tempfile.NamedTemporaryFile(dir=os.getcwd(), delete=False) as f:
            pass
        fname = f.name
        self.addCleanup(support.unlink, fname)
        with unittest.mock.patch("shutil._copyfileobj_readinto") as m:
            shutil.copyfile(fname, TESTFN2)
        assert not m.called
        self.assert_files_eq(fname, TESTFN2)


class _ZeroCopyFileTest(object):
    """Tests common to all zero-copy APIs."""
    FILESIZE = (10 * 1024 * 1024)  # 10 MiB
    FILEDATA = b""
    PATCHPOINT = ""

    @classmethod
    def setUpClass(cls):
        write_test_file(TESTFN, cls.FILESIZE)
        with open(TESTFN, 'rb') as f:
            cls.FILEDATA = f.read()
            assert len(cls.FILEDATA) == cls.FILESIZE

    @classmethod
    def tearDownClass(cls):
        support.unlink(TESTFN)

    def tearDown(self):
        support.unlink(TESTFN2)

    @contextlib.contextmanager
    def get_files(self):
        with open(TESTFN, "rb") as src:
            with open(TESTFN2, "wb") as dst:
                yield (src, dst)

    def zerocopy_fun(self, *args, **kwargs):
        raise NotImplementedError("must be implemented in subclass")

    def reset(self):
        self.tearDown()
        self.tearDownClass()
        self.setUpClass()
        self.setUp()

    # ---

    def test_regular_copy(self):
        with self.get_files() as (src, dst):
            self.zerocopy_fun(src, dst)
        self.assertEqual(read_file(TESTFN2, binary=True), self.FILEDATA)
        # Make sure the fallback function is not called.
        with self.get_files() as (src, dst):
            with unittest.mock.patch('shutil.copyfileobj') as m:
                shutil.copyfile(TESTFN, TESTFN2)
            assert not m.called

    def test_same_file(self):
        self.addCleanup(self.reset)
        with self.get_files() as (src, dst):
            with self.assertRaises(Exception):
                self.zerocopy_fun(src, src)
        # Make sure src file is not corrupted.
        self.assertEqual(read_file(TESTFN, binary=True), self.FILEDATA)

    def test_non_existent_src(self):
        name = tempfile.mktemp(dir=os.getcwd())
        with self.assertRaises(FileNotFoundError) as cm:
            shutil.copyfile(name, "new")
        self.assertEqual(cm.exception.filename, name)

    def test_empty_file(self):
        srcname = TESTFN + 'src'
        dstname = TESTFN + 'dst'
        self.addCleanup(lambda: support.unlink(srcname))
        self.addCleanup(lambda: support.unlink(dstname))
        with open(srcname, "wb"):
            pass

        with open(srcname, "rb") as src:
            with open(dstname, "wb") as dst:
                self.zerocopy_fun(src, dst)

        self.assertEqual(read_file(dstname, binary=True), b"")

    def test_unhandled_exception(self):
        with unittest.mock.patch(self.PATCHPOINT,
                                 side_effect=ZeroDivisionError):
            self.assertRaises(ZeroDivisionError,
                              shutil.copyfile, TESTFN, TESTFN2)

    def test_exception_on_first_call(self):
        # Emulate a case where the first call to the zero-copy
        # function raises an exception in which case the function is
        # supposed to give up immediately.
        with unittest.mock.patch(self.PATCHPOINT,
                                 side_effect=OSError(errno.EINVAL, "yo")):
            with self.get_files() as (src, dst):
                with self.assertRaises(_GiveupOnFastCopy):
                    self.zerocopy_fun(src, dst)

    def test_filesystem_full(self):
        # Emulate a case where filesystem is full and sendfile() fails
        # on first call.
        with unittest.mock.patch(self.PATCHPOINT,
                                 side_effect=OSError(errno.ENOSPC, "yo")):
            with self.get_files() as (src, dst):
                self.assertRaises(OSError, self.zerocopy_fun, src, dst)


@unittest.skipIf(not SUPPORTS_SENDFILE, 'os.sendfile() not supported')
class TestZeroCopySendfile(_ZeroCopyFileTest, unittest.TestCase):
    PATCHPOINT = "os.sendfile"

    def zerocopy_fun(self, fsrc, fdst):
        return shutil._fastcopy_sendfile(fsrc, fdst)

    def test_non_regular_file_src(self):
        with io.BytesIO(self.FILEDATA) as src:
            with open(TESTFN2, "wb") as dst:
                with self.assertRaises(_GiveupOnFastCopy):
                    self.zerocopy_fun(src, dst)
                shutil.copyfileobj(src, dst)

        self.assertEqual(read_file(TESTFN2, binary=True), self.FILEDATA)

    def test_non_regular_file_dst(self):
        with open(TESTFN, "rb") as src:
            with io.BytesIO() as dst:
                with self.assertRaises(_GiveupOnFastCopy):
                    self.zerocopy_fun(src, dst)
                shutil.copyfileobj(src, dst)
                dst.seek(0)
                self.assertEqual(dst.read(), self.FILEDATA)

    def test_exception_on_second_call(self):
        def sendfile(*args, **kwargs):
            if not flag:
                flag.append(None)
                return orig_sendfile(*args, **kwargs)
            else:
                raise OSError(errno.EBADF, "yo")

        flag = []
        orig_sendfile = os.sendfile
        with unittest.mock.patch('os.sendfile', create=True,
                                 side_effect=sendfile):
            with self.get_files() as (src, dst):
                with self.assertRaises(OSError) as cm:
                    shutil._fastcopy_sendfile(src, dst)
        assert flag
        self.assertEqual(cm.exception.errno, errno.EBADF)

    def test_cant_get_size(self):
        # Emulate a case where src file size cannot be determined.
        # Internally bufsize will be set to a small value and
        # sendfile() will be called repeatedly.
        with unittest.mock.patch('os.fstat', side_effect=OSError) as m:
            with self.get_files() as (src, dst):
                shutil._fastcopy_sendfile(src, dst)
                assert m.called
        self.assertEqual(read_file(TESTFN2, binary=True), self.FILEDATA)

    def test_small_chunks(self):
        # Force internal file size detection to be smaller than the
        # actual file size. We want to force sendfile() to be called
        # multiple times, also in order to emulate a src fd which gets
        # bigger while it is being copied.
        mock = unittest.mock.Mock()
        mock.st_size = 65536 + 1
        with unittest.mock.patch('os.fstat', return_value=mock) as m:
            with self.get_files() as (src, dst):
                shutil._fastcopy_sendfile(src, dst)
                assert m.called
        self.assertEqual(read_file(TESTFN2, binary=True), self.FILEDATA)

    def test_big_chunk(self):
        # Force internal file size detection to be +100MB bigger than
        # the actual file size. Make sure sendfile() does not rely on
        # file size value except for (maybe) a better throughput /
        # performance.
        mock = unittest.mock.Mock()
        mock.st_size = self.FILESIZE + (100 * 1024 * 1024)
        with unittest.mock.patch('os.fstat', return_value=mock) as m:
            with self.get_files() as (src, dst):
                shutil._fastcopy_sendfile(src, dst)
                assert m.called
        self.assertEqual(read_file(TESTFN2, binary=True), self.FILEDATA)

    def test_blocksize_arg(self):
        with unittest.mock.patch('os.sendfile',
                                 side_effect=ZeroDivisionError) as m:
            self.assertRaises(ZeroDivisionError,
                              shutil.copyfile, TESTFN, TESTFN2)
            blocksize = m.call_args[0][3]
            # Make sure file size and the block size arg passed to
            # sendfile() are the same.
            self.assertEqual(blocksize, os.path.getsize(TESTFN))
            # ...unless we're dealing with a small file.
            support.unlink(TESTFN2)
            write_file(TESTFN2, b"hello", binary=True)
            self.addCleanup(support.unlink, TESTFN2 + '3')
            self.assertRaises(ZeroDivisionError,
                              shutil.copyfile, TESTFN2, TESTFN2 + '3')
            blocksize = m.call_args[0][3]
            self.assertEqual(blocksize, 2 ** 23)

    def test_file2file_not_supported(self):
        # Emulate a case where sendfile() only support file->socket
        # fds. In such a case copyfile() is supposed to skip the
        # fast-copy attempt from then on.
        assert shutil._USE_CP_SENDFILE
        try:
            with unittest.mock.patch(
                    self.PATCHPOINT,
                    side_effect=OSError(errno.ENOTSOCK, "yo")) as m:
                with self.get_files() as (src, dst):
                    with self.assertRaises(_GiveupOnFastCopy):
                        shutil._fastcopy_sendfile(src, dst)
                assert m.called
            assert not shutil._USE_CP_SENDFILE

            with unittest.mock.patch(self.PATCHPOINT) as m:
                shutil.copyfile(TESTFN, TESTFN2)
                assert not m.called
        finally:
            shutil._USE_CP_SENDFILE = True


@unittest.skipIf(not MACOS, 'macOS only')
class TestZeroCopyMACOS(_ZeroCopyFileTest, unittest.TestCase):
    PATCHPOINT = "posix._fcopyfile"

    def zerocopy_fun(self, src, dst):
        return shutil._fastcopy_fcopyfile(src, dst, posix._COPYFILE_DATA)


class TestGetTerminalSize(unittest.TestCase):
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
            del env['LINES']
            size = shutil.get_terminal_size()
        self.assertEqual(size.columns, 777)

        with support.EnvironmentVarGuard() as env:
            del env['COLUMNS']
            env['LINES'] = '888'
            size = shutil.get_terminal_size()
        self.assertEqual(size.lines, 888)

    def test_bad_environ(self):
        with support.EnvironmentVarGuard() as env:
            env['COLUMNS'] = 'xxx'
            env['LINES'] = 'yyy'
            size = shutil.get_terminal_size()
        self.assertGreaterEqual(size.columns, 0)
        self.assertGreaterEqual(size.lines, 0)

    @unittest.skipUnless(os.isatty(sys.__stdout__.fileno()), "not on tty")
    @unittest.skipUnless(hasattr(os, 'get_terminal_size'),
                         'need os.get_terminal_size()')
    def test_stty_match(self):
        """Check if stty returns the same results ignoring env

        This test will fail if stdin and stdout are connected to
        different terminals with different sizes. Nevertheless, such
        situations should be pretty rare.
        """
        try:
            size = subprocess.check_output(['stty', 'size']).decode().split()
        except (FileNotFoundError, PermissionError,
                subprocess.CalledProcessError):
            self.skipTest("stty invocation failed")
        expected = (int(size[1]), int(size[0])) # reversed order

        with support.EnvironmentVarGuard() as env:
            del env['LINES']
            del env['COLUMNS']
            actual = shutil.get_terminal_size()

        self.assertEqual(expected, actual)

    def test_fallback(self):
        with support.EnvironmentVarGuard() as env:
            del env['LINES']
            del env['COLUMNS']

            # sys.__stdout__ has no fileno()
            with support.swap_attr(sys, '__stdout__', None):
                size = shutil.get_terminal_size(fallback=(10, 20))
            self.assertEqual(size.columns, 10)
            self.assertEqual(size.lines, 20)

            # sys.__stdout__ is not a terminal on Unix
            # or fileno() not in (0, 1, 2) on Windows
            with open(os.devnull, 'w') as f, \
                 support.swap_attr(sys, '__stdout__', f):
                size = shutil.get_terminal_size(fallback=(30, 40))
            self.assertEqual(size.columns, 30)
            self.assertEqual(size.lines, 40)


class PublicAPITests(unittest.TestCase):
    """Ensures that the correct values are exposed in the public API."""

    def test_module_all_attribute(self):
        self.assertTrue(hasattr(shutil, '__all__'))
        target_api = ['copyfileobj', 'copyfile', 'copymode', 'copystat',
                      'copy', 'copy2', 'copytree', 'move', 'rmtree', 'Error',
                      'SpecialFileError', 'ExecError', 'make_archive',
                      'get_archive_formats', 'register_archive_format',
                      'unregister_archive_format', 'get_unpack_formats',
                      'register_unpack_format', 'unregister_unpack_format',
                      'unpack_archive', 'ignore_patterns', 'chown', 'which',
                      'get_terminal_size', 'SameFileError']
        if hasattr(os, 'statvfs') or os.name == 'nt':
            target_api.append('disk_usage')
        self.assertEqual(set(shutil.__all__), set(target_api))


if __name__ == '__main__':
    unittest.main()
