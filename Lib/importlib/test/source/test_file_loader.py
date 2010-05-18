import importlib
from importlib import _bootstrap
from .. import abc
from . import util as source_util

import imp
import os
import py_compile
import stat
import sys
import unittest

from test.support import make_legacy_pyc


class SimpleTest(unittest.TestCase):

    """Should have no issue importing a source module [basic]. And if there is
    a syntax error, it should raise a SyntaxError [syntax error].

    """

    # [basic]
    def test_module(self):
        with source_util.create_modules('_temp') as mapping:
            loader = _bootstrap._PyPycFileLoader('_temp', mapping['_temp'],
                                                    False)
            module = loader.load_module('_temp')
            self.assertTrue('_temp' in sys.modules)
            check = {'__name__': '_temp', '__file__': mapping['_temp'],
                     '__package__': ''}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)

    def test_package(self):
        with source_util.create_modules('_pkg.__init__') as mapping:
            loader = _bootstrap._PyPycFileLoader('_pkg',
                                                 mapping['_pkg.__init__'],
                                                 True)
            module = loader.load_module('_pkg')
            self.assertTrue('_pkg' in sys.modules)
            check = {'__name__': '_pkg', '__file__': mapping['_pkg.__init__'],
                     '__path__': [os.path.dirname(mapping['_pkg.__init__'])],
                     '__package__': '_pkg'}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)


    def test_lacking_parent(self):
        with source_util.create_modules('_pkg.__init__', '_pkg.mod')as mapping:
            loader = _bootstrap._PyPycFileLoader('_pkg.mod',
                                                    mapping['_pkg.mod'], False)
            module = loader.load_module('_pkg.mod')
            self.assertTrue('_pkg.mod' in sys.modules)
            check = {'__name__': '_pkg.mod', '__file__': mapping['_pkg.mod'],
                     '__package__': '_pkg'}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)

    def fake_mtime(self, fxn):
        """Fake mtime to always be higher than expected."""
        return lambda name: fxn(name) + 1

    def test_module_reuse(self):
        with source_util.create_modules('_temp') as mapping:
            loader = _bootstrap._PyPycFileLoader('_temp', mapping['_temp'],
                                                    False)
            module = loader.load_module('_temp')
            module_id = id(module)
            module_dict_id = id(module.__dict__)
            with open(mapping['_temp'], 'w') as file:
                file.write("testing_var = 42\n")
            # For filesystems where the mtime is only to a second granularity,
            # everything that has happened above can be too fast;
            # force an mtime on the source that is guaranteed to be different
            # than the original mtime.
            loader.source_mtime = self.fake_mtime(loader.source_mtime)
            module = loader.load_module('_temp')
            self.assertTrue('testing_var' in module.__dict__,
                         "'testing_var' not in "
                            "{0}".format(list(module.__dict__.keys())))
            self.assertEqual(module, sys.modules['_temp'])
            self.assertEqual(id(module), module_id)
            self.assertEqual(id(module.__dict__), module_dict_id)

    def test_state_after_failure(self):
        # A failed reload should leave the original module intact.
        attributes = ('__file__', '__path__', '__package__')
        value = '<test>'
        name = '_temp'
        with source_util.create_modules(name) as mapping:
            orig_module = imp.new_module(name)
            for attr in attributes:
                setattr(orig_module, attr, value)
            with open(mapping[name], 'w') as file:
                file.write('+++ bad syntax +++')
            loader = _bootstrap._PyPycFileLoader('_temp', mapping['_temp'],
                                                    False)
            with self.assertRaises(SyntaxError):
                loader.load_module(name)
            for attr in attributes:
                self.assertEqual(getattr(orig_module, attr), value)

    # [syntax error]
    def test_bad_syntax(self):
        with source_util.create_modules('_temp') as mapping:
            with open(mapping['_temp'], 'w') as file:
                file.write('=')
            loader = _bootstrap._PyPycFileLoader('_temp', mapping['_temp'],
                                                    False)
            with self.assertRaises(SyntaxError):
                loader.load_module('_temp')
            self.assertTrue('_temp' not in sys.modules)


class BadBytecodeTest(unittest.TestCase):

    def import_(self, file, module_name):
        loader = _bootstrap._PyPycFileLoader(module_name, file, False)
        module = loader.load_module(module_name)
        self.assertTrue(module_name in sys.modules)

    def manipulate_bytecode(self, name, mapping, manipulator, *,
                            del_source=False):
        """Manipulate the bytecode of a module by passing it into a callable
        that returns what to use as the new bytecode."""
        try:
            del sys.modules['_temp']
        except KeyError:
            pass
        py_compile.compile(mapping[name])
        bytecode_path = imp.cache_from_source(mapping[name])
        with open(bytecode_path, 'rb') as file:
            bc = file.read()
        new_bc = manipulator(bc)
        with open(bytecode_path, 'wb') as file:
            if new_bc:
                file.write(new_bc)
        if del_source:
            os.unlink(mapping[name])
            make_legacy_pyc(mapping[name])
        return bytecode_path

    @source_util.writes_bytecode_files
    def test_empty_file(self):
        # When a .pyc is empty, regenerate it if possible, else raise
        # ImportError.
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: None)
            self.import_(mapping['_temp'], '_temp')
            with open(bc_path, 'rb') as file:
                self.assertGreater(len(file.read()), 8)
            self.manipulate_bytecode('_temp', mapping, lambda bc: None,
                                        del_source=True)
            with self.assertRaises(ImportError):
                self.import_(mapping['_temp'], '_temp')

    @source_util.writes_bytecode_files
    def test_partial_magic(self):
        # When their are less than 4 bytes to a .pyc, regenerate it if
        # possible, else raise ImportError.
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:3])
            self.import_(mapping['_temp'], '_temp')
            with open(bc_path, 'rb') as file:
                self.assertGreater(len(file.read()), 8)
            self.manipulate_bytecode('_temp', mapping, lambda bc: bc[:3],
                                        del_source=True)
            with self.assertRaises(ImportError):
                self.import_(mapping['_temp'], '_temp')

    @source_util.writes_bytecode_files
    def test_magic_only(self):
        # When there is only the magic number, regenerate the .pyc if possible,
        # else raise EOFError.
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:4])
            self.import_(mapping['_temp'], '_temp')
            with open(bc_path, 'rb') as file:
                self.assertGreater(len(file.read()), 8)
            self.manipulate_bytecode('_temp', mapping, lambda bc: bc[:4],
                                        del_source=True)
            with self.assertRaises(EOFError):
                self.import_(mapping['_temp'], '_temp')

    @source_util.writes_bytecode_files
    def test_partial_timestamp(self):
        # When the timestamp is partial, regenerate the .pyc, else
        # raise EOFError.
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:7])
            self.import_(mapping['_temp'], '_temp')
            with open(bc_path, 'rb') as file:
                self.assertGreater(len(file.read()), 8)
            self.manipulate_bytecode('_temp', mapping, lambda bc: bc[:7],
                                        del_source=True)
            with self.assertRaises(EOFError):
                self.import_(mapping['_temp'], '_temp')

    @source_util.writes_bytecode_files
    def test_no_marshal(self):
        # When there is only the magic number and timestamp, raise EOFError.
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:8])
            with self.assertRaises(EOFError):
                self.import_(mapping['_temp'], '_temp')

    @source_util.writes_bytecode_files
    def test_bad_magic(self):
        # When the magic number is different, the bytecode should be
        # regenerated.
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                    lambda bc: b'\x00\x00\x00\x00' + bc[4:])
            self.import_(mapping['_temp'], '_temp')
            with open(bc_path, 'rb') as bytecode_file:
                self.assertEqual(bytecode_file.read(4), imp.get_magic())

    # [bad timestamp]
    @source_util.writes_bytecode_files
    def test_bad_bytecode(self):
        # When the timestamp is older than the source, bytecode should be
        # regenerated.
        zeros = b'\x00\x00\x00\x00'
        with source_util.create_modules('_temp') as mapping:
            py_compile.compile(mapping['_temp'])
            bytecode_path = imp.cache_from_source(mapping['_temp'])
            with open(bytecode_path, 'r+b') as bytecode_file:
                bytecode_file.seek(4)
                bytecode_file.write(zeros)
            self.import_(mapping['_temp'], '_temp')
            source_mtime = os.path.getmtime(mapping['_temp'])
            source_timestamp = importlib._w_long(source_mtime)
            with open(bytecode_path, 'rb') as bytecode_file:
                bytecode_file.seek(4)
                self.assertEqual(bytecode_file.read(4), source_timestamp)

    # [bad marshal]
    @source_util.writes_bytecode_files
    def test_bad_marshal(self):
        # Bad marshal data should raise a ValueError.
        with source_util.create_modules('_temp') as mapping:
            bytecode_path = imp.cache_from_source(mapping['_temp'])
            source_mtime = os.path.getmtime(mapping['_temp'])
            source_timestamp = importlib._w_long(source_mtime)
            source_util.ensure_bytecode_path(bytecode_path)
            with open(bytecode_path, 'wb') as bytecode_file:
                bytecode_file.write(imp.get_magic())
                bytecode_file.write(source_timestamp)
                bytecode_file.write(b'AAAA')
            with self.assertRaises(ValueError):
                self.import_(mapping['_temp'], '_temp')
            self.assertTrue('_temp' not in sys.modules)

    # [bytecode read-only]
    @source_util.writes_bytecode_files
    def test_read_only_bytecode(self):
        # When bytecode is read-only but should be rewritten, fail silently.
        with source_util.create_modules('_temp') as mapping:
            # Create bytecode that will need to be re-created.
            py_compile.compile(mapping['_temp'])
            bytecode_path = imp.cache_from_source(mapping['_temp'])
            with open(bytecode_path, 'r+b') as bytecode_file:
                bytecode_file.seek(0)
                bytecode_file.write(b'\x00\x00\x00\x00')
            # Make the bytecode read-only.
            os.chmod(bytecode_path,
                        stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
            try:
                # Should not raise IOError!
                self.import_(mapping['_temp'], '_temp')
            finally:
                # Make writable for eventual clean-up.
                os.chmod(bytecode_path, stat.S_IWUSR)


def test_main():
    from test.support import run_unittest
    run_unittest(SimpleTest, BadBytecodeTest)


if __name__ == '__main__':
    test_main()
