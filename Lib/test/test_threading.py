# Very rudimentary test of threading module

import test.test_support
from test.test_support import verbose
import random
import sys
import threading
import thread
import time
import unittest
import weakref

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
        delay = random.random() / 10000.0
        if verbose:
            print('task %s will run for %.1f usec' %
                  (self.getName(), delay * 1e6))

        with self.sema:
            with self.mutex:
                self.nrunning.inc()
                if verbose:
                    print(self.nrunning.get(), 'tasks are running')
                self.testcase.assert_(self.nrunning.get() <= 3)

            time.sleep(delay)
            if verbose:
                print('task', self.getName(), 'done')
            with self.mutex:
                self.nrunning.dec()
                self.testcase.assert_(self.nrunning.get() >= 0)
                if verbose:
                    print('%s is finished. %d tasks are running' %
                          (self.getName(), self.nrunning.get()))

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
            print('waiting for all tasks to complete')
        for t in threads:
            t.join(NUMTASKS)
            self.assert_(not t.isAlive())
        if verbose:
            print('all tasks done')
        self.assertEqual(numrunning.get(), 0)

    # run with a small(ish) thread stack size (256kB)
    def test_various_ops_small_stack(self):
        if verbose:
            print('with 256kB thread stack size...')
        try:
            threading.stack_size(262144)
        except thread.error:
            if verbose:
                print('platform does not support changing thread stack size')
            return
        self.test_various_ops()
        threading.stack_size(0)

    # run with a large thread stack size (1MB)
    def test_various_ops_large_stack(self):
        if verbose:
            print('with 1MB thread stack size...')
        try:
            threading.stack_size(0x100000)
        except thread.error:
            if verbose:
                print('platform does not support changing thread stack size')
            return
        self.test_various_ops()
        threading.stack_size(0)

    def test_foreign_thread(self):
        # Check that a "foreign" thread can use the threading module.
        def f(mutex):
            # Acquiring an RLock forces an entry for the foreign
            # thread to get made in the threading._active map.
            r = threading.RLock()
            r.acquire()
            r.release()
            mutex.release()

        mutex = threading.Lock()
        mutex.acquire()
        tid = thread.start_new_thread(f, (mutex,))
        # Wait for the thread to finish.
        mutex.acquire()
        self.assert_(tid in threading._active)
        self.assert_(isinstance(threading._active[tid],
                                threading._DummyThread))
        del threading._active[tid]

    # PyThreadState_SetAsyncExc() is a CPython-only gimmick, not (currently)
    # exposed at the Python level.  This test relies on ctypes to get at it.
    def test_PyThreadState_SetAsyncExc(self):
        try:
            import ctypes
        except ImportError:
            if verbose:
                print("test_PyThreadState_SetAsyncExc can't import ctypes")
            return  # can't do anything

        set_async_exc = ctypes.pythonapi.PyThreadState_SetAsyncExc

        class AsyncExc(Exception):
            pass

        exception = ctypes.py_object(AsyncExc)

        # `worker_started` is set by the thread when it's inside a try/except
        # block waiting to catch the asynchronously set AsyncExc exception.
        # `worker_saw_exception` is set by the thread upon catching that
        # exception.
        worker_started = threading.Event()
        worker_saw_exception = threading.Event()

        class Worker(threading.Thread):
            def run(self):
                self.id = thread.get_ident()
                self.finished = False

                try:
                    while True:
                        worker_started.set()
                        time.sleep(0.1)
                except AsyncExc:
                    self.finished = True
                    worker_saw_exception.set()

        t = Worker()
        t.setDaemon(True) # so if this fails, we don't hang Python at shutdown
        t.start()
        if verbose:
            print("    started worker thread")

        # Try a thread id that doesn't make sense.
        if verbose:
            print("    trying nonsensical thread id")
        result = set_async_exc(ctypes.c_long(-1), exception)
        self.assertEqual(result, 0)  # no thread states modified

        # Now raise an exception in the worker thread.
        if verbose:
            print("    waiting for worker thread to get started")
        worker_started.wait()
        if verbose:
            print("    verifying worker hasn't exited")
        self.assert_(not t.finished)
        if verbose:
            print("    attempting to raise asynch exception in worker")
        result = set_async_exc(ctypes.c_long(t.id), exception)
        self.assertEqual(result, 1) # one thread state modified
        if verbose:
            print("    waiting for worker to say it caught the exception")
        worker_saw_exception.wait(timeout=10)
        self.assert_(t.finished)
        if verbose:
            print("    all OK -- joining worker")
        if t.finished:
            t.join()
        # else the thread is still running, and we have no way to kill it

    def test_finalize_runnning_thread(self):
        # Issue 1402: the PyGILState_Ensure / _Release functions may be called
        # very late on python exit: on deallocation of a running thread for
        # example.
        try:
            import ctypes
        except ImportError:
            if verbose:
                print("test_finalize_with_runnning_thread can't import ctypes")
            return  # can't do anything

        import subprocess
        rc = subprocess.call([sys.executable, "-c", """if 1:
            import ctypes, sys, time, thread

            # This lock is used as a simple event variable.
            ready = thread.allocate_lock()
            ready.acquire()

            # Module globals are cleared before __del__ is run
            # So we save the functions in class dict
            class C:
                ensure = ctypes.pythonapi.PyGILState_Ensure
                release = ctypes.pythonapi.PyGILState_Release
                def __del__(self):
                    state = self.ensure()
                    self.release(state)

            def waitingThread():
                x = C()
                ready.release()
                time.sleep(100)

            thread.start_new_thread(waitingThread, ())
            ready.acquire()  # Be sure the other thread is waiting.
            sys.exit(42)
            """])
        self.assertEqual(rc, 42)

    def test_finalize_with_trace(self):
        # Issue1733757
        # Avoid a deadlock when sys.settrace steps into threading._shutdown
        import subprocess
        rc = subprocess.call([sys.executable, "-c", """if 1:
            import sys, threading

            # A deadlock-killer, to prevent the
            # testsuite to hang forever
            def killer():
                import os, time
                time.sleep(2)
                print('program blocked; aborting')
                os._exit(2)
            t = threading.Thread(target=killer)
            t.setDaemon(True)
            t.start()

            # This is the trace function
            def func(frame, event, arg):
                threading.currentThread()
                return func

            sys.settrace(func)
            """])
        self.failIf(rc == 2, "interpreted was blocked")
        self.failUnless(rc == 0, "Unexpected error")


    def test_enumerate_after_join(self):
        # Try hard to trigger #1703448: a thread is still returned in
        # threading.enumerate() after it has been join()ed.
        enum = threading.enumerate
        old_interval = sys.getcheckinterval()
        try:
            for i in range(1, 100):
                # Try a couple times at each thread-switching interval
                # to get more interleavings.
                sys.setcheckinterval(i // 5)
                t = threading.Thread(target=lambda: None)
                t.start()
                t.join()
                l = enum()
                self.assertFalse(t in l,
                    "#1703448 triggered after %d trials: %s" % (i, l))
        finally:
            sys.setcheckinterval(old_interval)

    def test_no_refcycle_through_target(self):
        class RunSelfFunction(object):
            def __init__(self, should_raise):
                # The links in this refcycle from Thread back to self
                # should be cleaned up when the thread completes.
                self.should_raise = should_raise
                self.thread = threading.Thread(target=self._run,
                                               args=(self,),
                                               kwargs={'yet_another':self})
                self.thread.start()

            def _run(self, other_ref, yet_another):
                if self.should_raise:
                    raise SystemExit

        cyclic_object = RunSelfFunction(should_raise=False)
        weak_cyclic_object = weakref.ref(cyclic_object)
        cyclic_object.thread.join()
        del cyclic_object
        self.assertEquals(None, weak_cyclic_object(),
                          msg=('%d references still around' %
                               sys.getrefcount(weak_cyclic_object())))

        raising_cyclic_object = RunSelfFunction(should_raise=True)
        weak_raising_cyclic_object = weakref.ref(raising_cyclic_object)
        raising_cyclic_object.thread.join()
        del raising_cyclic_object
        self.assertEquals(None, weak_raising_cyclic_object(),
                          msg=('%d references still around' %
                               sys.getrefcount(weak_raising_cyclic_object())))


class ThreadingExceptionTests(unittest.TestCase):
    # A RuntimeError should be raised if Thread.start() is called
    # multiple times.
    def test_start_thread_again(self):
        thread = threading.Thread()
        thread.start()
        self.assertRaises(RuntimeError, thread.start)

    def test_releasing_unacquired_rlock(self):
        rlock = threading.RLock()
        self.assertRaises(RuntimeError, rlock.release)

    def test_waiting_on_unacquired_condition(self):
        cond = threading.Condition()
        self.assertRaises(RuntimeError, cond.wait)

    def test_notify_on_unacquired_condition(self):
        cond = threading.Condition()
        self.assertRaises(RuntimeError, cond.notify)

    def test_semaphore_with_negative_value(self):
        self.assertRaises(ValueError, threading.Semaphore, value = -1)
        self.assertRaises(ValueError, threading.Semaphore, value = -sys.maxsize)

    def test_joining_current_thread(self):
        currentThread = threading.currentThread()
        self.assertRaises(RuntimeError, currentThread.join);

    def test_joining_inactive_thread(self):
        thread = threading.Thread()
        self.assertRaises(RuntimeError, thread.join)

    def test_daemonize_active_thread(self):
        thread = threading.Thread()
        thread.start()
        self.assertRaises(RuntimeError, thread.setDaemon, True)


def test_main():
    test.test_support.run_unittest(ThreadTests,
                                   ThreadingExceptionTests)

if __name__ == "__main__":
    test_main()
