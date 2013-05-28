# Skip test if tkinter wasn't built or idlelib was deleted.
from test.support import import_module
import_module('tkinter')  # discard return
itdir = import_module('idlelib.idle_test')

# Without test_main present, regrtest.runtest_inner (line1219)
# imitates unittest.main by calling
# unittest.TestLoader().loadTestsFromModule(this_module)
# which look for load_tests and uses it if found.
load_tests = itdir.load_tests

if __name__ == '__main__':
    import unittest
    unittest.main(verbosity=2, exit=False)
