# Run the _test module tests (tests for the Python/C API):  by defn, these
# are all functions _test exports whose name begins with 'test_'.

import sys
import test_support
import _test

for name in dir(_test):
    if name.startswith('test_'):
        test = getattr(_test, name)
        if test_support.verbose:
            print "internal", name
        try:
            test()
        except _test.error:
            raise test_support.TestFailed, sys.exc_info()[1]
