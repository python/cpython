# Copyright (c) 2004 Python Software Foundation.
# All rights reserved.

# Written by Eric Price <eprice at tjhsst.edu>
#    and Facundo Batista <facundo at taniquetil.com.ar>
#    and Raymond Hettinger <python at rcn.com>
#    and Aahz (aahz at pobox.com)
#    and Tim Peters

"""
These are the test cases for the Decimal module.

There are two groups of tests, Arithmetic and Behaviour. The former test
the Decimal arithmetic using the tests provided by Mike Cowlishaw. The latter
test the pythonic behaviour according to PEP 327.

Cowlishaw's tests can be downloaded from:

   http://speleotrove.com/decimal/dectest.zip

This test module can be called from command line with one parameter (Arithmetic
or Behaviour) to test each part, or without parameter to test both parts. If
you're working through IDLE, you can import this test module and call test()
with the corresponding argument.
"""

import unittest
import logging
import os
import sys
from test.support import (is_resource_enabled, TestFailed,
                          darwin_malloc_err_warning,
                          STDLIB_DIR)
from test.support.import_helper import import_fresh_module


if sys.platform == 'darwin':
    darwin_malloc_err_warning('test_decimal')


C = import_fresh_module('decimal', fresh=['_decimal'])
P = import_fresh_module('decimal', blocked=['_decimal'])
import decimal as orig_sys_decimal

# fractions module must import the correct decimal module.
cfractions = import_fresh_module('fractions', fresh=['fractions'])
sys.modules['decimal'] = P
pfractions = import_fresh_module('fractions', fresh=['fractions'])
sys.modules['decimal'] = C
fractions = {C: cfractions, P: pfractions}
sys.modules['decimal'] = orig_sys_decimal

requires_cdecimal = unittest.skipUnless(C, "test requires C version")

# Useful Test Constant
Signals = {
  C: tuple(C.getcontext().flags.keys()) if C else None,
  P: tuple(P.getcontext().flags.keys())
}
# Signals ordered with respect to precedence: when an operation
# produces multiple signals, signals occurring later in the list
# should be handled before those occurring earlier in the list.
OrderedSignals = {
  C: [C.Clamped, C.Rounded, C.Inexact, C.Subnormal, C.Underflow,
      C.Overflow, C.DivisionByZero, C.InvalidOperation,
      C.FloatOperation] if C else None,
  P: [P.Clamped, P.Rounded, P.Inexact, P.Subnormal, P.Underflow,
      P.Overflow, P.DivisionByZero, P.InvalidOperation,
      P.FloatOperation]
}

def assert_signals(cls, context, attr, expected):
    d = getattr(context, attr)
    cls.assertTrue(all(d[s] if s in expected else not d[s] for s in d))

RoundingModes = [
  P.ROUND_UP, P.ROUND_DOWN, P.ROUND_CEILING, P.ROUND_FLOOR,
  P.ROUND_HALF_UP, P.ROUND_HALF_DOWN, P.ROUND_HALF_EVEN,
  P.ROUND_05UP
]

# Tests are built around these assumed context defaults.
# tearDownModule() restores the original context.
ORIGINAL_CONTEXT = {
  C: C.getcontext().copy() if C else None,
  P: P.getcontext().copy()
}

def init(m):
    if not m: return
    DefaultTestContext = m.Context(
       prec=9, rounding=P.ROUND_HALF_EVEN, traps=dict.fromkeys(Signals[m], 0)
    )
    m.setcontext(DefaultTestContext)

# Test extra functionality in the C version (-DEXTRA_FUNCTIONALITY).
EXTRA_FUNCTIONALITY = True if hasattr(C, 'DecClamped') else False
requires_extra_functionality = unittest.skipUnless(
  EXTRA_FUNCTIONALITY, "test requires build with -DEXTRA_FUNCTIONALITY")
skip_if_extra_functionality = unittest.skipIf(
  EXTRA_FUNCTIONALITY, "test requires regular build")


TEST_ALL = is_resource_enabled('decimal')
TODO_TESTS = None
DEBUG = False


def load_tests_for_base_classes(loader, tests, base_classes):
    for prefix, mod in ('C', C), ('Py', P):
        if not mod:
            continue

        for base_class in base_classes:
            test_class = type(prefix + base_class.__name__,
                              (base_class, unittest.TestCase),
                              {'decimal': mod})
            tests.addTest(loader.loadTestsFromTestCase(test_class))

    return tests

def load_arithmetic_module():
    module = __import__("test.test_decimal.test_arithmetic")
    arithmetic = module.test_decimal.test_arithmetic
    arithmetic.TEST_ALL = TEST_ALL
    arithmetic.TODO_TESTS = TODO_TESTS
    arithmetic.DEBUG = DEBUG
    return arithmetic

def load_tests(loader, tests, pattern):
    arithmetic = load_arithmetic_module()
    if TODO_TESTS is not None:
        # Run only Arithmetic tests
        tests = loader.suiteClass()
        arithmetic.load_tests(loader, tests, pattern)
        return tests

    start_dir = os.path.dirname(__file__)
    package_tests = loader.discover(start_dir=start_dir,
                                    top_level_dir=STDLIB_DIR)
    tests.addTests(package_tests)
    return tests

def setUpModule():
    init(C)
    init(P)

def tearDownModule():
    if C: C.setcontext(ORIGINAL_CONTEXT[C].copy())
    P.setcontext(ORIGINAL_CONTEXT[P].copy())
    if not C:
        logging.getLogger(__name__).warning(
            'C tests skipped: no module named _decimal.'
        )
    if not orig_sys_decimal is sys.modules['decimal']:
        raise TestFailed("Internal error: unbalanced number of changes to "
                         "sys.modules['decimal'].")

