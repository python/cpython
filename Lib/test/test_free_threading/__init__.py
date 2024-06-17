from test import support


if support.Py_GIL_DISABLED:
    import os

    def load_tests(*args):
        return support.load_package_tests(os.path.dirname(__file__), *args)
