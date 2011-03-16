# Run the _testcapi module tests (tests for the Python/C API):  by defn,
# these are all functions _testcapi exports whose name begins with 'test_'.

from __future__ import with_statement
import sys
import time
import random
import unittest
from test import test_support
try:
    import threading
except ImportError:
    threading = None
import _testcapi

@unittest.skipUnless(threading, 'Threading required for this test.')
class TestPendingCalls(unittest.TestCase):

    def pendingcalls_submit(self, l, n):
        def callback():
            #this function can be interrupted by thread switching so let's
            #use an atomic operation
            l.append(None)

        for i in range(n):
            time.sleep(random.random()*0.02) #0.01 secs on average
            #try submitting callback until successful.
            #rely on regular interrupt to flush queue if we are
            #unsuccessful.
            while True:
                if _testcapi._pending_threadfunc(callback):
                    break;

    def pendingcalls_wait(self, l, n, context = None):
        #now, stick around until l[0] has grown to 10
        count = 0;
        while len(l) != n:
            #this busy loop is where we expect to be interrupted to
            #run our callbacks.  Note that callbacks are only run on the
            #main thread
            if False and test_support.verbose:
                print "(%i)"%(len(l),),
            for i in xrange(1000):
                a = i*i
            if context and not context.event.is_set():
                continue
            count += 1
            self.assertTrue(count < 10000,
                "timeout waiting for %i callbacks, got %i"%(n, len(l)))
        if False and test_support.verbose:
            print "(%i)"%(len(l),)

    def test_pendingcalls_threaded(self):
        #do every callback on a separate thread
        n = 32 #total callbacks
        threads = []
        class foo(object):pass
        context = foo()
        context.l = []
        context.n = 2 #submits per thread
        context.nThreads = n // context.n
        context.nFinished = 0
        context.lock = threading.Lock()
        context.event = threading.Event()

        for i in range(context.nThreads):
            t = threading.Thread(target=self.pendingcalls_thread, args = (context,))
            t.start()
            threads.append(t)

        self.pendingcalls_wait(context.l, n, context)

        for t in threads:
            t.join()

    def pendingcalls_thread(self, context):
        try:
            self.pendingcalls_submit(context.l, context.n)
        finally:
            with context.lock:
                context.nFinished += 1
                nFinished = context.nFinished
                if False and test_support.verbose:
                    print "finished threads: ", nFinished
            if nFinished == context.nThreads:
                context.event.set()

    def test_pendingcalls_non_threaded(self):
        #again, just using the main thread, likely they will all be dispatched at
        #once.  It is ok to ask for too many, because we loop until we find a slot.
        #the loop can be interrupted to dispatch.
        #there are only 32 dispatch slots, so we go for twice that!
        l = []
        n = 64
        self.pendingcalls_submit(l, n)
        self.pendingcalls_wait(l, n)


def test_main():

    for name in dir(_testcapi):
        if name.startswith('test_'):
            test = getattr(_testcapi, name)
            if test_support.verbose:
                print "internal", name
            try:
                test()
            except _testcapi.error:
                raise test_support.TestFailed, sys.exc_info()[1]

    # some extra thread-state tests driven via _testcapi
    def TestThreadState():
        if test_support.verbose:
            print "auto-thread-state"

        idents = []

        def callback():
            idents.append(thread.get_ident())

        _testcapi._test_thread_state(callback)
        a = b = callback
        time.sleep(1)
        # Check our main thread is in the list exactly 3 times.
        if idents.count(thread.get_ident()) != 3:
            raise test_support.TestFailed, \
                  "Couldn't find main thread correctly in the list"

    if threading:
        import thread
        import time
        TestThreadState()
        t=threading.Thread(target=TestThreadState)
        t.start()
        t.join()

    test_support.run_unittest(TestPendingCalls)

if __name__ == "__main__":
    test_main()
