"""This test checks for correct wait4() behavior.
"""

import os
import time
import sys
from test.fork_wait import ForkWait
from test.support import run_unittest, reap_children, get_attribute

# If either of these do not exist, skip this test.
get_attribute(os, 'fork')
get_attribute(os, 'wait4')


class Wait4Test(ForkWait):
    def wait_impl(self, cpid):
        option = os.WNOHANG
        if sys.platform.startswith('aix'):
            # Issue #11185: wait4 is broken on AIX and will always return 0
            # with WNOHANG.
            option = 0
        for i in range(10):
            # wait4() shouldn't hang, but some of the buildbots seem to hang
            # in the forking tests.  This is an attempt to fix the problem.
            spid, status, rusage = os.wait4(cpid, option)
            if spid == cpid:
                break
            time.sleep(1.0)
        self.assertEqual(spid, cpid)
        self.assertEqual(status, 0, "cause = %d, exit = %d" % (status&0xff, status>>8))
        self.assertTrue(rusage)

def test_main():
    run_unittest(Wait4Test)
    reap_children()

if __name__ == "__main__":
    test_main()
