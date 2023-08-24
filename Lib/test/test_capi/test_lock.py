import unittest
from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
_testinternalcapi = import_helper.import_module('_testinternalcapi')

class PyAtomicTests(unittest.TestCase):
    pass

for name in sorted(dir(_testinternalcapi)):
    if name.startswith('test_lock_'):
        setattr(PyAtomicTests, name, getattr(_testinternalcapi, name))

if __name__ == "__main__":
    unittest.main()
