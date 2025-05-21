import unittest
import importlib
import sys
from importlib.machinery import ExtensionFileLoader

try:
    import _testmultiphase
except ImportError:
    _testmultiphase = None

def import_extension_from_file(modname, filename, *, put_in_sys_modules=True):
    loader = ExtensionFileLoader(modname, filename)
    spec = importlib.util.spec_from_loader(modname, loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    if put_in_sys_modules:
        sys.modules[modname] = module
    return module


class TestModExport(unittest.TestCase):
    @unittest.skipIf(_testmultiphase is None, "test requires _testmultiphase module")
    def test_modexport(self):
        modname = '_test_modexport'
        filename = _testmultiphase.__file__
        module = import_extension_from_file(modname, filename,
                                            put_in_sys_modules=False)

        self.assertEqual(module.__name__, modname)
