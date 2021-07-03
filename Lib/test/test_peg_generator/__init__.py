import os

from test.support import load_package_tests

# Load all tests in package
def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
