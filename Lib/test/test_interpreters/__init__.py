import os
from test.support import load_package_tests, Py_GIL_DISABLED

if not Py_GIL_DISABLED:
    def load_tests(*args):
        return load_package_tests(os.path.dirname(__file__), *args)
