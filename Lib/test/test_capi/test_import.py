import importlib.util
import os.path
import sys
import types
import unittest
from test.support import os_helper
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
                old_module = sys.modules[name]
                module = import_module(name)
                self.assertIsInstance(module, types.ModuleType)
                self.assertIs(module, old_module)

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
        getmodule = _testlimitedcapi.PyImport_GetModule
        self.check_import_loaded_module(getmodule)

        nonexistent = 'nonexistent'
        self.assertNotIn(nonexistent, sys.modules)
        self.assertIs(getmodule(nonexistent), KeyError)
        self.assertIs(getmodule(''), KeyError)
        self.assertIs(getmodule(object()), KeyError)

        self.assertRaises(TypeError, getmodule, [])  # unhashable
        # CRASHES getmodule(NULL)

    def check_addmodule(self, add_module, accept_nonstr=False):
        # create a new module
        names = ['nonexistent']
        if accept_nonstr:
            names.append(b'\xff')  # non-UTF-8
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
        addmoduleobject = _testlimitedcapi.PyImport_AddModuleObject
        self.check_addmodule(addmoduleobject, accept_nonstr=True)

        self.assertRaises(TypeError, addmoduleobject, [])  # unhashable
        # CRASHES addmoduleobject(NULL)

    def test_addmodule(self):
        # Test PyImport_AddModule()
        addmodule = _testlimitedcapi.PyImport_AddModule
        self.check_addmodule(addmodule)

        self.assertRaises(UnicodeDecodeError, addmodule, b'\xff')
        # CRASHES addmodule(NULL)

    def test_addmoduleref(self):
        # Test PyImport_AddModuleRef()
        addmoduleref = _testlimitedcapi.PyImport_AddModuleRef
        self.check_addmodule(addmoduleref)

        self.assertRaises(UnicodeDecodeError, addmoduleref, b'\xff')
        # CRASHES addmoduleref(NULL)

    def check_import_func(self, import_module):
        self.check_import_loaded_module(import_module)
        self.check_import_fresh_module(import_module)
        self.assertRaises(ModuleNotFoundError, import_module, 'nonexistent')
        self.assertRaises(ValueError, import_module, '')

    def test_import(self):
        # Test PyImport_Import()
        import_ = _testlimitedcapi.PyImport_Import
        self.check_import_func(import_)

        self.assertRaises(TypeError, import_, b'os')
        self.assertRaises(SystemError, import_, NULL)

    def test_importmodule(self):
        # Test PyImport_ImportModule()
        importmodule = _testlimitedcapi.PyImport_ImportModule
        self.check_import_func(importmodule)

        self.assertRaises(UnicodeDecodeError, importmodule, b'\xff')
        # CRASHES importmodule(NULL)

    def test_importmodulenoblock(self):
        # Test deprecated PyImport_ImportModuleNoBlock()
        importmodulenoblock = _testlimitedcapi.PyImport_ImportModuleNoBlock
        with check_warnings(('', DeprecationWarning)):
            self.check_import_func(importmodulenoblock)
            self.assertRaises(UnicodeDecodeError, importmodulenoblock, b'\xff')

        # CRASHES importmodulenoblock(NULL)

    def check_frozen_import(self, import_frozen_module):
        # Importing a frozen module executes its code, so start by unloading
        # the module to execute the code in a new (temporary) module.
        old_zipimport = sys.modules.pop('zipimport')
        try:
            self.assertEqual(import_frozen_module('zipimport'), 1)

            # import zipimport again
            self.assertEqual(import_frozen_module('zipimport'), 1)
        finally:
            sys.modules['zipimport'] = old_zipimport

        # not a frozen module
        self.assertEqual(import_frozen_module('sys'), 0)
        self.assertEqual(import_frozen_module('nonexistent'), 0)
        self.assertEqual(import_frozen_module(''), 0)

    def test_importfrozenmodule(self):
        # Test PyImport_ImportFrozenModule()
        importfrozenmodule = _testlimitedcapi.PyImport_ImportFrozenModule
        self.check_frozen_import(importfrozenmodule)

        self.assertRaises(UnicodeDecodeError, importfrozenmodule, b'\xff')
        # CRASHES importfrozenmodule(NULL)

    def test_importfrozenmoduleobject(self):
        # Test PyImport_ImportFrozenModuleObject()
        importfrozenmoduleobject = _testlimitedcapi.PyImport_ImportFrozenModuleObject
        self.check_frozen_import(importfrozenmoduleobject)
        self.assertEqual(importfrozenmoduleobject(b'zipimport'), 0)
        self.assertEqual(importfrozenmoduleobject(NULL), 0)

    def test_importmoduleex(self):
        # Test PyImport_ImportModuleEx()
        importmoduleex = _testlimitedcapi.PyImport_ImportModuleEx
        self.check_import_func(lambda name: importmoduleex(name, NULL, NULL, NULL))

        self.assertRaises(ModuleNotFoundError, importmoduleex, 'nonexistent', NULL, NULL, NULL)
        self.assertRaises(ValueError, importmoduleex, '', NULL, NULL, NULL)
        self.assertRaises(UnicodeDecodeError, importmoduleex, b'\xff', NULL, NULL, NULL)
        # CRASHES importmoduleex(NULL, NULL, NULL, NULL)

    def check_importmodulelevel(self, importmodulelevel):
        self.check_import_func(lambda name: importmodulelevel(name, NULL, NULL, NULL, 0))

        self.assertRaises(ModuleNotFoundError, importmodulelevel, 'nonexistent', NULL, NULL, NULL, 0)
        self.assertRaises(ValueError, importmodulelevel, '', NULL, NULL, NULL, 0)

        if __package__:
            self.assertIs(importmodulelevel('test_import', globals(), NULL, NULL, 1),
                          sys.modules['test.test_capi.test_import'])
            self.assertIs(importmodulelevel('test_capi', globals(), NULL, NULL, 2),
                          sys.modules['test.test_capi'])
        self.assertRaises(ValueError, importmodulelevel, 'os', NULL, NULL, NULL, -1)
        with self.assertWarns(ImportWarning):
            self.assertRaises(KeyError, importmodulelevel, 'test_import', {}, NULL, NULL, 1)
        self.assertRaises(TypeError, importmodulelevel, 'test_import', [], NULL, NULL, 1)

    def test_importmodulelevel(self):
        # Test PyImport_ImportModuleLevel()
        importmodulelevel = _testlimitedcapi.PyImport_ImportModuleLevel
        self.check_importmodulelevel(importmodulelevel)

        self.assertRaises(UnicodeDecodeError, importmodulelevel, b'\xff', NULL, NULL, NULL, 0)
        # CRASHES importmodulelevel(NULL, NULL, NULL, NULL, 0)

    def test_importmodulelevelobject(self):
        # Test PyImport_ImportModuleLevelObject()
        importmodulelevel = _testlimitedcapi.PyImport_ImportModuleLevelObject
        self.check_importmodulelevel(importmodulelevel)

        self.assertRaises(TypeError, importmodulelevel, b'os', NULL, NULL, NULL, 0)
        self.assertRaises(ValueError, importmodulelevel, NULL, NULL, NULL, NULL, 0)

    def check_executecodemodule(self, execute_code, *args):
        name = 'test_import_executecode'
        try:
            # Create a temporary module where the code will be executed
            self.assertNotIn(name, sys.modules)
            module = _testlimitedcapi.PyImport_AddModuleRef(name)
            self.assertFalse(hasattr(module, 'attr'))

            # Execute the code
            code = compile('attr = 1', '<test>', 'exec')
            module2 = execute_code(name, code, *args)
            self.assertIs(module2, module)

            # Check the function side effects
            self.assertEqual(module.attr, 1)
        finally:
            sys.modules.pop(name, None)
        return module.__spec__.origin

    def test_executecodemodule(self):
        # Test PyImport_ExecCodeModule()
        execcodemodule = _testlimitedcapi.PyImport_ExecCodeModule
        self.check_executecodemodule(execcodemodule)

        code = compile('attr = 1', '<test>', 'exec')
        self.assertRaises(UnicodeDecodeError, execcodemodule, b'\xff', code)
        # CRASHES execcodemodule(NULL, code)
        # CRASHES execcodemodule(name, NULL)

    def test_executecodemoduleex(self):
        # Test PyImport_ExecCodeModuleEx()
        execcodemoduleex = _testlimitedcapi.PyImport_ExecCodeModuleEx

        # Test NULL path (it should not crash)
        self.check_executecodemodule(execcodemoduleex, NULL)

        # Test non-NULL path
        pathname = b'pathname'
        origin = self.check_executecodemodule(execcodemoduleex, pathname)
        self.assertEqual(origin, os.path.abspath(os.fsdecode(pathname)))

        pathname = os_helper.TESTFN_UNDECODABLE
        if pathname:
            origin = self.check_executecodemodule(execcodemoduleex, pathname)
            self.assertEqual(origin, os.path.abspath(os.fsdecode(pathname)))

        code = compile('attr = 1', '<test>', 'exec')
        self.assertRaises(UnicodeDecodeError, execcodemoduleex, b'\xff', code, NULL)
        # CRASHES execcodemoduleex(NULL, code, NULL)
        # CRASHES execcodemoduleex(name, NULL, NULL)

    def check_executecode_pathnames(self, execute_code_func, object=False):
        # Test non-NULL pathname and NULL cpathname

        # Test NULL paths (it should not crash)
        self.check_executecodemodule(execute_code_func, NULL, NULL)

        pathname = 'pathname'
        origin = self.check_executecodemodule(execute_code_func, pathname, NULL)
        self.assertEqual(origin, os.path.abspath(os.fsdecode(pathname)))
        origin = self.check_executecodemodule(execute_code_func, NULL, pathname)
        if not object:
            self.assertEqual(origin, os.path.abspath(os.fsdecode(pathname)))

        pathname = os_helper.TESTFN_UNDECODABLE
        if pathname:
            if object:
                pathname = os.fsdecode(pathname)
            origin = self.check_executecodemodule(execute_code_func, pathname, NULL)
            self.assertEqual(origin, os.path.abspath(os.fsdecode(pathname)))
            self.check_executecodemodule(execute_code_func, NULL, pathname)

        # Test NULL pathname and non-NULL cpathname
        pyc_filename = importlib.util.cache_from_source(__file__)
        py_filename = importlib.util.source_from_cache(pyc_filename)
        origin = self.check_executecodemodule(execute_code_func, NULL, pyc_filename)
        if not object:
            self.assertEqual(origin, py_filename)

    def test_executecodemodulewithpathnames(self):
        # Test PyImport_ExecCodeModuleWithPathnames()
        execute_code_func = _testlimitedcapi.PyImport_ExecCodeModuleWithPathnames
        self.check_executecode_pathnames(execute_code_func)

        code = compile('attr = 1', '<test>', 'exec')
        self.assertRaises(UnicodeDecodeError, execute_code_func, b'\xff', code, NULL, NULL)
        # CRASHES execute_code_func(NULL, code, NULL, NULL)
        # CRASHES execute_code_func(name, NULL, NULL, NULL)

    def test_executecodemoduleobject(self):
        # Test PyImport_ExecCodeModuleObject()
        execute_code_func = _testlimitedcapi.PyImport_ExecCodeModuleObject
        self.check_executecode_pathnames(execute_code_func, object=True)

        code = compile('attr = 1', '<test>', 'exec')
        self.assertRaises(TypeError, execute_code_func, [], code, NULL, NULL)
        # CRASHES execute_code_func(NULL, code, NULL, NULL)
        # CRASHES execute_code_func(name, NULL, NULL, NULL)

    # TODO: test PyImport_GetImporter()
    # TODO: test PyImport_ReloadModule()
    # TODO: test PyImport_ExtendInittab()
    # PyImport_AppendInittab() is tested by test_embed


if __name__ == "__main__":
    unittest.main()
