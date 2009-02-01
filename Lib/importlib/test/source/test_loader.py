import importlib
from .. import abc
from . import util as source_util

import imp
import os
import py_compile
import sys
import unittest


class SimpleTest(unittest.TestCase):

    """Should have no issue importing a source module [basic]. And if there is
    a syntax error, it should raise a SyntaxError [syntax error].

    """

    # [basic]
    def test_module(self):
        with source_util.create_modules('_temp') as mapping:
            loader = importlib._PyFileLoader('_temp', mapping['_temp'], False)
            module = loader.load_module('_temp')
            self.assert_('_temp' in sys.modules)
            check = {'__name__': '_temp', '__file__': mapping['_temp'],
                     '__package__': None}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)

    def test_package(self):
        with source_util.create_modules('_pkg.__init__') as mapping:
            loader = importlib._PyFileLoader('_pkg', mapping['_pkg.__init__'],
                                             True)
            module = loader.load_module('_pkg')
            self.assert_('_pkg' in sys.modules)
            check = {'__name__': '_pkg', '__file__': mapping['_pkg.__init__'],
                     '__path__': [os.path.dirname(mapping['_pkg.__init__'])],
                     '__package__': '_pkg'}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)


    def test_lacking_parent(self):
        with source_util.create_modules('_pkg.__init__', '_pkg.mod')as mapping:
            loader = importlib._PyFileLoader('_pkg.mod', mapping['_pkg.mod'],
                                             False)
            module = loader.load_module('_pkg.mod')
            self.assert_('_pkg.mod' in sys.modules)
            check = {'__name__': '_pkg.mod', '__file__': mapping['_pkg.mod'],
                     '__package__': '_pkg'}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)

    def fake_mtime(self, fxn):
        """Fake mtime to always be higher than expected."""
        return lambda name: fxn(name) + 1

    def test_module_reuse(self):
        with source_util.create_modules('_temp') as mapping:
            loader = importlib._PyFileLoader('_temp', mapping['_temp'], False)
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
            self.assert_('testing_var' in module.__dict__,
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
            loader = importlib._PyFileLoader('_temp', mapping['_temp'], False)
            self.assertRaises(SyntaxError, loader.load_module, name)
            for attr in attributes:
                self.assertEqual(getattr(orig_module, attr), value)

    # [syntax error]
    def test_bad_syntax(self):
        with source_util.create_modules('_temp') as mapping:
            with open(mapping['_temp'], 'w') as file:
                file.write('=')
            loader = importlib._PyFileLoader('_temp', mapping['_temp'], False)
            self.assertRaises(SyntaxError, loader.load_module, '_temp')
            self.assert_('_temp' not in sys.modules)


class DontWriteBytecodeTest(unittest.TestCase):

    """If sys.dont_write_bytcode is true then no bytecode should be created."""

    def tearDown(self):
        sys.dont_write_bytecode = False

    @source_util.writes_bytecode
    def run_test(self, assertion):
        with source_util.create_modules('_temp') as mapping:
            loader = importlib._PyFileLoader('_temp', mapping['_temp'], False)
            loader.load_module('_temp')
            bytecode_path = source_util.bytecode_path(mapping['_temp'])
            assertion(bytecode_path)

    def test_bytecode_written(self):
        fxn = lambda bc_path: self.assert_(os.path.exists(bc_path))
        self.run_test(fxn)

    def test_bytecode_not_written(self):
        sys.dont_write_bytecode = True
        fxn = lambda bc_path: self.assert_(not os.path.exists(bc_path))
        self.run_test(fxn)


class BadDataTest(unittest.TestCase):

    """If the bytecode has a magic number that does not match the
    interpreters', ImportError is raised [bad magic]. The timestamp can have
    any value. And bad marshal data raises ValueError.

    """

    # [bad magic]
    def test_bad_magic(self):
        with source_util.create_modules('_temp') as mapping:
            py_compile.compile(mapping['_temp'])
            os.unlink(mapping['_temp'])
            bytecode_path = source_util.bytecode_path(mapping['_temp'])
            with open(bytecode_path, 'r+b') as file:
                file.seek(0)
                file.write(b'\x00\x00\x00\x00')
            loader = importlib._PyFileLoader('_temp', mapping['_temp'], False)
            self.assertRaises(ImportError, loader.load_module, '_temp')
            self.assert_('_temp' not in sys.modules)


class SourceBytecodeInteraction(unittest.TestCase):

    """When both source and bytecode are present, certain rules dictate which
    version of the code takes precedent. All things being equal, the bytecode
    is used with the value of __file__ set to the source [basic top-level],
    [basic package], [basic sub-module], [basic sub-package].

    """

    def import_(self, file, module, *, pkg=False):
        loader = importlib._PyFileLoader(module, file, pkg)
        return loader.load_module(module)

    def run_test(self, test, *create, pkg=False):
        create += (test,)
        with source_util.create_modules(*create) as mapping:
            for name in create:
                py_compile.compile(mapping[name])
            if pkg:
                import_name = test.rsplit('.', 1)[0]
            else:
                import_name = test
            loader = importlib._PyFileLoader(import_name, mapping[test], pkg)
            # Because some platforms only have a granularity to the second for
            # atime you can't check the physical files. Instead just make it an
            # exception trigger if source was read.
            loader.get_source = lambda self, x: 42
            module = loader.load_module(import_name)
            self.assertEqual(module.__file__, mapping[name])
            self.assert_(import_name in sys.modules)
            self.assertEqual(id(module), id(sys.modules[import_name]))

    # [basic top-level]
    def test_basic_top_level(self):
        self.run_test('top_level')

    # [basic package]
    def test_basic_package(self):
        self.run_test('pkg.__init__', pkg=True)

    # [basic sub-module]
    def test_basic_sub_module(self):
        self.run_test('pkg.sub', 'pkg.__init__')

    # [basic sub-package]
    def test_basic_sub_package(self):
        self.run_test('pkg.sub.__init__', 'pkg.__init__', pkg=True)


class BadBytecodeTest(unittest.TestCase):

    """But there are several things about the bytecode which might lead to the
    source being preferred. If the magic number differs from what the
    interpreter uses, then the source is used with the bytecode regenerated.
    If the timestamp is older than the modification time for the source then
    the bytecode is not used [bad timestamp].

    But if the marshal data is bad, even if the magic number and timestamp
    work, a ValueError is raised and the source is not used [bad marshal].

    """

    def import_(self, file, module_name):
        loader = importlib._PyFileLoader(module_name, file, False)
        module = loader.load_module(module_name)
        self.assert_(module_name in sys.modules)

    # [bad magic]
    @source_util.writes_bytecode
    def test_bad_magic(self):
        with source_util.create_modules('_temp') as mapping:
            py_compile.compile(mapping['_temp'])
            bytecode_path = source_util.bytecode_path(mapping['_temp'])
            with open(bytecode_path, 'r+b') as bytecode_file:
                bytecode_file.seek(0)
                bytecode_file.write(b'\x00\x00\x00\x00')
            self.import_(mapping['_temp'], '_temp')
            with open(bytecode_path, 'rb') as bytecode_file:
                self.assertEqual(bytecode_file.read(4), imp.get_magic())

    # [bad timestamp]
    @source_util.writes_bytecode
    def test_bad_bytecode(self):
        zeros = b'\x00\x00\x00\x00'
        with source_util.create_modules('_temp') as mapping:
            py_compile.compile(mapping['_temp'])
            bytecode_path = source_util.bytecode_path(mapping['_temp'])
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
    def test_bad_marshal(self):
        with source_util.create_modules('_temp') as mapping:
            bytecode_path = source_util.bytecode_path(mapping['_temp'])
            source_mtime = os.path.getmtime(mapping['_temp'])
            source_timestamp = importlib._w_long(source_mtime)
            with open(bytecode_path, 'wb') as bytecode_file:
                bytecode_file.write(imp.get_magic())
                bytecode_file.write(source_timestamp)
                bytecode_file.write(b'AAAA')
            self.assertRaises(ValueError, self.import_, mapping['_temp'],
                                '_temp')
            self.assert_('_temp' not in sys.modules)


def test_main():
    from test.support import run_unittest
    run_unittest(SimpleTest, DontWriteBytecodeTest, BadDataTest,
                 SourceBytecodeInteraction, BadBytecodeTest)


if __name__ == '__main__':
    test_main()
