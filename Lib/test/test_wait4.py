"""This test checks for correct wait4() behavior.
"""

import os
import time
import sys
import unittest
from test.fork_wait import ForkWait
from test import support

# If either of these do not exist, skip this test.
support.get_attribute(os, 'fork')
support.get_attribute(os, 'wait4')


class Wait4Test(ForkWait):
    def wait_impl(self, cpid):
        option = os.WNOHANG
        if sys.platform.startswith('aix'):
            # Issue #11185: wait4 is broken on AIX and will always return 0
            # with WNOHANG.
            option = 0
        deadline = time.monotonic() + support.SHORT_TIMEOUT
        while time.monotonic() <= deadline:
            # wait4() shouldn't hang, but some of the buildbots seem to hang
            # in the forking tests.  This is an attempt to fix the problem.
            spid, status, rusage = os.wait4(cpid, option)
            if spid == cpid:
                break
            time.sleep(0.1)
        self.assertEqual(spid, cpid)
        self.assertEqual(status, 0, "cause = %d, exit = %d" % (status&0xff, status>>8))
        self.assertTrue(rusage)

def tearDownModule():
    support.reap_children()

if __name__ == "__main__":
    unittest.main()
