# Copyright (C) 2003 Python Software Foundation

import unittest
import shutil
import tempfile
import sys
import stat
import os
import os.path
import functools
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
from test.support import TESTFN, check_warnings, captured_stdout

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
    import zlib
except ImportError:
    zlib = None

try:
    import zipfile
    ZIP_SUPPORT = True
except ImportError:
    ZIP_SUPPORT = find_executable('zip')

def _fake_rename(*args, **kwargs):
    # Pretend the destination path is on a different filesystem.
    raise OSError()

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

class TestShutil(unittest.TestCase):

    def setUp(self):
        super(TestShutil, self).setUp()
        self.tempdirs = []

    def tearDown(self):
        super(TestShutil, self).tearDown()
        while self.tempdirs:
            d = self.tempdirs.pop()
            shutil.rmtree(d, os.name in ('nt', 'cygwin'))

    def write_file(self, path, content='xxx'):
        """Writes a file in the given path.


        path can be a string or a sequence.
        """
        if isinstance(path, (list, tuple)):
            path = os.path.join(*path)
        f = open(path, 'w')
        try:
            f.write(content)
        finally:
            f.close()

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
            self.childpath = os.path.join(TESTFN, 'a')
            f = open(self.childpath, 'w')
            f.close()
            old_dir_mode = os.stat(TESTFN).st_mode
            old_child_mode = os.stat(self.childpath).st_mode
            # Make unwritable.
            os.chmod(self.childpath, stat.S_IREAD)
            os.chmod(TESTFN, stat.S_IREAD)

            shutil.rmtree(TESTFN, onerror=self.check_args_to_onerror)
            # Test whether onerror has actually been called.
            self.assertEqual(self.errorState, 2,
                             "Expected call to onerror function did not happen.")

            # Make writable again.
            os.chmod(TESTFN, old_dir_mode)
            os.chmod(self.childpath, old_child_mode)

            # Clean up.
            shutil.rmtree(TESTFN)

    def check_args_to_onerror(self, func, arg, exc):
        # test_rmtree_errors deliberately runs rmtree
        # on a directory that is chmod 400, which will fail.
        # This function is run when shutil.rmtree fails.
        # 99.9% of the time it initially fails to remove
        # a file in the directory, so the first time through
        # func is os.remove.
        # However, some Linux machines running ZFS on
        # FUSE experienced a failure earlier in the process
        # at os.listdir.  The first failure may legally
        # be either.
        if self.errorState == 0:
            if func is os.remove:
                self.assertEqual(arg, self.childpath)
            else:
                self.assertIs(func, os.listdir,
                              "func must be either os.remove or os.listdir")
                self.assertEqual(arg, TESTFN)
            self.assertTrue(issubclass(exc[0], OSError))
            self.errorState = 1
        else:
            self.assertEqual(func, os.rmdir)
            self.assertEqual(arg, TESTFN)
            self.assertTrue(issubclass(exc[0], OSError))
            self.errorState = 2

    def test_rmtree_dont_delete_file(self):
        # When called on a file instead of a directory, don't delete it.
        handle, path = tempfile.mkstemp()
        os.fdopen(handle).close()
        self.assertRaises(OSError, shutil.rmtree, path)
        os.remove(path)

    def _write_data(self, path, data):
        f = open(path, "w")
        f.write(data)
        f.close()

    def test_copytree_simple(self):

        def read_data(path):
            f = open(path)
            data = f.read()
            f.close()
            return data

        src_dir = tempfile.mkdtemp()
        dst_dir = os.path.join(tempfile.mkdtemp(), 'destination')
        self._write_data(os.path.join(src_dir, 'test.txt'), '123')
        os.mkdir(os.path.join(src_dir, 'test_dir'))
        self._write_data(os.path.join(src_dir, 'test_dir', 'test.txt'), '456')

        try:
            shutil.copytree(src_dir, dst_dir)
            self.assertTrue(os.path.isfile(os.path.join(dst_dir, 'test.txt')))
            self.assertTrue(os.path.isdir(os.path.join(dst_dir, 'test_dir')))
            self.assertTrue(os.path.isfile(os.path.join(dst_dir, 'test_dir',
                                                        'test.txt')))
            actual = read_data(os.path.join(dst_dir, 'test.txt'))
            self.assertEqual(actual, '123')
            actual = read_data(os.path.join(dst_dir, 'test_dir', 'test.txt'))
            self.assertEqual(actual, '456')
        finally:
            for path in (
                    os.path.join(src_dir, 'test.txt'),
                    os.path.join(dst_dir, 'test.txt'),
                    os.path.join(src_dir, 'test_dir', 'test.txt'),
                    os.path.join(dst_dir, 'test_dir', 'test.txt'),
                ):
                if os.path.exists(path):
                    os.remove(path)
            for path in (src_dir,
                    os.path.dirname(dst_dir)
                ):
                if os.path.exists(path):
                    shutil.rmtree(path)

    def test_copytree_with_exclude(self):

        def read_data(path):
            f = open(path)
            data = f.read()
            f.close()
            return data

        # creating data
        join = os.path.join
        exists = os.path.exists
        src_dir = tempfile.mkdtemp()
        try:
            dst_dir = join(tempfile.mkdtemp(), 'destination')
            self._write_data(join(src_dir, 'test.txt'), '123')
            self._write_data(join(src_dir, 'test.tmp'), '123')
            os.mkdir(join(src_dir, 'test_dir'))
            self._write_data(join(src_dir, 'test_dir', 'test.txt'), '456')
            os.mkdir(join(src_dir, 'test_dir2'))
            self._write_data(join(src_dir, 'test_dir2', 'test.txt'), '456')
            os.mkdir(join(src_dir, 'test_dir2', 'subdir'))
            os.mkdir(join(src_dir, 'test_dir2', 'subdir2'))
            self._write_data(join(src_dir, 'test_dir2', 'subdir', 'test.txt'),
                             '456')
            self._write_data(join(src_dir, 'test_dir2', 'subdir2', 'test.py'),
                             '456')


            # testing glob-like patterns
            try:
                patterns = shutil.ignore_patterns('*.tmp', 'test_dir2')
                shutil.copytree(src_dir, dst_dir, ignore=patterns)
                # checking the result: some elements should not be copied
                self.assertTrue(exists(join(dst_dir, 'test.txt')))
                self.assertTrue(not exists(join(dst_dir, 'test.tmp')))
                self.assertTrue(not exists(join(dst_dir, 'test_dir2')))
            finally:
                if os.path.exists(dst_dir):
                    shutil.rmtree(dst_dir)
            try:
                patterns = shutil.ignore_patterns('*.tmp', 'subdir*')
                shutil.copytree(src_dir, dst_dir, ignore=patterns)
                # checking the result: some elements should not be copied
                self.assertTrue(not exists(join(dst_dir, 'test.tmp')))
                self.assertTrue(not exists(join(dst_dir, 'test_dir2', 'subdir2')))
                self.assertTrue(not exists(join(dst_dir, 'test_dir2', 'subdir')))
            finally:
                if os.path.exists(dst_dir):
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
                self.assertTrue(not exists(join(dst_dir, 'test_dir2', 'subdir2',
                                        'test.py')))
                self.assertTrue(not exists(join(dst_dir, 'test_dir2', 'subdir')))

            finally:
                if os.path.exists(dst_dir):
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
        self._write_data(os.path.join(src_dir, 'test.txt'), '123')
        os.mkdir(os.path.join(src_dir, 'test_dir'))
        self._write_data(os.path.join(src_dir, 'test_dir', 'test.txt'), '456')

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
        self._write_data(os.path.join(src_dir, 'test_dir', 'test.txt'), '456')
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
        self.write_file([tmpdir, fname])
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

    @unittest.skipUnless(zlib, "requires zlib")
    def test_make_tarball(self):
        # creating something to tar
        tmpdir = self.mkdtemp()
        self.write_file([tmpdir, 'file1'], 'xxx')
        self.write_file([tmpdir, 'file2'], 'xxx')
        os.mkdir(os.path.join(tmpdir, 'sub'))
        self.write_file([tmpdir, 'sub', 'file3'], 'xxx')

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
        self.write_file([dist, 'file1'], 'xxx')
        self.write_file([dist, 'file2'], 'xxx')
        os.mkdir(os.path.join(dist, 'sub'))
        self.write_file([dist, 'sub', 'file3'], 'xxx')
        os.mkdir(os.path.join(dist, 'sub2'))
        tmpdir2 = self.mkdtemp()
        base_name = os.path.join(tmpdir2, 'archive')
        return tmpdir, tmpdir2, base_name

    @unittest.skipUnless(zlib, "Requires zlib")
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

    @unittest.skipUnless(zlib, "Requires zlib")
    @unittest.skipUnless(ZIP_SUPPORT, 'Need zip support to run')
    def test_make_zipfile(self):
        # creating something to tar
        tmpdir = self.mkdtemp()
        self.write_file([tmpdir, 'file1'], 'xxx')
        self.write_file([tmpdir, 'file2'], 'xxx')

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

    @unittest.skipUnless(zlib, "Requires zlib")
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


    @unittest.skipUnless(zlib, "Requires zlib")
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

    @unittest.skipUnless(zlib, "Requires zlib")
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
            if os.path.exists(dst_dir):
                os.rmdir(dst_dir)



def test_main():
    support.run_unittest(TestShutil, TestMove, TestCopyFile)

if __name__ == '__main__':
    test_main()
