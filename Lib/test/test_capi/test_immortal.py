import unittest
from test.support import import_helper, Py_GIL_DISABLED, subTests
from test.support.script_helper import assert_python_ok

_testcapi = import_helper.import_module('_testcapi')
_testinternalcapi = import_helper.import_module('_testinternalcapi')


class TestUnstableCAPI(unittest.TestCase):
    def test_immortal(self):
        # Not extensive
        known_immortals = (True, False, None, 0, ())
        for immortal in known_immortals:
            with self.subTest(immortal=immortal):
                self.assertTrue(_testcapi.is_immortal(immortal))

        # Some arbitrary mutable objects
        non_immortals = (object(), self, [object()])
        for non_immortal in non_immortals:
            with self.subTest(non_immortal=non_immortal):
                self.assertFalse(_testcapi.is_immortal(non_immortal))

        # CRASHES _testcapi.is_immortal(NULL)


class TestInternalCAPI(unittest.TestCase):

    def test_immortal_builtins(self):
        for obj in range(-5, 1025):
            self.assertTrue(_testinternalcapi.is_static_immortal(obj))
        self.assertTrue(_testinternalcapi.is_static_immortal(None))
        self.assertTrue(_testinternalcapi.is_static_immortal(False))
        self.assertTrue(_testinternalcapi.is_static_immortal(True))
        self.assertTrue(_testinternalcapi.is_static_immortal(...))
        self.assertTrue(_testinternalcapi.is_static_immortal(()))
        for obj in range(1025, 1125):
            self.assertFalse(_testinternalcapi.is_static_immortal(obj))
        for obj in ([], {}, set()):
            self.assertFalse(_testinternalcapi.is_static_immortal(obj))


    @unittest.skipIf(Py_GIL_DISABLED,
                     "can't de-immortalize on free-threaded build")
    def test_deimmortalized_bool(self):
        # gh-158401: Old Stable ABI extensions de-immortalize objects,
        # which could change a bool's truth value.
        # Run in a subprocess, since other tests assume deimmortalization
        # doesn't happen.
        assert_python_ok("-c", f"""
            import sys
            import _testcapi
            import _testinternalcapi

            class C:
                def __init__(self):
                    self.flag = True

            def check_attr(o):
                if not o.flag:
                    raise RuntimeError("o.flag is True but tested false")
                return "ok"

            def get_true():
                return True

            def check_return():
                if not get_true():
                    raise RuntimeError("get_true() returned True but tested false")
                return "ok"

            o = C()
            for _ in range(1000):
                check_attr(o), check_return()

            _testinternalcapi.deimmortalize(True)
            check_attr(o)
            check_return()
        """)

if __name__ == "__main__":
    unittest.main()
