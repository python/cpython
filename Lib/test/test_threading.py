# Very rudimentary test of threading module

import test.test_support
from test.test_support import verbose
import random
import threading
import time
import unittest

# A trivial mutable counter.
class Counter(object):
    def __init__(self):
        self.value = 0
    def inc(self):
        self.value += 1
    def dec(self):
        self.value -= 1
    def get(self):
        return self.value

class TestThread(threading.Thread):
    def __init__(self, name, testcase, sema, mutex, nrunning):
        threading.Thread.__init__(self, name=name)
        self.testcase = testcase
        self.sema = sema
        self.mutex = mutex
        self.nrunning = nrunning

    def run(self):
        delay = random.random() * 2
        if verbose:
            print 'task', self.getName(), 'will run for', delay, 'sec'

        self.sema.acquire()

        self.mutex.acquire()
        self.nrunning.inc()
        if verbose:
            print self.nrunning.get(), 'tasks are running'
        self.testcase.assert_(self.nrunning.get() <= 3)
        self.mutex.release()

        time.sleep(delay)
        if verbose:
            print 'task', self.getName(), 'done'

        self.mutex.acquire()
        self.nrunning.dec()
        self.testcase.assert_(self.nrunning.get() >= 0)
        if verbose:
            print self.getName(), 'is finished.', self.nrunning.get(), \
                  'tasks are running'
        self.mutex.release()

        self.sema.release()

class ThreadTests(unittest.TestCase):

    # Create a bunch of threads, let each do some work, wait until all are
    # done.
    def test_various_ops(self):
        # This takes about n/3 seconds to run (about n/3 clumps of tasks,
        # times about 1 second per clump).
        NUMTASKS = 10

        # no more than 3 of the 10 can run at once
        sema = threading.BoundedSemaphore(value=3)
        mutex = threading.RLock()
        numrunning = Counter()

        threads = []

        for i in range(NUMTASKS):
            t = TestThread("<thread %d>"%i, self, sema, mutex, numrunning)
            threads.append(t)
            t.start()

        if verbose:
            print 'waiting for all tasks to complete'
        for t in threads:
            t.join()
        if verbose:
            print 'all tasks done'
        self.assertEqual(numrunning.get(), 0)


def test_main():
    test.test_support.run_unittest(ThreadTests)

if __name__ == "__main__":
    test_main()
