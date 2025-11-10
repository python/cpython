import unittest
import sys
from doctest import DocTestSuite, IGNORE_EXCEPTION_DETAIL
from . import C, P, orig_sys_decimal

def get_test_suite_for_module(mod):
    if not mod:
        return None

    def setUp(self, mod=mod):
        sys.modules['decimal'] = mod

    def tearDown(self):
        sys.modules['decimal'] = orig_sys_decimal

    optionflags = IGNORE_EXCEPTION_DETAIL if mod is C else 0
    return DocTestSuite(mod, setUp=setUp, tearDown=tearDown,
                        optionflags=optionflags)

def load_tests(loader, tests, pattern):
    for mod in C, P:
        sys.modules['decimal'] = mod
        test_suite = get_test_suite_for_module(mod)
        if test_suite:
            tests.addTest(test_suite)

    return tests


if __name__ == '__main__':
    unittest.main()
