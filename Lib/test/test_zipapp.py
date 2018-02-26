"""Test harness for the zipapp module."""

import io
import pathlib
import stat
import sys
import tempfile
import unittest
import zipapp
import zipfile

from unittest.mock import patch

class ZipAppTest(unittest.TestCase):

    """Test zipapp module functionality."""

    def setUp(self):
        tmpdir = tempfile.TemporaryDirectory()
        self.addCleanup(tmpdir.cleanup)
        self.tmpdir = pathlib.Path(tmpdir.name)

    def test_create_archive(self):
        # Test packing a directory.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target))
        self.assertTrue(target.is_file())

    def test_create_archive_with_pathlib(self):
        # Test packing a directory using Path objects for source and target.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(source, target)
        self.assertTrue(target.is_file())

    def test_create_archive_with_subdirs(self):
        # Test packing a directory includes entries for subdirectories.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        (source / 'foo').mkdir()
        (source / 'bar').mkdir()
        (source / 'foo' / '__init__.py').touch()
        target = io.BytesIO()
        zipapp.create_archive(str(source), target)
        target.seek(0)
        with zipfile.ZipFile(target, 'r') as z:
            self.assertIn('foo/', z.namelist())
            self.assertIn('bar/', z.namelist())

    def test_create_archive_default_target(self):
        # Test packing a directory to the default name.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        zipapp.create_archive(str(source))
        expected_target = self.tmpdir / 'source.pyz'
        self.assertTrue(expected_target.is_file())

    def test_no_main(self):
        # Test that packing a directory with no __main__.py fails.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / 'foo.py').touch()
        target = self.tmpdir / 'source.pyz'
        with self.assertRaises(zipapp.ZipAppError):
            zipapp.create_archive(str(source), str(target))

    def test_main_and_main_py(self):
        # Test that supplying a main argument with __main__.py fails.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        with self.assertRaises(zipapp.ZipAppError):
            zipapp.create_archive(str(source), str(target), main='pkg.mod:fn')

    def test_main_written(self):
        # Test that the __main__.py is written correctly.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / 'foo.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target), main='pkg.mod:fn')
        with zipfile.ZipFile(str(target), 'r') as z:
            self.assertIn('__main__.py', z.namelist())
            self.assertIn(b'pkg.mod.fn()', z.read('__main__.py'))

    def test_main_only_written_once(self):
        # Test that we don't write multiple __main__.py files.
        # The initial implementation had this bug; zip files allow
        # multiple entries with the same name
        source = self.tmpdir / 'source'
        source.mkdir()
        # Write 2 files, as the original bug wrote __main__.py
        # once for each file written :-(
        # See http://bugs.python.org/review/23491/diff/13982/Lib/zipapp.py#newcode67Lib/zipapp.py:67
        # (line 67)
        (source / 'foo.py').touch()
        (source / 'bar.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target), main='pkg.mod:fn')
        with zipfile.ZipFile(str(target), 'r') as z:
            self.assertEqual(1, z.namelist().count('__main__.py'))

    def test_main_validation(self):
        # Test that invalid values for main are rejected.
        source = self.tmpdir / 'source'
        source.mkdir()
        target = self.tmpdir / 'source.pyz'
        problems = [
            '', 'foo', 'foo:', ':bar', '12:bar', 'a.b.c.:d',
            '.a:b', 'a:b.', 'a:.b', 'a:silly name'
        ]
        for main in problems:
            with self.subTest(main=main):
                with self.assertRaises(zipapp.ZipAppError):
                    zipapp.create_archive(str(source), str(target), main=main)

    def test_default_no_shebang(self):
        # Test that no shebang line is written to the target by default.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target))
        with target.open('rb') as f:
            self.assertNotEqual(f.read(2), b'#!')

    def test_custom_interpreter(self):
        # Test that a shebang line with a custom interpreter is written
        # correctly.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target), interpreter='python')
        with target.open('rb') as f:
            self.assertEqual(f.read(2), b'#!')
            self.assertEqual(b'python\n', f.readline())

    def test_pack_to_fileobj(self):
        # Test that we can pack to a file object.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = io.BytesIO()
        zipapp.create_archive(str(source), target, interpreter='python')
        self.assertTrue(target.getvalue().startswith(b'#!python\n'))

    def test_read_shebang(self):
        # Test that we can read the shebang line correctly.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target), interpreter='python')
        self.assertEqual(zipapp.get_interpreter(str(target)), 'python')

    def test_read_missing_shebang(self):
        # Test that reading the shebang line of a file without one returns None.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target))
        self.assertEqual(zipapp.get_interpreter(str(target)), None)

    def test_modify_shebang(self):
        # Test that we can change the shebang of a file.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target), interpreter='python')
        new_target = self.tmpdir / 'changed.pyz'
        zipapp.create_archive(str(target), str(new_target), interpreter='python2.7')
        self.assertEqual(zipapp.get_interpreter(str(new_target)), 'python2.7')

    def test_write_shebang_to_fileobj(self):
        # Test that we can change the shebang of a file, writing the result to a
        # file object.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target), interpreter='python')
        new_target = io.BytesIO()
        zipapp.create_archive(str(target), new_target, interpreter='python2.7')
        self.assertTrue(new_target.getvalue().startswith(b'#!python2.7\n'))

    def test_read_from_pathobj(self):
        # Test that we can copy an archive using a pathlib.Path object
        # for the source.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target1 = self.tmpdir / 'target1.pyz'
        target2 = self.tmpdir / 'target2.pyz'
        zipapp.create_archive(source, target1, interpreter='python')
        zipapp.create_archive(target1, target2, interpreter='python2.7')
        self.assertEqual(zipapp.get_interpreter(target2), 'python2.7')

    def test_read_from_fileobj(self):
        # Test that we can copy an archive using an open file object.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        temp_archive = io.BytesIO()
        zipapp.create_archive(str(source), temp_archive, interpreter='python')
        new_target = io.BytesIO()
        temp_archive.seek(0)
        zipapp.create_archive(temp_archive, new_target, interpreter='python2.7')
        self.assertTrue(new_target.getvalue().startswith(b'#!python2.7\n'))

    def test_remove_shebang(self):
        # Test that we can remove the shebang from a file.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target), interpreter='python')
        new_target = self.tmpdir / 'changed.pyz'
        zipapp.create_archive(str(target), str(new_target), interpreter=None)
        self.assertEqual(zipapp.get_interpreter(str(new_target)), None)

    def test_content_of_copied_archive(self):
        # Test that copying an archive doesn't corrupt it.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = io.BytesIO()
        zipapp.create_archive(str(source), target, interpreter='python')
        new_target = io.BytesIO()
        target.seek(0)
        zipapp.create_archive(target, new_target, interpreter=None)
        new_target.seek(0)
        with zipfile.ZipFile(new_target, 'r') as z:
            self.assertEqual(set(z.namelist()), {'__main__.py'})

    # (Unix only) tests that archives with shebang lines are made executable
    @unittest.skipIf(sys.platform == 'win32',
                     'Windows does not support an executable bit')
    def test_shebang_is_executable(self):
        # Test that an archive with a shebang line is made executable.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target), interpreter='python')
        self.assertTrue(target.stat().st_mode & stat.S_IEXEC)

    @unittest.skipIf(sys.platform == 'win32',
                     'Windows does not support an executable bit')
    def test_no_shebang_is_not_executable(self):
        # Test that an archive with no shebang line is not made executable.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(str(source), str(target), interpreter=None)
        self.assertFalse(target.stat().st_mode & stat.S_IEXEC)


class ZipAppCmdlineTest(unittest.TestCase):

    """Test zipapp module command line API."""

    def setUp(self):
        tmpdir = tempfile.TemporaryDirectory()
        self.addCleanup(tmpdir.cleanup)
        self.tmpdir = pathlib.Path(tmpdir.name)

    def make_archive(self):
        # Test that an archive with no shebang line is not made executable.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        target = self.tmpdir / 'source.pyz'
        zipapp.create_archive(source, target)
        return target

    def test_cmdline_create(self):
        # Test the basic command line API.
        source = self.tmpdir / 'source'
        source.mkdir()
        (source / '__main__.py').touch()
        args = [str(source)]
        zipapp.main(args)
        target = source.with_suffix('.pyz')
        self.assertTrue(target.is_file())

    def test_cmdline_copy(self):
        # Test copying an archive.
        original = self.make_archive()
        target = self.tmpdir / 'target.pyz'
        args = [str(original), '-o', str(target)]
        zipapp.main(args)
        self.assertTrue(target.is_file())

    def test_cmdline_copy_inplace(self):
        # Test copying an archive in place fails.
        original = self.make_archive()
        target = self.tmpdir / 'target.pyz'
        args = [str(original), '-o', str(original)]
        with self.assertRaises(SystemExit) as cm:
            zipapp.main(args)
        # Program should exit with a non-zero return code.
        self.assertTrue(cm.exception.code)

    def test_cmdline_copy_change_main(self):
        # Test copying an archive doesn't allow changing __main__.py.
        original = self.make_archive()
        target = self.tmpdir / 'target.pyz'
        args = [str(original), '-o', str(target), '-m', 'foo:bar']
        with self.assertRaises(SystemExit) as cm:
            zipapp.main(args)
        # Program should exit with a non-zero return code.
        self.assertTrue(cm.exception.code)

    @patch('sys.stdout', new_callable=io.StringIO)
    def test_info_command(self, mock_stdout):
        # Test the output of the info command.
        target = self.make_archive()
        args = [str(target), '--info']
        with self.assertRaises(SystemExit) as cm:
            zipapp.main(args)
        # Program should exit with a zero return code.
        self.assertEqual(cm.exception.code, 0)
        self.assertEqual(mock_stdout.getvalue(), "Interpreter: <none>\n")

    def test_info_error(self):
        # Test the info command fails when the archive does not exist.
        target = self.tmpdir / 'dummy.pyz'
        args = [str(target), '--info']
        with self.assertRaises(SystemExit) as cm:
            zipapp.main(args)
        # Program should exit with a non-zero return code.
        self.assertTrue(cm.exception.code)


if __name__ == "__main__":
    unittest.main()
