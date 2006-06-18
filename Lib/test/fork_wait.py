"""This test case provides support for checking forking and wait behavior.

To test different wait behavior, overrise the wait_impl method.

We want fork1() semantics -- only the forking thread survives in the
child after a fork().

On some systems (e.g. Solaris without posix threads) we find that all
active threads survive in the child after a fork(); this is an error.

While BeOS doesn't officially support fork and native threading in
the same application, the present example should work just fine.  DC
"""

import os, sys, time, thread, unittest
from test.test_support import TestSkipped

LONGSLEEP = 2
SHORTSLEEP = 0.5
NUM_THREADS = 4

class ForkWait(unittest.TestCase):

    def setUp(self):
        self.alive = {}
        self.stop = 0

    def f(self, id):
        while not self.stop:
            self.alive[id] = os.getpid()
            try:
                time.sleep(SHORTSLEEP)
            except IOError:
                pass

    def wait_impl(self, cpid):
        for i in range(10):
            # waitpid() shouldn't hang, but some of the buildbots seem to hang
            # in the forking tests.  This is an attempt to fix the problem.
            spid, status = os.waitpid(cpid, os.WNOHANG)
            if spid == cpid:
                break
            time.sleep(2 * SHORTSLEEP)

        self.assertEquals(spid, cpid)
        self.assertEquals(status, 0, "cause = %d, exit = %d" % (status&0xff, status>>8))

    def test_wait(self):
        for i in range(NUM_THREADS):
            thread.start_new(self.f, (i,))

        time.sleep(LONGSLEEP)

        a = self.alive.keys()
        a.sort()
        self.assertEquals(a, range(NUM_THREADS))

        prefork_lives = self.alive.copy()

        if sys.platform in ['unixware7']:
            cpid = os.fork1()
        else:
            cpid = os.fork()

        if cpid == 0:
            # Child
            time.sleep(LONGSLEEP)
            n = 0
            for key in self.alive:
                if self.alive[key] != prefork_lives[key]:
                    n += 1
            os._exit(n)
        else:
            # Parent
            self.wait_impl(cpid)
            # Tell threads to die
            self.stop = 1
            time.sleep(2*SHORTSLEEP) # Wait for threads to die
