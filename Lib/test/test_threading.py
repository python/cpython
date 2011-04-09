# Very rudimentary test of threading module

import test.test_support
from test.test_support import verbose
import random
import re
import sys
thread = test.test_support.import_module('thread')
threading = test.test_support.import_module('threading')
import time
import unittest
import weakref
import os
import subprocess

from test import lock_tests

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
            print 'task %s will run for %.1f usec' % (
                self.name, delay * 1e6)

        with self.sema:
            with self.mutex:
                self.nrunning.inc()
                if verbose:
                    print self.nrunning.get(), 'tasks are running'
                self.testcase.assertTrue(self.nrunning.get() <= 3)

            time.sleep(delay)
            if verbose:
                print 'task', self.name, 'done'

            with self.mutex:
                self.nrunning.dec()
                self.testcase.assertTrue(self.nrunning.get() >= 0)
                if verbose:
                    print '%s is finished. %d tasks are running' % (
                        self.name, self.nrunning.get())

class BaseTestCase(unittest.TestCase):
    def setUp(self):
        self._threads = test.test_support.threading_setup()

    def tearDown(self):
        test.test_support.threading_cleanup(*self._threads)
        test.test_support.reap_children()


class ThreadTests(BaseTestCase):

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
            self.assertEqual(t.ident, None)
            self.assertTrue(re.match('<TestThread\(.*, initial\)>', repr(t)))
            t.start()

        if verbose:
            print 'waiting for all tasks to complete'
        for t in threads:
            t.join(NUMTASKS)
            self.assertTrue(not t.is_alive())
            self.assertNotEqual(t.ident, 0)
            self.assertFalse(t.ident is None)
            self.assertTrue(re.match('<TestThread\(.*, \w+ -?\d+\)>', repr(t)))
        if verbose:
            print 'all tasks done'
        self.assertEqual(numrunning.get(), 0)

    def test_ident_of_no_threading_threads(self):
        # The ident still must work for the main thread and dummy threads.
        self.assertFalse(threading.currentThread().ident is None)
        def f():
            ident.append(threading.currentThread().ident)
            done.set()
        done = threading.Event()
        ident = []
        thread.start_new_thread(f, ())
        done.wait()
        self.assertFalse(ident[0] is None)
        # Kill the "immortal" _DummyThread
        del threading._active[ident[0]]

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
            # Calling current_thread() forces an entry for the foreign
            # thread to get made in the threading._active map.
            threading.current_thread()
            mutex.release()

        mutex = threading.Lock()
        mutex.acquire()
        tid = thread.start_new_thread(f, (mutex,))
        # Wait for the thread to finish.
        mutex.acquire()
        self.assertIn(tid, threading._active)
        self.assertIsInstance(threading._active[tid], threading._DummyThread)
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

        # First check it works when setting the exception from the same thread.
        tid = thread.get_ident()

        try:
            result = set_async_exc(ctypes.c_long(tid), exception)
            # The exception is async, so we might have to keep the VM busy until
            # it notices.
            while True:
                pass
        except AsyncExc:
            pass
        else:
            # This code is unreachable but it reflects the intent. If we wanted
            # to be smarter the above loop wouldn't be infinite.
            self.fail("AsyncExc not raised")
        try:
            self.assertEqual(result, 1) # one thread state modified
        except UnboundLocalError:
            # The exception was raised too quickly for us to get the result.
            pass

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
        t.daemon = True # so if this fails, we don't hang Python at shutdown
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
        ret = worker_started.wait()
        self.assertTrue(ret)
        if verbose:
            print "    verifying worker hasn't exited"
        self.assertTrue(not t.finished)
        if verbose:
            print "    attempting to raise asynch exception in worker"
        result = set_async_exc(ctypes.c_long(t.id), exception)
        self.assertEqual(result, 1) # one thread state modified
        if verbose:
            print "    waiting for worker to say it caught the exception"
        worker_saw_exception.wait(timeout=10)
        self.assertTrue(t.finished)
        if verbose:
            print "    all OK -- joining worker"
        if t.finished:
            t.join()
        # else the thread is still running, and we have no way to kill it

    def test_limbo_cleanup(self):
        # Issue 7481: Failure to start thread should cleanup the limbo map.
        def fail_new_thread(*args):
            raise thread.error()
        _start_new_thread = threading._start_new_thread
        threading._start_new_thread = fail_new_thread
        try:
            t = threading.Thread(target=lambda: None)
            self.assertRaises(thread.error, t.start)
            self.assertFalse(
                t in threading._limbo,
                "Failed to cleanup _limbo map on failure of Thread.start().")
        finally:
            threading._start_new_thread = _start_new_thread

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
        p = subprocess.Popen([sys.executable, "-c", """if 1:
            import sys, threading

            # A deadlock-killer, to prevent the
            # testsuite to hang forever
            def killer():
                import os, time
                time.sleep(2)
                print 'program blocked; aborting'
                os._exit(2)
            t = threading.Thread(target=killer)
            t.daemon = True
            t.start()

            # This is the trace function
            def func(frame, event, arg):
                threading.current_thread()
                return func

            sys.settrace(func)
            """],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        stdout, stderr = p.communicate()
        rc = p.returncode
        self.assertFalse(rc == 2, "interpreted was blocked")
        self.assertTrue(rc == 0,
                        "Unexpected error: " + repr(stderr))

    def test_join_nondaemon_on_shutdown(self):
        # Issue 1722344
        # Raising SystemExit skipped threading._shutdown
        p = subprocess.Popen([sys.executable, "-c", """if 1:
                import threading
                from time import sleep

                def child():
                    sleep(1)
                    # As a non-daemon thread we SHOULD wake up and nothing
                    # should be torn down yet
                    print "Woke up, sleep function is:", sleep

                threading.Thread(target=child).start()
                raise SystemExit
            """],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        stdout, stderr = p.communicate()
        self.assertEqual(stdout.strip(),
            "Woke up, sleep function is: <built-in function sleep>")
        stderr = re.sub(r"^\[\d+ refs\]", "", stderr, re.MULTILINE).strip()
        self.assertEqual(stderr, "")

    def test_enumerate_after_join(self):
        # Try hard to trigger #1703448: a thread is still returned in
        # threading.enumerate() after it has been join()ed.
        enum = threading.enumerate
        old_interval = sys.getcheckinterval()
        try:
            for i in xrange(1, 100):
                # Try a couple times at each thread-switching interval
                # to get more interleavings.
                sys.setcheckinterval(i // 5)
                t = threading.Thread(target=lambda: None)
                t.start()
                t.join()
                l = enum()
                self.assertNotIn(t, l,
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
        self.assertEqual(None, weak_cyclic_object(),
                         msg=('%d references still around' %
                              sys.getrefcount(weak_cyclic_object())))

        raising_cyclic_object = RunSelfFunction(should_raise=True)
        weak_raising_cyclic_object = weakref.ref(raising_cyclic_object)
        raising_cyclic_object.thread.join()
        del raising_cyclic_object
        self.assertEqual(None, weak_raising_cyclic_object(),
                         msg=('%d references still around' %
                              sys.getrefcount(weak_raising_cyclic_object())))


class ThreadJoinOnShutdown(BaseTestCase):

    def _run_and_join(self, script):
        script = """if 1:
            import sys, os, time, threading

            # a thread, which waits for the main program to terminate
            def joiningfunc(mainthread):
                mainthread.join()
                print 'end of thread'
        \n""" + script

        p = subprocess.Popen([sys.executable, "-c", script], stdout=subprocess.PIPE)
        rc = p.wait()
        data = p.stdout.read().replace('\r', '')
        p.stdout.close()
        self.assertEqual(data, "end of main\nend of thread\n")
        self.assertFalse(rc == 2, "interpreter was blocked")
        self.assertTrue(rc == 0, "Unexpected error")

    def test_1_join_on_shutdown(self):
        # The usual case: on exit, wait for a non-daemon thread
        script = """if 1:
            import os
            t = threading.Thread(target=joiningfunc,
                                 args=(threading.current_thread(),))
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
                                 args=(threading.current_thread(),))
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
        if sys.platform in ('freebsd4', 'freebsd5', 'freebsd6', 'netbsd5',
                           'os2emx'):
            print >>sys.stderr, ('Skipping test_3_join_in_forked_from_thread'
                                 ' due to known OS bugs on'), sys.platform
            return
        script = """if 1:
            main_thread = threading.current_thread()
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

    def assertScriptHasOutput(self, script, expected_output):
        p = subprocess.Popen([sys.executable, "-c", script],
                             stdout=subprocess.PIPE)
        rc = p.wait()
        data = p.stdout.read().decode().replace('\r', '')
        self.assertEqual(rc, 0, "Unexpected error")
        self.assertEqual(data, expected_output)

    @unittest.skipUnless(hasattr(os, 'fork'), "needs os.fork()")
    def test_4_joining_across_fork_in_worker_thread(self):
        # There used to be a possible deadlock when forking from a child
        # thread.  See http://bugs.python.org/issue6643.

        # Skip platforms with known problems forking from a worker thread.
        # See http://bugs.python.org/issue3863.
        if sys.platform in ('freebsd4', 'freebsd5', 'freebsd6', 'os2emx'):
            raise unittest.SkipTest('due to known OS bugs on ' + sys.platform)

        # The script takes the following steps:
        # - The main thread in the parent process starts a new thread and then
        #   tries to join it.
        # - The join operation acquires the Lock inside the thread's _block
        #   Condition.  (See threading.py:Thread.join().)
        # - We stub out the acquire method on the condition to force it to wait
        #   until the child thread forks.  (See LOCK ACQUIRED HERE)
        # - The child thread forks.  (See LOCK HELD and WORKER THREAD FORKS
        #   HERE)
        # - The main thread of the parent process enters Condition.wait(),
        #   which releases the lock on the child thread.
        # - The child process returns.  Without the necessary fix, when the
        #   main thread of the child process (which used to be the child thread
        #   in the parent process) attempts to exit, it will try to acquire the
        #   lock in the Thread._block Condition object and hang, because the
        #   lock was held across the fork.

        script = """if 1:
            import os, time, threading

            finish_join = False
            start_fork = False

            def worker():
                # Wait until this thread's lock is acquired before forking to
                # create the deadlock.
                global finish_join
                while not start_fork:
                    time.sleep(0.01)
                # LOCK HELD: Main thread holds lock across this call.
                childpid = os.fork()
                finish_join = True
                if childpid != 0:
                    # Parent process just waits for child.
                    os.waitpid(childpid, 0)
                # Child process should just return.

            w = threading.Thread(target=worker)

            # Stub out the private condition variable's lock acquire method.
            # This acquires the lock and then waits until the child has forked
            # before returning, which will release the lock soon after.  If
            # someone else tries to fix this test case by acquiring this lock
            # before forking instead of resetting it, the test case will
            # deadlock when it shouldn't.
            condition = w._block
            orig_acquire = condition.acquire
            call_count_lock = threading.Lock()
            call_count = 0
            def my_acquire():
                global call_count
                global start_fork
                orig_acquire()  # LOCK ACQUIRED HERE
                start_fork = True
                if call_count == 0:
                    while not finish_join:
                        time.sleep(0.01)  # WORKER THREAD FORKS HERE
                with call_count_lock:
                    call_count += 1
            condition.acquire = my_acquire

            w.start()
            w.join()
            print('end of main')
            """
        self.assertScriptHasOutput(script, "end of main\n")

    @unittest.skipUnless(hasattr(os, 'fork'), "needs os.fork()")
    def test_5_clear_waiter_locks_to_avoid_crash(self):
        # Check that a spawned thread that forks doesn't segfault on certain
        # platforms, namely OS X.  This used to happen if there was a waiter
        # lock in the thread's condition variable's waiters list.  Even though
        # we know the lock will be held across the fork, it is not safe to
        # release locks held across forks on all platforms, so releasing the
        # waiter lock caused a segfault on OS X.  Furthermore, since locks on
        # OS X are (as of this writing) implemented with a mutex + condition
        # variable instead of a semaphore, while we know that the Python-level
        # lock will be acquired, we can't know if the internal mutex will be
        # acquired at the time of the fork.

        # Skip platforms with known problems forking from a worker thread.
        # See http://bugs.python.org/issue3863.
        if sys.platform in ('freebsd4', 'freebsd5', 'freebsd6', 'os2emx'):
            raise unittest.SkipTest('due to known OS bugs on ' + sys.platform)
        script = """if True:
            import os, time, threading

            start_fork = False

            def worker():
                # Wait until the main thread has attempted to join this thread
                # before continuing.
                while not start_fork:
                    time.sleep(0.01)
                childpid = os.fork()
                if childpid != 0:
                    # Parent process just waits for child.
                    (cpid, rc) = os.waitpid(childpid, 0)
                    assert cpid == childpid
                    assert rc == 0
                    print('end of worker thread')
                else:
                    # Child process should just return.
                    pass

            w = threading.Thread(target=worker)

            # Stub out the private condition variable's _release_save method.
            # This releases the condition's lock and flips the global that
            # causes the worker to fork.  At this point, the problematic waiter
            # lock has been acquired once by the waiter and has been put onto
            # the waiters list.
            condition = w._block
            orig_release_save = condition._release_save
            def my_release_save():
                global start_fork
                orig_release_save()
                # Waiter lock held here, condition lock released.
                start_fork = True
            condition._release_save = my_release_save

            w.start()
            w.join()
            print('end of main thread')
            """
        output = "end of worker thread\nend of main thread\n"
        self.assertScriptHasOutput(script, output)


class ThreadingExceptionTests(BaseTestCase):
    # A RuntimeError should be raised if Thread.start() is called
    # multiple times.
    def test_start_thread_again(self):
        thread = threading.Thread()
        thread.start()
        self.assertRaises(RuntimeError, thread.start)

    def test_joining_current_thread(self):
        current_thread = threading.current_thread()
        self.assertRaises(RuntimeError, current_thread.join);

    def test_joining_inactive_thread(self):
        thread = threading.Thread()
        self.assertRaises(RuntimeError, thread.join)

    def test_daemonize_active_thread(self):
        thread = threading.Thread()
        thread.start()
        self.assertRaises(RuntimeError, setattr, thread, "daemon", True)

    def test_recursion_limit(self):
        # Issue 9670
        # test that excessive recursion within a non-main thread causes
        # an exception rather than crashing the interpreter on platforms
        # like Mac OS X or FreeBSD which have small default stack sizes
        # for threads
        script = """if True:
            import threading

            def recurse():
                return recurse()

            def outer():
                try:
                    recurse()
                except RuntimeError:
                    pass

            w = threading.Thread(target=outer)
            w.start()
            w.join()
            print('end of main thread')
            """
        expected_output = "end of main thread\n"
        p = subprocess.Popen([sys.executable, "-c", script],
                             stdout=subprocess.PIPE)
        stdout, stderr = p.communicate()
        data = stdout.decode().replace('\r', '')
        self.assertEqual(p.returncode, 0, "Unexpected error")
        self.assertEqual(data, expected_output)

class LockTests(lock_tests.LockTests):
    locktype = staticmethod(threading.Lock)

class RLockTests(lock_tests.RLockTests):
    locktype = staticmethod(threading.RLock)

class EventTests(lock_tests.EventTests):
    eventtype = staticmethod(threading.Event)

class ConditionAsRLockTests(lock_tests.RLockTests):
    # An Condition uses an RLock by default and exports its API.
    locktype = staticmethod(threading.Condition)

class ConditionTests(lock_tests.ConditionTests):
    condtype = staticmethod(threading.Condition)

class SemaphoreTests(lock_tests.SemaphoreTests):
    semtype = staticmethod(threading.Semaphore)

class BoundedSemaphoreTests(lock_tests.BoundedSemaphoreTests):
    semtype = staticmethod(threading.BoundedSemaphore)


def test_main():
    test.test_support.run_unittest(LockTests, RLockTests, EventTests,
                                   ConditionAsRLockTests, ConditionTests,
                                   SemaphoreTests, BoundedSemaphoreTests,
                                   ThreadTests,
                                   ThreadJoinOnShutdown,
                                   ThreadingExceptionTests,
                                   )

if __name__ == "__main__":
    test_main()
