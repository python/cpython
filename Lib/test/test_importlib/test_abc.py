import contextlib
import inspect
import io
import marshal
import os
import sys
from test import support
import types
import unittest
from unittest import mock

from . import util

frozen_init, source_init = util.import_importlib('importlib')
frozen_abc, source_abc = util.import_importlib('importlib.abc')
frozen_util, source_util = util.import_importlib('importlib.util')

##### Inheritance ##############################################################
class InheritanceTests:

    """Test that the specified class is a subclass/superclass of the expected
    classes."""

    subclasses = []
    superclasses = []

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.superclasses = [getattr(self.abc, class_name)
                             for class_name in self.superclass_names]
        if hasattr(self, 'subclass_names'):
            # Because test.support.import_fresh_module() creates a new
            # importlib._bootstrap per module, inheritance checks fail when
            # checking across module boundaries (i.e. the _bootstrap in abc is
            # not the same as the one in machinery). That means stealing one of
            # the modules from the other to make sure the same instance is used.
            self.subclasses = [getattr(self.abc.machinery, class_name)
                                for class_name in self.subclass_names]
        assert self.subclasses or self.superclasses, self.__class__
        testing = self.__class__.__name__.partition('_')[2]
        self.__test = getattr(self.abc, testing)

    def test_subclasses(self):
        # Test that the expected subclasses inherit.
        for subclass in self.subclasses:
            self.assertTrue(issubclass(subclass, self.__test),
                "{0} is not a subclass of {1}".format(subclass, self.__test))

    def test_superclasses(self):
        # Test that the class inherits from the expected superclasses.
        for superclass in self.superclasses:
            self.assertTrue(issubclass(self.__test, superclass),
               "{0} is not a superclass of {1}".format(superclass, self.__test))

def create_inheritance_tests(base_class):
    def set_frozen(ns):
        ns['abc'] = frozen_abc
    def set_source(ns):
        ns['abc'] = source_abc

    classes = []
    for prefix, ns_set in [('Frozen', set_frozen), ('Source', set_source)]:
        classes.append(types.new_class('_'.join([prefix, base_class.__name__]),
                                       (base_class, unittest.TestCase),
                                       exec_body=ns_set))
    return classes


class MetaPathFinder(InheritanceTests):
    superclass_names = ['Finder']
    subclass_names = ['BuiltinImporter', 'FrozenImporter', 'PathFinder',
                      'WindowsRegistryFinder']

tests = create_inheritance_tests(MetaPathFinder)
Frozen_MetaPathFinderInheritanceTests, Source_MetaPathFinderInheritanceTests = tests


class PathEntryFinder(InheritanceTests):
    superclass_names = ['Finder']
    subclass_names = ['FileFinder']

tests = create_inheritance_tests(PathEntryFinder)
Frozen_PathEntryFinderInheritanceTests, Source_PathEntryFinderInheritanceTests = tests


class ResourceLoader(InheritanceTests):
    superclass_names = ['Loader']

tests = create_inheritance_tests(ResourceLoader)
Frozen_ResourceLoaderInheritanceTests, Source_ResourceLoaderInheritanceTests = tests


class InspectLoader(InheritanceTests):
    superclass_names = ['Loader']
    subclass_names = ['BuiltinImporter', 'FrozenImporter', 'ExtensionFileLoader']

tests = create_inheritance_tests(InspectLoader)
Frozen_InspectLoaderInheritanceTests, Source_InspectLoaderInheritanceTests = tests


class ExecutionLoader(InheritanceTests):
    superclass_names = ['InspectLoader']
    subclass_names = ['ExtensionFileLoader']

tests = create_inheritance_tests(ExecutionLoader)
Frozen_ExecutionLoaderInheritanceTests, Source_ExecutionLoaderInheritanceTests = tests


class FileLoader(InheritanceTests):
    superclass_names = ['ResourceLoader', 'ExecutionLoader']
    subclass_names = ['SourceFileLoader', 'SourcelessFileLoader']

tests = create_inheritance_tests(FileLoader)
Frozen_FileLoaderInheritanceTests, Source_FileLoaderInheritanceTests = tests


class SourceLoader(InheritanceTests):
    superclass_names = ['ResourceLoader', 'ExecutionLoader']
    subclass_names = ['SourceFileLoader']

tests = create_inheritance_tests(SourceLoader)
Frozen_SourceLoaderInheritanceTests, Source_SourceLoaderInheritanceTests = tests

##### Default return values ####################################################
def make_abc_subclasses(base_class):
    classes = []
    for kind, abc in [('Frozen', frozen_abc), ('Source', source_abc)]:
        name = '_'.join([kind, base_class.__name__])
        base_classes = base_class, getattr(abc, base_class.__name__)
        classes.append(types.new_class(name, base_classes))
    return classes

def make_return_value_tests(base_class, test_class):
    frozen_class, source_class = make_abc_subclasses(base_class)
    tests = []
    for prefix, class_in_test in [('Frozen', frozen_class), ('Source', source_class)]:
        def set_ns(ns):
            ns['ins'] = class_in_test()
        tests.append(types.new_class('_'.join([prefix, test_class.__name__]),
                                     (test_class, unittest.TestCase),
                                     exec_body=set_ns))
    return tests


class MetaPathFinder:

    def find_module(self, fullname, path):
        return super().find_module(fullname, path)

Frozen_MPF, Source_MPF = make_abc_subclasses(MetaPathFinder)


class MetaPathFinderDefaultsTests:

    def test_find_module(self):
        # Default should return None.
        self.assertIsNone(self.ins.find_module('something', None))

    def test_invalidate_caches(self):
        # Calling the method is a no-op.
        self.ins.invalidate_caches()


tests = make_return_value_tests(MetaPathFinder, MetaPathFinderDefaultsTests)
Frozen_MPFDefaultTests, Source_MPFDefaultTests = tests


class PathEntryFinder:

    def find_loader(self, fullname):
        return super().find_loader(fullname)

Frozen_PEF, Source_PEF = make_abc_subclasses(PathEntryFinder)


class PathEntryFinderDefaultsTests:

    def test_find_loader(self):
        self.assertEqual((None, []), self.ins.find_loader('something'))

    def find_module(self):
        self.assertEqual(None, self.ins.find_module('something'))

    def test_invalidate_caches(self):
        # Should be a no-op.
        self.ins.invalidate_caches()


tests = make_return_value_tests(PathEntryFinder, PathEntryFinderDefaultsTests)
Frozen_PEFDefaultTests, Source_PEFDefaultTests = tests


class Loader:

    def load_module(self, fullname):
        return super().load_module(fullname)


Frozen_L, Source_L = make_abc_subclasses(Loader)


class LoaderDefaultsTests:

    def test_load_module(self):
        with self.assertRaises(ImportError):
            self.ins.load_module('something')

    def test_module_repr(self):
        mod = types.ModuleType('blah')
        with self.assertRaises(NotImplementedError):
            self.ins.module_repr(mod)
        original_repr = repr(mod)
        mod.__loader__ = self.ins
        # Should still return a proper repr.
        self.assertTrue(repr(mod))


tests = make_return_value_tests(Loader, LoaderDefaultsTests)
Frozen_LDefaultTests, SourceLDefaultTests = tests


class ResourceLoader(Loader):

    def get_data(self, path):
        return super().get_data(path)


Frozen_RL, Source_RL = make_abc_subclasses(ResourceLoader)


class ResourceLoaderDefaultsTests:

    def test_get_data(self):
        with self.assertRaises(IOError):
            self.ins.get_data('/some/path')


tests = make_return_value_tests(ResourceLoader, ResourceLoaderDefaultsTests)
Frozen_RLDefaultTests, Source_RLDefaultTests = tests


class InspectLoader(Loader):

    def is_package(self, fullname):
        return super().is_package(fullname)

    def get_source(self, fullname):
        return super().get_source(fullname)


Frozen_IL, Source_IL = make_abc_subclasses(InspectLoader)


class InspectLoaderDefaultsTests:

    def test_is_package(self):
        with self.assertRaises(ImportError):
            self.ins.is_package('blah')

    def test_get_source(self):
        with self.assertRaises(ImportError):
            self.ins.get_source('blah')


tests = make_return_value_tests(InspectLoader, InspectLoaderDefaultsTests)
Frozen_ILDefaultTests, Source_ILDefaultTests = tests


class ExecutionLoader(InspectLoader):

    def get_filename(self, fullname):
        return super().get_filename(fullname)

Frozen_EL, Source_EL = make_abc_subclasses(ExecutionLoader)


class ExecutionLoaderDefaultsTests:

    def test_get_filename(self):
        with self.assertRaises(ImportError):
            self.ins.get_filename('blah')


tests = make_return_value_tests(ExecutionLoader, InspectLoaderDefaultsTests)
Frozen_ELDefaultTests, Source_ELDefaultsTests = tests

##### Loader concrete methods ##################################################
class LoaderConcreteMethodTests:

    def test_init_module_attrs(self):
        loader = self.LoaderSubclass()
        module = types.ModuleType('blah')
        loader.init_module_attrs(module)
        self.assertEqual(module.__loader__, loader)


class Frozen_LoaderConcreateMethodTests(LoaderConcreteMethodTests, unittest.TestCase):
    LoaderSubclass = Frozen_L

class Source_LoaderConcreateMethodTests(LoaderConcreteMethodTests, unittest.TestCase):
    LoaderSubclass = Source_L


##### InspectLoader concrete methods ###########################################
class InspectLoaderSourceToCodeTests:

    def source_to_module(self, data, path=None):
        """Help with source_to_code() tests."""
        module = types.ModuleType('blah')
        loader = self.InspectLoaderSubclass()
        if path is None:
            code = loader.source_to_code(data)
        else:
            code = loader.source_to_code(data, path)
        exec(code, module.__dict__)
        return module

    def test_source_to_code_source(self):
        # Since compile() can handle strings, so should source_to_code().
        source = 'attr = 42'
        module = self.source_to_module(source)
        self.assertTrue(hasattr(module, 'attr'))
        self.assertEqual(module.attr, 42)

    def test_source_to_code_bytes(self):
        # Since compile() can handle bytes, so should source_to_code().
        source = b'attr = 42'
        module = self.source_to_module(source)
        self.assertTrue(hasattr(module, 'attr'))
        self.assertEqual(module.attr, 42)

    def test_source_to_code_path(self):
        # Specifying a path should set it for the code object.
        path = 'path/to/somewhere'
        loader = self.InspectLoaderSubclass()
        code = loader.source_to_code('', path)
        self.assertEqual(code.co_filename, path)

    def test_source_to_code_no_path(self):
        # Not setting a path should still work and be set to <string> since that
        # is a pre-existing practice as a default to compile().
        loader = self.InspectLoaderSubclass()
        code = loader.source_to_code('')
        self.assertEqual(code.co_filename, '<string>')


class Frozen_ILSourceToCodeTests(InspectLoaderSourceToCodeTests, unittest.TestCase):
    InspectLoaderSubclass = Frozen_IL

class Source_ILSourceToCodeTests(InspectLoaderSourceToCodeTests, unittest.TestCase):
    InspectLoaderSubclass = Source_IL


class InspectLoaderGetCodeTests:

    def test_get_code(self):
        # Test success.
        module = types.ModuleType('blah')
        with mock.patch.object(self.InspectLoaderSubclass, 'get_source') as mocked:
            mocked.return_value = 'attr = 42'
            loader = self.InspectLoaderSubclass()
            code = loader.get_code('blah')
        exec(code, module.__dict__)
        self.assertEqual(module.attr, 42)

    def test_get_code_source_is_None(self):
        # If get_source() is None then this should be None.
        with mock.patch.object(self.InspectLoaderSubclass, 'get_source') as mocked:
            mocked.return_value = None
            loader = self.InspectLoaderSubclass()
            code = loader.get_code('blah')
        self.assertIsNone(code)

    def test_get_code_source_not_found(self):
        # If there is no source then there is no code object.
        loader = self.InspectLoaderSubclass()
        with self.assertRaises(ImportError):
            loader.get_code('blah')


class Frozen_ILGetCodeTests(InspectLoaderGetCodeTests, unittest.TestCase):
    InspectLoaderSubclass = Frozen_IL

class Source_ILGetCodeTests(InspectLoaderGetCodeTests, unittest.TestCase):
    InspectLoaderSubclass = Source_IL


class InspectLoaderInitModuleTests:

    def mock_is_package(self, return_value):
        return mock.patch.object(self.InspectLoaderSubclass, 'is_package',
                                 return_value=return_value)

    def init_module_attrs(self, name):
        loader = self.InspectLoaderSubclass()
        module = types.ModuleType(name)
        loader.init_module_attrs(module)
        self.assertEqual(module.__loader__, loader)
        return module

    def test_package(self):
        # If a package, then __package__ == __name__, __path__ == []
        with self.mock_is_package(True):
            name = 'blah'
            module = self.init_module_attrs(name)
            self.assertEqual(module.__package__, name)
            self.assertEqual(module.__path__, [])

    def test_toplevel(self):
        # If a module is top-level, __package__ == ''
        with self.mock_is_package(False):
            name = 'blah'
            module = self.init_module_attrs(name)
            self.assertEqual(module.__package__, '')

    def test_submodule(self):
        # If a module is contained within a package then set __package__ to the
        # package name.
        with self.mock_is_package(False):
            name = 'pkg.mod'
            module = self.init_module_attrs(name)
            self.assertEqual(module.__package__, 'pkg')

    def test_is_package_ImportError(self):
        # If is_package() raises ImportError, __package__ should be None and
        # __path__ should not be set.
        with self.mock_is_package(False) as mocked_method:
            mocked_method.side_effect = ImportError
            name = 'mod'
            module = self.init_module_attrs(name)
            self.assertIsNone(module.__package__)
            self.assertFalse(hasattr(module, '__path__'))


class Frozen_ILInitModuleTests(InspectLoaderInitModuleTests, unittest.TestCase):
    InspectLoaderSubclass = Frozen_IL

class Source_ILInitModuleTests(InspectLoaderInitModuleTests, unittest.TestCase):
    InspectLoaderSubclass = Source_IL


class InspectLoaderLoadModuleTests:

    """Test InspectLoader.load_module()."""

    module_name = 'blah'

    def setUp(self):
        support.unload(self.module_name)
        self.addCleanup(support.unload, self.module_name)

    def mock_get_code(self):
        return mock.patch.object(self.InspectLoaderSubclass, 'get_code')

    def test_get_code_ImportError(self):
        # If get_code() raises ImportError, it should propagate.
        with self.mock_get_code() as mocked_get_code:
            mocked_get_code.side_effect = ImportError
            with self.assertRaises(ImportError):
                loader = self.InspectLoaderSubclass()
                loader.load_module(self.module_name)

    def test_get_code_None(self):
        # If get_code() returns None, raise ImportError.
        with self.mock_get_code() as mocked_get_code:
            mocked_get_code.return_value = None
            with self.assertRaises(ImportError):
                loader = self.InspectLoaderSubclass()
                loader.load_module(self.module_name)

    def test_module_returned(self):
        # The loaded module should be returned.
        code = compile('attr = 42', '<string>', 'exec')
        with self.mock_get_code() as mocked_get_code:
            mocked_get_code.return_value = code
            loader = self.InspectLoaderSubclass()
            module = loader.load_module(self.module_name)
            self.assertEqual(module, sys.modules[self.module_name])


class Frozen_ILLoadModuleTests(InspectLoaderLoadModuleTests, unittest.TestCase):
    InspectLoaderSubclass = Frozen_IL

class Source_ILLoadModuleTests(InspectLoaderLoadModuleTests, unittest.TestCase):
    InspectLoaderSubclass = Source_IL


##### ExecutionLoader concrete methods #########################################
class ExecutionLoaderGetCodeTests:

    def mock_methods(self, *, get_source=False, get_filename=False):
        source_mock_context, filename_mock_context = None, None
        if get_source:
            source_mock_context = mock.patch.object(self.ExecutionLoaderSubclass,
                                                    'get_source')
        if get_filename:
            filename_mock_context = mock.patch.object(self.ExecutionLoaderSubclass,
                                                      'get_filename')
        return source_mock_context, filename_mock_context

    def test_get_code(self):
        path = 'blah.py'
        source_mock_context, filename_mock_context = self.mock_methods(
                get_source=True, get_filename=True)
        with source_mock_context as source_mock, filename_mock_context as name_mock:
            source_mock.return_value = 'attr = 42'
            name_mock.return_value = path
            loader = self.ExecutionLoaderSubclass()
            code = loader.get_code('blah')
        self.assertEqual(code.co_filename, path)
        module = types.ModuleType('blah')
        exec(code, module.__dict__)
        self.assertEqual(module.attr, 42)

    def test_get_code_source_is_None(self):
        # If get_source() is None then this should be None.
        source_mock_context, _ = self.mock_methods(get_source=True)
        with source_mock_context as mocked:
            mocked.return_value = None
            loader = self.ExecutionLoaderSubclass()
            code = loader.get_code('blah')
        self.assertIsNone(code)

    def test_get_code_source_not_found(self):
        # If there is no source then there is no code object.
        loader = self.ExecutionLoaderSubclass()
        with self.assertRaises(ImportError):
            loader.get_code('blah')

    def test_get_code_no_path(self):
        # If get_filename() raises ImportError then simply skip setting the path
        # on the code object.
        source_mock_context, filename_mock_context = self.mock_methods(
                get_source=True, get_filename=True)
        with source_mock_context as source_mock, filename_mock_context as name_mock:
            source_mock.return_value = 'attr = 42'
            name_mock.side_effect = ImportError
            loader = self.ExecutionLoaderSubclass()
            code = loader.get_code('blah')
        self.assertEqual(code.co_filename, '<string>')
        module = types.ModuleType('blah')
        exec(code, module.__dict__)
        self.assertEqual(module.attr, 42)


class Frozen_ELGetCodeTests(ExecutionLoaderGetCodeTests, unittest.TestCase):
    ExecutionLoaderSubclass = Frozen_EL

class Source_ELGetCodeTests(ExecutionLoaderGetCodeTests, unittest.TestCase):
    ExecutionLoaderSubclass = Source_EL


class ExecutionLoaderInitModuleTests:

    def mock_is_package(self, return_value):
        return mock.patch.object(self.ExecutionLoaderSubclass, 'is_package',
                                 return_value=return_value)

    @contextlib.contextmanager
    def mock_methods(self, is_package, filename):
        is_package_manager = self.mock_is_package(is_package)
        get_filename_manager = mock.patch.object(self.ExecutionLoaderSubclass,
                'get_filename', return_value=filename)
        with is_package_manager as mock_is_package:
            with get_filename_manager as mock_get_filename:
                yield {'is_package': mock_is_package,
                       'get_filename': mock_get_filename}

    def test_toplevel(self):
        # Verify __loader__, __file__, and __package__; no __path__.
        name = 'blah'
        path = os.path.join('some', 'path', '{}.py'.format(name))
        with self.mock_methods(False, path):
            loader = self.ExecutionLoaderSubclass()
            module = types.ModuleType(name)
            loader.init_module_attrs(module)
        self.assertIs(module.__loader__, loader)
        self.assertEqual(module.__file__, path)
        self.assertEqual(module.__package__, '')
        self.assertFalse(hasattr(module, '__path__'))

    def test_package(self):
        # Verify __loader__, __file__, __package__, and __path__.
        name = 'pkg'
        path = os.path.join('some', 'pkg', '__init__.py')
        with self.mock_methods(True, path):
            loader = self.ExecutionLoaderSubclass()
            module = types.ModuleType(name)
            loader.init_module_attrs(module)
        self.assertIs(module.__loader__, loader)
        self.assertEqual(module.__file__, path)
        self.assertEqual(module.__package__, 'pkg')
        self.assertEqual(module.__path__, [os.path.dirname(path)])

    def test_submodule(self):
        # Verify __package__ and not __path__; test_toplevel() takes care of
        # other attributes.
        name = 'pkg.submodule'
        path = os.path.join('some', 'pkg', 'submodule.py')
        with self.mock_methods(False, path):
            loader = self.ExecutionLoaderSubclass()
            module = types.ModuleType(name)
            loader.init_module_attrs(module)
        self.assertEqual(module.__package__, 'pkg')
        self.assertEqual(module.__file__, path)
        self.assertFalse(hasattr(module, '__path__'))

    def test_get_filename_ImportError(self):
        # If get_filename() raises ImportError, don't set __file__.
        name = 'blah'
        path = 'blah.py'
        with self.mock_methods(False, path) as mocked_methods:
            mocked_methods['get_filename'].side_effect = ImportError
            loader = self.ExecutionLoaderSubclass()
            module = types.ModuleType(name)
            loader.init_module_attrs(module)
        self.assertFalse(hasattr(module, '__file__'))


class Frozen_ELInitModuleTests(ExecutionLoaderInitModuleTests, unittest.TestCase):
    ExecutionLoaderSubclass = Frozen_EL

class Source_ELInitModuleTests(ExecutionLoaderInitModuleTests, unittest.TestCase):
    ExecutionLoaderSubclass = Source_EL


##### SourceLoader concrete methods ############################################
class SourceLoader:

    # Globals that should be defined for all modules.
    source = (b"_ = '::'.join([__name__, __file__, __cached__, __package__, "
              b"repr(__loader__)])")

    def __init__(self, path):
        self.path = path

    def get_data(self, path):
        if path != self.path:
            raise IOError
        return self.source

    def get_filename(self, fullname):
        return self.path

    def module_repr(self, module):
        return '<module>'


Frozen_SourceOnlyL, Source_SourceOnlyL = make_abc_subclasses(SourceLoader)


class SourceLoader(SourceLoader):

    source_mtime = 1

    def __init__(self, path, magic=None):
        super().__init__(path)
        self.bytecode_path = self.util.cache_from_source(self.path)
        self.source_size = len(self.source)
        if magic is None:
            magic = self.util.MAGIC_NUMBER
        data = bytearray(magic)
        data.extend(self.init._w_long(self.source_mtime))
        data.extend(self.init._w_long(self.source_size))
        code_object = compile(self.source, self.path, 'exec',
                                dont_inherit=True)
        data.extend(marshal.dumps(code_object))
        self.bytecode = bytes(data)
        self.written = {}

    def get_data(self, path):
        if path == self.path:
            return super().get_data(path)
        elif path == self.bytecode_path:
            return self.bytecode
        else:
            raise OSError

    def path_stats(self, path):
        if path != self.path:
            raise IOError
        return {'mtime': self.source_mtime, 'size': self.source_size}

    def set_data(self, path, data):
        self.written[path] = bytes(data)
        return path == self.bytecode_path


Frozen_SL, Source_SL = make_abc_subclasses(SourceLoader)
Frozen_SL.util = frozen_util
Source_SL.util = source_util
Frozen_SL.init = frozen_init
Source_SL.init = source_init


class SourceLoaderTestHarness:

    def setUp(self, *, is_package=True, **kwargs):
        self.package = 'pkg'
        if is_package:
            self.path = os.path.join(self.package, '__init__.py')
            self.name = self.package
        else:
            module_name = 'mod'
            self.path = os.path.join(self.package, '.'.join(['mod', 'py']))
            self.name = '.'.join([self.package, module_name])
        self.cached = self.util.cache_from_source(self.path)
        self.loader = self.loader_mock(self.path, **kwargs)

    def verify_module(self, module):
        self.assertEqual(module.__name__, self.name)
        self.assertEqual(module.__file__, self.path)
        self.assertEqual(module.__cached__, self.cached)
        self.assertEqual(module.__package__, self.package)
        self.assertEqual(module.__loader__, self.loader)
        values = module._.split('::')
        self.assertEqual(values[0], self.name)
        self.assertEqual(values[1], self.path)
        self.assertEqual(values[2], self.cached)
        self.assertEqual(values[3], self.package)
        self.assertEqual(values[4], repr(self.loader))

    def verify_code(self, code_object):
        module = types.ModuleType(self.name)
        module.__file__ = self.path
        module.__cached__ = self.cached
        module.__package__ = self.package
        module.__loader__ = self.loader
        module.__path__ = []
        exec(code_object, module.__dict__)
        self.verify_module(module)


class SourceOnlyLoaderTests(SourceLoaderTestHarness):

    """Test importlib.abc.SourceLoader for source-only loading.

    Reload testing is subsumed by the tests for
    importlib.util.module_for_loader.

    """

    def test_get_source(self):
        # Verify the source code is returned as a string.
        # If an OSError is raised by get_data then raise ImportError.
        expected_source = self.loader.source.decode('utf-8')
        self.assertEqual(self.loader.get_source(self.name), expected_source)
        def raise_OSError(path):
            raise OSError
        self.loader.get_data = raise_OSError
        with self.assertRaises(ImportError) as cm:
            self.loader.get_source(self.name)
        self.assertEqual(cm.exception.name, self.name)

    def test_is_package(self):
        # Properly detect when loading a package.
        self.setUp(is_package=False)
        self.assertFalse(self.loader.is_package(self.name))
        self.setUp(is_package=True)
        self.assertTrue(self.loader.is_package(self.name))
        self.assertFalse(self.loader.is_package(self.name + '.__init__'))

    def test_get_code(self):
        # Verify the code object is created.
        code_object = self.loader.get_code(self.name)
        self.verify_code(code_object)

    def test_source_to_code(self):
        # Verify the compiled code object.
        code = self.loader.source_to_code(self.loader.source, self.path)
        self.verify_code(code)

    def test_load_module(self):
        # Loading a module should set __name__, __loader__, __package__,
        # __path__ (for packages), __file__, and __cached__.
        # The module should also be put into sys.modules.
        with util.uncache(self.name):
            module = self.loader.load_module(self.name)
            self.verify_module(module)
            self.assertEqual(module.__path__, [os.path.dirname(self.path)])
            self.assertIn(self.name, sys.modules)

    def test_package_settings(self):
        # __package__ needs to be set, while __path__ is set on if the module
        # is a package.
        # Testing the values for a package are covered by test_load_module.
        self.setUp(is_package=False)
        with util.uncache(self.name):
            module = self.loader.load_module(self.name)
            self.verify_module(module)
            self.assertTrue(not hasattr(module, '__path__'))

    def test_get_source_encoding(self):
        # Source is considered encoded in UTF-8 by default unless otherwise
        # specified by an encoding line.
        source = "_ = 'ü'"
        self.loader.source = source.encode('utf-8')
        returned_source = self.loader.get_source(self.name)
        self.assertEqual(returned_source, source)
        source = "# coding: latin-1\n_ = ü"
        self.loader.source = source.encode('latin-1')
        returned_source = self.loader.get_source(self.name)
        self.assertEqual(returned_source, source)


class Frozen_SourceOnlyLTests(SourceOnlyLoaderTests, unittest.TestCase):
    loader_mock = Frozen_SourceOnlyL
    util = frozen_util

class Source_SourceOnlyLTests(SourceOnlyLoaderTests, unittest.TestCase):
    loader_mock = Source_SourceOnlyL
    util = source_util


@unittest.skipIf(sys.dont_write_bytecode, "sys.dont_write_bytecode is true")
class SourceLoaderBytecodeTests(SourceLoaderTestHarness):

    """Test importlib.abc.SourceLoader's use of bytecode.

    Source-only testing handled by SourceOnlyLoaderTests.

    """

    def verify_code(self, code_object, *, bytecode_written=False):
        super().verify_code(code_object)
        if bytecode_written:
            self.assertIn(self.cached, self.loader.written)
            data = bytearray(self.util.MAGIC_NUMBER)
            data.extend(self.init._w_long(self.loader.source_mtime))
            data.extend(self.init._w_long(self.loader.source_size))
            data.extend(marshal.dumps(code_object))
            self.assertEqual(self.loader.written[self.cached], bytes(data))

    def test_code_with_everything(self):
        # When everything should work.
        code_object = self.loader.get_code(self.name)
        self.verify_code(code_object)

    def test_no_bytecode(self):
        # If no bytecode exists then move on to the source.
        self.loader.bytecode_path = "<does not exist>"
        # Sanity check
        with self.assertRaises(OSError):
            bytecode_path = self.util.cache_from_source(self.path)
            self.loader.get_data(bytecode_path)
        code_object = self.loader.get_code(self.name)
        self.verify_code(code_object, bytecode_written=True)

    def test_code_bad_timestamp(self):
        # Bytecode is only used when the timestamp matches the source EXACTLY.
        for source_mtime in (0, 2):
            assert source_mtime != self.loader.source_mtime
            original = self.loader.source_mtime
            self.loader.source_mtime = source_mtime
            # If bytecode is used then EOFError would be raised by marshal.
            self.loader.bytecode = self.loader.bytecode[8:]
            code_object = self.loader.get_code(self.name)
            self.verify_code(code_object, bytecode_written=True)
            self.loader.source_mtime = original

    def test_code_bad_magic(self):
        # Skip over bytecode with a bad magic number.
        self.setUp(magic=b'0000')
        # If bytecode is used then EOFError would be raised by marshal.
        self.loader.bytecode = self.loader.bytecode[8:]
        code_object = self.loader.get_code(self.name)
        self.verify_code(code_object, bytecode_written=True)

    def test_dont_write_bytecode(self):
        # Bytecode is not written if sys.dont_write_bytecode is true.
        # Can assume it is false already thanks to the skipIf class decorator.
        try:
            sys.dont_write_bytecode = True
            self.loader.bytecode_path = "<does not exist>"
            code_object = self.loader.get_code(self.name)
            self.assertNotIn(self.cached, self.loader.written)
        finally:
            sys.dont_write_bytecode = False

    def test_no_set_data(self):
        # If set_data is not defined, one can still read bytecode.
        self.setUp(magic=b'0000')
        original_set_data = self.loader.__class__.mro()[1].set_data
        try:
            del self.loader.__class__.mro()[1].set_data
            code_object = self.loader.get_code(self.name)
            self.verify_code(code_object)
        finally:
            self.loader.__class__.mro()[1].set_data = original_set_data

    def test_set_data_raises_exceptions(self):
        # Raising NotImplementedError or OSError is okay for set_data.
        def raise_exception(exc):
            def closure(*args, **kwargs):
                raise exc
            return closure

        self.setUp(magic=b'0000')
        self.loader.set_data = raise_exception(NotImplementedError)
        code_object = self.loader.get_code(self.name)
        self.verify_code(code_object)


class Frozen_SLBytecodeTests(SourceLoaderBytecodeTests, unittest.TestCase):
    loader_mock = Frozen_SL
    init = frozen_init
    util = frozen_util

class SourceSLBytecodeTests(SourceLoaderBytecodeTests, unittest.TestCase):
    loader_mock = Source_SL
    init = source_init
    util = source_util


class SourceLoaderGetSourceTests:

    """Tests for importlib.abc.SourceLoader.get_source()."""

    def test_default_encoding(self):
        # Should have no problems with UTF-8 text.
        name = 'mod'
        mock = self.SourceOnlyLoaderMock('mod.file')
        source = 'x = "ü"'
        mock.source = source.encode('utf-8')
        returned_source = mock.get_source(name)
        self.assertEqual(returned_source, source)

    def test_decoded_source(self):
        # Decoding should work.
        name = 'mod'
        mock = self.SourceOnlyLoaderMock("mod.file")
        source = "# coding: Latin-1\nx='ü'"
        assert source.encode('latin-1') != source.encode('utf-8')
        mock.source = source.encode('latin-1')
        returned_source = mock.get_source(name)
        self.assertEqual(returned_source, source)

    def test_universal_newlines(self):
        # PEP 302 says universal newlines should be used.
        name = 'mod'
        mock = self.SourceOnlyLoaderMock('mod.file')
        source = "x = 42\r\ny = -13\r\n"
        mock.source = source.encode('utf-8')
        expect = io.IncrementalNewlineDecoder(None, True).decode(source)
        self.assertEqual(mock.get_source(name), expect)


class Frozen_SourceOnlyLGetSourceTests(SourceLoaderGetSourceTests, unittest.TestCase):
    SourceOnlyLoaderMock = Frozen_SourceOnlyL

class Source_SourceOnlyLGetSourceTests(SourceLoaderGetSourceTests, unittest.TestCase):
    SourceOnlyLoaderMock = Source_SourceOnlyL


class SourceLoaderInitModuleAttrTests:

    """Tests for importlib.abc.SourceLoader.init_module_attrs()."""

    def test_init_module_attrs(self):
        # If __file__ set, __cached__ == importlib.util.cached_from_source(__file__).
        name = 'blah'
        path = 'blah.py'
        loader = self.SourceOnlyLoaderMock(path)
        module = types.ModuleType(name)
        loader.init_module_attrs(module)
        self.assertEqual(module.__loader__, loader)
        self.assertEqual(module.__package__, '')
        self.assertEqual(module.__file__, path)
        self.assertEqual(module.__cached__, self.util.cache_from_source(path))

    def test_no_get_filename(self):
        # No __file__, no __cached__.
        with mock.patch.object(self.SourceOnlyLoaderMock, 'get_filename') as mocked:
            mocked.side_effect = ImportError
            name = 'blah'
            loader = self.SourceOnlyLoaderMock('blah.py')
            module = types.ModuleType(name)
            loader.init_module_attrs(module)
        self.assertFalse(hasattr(module, '__file__'))
        self.assertFalse(hasattr(module, '__cached__'))


class Frozen_SLInitModuleAttrTests(SourceLoaderInitModuleAttrTests, unittest.TestCase):
    SourceOnlyLoaderMock = Frozen_SourceOnlyL
    util = frozen_util

    # Difficult to test under source thanks to cross-module mocking needs.
    @mock.patch('importlib._bootstrap.cache_from_source')
    def test_cache_from_source_NotImplementedError(self, mock_cache_from_source):
        # If importlib.util.cache_from_source() raises NotImplementedError don't set
        # __cached__.
        mock_cache_from_source.side_effect = NotImplementedError
        name = 'blah'
        path = 'blah.py'
        loader = self.SourceOnlyLoaderMock(path)
        module = types.ModuleType(name)
        loader.init_module_attrs(module)
        self.assertEqual(module.__file__, path)
        self.assertFalse(hasattr(module, '__cached__'))


class Source_SLInitModuleAttrTests(SourceLoaderInitModuleAttrTests, unittest.TestCase):
    SourceOnlyLoaderMock = Source_SourceOnlyL
    util = source_util



if __name__ == '__main__':
    unittest.main()
