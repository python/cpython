# Tests that the crashers in the Lib/test/crashers directory actually
# do crash the interpreter as expected
#
# If a crasher is fixed, it should be moved elsewhere in the test suite to
# ensure it continues to work correctly.

import unittest
import glob
import os.path
import test.support
from test.support.script_helper import assert_python_failure

CRASHER_DIR = os.path.join(os.path.dirname(__file__), "crashers")
CRASHER_FILES = os.path.join(glob.escape(CRASHER_DIR), "*.py")

infinite_loops = ["infinite_loop_re.py", "nasty_eq_vs_dict.py"]

class CrasherTest(unittest.TestCase):

    @unittest.skip("these tests are too fragile")
    @test.support.cpython_only
    def test_crashers_crash(self):
        for fname in glob.glob(CRASHER_FILES):
            if os.path.basename(fname) in infinite_loops:
                continue
            # Some "crashers" only trigger an exception rather than a
            # segfault. Consider that an acceptable outcome.
            if test.support.verbose:
                print("Checking crasher:", fname)
            assert_python_failure(fname)


def tearDownModule():
    test.support.reap_children()

if __name__ == "__main__":
    unittest.main()
