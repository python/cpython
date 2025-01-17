import importlib.util
import os.path
import sys
import types
import unittest
from test.support import import_helper
from test.support.warnings_helper import check_warnings

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
NULL = None


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
            for name in ('colorsys', 'math'):
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
        for name in (nonexistent, '', object()):
            with self.subTest(name=name):
                with self.assertRaises(KeyError):
                    _testlimitedcapi.PyImport_GetModule(name)

        # CRASHES PyImport_GetModule(NULL)

    def check_addmodule(self, add_module, accept_nonstr=False):
        # create a new module
        names = ['nonexistent']
        if accept_nonstr:
            # PyImport_AddModuleObject() accepts non-string names
            names.append(object())
        for name in names:
            with self.subTest(name=name):
                self.assertNotIn(name, sys.modules)
                try:
                    module = add_module(name)
                    self.assertIsInstance(module, types.ModuleType)
                    self.assertEqual(module.__name__, name)
                    self.assertIs(module, sys.modules[name])
                finally:
                    sys.modules.pop(name, None)

        # get an existing module
        self.check_import_loaded_module(add_module)

    def test_addmoduleobject(self):
        # Test PyImport_AddModuleObject()
        self.check_addmodule(_testlimitedcapi.PyImport_AddModuleObject,
                             accept_nonstr=True)

        # CRASHES PyImport_AddModuleObject(NULL)

    def test_addmodule(self):
        # Test PyImport_AddModule()
        self.check_addmodule(_testlimitedcapi.PyImport_AddModule)

        # CRASHES PyImport_AddModule(NULL)

    def test_addmoduleref(self):
        # Test PyImport_AddModuleRef()
        self.check_addmodule(_testlimitedcapi.PyImport_AddModuleRef)

        # CRASHES PyImport_AddModuleRef(NULL)

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

        with self.assertRaises(SystemError):
            _testlimitedcapi.PyImport_Import(NULL)

    def test_importmodule(self):
        # Test PyImport_ImportModule()
        self.check_import_func(_testlimitedcapi.PyImport_ImportModule)

        # CRASHES PyImport_ImportModule(NULL)

    def test_importmodulenoblock(self):
        # Test deprecated PyImport_ImportModuleNoBlock()
        with check_warnings(('', DeprecationWarning)):
            self.check_import_func(_testlimitedcapi.PyImport_ImportModuleNoBlock)

        # CRASHES PyImport_ImportModuleNoBlock(NULL)

    def check_frozen_import(self, import_frozen_module):
        # Importing a frozen module executes its code, so start by unloading
        # the module to execute the code in a new (temporary) module.
        old_zipimport = sys.modules.pop('zipimport')
        try:
            self.assertEqual(import_frozen_module('zipimport'), 1)
        finally:
            sys.modules['zipimport'] = old_zipimport

        # not a frozen module
        self.assertEqual(import_frozen_module('sys'), 0)

    def test_importfrozenmodule(self):
        # Test PyImport_ImportFrozenModule()
        self.check_frozen_import(_testlimitedcapi.PyImport_ImportFrozenModule)

        # CRASHES PyImport_ImportFrozenModule(NULL)

    def test_importfrozenmoduleobject(self):
        # Test PyImport_ImportFrozenModuleObject()
        PyImport_ImportFrozenModuleObject = _testlimitedcapi.PyImport_ImportFrozenModuleObject
        self.check_frozen_import(PyImport_ImportFrozenModuleObject)

        # Bad name is treated as "not found"
        self.assertEqual(PyImport_ImportFrozenModuleObject(None), 0)

    def test_importmoduleex(self):
        # Test PyImport_ImportModuleEx()
        def import_module(name):
            return _testlimitedcapi.PyImport_ImportModuleEx(
                name, globals(), {}, [])

        self.check_import_func(import_module)

    def test_importmodulelevel(self):
        # Test PyImport_ImportModuleLevel()
        def import_module(name):
            return _testlimitedcapi.PyImport_ImportModuleLevel(
                name, globals(), {}, [], 0)

        self.check_import_func(import_module)

    def test_importmodulelevelobject(self):
        # Test PyImport_ImportModuleLevelObject()
        def import_module(name):
            return _testlimitedcapi.PyImport_ImportModuleLevelObject(
                name, globals(), {}, [], 0)

        self.check_import_func(import_module)

    def check_executecodemodule(self, execute_code, pathname=None):
        name = 'test_import_executecode'
        try:
            # Create a temporary module where the code will be executed
            self.assertNotIn(name, sys.modules)
            module = _testlimitedcapi.PyImport_AddModuleRef(name)
            self.assertNotHasAttr(module, 'attr')

            # Execute the code
            if pathname is not None:
                code_filename = pathname
            else:
                code_filename = '<string>'
            code = compile('attr = 1', code_filename, 'exec')
            module2 = execute_code(name, code)
            self.assertIs(module2, module)

            # Check the function side effects
            self.assertEqual(module.attr, 1)
            if pathname is not None:
                self.assertEqual(module.__spec__.origin, pathname)
        finally:
            sys.modules.pop(name, None)

    def test_executecodemodule(self):
        # Test PyImport_ExecCodeModule()
        self.check_executecodemodule(_testlimitedcapi.PyImport_ExecCodeModule)

    def test_executecodemoduleex(self):
        # Test PyImport_ExecCodeModuleEx()

        # Test NULL path (it should not crash)
        def execute_code1(name, code):
            return _testlimitedcapi.PyImport_ExecCodeModuleEx(name, code,
                                                              NULL)
        self.check_executecodemodule(execute_code1)

        # Test non-NULL path
        pathname = os.path.abspath('pathname')
        def execute_code2(name, code):
            return _testlimitedcapi.PyImport_ExecCodeModuleEx(name, code,
                                                              pathname)
        self.check_executecodemodule(execute_code2, pathname)

    def check_executecode_pathnames(self, execute_code_func):
        # Test non-NULL pathname and NULL cpathname
        pathname = os.path.abspath('pathname')

        def execute_code1(name, code):
            return execute_code_func(name, code, pathname, NULL)
        self.check_executecodemodule(execute_code1, pathname)

        # Test NULL pathname and non-NULL cpathname
        pyc_filename = importlib.util.cache_from_source(__file__)
        py_filename = importlib.util.source_from_cache(pyc_filename)

        def execute_code2(name, code):
            return execute_code_func(name, code, NULL, pyc_filename)
        self.check_executecodemodule(execute_code2, py_filename)

    def test_executecodemodulewithpathnames(self):
        # Test PyImport_ExecCodeModuleWithPathnames()
        self.check_executecode_pathnames(_testlimitedcapi.PyImport_ExecCodeModuleWithPathnames)

    def test_executecodemoduleobject(self):
        # Test PyImport_ExecCodeModuleObject()
        self.check_executecode_pathnames(_testlimitedcapi.PyImport_ExecCodeModuleObject)

    # TODO: test PyImport_GetImporter()
    # TODO: test PyImport_ReloadModule()
    # TODO: test PyImport_ExtendInittab()
    # PyImport_AppendInittab() is tested by test_embed


if __name__ == "__main__":
    unittest.main()
