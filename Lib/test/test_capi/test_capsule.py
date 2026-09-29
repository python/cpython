import importlib
import os
import sys
import textwrap
import unittest
from test.support import import_helper, os_helper

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')


class CapsuleImportTests(unittest.TestCase):
    """Tests for PyCapsule_Import()."""

    @classmethod
    def setUpClass(cls):
        tmp = cls.tmp = cls.enterClassContext(os_helper.temp_dir())
        cls.enterClassContext(import_helper.DirsOnSysPath(tmp))
        cls.write_file(os.path.join(tmp, 'capsule_mod.py'), '''
            import _testlimitedcapi

            capsule = _testlimitedcapi.capsule_new('capsule_mod.capsule')
            капсула = _testlimitedcapi.capsule_new('capsule_mod.капсула')
            mismatched = _testlimitedcapi.capsule_new('other.name')
            nonutf8 = _testlimitedcapi.capsule_new(b'capsule_mod.nonutf8\\xff')
            nullname = _testlimitedcapi.capsule_new(None)
            not_capsule = 42

            class ns:
                nested = _testlimitedcapi.capsule_new('capsule_mod.ns.nested')

            def __getattr__(name):
                if name == 'bad_attr':
                    raise FloatingPointError('bad attribute')
                raise AttributeError(name)
        ''')
        pkg = os.path.join(tmp, 'capsule_pkg')
        os.mkdir(pkg)
        cls.write_file(os.path.join(pkg, '__init__.py'), '')
        cls.write_file(os.path.join(pkg, 'sub.py'), '''
            import _testlimitedcapi

            capsule = _testlimitedcapi.capsule_new('capsule_pkg.sub.capsule')
        ''')
        autopkg = os.path.join(tmp, 'capsule_autopkg')
        os.mkdir(autopkg)
        cls.write_file(os.path.join(autopkg, '__init__.py'), 'from . import sub\n')
        cls.write_file(os.path.join(autopkg, 'sub.py'), '''
            import _testlimitedcapi

            capsule = _testlimitedcapi.capsule_new('capsule_autopkg.sub.capsule')
        ''')
        cls.write_file(os.path.join(tmp, 'capsule_broken.py'), '1/0\n')
        importlib.invalidate_caches()

    def setUp(self):
        for name in ('capsule_mod', 'capsule_pkg.sub', 'capsule_pkg',
                     'capsule_autopkg.sub', 'capsule_autopkg',
                     'capsule_broken'):
            self.addCleanup(import_helper.unload, name)

    @staticmethod
    def write_file(path, source):
        with open(path, 'w', encoding='utf-8') as f:
            f.write(textwrap.dedent(source))

    def check_import(self, name, no_block=0):
        # _testlimitedcapi.PyCapsule_Import() returns the name stored as the
        # pointer by _testlimitedcapi.capsule_new().
        self.assertEqual(_testlimitedcapi.PyCapsule_Import(name, no_block), name)

    def test_import(self):
        # The module is imported if not already imported.
        self.assertNotIn('capsule_mod', sys.modules)
        self.check_import('capsule_mod.capsule')
        # Attributes after the first component are plain attribute lookups.
        self.check_import('capsule_mod.ns.nested')
        # Non-ASCII capsule and attribute name.
        self.check_import('capsule_mod.капсула')
        # The no_block argument is ignored.
        self.check_import('capsule_mod.capsule', 1)

    @unittest.skipUnless(os_helper.TESTFN_NONASCII,
                         'requires non-ASCII file name support')
    def test_non_ascii_module_name(self):
        name = os_helper.TESTFN_NONASCII
        self.write_file(os.path.join(self.tmp, name + '.py'), f'''
            import _testlimitedcapi

            capsule = _testlimitedcapi.capsule_new('{name}.capsule')
        ''')
        importlib.invalidate_caches()
        self.addCleanup(import_helper.unload, name)
        self.check_import(f'{name}.capsule')

    def test_submodule(self):
        # Only the first component is imported; a submodule not imported
        # by its package is not found.
        self.assertRaises(AttributeError,
                          _testlimitedcapi.PyCapsule_Import, 'capsule_pkg.sub.capsule')
        # It is found after explicit import.
        importlib.import_module('capsule_pkg.sub')
        self.check_import('capsule_pkg.sub.capsule')
        # A submodule imported by its package is found.
        self.check_import('capsule_autopkg.sub.capsule')

    def test_invalid_name(self):
        pycapsule_import = _testlimitedcapi.PyCapsule_Import
        # Non-existing module.
        self.assertRaisesRegex(ImportError,
            'PyCapsule_Import could not import module "capsule_nonexistent"',
            pycapsule_import, 'capsule_nonexistent.capsule')
        # Non-UTF-8 module name.
        self.assertRaisesRegex(ImportError,
            'PyCapsule_Import could not import module',
            pycapsule_import, b'\xff\xfe.capsule')
        # Empty module name.
        self.assertRaisesRegex(ImportError,
            'PyCapsule_Import could not import module ""',
            pycapsule_import, '.capsule_mod.capsule')
        # Empty name.
        self.assertRaisesRegex(ImportError,
            'PyCapsule_Import could not import module ""',
            pycapsule_import, '')
        # Only a dot.
        self.assertRaisesRegex(ImportError,
            'PyCapsule_Import could not import module ""',
            pycapsule_import, '.')
        # Non-existing attribute.
        self.assertRaises(AttributeError,
                          pycapsule_import, 'capsule_mod.nonexistent')
        # Empty attribute name.
        self.assertRaises(AttributeError, pycapsule_import, 'capsule_mod.')
        # Consecutive dots.
        self.assertRaises(AttributeError,
                          pycapsule_import, 'capsule_mod..capsule')
        # Attribute of an object which is not a module.
        self.assertRaises(AttributeError,
                          pycapsule_import, 'capsule_mod.not_capsule.capsule')
        # No attribute name.
        self.assertRaisesRegex(AttributeError, 'is not valid',
                               pycapsule_import, 'capsule_mod')

        # CRASHES pycapsule_import(NULL)

    def test_invalid_capsule(self):
        pycapsule_import = _testlimitedcapi.PyCapsule_Import
        # The attribute is not a capsule.
        self.assertRaisesRegex(AttributeError, 'is not valid',
                               pycapsule_import, 'capsule_mod.not_capsule')
        # The capsule name does not match the requested name.
        self.assertRaisesRegex(AttributeError, 'is not valid',
                               pycapsule_import, 'capsule_mod.mismatched')
        # The capsule name contains a byte not decodable from UTF-8.
        self.assertRaisesRegex(AttributeError, 'is not valid',
                               pycapsule_import, 'capsule_mod.nonutf8')
        # Even the exactly matching name fails: the attribute lookup
        # requires a name decodable from UTF-8.
        self.assertRaises(UnicodeDecodeError,
                          pycapsule_import, b'capsule_mod.nonutf8\xff')
        # The capsule name is NULL.
        self.assertRaisesRegex(AttributeError, 'is not valid',
                               pycapsule_import, 'capsule_mod.nullname')

    def test_error_from_import(self):
        # The exception raised during importing the module is replaced
        # with generic ImportError.
        with self.assertRaises(ImportError) as cm:
            _testlimitedcapi.PyCapsule_Import('capsule_broken.capsule')
        self.assertEqual(str(cm.exception),
                         'PyCapsule_Import could not import '
                         'module "capsule_broken"')

    def test_error_from_attribute_lookup(self):
        self.assertRaises(FloatingPointError,
                          _testlimitedcapi.PyCapsule_Import, 'capsule_mod.bad_attr')
        self.assertRaises(FloatingPointError,
                          _testlimitedcapi.PyCapsule_Import, 'capsule_mod.bad_attr.capsule')


if __name__ == "__main__":
    unittest.main()
