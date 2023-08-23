import unittest
import test._test_multiprocessing

from test import support

# This test spawns many processes and is slow
support.requires('cpu')

if support.PGO:
    raise unittest.SkipTest("test is not helpful for PGO")

test._test_multiprocessing.install_tests_in_module_dict(globals(), 'spawn')

if __name__ == '__main__':
    unittest.main()
