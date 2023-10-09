import os
from test.support import import_helper, load_package_tests

# Do not run the whole directory, if `_testcapi` module is missing.
# It is assumed that all tests in this directory rely on it.
import_helper.import_module('_testcapi')


def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
