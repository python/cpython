# Run the _testcapi module tests (tests for the Python/C API):  by defn,
# these are all functions _testcapi exports whose name begins with 'test_'.

from __future__ import with_statement
import os
import pickle
import random
import subprocess
import sys
import time
import _thread
import unittest
from test import support
try:
    import _posixsubprocess
except ImportError:
    _posixsubprocess = None
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
        self.assertEqual(err.rstrip(),
                         b'Fatal Python error:'
                         b' PyThreadState_Get: no current thread')

    def test_memoryview_from_NULL_pointer(self):
        self.assertRaises(ValueError, _testcapi.make_memoryview_from_NULL_pointer)

    @unittest.skipUnless(_posixsubprocess, '_posixsubprocess required for this test.')
    def test_seq_bytes_to_charp_array(self):
        # Issue #15732: crash in _PySequence_BytesToCharpArray()
        class Z(object):
            def __len__(self):
                return 1
        self.assertRaises(TypeError, _posixsubprocess.fork_exec,
                          1,Z(),3,[1, 2],5,6,7,8,9,10,11,12,13,14,15,16,17)
        # Issue #15736: overflow in _PySequence_BytesToCharpArray()
        class Z(object):
            def __len__(self):
                return sys.maxsize
            def __getitem__(self, i):
                return b'x'
        self.assertRaises(MemoryError, _posixsubprocess.fork_exec,
                          1,Z(),3,[1, 2],5,6,7,8,9,10,11,12,13,14,15,16,17)

    @unittest.skipUnless(_posixsubprocess, '_posixsubprocess required for this test.')
    def test_subprocess_fork_exec(self):
        class Z(object):
            def __len__(self):
                return 1

        # Issue #15738: crash in subprocess_fork_exec()
        self.assertRaises(TypeError, _posixsubprocess.fork_exec,
                          Z(),[b'1'],3,[1, 2],5,6,7,8,9,10,11,12,13,14,15,16,17)

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


@unittest.skipUnless(threading, 'Threading required for this test.')
class TestThreadState(unittest.TestCase):

    @support.reap_threads
    def test_thread_state(self):
        # some extra thread-state tests driven via _testcapi
        def target():
            idents = []

            def callback():
                idents.append(_thread.get_ident())

            _testcapi._test_thread_state(callback)
            a = b = callback
            time.sleep(1)
            # Check our main thread is in the list exactly 3 times.
            self.assertEqual(idents.count(_thread.get_ident()), 3,
                             "Couldn't find main thread correctly in the list")

        target()
        t = threading.Thread(target=target)
        t.start()
        t.join()


def test_main():
    support.run_unittest(CAPITest, TestPendingCalls, Test6012,
                         EmbeddingTest, TestThreadState)

    for name in dir(_testcapi):
        if name.startswith('test_'):
            test = getattr(_testcapi, name)
            if support.verbose:
                print("internal", name)
            test()

if __name__ == "__main__":
    test_main()
