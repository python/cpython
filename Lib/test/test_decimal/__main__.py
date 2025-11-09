import unittest
from . import load_tests
import optparse

def execute_tests(arith=None, verbose=None, todo_tests=None, debug=None):
    """ Execute the tests.

    Runs all arithmetic tests if arith is True or if the "decimal" resource
    is enabled in regrtest.py
    """

    module = getattr(__import__("test.test_decimal"), "test_decimal")
    module.ARITH = arith
    module.TODO_TESTS = todo_tests
    module.DEBUG = debug
    unittest.main(module, verbosity=2 if verbose else 1, exit=False, argv=[__name__])

p = optparse.OptionParser("test_decimal [--debug] [{--skip | test1 [test2 [...]]}]")
p.add_option('--debug', '-d', action='store_true', help='shows the test number and context before each test')
p.add_option('--skip',  '-s', action='store_true', help='skip over 90% of the arithmetic tests')
(opt, args) = p.parse_args()

if opt.skip:
    execute_tests(arith=False, verbose=True)
elif args:
    execute_tests(arith=True, verbose=True, todo_tests=args, debug=opt.debug)
else:
    execute_tests(arith=True, verbose=True)
