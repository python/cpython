import os
from test.support import load_package_tests, import_helper

import_helper.import_module("_testcapi")

def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
