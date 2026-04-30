from test.support import import_helper, subTests
import contextlib
import unittest
import types
import sys

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
_testcapi = import_helper.import_module('_testcapi')

class FakeSpec:
    name = 'module'


# See Modules/_testlimitedcapi/slots.c for the definitions.
# This module is full of "magic constants" which simply need to match
# between the C and Python part of the tests.

class TypeSlotsTests(unittest.TestCase):
    def test_basic_type_slots(self):
        cls = _testlimitedcapi.type_from_slots("basic")
        self.assertEqual(cls.__name__, "MyType")

        # Py_TPFLAGS_IMMUTABLETYPE is *not* set
        cls.attr = 123

        # Py_TPFLAGS_BASETYPE is *not* set
        with self.assertRaises(TypeError):
            class Sub(cls):
                pass

    def test_mod_slot_in_type(self):
        with self.assertRaisesRegex(SystemError, "invalid.* 100 .*Py_mod_name"):
            _testlimitedcapi.type_from_slots("foreign_slot")

    def test_size_slots(self):
        cls = _testlimitedcapi.type_from_slots("basicsize")
        self.assertGreaterEqual(cls.__basicsize__, 256)

        cls = _testlimitedcapi.type_from_slots("extra_basicsize")
        self.assertGreaterEqual(cls.__basicsize__, object.__basicsize__ + 256)

        cls = _testlimitedcapi.type_from_slots("itemsize")
        self.assertGreaterEqual(cls.__itemsize__, 16)

    def test_flag_slots(self):
        cls = _testlimitedcapi.type_from_slots("flags")
        with self.assertRaises(TypeError):
            # Py_TPFLAGS_IMMUTABLETYPE is set
            cls.attr = 123
        class Sub(cls):
            # Py_TPFLAGS_BASETYPE is set
            pass

    def test_func_slots(self):
        cls = _testlimitedcapi.type_from_slots("matmul_123")
        self.assertEqual(cls() @ None, 123)

    def test_optional_end(self):
        with self.assertRaisesRegex(SystemError, "invalid flags.*Py_slot_end"):
            cls = _testlimitedcapi.type_from_slots("optional_end")

    def test_invalid(self):
        with self.assertRaisesRegex(SystemError, "Py_slot_invalid"):
            cls = _testlimitedcapi.type_from_slots("invalid")
        with self.assertRaisesRegex(SystemError, f"slot ID {0xfbad}"):
            cls = _testlimitedcapi.type_from_slots("invalid_fbad")

        cls = _testlimitedcapi.type_from_slots("optional_invalid")
        self.assertGreaterEqual(cls.__basicsize__, object.__basicsize__ + 256)

        cls = _testlimitedcapi.type_from_slots("optional_invalid_fbad")
        self.assertGreaterEqual(cls.__basicsize__, object.__basicsize__ + 256)

    @subTests("case_name", ["old_slot_numbers", "new_slot_numbers"])
    def test_compat_slot_numbers(self, case_name):
        cls = _testlimitedcapi.type_from_slots(case_name)
        obj = cls()

        # Py_bf_getbuffer (1), Py_bf_releasebuffer (2)
        self.assertEqual(obj.buf_counter, 0)
        mem = memoryview(obj)
        self.assertEqual(bytes(mem), b"buf\0")
        self.assertEqual(obj.buf_counter, 1)
        mem.release()
        self.assertEqual(obj.buf_counter, 0)

        # Py_mp_ass_subscript (3)
        with self.assertRaises(KeyError):
            obj["key"] = "value"

        # Py_mp_length (4)
        self.assertEqual(len(obj), 456)

    def test_nonstatic_tp_members(self):
        with self.assertRaisesRegex(SystemError, "Py_tp_members.*STATIC"):
            _testlimitedcapi.type_from_slots("nonstatic_tp_members")

    def test_intptr_flags(self):
        cls = _testlimitedcapi.type_from_slots("intptr_flags_macro")
        with self.assertRaises(TypeError):
            # Py_TPFLAGS_IMMUTABLETYPE is set
            cls.attr = 123

        cls = _testlimitedcapi.type_from_slots("intptr_flags_struct")
        with self.assertRaises(TypeError):
            # Py_TPFLAGS_IMMUTABLETYPE is set
            cls.attr = 123

        cls = _testlimitedcapi.type_from_slots("intptr_static")
        cls.attribute = 123

    def test_nested(self):
        cls = _testlimitedcapi.type_from_slots("nested")
        self.assertEqual(cls() + 1, 123)
        self.assertEqual(cls() - 1, 234)

        cls = _testlimitedcapi.type_from_slots("nested_max")
        self.assertEqual(cls() + 1, 123)
        self.assertEqual(cls() - 1, 234)
        self.assertEqual(cls() * 1, 345)
        self.assertEqual(cls() / 1, 456)
        self.assertEqual(cls() % 1, 567)

        with self.assertRaisesRegex(SystemError, "too many levels"):
            _testlimitedcapi.type_from_slots("nested_over_limit")

        cls = _testlimitedcapi.type_from_slots("nested_old")
        self.assertEqual(cls() + 1, 123)
        self.assertEqual(cls() - 1, 234)

        cls = _testlimitedcapi.type_from_slots("nested_old_max")
        self.assertEqual(cls() + 1, 123)
        self.assertEqual(cls() - 1, 234)
        self.assertEqual(cls() * 1, 345)
        self.assertEqual(cls() / 1, 456)
        self.assertEqual(cls() % 1, 567)

        with self.assertRaisesRegex(SystemError, "too many levels"):
            _testlimitedcapi.type_from_slots("nested_old_over_limit")

        cls = _testlimitedcapi.type_from_slots("nested_pingpong")
        self.assertEqual(cls() + 1, 123)
        self.assertEqual(cls() - 1, 234)
        self.assertEqual(cls() * 1, 345)
        self.assertEqual(cls() / 1, 456)
        self.assertEqual(cls() % 1, 567)

    # Slot names aren't exposed to Python yet; see Include/slots_generated.h
    # for the definitions.

    @subTests("slot_number", [
        *range(1, 83),      # Original slots
        *range(88, 92),     # New compat slot values
        *range(95, 99),     # Slots for PyType_Spec fields
        *range(107, 109),   # Slots for PyType_FromMetaclass args
    ])
    def test_null_slot_handling(self, slot_number):
        if slot_number == 56:
            # Py_tp_doc
            return
        elif slot_number == 72 or slot_number >= 95:
            # Py_tp_members; all new slots
            ctx = self.assertRaisesRegex(
                SystemError, "NULL not allowed|must be positive")
            ctx_old = ctx
        else:
            ctx = self.assertWarnsRegex(DeprecationWarning, "NULL")
            ctx_old = contextlib.nullcontext()
        with ctx:
            _testlimitedcapi.type_from_null_slot(slot_number)
        if slot_number < 95:
            with ctx_old:
                _testlimitedcapi.type_from_null_spec_slot(slot_number)

    def test_repeat_warning(self):
        with self.assertWarnsRegex(DeprecationWarning, "multiple"):
            cls = _testlimitedcapi.type_from_slots("repeat_add")
        self.assertEqual(cls() + 1, 456)

    def test_repeat_error(self):
        with self.assertRaisesRegex(SystemError, "multiple"):
            cls = _testlimitedcapi.type_from_slots("repeat_module")

class ModuleSlotsTests(unittest.TestCase):
    def test_basic_module_slots(self):
        mod = _testlimitedcapi.module_from_slots("basic", FakeSpec())
        self.assertIsInstance(mod, types.ModuleType)

    def test_type_slot_in_module(self):
        with self.assertRaisesRegex(SystemError, "invalid.* 95 .*Py_tp_name"):
            _testlimitedcapi.module_from_slots("foreign_slot", FakeSpec())

    def test_size_slots(self):
        mod = _testlimitedcapi.module_from_slots("state_size", FakeSpec())
        self.assertEqual(mod.state_size, 42)

    def test_flag_slots(self):
        mod = _testlimitedcapi.module_from_slots("multi_interp", FakeSpec())

    def test_exec_slot(self):
        mod = _testlimitedcapi.module_from_slots("exec", FakeSpec())
        self.assertEqual(mod.exec_done, "yes")

    def test_optional_end(self):
        with self.assertRaisesRegex(SystemError, "invalid flags.*Py_slot_end"):
            _testlimitedcapi.module_from_slots("optional_end", FakeSpec())

    def test_invalid(self):
        with self.assertRaisesRegex(SystemError, "Py_slot_invalid"):
            _testlimitedcapi.module_from_slots("invalid", FakeSpec())
        with self.assertRaisesRegex(SystemError, f"slot ID {0xfbad}"):
            _testlimitedcapi.module_from_slots("invalid_fbad", FakeSpec())

        mod = _testlimitedcapi.module_from_slots("optional_invalid", FakeSpec())
        self.assertEqual(mod.exec_done, "yes")

        mod = _testlimitedcapi.module_from_slots("optional_invalid_fbad", FakeSpec())
        self.assertEqual(mod.exec_done, "yes")

    @subTests("case_name", ["old_slot_numbers", "new_slot_numbers"])
    def test_compat_slot_numbers(self, case_name):
        mod = _testlimitedcapi.module_from_slots(case_name, FakeSpec())
        self.assertEqual(mod.exec_done, "yes")

    @subTests("case_name", ["old_slot_number_create", "new_slot_number_create"])
    def test_compat_slot_number_create(self, case_name):
        spec = FakeSpec()
        mod = _testlimitedcapi.module_from_slots(case_name, spec)
        self.assertIs(mod, spec)

    @subTests("slot_number", [4, 87])
    def test_compat_slot_number_gil(self, slot_number):
        spec = FakeSpec()
        gil_enabled = sys._is_gil_enabled()
        mod = _testlimitedcapi.module_from_gil_slot(slot_number, spec)
        self.assertEqual(gil_enabled, sys._is_gil_enabled())

    def test_nonstatic_mod_methods(self):
        with self.assertRaisesRegex(SystemError, "Py_mod_methods.*STATIC"):
            _testlimitedcapi.module_from_slots("nonstatic_mod_methods",
                                               FakeSpec())

    def test_intptr_methods(self):
        mod = _testlimitedcapi.module_from_slots("intptr_methods",
                                                 FakeSpec())
        self.assertEqual(mod.type_from_slots.__name__, "type_from_slots")

    def test_nested(self):
        mod = _testlimitedcapi.module_from_slots("nested", FakeSpec())
        self.assertEqual(mod.exec_done, "yes")
        self.assertEqual(mod.__doc__, "doc")

        mod = _testlimitedcapi.module_from_slots("nested_max", FakeSpec())
        self.assertEqual(mod.exec_done, "yes")
        self.assertEqual(mod.state_size, 53)
        self.assertEqual(mod.__doc__, "doc")

        with self.assertRaisesRegex(SystemError, "too many levels"):
            _testlimitedcapi.module_from_slots("nested_over_limit", FakeSpec())

        mod = _testlimitedcapi.module_from_slots("nested_old", FakeSpec())
        self.assertEqual(mod.exec_done, "yes")
        self.assertEqual(mod.__doc__, "doc")

        mod = _testlimitedcapi.module_from_slots("nested_old_max", FakeSpec())
        self.assertEqual(mod.exec_done, "yes")
        self.assertEqual(mod.state_size, 53)
        self.assertEqual(mod.__doc__, "doc")

        with self.assertRaisesRegex(SystemError, "too many levels"):
            _testlimitedcapi.module_from_slots("nested_old_over_limit", FakeSpec())

        mod = _testlimitedcapi.module_from_slots("nested_pingpong", FakeSpec())
        self.assertEqual(mod.exec_done, "yes")
        self.assertEqual(mod.state_size, 53)
        self.assertEqual(mod.__doc__, "doc")

    def test_nested_nonstatic_from_def(self):
        with self.assertRaisesRegex(SystemError, "must be static"):
            _testcapi.module_from_def_nonstatic_nested(FakeSpec())

    # Slot names aren't exposed to Python yet; see Include/slots_generated.h
    # for the definitions.

    @subTests("slot_number", [
        *range(1, 5),       # Old compat slot values
        *range(84, 88),     # New compat slot values
        *range(100, 107),   # Slots for PyModuleDef fields
        *range(109, 111),   # Slots new in 3.15
    ])
    def test_null_slot_handling(self, slot_number):
        if slot_number in {3, 86, 4, 87, 102}:
            # Py_mod_mult.interp., Py_mod_gil, Py_mod_state_size
            return
        elif slot_number in {2, 85} or slot_number > 85:
            # Py_mod_exec, new slots
            ctx = self.assertRaisesRegex(SystemError, "NULL not allowed")
            ctx_old = ctx
        else:
            ctx = self.assertWarnsRegex(DeprecationWarning, "NULL")
            ctx_old = contextlib.nullcontext()
        with ctx:
            _testlimitedcapi.module_from_null_slot(slot_number, FakeSpec())
        with ctx_old:
            _testcapi.module_from_null_def_slot(slot_number,
                                                FakeSpec())

    def test_repeat_error(self):
        with self.assertRaisesRegex(SystemError, "multiple"):
            _testlimitedcapi.module_from_slots("repeat_create", FakeSpec())
        with self.assertRaisesRegex(SystemError, "multiple"):
            _testlimitedcapi.module_from_slots("repeat_exec", FakeSpec())
        with self.assertRaisesRegex(SystemError, "multiple"):
            _testlimitedcapi.module_from_slots("repeat_gil", FakeSpec())
