"""Tests for calling C functions through mis-cast function pointers.

Extension modules frequently cast C functions with the wrong number of
arguments to PyCFunction, getter, setter or ternaryfunc. Native ABIs tolerate
this; on WebAssembly, call_indirect checks the signature, so CPython routes
these calls through a trampoline (Python/wasm_trampoline.c) that detects the
real signature. Calling each variant must work everywhere.
"""
import unittest
from test.support import check_sanitizer, import_helper, is_wasm32

_testcapi = import_helper.import_module('_testcapi')


# Native ABIs tolerate these calls but they are implementation-defined behavior.
# -fsanitize=undefined correctly trips on them.
@unittest.skipIf(check_sanitizer(ub=True),
                 "calls through mis-cast function pointers are UB natively")
class FpcastTest(unittest.TestCase):
    def check_calls(self, obj, prefix):
        for arity in range(4):
            with self.subTest(kind="noargs", arity=arity):
                self.assertIsNone(getattr(obj, f"{prefix}noargs{arity}")())
            with self.subTest(kind="o", arity=arity):
                self.assertIsNone(getattr(obj, f"{prefix}o{arity}")(1))
            with self.subTest(kind="varargs", arity=arity):
                self.assertIsNone(getattr(obj, f"{prefix}varargs{arity}")())
                self.assertIsNone(getattr(obj, f"{prefix}varargs{arity}")(1, 2))
            with self.subTest(kind="kwargs", arity=arity):
                self.assertIsNone(getattr(obj, f"{prefix}kwargs{arity}")())
                self.assertIsNone(getattr(obj, f"{prefix}kwargs{arity}")(1, x=2))

    def test_module_functions(self):
        self.check_calls(_testcapi, "fpcast_")

    def test_methods(self):
        self.check_calls(_testcapi.FpcastTestType(), "")

    def test_tp_call(self):
        for arity in range(4):
            with self.subTest(arity=arity):
                cls = getattr(_testcapi, f"FpcastCallable{arity}")
                self.assertIsNone(cls()())
                self.assertIsNone(cls()(1, x=2))

    def test_getset(self):
        t = _testcapi.FpcastTestType()
        self.assertIsNone(t.getset0)
        self.assertIsNone(t.getset1)
        self.assertIsNone(t.getset2)
        t.getset1 = 5
        sentinel = object()
        t.getset2 = sentinel
        self.assertIs(_testcapi.fpcast_last_set_value(), sentinel)
        with self.assertRaises(AttributeError):
            t.getset0 = 1

    @unittest.skipUnless(is_wasm32, "requires the wasm call trampoline")
    def test_unsupported_signature(self):
        # A function with four pointer arguments matches none of the
        # signatures the trampoline knows about: it must raise SystemError
        # rather than trap.
        t = _testcapi.FpcastTestType()
        with self.assertRaises(SystemError):
            _testcapi.fpcast_noargs4()
        with self.assertRaises(SystemError):
            t.noargs4()
        with self.assertRaises(SystemError):
            _testcapi.FpcastCallable4()()
        with self.assertRaises(SystemError):
            t.getset4
        with self.assertRaises(SystemError):
            t.getset4 = 1


if __name__ == "__main__":
    unittest.main()
