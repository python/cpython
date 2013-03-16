import importlib
from importlib import _bootstrap
from .. import abc
from .. import util
from . import util as source_util

import errno
import imp
import marshal
import os
import py_compile
import shutil
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
            loader = _bootstrap._SourceFileLoader('_temp', mapping['_temp'])
            module = loader.load_module('_temp')
            self.assertTrue('_temp' in sys.modules)
            check = {'__name__': '_temp', '__file__': mapping['_temp'],
                     '__package__': ''}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)

    def test_package(self):
        with source_util.create_modules('_pkg.__init__') as mapping:
            loader = _bootstrap._SourceFileLoader('_pkg',
                                                 mapping['_pkg.__init__'])
            module = loader.load_module('_pkg')
            self.assertTrue('_pkg' in sys.modules)
            check = {'__name__': '_pkg', '__file__': mapping['_pkg.__init__'],
                     '__path__': [os.path.dirname(mapping['_pkg.__init__'])],
                     '__package__': '_pkg'}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)


    def test_lacking_parent(self):
        with source_util.create_modules('_pkg.__init__', '_pkg.mod')as mapping:
            loader = _bootstrap._SourceFileLoader('_pkg.mod',
                                                    mapping['_pkg.mod'])
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
            loader = _bootstrap._SourceFileLoader('_temp', mapping['_temp'])
            module = loader.load_module('_temp')
            module_id = id(module)
            module_dict_id = id(module.__dict__)
            with open(mapping['_temp'], 'w') as file:
                file.write("testing_var = 42\n")
            # For filesystems where the mtime is only to a second granularity,
            # everything that has happened above can be too fast;
            # force an mtime on the source that is guaranteed to be different
            # than the original mtime.
            loader.path_mtime = self.fake_mtime(loader.path_mtime)
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
            loader = _bootstrap._SourceFileLoader('_temp', mapping['_temp'])
            with self.assertRaises(SyntaxError):
                loader.load_module(name)
            for attr in attributes:
                self.assertEqual(getattr(orig_module, attr), value)

    # [syntax error]
    def test_bad_syntax(self):
        with source_util.create_modules('_temp') as mapping:
            with open(mapping['_temp'], 'w') as file:
                file.write('=')
            loader = _bootstrap._SourceFileLoader('_temp', mapping['_temp'])
            with self.assertRaises(SyntaxError):
                loader.load_module('_temp')
            self.assertTrue('_temp' not in sys.modules)

    def test_file_from_empty_string_dir(self):
        # Loading a module found from an empty string entry on sys.path should
        # not only work, but keep all attributes relative.
        file_path = '_temp.py'
        with open(file_path, 'w') as file:
            file.write("# test file for importlib")
        try:
            with util.uncache('_temp'):
                loader = _bootstrap._SourceFileLoader('_temp', file_path)
                mod = loader.load_module('_temp')
                self.assertEqual(file_path, mod.__file__)
                self.assertEqual(imp.cache_from_source(file_path),
                                 mod.__cached__)
        finally:
            os.unlink(file_path)
            pycache = os.path.dirname(imp.cache_from_source(file_path))
            if os.path.exists(pycache):
                shutil.rmtree(pycache)

    def test_timestamp_overflow(self):
        # When a modification timestamp is larger than 2**32, it should be
        # truncated rather than raise an OverflowError.
        with source_util.create_modules('_temp') as mapping:
            source = mapping['_temp']
            compiled = imp.cache_from_source(source)
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
            loader = _bootstrap._SourceFileLoader('_temp', mapping['_temp'])
            mod = loader.load_module('_temp')
            # Sanity checks.
            self.assertEqual(mod.__cached__, compiled)
            self.assertEqual(mod.x, 5)
            # The pyc file was created.
            os.stat(compiled)


class BadBytecodeTest(unittest.TestCase):

    def import_(self, file, module_name):
        loader = self.loader(module_name, file)
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
        if not del_source:
            bytecode_path = imp.cache_from_source(mapping[name])
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
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: b'',
                                                del_source=del_source)
            test('_temp', mapping, bc_path)

    @source_util.writes_bytecode_files
    def _test_partial_magic(self, test, *, del_source=False):
        # When their are less than 4 bytes to a .pyc, regenerate it if
        # possible, else raise ImportError.
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:3],
                                                del_source=del_source)
            test('_temp', mapping, bc_path)

    def _test_magic_only(self, test, *, del_source=False):
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:4],
                                                del_source=del_source)
            test('_temp', mapping, bc_path)

    def _test_partial_timestamp(self, test, *, del_source=False):
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:7],
                                                del_source=del_source)
            test('_temp', mapping, bc_path)

    def _test_no_marshal(self, *, del_source=False):
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:8],
                                                del_source=del_source)
            file_path = mapping['_temp'] if not del_source else bc_path
            with self.assertRaises(EOFError):
                self.import_(file_path, '_temp')

    def _test_non_code_marshal(self, *, del_source=False):
        with source_util.create_modules('_temp') as mapping:
            bytecode_path = self.manipulate_bytecode('_temp', mapping,
                                    lambda bc: bc[:8] + marshal.dumps(b'abcd'),
                                    del_source=del_source)
            file_path = mapping['_temp'] if not del_source else bytecode_path
            with self.assertRaises(ImportError):
                self.import_(file_path, '_temp')

    def _test_bad_marshal(self, *, del_source=False):
        with source_util.create_modules('_temp') as mapping:
            bytecode_path = self.manipulate_bytecode('_temp', mapping,
                                                lambda bc: bc[:8] + b'<test>',
                                                del_source=del_source)
            file_path = mapping['_temp'] if not del_source else bytecode_path
            with self.assertRaises(EOFError):
                self.import_(file_path, '_temp')

    def _test_bad_magic(self, test, *, del_source=False):
        with source_util.create_modules('_temp') as mapping:
            bc_path = self.manipulate_bytecode('_temp', mapping,
                                    lambda bc: b'\x00\x00\x00\x00' + bc[4:])
            test('_temp', mapping, bc_path)


class SourceLoaderBadBytecodeTest(BadBytecodeTest):

    loader = _bootstrap._SourceFileLoader

    @source_util.writes_bytecode_files
    def test_empty_file(self):
        # When a .pyc is empty, regenerate it if possible, else raise
        # ImportError.
        def test(name, mapping, bytecode_path):
            self.import_(mapping[name], name)
            with open(bytecode_path, 'rb') as file:
                self.assertGreater(len(file.read()), 8)

        self._test_empty_file(test)

    def test_partial_magic(self):
        def test(name, mapping, bytecode_path):
            self.import_(mapping[name], name)
            with open(bytecode_path, 'rb') as file:
                self.assertGreater(len(file.read()), 8)

        self._test_partial_magic(test)

    @source_util.writes_bytecode_files
    def test_magic_only(self):
        # When there is only the magic number, regenerate the .pyc if possible,
        # else raise EOFError.
        def test(name, mapping, bytecode_path):
            self.import_(mapping[name], name)
            with open(bytecode_path, 'rb') as file:
                self.assertGreater(len(file.read()), 8)

        self._test_magic_only(test)

    @source_util.writes_bytecode_files
    def test_bad_magic(self):
        # When the magic number is different, the bytecode should be
        # regenerated.
        def test(name, mapping, bytecode_path):
            self.import_(mapping[name], name)
            with open(bytecode_path, 'rb') as bytecode_file:
                self.assertEqual(bytecode_file.read(4), imp.get_magic())

        self._test_bad_magic(test)

    @source_util.writes_bytecode_files
    def test_partial_timestamp(self):
        # When the timestamp is partial, regenerate the .pyc, else
        # raise EOFError.
        def test(name, mapping, bc_path):
            self.import_(mapping[name], name)
            with open(bc_path, 'rb') as file:
                self.assertGreater(len(file.read()), 8)

        self._test_partial_timestamp(test)

    @source_util.writes_bytecode_files
    def test_no_marshal(self):
        # When there is only the magic number and timestamp, raise EOFError.
        self._test_no_marshal()

    @source_util.writes_bytecode_files
    def test_non_code_marshal(self):
        self._test_non_code_marshal()
        # XXX ImportError when sourceless

    # [bad marshal]
    @source_util.writes_bytecode_files
    def test_bad_marshal(self):
        # Bad marshal data should raise a ValueError.
        self._test_bad_marshal()

    # [bad timestamp]
    @source_util.writes_bytecode_files
    def test_old_timestamp(self):
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


class SourcelessLoaderBadBytecodeTest(BadBytecodeTest):

    loader = _bootstrap._SourcelessFileLoader

    def test_empty_file(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(ImportError):
                self.import_(bytecode_path, name)

        self._test_empty_file(test, del_source=True)

    def test_partial_magic(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(ImportError):
                self.import_(bytecode_path, name)
        self._test_partial_magic(test, del_source=True)

    def test_magic_only(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(EOFError):
                self.import_(bytecode_path, name)

        self._test_magic_only(test, del_source=True)

    def test_bad_magic(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(ImportError):
                self.import_(bytecode_path, name)

        self._test_bad_magic(test, del_source=True)

    def test_partial_timestamp(self):
        def test(name, mapping, bytecode_path):
            with self.assertRaises(EOFError):
                self.import_(bytecode_path, name)

        self._test_partial_timestamp(test, del_source=True)

    def test_no_marshal(self):
        self._test_no_marshal(del_source=True)

    def test_non_code_marshal(self):
        self._test_non_code_marshal(del_source=True)


def test_main():
    from test.support import run_unittest
    run_unittest(SimpleTest,
                 SourceLoaderBadBytecodeTest,
                 SourcelessLoaderBadBytecodeTest
                )


if __name__ == '__main__':
    test_main()
