import importlib
from importlib import abc

from .. import abc as testing_abc
from .. import util
from . import util as source_util

import imp
import inspect
import io
import marshal
import os
import sys
import types
import unittest
import warnings


class SourceOnlyLoaderMock(abc.SourceLoader):

    # Globals that should be defined for all modules.
    source = (b"_ = '::'.join([__name__, __file__, __cached__, __package__, "
              b"repr(__loader__)])")

    def __init__(self, path):
        self.path = path

    def get_data(self, path):
        assert self.path == path
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
            raise IOError

    def path_stats(self, path):
        assert path == self.path
        return {'mtime': self.source_mtime, 'size': self.source_size}

    def set_data(self, path, data):
        self.written[path] = bytes(data)
        return path == self.bytecode_path


def raise_ImportError(*args, **kwargs):
    raise ImportError


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
        # If an IOError is raised by get_data then raise ImportError.
        expected_source = self.loader.source.decode('utf-8')
        self.assertEqual(self.loader.get_source(self.name), expected_source)
        def raise_IOError(path):
            raise IOError
        self.loader.get_data = raise_IOError
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
        with self.assertRaises(IOError):
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
        # Raising NotImplementedError or IOError is okay for set_data.
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


class AbstractMethodImplTests(unittest.TestCase):

    """Test the concrete abstractmethod implementations."""

    class MetaPathFinder(abc.MetaPathFinder):
        def find_module(self, fullname, path):
            super().find_module(fullname, path)

    class PathEntryFinder(abc.PathEntryFinder):
        def find_module(self, _):
            super().find_module(_)

        def find_loader(self, _):
            super().find_loader(_)

    class Finder(abc.Finder):
        def find_module(self, fullname, path):
            super().find_module(fullname, path)

    class Loader(abc.Loader):
        def load_module(self, fullname):
            super().load_module(fullname)

        def module_repr(self, module):
            super().module_repr(module)

    class ResourceLoader(Loader, abc.ResourceLoader):
        def get_data(self, _):
            super().get_data(_)

    class InspectLoader(Loader, abc.InspectLoader):
        def is_package(self, _):
            super().is_package(_)

        def get_code(self, _):
            super().get_code(_)

        def get_source(self, _):
            super().get_source(_)

    class ExecutionLoader(InspectLoader, abc.ExecutionLoader):
        def get_filename(self, _):
            super().get_filename(_)

    class SourceLoader(ResourceLoader, ExecutionLoader, abc.SourceLoader):
        pass

    def raises_NotImplementedError(self, ins, *args):
        for method_name in args:
            method = getattr(ins, method_name)
            arg_count = len(inspect.getfullargspec(method)[0]) - 1
            args = [''] * arg_count
            try:
                method(*args)
            except NotImplementedError:
                pass
            else:
                msg = "{}.{} did not raise NotImplementedError"
                self.fail(msg.format(ins.__class__.__name__, method_name))

    def test_Loader(self):
        self.raises_NotImplementedError(self.Loader(), 'load_module')

    # XXX misplaced; should be somewhere else
    def test_Finder(self):
        self.raises_NotImplementedError(self.Finder(), 'find_module')

    def test_ResourceLoader(self):
        self.raises_NotImplementedError(self.ResourceLoader(), 'load_module',
                                        'get_data')

    def test_InspectLoader(self):
        self.raises_NotImplementedError(self.InspectLoader(), 'load_module',
                                        'is_package', 'get_code', 'get_source')

    def test_ExecutionLoader(self):
        self.raises_NotImplementedError(self.ExecutionLoader(), 'load_module',
                                        'is_package', 'get_code', 'get_source',
                                        'get_filename')

    def test_SourceLoader(self):
        ins = self.SourceLoader()
        # Required abstractmethods.
        self.raises_NotImplementedError(ins, 'get_filename', 'get_data')
        # Optional abstractmethods.
        self.raises_NotImplementedError(ins, 'path_stats', 'set_data')


def test_main():
    from test.support import run_unittest
    run_unittest(SkipWritingBytecodeTests, RegeneratedBytecodeTests,
                    BadBytecodeFailureTests, MissingPathsTests,
                    SourceOnlyLoaderTests,
                    SourceLoaderBytecodeTests,
                    SourceLoaderGetSourceTests,
                    AbstractMethodImplTests)


if __name__ == '__main__':
    test_main()
