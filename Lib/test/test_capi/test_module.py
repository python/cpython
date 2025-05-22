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
        self.assertEqual(mod.__doc__, None)

    def test_doc(self):
        mod = _testcapi.module_from_slots_doc(FakeSpec())
        self.assertIsInstance(mod, types.ModuleType)
        self.assertEqual(mod.__name__, 'testmod')
        self.assertEqual(mod.__doc__, 'the docstring')

    def test_size(self):
        mod = _testcapi.module_from_slots_size(FakeSpec())
        self.assertIsInstance(mod, types.ModuleType)
        self.assertEqual(mod.__name__, 'testmod')
        self.assertEqual(mod.__doc__, None)
        self.assertEqual(mod.size, 123)

    def test_def_name(self):
        with self.assertRaises(SystemError) as cm:
            _testcapi.module_from_def_name(FakeSpec())
        self.assertIn("Py_mod_name", str(cm.exception),)
        self.assertIn("PyModuleDef", str(cm.exception), )

    def test_repeated_new_slot(self):
        for name in 'Py_mod_name', 'Py_mod_doc':
            with self.subTest(name):
                spec = FakeSpec()
                spec._test_slot_id = getattr(_testcapi, name)
                with self.assertRaises(SystemError) as cm:
                    _testcapi.module_from_slots_repeat_slot(spec)
                self.assertIn(name, str(cm.exception),)
                self.assertIn("repeated", str(cm.exception), )

    def test_null_name(self):
        for name in 'Py_mod_name', 'Py_mod_doc':
            with self.subTest(name):
                spec = FakeSpec()
                spec._test_slot_id = getattr(_testcapi, name)
                with self.assertRaises(SystemError) as cm:
                    _testcapi.module_from_slots_null_slot(spec)
                self.assertIn(name, str(cm.exception),)
                self.assertIn("NULL", str(cm.exception), )
