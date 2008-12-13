# Very rudimentary test of threading module

import test.test_support
from test.test_support import verbose
import random
import sys
import threading
import thread
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
            t.join(NUMTASKS)
            self.assert_(not t.isAlive())
        if verbose:
            print 'all tasks done'
        self.assertEqual(numrunning.get(), 0)

    # run with a small(ish) thread stack size (256kB)
    def test_various_ops_small_stack(self):
        if verbose:
            print 'with 256kB thread stack size...'
        try:
            threading.stack_size(262144)
        except thread.error:
            if verbose:
                print 'platform does not support changing thread stack size'
            return
        self.test_various_ops()
        threading.stack_size(0)

    # run with a large thread stack size (1MB)
    def test_various_ops_large_stack(self):
        if verbose:
            print 'with 1MB thread stack size...'
        try:
            threading.stack_size(0x100000)
        except thread.error:
            if verbose:
                print 'platform does not support changing thread stack size'
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
                print "test_PyThreadState_SetAsyncExc can't import ctypes"
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
            print "    started worker thread"

        # Try a thread id that doesn't make sense.
        if verbose:
            print "    trying nonsensical thread id"
        result = set_async_exc(ctypes.c_long(-1), exception)
        self.assertEqual(result, 0)  # no thread states modified

        # Now raise an exception in the worker thread.
        if verbose:
            print "    waiting for worker thread to get started"
        worker_started.wait()
        if verbose:
            print "    verifying worker hasn't exited"
        self.assert_(not t.finished)
        if verbose:
            print "    attempting to raise asynch exception in worker"
        result = set_async_exc(ctypes.c_long(t.id), exception)
        self.assertEqual(result, 1) # one thread state modified
        if verbose:
            print "    waiting for worker to say it caught the exception"
        worker_saw_exception.wait(timeout=10)
        self.assert_(t.finished)
        if verbose:
            print "    all OK -- joining worker"
        if t.finished:
            t.join()
        # else the thread is still running, and we have no way to kill it

    def test_enumerate_after_join(self):
        # Try hard to trigger #1703448: a thread is still returned in
        # threading.enumerate() after it has been join()ed.
        enum = threading.enumerate
        old_interval = sys.getcheckinterval()
        sys.setcheckinterval(1)
        try:
            for i in xrange(1, 1000):
                t = threading.Thread(target=lambda: None)
                t.start()
                t.join()
                l = enum()
                self.assertFalse(t in l,
                    "#1703448 triggered after %d trials: %s" % (i, l))
        finally:
            sys.setcheckinterval(old_interval)


class ThreadJoinOnShutdown(unittest.TestCase):

    def _run_and_join(self, script):
        script = """if 1:
            import sys, os, time, threading

            # a thread, which waits for the main program to terminate
            def joiningfunc(mainthread):
                mainthread.join()
                print 'end of thread'
        \n""" + script

        import subprocess
        p = subprocess.Popen([sys.executable, "-c", script], stdout=subprocess.PIPE)
        rc = p.wait()
        data = p.stdout.read().replace('\r', '')
        self.assertEqual(data, "end of main\nend of thread\n")
        self.failIf(rc == 2, "interpreter was blocked")
        self.failUnless(rc == 0, "Unexpected error")

    def test_1_join_on_shutdown(self):
        # The usual case: on exit, wait for a non-daemon thread
        script = """if 1:
            import os
            t = threading.Thread(target=joiningfunc,
                                 args=(threading.currentThread(),))
            t.start()
            time.sleep(0.1)
            print 'end of main'
            """
        self._run_and_join(script)


    def test_2_join_in_forked_process(self):
        # Like the test above, but from a forked interpreter
        import os
        if not hasattr(os, 'fork'):
            return
        script = """if 1:
            childpid = os.fork()
            if childpid != 0:
                os.waitpid(childpid, 0)
                sys.exit(0)

            t = threading.Thread(target=joiningfunc,
                                 args=(threading.currentThread(),))
            t.start()
            print 'end of main'
            """
        self._run_and_join(script)

    def test_3_join_in_forked_from_thread(self):
        # Like the test above, but fork() was called from a worker thread
        # In the forked process, the main Thread object must be marked as stopped.
        import os
        if not hasattr(os, 'fork'):
            return
        # Skip platforms with known problems forking from a worker thread.
        # See http://bugs.python.org/issue3863.
        if sys.platform in ('freebsd4', 'freebsd5', 'freebsd6', 'os2emx'):
            print >>sys.stderr, ('Skipping test_3_join_in_forked_from_thread'
                                 ' due to known OS bugs on'), sys.platform
            return
        script = """if 1:
            main_thread = threading.currentThread()
            def worker():
                childpid = os.fork()
                if childpid != 0:
                    os.waitpid(childpid, 0)
                    sys.exit(0)

                t = threading.Thread(target=joiningfunc,
                                     args=(main_thread,))
                print 'end of main'
                t.start()
                t.join() # Should not block: main_thread is already stopped

            w = threading.Thread(target=worker)
            w.start()
            """
        self._run_and_join(script)


def test_main():
    test.test_support.run_unittest(ThreadTests,
                                   ThreadJoinOnShutdown)

if __name__ == "__main__":
    test_main()
