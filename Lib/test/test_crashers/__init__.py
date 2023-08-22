# Tests that the crashers in the Lib/test/crashers directory actually
# do crash the interpreter as expected
#
# If a crasher is fixed, it should be moved elsewhere in the test suite to
# ensure it continues to work correctly.

import glob
import os.path
import unittest
from test import support
from test.support.script_helper import assert_python_failure


CRASHER_DIR = os.path.abspath(os.path.dirname(__file__))
CRASHER_FILES = os.path.join(glob.escape(CRASHER_DIR), "*.py")
infinite_loops = frozenset(["infinite_loop_re.py"])


class CrasherTest(unittest.TestCase):
    @support.cpython_only
    def test_crashers_crash(self):
        if support.verbose:
            print()
        for fname in glob.glob(CRASHER_FILES):
            script = os.path.basename(fname)
            if script == "__init__.py":
                continue
            if script in infinite_loops:
                continue
            # Some "crashers" only trigger an exception rather than a
            # segfault. Consider that an acceptable outcome.
            if support.verbose:
                print(f"Checking crasher: {script}", flush=True)
            proc = assert_python_failure(fname)
            if os.name != "nt":
                # On Unix, if proc.rc is negative, the process was killed
                # by a signal
                self.assertLess(proc.rc, 0, proc)
            else:
                # Windows. For example, C0000005 is an Access Violation
                self.assertGreaterEqual(proc.rc, 0xC0000000, proc)


def tearDownModule():
    support.reap_children()


if __name__ == "__main__":
    unittest.main()
