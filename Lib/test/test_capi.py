# Run the _testcapi module tests (tests for the Python/C API):  by defn,
# these are all functions _testcapi exports whose name begins with 'test_'.

import sys
import test_support
import _testcapi

for name in dir(_testcapi):
    if name.startswith('test_'):
        test = getattr(_testcapi, name)
        if test_support.verbose:
            print "internal", name
        try:
            test()
        except _testcapi.error:
            raise test_support.TestFailed, sys.exc_info()[1]
