import importlib
from .. import support

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
    def test_basic(self):
        with support.create_modules('_temp') as mapping:
            loader = importlib._PyFileLoader('_temp', mapping['_temp'], False)
            loader.load_module('_temp')
            self.assert_('_temp' in sys.modules)

    # [syntax error]
    def test_bad_syntax(self):
        with support.create_modules('_temp') as mapping:
            with open(mapping['_temp'], 'w') as file:
                file.write('=')
            loader = importlib._PyFileLoader('_temp', mapping['_temp'], False)
            self.assertRaises(SyntaxError, loader.load_module, '_temp')
            self.assert_('_temp' not in sys.modules)


class DontWriteBytecodeTest(unittest.TestCase):

    """If sys.dont_write_bytcode is true then no bytecode should be created."""

    def tearDown(self):
        sys.dont_write_bytecode = False

    @support.writes_bytecode
    def run_test(self, assertion):
        with support.create_modules('_temp') as mapping:
            loader = importlib._PyFileLoader('_temp', mapping['_temp'], False)
            loader.load_module('_temp')
            bytecode_path = support.bytecode_path(mapping['_temp'])
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
        with support.create_modules('_temp') as mapping:
            py_compile.compile(mapping['_temp'])
            os.unlink(mapping['_temp'])
            bytecode_path = support.bytecode_path(mapping['_temp'])
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
        with support.create_modules(*create) as mapping:
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
    @support.writes_bytecode
    def test_bad_magic(self):
        with support.create_modules('_temp') as mapping:
            py_compile.compile(mapping['_temp'])
            bytecode_path = support.bytecode_path(mapping['_temp'])
            with open(bytecode_path, 'r+b') as bytecode_file:
                bytecode_file.seek(0)
                bytecode_file.write(b'\x00\x00\x00\x00')
            self.import_(mapping['_temp'], '_temp')
            with open(bytecode_path, 'rb') as bytecode_file:
                self.assertEqual(bytecode_file.read(4), imp.get_magic())

    # [bad timestamp]
    @support.writes_bytecode
    def test_bad_bytecode(self):
        zeros = b'\x00\x00\x00\x00'
        with support.create_modules('_temp') as mapping:
            py_compile.compile(mapping['_temp'])
            bytecode_path = support.bytecode_path(mapping['_temp'])
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
        with support.create_modules('_temp') as mapping:
            bytecode_path = support.bytecode_path(mapping['_temp'])
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
