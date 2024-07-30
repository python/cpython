import unittest
from test._test_multiprocessing import install_tests_in_module_dict

install_tests_in_module_dict(globals(), 'fork', only_type="threads")

if __name__ == '__main__':
    unittest.main()
