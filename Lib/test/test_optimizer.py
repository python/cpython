import unittest
import types
from test.support import import_helper


_testinternalcapi = import_helper.import_module("_testinternalcapi")


class TestRareEventCounters(unittest.TestCase):
    def setUp(self):
        _testinternalcapi.reset_rare_event_counters()

    def test_set_class(self):
        class A:
            pass
        class B:
            pass
        a = A()

        orig_counter = _testinternalcapi.get_rare_event_counters()["set_class"]
        a.__class__ = B
        self.assertEqual(
            orig_counter + 1,
            _testinternalcapi.get_rare_event_counters()["set_class"]
        )

    def test_set_bases(self):
        class A:
            pass
        class B:
            pass
        class C(B):
            pass

        orig_counter = _testinternalcapi.get_rare_event_counters()["set_bases"]
        C.__bases__ = (A,)
        self.assertEqual(
            orig_counter + 1,
            _testinternalcapi.get_rare_event_counters()["set_bases"]
        )

    def test_set_eval_frame_func(self):
        orig_counter = _testinternalcapi.get_rare_event_counters()["set_eval_frame_func"]
        _testinternalcapi.set_eval_frame_record([])
        self.assertEqual(
            orig_counter + 1,
            _testinternalcapi.get_rare_event_counters()["set_eval_frame_func"]
        )
        _testinternalcapi.set_eval_frame_default()

    def test_builtin_dict(self):
        orig_counter = _testinternalcapi.get_rare_event_counters()["builtin_dict"]
        if isinstance(__builtins__, types.ModuleType):
            builtins = __builtins__.__dict__
        else:
            builtins = __builtins__
        builtins["FOO"] = 42
        self.assertEqual(
            orig_counter + 1,
            _testinternalcapi.get_rare_event_counters()["builtin_dict"]
        )
        del builtins["FOO"]

    def test_func_modification(self):
        def func(x=0):
            pass

        for attribute in (
            "__code__",
            "__defaults__",
            "__kwdefaults__"
        ):
            orig_counter = _testinternalcapi.get_rare_event_counters()["func_modification"]
            setattr(func, attribute, getattr(func, attribute))
            self.assertEqual(
                orig_counter + 1,
                _testinternalcapi.get_rare_event_counters()["func_modification"]
            )


class TestOptimizerSymbols(unittest.TestCase):

    @unittest.skipUnless(hasattr(_testinternalcapi, "uop_symbols_test"),
                "requires _testinternalcapi.uop_symbols_test")
    def test_optimizer_symbols(self):
        _testinternalcapi.uop_symbols_test()


if __name__ == "__main__":
    unittest.main()
