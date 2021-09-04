import os
from test import support

# Skip tests if we don't have concurrent.futures.
support.import_module('concurrent.futures')


def load_tests(*args):
    pkg_dir = os.path.dirname(__file__)
    return support.load_package_tests(pkg_dir, *args)
