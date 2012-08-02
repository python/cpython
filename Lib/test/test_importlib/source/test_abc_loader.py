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


class PyLoaderMock(abc.PyLoader):

    # Globals that should be defined for all modules.
    source = (b"_ = '::'.join([__name__, __file__, __package__, "
              b"repr(__loader__)])")

    def __init__(self, data):
        """Take a dict of 'module_name: path' pairings.

        Paths should have no file extension, allowing packages to be denoted by
        ending in '__init__'.

        """
        self.module_paths = data
        self.path_to_module = {val:key for key,val in data.items()}

    def get_data(self, path):
        if path not in self.path_to_module:
            raise IOError
        return self.source

    def is_package(self, name):
        filename = os.path.basename(self.get_filename(name))
        return os.path.splitext(filename)[0] == '__init__'

    def source_path(self, name):
        try:
            return self.module_paths[name]
        except KeyError:
            raise ImportError

    def get_filename(self, name):
        """Silence deprecation warning."""
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            path = super().get_filename(name)
            assert len(w) == 1
            assert issubclass(w[0].category, DeprecationWarning)
            return path

    def module_repr(self):
        return '<module>'


class PyLoaderCompatMock(PyLoaderMock):

    """Mock that matches what is suggested to have a loader that is compatible
    from Python 3.1 onwards."""

    def get_filename(self, fullname):
        try:
            return self.module_paths[fullname]
        except KeyError:
            raise ImportError

    def source_path(self, fullname):
        try:
            return self.get_filename(fullname)
        except ImportError:
            return None


class PyPycLoaderMock(abc.PyPycLoader, PyLoaderMock):

    default_mtime = 1

    def __init__(self, source, bc={}):
        """Initialize mock.

        'bc' is a dict keyed on a module's name. The value is dict with
        possible keys of 'path', 'mtime', 'magic', and 'bc'. Except for 'path',
        each of those keys control if any part of created bytecode is to
        deviate from default values.

        """
        super().__init__(source)
        self.module_bytecode = {}
        self.path_to_bytecode = {}
        self.bytecode_to_path = {}
        for name, data in bc.items():
            self.path_to_bytecode[data['path']] = name
            self.bytecode_to_path[name] = data['path']
            magic = data.get('magic', imp.get_magic())
            mtime = importlib._w_long(data.get('mtime', self.default_mtime))
            source_size = importlib._w_long(len(self.source) & 0xFFFFFFFF)
            if 'bc' in data:
                bc = data['bc']
            else:
                bc = self.compile_bc(name)
            self.module_bytecode[name] = magic + mtime + source_size + bc

    def compile_bc(self, name):
        source_path = self.module_paths.get(name, '<test>') or '<test>'
        code = compile(self.source, source_path, 'exec')
        return marshal.dumps(code)

    def source_mtime(self, name):
        if name in self.module_paths:
            return self.default_mtime
        elif name in self.module_bytecode:
            return None
        else:
            raise ImportError

    def bytecode_path(self, name):
        try:
            return self.bytecode_to_path[name]
        except KeyError:
            if name in self.module_paths:
                return None
            else:
                raise ImportError

    def write_bytecode(self, name, bytecode):
        self.module_bytecode[name] = bytecode
        return True

    def get_data(self, path):
        if path in self.path_to_module:
            return super().get_data(path)
        elif path in self.path_to_bytecode:
            name = self.path_to_bytecode[path]
            return self.module_bytecode[name]
        else:
            raise IOError

    def is_package(self, name):
        try:
            return super().is_package(name)
        except TypeError:
            return '__init__' in self.bytecode_to_path[name]

    def get_code(self, name):
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            code_object = super().get_code(name)
            assert len(w) == 1
            assert issubclass(w[0].category, DeprecationWarning)
            return code_object

class PyLoaderTests(testing_abc.LoaderTests):

    """Tests for importlib.abc.PyLoader."""

    mocker = PyLoaderMock

    def eq_attrs(self, ob, **kwargs):
        for attr, val in kwargs.items():
            found = getattr(ob, attr)
            self.assertEqual(found, val,
                    "{} attribute: {} != {}".format(attr, found, val))

    def test_module(self):
        name = '<module>'
        path = os.path.join('', 'path', 'to', 'module')
        mock = self.mocker({name: path})
        with util.uncache(name):
            module = mock.load_module(name)
            self.assertIn(name, sys.modules)
        self.eq_attrs(module, __name__=name, __file__=path, __package__='',
                        __loader__=mock)
        self.assertTrue(not hasattr(module, '__path__'))
        return mock, name

    def test_package(self):
        name = '<pkg>'
        path = os.path.join('path', 'to', name, '__init__')
        mock = self.mocker({name: path})
        with util.uncache(name):
            module = mock.load_module(name)
            self.assertIn(name, sys.modules)
        self.eq_attrs(module, __name__=name, __file__=path,
                __path__=[os.path.dirname(path)], __package__=name,
                __loader__=mock)
        return mock, name

    def test_lacking_parent(self):
        name = 'pkg.mod'
        path = os.path.join('path', 'to', 'pkg', 'mod')
        mock = self.mocker({name: path})
        with util.uncache(name):
            module = mock.load_module(name)
            self.assertIn(name, sys.modules)
        self.eq_attrs(module, __name__=name, __file__=path, __package__='pkg',
                        __loader__=mock)
        self.assertFalse(hasattr(module, '__path__'))
        return mock, name

    def test_module_reuse(self):
        name = 'mod'
        path = os.path.join('path', 'to', 'mod')
        module = imp.new_module(name)
        mock = self.mocker({name: path})
        with util.uncache(name):
            sys.modules[name] = module
            loaded_module = mock.load_module(name)
            self.assertIs(loaded_module, module)
            self.assertIs(sys.modules[name], module)
        return mock, name

    def test_state_after_failure(self):
        name = "mod"
        module = imp.new_module(name)
        module.blah = None
        mock = self.mocker({name: os.path.join('path', 'to', 'mod')})
        mock.source = b"1/0"
        with util.uncache(name):
            sys.modules[name] = module
            with self.assertRaises(ZeroDivisionError):
                mock.load_module(name)
            self.assertIs(sys.modules[name], module)
            self.assertTrue(hasattr(module, 'blah'))
        return mock

    def test_unloadable(self):
        name = "mod"
        mock = self.mocker({name: os.path.join('path', 'to', 'mod')})
        mock.source = b"1/0"
        with util.uncache(name):
            with self.assertRaises(ZeroDivisionError):
                mock.load_module(name)
            self.assertNotIn(name, sys.modules)
        return mock


class PyLoaderCompatTests(PyLoaderTests):

    """Test that the suggested code to make a loader that is compatible from
    Python 3.1 forward works."""

    mocker = PyLoaderCompatMock


class PyLoaderInterfaceTests(unittest.TestCase):

    """Tests for importlib.abc.PyLoader to make sure that when source_path()
    doesn't return a path everything works as expected."""

    def test_no_source_path(self):
        # No source path should lead to ImportError.
        name = 'mod'
        mock = PyLoaderMock({})
        with util.uncache(name), self.assertRaises(ImportError):
            mock.load_module(name)

    def test_source_path_is_None(self):
        name = 'mod'
        mock = PyLoaderMock({name: None})
        with util.uncache(name), self.assertRaises(ImportError):
            mock.load_module(name)

    def test_get_filename_with_source_path(self):
        # get_filename() should return what source_path() returns.
        name = 'mod'
        path = os.path.join('path', 'to', 'source')
        mock = PyLoaderMock({name: path})
        with util.uncache(name):
            self.assertEqual(mock.get_filename(name), path)

    def test_get_filename_no_source_path(self):
        # get_filename() should raise ImportError if source_path returns None.
        name = 'mod'
        mock = PyLoaderMock({name: None})
        with util.uncache(name), self.assertRaises(ImportError):
            mock.get_filename(name)


class PyPycLoaderTests(PyLoaderTests):

    """Tests for importlib.abc.PyPycLoader."""

    mocker = PyPycLoaderMock

    @source_util.writes_bytecode_files
    def verify_bytecode(self, mock, name):
        assert name in mock.module_paths
        self.assertIn(name, mock.module_bytecode)
        magic = mock.module_bytecode[name][:4]
        self.assertEqual(magic, imp.get_magic())
        mtime = importlib._r_long(mock.module_bytecode[name][4:8])
        self.assertEqual(mtime, 1)
        source_size = mock.module_bytecode[name][8:12]
        self.assertEqual(len(mock.source) & 0xFFFFFFFF,
                         importlib._r_long(source_size))
        bc = mock.module_bytecode[name][12:]
        self.assertEqual(bc, mock.compile_bc(name))

    def test_module(self):
        mock, name = super().test_module()
        self.verify_bytecode(mock, name)

    def test_package(self):
        mock, name = super().test_package()
        self.verify_bytecode(mock, name)

    def test_lacking_parent(self):
        mock, name = super().test_lacking_parent()
        self.verify_bytecode(mock, name)

    def test_module_reuse(self):
        mock, name = super().test_module_reuse()
        self.verify_bytecode(mock, name)

    def test_state_after_failure(self):
        super().test_state_after_failure()

    def test_unloadable(self):
        super().test_unloadable()


class PyPycLoaderInterfaceTests(unittest.TestCase):

    """Test for the interface of importlib.abc.PyPycLoader."""

    def get_filename_check(self, src_path, bc_path, expect):
        name = 'mod'
        mock = PyPycLoaderMock({name: src_path}, {name: {'path': bc_path}})
        with util.uncache(name):
            assert mock.source_path(name) == src_path
            assert mock.bytecode_path(name) == bc_path
            self.assertEqual(mock.get_filename(name), expect)

    def test_filename_with_source_bc(self):
        # When source and bytecode paths present, return the source path.
        self.get_filename_check('source_path', 'bc_path', 'source_path')

    def test_filename_with_source_no_bc(self):
        # With source but no bc, return source path.
        self.get_filename_check('source_path', None, 'source_path')

    def test_filename_with_no_source_bc(self):
        # With not source but bc, return the bc path.
        self.get_filename_check(None, 'bc_path', 'bc_path')

    def test_filename_with_no_source_or_bc(self):
        # With no source or bc, raise ImportError.
        name = 'mod'
        mock = PyPycLoaderMock({name: None}, {name: {'path': None}})
        with util.uncache(name), self.assertRaises(ImportError):
            mock.get_filename(name)


class SkipWritingBytecodeTests(unittest.TestCase):

    """Test that bytecode is properly handled based on
    sys.dont_write_bytecode."""

    @source_util.writes_bytecode_files
    def run_test(self, dont_write_bytecode):
        name = 'mod'
        mock = PyPycLoaderMock({name: os.path.join('path', 'to', 'mod')})
        sys.dont_write_bytecode = dont_write_bytecode
        with util.uncache(name):
            mock.load_module(name)
        self.assertIsNot(name in mock.module_bytecode, dont_write_bytecode)

    def test_no_bytecode_written(self):
        self.run_test(True)

    def test_bytecode_written(self):
        self.run_test(False)


class RegeneratedBytecodeTests(unittest.TestCase):

    """Test that bytecode is regenerated as expected."""

    @source_util.writes_bytecode_files
    def test_different_magic(self):
        # A different magic number should lead to new bytecode.
        name = 'mod'
        bad_magic = b'\x00\x00\x00\x00'
        assert bad_magic != imp.get_magic()
        mock = PyPycLoaderMock({name: os.path.join('path', 'to', 'mod')},
                                {name: {'path': os.path.join('path', 'to',
                                                    'mod.bytecode'),
                                        'magic': bad_magic}})
        with util.uncache(name):
            mock.load_module(name)
        self.assertIn(name, mock.module_bytecode)
        magic = mock.module_bytecode[name][:4]
        self.assertEqual(magic, imp.get_magic())

    @source_util.writes_bytecode_files
    def test_old_mtime(self):
        # Bytecode with an older mtime should be regenerated.
        name = 'mod'
        old_mtime = PyPycLoaderMock.default_mtime - 1
        mock = PyPycLoaderMock({name: os.path.join('path', 'to', 'mod')},
                {name: {'path': 'path/to/mod.bytecode', 'mtime': old_mtime}})
        with util.uncache(name):
            mock.load_module(name)
        self.assertIn(name, mock.module_bytecode)
        mtime = importlib._r_long(mock.module_bytecode[name][4:8])
        self.assertEqual(mtime, PyPycLoaderMock.default_mtime)


class BadBytecodeFailureTests(unittest.TestCase):

    """Test import failures when there is no source and parts of the bytecode
    is bad."""

    def test_bad_magic(self):
        # A bad magic number should lead to an ImportError.
        name = 'mod'
        bad_magic = b'\x00\x00\x00\x00'
        bc = {name:
                {'path': os.path.join('path', 'to', 'mod'),
                 'magic': bad_magic}}
        mock = PyPycLoaderMock({name: None}, bc)
        with util.uncache(name), self.assertRaises(ImportError) as cm:
            mock.load_module(name)
        self.assertEqual(cm.exception.name, name)

    def test_no_bytecode(self):
        # Missing code object bytecode should lead to an EOFError.
        name = 'mod'
        bc = {name: {'path': os.path.join('path', 'to', 'mod'), 'bc': b''}}
        mock = PyPycLoaderMock({name: None}, bc)
        with util.uncache(name), self.assertRaises(EOFError):
            mock.load_module(name)

    def test_bad_bytecode(self):
        # Malformed code object bytecode should lead to a ValueError.
        name = 'mod'
        bc = {name: {'path': os.path.join('path', 'to', 'mod'), 'bc': b'1234'}}
        mock = PyPycLoaderMock({name: None}, bc)
        with util.uncache(name), self.assertRaises(ValueError):
            mock.load_module(name)


def raise_ImportError(*args, **kwargs):
    raise ImportError

class MissingPathsTests(unittest.TestCase):

    """Test what happens when a source or bytecode path does not exist (either
    from *_path returning None or raising ImportError)."""

    def test_source_path_None(self):
        # Bytecode should be used when source_path returns None, along with
        # __file__ being set to the bytecode path.
        name = 'mod'
        bytecode_path = 'path/to/mod'
        mock = PyPycLoaderMock({name: None}, {name: {'path': bytecode_path}})
        with util.uncache(name):
            module = mock.load_module(name)
        self.assertEqual(module.__file__, bytecode_path)

    # Testing for bytecode_path returning None handled by all tests where no
    # bytecode initially exists.

    def test_all_paths_None(self):
        # If all *_path methods return None, raise ImportError.
        name = 'mod'
        mock = PyPycLoaderMock({name: None})
        with util.uncache(name), self.assertRaises(ImportError) as cm:
            mock.load_module(name)
        self.assertEqual(cm.exception.name, name)

    def test_source_path_ImportError(self):
        # An ImportError from source_path should trigger an ImportError.
        name = 'mod'
        mock = PyPycLoaderMock({}, {name: {'path': os.path.join('path', 'to',
                                                                'mod')}})
        with util.uncache(name), self.assertRaises(ImportError):
            mock.load_module(name)

    def test_bytecode_path_ImportError(self):
        # An ImportError from bytecode_path should trigger an ImportError.
        name = 'mod'
        mock = PyPycLoaderMock({name: os.path.join('path', 'to', 'mod')})
        bad_meth = types.MethodType(raise_ImportError, mock)
        mock.bytecode_path = bad_meth
        with util.uncache(name), self.assertRaises(ImportError) as cm:
            mock.load_module(name)


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

    class PyLoader(ResourceLoader, InspectLoader, abc.PyLoader):
        def source_path(self, _):
            super().source_path(_)

    class PyPycLoader(PyLoader, abc.PyPycLoader):
        def bytecode_path(self, _):
            super().bytecode_path(_)

        def source_mtime(self, _):
            super().source_mtime(_)

        def write_bytecode(self, _, _2):
            super().write_bytecode(_, _2)

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
        self.raises_NotImplementedError(ins,'path_stats', 'set_data')

    def test_PyLoader(self):
        self.raises_NotImplementedError(self.PyLoader(), 'source_path',
                                        'get_data', 'is_package')

    def test_PyPycLoader(self):
        self.raises_NotImplementedError(self.PyPycLoader(), 'source_path',
                                        'source_mtime', 'bytecode_path',
                                        'write_bytecode')


def test_main():
    from test.support import run_unittest
    run_unittest(PyLoaderTests, PyLoaderCompatTests,
                    PyLoaderInterfaceTests,
                    PyPycLoaderTests, PyPycLoaderInterfaceTests,
                    SkipWritingBytecodeTests, RegeneratedBytecodeTests,
                    BadBytecodeFailureTests, MissingPathsTests,
                    SourceOnlyLoaderTests,
                    SourceLoaderBytecodeTests,
                    SourceLoaderGetSourceTests,
                    AbstractMethodImplTests)


if __name__ == '__main__':
    test_main()
