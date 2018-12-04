# Run the _testcapi module tests (tests for the Python/C API):  by defn,
# these are all functions _testcapi exports whose name begins with 'test_'.

from __future__ import with_statement
import string
import sys
import time
import random
import unittest
from test import test_support as support
try:
    import thread
    import threading
except ImportError:
    thread = None
    threading = None
# Skip this test if the _testcapi module isn't available.
_testcapi = support.import_module('_testcapi')

class CAPITest(unittest.TestCase):

    def test_buildvalue_N(self):
        _testcapi.test_buildvalue_N()


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
            if False and support.verbose:
                print "(%i)"%(len(l),),
            for i in xrange(1000):
                a = i*i
            if context and not context.event.is_set():
                continue
            count += 1
            self.assertTrue(count < 10000,
                "timeout waiting for %i callbacks, got %i"%(n, len(l)))
        if False and support.verbose:
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

        threads = [threading.Thread(target=self.pendingcalls_thread,
                                    args=(context,))
                   for i in range(context.nThreads)]
        with support.start_threads(threads):
            self.pendingcalls_wait(context.l, n, context)

    def pendingcalls_thread(self, context):
        try:
            self.pendingcalls_submit(context.l, context.n)
        finally:
            with context.lock:
                context.nFinished += 1
                nFinished = context.nFinished
                if False and support.verbose:
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


class TestGetIndices(unittest.TestCase):

    def test_get_indices(self):
        self.assertEqual(_testcapi.get_indices(slice(10L, 20, 1), 100), (0, 10, 20, 1))
        self.assertEqual(_testcapi.get_indices(slice(10.1, 20, 1), 100), None)
        self.assertEqual(_testcapi.get_indices(slice(10, 20L, 1), 100), (0, 10, 20, 1))
        self.assertEqual(_testcapi.get_indices(slice(10, 20.1, 1), 100), None)

        self.assertEqual(_testcapi.get_indices(slice(10L, 20, 1L), 100), (0, 10, 20, 1))
        self.assertEqual(_testcapi.get_indices(slice(10.1, 20, 1L), 100), None)
        self.assertEqual(_testcapi.get_indices(slice(10, 20L, 1L), 100), (0, 10, 20, 1))
        self.assertEqual(_testcapi.get_indices(slice(10, 20.1, 1L), 100), None)


@unittest.skipUnless(threading and thread, 'Threading required for this test.')
class TestThreadState(unittest.TestCase):

    @support.reap_threads
    def test_thread_state(self):
        # some extra thread-state tests driven via _testcapi
        def target():
            idents = []

            def callback():
                idents.append(thread.get_ident())

            _testcapi._test_thread_state(callback)
            a = b = callback
            time.sleep(1)
            # Check our main thread is in the list exactly 3 times.
            self.assertEqual(idents.count(thread.get_ident()), 3,
                             "Couldn't find main thread correctly in the list")

        target()
        t = threading.Thread(target=target)
        t.start()
        t.join()


class Test_testcapi(unittest.TestCase):
    locals().update((name, getattr(_testcapi, name))
                    for name in dir(_testcapi)
                    if name.startswith('test_') and not name.endswith('_code'))


def test_main():
    support.run_unittest(CAPITest, TestPendingCalls,
                         TestThreadState, TestGetIndices, Test_testcapi)

if __name__ == "__main__":
    test_main()
