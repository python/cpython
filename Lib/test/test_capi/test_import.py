import importlib.util
import sys
import types
import unittest
from test.support import import_helper
from test.support.warnings_helper import check_warnings

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')


class ImportTests(unittest.TestCase):
    def test_getmagicnumber(self):
        # Test PyImport_GetMagicNumber()
        magic = _testlimitedcapi.PyImport_GetMagicNumber()
        self.assertEqual(magic,
                         int.from_bytes(importlib.util.MAGIC_NUMBER, 'little'))

    def test_getmagictag(self):
        # Test PyImport_GetMagicTag()
        tag = _testlimitedcapi.PyImport_GetMagicTag()
        self.assertEqual(tag, sys.implementation.cache_tag)

    def test_getmoduledict(self):
        # Test PyImport_GetModuleDict()
        modules = _testlimitedcapi.PyImport_GetModuleDict()
        self.assertIs(modules, sys.modules)

    def check_import_loaded_module(self, import_module):
        for name in ('os', 'sys', 'test', 'unittest'):
            with self.subTest(name=name):
                self.assertIn(name, sys.modules)
                module = import_module(name)
                self.assertIsInstance(module, types.ModuleType)
                self.assertIs(module, sys.modules[name])

    def check_import_fresh_module(self, import_module):
        old_modules = dict(sys.modules)
        try:
            for name in ('colorsys', 'datetime', 'math'):
                with self.subTest(name=name):
                    sys.modules.pop(name, None)
                    module = import_module(name)
                    self.assertIsInstance(module, types.ModuleType)
                    self.assertIs(module, sys.modules[name])
                    self.assertEqual(module.__name__, name)
        finally:
            sys.modules.clear()
            sys.modules.update(old_modules)

    def test_getmodule(self):
        # Test PyImport_GetModule()
        self.check_import_loaded_module(_testlimitedcapi.PyImport_GetModule)

        nonexistent = 'nonexistent'
        self.assertNotIn(nonexistent, sys.modules)
        self.assertIsNone(_testlimitedcapi.PyImport_GetModule(nonexistent))
        self.assertIsNone(_testlimitedcapi.PyImport_GetModule(''))
        self.assertIsNone(_testlimitedcapi.PyImport_GetModule(object()))

    def check_addmodule(self, add_module):
        # create a new module
        name = 'nonexistent'
        self.assertNotIn(name, sys.modules)
        try:
            module = add_module(name)
            self.assertIsInstance(module, types.ModuleType)
            self.assertIs(module, sys.modules[name])
        finally:
            sys.modules.pop(name, None)

        # get an existing module
        self.check_import_loaded_module(add_module)

    def test_addmoduleobject(self):
        # Test PyImport_AddModuleObject()
        self.check_addmodule(_testlimitedcapi.PyImport_AddModuleObject)

    def test_addmodule(self):
        # Test PyImport_AddModule()
        self.check_addmodule(_testlimitedcapi.PyImport_AddModule)

    def test_addmoduleref(self):
        # Test PyImport_AddModuleRef()
        self.check_addmodule(_testlimitedcapi.PyImport_AddModuleRef)

    def check_import_func(self, import_module):
        self.check_import_loaded_module(import_module)
        self.check_import_fresh_module(import_module)

        # Invalid module name types
        with self.assertRaises(TypeError):
            import_module(123)
        with self.assertRaises(TypeError):
            import_module(object())

    def test_import(self):
        # Test PyImport_Import()
        self.check_import_func(_testlimitedcapi.PyImport_Import)

    def test_importmodule(self):
        # Test PyImport_ImportModule()
        self.check_import_func(_testlimitedcapi.PyImport_ImportModule)

    def test_importmodulenoblock(self):
        # Test deprecated PyImport_ImportModuleNoBlock()
        with check_warnings(('', DeprecationWarning)):
            self.check_import_func(_testlimitedcapi.PyImport_ImportModuleNoBlock)

    # TODO: test PyImport_ExecCodeModule()
    # TODO: test PyImport_ExecCodeModuleEx()
    # TODO: test PyImport_ExecCodeModuleWithPathnames()
    # TODO: test PyImport_ExecCodeModuleObject()
    # TODO: test PyImport_ImportModuleLevel()
    # TODO: test PyImport_ImportModuleLevelObject()
    # TODO: test PyImport_ImportModuleEx()
    # TODO: test PyImport_GetImporter()
    # TODO: test PyImport_ReloadModule()
    # TODO: test PyImport_ImportFrozenModuleObject()
    # TODO: test PyImport_ImportFrozenModule()
    # TODO: test PyImport_AppendInittab()
    # TODO: test PyImport_ExtendInittab()


if __name__ == "__main__":
    unittest.main()
