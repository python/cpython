import os.path
from test import support
from test.support import load_package_tests


# Creating a virtual environment and building C extensions is slow
support.requires('cpu')


# Load all tests in package
def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
