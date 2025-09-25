import os
import stat
import sys
import unittest
from test import support
from test.support import os_helper
from test.support import warnings_helper
from .utils import create_file


class TestScandir(unittest.TestCase):
    check_no_resource_warning = warnings_helper.check_no_resource_warning

    def setUp(self):
        self.path = os.path.realpath(os_helper.TESTFN)
        self.bytes_path = os.fsencode(self.path)
        self.addCleanup(os_helper.rmtree, self.path)
        os.mkdir(self.path)

    def create_file(self, name="file.txt"):
        path = self.bytes_path if isinstance(name, bytes) else self.path
        filename = os.path.join(path, name)
        create_file(filename, b'python')
        return filename

    def get_entries(self, names):
        entries = dict((entry.name, entry)
                       for entry in os.scandir(self.path))
        self.assertEqual(sorted(entries.keys()), names)
        return entries

    def assert_stat_equal(self, stat1, stat2, skip_fields):
        if skip_fields:
            for attr in dir(stat1):
                if not attr.startswith("st_"):
                    continue
                if attr in ("st_dev", "st_ino", "st_nlink", "st_ctime",
                            "st_ctime_ns"):
                    continue
                self.assertEqual(getattr(stat1, attr),
                                 getattr(stat2, attr),
                                 (stat1, stat2, attr))
        else:
            self.assertEqual(stat1, stat2)

    def test_uninstantiable(self):
        scandir_iter = os.scandir(self.path)
        self.assertRaises(TypeError, type(scandir_iter))
        scandir_iter.close()

    def test_unpickable(self):
        filename = self.create_file("file.txt")
        scandir_iter = os.scandir(self.path)
        import pickle
        self.assertRaises(TypeError, pickle.dumps, scandir_iter, filename)
        scandir_iter.close()

    def check_entry(self, entry, name, is_dir, is_file, is_symlink):
        self.assertIsInstance(entry, os.DirEntry)
        self.assertEqual(entry.name, name)
        self.assertEqual(entry.path, os.path.join(self.path, name))
        self.assertEqual(entry.inode(),
                         os.stat(entry.path, follow_symlinks=False).st_ino)

        entry_stat = os.stat(entry.path)
        self.assertEqual(entry.is_dir(),
                         stat.S_ISDIR(entry_stat.st_mode))
        self.assertEqual(entry.is_file(),
                         stat.S_ISREG(entry_stat.st_mode))
        self.assertEqual(entry.is_symlink(),
                         os.path.islink(entry.path))

        entry_lstat = os.stat(entry.path, follow_symlinks=False)
        self.assertEqual(entry.is_dir(follow_symlinks=False),
                         stat.S_ISDIR(entry_lstat.st_mode))
        self.assertEqual(entry.is_file(follow_symlinks=False),
                         stat.S_ISREG(entry_lstat.st_mode))

        self.assertEqual(entry.is_junction(), os.path.isjunction(entry.path))

        self.assert_stat_equal(entry.stat(),
                               entry_stat,
                               os.name == 'nt' and not is_symlink)
        self.assert_stat_equal(entry.stat(follow_symlinks=False),
                               entry_lstat,
                               os.name == 'nt')

    def test_attributes(self):
        link = os_helper.can_hardlink()
        symlink = os_helper.can_symlink()

        dirname = os.path.join(self.path, "dir")
        os.mkdir(dirname)
        filename = self.create_file("file.txt")
        if link:
            try:
                os.link(filename, os.path.join(self.path, "link_file.txt"))
            except PermissionError as e:
                self.skipTest('os.link(): %s' % e)
        if symlink:
            os.symlink(dirname, os.path.join(self.path, "symlink_dir"),
                       target_is_directory=True)
            os.symlink(filename, os.path.join(self.path, "symlink_file.txt"))

        names = ['dir', 'file.txt']
        if link:
            names.append('link_file.txt')
        if symlink:
            names.extend(('symlink_dir', 'symlink_file.txt'))
        entries = self.get_entries(names)

        entry = entries['dir']
        self.check_entry(entry, 'dir', True, False, False)

        entry = entries['file.txt']
        self.check_entry(entry, 'file.txt', False, True, False)

        if link:
            entry = entries['link_file.txt']
            self.check_entry(entry, 'link_file.txt', False, True, False)

        if symlink:
            entry = entries['symlink_dir']
            self.check_entry(entry, 'symlink_dir', True, False, True)

            entry = entries['symlink_file.txt']
            self.check_entry(entry, 'symlink_file.txt', False, True, True)

    @unittest.skipIf(sys.platform != 'win32', "Can only test junctions with creation on win32.")
    def test_attributes_junctions(self):
        dirname = os.path.join(self.path, "tgtdir")
        os.mkdir(dirname)

        import _winapi
        try:
            _winapi.CreateJunction(dirname, os.path.join(self.path, "srcjunc"))
        except OSError:
            raise unittest.SkipTest('creating the test junction failed')

        entries = self.get_entries(['srcjunc', 'tgtdir'])
        self.assertEqual(entries['srcjunc'].is_junction(), True)
        self.assertEqual(entries['tgtdir'].is_junction(), False)

    def get_entry(self, name):
        path = self.bytes_path if isinstance(name, bytes) else self.path
        entries = list(os.scandir(path))
        self.assertEqual(len(entries), 1)

        entry = entries[0]
        self.assertEqual(entry.name, name)
        return entry

    def create_file_entry(self, name='file.txt'):
        filename = self.create_file(name=name)
        return self.get_entry(os.path.basename(filename))

    def test_current_directory(self):
        filename = self.create_file()
        old_dir = os.getcwd()
        try:
            os.chdir(self.path)

            # call scandir() without parameter: it must list the content
            # of the current directory
            entries = dict((entry.name, entry) for entry in os.scandir())
            self.assertEqual(sorted(entries.keys()),
                             [os.path.basename(filename)])
        finally:
            os.chdir(old_dir)

    def test_repr(self):
        entry = self.create_file_entry()
        self.assertEqual(repr(entry), "<DirEntry 'file.txt'>")

    def test_fspath_protocol(self):
        entry = self.create_file_entry()
        self.assertEqual(os.fspath(entry), os.path.join(self.path, 'file.txt'))

    def test_fspath_protocol_bytes(self):
        bytes_filename = os.fsencode('bytesfile.txt')
        bytes_entry = self.create_file_entry(name=bytes_filename)
        fspath = os.fspath(bytes_entry)
        self.assertIsInstance(fspath, bytes)
        self.assertEqual(fspath,
                         os.path.join(os.fsencode(self.path),bytes_filename))

    def test_removed_dir(self):
        path = os.path.join(self.path, 'dir')

        os.mkdir(path)
        entry = self.get_entry('dir')
        os.rmdir(path)

        # On POSIX, is_dir() result depends if scandir() filled d_type or not
        if os.name == 'nt':
            self.assertTrue(entry.is_dir())
        self.assertFalse(entry.is_file())
        self.assertFalse(entry.is_symlink())
        if os.name == 'nt':
            self.assertRaises(FileNotFoundError, entry.inode)
            # don't fail
            entry.stat()
            entry.stat(follow_symlinks=False)
        else:
            self.assertGreater(entry.inode(), 0)
            self.assertRaises(FileNotFoundError, entry.stat)
            self.assertRaises(FileNotFoundError, entry.stat, follow_symlinks=False)

    def test_removed_file(self):
        entry = self.create_file_entry()
        os.unlink(entry.path)

        self.assertFalse(entry.is_dir())
        # On POSIX, is_dir() result depends if scandir() filled d_type or not
        if os.name == 'nt':
            self.assertTrue(entry.is_file())
        self.assertFalse(entry.is_symlink())
        if os.name == 'nt':
            self.assertRaises(FileNotFoundError, entry.inode)
            # don't fail
            entry.stat()
            entry.stat(follow_symlinks=False)
        else:
            self.assertGreater(entry.inode(), 0)
            self.assertRaises(FileNotFoundError, entry.stat)
            self.assertRaises(FileNotFoundError, entry.stat, follow_symlinks=False)

    def test_broken_symlink(self):
        if not os_helper.can_symlink():
            return self.skipTest('cannot create symbolic link')

        filename = self.create_file("file.txt")
        os.symlink(filename,
                   os.path.join(self.path, "symlink.txt"))
        entries = self.get_entries(['file.txt', 'symlink.txt'])
        entry = entries['symlink.txt']
        os.unlink(filename)

        self.assertGreater(entry.inode(), 0)
        self.assertFalse(entry.is_dir())
        self.assertFalse(entry.is_file())  # broken symlink returns False
        self.assertFalse(entry.is_dir(follow_symlinks=False))
        self.assertFalse(entry.is_file(follow_symlinks=False))
        self.assertTrue(entry.is_symlink())
        self.assertRaises(FileNotFoundError, entry.stat)
        # don't fail
        entry.stat(follow_symlinks=False)

    def test_bytes(self):
        self.create_file("file.txt")

        path_bytes = os.fsencode(self.path)
        entries = list(os.scandir(path_bytes))
        self.assertEqual(len(entries), 1, entries)
        entry = entries[0]

        self.assertEqual(entry.name, b'file.txt')
        self.assertEqual(entry.path,
                         os.fsencode(os.path.join(self.path, 'file.txt')))

    def test_bytes_like(self):
        self.create_file("file.txt")

        for cls in bytearray, memoryview:
            path_bytes = cls(os.fsencode(self.path))
            with self.assertRaises(TypeError):
                os.scandir(path_bytes)

    @unittest.skipUnless(os.listdir in os.supports_fd,
                         'fd support for listdir required for this test.')
    def test_fd(self):
        self.assertIn(os.scandir, os.supports_fd)
        self.create_file('file.txt')
        expected_names = ['file.txt']
        if os_helper.can_symlink():
            os.symlink('file.txt', os.path.join(self.path, 'link'))
            expected_names.append('link')

        with os_helper.open_dir_fd(self.path) as fd:
            with os.scandir(fd) as it:
                entries = list(it)
            names = [entry.name for entry in entries]
            self.assertEqual(sorted(names), expected_names)
            self.assertEqual(names, os.listdir(fd))
            for entry in entries:
                self.assertEqual(entry.path, entry.name)
                self.assertEqual(os.fspath(entry), entry.name)
                self.assertEqual(entry.is_symlink(), entry.name == 'link')
                if os.stat in os.supports_dir_fd:
                    st = os.stat(entry.name, dir_fd=fd)
                    self.assertEqual(entry.stat(), st)
                    st = os.stat(entry.name, dir_fd=fd, follow_symlinks=False)
                    self.assertEqual(entry.stat(follow_symlinks=False), st)

    @unittest.skipIf(support.is_wasi, "WASI maps '' to cwd")
    def test_empty_path(self):
        self.assertRaises(FileNotFoundError, os.scandir, '')

    def test_consume_iterator_twice(self):
        self.create_file("file.txt")
        iterator = os.scandir(self.path)

        entries = list(iterator)
        self.assertEqual(len(entries), 1, entries)

        # check than consuming the iterator twice doesn't raise exception
        entries2 = list(iterator)
        self.assertEqual(len(entries2), 0, entries2)

    def test_bad_path_type(self):
        for obj in [1.234, {}, []]:
            self.assertRaises(TypeError, os.scandir, obj)

    def test_close(self):
        self.create_file("file.txt")
        self.create_file("file2.txt")
        iterator = os.scandir(self.path)
        next(iterator)
        iterator.close()
        # multiple closes
        iterator.close()
        with self.check_no_resource_warning():
            del iterator

    def test_context_manager(self):
        self.create_file("file.txt")
        self.create_file("file2.txt")
        with os.scandir(self.path) as iterator:
            next(iterator)
        with self.check_no_resource_warning():
            del iterator

    def test_context_manager_close(self):
        self.create_file("file.txt")
        self.create_file("file2.txt")
        with os.scandir(self.path) as iterator:
            next(iterator)
            iterator.close()

    def test_context_manager_exception(self):
        self.create_file("file.txt")
        self.create_file("file2.txt")
        with self.assertRaises(ZeroDivisionError):
            with os.scandir(self.path) as iterator:
                next(iterator)
                1/0
        with self.check_no_resource_warning():
            del iterator

    def test_resource_warning(self):
        self.create_file("file.txt")
        self.create_file("file2.txt")
        iterator = os.scandir(self.path)
        next(iterator)
        with self.assertWarns(ResourceWarning):
            del iterator
            support.gc_collect()
        # exhausted iterator
        iterator = os.scandir(self.path)
        list(iterator)
        with self.check_no_resource_warning():
            del iterator


class TestDirEntry(unittest.TestCase):
    def setUp(self):
        self.path = os.path.realpath(os_helper.TESTFN)
        self.addCleanup(os_helper.rmtree, self.path)
        os.mkdir(self.path)

    def test_uninstantiable(self):
        self.assertRaises(TypeError, os.DirEntry)

    def test_unpickable(self):
        filename = create_file(os.path.join(self.path, "file.txt"), b'python')
        entry = [entry for entry in os.scandir(self.path)].pop()
        self.assertIsInstance(entry, os.DirEntry)
        self.assertEqual(entry.name, "file.txt")
        import pickle
        self.assertRaises(TypeError, pickle.dumps, entry, filename)


if __name__ == "__main__":
    unittest.main()
