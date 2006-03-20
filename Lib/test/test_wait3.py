"""This test checks for correct wait3() behavior.
"""

import os
from test.fork_wait import ForkWait
from test.test_support import TestSkipped, run_unittest

try:
    os.fork
except AttributeError:
    raise TestSkipped, "os.fork not defined -- skipping test_wait3"

try:
    os.wait3
except AttributeError:
    raise TestSkipped, "os.wait3 not defined -- skipping test_wait3"

class Wait3Test(ForkWait):
    def wait_impl(self, cpid):
        while 1:
            spid, status, rusage = os.wait3(0)
            if spid == cpid:
                break
        self.assertEqual(spid, cpid)
        self.assertEqual(status, 0, "cause = %d, exit = %d" % (status&0xff, status>>8))
        self.assertTrue(rusage)

def test_main():
    run_unittest(Wait3Test)

if __name__ == "__main__":
    test_main()
