import os
import sys
from test.support import import_helper, load_package_tests


if sys.platform != "win32":
    import_helper.import_module("termios")


def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
