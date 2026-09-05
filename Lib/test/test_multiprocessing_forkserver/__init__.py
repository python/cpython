import multiprocessing
import os.path
import sys
import unittest
from test import support

if support.PGO:
    raise unittest.SkipTest("test is not helpful for PGO")

if sys.platform in ("win32", "cygwin"):
    raise unittest.SkipTest("forkserver is not available on Windows")

if not support.has_fork_support:
    raise unittest.SkipTest("requires working os.fork()")

# The forkserver start method requires passing file descriptors over a Unix
# socket, which is not available on every platform (e.g. Solaris/illumos).
if "forkserver" not in multiprocessing.get_all_start_methods():
    raise unittest.SkipTest("forkserver start method is not available")

def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
