import importlib
from importlib import abc
from importlib import machinery

import imp
import inspect
import io
import marshal
import os
import sys
import unittest

from . import util

##### Inheritance
class InheritanceTests:

    """Test that the specified class is a subclass/superclass of the expected
    classes."""

    subclasses = []
    superclasses = []

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        assert self.subclasses or self.superclasses, self.__class__
        self.__test = getattr(abc, self.__class__.__name__)

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


class MetaPathFinder(InheritanceTests, unittest.TestCase):

    superclasses = [abc.Finder]
    subclasses = [machinery.BuiltinImporter, machinery.FrozenImporter,
                    machinery.PathFinder, machinery.WindowsRegistryFinder]


class PathEntryFinder(InheritanceTests, unittest.TestCase):

    superclasses = [abc.Finder]
    subclasses = [machinery.FileFinder]


class ResourceLoader(InheritanceTests, unittest.TestCase):

    superclasses = [abc.Loader]


class InspectLoader(InheritanceTests, unittest.TestCase):

    superclasses = [abc.Loader]
    subclasses = [machinery.BuiltinImporter,
                    machinery.FrozenImporter, machinery.ExtensionFileLoader]


class ExecutionLoader(InheritanceTests, unittest.TestCase):

    superclasses = [abc.InspectLoader]


class FileLoader(InheritanceTests, unittest.TestCase):

    superclasses = [abc.ResourceLoader, abc.ExecutionLoader]
    subclasses = [machinery.SourceFileLoader, machinery.SourcelessFileLoader]


class SourceLoader(InheritanceTests, unittest.TestCase):

    superclasses = [abc.ResourceLoader, abc.ExecutionLoader]
    subclasses = [machinery.SourceFileLoader]


##### Default semantics
class MetaPathFinderSubclass(abc.MetaPathFinder):

    def find_module(self, fullname, path):
        return super().find_module(fullname, path)


class MetaPathFinderDefaultsTests(unittest.TestCase):

    ins = MetaPathFinderSubclass()

    def test_find_module(self):
        # Default should return None.
        self.assertIsNone(self.ins.find_module('something', None))

    def test_invalidate_caches(self):
        # Calling the method is a no-op.
        self.ins.invalidate_caches()


class PathEntryFinderSubclass(abc.PathEntryFinder):

    def find_loader(self, fullname):
        return super().find_loader(fullname)


class PathEntryFinderDefaultsTests(unittest.TestCase):

    ins = PathEntryFinderSubclass()

    def test_find_loader(self):
        self.assertEqual((None, []), self.ins.find_loader('something'))

    def find_module(self):
        self.assertEqual(None, self.ins.find_module('something'))

    def test_invalidate_caches(self):
        # Should be a no-op.
        self.ins.invalidate_caches()


class LoaderSubclass(abc.Loader):

    def load_module(self, fullname):
        return super().load_module(fullname)


class LoaderDefaultsTests(unittest.TestCase):

    ins = LoaderSubclass()

    def test_load_module(self):
        with self.assertRaises(ImportError):
            self.ins.load_module('something')

    def test_module_repr(self):
        mod = imp.new_module('blah')
        with self.assertRaises(NotImplementedError):
            self.ins.module_repr(mod)
        original_repr = repr(mod)
        mod.__loader__ = self.ins
        # Should still return a proper repr.
        self.assertTrue(repr(mod))


class ResourceLoaderSubclass(LoaderSubclass, abc.ResourceLoader):

    def get_data(self, path):
        return super().get_data(path)


class ResourceLoaderDefaultsTests(unittest.TestCase):

    ins = ResourceLoaderSubclass()

    def test_get_data(self):
        with self.assertRaises(IOError):
            self.ins.get_data('/some/path')


class InspectLoaderSubclass(LoaderSubclass, abc.InspectLoader):

    def is_package(self, fullname):
        return super().is_package(fullname)

    def get_code(self, fullname):
        return super().get_code(fullname)

    def get_source(self, fullname):
        return super().get_source(fullname)


class InspectLoaderDefaultsTests(unittest.TestCase):

    ins = InspectLoaderSubclass()

    def test_is_package(self):
        with self.assertRaises(ImportError):
            self.ins.is_package('blah')

    def test_get_code(self):
        with self.assertRaises(ImportError):
            self.ins.get_code('blah')

    def test_get_source(self):
        with self.assertRaises(ImportError):
            self.ins.get_source('blah')


class ExecutionLoaderSubclass(InspectLoaderSubclass, abc.ExecutionLoader):

    def get_filename(self, fullname):
        return super().get_filename(fullname)


class ExecutionLoaderDefaultsTests(unittest.TestCase):

    ins = ExecutionLoaderSubclass()

    def test_get_filename(self):
        with self.assertRaises(ImportError):
            self.ins.get_filename('blah')


##### SourceLoader
class SourceOnlyLoaderMock(abc.SourceLoader):

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


class SourceLoaderMock(SourceOnlyLoaderMock):

    source_mtime = 1

    def __init__(self, path, magic=imp.get_magic()):
        super().__init__(path)
        self.bytecode_path = imp.cache_from_source(self.path)
        self.source_size = len(self.source)
        data = bytearray(magic)
        data.extend(importlib._w_long(self.source_mtime))
        data.extend(importlib._w_long(self.source_size))
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


class SourceLoaderTestHarness(unittest.TestCase):

    def setUp(self, *, is_package=True, **kwargs):
        self.package = 'pkg'
        if is_package:
            self.path = os.path.join(self.package, '__init__.py')
            self.name = self.package
        else:
            module_name = 'mod'
            self.path = os.path.join(self.package, '.'.join(['mod', 'py']))
            self.name = '.'.join([self.package, module_name])
        self.cached = imp.cache_from_source(self.path)
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
        module = imp.new_module(self.name)
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

    loader_mock = SourceOnlyLoaderMock

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


@unittest.skipIf(sys.dont_write_bytecode, "sys.dont_write_bytecode is true")
class SourceLoaderBytecodeTests(SourceLoaderTestHarness):

    """Test importlib.abc.SourceLoader's use of bytecode.

    Source-only testing handled by SourceOnlyLoaderTests.

    """

    loader_mock = SourceLoaderMock

    def verify_code(self, code_object, *, bytecode_written=False):
        super().verify_code(code_object)
        if bytecode_written:
            self.assertIn(self.cached, self.loader.written)
            data = bytearray(imp.get_magic())
            data.extend(importlib._w_long(self.loader.source_mtime))
            data.extend(importlib._w_long(self.loader.source_size))
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
            bytecode_path = imp.cache_from_source(self.path)
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
        original_set_data = self.loader.__class__.set_data
        try:
            del self.loader.__class__.set_data
            code_object = self.loader.get_code(self.name)
            self.verify_code(code_object)
        finally:
            self.loader.__class__.set_data = original_set_data

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


class SourceLoaderGetSourceTests(unittest.TestCase):

    """Tests for importlib.abc.SourceLoader.get_source()."""

    def test_default_encoding(self):
        # Should have no problems with UTF-8 text.
        name = 'mod'
        mock = SourceOnlyLoaderMock('mod.file')
        source = 'x = "ü"'
        mock.source = source.encode('utf-8')
        returned_source = mock.get_source(name)
        self.assertEqual(returned_source, source)

    def test_decoded_source(self):
        # Decoding should work.
        name = 'mod'
        mock = SourceOnlyLoaderMock("mod.file")
        source = "# coding: Latin-1\nx='ü'"
        assert source.encode('latin-1') != source.encode('utf-8')
        mock.source = source.encode('latin-1')
        returned_source = mock.get_source(name)
        self.assertEqual(returned_source, source)

    def test_universal_newlines(self):
        # PEP 302 says universal newlines should be used.
        name = 'mod'
        mock = SourceOnlyLoaderMock('mod.file')
        source = "x = 42\r\ny = -13\r\n"
        mock.source = source.encode('utf-8')
        expect = io.IncrementalNewlineDecoder(None, True).decode(source)
        self.assertEqual(mock.get_source(name), expect)


if __name__ == '__main__':
    unittest.main()
