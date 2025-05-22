import unittest
import types
from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')


class FakeSpec:
    name = 'testmod'


class TestModFromSlotsAndSpec(unittest.TestCase):
    def test_empty(self):
        mod = _testcapi.module_from_slots_empty(FakeSpec())
        self.assertIsInstance(mod, types.ModuleType)
        self.assertEqual(mod.__name__, 'testmod')

    def test_name(self):
        # Py_mod_name (and PyModuleDef.m_name) are currently ignored when
        # spec is given.
        # We still test that it's accepted.
        mod = _testcapi.module_from_slots_name(FakeSpec())
        self.assertIsInstance(mod, types.ModuleType)
        self.assertEqual(mod.__name__, 'testmod')
