import unittest
import types
from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')


class FakeSpec:
    name = 'testmod'

DEF_SLOTS = (
    'Py_mod_name', 'Py_mod_doc', 'Py_mod_size', 'Py_mod_methods',
    'Py_mod_traverse', 'Py_mod_clear', 'Py_mod_free', 'Py_mod_token',
)


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

    def test_methods(self):
        mod = _testcapi.module_from_slots_methods(FakeSpec())
        self.assertIsInstance(mod, types.ModuleType)
        self.assertEqual(mod.__name__, 'testmod')
        self.assertEqual(mod.__doc__, None)
        self.assertEqual(mod.a_method(456), (mod, 456))

    def test_gc(self):
        mod = _testcapi.module_from_slots_gc(FakeSpec())
        self.assertIsInstance(mod, types.ModuleType)
        self.assertEqual(mod.__name__, 'testmod')
        self.assertEqual(mod.__doc__, None)

    def test_token(self):
        mod = _testcapi.module_from_slots_token(FakeSpec())
        self.assertIsInstance(mod, types.ModuleType)
        self.assertEqual(mod.__name__, 'testmod')
        self.assertEqual(mod.__doc__, None)

    def test_exec(self):
        mod = _testcapi.module_from_slots_exec(FakeSpec())
        self.assertIsInstance(mod, types.ModuleType)
        self.assertEqual(mod.__name__, 'testmod')
        self.assertEqual(mod.__doc__, None)
        self.assertEqual(mod.a_number, 456)

    def test_create(self):
        spec = FakeSpec()
        spec._gimme_this = "not a module object"
        mod = _testcapi.module_from_slots_create(spec)
        self.assertIsInstance(mod, str)
        self.assertEqual(mod, "not a module object")

    def test_def_slot(self):
        """Slots that replace PyModuleDef fields can't be used with PyModuleDef
        """
        for name in DEF_SLOTS:
            with self.subTest(name):
                spec = FakeSpec()
                spec._test_slot_id = getattr(_testcapi, name)
                with self.assertRaises(SystemError) as cm:
                    _testcapi.module_from_def_slot(spec)
                self.assertIn(name, str(cm.exception))
                self.assertIn("PyModuleDef", str(cm.exception))

    def test_repeated_def_slot(self):
        """Slots that replace PyModuleDef fields can't be repeated"""
        for name in (*DEF_SLOTS, 'Py_mod_exec'):
            with self.subTest(name):
                spec = FakeSpec()
                spec._test_slot_id = getattr(_testcapi, name)
                with self.assertRaises(SystemError) as cm:
                    _testcapi.module_from_slots_repeat_slot(spec)
                self.assertIn(name, str(cm.exception))
                self.assertIn("more than one", str(cm.exception))

    def test_null_def_slot(self):
        """Slots that replace PyModuleDef fields can't be NULL"""
        for name in (*DEF_SLOTS, 'Py_mod_exec'):
            with self.subTest(name):
                spec = FakeSpec()
                spec._test_slot_id = getattr(_testcapi, name)
                with self.assertRaises(SystemError) as cm:
                    _testcapi.module_from_slots_null_slot(spec)
                self.assertIn(name, str(cm.exception))
                self.assertIn("NULL", str(cm.exception))
