import os.path
import pathlib
import unittest
from importlib import import_module
from importlib.resources.readers import MultiplexedPath, NamespaceReader

from . import util


class MultiplexedPathTest(util.DiskSetup, unittest.TestCase):
    MODULE = 'namespacedata01'

    def setUp(self):
        super().setUp()
        self.folder = pathlib.Path(self.data.__path__[0])
        self.data01 = pathlib.Path(self.load_fixture('data01').__file__).parent
        self.data02 = pathlib.Path(self.load_fixture('data02').__file__).parent

    def test_init_no_paths(self):
        with self.assertRaises(FileNotFoundError):
            MultiplexedPath()

    def test_init_file(self):
        with self.assertRaises(NotADirectoryError):
            MultiplexedPath(self.folder / 'binary.file')

    def test_iterdir(self):
        contents = {path.name for path in MultiplexedPath(self.folder).iterdir()}
        try:
            contents.remove('__pycache__')
        except (KeyError, ValueError):
            pass
        assert contents == {'subdirectory', 'binary.file', 'utf-16.file', 'utf-8.file'}

    def test_iterdir_duplicate(self):
        contents = {
            path.name for path in MultiplexedPath(self.folder, self.data01).iterdir()
        }
        for remove in ('__pycache__', '__init__.pyc'):
            try:
                contents.remove(remove)
            except (KeyError, ValueError):
                pass
        assert contents == {
            '__init__.py',
            'binary.file',
            'subdirectory',
            'utf-16.file',
            'utf-8.file',
        }

    def test_is_dir(self):
        assert MultiplexedPath(self.folder).is_dir()

    def test_is_file(self):
        assert not MultiplexedPath(self.folder).is_file()

    def test_open_file(self):
        path = MultiplexedPath(self.folder)
        with self.assertRaises(FileNotFoundError):
            path.read_bytes()
        with self.assertRaises(FileNotFoundError):
            path.read_text()
        with self.assertRaises(FileNotFoundError):
            path.open()

    def test_join_path(self):
        prefix = str(self.folder.parent)
        path = MultiplexedPath(self.folder, self.data01)
        assert str(path.joinpath('binary.file'))[len(prefix) + 1 :] == os.path.join(
            'namespacedata01', 'binary.file'
        )
        sub = path.joinpath('subdirectory')
        assert isinstance(sub, MultiplexedPath)
        assert 'namespacedata01' in str(sub)
        assert 'data01' in str(sub)
        assert str(path.joinpath('imaginary'))[len(prefix) + 1 :] == os.path.join(
            'namespacedata01', 'imaginary'
        )
        assert path.joinpath() == path

    def test_join_path_compound(self):
        path = MultiplexedPath(self.folder)
        assert not path.joinpath('imaginary/foo.py').exists()

    def test_join_path_common_subdir(self):
        prefix = str(self.data02.parent)
        path = MultiplexedPath(self.data01, self.data02)
        assert isinstance(path.joinpath('subdirectory'), MultiplexedPath)
        assert str(path.joinpath('subdirectory', 'subsubdir'))[len(prefix) + 1 :] == (
            os.path.join('data02', 'subdirectory', 'subsubdir')
        )

    def test_repr(self):
        assert repr(MultiplexedPath(self.folder)) == f"MultiplexedPath('{self.folder}')"

    def test_name(self):
        assert MultiplexedPath(self.folder).name == os.path.basename(self.folder)


class NamespaceReaderTest(util.DiskSetup, unittest.TestCase):
    MODULE = 'namespacedata01'

    def test_init_error(self):
        with self.assertRaises(ValueError):
            NamespaceReader(['path1', 'path2'])

    def test_resource_path(self):
        namespacedata01 = import_module('namespacedata01')
        reader = NamespaceReader(namespacedata01.__spec__.submodule_search_locations)

        root = self.data.__path__[0]
        assert reader.resource_path('binary.file') == os.path.join(root, 'binary.file')
        assert reader.resource_path('imaginary') == os.path.join(root, 'imaginary')

    def test_files(self):
        reader = NamespaceReader(self.data.__spec__.submodule_search_locations)
        root = self.data.__path__[0]
        assert isinstance(reader.files(), MultiplexedPath)
        assert repr(reader.files()) == f"MultiplexedPath('{root}')"


if __name__ == '__main__':
    unittest.main()
