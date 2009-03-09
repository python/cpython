import importlib
from importlib import abc
from .. import abc as testing_abc
from .. import util
from . import util as source_util
import imp
import marshal
import os
import sys
import types
import unittest


class PyLoaderMock(abc.PyLoader):

    # Globals that should be defined for all modules.
    source = ("_ = '::'.join([__name__, __file__, __package__, "
              "repr(__loader__)])")

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
        return self.source.encode('utf-8')

    def is_package(self, name):
        try:
            return '__init__' in self.module_paths[name]
        except KeyError:
            raise ImportError

    def get_source(self, name):  # Should not be needed.
        raise NotImplementedError

    def source_path(self, name):
        try:
            return self.module_paths[name]
        except KeyError:
            raise ImportError


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
            if 'bc' in data:
                bc = data['bc']
            else:
                bc = self.compile_bc(name)
            self.module_bytecode[name] = magic + mtime + bc

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


class PyLoaderTests(testing_abc.LoaderTests):

    """Tests for importlib.abc.PyLoader."""

    mocker = PyLoaderMock

    def eq_attrs(self, ob, **kwargs):
        for attr, val in kwargs.items():
            self.assertEqual(getattr(ob, attr), val)

    def test_module(self):
        name = '<module>'
        path = 'path/to/module'
        mock = self.mocker({name: path})
        with util.uncache(name):
            module = mock.load_module(name)
            self.assert_(name in sys.modules)
        self.eq_attrs(module, __name__=name, __file__=path, __package__='',
                        __loader__=mock)
        self.assert_(not hasattr(module, '__path__'))
        return mock, name

    def test_package(self):
        name = '<pkg>'
        path = '/path/to/<pkg>/__init__'
        mock = self.mocker({name: path})
        with util.uncache(name):
            module = mock.load_module(name)
            self.assert_(name in sys.modules)
        self.eq_attrs(module, __name__=name, __file__=path,
                __path__=[os.path.dirname(path)], __package__=name,
                __loader__=mock)
        return mock, name

    def test_lacking_parent(self):
        name = 'pkg.mod'
        path = 'path/to/pkg/mod'
        mock = self.mocker({name: path})
        with util.uncache(name):
            module = mock.load_module(name)
            self.assert_(name in sys.modules)
        self.eq_attrs(module, __name__=name, __file__=path, __package__='pkg',
                        __loader__=mock)
        self.assert_(not hasattr(module, '__path__'))
        return mock, name

    def test_module_reuse(self):
        name = 'mod'
        path = 'path/to/mod'
        module = imp.new_module(name)
        mock = self.mocker({name: path})
        with util.uncache(name):
            sys.modules[name] = module
            loaded_module = mock.load_module(name)
            self.assert_(loaded_module is module)
            self.assert_(sys.modules[name] is module)
        return mock, name

    def test_state_after_failure(self):
        name = "mod"
        module = imp.new_module(name)
        module.blah = None
        mock = self.mocker({name: 'path/to/mod'})
        mock.source = "1/0"
        with util.uncache(name):
            sys.modules[name] = module
            self.assertRaises(ZeroDivisionError, mock.load_module, name)
            self.assert_(sys.modules[name] is module)
            self.assert_(hasattr(module, 'blah'))
        return mock

    def test_unloadable(self):
        name = "mod"
        mock = self.mocker({name: 'path/to/mod'})
        mock.source = "1/0"
        with util.uncache(name):
            self.assertRaises(ZeroDivisionError, mock.load_module, name)
            self.assert_(name not in sys.modules)
        return mock


class PyLoaderInterfaceTests(unittest.TestCase):


    def test_no_source_path(self):
        # No source path should lead to ImportError.
        name = 'mod'
        mock = PyLoaderMock({})
        with util.uncache(name):
            self.assertRaises(ImportError, mock.load_module, name)

    def test_source_path_is_None(self):
        name = 'mod'
        mock = PyLoaderMock({name: None})
        with util.uncache(name):
            self.assertRaises(ImportError, mock.load_module, name)


class PyPycLoaderTests(PyLoaderTests):

    """Tests for importlib.abc.PyPycLoader."""

    mocker = PyPycLoaderMock

    @source_util.writes_bytecode
    def verify_bytecode(self, mock, name):
        assert name in mock.module_paths
        self.assert_(name in mock.module_bytecode)
        magic = mock.module_bytecode[name][:4]
        self.assertEqual(magic, imp.get_magic())
        mtime = importlib._r_long(mock.module_bytecode[name][4:8])
        self.assertEqual(mtime, 1)
        bc = mock.module_bytecode[name][8:]


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


class SkipWritingBytecodeTests(unittest.TestCase):

    """Test that bytecode is properly handled based on
    sys.dont_write_bytecode."""

    @source_util.writes_bytecode
    def run_test(self, dont_write_bytecode):
        name = 'mod'
        mock = PyPycLoaderMock({name: 'path/to/mod'})
        sys.dont_write_bytecode = dont_write_bytecode
        with util.uncache(name):
            mock.load_module(name)
        self.assert_((name in mock.module_bytecode) is not
                        dont_write_bytecode)

    def test_no_bytecode_written(self):
        self.run_test(True)

    def test_bytecode_written(self):
        self.run_test(False)


class RegeneratedBytecodeTests(unittest.TestCase):

    """Test that bytecode is regenerated as expected."""

    @source_util.writes_bytecode
    def test_different_magic(self):
        # A different magic number should lead to new bytecode.
        name = 'mod'
        bad_magic = b'\x00\x00\x00\x00'
        assert bad_magic != imp.get_magic()
        mock = PyPycLoaderMock({name: 'path/to/mod'},
                                {name: {'path': 'path/to/mod.bytecode',
                                        'magic': bad_magic}})
        with util.uncache(name):
            mock.load_module(name)
        self.assert_(name in mock.module_bytecode)
        magic = mock.module_bytecode[name][:4]
        self.assertEqual(magic, imp.get_magic())

    @source_util.writes_bytecode
    def test_old_mtime(self):
        # Bytecode with an older mtime should be regenerated.
        name = 'mod'
        old_mtime = PyPycLoaderMock.default_mtime - 1
        mock = PyPycLoaderMock({name: 'path/to/mod'},
                {name: {'path': 'path/to/mod.bytecode', 'mtime': old_mtime}})
        with util.uncache(name):
            mock.load_module(name)
        self.assert_(name in mock.module_bytecode)
        mtime = importlib._r_long(mock.module_bytecode[name][4:8])
        self.assertEqual(mtime, PyPycLoaderMock.default_mtime)


class BadBytecodeFailureTests(unittest.TestCase):

    """Test import failures when there is no source and parts of the bytecode
    is bad."""

    def test_bad_magic(self):
        # A bad magic number should lead to an ImportError.
        name = 'mod'
        bad_magic = b'\x00\x00\x00\x00'
        mock = PyPycLoaderMock({}, {name: {'path': 'path/to/mod',
                                            'magic': bad_magic}})
        with util.uncache(name):
            self.assertRaises(ImportError, mock.load_module, name)

    def test_bad_bytecode(self):
        # Bad code object bytecode should elad to an ImportError.
        name = 'mod'
        mock = PyPycLoaderMock({}, {name: {'path': '/path/to/mod', 'bc': b''}})
        with util.uncache(name):
            self.assertRaises(ImportError, mock.load_module, name)


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
        with util.uncache(name):
            self.assertRaises(ImportError, mock.load_module, name)

    def test_source_path_ImportError(self):
        # An ImportError from source_path should trigger an ImportError.
        name = 'mod'
        mock = PyPycLoaderMock({}, {name: {'path': 'path/to/mod'}})
        with util.uncache(name):
            self.assertRaises(ImportError, mock.load_module, name)

    def test_bytecode_path_ImportError(self):
        # An ImportError from bytecode_path should trigger an ImportError.
        name = 'mod'
        mock = PyPycLoaderMock({name: 'path/to/mod'})
        bad_meth = types.MethodType(raise_ImportError, mock)
        mock.bytecode_path = bad_meth
        with util.uncache(name):
            self.assertRaises(ImportError, mock.load_module, name)


def test_main():
    from test.support import run_unittest
    run_unittest(PyLoaderTests, PyLoaderInterfaceTests,
                    PyPycLoaderTests, SkipWritingBytecodeTests,
                    RegeneratedBytecodeTests, BadBytecodeFailureTests,
                    MissingPathsTests)


if __name__ == '__main__':
    test_main()
