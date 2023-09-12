import unittest
from test.support import import_helper, threading_helper

# Skip this test if the _testcapi module isn't available.
_testinternalcapi = import_helper.import_module('_testinternalcapi')

# Lock tests require threads
threading_helper.requires_working_threading(module=True)


class PyLockTests(unittest.TestCase):
    pass

for name in sorted(dir(_testinternalcapi)):
    if name.startswith('test_lock_'):
        setattr(PyLockTests, name, getattr(_testinternalcapi, name))

if __name__ == "__main__":
    unittest.main()
