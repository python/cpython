import unittest
from test.support import import_helper

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


if __name__ == "__main__":
    unittest.main()
