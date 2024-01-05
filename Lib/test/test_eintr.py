import os
import signal
import unittest
from test import support
from test.support import script_helper


@unittest.skipUnless(os.name == "posix", "only supported on Unix")
class EINTRTests(unittest.TestCase):

    @unittest.skipUnless(hasattr(signal, "setitimer"), "requires setitimer()")
    @support.requires_resource('walltime')
    def test_all(self):
        # Run the tester in a sub-process, to make sure there is only one
        # thread (for reliable signal delivery).
        script = support.findfile("_test_eintr.py")
        script_helper.run_test_script(script)


if __name__ == "__main__":
    unittest.main()
