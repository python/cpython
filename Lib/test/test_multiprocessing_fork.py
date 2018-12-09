import unittest
import test._test_multiprocessing

import sys
from test import support

if support.PGO:
    raise unittest.SkipTest("test is not helpful for PGO")

if sys.platform == 'darwin':
    raise unittest.SkipTest("test may crash on macOS (bpo-33725)")

test._test_multiprocessing.install_tests_in_module_dict(globals(), 'fork')

if __name__ == '__main__':
    unittest.main()
