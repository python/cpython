import unittest
import sys
from doctest import DocTestSuite, IGNORE_EXCEPTION_DETAIL
from . import (C, P, init, orig_sys_decimal, ORIGINAL_CONTEXT)

def get_test_suite_for_module(mod):
    if not mod:
        return None

    orig_context = orig_sys_decimal.getcontext().copy()

    def setUp(self, mod=mod):
        sys.modules['decimal'] = mod
        init(mod)

    def tearDown(slf, mod=mod, orig_context=orig_context):
        sys.modules['decimal'] = orig_sys_decimal
        mod.setcontext(ORIGINAL_CONTEXT[mod].copy())
        orig_sys_decimal.setcontext(orig_context.copy())

    optionflags = IGNORE_EXCEPTION_DETAIL if mod is C else 0
    return DocTestSuite(mod, setUp=setUp, tearDown=tearDown,
                        optionflags=optionflags)

def load_tests(loader, tests, pattern):
    for mod in C, P:
        sys.modules['decimal'] = mod
        test_suite = get_test_suite_for_module(mod)
        if test_suite:
            tests.addTest(test_suite)

    sys.modules['decimal'] = orig_sys_decimal
    return tests


if __name__ == '__main__':
    unittest.main()
