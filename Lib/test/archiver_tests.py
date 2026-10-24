"""Tests common to tarfile and zipfile."""

import os
import sys

from test.support import swap_attr
from test.support import os_helper

class OverwriteTests:

    def setUp(self):
        os.makedirs(self.testdir)
        self.addCleanup(os_helper.rmtree, self.testdir)

    def create_file(self, path, content=b''):
        with open(path, 'wb') as f:
            f.write(content)

    def open(self, path):
        raise NotImplementedError

    def extractall(self, ar):
        raise NotImplementedError


    def test_overwrite_file_as_file(self):
        target = os.path.join(self.testdir, 'test')
        self.create_file(target, b'content')
        with self.open(self.ar_with_file) as ar:
            self.extractall(ar)
        self.assertTrue(os.path.isfile(target))
        with open(target, 'rb') as f:
            self.assertEqual(f.read(), b'newcontent')

    def test_overwrite_dir_as_dir(self):
        target = os.path.join(self.testdir, 'test')
        os.mkdir(target)
        with self.open(self.ar_with_dir) as ar:
            self.extractall(ar)
        self.assertTrue(os.path.isdir(target))

    def test_overwrite_dir_as_implicit_dir(self):
        target = os.path.join(self.testdir, 'test')
        os.mkdir(target)
        with self.open(self.ar_with_implicit_dir) as ar:
            self.extractall(ar)
        self.assertTrue(os.path.isdir(target))
        self.assertTrue(os.path.isfile(os.path.join(target, 'file')))
        with open(os.path.join(target, 'file'), 'rb') as f:
            self.assertEqual(f.read(), b'newcontent')

    def test_overwrite_dir_as_file(self):
        target = os.path.join(self.testdir, 'test')
        os.mkdir(target)
        with self.open(self.ar_with_file) as ar:
            # Removing the directory fails with EISDIR on Linux and EPERM on
            # other platforms.
            with self.assertRaises((IsADirectoryError, PermissionError)):
                self.extractall(ar)
        self.assertTrue(os.path.isdir(target))

    def test_overwrite_file_as_dir(self):
        target = os.path.join(self.testdir, 'test')
        self.create_file(target, b'content')
        with self.open(self.ar_with_dir) as ar:
            with self.assertRaises(FileExistsError):
                self.extractall(ar)
        self.assertTrue(os.path.isfile(target))
        with open(target, 'rb') as f:
            self.assertEqual(f.read(), b'content')

    def test_overwrite_file_as_implicit_dir(self):
        target = os.path.join(self.testdir, 'test')
        self.create_file(target, b'content')
        with self.open(self.ar_with_implicit_dir) as ar:
            with self.assertRaises(FileNotFoundError if sys.platform == 'win32'
                                   else NotADirectoryError):
                self.extractall(ar)
        self.assertTrue(os.path.isfile(target))
        with open(target, 'rb') as f:
            self.assertEqual(f.read(), b'content')

    @os_helper.skip_unless_symlink
    def test_overwrite_file_symlink_as_file(self):
        target = os.path.join(self.testdir, 'test')
        target2 = os.path.join(self.testdir, 'test2')
        self.create_file(target2, b'content')
        os.symlink('test2', target)
        with self.open(self.ar_with_file) as ar:
            self.extractall(ar)
        self.assertFalse(os.path.islink(target))
        with open(target, 'rb') as f:
            self.assertEqual(f.read(), b'newcontent')
        with open(target2, 'rb') as f:
            self.assertEqual(f.read(), b'content')

    @os_helper.skip_unless_symlink
    def test_overwrite_broken_file_symlink_as_file(self):
        target = os.path.join(self.testdir, 'test')
        target2 = os.path.join(self.testdir, 'test2')
        os.symlink('test2', target)
        with self.open(self.ar_with_file) as ar:
            self.extractall(ar)
        self.assertFalse(os.path.islink(target))
        with open(target, 'rb') as f:
            self.assertEqual(f.read(), b'newcontent')
        self.assertFalse(os.path.lexists(target2))

    @os_helper.skip_unless_hardlink
    def test_overwrite_file_hardlink_as_file(self):
        target = os.path.join(self.testdir, 'test')
        target2 = os.path.join(self.testdir, 'test2')
        self.create_file(target2, b'content')
        os.link(target2, target)
        with self.open(self.ar_with_file) as ar:
            self.extractall(ar)
        with open(target, 'rb') as f:
            self.assertEqual(f.read(), b'newcontent')
        with open(target2, 'rb') as f:
            self.assertEqual(f.read(), b'content')

    @os_helper.skip_unless_symlink
    def test_overwrite_dir_symlink_as_dir(self):
        # XXX: It is potential security vulnerability.
        target = os.path.join(self.testdir, 'test')
        target2 = os.path.join(self.testdir, 'test2')
        os.mkdir(target2)
        os.symlink('test2', target, target_is_directory=True)
        with self.open(self.ar_with_dir) as ar:
            self.extractall(ar)
        self.assertTrue(os.path.islink(target))
        self.assertTrue(os.path.isdir(target2))

    @os_helper.skip_unless_symlink
    def test_overwrite_dir_symlink_as_implicit_dir(self):
        # XXX: It is potential security vulnerability.
        target = os.path.join(self.testdir, 'test')
        target2 = os.path.join(self.testdir, 'test2')
        os.mkdir(target2)
        os.symlink('test2', target, target_is_directory=True)
        with self.open(self.ar_with_implicit_dir) as ar:
            self.extractall(ar)
        self.assertTrue(os.path.islink(target))
        self.assertTrue(os.path.isdir(target2))
        self.assertTrue(os.path.isfile(os.path.join(target2, 'file')))
        with open(os.path.join(target2, 'file'), 'rb') as f:
            self.assertEqual(f.read(), b'newcontent')

    @os_helper.skip_unless_symlink
    def test_overwrite_broken_dir_symlink_as_dir(self):
        target = os.path.join(self.testdir, 'test')
        target2 = os.path.join(self.testdir, 'test2')
        os.symlink('test2', target, target_is_directory=True)
        with self.open(self.ar_with_dir) as ar:
            with self.assertRaises(FileExistsError):
                self.extractall(ar)
        self.assertTrue(os.path.islink(target))
        self.assertFalse(os.path.exists(target2))

    @os_helper.skip_unless_symlink
    def test_overwrite_broken_dir_symlink_as_implicit_dir(self):
        target = os.path.join(self.testdir, 'test')
        target2 = os.path.join(self.testdir, 'test2')
        os.symlink('test2', target, target_is_directory=True)
        with self.open(self.ar_with_implicit_dir) as ar:
            with self.assertRaises(FileExistsError):
                self.extractall(ar)
        self.assertTrue(os.path.islink(target))
        self.assertFalse(os.path.exists(target2))

    def test_concurrent_extract_dir(self):
        target = os.path.join(self.testdir, 'test')
        def concurrent_mkdir(*args, **kwargs):
            orig_mkdir(*args, **kwargs)
            orig_mkdir(*args, **kwargs)
        with swap_attr(os, 'mkdir', concurrent_mkdir) as orig_mkdir:
            with self.open(self.ar_with_dir) as ar:
                self.extractall(ar)
        self.assertTrue(os.path.isdir(target))

    def test_concurrent_extract_implicit_dir(self):
        target = os.path.join(self.testdir, 'test')
        def concurrent_mkdir(*args, **kwargs):
            orig_mkdir(*args, **kwargs)
            orig_mkdir(*args, **kwargs)
        with swap_attr(os, 'mkdir', concurrent_mkdir) as orig_mkdir:
            with self.open(self.ar_with_implicit_dir) as ar:
                self.extractall(ar)
        self.assertTrue(os.path.isdir(target))
        self.assertTrue(os.path.isfile(os.path.join(target, 'file')))
