# Copyright (C) 2003 Python Software Foundation

import unittest
import shutil
import tempfile
import sys
import stat
import os
import os.path
from os.path import splitdrive
from distutils.spawn import find_executable, spawn
from shutil import (_make_tarball, _make_zipfile, make_archive,
                    register_archive_format, unregister_archive_format,
                    get_archive_formats)
import tarfile
import warnings

from test import test_support
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
            # skip
            return
        self._check_move_file(self.src_file, self.file_other_fs,
            self.file_other_fs)

    def test_move_file_to_dir_other_fs(self):
        # Move a file to another location on another filesystem.
        if not self.dir_other_fs:
            # skip
            return
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
            # skip
            return
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
            # skip
            return
        self._check_move_dir(self.src_dir, self.dir_other_fs,
            os.path.join(self.dir_other_fs, os.path.basename(self.src_dir)))

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
    test_support.run_unittest(TestShutil, TestMove, TestCopyFile)

if __name__ == '__main__':
    test_main()
