import os
from test import support


def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
