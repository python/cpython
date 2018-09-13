# Copyright (C) 2003 Python Software Foundation

import unittest
import shutil
import tempfile
import sys
import stat
import os
import os.path
import errno
import subprocess
from distutils.spawn import find_executable
from shutil import (make_archive,
                    register_archive_format, unregister_archive_format,
                    get_archive_formats)
import tarfile
import warnings

from test import test_support as support
from test.test_support import TESTFN, check_warnings, captured_stdout

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
    import zlib
    ZIP_SUPPORT = True
except ImportError:
    ZIP_SUPPORT = find_executable('zip')

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

    @unittest.skipUnless(hasattr(os, 'chmod'), 'requires os.chmod()')
    @unittest.skipIf(sys.platform[:6] == 'cygwin',
                     "This test can't be run on Cygwin (issue #1071513).")
    @unittest.skipIf(hasattr(os, 'geteuid') and os.geteuid() == 0,
                     "This test can't be run reliably as root (issue #1076467).")
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

    def test_copytree_simple(self):
        def write_data(path, data):
            f = open(path, "w")
            f.write(data)
            f.close()

        def read_data(path):
            f = open(path)
            data = f.read()
            f.close()
            return data

        src_dir = tempfile.mkdtemp()
        dst_dir = os.path.join(tempfile.mkdtemp(), 'destination')

        write_data(os.path.join(src_dir, 'test.txt'), '123')

        os.mkdir(os.path.join(src_dir, 'test_dir'))
        write_data(os.path.join(src_dir, 'test_dir', 'test.txt'), '456')

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

        def write_data(path, data):
            f = open(path, "w")
            f.write(data)
            f.close()

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
            write_data(join(src_dir, 'test.txt'), '123')
            write_data(join(src_dir, 'test.tmp'), '123')
            os.mkdir(join(src_dir, 'test_dir'))
            write_data(join(src_dir, 'test_dir', 'test.txt'), '456')
            os.mkdir(join(src_dir, 'test_dir2'))
            write_data(join(src_dir, 'test_dir2', 'test.txt'), '456')
            os.mkdir(join(src_dir, 'test_dir2', 'subdir'))
            os.mkdir(join(src_dir, 'test_dir2', 'subdir2'))
            write_data(join(src_dir, 'test_dir2', 'subdir', 'test.txt'), '456')
            write_data(join(src_dir, 'test_dir2', 'subdir2', 'test.py'), '456')


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

    if hasattr(os, "symlink"):
        def test_dont_copy_file_onto_link_to_itself(self):
            # bug 851123.
            os.mkdir(TESTFN)
            src = os.path.join(TESTFN, 'cheese')
            dst = os.path.join(TESTFN, 'shop')
            try:
                f = open(src, 'w')
                f.write('cheddar')
                f.close()

                os.link(src, dst)
                self.assertRaises(shutil.Error, shutil.copyfile, src, dst)
                with open(src, 'r') as f:
                    self.assertEqual(f.read(), 'cheddar')
                os.remove(dst)

                # Using `src` here would mean we end up with a symlink pointing
                # to TESTFN/TESTFN/cheese, while it should point at
                # TESTFN/cheese.
                os.symlink('cheese', dst)
                self.assertRaises(shutil.Error, shutil.copyfile, src, dst)
                with open(src, 'r') as f:
                    self.assertEqual(f.read(), 'cheddar')
                os.remove(dst)
            finally:
                try:
                    shutil.rmtree(TESTFN)
                except OSError:
                    pass

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

    # Issue #3002: copyfile and copytree block indefinitely on named pipes
    @unittest.skipUnless(hasattr(os, "mkfifo"), 'requires os.mkfifo()')
    def test_copyfile_named_pipe(self):
        os.mkfifo(TESTFN)
        try:
            self.assertRaises(shutil.SpecialFileError,
                              shutil.copyfile, TESTFN, TESTFN2)
            self.assertRaises(shutil.SpecialFileError,
                              shutil.copyfile, __file__, TESTFN)
        finally:
            os.remove(TESTFN)

    @unittest.skipUnless(hasattr(os, "mkfifo"), 'requires os.mkfifo()')
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

    @unittest.skipUnless(hasattr(os, 'chflags') and
                         hasattr(errno, 'EOPNOTSUPP') and
                         hasattr(errno, 'ENOTSUP'),
                         "requires os.chflags, EOPNOTSUPP & ENOTSUP")
    def test_copystat_handles_harmless_chflags_errors(self):
        tmpdir = self.mkdtemp()
        file1 = os.path.join(tmpdir, 'file1')
        file2 = os.path.join(tmpdir, 'file2')
        self.write_file(file1, 'xxx')
        self.write_file(file2, 'xxx')

        def make_chflags_raiser(err):
            ex = OSError()

            def _chflags_raiser(path, flags):
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

    @unittest.skipUnless(zlib, "requires zlib")
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
            self.assertEqual(sorted(tf.getnames()),
                             ['.', './file1', './file2',
                              './sub', './sub/file3', './sub2'])

        # trying an uncompressed one
        with support.change_cwd(work_dir):
            tarball = make_archive(rel_base_name, 'tar', root_dir, '.')
        self.assertEqual(tarball, base_name + '.tar')
        self.assertTrue(os.path.isfile(tarball))
        self.assertTrue(tarfile.is_tarfile(tarball))
        with tarfile.open(tarball, 'r') as tf:
            self.assertEqual(sorted(tf.getnames()),
                             ['.', './file1', './file2',
                              './sub', './sub/file3', './sub2'])

    def _tarinfo(self, path):
        with tarfile.open(path) as tar:
            names = tar.getnames()
            names.sort()
            return tuple(names)

    def _create_files(self, base_dir='dist'):
        # creating something to tar
        root_dir = self.mkdtemp()
        dist = os.path.join(root_dir, base_dir)
        if not os.path.isdir(dist):
            os.makedirs(dist)
        self.write_file((dist, 'file1'), 'xxx')
        self.write_file((dist, 'file2'), 'xxx')
        os.mkdir(os.path.join(dist, 'sub'))
        self.write_file((dist, 'sub', 'file3'), 'xxx')
        os.mkdir(os.path.join(dist, 'sub2'))
        if base_dir:
            self.write_file((root_dir, 'outer'), 'xxx')
        return root_dir, base_dir

    @unittest.skipUnless(zlib, "Requires zlib")
    @unittest.skipUnless(find_executable('tar'),
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
        subprocess.check_call(tar_cmd, cwd=root_dir)

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

    @unittest.skipUnless(ZIP_SUPPORT, 'Need zip support to run')
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
            self.assertEqual(sorted(zf.namelist()),
                    ['dist/', 'dist/file1', 'dist/file2',
                     'dist/sub/', 'dist/sub/file3', 'dist/sub2/',
                     'outer'])
        support.unlink(res)

        with support.change_cwd(work_dir):
            base_name = os.path.abspath(rel_base_name)
            res = make_archive(rel_base_name, 'zip', root_dir, base_dir)

        self.assertEqual(res, base_name + '.zip')
        self.assertTrue(os.path.isfile(res))
        self.assertTrue(zipfile.is_zipfile(res))
        with zipfile.ZipFile(res) as zf:
            self.assertEqual(sorted(zf.namelist()),
                    ['dist/', 'dist/file1', 'dist/file2',
                     'dist/sub/', 'dist/sub/file3', 'dist/sub2/'])

    @unittest.skipUnless(ZIP_SUPPORT, 'Need zip support to run')
    @unittest.skipUnless(find_executable('zip'),
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
        subprocess.check_call(zip_cmd, cwd=root_dir)

        self.assertTrue(os.path.isfile(archive2))
        # let's compare both ZIP files
        with zipfile.ZipFile(archive) as zf:
            names = zf.namelist()
        with zipfile.ZipFile(archive2) as zf:
            names2 = zf.namelist()
        self.assertEqual(sorted(names), sorted(names2))

    @unittest.skipUnless(ZIP_SUPPORT, 'Need zip support to run')
    @unittest.skipUnless(find_executable('unzip'),
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
                details = exc.output
                if 'unrecognized option: t' in details:
                    self.skipTest("unzip doesn't support -t")
                msg = "{}\n\n**Unzip Output**\n{}"
                self.fail(msg.format(exc, details))

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

    @unittest.skipUnless(zlib, "Requires zlib")
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
        saved_dir = os.getcwd()
        try:
            os.chdir(root_dir)
            self.assertEqual(make_archive('test', 'tar'), 'test.tar')
            self.assertTrue(os.path.isfile('test.tar'))
        finally:
            os.chdir(saved_dir)

    @unittest.skipUnless(zlib, "Requires zlib")
    def test_make_zipfile_in_curdir(self):
        # Issue #21280
        root_dir = self.mkdtemp()
        saved_dir = os.getcwd()
        try:
            os.chdir(root_dir)
            self.assertEqual(make_archive('test', 'zip'), 'test.zip')
            self.assertTrue(os.path.isfile('test.zip'))
        finally:
            os.chdir(saved_dir)

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


class TestMove(unittest.TestCase):

    def setUp(self):
        filename = "foo"
        self.src_dir = tempfile.mkdtemp()
        self.dst_dir = tempfile.mkdtemp()
        self.src_file = os.path.join(self.src_dir, filename)
        self.dst_file = os.path.join(self.dst_dir, filename)
        # Try to create a dir in the current directory, hoping that it is
        # not located on the same filesystem as the system tmp dir.
        try:
            self.dir_other_fs = tempfile.mkdtemp(
                dir=os.path.dirname(__file__))
            self.file_other_fs = os.path.join(self.dir_other_fs,
                filename)
        except OSError:
            self.dir_other_fs = None
        with open(self.src_file, "wb") as f:
            f.write("spam")

    def tearDown(self):
        for d in (self.src_dir, self.dst_dir, self.dir_other_fs):
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

    def test_move_file_other_fs(self):
        # Move a file to an existing dir on another filesystem.
        if not self.dir_other_fs:
            self.skipTest('dir on other filesystem not available')
        self._check_move_file(self.src_file, self.file_other_fs,
            self.file_other_fs)

    def test_move_file_to_dir_other_fs(self):
        # Move a file to another location on another filesystem.
        if not self.dir_other_fs:
            self.skipTest('dir on other filesystem not available')
        self._check_move_file(self.src_file, self.dir_other_fs,
            self.file_other_fs)

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

    def test_move_dir_other_fs(self):
        # Move a dir to another location on another filesystem.
        if not self.dir_other_fs:
            self.skipTest('dir on other filesystem not available')
        dst_dir = tempfile.mktemp(dir=self.dir_other_fs)
        try:
            self._check_move_dir(self.src_dir, dst_dir, dst_dir)
        finally:
            try:
                shutil.rmtree(dst_dir)
            except:
                pass

    def test_move_dir_to_dir(self):
        # Move a dir inside an existing dir on the same filesystem.
        self._check_move_dir(self.src_dir, self.dst_dir,
            os.path.join(self.dst_dir, os.path.basename(self.src_dir)))

    def test_move_dir_to_dir_other_fs(self):
        # Move a dir inside an existing dir on another filesystem.
        if not self.dir_other_fs:
            self.skipTest('dir on other filesystem not available')
        self._check_move_dir(self.src_dir, self.dir_other_fs,
            os.path.join(self.dir_other_fs, os.path.basename(self.src_dir)))

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
