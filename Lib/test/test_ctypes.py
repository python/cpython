import unittest
from test.support import import_module

ctypes_test = import_module('Lib.test.ctypes_test')

load_tests = ctypes_test.load_tests

if __name__ == "__main__":
    unittest.main()
