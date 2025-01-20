import typing
import os
import pathlib
import py_compile
import shutil
import textwrap
import unittest
import warnings
import importlib
import contextlib

from importlib import resources
from importlib.resources.abc import Traversable
from . import data01
from . import util
from . import _path
from test.support import os_helper, import_helper


@contextlib.contextmanager
def suppress_known_deprecation():
    with warnings.catch_warnings(record=True) as ctx:
        warnings.simplefilter('default', category=DeprecationWarning)
        yield ctx


class FilesTests:
    def test_read_bytes(self):
        files = resources.files(self.data)
        actual = files.joinpath('utf-8.file').read_bytes()
        assert actual == b'Hello, UTF-8 world!\n'

    def test_read_text(self):
        files = resources.files(self.data)
        actual = files.joinpath('utf-8.file').read_text(encoding='utf-8')
        assert actual == 'Hello, UTF-8 world!\n'

    @unittest.skipUnless(
        hasattr(typing, 'runtime_checkable'),
        "Only suitable when typing supports runtime_checkable",
    )
    def test_traversable(self):
        assert isinstance(resources.files(self.data), Traversable)

    def test_old_parameter(self):
        """
        Files used to take a 'package' parameter. Make sure anyone
        passing by name is still supported.
        """
        with suppress_known_deprecation():
            resources.files(package=self.data)


class OpenDiskTests(FilesTests, unittest.TestCase):
    def setUp(self):
        self.data = data01


class OpenZipTests(FilesTests, util.ZipSetup, unittest.TestCase):
    pass


class OpenNamespaceTests(FilesTests, unittest.TestCase):
    def setUp(self):
        from . import namespacedata01

        self.data = namespacedata01


class SiteDir:
    def setUp(self):
        self.fixtures = contextlib.ExitStack()
        self.addCleanup(self.fixtures.close)
        self.site_dir = self.fixtures.enter_context(os_helper.temp_dir())
        self.fixtures.enter_context(import_helper.DirsOnSysPath(self.site_dir))
        self.fixtures.enter_context(import_helper.isolated_modules())


class ModulesFilesTests(SiteDir, unittest.TestCase):
    def test_module_resources(self):
        """
        A module can have resources found adjacent to the module.
        """
        spec = {
            'mod.py': '',
            'res.txt': 'resources are the best',
        }
        _path.build(spec, self.site_dir)
        import mod

        actual = resources.files(mod).joinpath('res.txt').read_text(encoding='utf-8')
        assert actual == spec['res.txt']


class ImplicitContextFilesTests(SiteDir, unittest.TestCase):
    def test_implicit_files(self):
        """
        Without any parameter, files() will infer the location as the caller.
        """
        spec = {
            'somepkg': {
                '__init__.py': textwrap.dedent(
                    """
                    import importlib.resources as res
                    val = res.files().joinpath('res.txt').read_text(encoding='utf-8')
                    """
                ),
                'res.txt': 'resources are the best',
            },
        }
        _path.build(spec, self.site_dir)
        assert importlib.import_module('somepkg').val == 'resources are the best'

    def test_implicit_files_zip_submodule(self):
        """
        Special test for gh-121735 for Python 3.12.
        """
        import zipfile

        def create_zip_from_directory(source_dir, zip_filename):
            with zipfile.ZipFile(zip_filename, 'w') as zipf:
                for root, _, files in os.walk(source_dir):
                    for file in files:
                        file_path = os.path.join(root, file)
                        # Ensure files are at the root
                        arcname = os.path.relpath(file_path, source_dir)
                        zipf.write(file_path, arcname)

        set_val = textwrap.dedent(
            """
            import importlib.resources as res
            val = res.files().joinpath('res.txt').read_text(encoding='utf-8')
            """
        )
        spec = {
            'somepkg': {
                '__init__.py': set_val,
                'submod.py': set_val,
                'res.txt': 'resources are the best',
            },
        }
        build_dir = self.fixtures.enter_context(os_helper.temp_dir())
        _path.build(spec, build_dir)
        zip_file = os.path.join(self.site_dir, 'thepkg.zip')
        create_zip_from_directory(build_dir, zip_file)
        self.fixtures.enter_context(import_helper.DirsOnSysPath(zip_file))
        assert importlib.import_module('somepkg.submod').val == 'resources are the best'

    def _compile_importlib(self):
        """
        Make a compiled-only copy of the importlib resources package.

        Currently only code is copied, as importlib resources doesn't itself
        have any resources.
        """
        bin_site = self.fixtures.enter_context(os_helper.temp_dir())
        c_resources = pathlib.Path(bin_site, 'c_resources')
        sources = pathlib.Path(resources.__file__).parent

        for source_path in sources.glob('**/*.py'):
            c_path = c_resources.joinpath(source_path.relative_to(sources)).with_suffix('.pyc')
            py_compile.compile(source_path, c_path)
        self.fixtures.enter_context(import_helper.DirsOnSysPath(bin_site))

    def test_implicit_files_with_compiled_importlib(self):
        """
        Caller detection works for compiled-only resources module.

        python/cpython#123085
        """
        set_val = textwrap.dedent(
            f"""
            import {resources.__name__} as res
            val = res.files().joinpath('res.txt').read_text(encoding='utf-8')
            """
        )
        spec = {
            'frozenpkg': {
                '__init__.py': set_val.replace(resources.__name__, 'c_resources'),
                'res.txt': 'resources are the best',
            },
        }
        _path.build(spec, self.site_dir)
        self._compile_importlib()
        assert importlib.import_module('frozenpkg').val == 'resources are the best'


if __name__ == '__main__':
    unittest.main()
