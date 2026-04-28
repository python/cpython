import os
import unittest
from test import support
from test.support import load_package_tests

# NSKIP050 https://github.com/nanvix/cpython/issues/530
if support.is_nanvix and not support.is_nanvix_standalone:
    raise unittest.SkipTest("NSKIP050: hosted Nanvix unable to run this module cleanly (rmdir errno 88 cascade and/or other linuxd VFS issues)")  # detail: not bisected, see #530

def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
