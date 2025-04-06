"""Tests common to tarfile and zipfile."""

import os
import sys

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
            with self.assertRaises(PermissionError if sys.platform == 'win32'
                                   else IsADirectoryError):
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
        # XXX: It is potential security vulnerability.
        target = os.path.join(self.testdir, 'test')
        target2 = os.path.join(self.testdir, 'test2')
        self.create_file(target2, b'content')
        os.symlink('test2', target)
        with self.open(self.ar_with_file) as ar:
            self.extractall(ar)
        self.assertTrue(os.path.islink(target))
        self.assertTrue(os.path.isfile(target2))
        with open(target2, 'rb') as f:
            self.assertEqual(f.read(), b'newcontent')

    @os_helper.skip_unless_symlink
    def test_overwrite_broken_file_symlink_as_file(self):
        # XXX: It is potential security vulnerability.
        target = os.path.join(self.testdir, 'test')
        target2 = os.path.join(self.testdir, 'test2')
        os.symlink('test2', target)
        with self.open(self.ar_with_file) as ar:
            self.extractall(ar)
        self.assertTrue(os.path.islink(target))
        self.assertTrue(os.path.isfile(target2))
        with open(target2, 'rb') as f:
            self.assertEqual(f.read(), b'newcontent')

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
