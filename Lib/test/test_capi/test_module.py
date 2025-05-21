import unittest
import types
from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')


class TestModExport(unittest.TestCase):
    def test_modexport(self):
        mod = _testcapi.module_from_slots_and_spec([])
        self.assertIsInstance(mod, types.ModuleType)
