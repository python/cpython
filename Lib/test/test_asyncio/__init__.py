import os
from test import support
from test.support import load_package_tests
from test.support import import_helper

support.requires_working_socket(module=True)

# Skip tests if we don't have concurrent.futures.
import_helper.import_module('concurrent.futures')

def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
