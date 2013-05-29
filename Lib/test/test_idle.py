# Skip test if _tkinter or _thread wasn't built or idlelib was deleted.
from test.support import import_module
import_module('tkinter')
import_module('threading')  # imported by PyShell, imports _thread
idletest = import_module('idlelib.idle_test')

# Without test_main present, regrtest.runtest_inner (line1219) calls
# unittest.TestLoader().loadTestsFromModule(this_module) which calls
# load_tests() if it finds it. (Unittest.main does the same.)
load_tests = idletest.load_tests

if __name__ == '__main__':
    import unittest
    unittest.main(verbosity=2, exit=False)
