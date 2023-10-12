import unittest
from test._test_multiprocessing import install_tests_in_module_dict

install_tests_in_module_dict(globals(), 'forkserver', exclude_types=True)

if __name__ == '__main__':
    unittest.main()
