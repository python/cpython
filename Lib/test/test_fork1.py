"""This test checks for correct fork() behavior.
"""

import os
from test.fork_wait import ForkWait
from test.test_support import TestSkipped, run_unittest

try:
    os.fork
except AttributeError:
    raise TestSkipped, "os.fork not defined -- skipping test_fork1"

class ForkTest(ForkWait):
    def wait_impl(self, cpid):
        spid, status = os.waitpid(cpid, 0)
        self.assertEqual(spid, cpid)
        self.assertEqual(status, 0, "cause = %d, exit = %d" % (status&0xff, status>>8))

def test_main():
    run_unittest(ForkTest)

if __name__ == "__main__":
    test_main()
