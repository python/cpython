from test.test_importlib import abc, util

importlib = util.import_importlib('importlib')
importlib_abc = util.import_importlib('importlib.abc')
machinery = util.import_importlib('importlib.machinery')
importlib_util = util.import_importlib('importlib.util')

import errno
import marshal
import os
import py_compile
import shutil
import stat
import sys
import types
import unittest
import warnings

from test.support.import_helper import make_legacy_pyc, unload

from test.test_py_compile import without_source_date_epoch
from test.test_py_compile import SourceDateEpochTestMeta


class SimpleTest:

    """Should have no issue importing a source module [basic]. And if there is
    a syntax error, it should raise a SyntaxError [syntax error].

    """

    def setUp(self):
        self.name = 'spam'
        self.filepath = os.path.join('ham', self.name + '.py')
        self.loader = self.machinery.SourceFileLoader(self.name, self.filepath)

    def test_get_filename_API(self):
        # If fullname is not set then assume self.path is desired.
        class Tester(self.abc.FileLoader):
            def get_code(self, _): pass
            def get_source(self, _): pass
            def is_package(self, _): pass

        path = 'some_path'
        name = 'some_name'
        loader = Tester(name, path)
        self.assertEqual(path, loader.get_filename(name))
        self.assertEqual(path, loader.get_filename())
        self.assertEqual(path, loader.get_filename(None))
        with self.assertRaises(ImportError):
            loader.get_filename(name + 'XXX')

    def test_equality(self):
        other = self.machinery.SourceFileLoader(self.name, self.filepath)
        self.assertEqual(self.loader, other)

    def test_inequality(self):
        other = self.machinery.SourceFileLoader('_' + self.name, self.filepath)
        self.assertNotEqual(self.loader, other)


    def fake_mtime(self, fxn):
        """Fake mtime to always be higher than expected."""
        return lambda name: fxn(name) + 1

    @util.writes_bytecode_files
    def test_checked_hash_based_pyc(self):
        with util.create_modules('_temp') as mapping:
            source = mapping['_temp']
            pyc = self.util.cache_from_source(source)
            with open(source, 'wb') as fp:
                fp.write(b'state = "old"')
            os.utime(source, (50, 50))
            py_compile.compile(
                source,
                invalidation_mode=py_compile.PycInvalidationMode.CHECKED_HASH,
            )
            loader = self.machinery.SourceFileLoader('_temp', source)
            mod = types.ModuleType('_temp')
            mod.__spec__ = self.util.spec_from_loader('_temp', loader)
            loader.exec_module(mod)
            self.assertEqual(mod.state, 'old')
            # Write a new source with the same mtime and size as before.
            with open(source, 'wb') as fp:
                fp.write(b'state = "new"')
            os.utime(source, (50, 50))
            loader.exec_module(mod)
            self.assertEqual(mod.state, 'new')
            with open(pyc, 'rb') as fp:
                data = fp.read()
            self.assertEqual(int.from_bytes(data[4:8], 'little'), 0b11)
            self.assertEqual(
                self.util.source_hash(b'state = "new"'),
                data[8:16],
            )

    @util.writes_bytecode_files
    def test_overridden_checked_hash_based_pyc(self):
        with util.create_modules('_temp') as mapping, \
             unittest.mock.patch('_imp.check_hash_based_pycs', 'never'):
            source = mapping['_temp']
            pyc = self.util.cache_from_source(source)
            with open(source, 'wb') as fp:
                fp.write(b'state = "old"')
            os.utime(source, (50, 50))
            py_compile.compile(
                source,
                invalidation_mode=py_compile.PycInvalidationMode.CHECKED_HASH,
            )
            loader = self.machinery.SourceFileLoader('_temp', source)
            mod = types.ModuleType('_temp')
            mod.__spec__ = self.util.spec_from_loader('_temp', loader)
            loader.exec_module(mod)
            self.assertEqual(mod.state, 'old')
            # Write a new source with the same mtime and size as before.
            with open(source, 'wb') as fp:
                fp.write(b'state = "new"')
            os.utime(source, (50, 50))
            loader.exec_module(mod)
            self.assertEqual(mod.state, 'old')

    @util.writes_bytecode_files
    def test_unchecked_hash_based_pyc(self):
        with util.create_modules('_temp') as mapping:
            source = mapping['_temp']
            pyc = self.util.cache_from_source(source)
            with open(source, 'wb') as fp:
                fp.write(b'state = "old"')
            os.utime(source, (50, 50))
            py_compile.compile(
                source,
                invalidation_mode=py_compile.PycInvalidationMode.UNCHECKED_HASH,
            )
            loader = self.machinery.SourceFileLoader('_temp', source)
            mod = types.ModuleType('_temp')
            mod.__spec__ = self.util.spec_from_loader('_temp', loader)
            loader.exec_module(mod)
            self.assertEqual(mod.state, 'old')
            # Update the source file, which should be ignored.
            with open(source, 'wb') as fp:
                fp.write(b'state = "new"')
            loader.exec_module(mod)
            self.assertEqual(mod.state, 'old')
            with open(pyc, 'rb') as fp:
                data = fp.read()
            self.assertEqual(int.from_bytes(data[4:8], 'little'), 0b1)
            self.assertEqual(
                self.util.source_hash(b'state = "old"'),
                data[8:16],
            )

    @util.writes_bytecode_files
    def test_overridden_unchecked_hash_based_pyc(self):
        with util.create_modules('_temp') as mapping, \
             unittest.mock.patch('_imp.check_hash_based_pycs', 'always'):
            source = mapping['_temp']
            pyc = self.util.cache_from_source(source)
            with open(source, 'wb') as fp:
                fp.write(b'state = "old"')
            os.utime(source, (50, 50))
            py_compile.compile(
                source,
                invalidation_mode=py_compile.PycInvalidationMode.UNCHECKED_HASH,
            )
            loader = self.machinery.SourceFileLoader('_temp', source)
            mod = types.ModuleType('_temp')
            mod.__spec__ = self.util.spec_from_loader('_temp', loader)
            loader.exec_module(mod)
            self.assertEqual(mod.state, 'old')
            # Update the source file, which should be ignored.
            with open(source, 'wb') as fp:
                fp.write(b'state = "new"')
            loader.exec_module(mod)
            self.assertEqual(mod.state, 'new')
            with open(pyc, 'rb') as fp:
                data = fp.read()
            self.assertEqual(int.from_bytes(data[4:8], 'little'), 0b1)
            self.assertEqual(
                self.util.source_hash(b'state = "new"'),
                data[8:16],
            )


(Frozen_SimpleTest,
 Source_SimpleTest
 ) = util.test_both(SimpleTest, importlib=importlib, machinery=machinery,
                    abc=importlib_abc, util=importlib_util)


class SourceDateEpochTestMeta(SourceDateEpochTestMeta,
                              type(Source_SimpleTest)):
    pass


class SourceDateEpoch_SimpleTest(Source_SimpleTest,
                                 metaclass=SourceDateEpochTestMeta,
                                 source_date_epoch=True):
    pass


class BadBytecodeTest:

    def import_(self, file, module_name):
        raise NotImplementedError

    def manipulate_bytecode(self,
                            name, mapping, manipulator, *,
                            del_source=False,
                            invalidation_mode=py_compile.PycInvalidationMode.TIMESTAMP):
        """Manipulate the bytecode of a module by passing it into a callable
        that returns what to use as the new bytecode."""
        try:
            del sys.modules['_temp']
        except KeyError:
            pass
        if sys.implementation.cache_tag is None:
            if del_source:
                bytecode_path = mapping[name] + 'c'
                py_compile.compile(mapping[name], bytecode_path,
                                   invalidation_mode=invalidation_mode)
                os.unlink(mapping[name])
            else:
                raise unittest.SkipTest('requires sys.implementation.cache_tag')
        else:
            py_compile.compile(mapping[name], invalidation_mode=invalidation_mode)
            if not del_source:
                bytecode_path = self.util.cache_from_source(mapping[name])
            else:
                os.unlink(mapping[name])
                bytecode_path = make_legacy_pyc(mapping[name])
        if manipulator:
            with open(bytecode_path, 'rb') as file:
                bc = file.read()
                new_bc = manipulator(bc)
            with open(bytecode_path, 'wb') as file:
                if new_bc is not None:
                    file.write(new_bc)
        return bytecode_path

    def _test_empty_file(self, test, *, del_source=False):
        with util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: b'',
                                                del_source=del_source)
            test('_temp', mapping, bc_path)

    @util.writes_bytecode_files
    def _test_partial_magic(self, test, *, del_source=False):
        # When their are less than 4 bytes to a .pyc, regenerate it if
        # possible, else raise ImportError.
        with util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:3],
                                                del_source=del_source)
            test('_temp', mapping, bc_path)

    def _test_magic_only(self, test, *, del_source=False):
        with util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:4],
                                                del_source=del_source)
            test('_temp', mapping, bc_path)

    def _test_partial_flags(self, test, *, del_source=False):
        with util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                               lambda bc: bc[:7],
                                               del_source=del_source)
            test('_temp', mapping, bc_path)

    def _test_partial_hash(self, test, *, del_source=False):
        with util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode(
                '_temp',
                mapping,
                lambda bc: bc[:13],
                del_source=del_source,
                invalidation_mode=py_compile.PycInvalidationMode.CHECKED_HASH,
            )
            test('_temp', mapping, bc_path)
        with util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode(
                '_temp',
                mapping,
                lambda bc: bc[:13],
                del_source=del_source,
                invalidation_mode=py_compile.PycInvalidationMode.UNCHECKED_HASH,
            )
            test('_temp', mapping, bc_path)

    def _test_partial_timestamp(self, test, *, del_source=False):
        with util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:11],
                                                del_source=del_source)
            test('_temp', mapping, bc_path)

    def _test_partial_size(self, test, *, del_source=False):
        with util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:15],
                                                del_source=del_source)
            test('_temp', mapping, bc_path)

    def _test_no_marshal(self, *, del_source=False):
        with util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:16],
                                                del_source=del_source)
            file_path = mapping['_temp'] if not del_source else bc_path
            with self.assertRaises(EOFError):
                self.import_(file_path, '_temp')

    def _test_non_code_marshal(self, *, del_source=False):
        with util.create_modules('_temp') as mapping:
            bytecode_path = self.manipulate_bytecode('_temp', mapping,
                                    lambda bc: bc[:16] + marshal.dumps(b'abcd'),
                                    del_source=del_source)
            file_path = mapping['_temp'] if not del_source else bytecode_path
            with self.assertRaises(ImportError) as cm:
                self.import_(file_path, '_temp')
            self.assertEqual(cm.exception.name, '_temp')
            self.assertEqual(cm.exception.path, bytecode_path)

    def _test_bad_marshal(self, *, del_source=False):
        with util.create_modules('_temp') as mapping:
            bytecode_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:16] + b'<test>',
                                                del_source=del_source)
            file_path = mapping['_temp'] if not del_source else bytecode_path
            with self.assertRaises(EOFError):
                self.import_(file_path, '_temp')

    def _test_bad_magic(self, test, *, del_source=False):
        with util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                    lambda bc: b'\x00\x00\x00\x00' + bc[4:])
            test('_temp', mapping, bc_path)


class BadBytecodeTestPEP451(BadBytecodeTest):

    def import_(self, file, module_name):
        loader = self.loader(module_name, file)
        module = types.ModuleType(module_name)
        module.__spec__ = self.util.spec_from_loader(module_name, loader)
        loader.exec_module(module)


class SourceLoaderBadBytecodeTest:

    @classmethod
    def setUpClass(cls):
        cls.loader = cls.machinery.SourceFileLoader

    @util.writes_bytecode_files
    def test_empty_file(self):
        # When a .pyc is empty, regenerate it if possible, else raise
        # ImportError.
        def test(name, mapping, bytecode_path):
            self.import_(mapping[name], name)
            with open(bytecode_path, 'rb') as file:
                self.assertGreater(len(file.read()), 16)

        self._test_empty_file(test)

    def test_partial_magic(self):
        def test(name, mapping, bytecode_path):
            self.import_(mapping[name], name)
            with open(bytecode_path, 'rb') as file:
                self.assertGreater(len(file.read()), 16)

        self._test_partial_magic(test)

    @util.writes_bytecode_files
    def test_magic_only(self):
        # When there is only the magic number, regenerate the .pyc if possible,
        # else raise EOFError.
        def test(name, mapping, bytecode_path):
            self.import_(mapping[name], name)
            with open(bytecode_path, 'rb') as file:
                self.assertGreater(len(file.read()), 16)

        self._test_magic_only(test)

    @util.writes_bytecode_files
    def test_bad_magic(self):
        # When the magic number is different, the bytecode should be
        # regenerated.
        def test(name, mapping, bytecode_path):
            self.import_(mapping[name], name)
            with open(bytecode_path, 'rb') as bytecode_file:
                self.assertEqual(bytecode_file.read(4),
                                 self.util.MAGIC_NUMBER)

        self._test_bad_magic(test)

    @util.writes_bytecode_files
    def test_partial_timestamp(self):
        # When the timestamp is partial, regenerate the .pyc, else
        # raise EOFError.
        def test(name, mapping, bc_path):
            self.import_(mapping[name], name)
            with open(bc_path, 'rb') as file:
                self.assertGreater(len(file.read()), 16)

        self._test_partial_timestamp(test)

    @util.writes_bytecode_files
    def test_partial_flags(self):
        # When the flags is partial, regenerate the .pyc, else raise EOFError.
        def test(name, mapping, bc_path):
            self.import_(mapping[name], name)
            with open(bc_path, 'rb') as file:
                self.assertGreater(len(file.read()), 16)

        self._test_partial_flags(test)

    @util.writes_bytecode_files
    def test_partial_hash(self):
        # When the hash is partial, regenerate the .pyc, else raise EOFError.
        def test(name, mapping, bc_path):
            self.import_(mapping[name], name)
            with open(bc_path, 'rb') as file:
                self.assertGreater(len(file.read()), 16)

        self._test_partial_hash(test)

    @util.writes_bytecode_files
    def test_partial_size(self):
        # When the size is partial, regenerate the .pyc, else
        # raise EOFError.
        def test(name, mapping, bc_path):
            self.import_(mapping[name], name)
            with open(bc_path, 'rb') as file:
                self.assertGreater(len(file.read()), 16)

        self._test_partial_size(test)

    @util.writes_bytecode_files
    def test_no_marshal(self):
        # When there is only the magic number and timestamp, raise EOFError.
        self._test_no_marshal()

    @util.writes_bytecode_files
    def test_non_code_marshal(self):
        self._test_non_code_marshal()
        # XXX ImportError when sourceless

    # [bad marshal]
    @util.writes_bytecode_files
    def test_bad_marshal(self):
        # Bad marshal data should raise a ValueError.
        self._test_bad_marshal()

    # [bad timestamp]
    @util.writes_bytecode_files
    @without_source_date_epoch
    def test_old_timestamp(self):
        # When the timestamp is older than the source, bytecode should be
        # regenerated.
        zeros = b'\x00\x00\x00\x00'
        with util.create_modules('_temp') as mapping:
            py_compile.compile(mapping['_temp'])
            bytecode_path = self.util.cache_from_source(mapping['_temp'])
            with open(bytecode_path, 'r+b') as bytecode_file:
                bytecode_file.seek(8)
                bytecode_file.write(zeros)
            self.import_(mapping['_temp'], '_temp')
            source_mtime = os.path.getmtime(mapping['_temp'])
            source_timestamp = self.importlib._pack_uint32(source_mtime)
            with open(bytecode_path, 'rb') as bytecode_file:
                bytecode_file.seek(8)
                self.assertEqual(bytecode_file.read(4), source_timestamp)

    # [bytecode read-only]
    @util.writes_bytecode_files
    def test_read_only_bytecode(self):
        # When bytecode is read-only but should be rewritten, fail silently.
        with util.create_modules('_temp') as mapping:
            # Create bytecode that will need to be re-created.
            py_compile.compile(mapping['_temp'])
            bytecode_path = self.util.cache_from_source(mapping['_temp'])
            with open(bytecode_path, 'r+b') as bytecode_file:
                bytecode_file.seek(0)
                bytecode_file.write(b'\x00\x00\x00\x00')
            # Make the bytecode read-only.
            os.chmod(bytecode_path,
                        stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
            try:
                # Should not raise OSError!
                self.import_(mapping['_temp'], '_temp')
            finally:
                # Make writable for eventual clean-up.
                os.chmod(bytecode_path, stat.S_IWUSR)


class SourceLoaderBadBytecodeTestPEP451(
        SourceLoaderBadBytecodeTest, BadBytecodeTestPEP451):
    pass


(Frozen_SourceBadBytecodePEP451,
 Source_SourceBadBytecodePEP451
 ) = util.test_both(SourceLoaderBadBytecodeTestPEP451, importlib=importlib,
                    machinery=machinery, abc=importlib_abc,
                    util=importlib_util)


class SourcelessLoaderBadBytecodeTest:

    @classmethod
    def setUpClass(cls):
        cls.loader = cls.machinery.SourcelessFileLoader

    def test_empty_file(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(ImportError) as cm:
                self.import_(bytecode_path, name)
            self.assertEqual(cm.exception.name, name)
            self.assertEqual(cm.exception.path, bytecode_path)

        self._test_empty_file(test, del_source=True)

    def test_partial_magic(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(ImportError) as cm:
                self.import_(bytecode_path, name)
            self.assertEqual(cm.exception.name, name)
            self.assertEqual(cm.exception.path, bytecode_path)
        self._test_partial_magic(test, del_source=True)

    def test_magic_only(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(EOFError):
                self.import_(bytecode_path, name)

        self._test_magic_only(test, del_source=True)

    def test_bad_magic(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(ImportError) as cm:
                self.import_(bytecode_path, name)
            self.assertEqual(cm.exception.name, name)
            self.assertEqual(cm.exception.path, bytecode_path)

        self._test_bad_magic(test, del_source=True)

    def test_partial_timestamp(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(EOFError):
                self.import_(bytecode_path, name)

        self._test_partial_timestamp(test, del_source=True)

    def test_partial_flags(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(EOFError):
                self.import_(bytecode_path, name)

        self._test_partial_flags(test, del_source=True)

    def test_partial_hash(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(EOFError):
                self.import_(bytecode_path, name)

        self._test_partial_hash(test, del_source=True)

    def test_partial_size(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(EOFError):
                self.import_(bytecode_path, name)

        self._test_partial_size(test, del_source=True)

    def test_no_marshal(self):
        self._test_no_marshal(del_source=True)

    def test_non_code_marshal(self):
        self._test_non_code_marshal(del_source=True)


class SourcelessLoaderBadBytecodeTestPEP451(SourcelessLoaderBadBytecodeTest,
        BadBytecodeTestPEP451):
    pass


(Frozen_SourcelessBadBytecodePEP451,
 Source_SourcelessBadBytecodePEP451
 ) = util.test_both(SourcelessLoaderBadBytecodeTestPEP451, importlib=importlib,
                    machinery=machinery, abc=importlib_abc,
                    util=importlib_util)


if __name__ == '__main__':
    unittest.main()
