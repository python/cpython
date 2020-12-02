from .. import abc
from .. import util

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


class SimpleTest(abc.LoaderTests):

    """Should have no issue importing a source module [basic]. And if there is
    a syntax error, it should raise a SyntaxError [syntax error].

    """

    def setUp(self):
        self.name = 'spam'
        self.filepath = os.path.join('ham', self.name + '.py')
        self.loader = self.machinery.SourceFileLoader(self.name, self.filepath)

    def test_load_module_API(self):
        class Tester(self.abc.FileLoader):
            def get_source(self, _): return 'attr = 42'
            def is_package(self, _): return False

        loader = Tester('blah', 'blah.py')
        self.addCleanup(unload, 'blah')
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            module = loader.load_module()  # Should not raise an exception.

    def test_get_filename_API(self):
        # If fullname is not set then assume self.path is desired.
        class Tester(self.abc.FileLoader):
            def get_code(self, _): pass
            def get_source(self, _): pass
            def is_package(self, _): pass
            def module_repr(self, _): pass

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

    # [basic]
    def test_module(self):
        with util.create_modules('_temp') as mapping:
            loader = self.machinery.SourceFileLoader('_temp', mapping['_temp'])
            with warnings.catch_warnings():
                warnings.simplefilter('ignore', DeprecationWarning)
                module = loader.load_module('_temp')
            self.assertIn('_temp', sys.modules)
            check = {'__name__': '_temp', '__file__': mapping['_temp'],
                     '__package__': ''}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)

    def test_package(self):
        with util.create_modules('_pkg.__init__') as mapping:
            loader = self.machinery.SourceFileLoader('_pkg',
                                                 mapping['_pkg.__init__'])
            with warnings.catch_warnings():
                warnings.simplefilter('ignore', DeprecationWarning)
                module = loader.load_module('_pkg')
            self.assertIn('_pkg', sys.modules)
            check = {'__name__': '_pkg', '__file__': mapping['_pkg.__init__'],
                     '__path__': [os.path.dirname(mapping['_pkg.__init__'])],
                     '__package__': '_pkg'}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)


    def test_lacking_parent(self):
        with util.create_modules('_pkg.__init__', '_pkg.mod')as mapping:
            loader = self.machinery.SourceFileLoader('_pkg.mod',
                                                    mapping['_pkg.mod'])
            with warnings.catch_warnings():
                warnings.simplefilter('ignore', DeprecationWarning)
                module = loader.load_module('_pkg.mod')
            self.assertIn('_pkg.mod', sys.modules)
            check = {'__name__': '_pkg.mod', '__file__': mapping['_pkg.mod'],
                     '__package__': '_pkg'}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)

    def fake_mtime(self, fxn):
        """Fake mtime to always be higher than expected."""
        return lambda name: fxn(name) + 1

    def test_module_reuse(self):
        with util.create_modules('_temp') as mapping:
            loader = self.machinery.SourceFileLoader('_temp', mapping['_temp'])
            with warnings.catch_warnings():
                warnings.simplefilter('ignore', DeprecationWarning)
                module = loader.load_module('_temp')
            module_id = id(module)
            module_dict_id = id(module.__dict__)
            with open(mapping['_temp'], 'w') as file:
                file.write("testing_var = 42\n")
            with warnings.catch_warnings():
                warnings.simplefilter('ignore', DeprecationWarning)
                module = loader.load_module('_temp')
            self.assertIn('testing_var', module.__dict__,
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
        with util.create_modules(name) as mapping:
            orig_module = types.ModuleType(name)
            for attr in attributes:
                setattr(orig_module, attr, value)
            with open(mapping[name], 'w') as file:
                file.write('+++ bad syntax +++')
            loader = self.machinery.SourceFileLoader('_temp', mapping['_temp'])
            with self.assertRaises(SyntaxError):
                loader.exec_module(orig_module)
            for attr in attributes:
                self.assertEqual(getattr(orig_module, attr), value)
            with self.assertRaises(SyntaxError):
                with warnings.catch_warnings():
                    warnings.simplefilter('ignore', DeprecationWarning)
                    loader.load_module(name)
            for attr in attributes:
                self.assertEqual(getattr(orig_module, attr), value)

    # [syntax error]
    def test_bad_syntax(self):
        with util.create_modules('_temp') as mapping:
            with open(mapping['_temp'], 'w') as file:
                file.write('=')
            loader = self.machinery.SourceFileLoader('_temp', mapping['_temp'])
            with self.assertRaises(SyntaxError):
                with warnings.catch_warnings():
                    warnings.simplefilter('ignore', DeprecationWarning)
                    loader.load_module('_temp')
            self.assertNotIn('_temp', sys.modules)

    def test_file_from_empty_string_dir(self):
        # Loading a module found from an empty string entry on sys.path should
        # not only work, but keep all attributes relative.
        file_path = '_temp.py'
        with open(file_path, 'w') as file:
            file.write("# test file for importlib")
        try:
            with util.uncache('_temp'):
                loader = self.machinery.SourceFileLoader('_temp', file_path)
                with warnings.catch_warnings():
                    warnings.simplefilter('ignore', DeprecationWarning)
                    mod = loader.load_module('_temp')
                self.assertEqual(file_path, mod.__file__)
                self.assertEqual(self.util.cache_from_source(file_path),
                                 mod.__cached__)
        finally:
            os.unlink(file_path)
            pycache = os.path.dirname(self.util.cache_from_source(file_path))
            if os.path.exists(pycache):
                shutil.rmtree(pycache)

    @util.writes_bytecode_files
    def test_timestamp_overflow(self):
        # When a modification timestamp is larger than 2**32, it should be
        # truncated rather than raise an OverflowError.
        with util.create_modules('_temp') as mapping:
            source = mapping['_temp']
            compiled = self.util.cache_from_source(source)
            with open(source, 'w') as f:
                f.write("x = 5")
            try:
                os.utime(source, (2 ** 33 - 5, 2 ** 33 - 5))
            except OverflowError:
                self.skipTest("cannot set modification time to large integer")
            except OSError as e:
                if e.errno != getattr(errno, 'EOVERFLOW', None):
                    raise
                self.skipTest("cannot set modification time to large integer ({})".format(e))
            loader = self.machinery.SourceFileLoader('_temp', mapping['_temp'])
            # PEP 451
            module = types.ModuleType('_temp')
            module.__spec__ = self.util.spec_from_loader('_temp', loader)
            loader.exec_module(module)
            self.assertEqual(module.x, 5)
            self.assertTrue(os.path.exists(compiled))
            os.unlink(compiled)
            # PEP 302
            with warnings.catch_warnings():
                warnings.simplefilter('ignore', DeprecationWarning)
                mod = loader.load_module('_temp')
            # Sanity checks.
            self.assertEqual(mod.__cached__, compiled)
            self.assertEqual(mod.x, 5)
            # The pyc file was created.
            self.assertTrue(os.path.exists(compiled))

    def test_unloadable(self):
        loader = self.machinery.SourceFileLoader('good name', {})
        module = types.ModuleType('bad name')
        module.__spec__ = self.machinery.ModuleSpec('bad name', loader)
        with self.assertRaises(ImportError):
            loader.exec_module(module)
        with self.assertRaises(ImportError):
            with warnings.catch_warnings():
                warnings.simplefilter('ignore', DeprecationWarning)
                loader.load_module('bad name')

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


class BadBytecodeTestPEP302(BadBytecodeTest):

    def import_(self, file, module_name):
        loader = self.loader(module_name, file)
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            module = loader.load_module(module_name)
        self.assertIn(module_name, sys.modules)


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


class SourceLoaderBadBytecodeTestPEP302(
        SourceLoaderBadBytecodeTest, BadBytecodeTestPEP302):
    pass


(Frozen_SourceBadBytecodePEP302,
 Source_SourceBadBytecodePEP302
 ) = util.test_both(SourceLoaderBadBytecodeTestPEP302, importlib=importlib,
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


class SourcelessLoaderBadBytecodeTestPEP302(SourcelessLoaderBadBytecodeTest,
        BadBytecodeTestPEP302):
    pass


(Frozen_SourcelessBadBytecodePEP302,
 Source_SourcelessBadBytecodePEP302
 ) = util.test_both(SourcelessLoaderBadBytecodeTestPEP302, importlib=importlib,
                    machinery=machinery, abc=importlib_abc,
                    util=importlib_util)


if __name__ == '__main__':
    unittest.main()
