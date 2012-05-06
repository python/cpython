# Run the _testcapi module tests (tests for the Python/C API):  by defn,
# these are all functions _testcapi exports whose name begins with 'test_'.

from __future__ import with_statement
import os
import pickle
import random
import subprocess
import sys
import time
import unittest
from test import support
try:
    import threading
except ImportError:
    threading = None
import _testcapi


def testfunction(self):
    """some doc"""
    return self

class InstanceMethod:
    id = _testcapi.instancemethod(id)
    testfunction = _testcapi.instancemethod(testfunction)

class CAPITest(unittest.TestCase):

    def test_instancemethod(self):
        inst = InstanceMethod()
        self.assertEqual(id(inst), inst.id())
        self.assertTrue(inst.testfunction() is inst)
        self.assertEqual(inst.testfunction.__doc__, testfunction.__doc__)
        self.assertEqual(InstanceMethod.testfunction.__doc__, testfunction.__doc__)

        InstanceMethod.testfunction.attribute = "test"
        self.assertEqual(testfunction.attribute, "test")
        self.assertRaises(AttributeError, setattr, inst.testfunction, "attribute", "test")

    @unittest.skipUnless(threading, 'Threading required for this test.')
    def test_no_FatalError_infinite_loop(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import _testcapi;'
                              '_testcapi.crash_no_current_thread()'],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        (out, err) = p.communicate()
        self.assertEqual(out, b'')
        # This used to cause an infinite loop.
        self.assertTrue(err.rstrip().startswith(
                         b'Fatal Python error:'
                         b' PyThreadState_Get: no current thread'))

    def test_memoryview_from_NULL_pointer(self):
        self.assertRaises(ValueError, _testcapi.make_memoryview_from_NULL_pointer)

    def test_exc_info(self):
        raised_exception = ValueError("5")
        new_exc = TypeError("TEST")
        try:
            raise raised_exception
        except ValueError as e:
            tb = e.__traceback__
            orig_sys_exc_info = sys.exc_info()
            orig_exc_info = _testcapi.set_exc_info(new_exc.__class__, new_exc, None)
            new_sys_exc_info = sys.exc_info()
            new_exc_info = _testcapi.set_exc_info(*orig_exc_info)
            reset_sys_exc_info = sys.exc_info()

            self.assertEqual(orig_exc_info[1], e)

            self.assertSequenceEqual(orig_exc_info, (raised_exception.__class__, raised_exception, tb))
            self.assertSequenceEqual(orig_sys_exc_info, orig_exc_info)
            self.assertSequenceEqual(reset_sys_exc_info, orig_exc_info)
            self.assertSequenceEqual(new_exc_info, (new_exc.__class__, new_exc, None))
            self.assertSequenceEqual(new_sys_exc_info, new_exc_info)
        else:
            self.assertTrue(False)

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
                print("(%i)"%(len(l),),)
            for i in range(1000):
                a = i*i
            if context and not context.event.is_set():
                continue
            count += 1
            self.assertTrue(count < 10000,
                "timeout waiting for %i callbacks, got %i"%(n, len(l)))
        if False and support.verbose:
            print("(%i)"%(len(l),))

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
                if False and support.verbose:
                    print("finished threads: ", nFinished)
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

    def test_subinterps(self):
        # XXX this test leaks in refleak runs
        import builtins
        r, w = os.pipe()
        code = """if 1:
            import sys, builtins, pickle
            with open({:d}, "wb") as f:
                pickle.dump(id(sys.modules), f)
                pickle.dump(id(builtins), f)
            """.format(w)
        with open(r, "rb") as f:
            ret = _testcapi.run_in_subinterp(code)
            self.assertEqual(ret, 0)
            self.assertNotEqual(pickle.load(f), id(sys.modules))
            self.assertNotEqual(pickle.load(f), id(builtins))

# Bug #6012
class Test6012(unittest.TestCase):
    def test(self):
        self.assertEqual(_testcapi.argparsing("Hello", "World"), 1)


class EmbeddingTest(unittest.TestCase):

    @unittest.skipIf(
        sys.platform.startswith('win'),
        "test doesn't work under Windows")
    def test_subinterps(self):
        # XXX only tested under Unix checkouts
        basepath = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        oldcwd = os.getcwd()
        # This is needed otherwise we get a fatal error:
        # "Py_Initialize: Unable to get the locale encoding
        # LookupError: no codec search functions registered: can't find encoding"
        os.chdir(basepath)
        try:
            exe = os.path.join(basepath, "Modules", "_testembed")
            if not os.path.exists(exe):
                self.skipTest("%r doesn't exist" % exe)
            p = subprocess.Popen([exe],
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
            (out, err) = p.communicate()
            self.assertEqual(p.returncode, 0,
                             "bad returncode %d, stderr is %r" %
                             (p.returncode, err))
            if support.verbose:
                print()
                print(out.decode('latin1'))
                print(err.decode('latin1'))
        finally:
            os.chdir(oldcwd)


def test_main():
    support.run_unittest(CAPITest, TestPendingCalls, Test6012, EmbeddingTest)

    for name in dir(_testcapi):
        if name.startswith('test_'):
            test = getattr(_testcapi, name)
            if support.verbose:
                print("internal", name)
            test()

    # some extra thread-state tests driven via _testcapi
    def TestThreadState():
        if support.verbose:
            print("auto-thread-state")

        idents = []

        def callback():
            idents.append(threading.get_ident())

        _testcapi._test_thread_state(callback)
        a = b = callback
        time.sleep(1)
        # Check our main thread is in the list exactly 3 times.
        if idents.count(threading.get_ident()) != 3:
            raise support.TestFailed(
                        "Couldn't find main thread correctly in the list")

    if threading:
        import time
        TestThreadState()
        t = threading.Thread(target=TestThreadState)
        t.start()
        t.join()


if __name__ == "__main__":
    test_main()
