from .. import abc
from .. import util

machinery = util.import_importlib('importlib.machinery')

import errno
import os
import py_compile
import stat
import sys
import tempfile
from test.support.import_helper import make_legacy_pyc
import unittest
import warnings


class FinderTests(abc.FinderTests):

    """For a top-level module, it should just be found directly in the
    directory being searched. This is true for a directory with source
    [top-level source], bytecode [top-level bc], or both [top-level both].
    There is also the possibility that it is a package [top-level package], in
    which case there will be a directory with the module name and an
    __init__.py file. If there is a directory without an __init__.py an
    ImportWarning is returned [empty dir].

    For sub-modules and sub-packages, the same happens as above but only use
    the tail end of the name [sub module] [sub package] [sub empty].

    When there is a conflict between a package and module having the same name
    in the same directory, the package wins out [package over module]. This is
    so that imports of modules within the package can occur rather than trigger
    an import error.

    When there is a package and module with the same name, always pick the
    package over the module [package over module]. This is so that imports from
    the package have the possibility of succeeding.

    """

    def get_finder(self, root):
        loader_details = [(self.machinery.SourceFileLoader,
                            self.machinery.SOURCE_SUFFIXES),
                          (self.machinery.SourcelessFileLoader,
                            self.machinery.BYTECODE_SUFFIXES)]
        return self.machinery.FileFinder(root, *loader_details)

    def import_(self, root, module):
        finder = self.get_finder(root)
        return self._find(finder, module, loader_only=True)

    def run_test(self, test, create=None, *, compile_=None, unlink=None):
        """Test the finding of 'test' with the creation of modules listed in
        'create'.

        Any names listed in 'compile_' are byte-compiled. Modules
        listed in 'unlink' have their source files deleted.

        """
        if create is None:
            create = {test}
        with util.create_modules(*create) as mapping:
            if compile_:
                for name in compile_:
                    py_compile.compile(mapping[name])
            if unlink:
                for name in unlink:
                    os.unlink(mapping[name])
                    try:
                        make_legacy_pyc(mapping[name])
                    except OSError as error:
                        # Some tests do not set compile_=True so the source
                        # module will not get compiled and there will be no
                        # PEP 3147 pyc file to rename.
                        if error.errno != errno.ENOENT:
                            raise
            loader = self.import_(mapping['.root'], test)
            self.assertTrue(hasattr(loader, 'load_module'))
            return loader

    def test_module(self):
        # [top-level source]
        self.run_test('top_level')
        # [top-level bc]
        self.run_test('top_level', compile_={'top_level'},
                      unlink={'top_level'})
        # [top-level both]
        self.run_test('top_level', compile_={'top_level'})

    # [top-level package]
    def test_package(self):
        # Source.
        self.run_test('pkg', {'pkg.__init__'})
        # Bytecode.
        self.run_test('pkg', {'pkg.__init__'}, compile_={'pkg.__init__'},
                unlink={'pkg.__init__'})
        # Both.
        self.run_test('pkg', {'pkg.__init__'}, compile_={'pkg.__init__'})

    # [sub module]
    def test_module_in_package(self):
        with util.create_modules('pkg.__init__', 'pkg.sub') as mapping:
            pkg_dir = os.path.dirname(mapping['pkg.__init__'])
            loader = self.import_(pkg_dir, 'pkg.sub')
            self.assertTrue(hasattr(loader, 'load_module'))

    # [sub package]
    def test_package_in_package(self):
        context = util.create_modules('pkg.__init__', 'pkg.sub.__init__')
        with context as mapping:
            pkg_dir = os.path.dirname(mapping['pkg.__init__'])
            loader = self.import_(pkg_dir, 'pkg.sub')
            self.assertTrue(hasattr(loader, 'load_module'))

    # [package over modules]
    def test_package_over_module(self):
        name = '_temp'
        loader = self.run_test(name, {'{0}.__init__'.format(name), name})
        self.assertIn('__init__', loader.get_filename(name))

    def test_failure(self):
        with util.create_modules('blah') as mapping:
            nothing = self.import_(mapping['.root'], 'sdfsadsadf')
            self.assertIsNone(nothing)

    def test_empty_string_for_dir(self):
        # The empty string from sys.path means to search in the cwd.
        finder = self.machinery.FileFinder('', (self.machinery.SourceFileLoader,
            self.machinery.SOURCE_SUFFIXES))
        with open('mod.py', 'w', encoding='utf-8') as file:
            file.write("# test file for importlib")
        try:
            loader = self._find(finder, 'mod', loader_only=True)
            self.assertTrue(hasattr(loader, 'load_module'))
        finally:
            os.unlink('mod.py')

    def test_invalidate_caches(self):
        # invalidate_caches() should reset the mtime.
        finder = self.machinery.FileFinder('', (self.machinery.SourceFileLoader,
            self.machinery.SOURCE_SUFFIXES))
        finder._path_mtime = 42
        finder.invalidate_caches()
        self.assertEqual(finder._path_mtime, -1)

    # Regression test for http://bugs.python.org/issue14846
    def test_dir_removal_handling(self):
        mod = 'mod'
        with util.create_modules(mod) as mapping:
            finder = self.get_finder(mapping['.root'])
            found = self._find(finder, 'mod', loader_only=True)
            self.assertIsNotNone(found)
        found = self._find(finder, 'mod', loader_only=True)
        self.assertIsNone(found)

    @unittest.skipUnless(sys.platform != 'win32',
            'os.chmod() does not support the needed arguments under Windows')
    def test_no_read_directory(self):
        # Issue #16730
        tempdir = tempfile.TemporaryDirectory()
        original_mode = os.stat(tempdir.name).st_mode
        def cleanup(tempdir):
            """Cleanup function for the temporary directory.

            Since we muck with the permissions, we want to set them back to
            their original values to make sure the directory can be properly
            cleaned up.

            """
            os.chmod(tempdir.name, original_mode)
            # If this is not explicitly called then the __del__ method is used,
            # but since already mucking around might as well explicitly clean
            # up.
            tempdir.__exit__(None, None, None)
        self.addCleanup(cleanup, tempdir)
        os.chmod(tempdir.name, stat.S_IWUSR | stat.S_IXUSR)
        finder = self.get_finder(tempdir.name)
        found = self._find(finder, 'doesnotexist')
        self.assertEqual(found, self.NOT_FOUND)

    def test_ignore_file(self):
        # If a directory got changed to a file from underneath us, then don't
        # worry about looking for submodules.
        with tempfile.NamedTemporaryFile() as file_obj:
            finder = self.get_finder(file_obj.name)
            found = self._find(finder, 'doesnotexist')
            self.assertEqual(found, self.NOT_FOUND)


class FinderTestsPEP451(FinderTests):

    NOT_FOUND = None

    def _find(self, finder, name, loader_only=False):
        spec = finder.find_spec(name)
        return spec.loader if spec is not None else spec


(Frozen_FinderTestsPEP451,
 Source_FinderTestsPEP451
 ) = util.test_both(FinderTestsPEP451, machinery=machinery)


class FinderTestsPEP420(FinderTests):

    NOT_FOUND = (None, [])

    def _find(self, finder, name, loader_only=False):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            loader_portions = finder.find_loader(name)
            return loader_portions[0] if loader_only else loader_portions


(Frozen_FinderTestsPEP420,
 Source_FinderTestsPEP420
 ) = util.test_both(FinderTestsPEP420, machinery=machinery)


class FinderTestsPEP302(FinderTests):

    NOT_FOUND = None

    def _find(self, finder, name, loader_only=False):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            return finder.find_module(name)


(Frozen_FinderTestsPEP302,
 Source_FinderTestsPEP302
 ) = util.test_both(FinderTestsPEP302, machinery=machinery)


if __name__ == '__main__':
    unittest.main()
