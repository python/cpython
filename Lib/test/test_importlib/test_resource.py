import sys
import unittest
import uuid

from . import data01
from . import zipdata01, zipdata02
from . import util
from importlib import resources, import_module
from pathlib import Path
from test import support


class ResourceTests:
    # Subclasses are expected to set the `data` attribute.

    def test_is_resource_good_path(self):
        self.assertTrue(resources.is_resource(self.data, 'binary.file'))

    def test_is_resource_missing(self):
        self.assertFalse(resources.is_resource(self.data, 'not-a-file'))

    def test_is_resource_subresource_directory(self):
        # Directories are not resources.
        self.assertFalse(resources.is_resource(self.data, 'subdirectory'))

    def test_contents(self):
        contents = set(resources.contents(self.data))
        # There may be cruft in the directory listing of the data directory.
        # Under Python 3 we could have a __pycache__ directory, and under
        # Python 2 we could have .pyc files.  These are both artifacts of the
        # test suite importing these modules and writing these caches.  They
        # aren't germane to this test, so just filter them out.
        contents.discard('__pycache__')
        contents.discard('__init__.pyc')
        contents.discard('__init__.pyo')
        self.assertEqual(contents, {
            '__init__.py',
            'subdirectory',
            'utf-8.file',
            'binary.file',
            'utf-16.file',
            })


class ResourceDiskTests(ResourceTests, unittest.TestCase):
    def setUp(self):
        self.data = data01


class ResourceZipTests(ResourceTests, util.ZipSetup, unittest.TestCase):
    pass


class ResourceLoaderTests(unittest.TestCase):
    def test_resource_contents(self):
        package = util.create_package(
            file=data01, path=data01.__file__, contents=['A', 'B', 'C'])
        self.assertEqual(
            set(resources.contents(package)),
            {'A', 'B', 'C'})

    def test_resource_is_resource(self):
        package = util.create_package(
            file=data01, path=data01.__file__,
            contents=['A', 'B', 'C', 'D/E', 'D/F'])
        self.assertTrue(resources.is_resource(package, 'B'))

    def test_resource_directory_is_not_resource(self):
        package = util.create_package(
            file=data01, path=data01.__file__,
            contents=['A', 'B', 'C', 'D/E', 'D/F'])
        self.assertFalse(resources.is_resource(package, 'D'))

    def test_resource_missing_is_not_resource(self):
        package = util.create_package(
            file=data01, path=data01.__file__,
            contents=['A', 'B', 'C', 'D/E', 'D/F'])
        self.assertFalse(resources.is_resource(package, 'Z'))


class ResourceCornerCaseTests(unittest.TestCase):
    def test_package_has_no_reader_fallback(self):
        # Test odd ball packages which:
        # 1. Do not have a ResourceReader as a loader
        # 2. Are not on the file system
        # 3. Are not in a zip file
        module = util.create_package(
            file=data01, path=data01.__file__, contents=['A', 'B', 'C'])
        # Give the module a dummy loader.
        module.__loader__ = object()
        # Give the module a dummy origin.
        module.__file__ = '/path/which/shall/not/be/named'
        if sys.version_info >= (3,):
            module.__spec__.loader = module.__loader__
            module.__spec__.origin = module.__file__
        self.assertFalse(resources.is_resource(module, 'A'))


class ResourceFromZipsTest(util.ZipSetupBase, unittest.TestCase):
    ZIP_MODULE = zipdata02                          # type: ignore

    def test_unrelated_contents(self):
        # https://gitlab.com/python-devs/importlib_resources/issues/44
        #
        # Here we have a zip file with two unrelated subpackages.  The bug
        # reports that getting the contents of a resource returns unrelated
        # files.
        self.assertEqual(
            set(resources.contents('ziptestdata.one')),
            {'__init__.py', 'resource1.txt'})
        self.assertEqual(
            set(resources.contents('ziptestdata.two')),
            {'__init__.py', 'resource2.txt'})


class SubdirectoryResourceFromZipsTest(util.ZipSetupBase, unittest.TestCase):
    ZIP_MODULE = zipdata01                          # type: ignore

    def test_is_submodule_resource(self):
        submodule = import_module('ziptestdata.subdirectory')
        self.assertTrue(
            resources.is_resource(submodule, 'binary.file'))

    def test_read_submodule_resource_by_name(self):
        self.assertTrue(
            resources.is_resource('ziptestdata.subdirectory', 'binary.file'))

    def test_submodule_contents(self):
        submodule = import_module('ziptestdata.subdirectory')
        self.assertEqual(
            set(resources.contents(submodule)),
            {'__init__.py', 'binary.file'})

    def test_submodule_contents_by_name(self):
        self.assertEqual(
            set(resources.contents('ziptestdata.subdirectory')),
            {'__init__.py', 'binary.file'})


class NamespaceTest(unittest.TestCase):
    def test_namespaces_cannot_have_resources(self):
        contents = resources.contents('test.test_importlib.data03.namespace')
        self.assertFalse(list(contents))
        # Even though there is a file in the namespace directory, it is not
        # considered a resource, since namespace packages can't have them.
        self.assertFalse(resources.is_resource(
            'test.test_importlib.data03.namespace',
            'resource1.txt'))
        # We should get an exception if we try to read it or open it.
        self.assertRaises(
            FileNotFoundError,
            resources.open_text,
            'test.test_importlib.data03.namespace', 'resource1.txt')
        self.assertRaises(
            FileNotFoundError,
            resources.open_binary,
            'test.test_importlib.data03.namespace', 'resource1.txt')
        self.assertRaises(
            FileNotFoundError,
            resources.read_text,
            'test.test_importlib.data03.namespace', 'resource1.txt')
        self.assertRaises(
            FileNotFoundError,
            resources.read_binary,
            'test.test_importlib.data03.namespace', 'resource1.txt')


class DeletingZipsTest(unittest.TestCase):
    """Having accessed resources in a zip file should not keep an open
    reference to the zip.
    """
    ZIP_MODULE = zipdata01

    def setUp(self):
        modules = support.modules_setup()
        self.addCleanup(support.modules_cleanup, *modules)

        data_path = Path(self.ZIP_MODULE.__file__)
        data_dir = data_path.parent
        self.source_zip_path = data_dir / 'ziptestdata.zip'
        self.zip_path = Path.cwd() / '{}.zip'.format(uuid.uuid4())
        self.zip_path.write_bytes(self.source_zip_path.read_bytes())
        sys.path.append(str(self.zip_path))
        self.data = import_module('ziptestdata')

    def tearDown(self):
        try:
            sys.path.remove(str(self.zip_path))
        except ValueError:
            pass

        try:
            del sys.path_importer_cache[str(self.zip_path)]
            del sys.modules[self.data.__name__]
        except KeyError:
            pass

        try:
            support.unlink(self.zip_path)
        except OSError:
            # If the test fails, this will probably fail too
            pass

    def test_contents_does_not_keep_open(self):
        c = resources.contents('ziptestdata')
        self.zip_path.unlink()

    def test_is_resource_does_not_keep_open(self):
        c = resources.is_resource('ziptestdata', 'binary.file')
        self.zip_path.unlink()

    def test_is_resource_failure_does_not_keep_open(self):
        c = resources.is_resource('ziptestdata', 'not-present')
        self.zip_path.unlink()

    def test_path_does_not_keep_open(self):
        c = resources.path('ziptestdata', 'binary.file')
        self.zip_path.unlink()

    def test_entered_path_does_not_keep_open(self):
        # This is what certifi does on import to make its bundle
        # available for the process duration.
        c = resources.path('ziptestdata', 'binary.file').__enter__()
        self.zip_path.unlink()

    def test_read_binary_does_not_keep_open(self):
        c = resources.read_binary('ziptestdata', 'binary.file')
        self.zip_path.unlink()

    def test_read_text_does_not_keep_open(self):
        c = resources.read_text('ziptestdata', 'utf-8.file', encoding='utf-8')
        self.zip_path.unlink()

if __name__ == '__main__':
    unittest.main()
