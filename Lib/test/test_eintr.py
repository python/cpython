import os
import signal
import subprocess
import sys
import unittest

from test import support
from test.support import script_helper


@unittest.skipUnless(os.name == "posix", "only supported on Unix")
class EINTRTests(unittest.TestCase):

    @unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
    def test_all(self):
        # Run the tester in a sub-process, to make sure there is only one
        # thread (for reliable signal delivery).
        tester = support.findfile("eintr_tester.py", subdir="eintrdata")
        # use -u to try to get the full output if the test hangs or crash
        args = ["-u", tester, "-v"]
        if support.verbose:
            print()
            print("--- run eintr_tester.py ---", flush=True)
            # In verbose mode, the child process inherit stdout and stdout,
            # to see output in realtime and reduce the risk of losing output.
            args = [sys.executable, "-E", "-X", "faulthandler", *args]
            proc = subprocess.run(args)
            print(f"--- eintr_tester.py completed: "
                  f"exit code {proc.returncode} ---", flush=True)
            if proc.returncode:
                self.fail("eintr_tester.py failed")
        else:
            script_helper.assert_python_ok("-u", tester, "-v")


if __name__ == "__main__":
    unittest.main()
