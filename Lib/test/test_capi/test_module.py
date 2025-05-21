import unittest
import types
from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')


class TestModFromSlotsAndSpec(unittest.TestCase):
    def test_empty(self):
        spec_like = types.SimpleNamespace(
            name='testmod',
        )
        mod = _testcapi.module_from_slots_and_spec([], spec_like)
        self.assertIsInstance(mod, types.ModuleType)
        self.assertEqual(mod.__name__, 'testmod')
