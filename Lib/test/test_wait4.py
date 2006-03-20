"""This test checks for correct wait4() behavior.
"""

import os
from test.fork_wait import ForkWait
from test.test_support import TestSkipped, run_unittest

try:
    os.fork
except AttributeError:
    raise TestSkipped, "os.fork not defined -- skipping test_wait4"

try:
    os.wait4
except AttributeError:
    raise TestSkipped, "os.wait4 not defined -- skipping test_wait4"

class Wait4Test(ForkWait):
    def wait_impl(self, cpid):
        spid, status, rusage = os.wait4(cpid, 0)
        self.assertEqual(spid, cpid)
        self.assertEqual(status, 0, "cause = %d, exit = %d" % (status&0xff, status>>8))
        self.assertTrue(rusage)

def test_main():
    run_unittest(Wait4Test)

if __name__ == "__main__":
    test_main()
