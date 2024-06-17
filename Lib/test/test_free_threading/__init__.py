import os

from test import support


if support.Py_GIL_DISABLED:
    def load_tests(*args):
        return support.load_package_tests(os.path.dirname(__file__), *args)
