import types
import unittest
from test.test_importlib import util

machinery = util.import_importlib('importlib.machinery')

from test.test_importlib.extension.test_loader import MultiPhaseExtensionModuleTests


class NonModuleExtensionTests:
    setUp = MultiPhaseExtensionModuleTests.setUp
    load_module_by_name = MultiPhaseExtensionModuleTests.load_module_by_name

    def _test_nonmodule(self):
        # Test returning a non-module object from create works.
        name = self.name + '_nonmodule'
        mod = self.load_module_by_name(name)
        self.assertNotEqual(type(mod), type(unittest))
        self.assertEqual(mod.three, 3)

    # issue 27782
    def test_nonmodule_with_methods(self):
        # Test creating a non-module object with methods defined.
        name = self.name + '_nonmodule_with_methods'
        mod = self.load_module_by_name(name)
        self.assertNotEqual(type(mod), type(unittest))
        self.assertEqual(mod.three, 3)
        self.assertEqual(mod.bar(10, 1), 9)

    def test_null_slots(self):
        # Test that NULL slots aren't a problem.
        name = self.name + '_null_slots'
        module = self.load_module_by_name(name)
        self.assertIsInstance(module, types.ModuleType)
        self.assertEqual(module.__name__, name)


(Frozen_NonModuleExtensionTests,
 Source_NonModuleExtensionTests
 ) = util.test_both(NonModuleExtensionTests, machinery=machinery)


if __name__ == '__main__':
    unittest.main()
