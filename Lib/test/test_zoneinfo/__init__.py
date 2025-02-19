import os
import sys
from test.support import load_package_tests

if sys.platform == "wasm":
    unittest.skip("zlib unavailable")

def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
