from .. import abc
from . import util as source_util

from importlib import machinery
import errno
import imp
import os
import py_compile
from test.support import make_legacy_pyc
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

    def import_(self, root, module):
        loader_details = [(machinery.SourceFileLoader,
                            machinery.SOURCE_SUFFIXES, True),
                          (machinery.SourcelessFileLoader,
                            machinery.BYTECODE_SUFFIXES, True)]
        finder = machinery.FileFinder(root, *loader_details)
        return finder.find_module(module)

    def run_test(self, test, create=None, *, compile_=None, unlink=None):
        """Test the finding of 'test' with the creation of modules listed in
        'create'.

        Any names listed in 'compile_' are byte-compiled. Modules
        listed in 'unlink' have their source files deleted.

        """
        if create is None:
            create = {test}
        with source_util.create_modules(*create) as mapping:
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
        with source_util.create_modules('pkg.__init__', 'pkg.sub') as mapping:
            pkg_dir = os.path.dirname(mapping['pkg.__init__'])
            loader = self.import_(pkg_dir, 'pkg.sub')
            self.assertTrue(hasattr(loader, 'load_module'))

    # [sub package]
    def test_package_in_package(self):
        context = source_util.create_modules('pkg.__init__', 'pkg.sub.__init__')
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
        with source_util.create_modules('blah') as mapping:
            nothing = self.import_(mapping['.root'], 'sdfsadsadf')
            self.assertTrue(nothing is None)

    def test_empty_string_for_dir(self):
        # The empty string from sys.path means to search in the cwd.
        finder = machinery.FileFinder('', (machinery.SourceFileLoader,
            machinery.SOURCE_SUFFIXES, True))
        with open('mod.py', 'w') as file:
            file.write("# test file for importlib")
        try:
            loader = finder.find_module('mod')
            self.assertTrue(hasattr(loader, 'load_module'))
        finally:
            os.unlink('mod.py')

    def test_invalidate_caches(self):
        # invalidate_caches() should reset the mtime.
        finder = machinery.FileFinder('', (machinery.SourceFileLoader,
            machinery.SOURCE_SUFFIXES, True))
        finder._path_mtime = 42
        finder.invalidate_caches()
        self.assertEqual(finder._path_mtime, -1)


def test_main():
    from test.support import run_unittest
    run_unittest(FinderTests)


if __name__ == '__main__':
    test_main()
