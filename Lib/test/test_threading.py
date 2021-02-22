"""
Tests for the threading module.
"""

import test.support
from test.support import threading_helper
from test.support import verbose, cpython_only
from test.support.import_helper import import_module
from test.support.script_helper import assert_python_ok, assert_python_failure

import random
import sys
import _thread
import threading
import time
import unittest
import weakref
import os
import subprocess
import signal
import textwrap

from unittest import mock
from test import lock_tests
from test import support


# Between fork() and exec(), only async-safe functions are allowed (issues
# #12316 and #11870), and fork() from a worker thread is known to trigger
# problems with some operating systems (issue #3863): skip problematic tests
# on platforms known to behave badly.
platforms_to_skip = ('netbsd5', 'hp-ux11')


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
                  (self.name, delay * 1e6))

        with self.sema:
            with self.mutex:
                self.nrunning.inc()
                if verbose:
                    print(self.nrunning.get(), 'tasks are running')
                self.testcase.assertLessEqual(self.nrunning.get(), 3)

            time.sleep(delay)
            if verbose:
                print('task', self.name, 'done')

            with self.mutex:
                self.nrunning.dec()
                self.testcase.assertGreaterEqual(self.nrunning.get(), 0)
                if verbose:
                    print('%s is finished. %d tasks are running' %
                          (self.name, self.nrunning.get()))


class BaseTestCase(unittest.TestCase):
    def setUp(self):
        self._threads = threading_helper.threading_setup()

    def tearDown(self):
        threading_helper.threading_cleanup(*self._threads)
        test.support.reap_children()


class ThreadTests(BaseTestCase):

    @cpython_only
    def test_name(self):
        def func(): pass

        thread = threading.Thread(name="myname1")
        self.assertEqual(thread.name, "myname1")

        # Convert int name to str
        thread = threading.Thread(name=123)
        self.assertEqual(thread.name, "123")

        # target name is ignored if name is specified
        thread = threading.Thread(target=func, name="myname2")
        self.assertEqual(thread.name, "myname2")

        with mock.patch.object(threading, '_counter', return_value=2):
            thread = threading.Thread(name="")
            self.assertEqual(thread.name, "Thread-2")

        with mock.patch.object(threading, '_counter', return_value=3):
            thread = threading.Thread()
            self.assertEqual(thread.name, "Thread-3")

        with mock.patch.object(threading, '_counter', return_value=5):
            thread = threading.Thread(target=func)
            self.assertEqual(thread.name, "Thread-5 (func)")

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
            self.assertIsNone(t.ident)
            self.assertRegex(repr(t), r'^<TestThread\(.*, initial\)>$')
            t.start()

        if hasattr(threading, 'get_native_id'):
            native_ids = set(t.native_id for t in threads) | {threading.get_native_id()}
            self.assertNotIn(None, native_ids)
            self.assertEqual(len(native_ids), NUMTASKS + 1)

        if verbose:
            print('waiting for all tasks to complete')
        for t in threads:
            t.join()
            self.assertFalse(t.is_alive())
            self.assertNotEqual(t.ident, 0)
            self.assertIsNotNone(t.ident)
            self.assertRegex(repr(t), r'^<TestThread\(.*, stopped -?\d+\)>$')
        if verbose:
            print('all tasks done')
        self.assertEqual(numrunning.get(), 0)

    def test_ident_of_no_threading_threads(self):
        # The ident still must work for the main thread and dummy threads.
        self.assertIsNotNone(threading.currentThread().ident)
        def f():
            ident.append(threading.currentThread().ident)
            done.set()
        done = threading.Event()
        ident = []
        with threading_helper.wait_threads_exit():
            tid = _thread.start_new_thread(f, ())
            done.wait()
            self.assertEqual(ident[0], tid)
        # Kill the "immortal" _DummyThread
        del threading._active[ident[0]]

    # run with a small(ish) thread stack size (256 KiB)
    def test_various_ops_small_stack(self):
        if verbose:
            print('with 256 KiB thread stack size...')
        try:
            threading.stack_size(262144)
        except _thread.error:
            raise unittest.SkipTest(
                'platform does not support changing thread stack size')
        self.test_various_ops()
        threading.stack_size(0)

    # run with a large thread stack size (1 MiB)
    def test_various_ops_large_stack(self):
        if verbose:
            print('with 1 MiB thread stack size...')
        try:
            threading.stack_size(0x100000)
        except _thread.error:
            raise unittest.SkipTest(
                'platform does not support changing thread stack size')
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
        with threading_helper.wait_threads_exit():
            tid = _thread.start_new_thread(f, (mutex,))
            # Wait for the thread to finish.
            mutex.acquire()
        self.assertIn(tid, threading._active)
        self.assertIsInstance(threading._active[tid], threading._DummyThread)
        #Issue 29376
        self.assertTrue(threading._active[tid].is_alive())
        self.assertRegex(repr(threading._active[tid]), '_DummyThread')
        del threading._active[tid]

    # PyThreadState_SetAsyncExc() is a CPython-only gimmick, not (currently)
    # exposed at the Python level.  This test relies on ctypes to get at it.
    def test_PyThreadState_SetAsyncExc(self):
        ctypes = import_module("ctypes")

        set_async_exc = ctypes.pythonapi.PyThreadState_SetAsyncExc
        set_async_exc.argtypes = (ctypes.c_ulong, ctypes.py_object)

        class AsyncExc(Exception):
            pass

        exception = ctypes.py_object(AsyncExc)

        # First check it works when setting the exception from the same thread.
        tid = threading.get_ident()
        self.assertIsInstance(tid, int)
        self.assertGreater(tid, 0)

        try:
            result = set_async_exc(tid, exception)
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
                self.id = threading.get_ident()
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
            print("    started worker thread")

        # Try a thread id that doesn't make sense.
        if verbose:
            print("    trying nonsensical thread id")
        result = set_async_exc(-1, exception)
        self.assertEqual(result, 0)  # no thread states modified

        # Now raise an exception in the worker thread.
        if verbose:
            print("    waiting for worker thread to get started")
        ret = worker_started.wait()
        self.assertTrue(ret)
        if verbose:
            print("    verifying worker hasn't exited")
        self.assertFalse(t.finished)
        if verbose:
            print("    attempting to raise asynch exception in worker")
        result = set_async_exc(t.id, exception)
        self.assertEqual(result, 1) # one thread state modified
        if verbose:
            print("    waiting for worker to say it caught the exception")
        worker_saw_exception.wait(timeout=support.SHORT_TIMEOUT)
        self.assertTrue(t.finished)
        if verbose:
            print("    all OK -- joining worker")
        if t.finished:
            t.join()
        # else the thread is still running, and we have no way to kill it

    def test_limbo_cleanup(self):
        # Issue 7481: Failure to start thread should cleanup the limbo map.
        def fail_new_thread(*args):
            raise threading.ThreadError()
        _start_new_thread = threading._start_new_thread
        threading._start_new_thread = fail_new_thread
        try:
            t = threading.Thread(target=lambda: None)
            self.assertRaises(threading.ThreadError, t.start)
            self.assertFalse(
                t in threading._limbo,
                "Failed to cleanup _limbo map on failure of Thread.start().")
        finally:
            threading._start_new_thread = _start_new_thread

    def test_finalize_running_thread(self):
        # Issue 1402: the PyGILState_Ensure / _Release functions may be called
        # very late on python exit: on deallocation of a running thread for
        # example.
        import_module("ctypes")

        rc, out, err = assert_python_failure("-c", """if 1:
            import ctypes, sys, time, _thread

            # This lock is used as a simple event variable.
            ready = _thread.allocate_lock()
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

            _thread.start_new_thread(waitingThread, ())
            ready.acquire()  # Be sure the other thread is waiting.
            sys.exit(42)
            """)
        self.assertEqual(rc, 42)

    def test_finalize_with_trace(self):
        # Issue1733757
        # Avoid a deadlock when sys.settrace steps into threading._shutdown
        assert_python_ok("-c", """if 1:
            import sys, threading

            # A deadlock-killer, to prevent the
            # testsuite to hang forever
            def killer():
                import os, time
                time.sleep(2)
                print('program blocked; aborting')
                os._exit(2)
            t = threading.Thread(target=killer)
            t.daemon = True
            t.start()

            # This is the trace function
            def func(frame, event, arg):
                threading.current_thread()
                return func

            sys.settrace(func)
            """)

    def test_join_nondaemon_on_shutdown(self):
        # Issue 1722344
        # Raising SystemExit skipped threading._shutdown
        rc, out, err = assert_python_ok("-c", """if 1:
                import threading
                from time import sleep

                def child():
                    sleep(1)
                    # As a non-daemon thread we SHOULD wake up and nothing
                    # should be torn down yet
                    print("Woke up, sleep function is:", sleep)

                threading.Thread(target=child).start()
                raise SystemExit
            """)
        self.assertEqual(out.strip(),
            b"Woke up, sleep function is: <built-in function sleep>")
        self.assertEqual(err, b"")

    def test_enumerate_after_join(self):
        # Try hard to trigger #1703448: a thread is still returned in
        # threading.enumerate() after it has been join()ed.
        enum = threading.enumerate
        old_interval = sys.getswitchinterval()
        try:
            for i in range(1, 100):
                sys.setswitchinterval(i * 0.0002)
                t = threading.Thread(target=lambda: None)
                t.start()
                t.join()
                l = enum()
                self.assertNotIn(t, l,
                    "#1703448 triggered after %d trials: %s" % (i, l))
        finally:
            sys.setswitchinterval(old_interval)

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
        self.assertIsNone(weak_cyclic_object(),
                         msg=('%d references still around' %
                              sys.getrefcount(weak_cyclic_object())))

        raising_cyclic_object = RunSelfFunction(should_raise=True)
        weak_raising_cyclic_object = weakref.ref(raising_cyclic_object)
        raising_cyclic_object.thread.join()
        del raising_cyclic_object
        self.assertIsNone(weak_raising_cyclic_object(),
                         msg=('%d references still around' %
                              sys.getrefcount(weak_raising_cyclic_object())))

    def test_old_threading_api(self):
        # Just a quick sanity check to make sure the old method names are
        # still present
        t = threading.Thread()
        t.isDaemon()
        t.setDaemon(True)
        t.getName()
        t.setName("name")
        e = threading.Event()
        e.isSet()
        threading.activeCount()

    def test_repr_daemon(self):
        t = threading.Thread()
        self.assertNotIn('daemon', repr(t))
        t.daemon = True
        self.assertIn('daemon', repr(t))

    def test_daemon_param(self):
        t = threading.Thread()
        self.assertFalse(t.daemon)
        t = threading.Thread(daemon=False)
        self.assertFalse(t.daemon)
        t = threading.Thread(daemon=True)
        self.assertTrue(t.daemon)

    @unittest.skipUnless(hasattr(os, 'fork'), 'needs os.fork()')
    def test_fork_at_exit(self):
        # bpo-42350: Calling os.fork() after threading._shutdown() must
        # not log an error.
        code = textwrap.dedent("""
            import atexit
            import os
            import sys
            from test.support import wait_process

            # Import the threading module to register its "at fork" callback
            import threading

            def exit_handler():
                pid = os.fork()
                if not pid:
                    print("child process ok", file=sys.stderr, flush=True)
                    # child process
                else:
                    wait_process(pid, exitcode=0)

            # exit_handler() will be called after threading._shutdown()
            atexit.register(exit_handler)
        """)
        _, out, err = assert_python_ok("-c", code)
        self.assertEqual(out, b'')
        self.assertEqual(err.rstrip(), b'child process ok')

    @unittest.skipUnless(hasattr(os, 'fork'), 'test needs fork()')
    def test_dummy_thread_after_fork(self):
        # Issue #14308: a dummy thread in the active list doesn't mess up
        # the after-fork mechanism.
        code = """if 1:
            import _thread, threading, os, time

            def background_thread(evt):
                # Creates and registers the _DummyThread instance
                threading.current_thread()
                evt.set()
                time.sleep(10)

            evt = threading.Event()
            _thread.start_new_thread(background_thread, (evt,))
            evt.wait()
            assert threading.active_count() == 2, threading.active_count()
            if os.fork() == 0:
                assert threading.active_count() == 1, threading.active_count()
                os._exit(0)
            else:
                os.wait()
        """
        _, out, err = assert_python_ok("-c", code)
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')

    @unittest.skipUnless(hasattr(os, 'fork'), "needs os.fork()")
    def test_is_alive_after_fork(self):
        # Try hard to trigger #18418: is_alive() could sometimes be True on
        # threads that vanished after a fork.
        old_interval = sys.getswitchinterval()
        self.addCleanup(sys.setswitchinterval, old_interval)

        # Make the bug more likely to manifest.
        test.support.setswitchinterval(1e-6)

        for i in range(20):
            t = threading.Thread(target=lambda: None)
            t.start()
            pid = os.fork()
            if pid == 0:
                os._exit(11 if t.is_alive() else 10)
            else:
                t.join()

                support.wait_process(pid, exitcode=10)

    def test_main_thread(self):
        main = threading.main_thread()
        self.assertEqual(main.name, 'MainThread')
        self.assertEqual(main.ident, threading.current_thread().ident)
        self.assertEqual(main.ident, threading.get_ident())

        def f():
            self.assertNotEqual(threading.main_thread().ident,
                                threading.current_thread().ident)
        th = threading.Thread(target=f)
        th.start()
        th.join()

    @unittest.skipUnless(hasattr(os, 'fork'), "test needs os.fork()")
    @unittest.skipUnless(hasattr(os, 'waitpid'), "test needs os.waitpid()")
    def test_main_thread_after_fork(self):
        code = """if 1:
            import os, threading
            from test import support

            pid = os.fork()
            if pid == 0:
                main = threading.main_thread()
                print(main.name)
                print(main.ident == threading.current_thread().ident)
                print(main.ident == threading.get_ident())
            else:
                support.wait_process(pid, exitcode=0)
        """
        _, out, err = assert_python_ok("-c", code)
        data = out.decode().replace('\r', '')
        self.assertEqual(err, b"")
        self.assertEqual(data, "MainThread\nTrue\nTrue\n")

    @unittest.skipIf(sys.platform in platforms_to_skip, "due to known OS bug")
    @unittest.skipUnless(hasattr(os, 'fork'), "test needs os.fork()")
    @unittest.skipUnless(hasattr(os, 'waitpid'), "test needs os.waitpid()")
    def test_main_thread_after_fork_from_nonmain_thread(self):
        code = """if 1:
            import os, threading, sys
            from test import support

            def func():
                pid = os.fork()
                if pid == 0:
                    main = threading.main_thread()
                    print(main.name)
                    print(main.ident == threading.current_thread().ident)
                    print(main.ident == threading.get_ident())
                    # stdout is fully buffered because not a tty,
                    # we have to flush before exit.
                    sys.stdout.flush()
                else:
                    support.wait_process(pid, exitcode=0)

            th = threading.Thread(target=func)
            th.start()
            th.join()
        """
        _, out, err = assert_python_ok("-c", code)
        data = out.decode().replace('\r', '')
        self.assertEqual(err, b"")
        self.assertEqual(data, "Thread-1 (func)\nTrue\nTrue\n")

    def test_main_thread_during_shutdown(self):
        # bpo-31516: current_thread() should still point to the main thread
        # at shutdown
        code = """if 1:
            import gc, threading

            main_thread = threading.current_thread()
            assert main_thread is threading.main_thread()  # sanity check

            class RefCycle:
                def __init__(self):
                    self.cycle = self

                def __del__(self):
                    print("GC:",
                          threading.current_thread() is main_thread,
                          threading.main_thread() is main_thread,
                          threading.enumerate() == [main_thread])

            RefCycle()
            gc.collect()  # sanity check
            x = RefCycle()
        """
        _, out, err = assert_python_ok("-c", code)
        data = out.decode()
        self.assertEqual(err, b"")
        self.assertEqual(data.splitlines(),
                         ["GC: True True True"] * 2)

    def test_finalization_shutdown(self):
        # bpo-36402: Py_Finalize() calls threading._shutdown() which must wait
        # until Python thread states of all non-daemon threads get deleted.
        #
        # Test similar to SubinterpThreadingTests.test_threads_join_2(), but
        # test the finalization of the main interpreter.
        code = """if 1:
            import os
            import threading
            import time
            import random

            def random_sleep():
                seconds = random.random() * 0.010
                time.sleep(seconds)

            class Sleeper:
                def __del__(self):
                    random_sleep()

            tls = threading.local()

            def f():
                # Sleep a bit so that the thread is still running when
                # Py_Finalize() is called.
                random_sleep()
                tls.x = Sleeper()
                random_sleep()

            threading.Thread(target=f).start()
            random_sleep()
        """
        rc, out, err = assert_python_ok("-c", code)
        self.assertEqual(err, b"")

    def test_tstate_lock(self):
        # Test an implementation detail of Thread objects.
        started = _thread.allocate_lock()
        finish = _thread.allocate_lock()
        started.acquire()
        finish.acquire()
        def f():
            started.release()
            finish.acquire()
            time.sleep(0.01)
        # The tstate lock is None until the thread is started
        t = threading.Thread(target=f)
        self.assertIs(t._tstate_lock, None)
        t.start()
        started.acquire()
        self.assertTrue(t.is_alive())
        # The tstate lock can't be acquired when the thread is running
        # (or suspended).
        tstate_lock = t._tstate_lock
        self.assertFalse(tstate_lock.acquire(timeout=0), False)
        finish.release()
        # When the thread ends, the state_lock can be successfully
        # acquired.
        self.assertTrue(tstate_lock.acquire(timeout=support.SHORT_TIMEOUT), False)
        # But is_alive() is still True:  we hold _tstate_lock now, which
        # prevents is_alive() from knowing the thread's end-of-life C code
        # is done.
        self.assertTrue(t.is_alive())
        # Let is_alive() find out the C code is done.
        tstate_lock.release()
        self.assertFalse(t.is_alive())
        # And verify the thread disposed of _tstate_lock.
        self.assertIsNone(t._tstate_lock)
        t.join()

    def test_repr_stopped(self):
        # Verify that "stopped" shows up in repr(Thread) appropriately.
        started = _thread.allocate_lock()
        finish = _thread.allocate_lock()
        started.acquire()
        finish.acquire()
        def f():
            started.release()
            finish.acquire()
        t = threading.Thread(target=f)
        t.start()
        started.acquire()
        self.assertIn("started", repr(t))
        finish.release()
        # "stopped" should appear in the repr in a reasonable amount of time.
        # Implementation detail:  as of this writing, that's trivially true
        # if .join() is called, and almost trivially true if .is_alive() is
        # called.  The detail we're testing here is that "stopped" shows up
        # "all on its own".
        LOOKING_FOR = "stopped"
        for i in range(500):
            if LOOKING_FOR in repr(t):
                break
            time.sleep(0.01)
        self.assertIn(LOOKING_FOR, repr(t)) # we waited at least 5 seconds
        t.join()

    def test_BoundedSemaphore_limit(self):
        # BoundedSemaphore should raise ValueError if released too often.
        for limit in range(1, 10):
            bs = threading.BoundedSemaphore(limit)
            threads = [threading.Thread(target=bs.acquire)
                       for _ in range(limit)]
            for t in threads:
                t.start()
            for t in threads:
                t.join()
            threads = [threading.Thread(target=bs.release)
                       for _ in range(limit)]
            for t in threads:
                t.start()
            for t in threads:
                t.join()
            self.assertRaises(ValueError, bs.release)

    @cpython_only
    def test_frame_tstate_tracing(self):
        # Issue #14432: Crash when a generator is created in a C thread that is
        # destroyed while the generator is still used. The issue was that a
        # generator contains a frame, and the frame kept a reference to the
        # Python state of the destroyed C thread. The crash occurs when a trace
        # function is setup.

        def noop_trace(frame, event, arg):
            # no operation
            return noop_trace

        def generator():
            while 1:
                yield "generator"

        def callback():
            if callback.gen is None:
                callback.gen = generator()
            return next(callback.gen)
        callback.gen = None

        old_trace = sys.gettrace()
        sys.settrace(noop_trace)
        try:
            # Install a trace function
            threading.settrace(noop_trace)

            # Create a generator in a C thread which exits after the call
            import _testcapi
            _testcapi.call_in_temporary_c_thread(callback)

            # Call the generator in a different Python thread, check that the
            # generator didn't keep a reference to the destroyed thread state
            for test in range(3):
                # The trace function is still called here
                callback()
        finally:
            sys.settrace(old_trace)

    def test_gettrace(self):
        def noop_trace(frame, event, arg):
            # no operation
            return noop_trace
        old_trace = threading.gettrace()
        try:
            threading.settrace(noop_trace)
            trace_func = threading.gettrace()
            self.assertEqual(noop_trace,trace_func)
        finally:
            threading.settrace(old_trace)

    def test_getprofile(self):
        def fn(*args): pass
        old_profile = threading.getprofile()
        try:
            threading.setprofile(fn)
            self.assertEqual(fn, threading.getprofile())
        finally:
            threading.setprofile(old_profile)

    @cpython_only
    def test_shutdown_locks(self):
        for daemon in (False, True):
            with self.subTest(daemon=daemon):
                event = threading.Event()
                thread = threading.Thread(target=event.wait, daemon=daemon)

                # Thread.start() must add lock to _shutdown_locks,
                # but only for non-daemon thread
                thread.start()
                tstate_lock = thread._tstate_lock
                if not daemon:
                    self.assertIn(tstate_lock, threading._shutdown_locks)
                else:
                    self.assertNotIn(tstate_lock, threading._shutdown_locks)

                # unblock the thread and join it
                event.set()
                thread.join()

                # Thread._stop() must remove tstate_lock from _shutdown_locks.
                # Daemon threads must never add it to _shutdown_locks.
                self.assertNotIn(tstate_lock, threading._shutdown_locks)

    def test_locals_at_exit(self):
        # bpo-19466: thread locals must not be deleted before destructors
        # are called
        rc, out, err = assert_python_ok("-c", """if 1:
            import threading

            class Atexit:
                def __del__(self):
                    print("thread_dict.atexit = %r" % thread_dict.atexit)

            thread_dict = threading.local()
            thread_dict.atexit = "value"

            atexit = Atexit()
        """)
        self.assertEqual(out.rstrip(), b"thread_dict.atexit = 'value'")

    def test_boolean_target(self):
        # bpo-41149: A thread that had a boolean value of False would not
        # run, regardless of whether it was callable. The correct behaviour
        # is for a thread to do nothing if its target is None, and to call
        # the target otherwise.
        class BooleanTarget(object):
            def __init__(self):
                self.ran = False
            def __bool__(self):
                return False
            def __call__(self):
                self.ran = True

        target = BooleanTarget()
        thread = threading.Thread(target=target)
        thread.start()
        thread.join()
        self.assertTrue(target.ran)



class ThreadJoinOnShutdown(BaseTestCase):

    def _run_and_join(self, script):
        script = """if 1:
            import sys, os, time, threading

            # a thread, which waits for the main program to terminate
            def joiningfunc(mainthread):
                mainthread.join()
                print('end of thread')
                # stdout is fully buffered because not a tty, we have to flush
                # before exit.
                sys.stdout.flush()
        \n""" + script

        rc, out, err = assert_python_ok("-c", script)
        data = out.decode().replace('\r', '')
        self.assertEqual(data, "end of main\nend of thread\n")

    def test_1_join_on_shutdown(self):
        # The usual case: on exit, wait for a non-daemon thread
        script = """if 1:
            import os
            t = threading.Thread(target=joiningfunc,
                                 args=(threading.current_thread(),))
            t.start()
            time.sleep(0.1)
            print('end of main')
            """
        self._run_and_join(script)

    @unittest.skipUnless(hasattr(os, 'fork'), "needs os.fork()")
    @unittest.skipIf(sys.platform in platforms_to_skip, "due to known OS bug")
    def test_2_join_in_forked_process(self):
        # Like the test above, but from a forked interpreter
        script = """if 1:
            from test import support

            childpid = os.fork()
            if childpid != 0:
                # parent process
                support.wait_process(childpid, exitcode=0)
                sys.exit(0)

            # child process
            t = threading.Thread(target=joiningfunc,
                                 args=(threading.current_thread(),))
            t.start()
            print('end of main')
            """
        self._run_and_join(script)

    @unittest.skipUnless(hasattr(os, 'fork'), "needs os.fork()")
    @unittest.skipIf(sys.platform in platforms_to_skip, "due to known OS bug")
    def test_3_join_in_forked_from_thread(self):
        # Like the test above, but fork() was called from a worker thread
        # In the forked process, the main Thread object must be marked as stopped.

        script = """if 1:
            from test import support

            main_thread = threading.current_thread()
            def worker():
                childpid = os.fork()
                if childpid != 0:
                    # parent process
                    support.wait_process(childpid, exitcode=0)
                    sys.exit(0)

                # child process
                t = threading.Thread(target=joiningfunc,
                                     args=(main_thread,))
                print('end of main')
                t.start()
                t.join() # Should not block: main_thread is already stopped

            w = threading.Thread(target=worker)
            w.start()
            """
        self._run_and_join(script)

    @unittest.skipIf(sys.platform in platforms_to_skip, "due to known OS bug")
    def test_4_daemon_threads(self):
        # Check that a daemon thread cannot crash the interpreter on shutdown
        # by manipulating internal structures that are being disposed of in
        # the main thread.
        script = """if True:
            import os
            import random
            import sys
            import time
            import threading

            thread_has_run = set()

            def random_io():
                '''Loop for a while sleeping random tiny amounts and doing some I/O.'''
                while True:
                    with open(os.__file__, 'rb') as in_f:
                        stuff = in_f.read(200)
                        with open(os.devnull, 'wb') as null_f:
                            null_f.write(stuff)
                            time.sleep(random.random() / 1995)
                    thread_has_run.add(threading.current_thread())

            def main():
                count = 0
                for _ in range(40):
                    new_thread = threading.Thread(target=random_io)
                    new_thread.daemon = True
                    new_thread.start()
                    count += 1
                while len(thread_has_run) < count:
                    time.sleep(0.001)
                # Trigger process shutdown
                sys.exit(0)

            main()
            """
        rc, out, err = assert_python_ok('-c', script)
        self.assertFalse(err)

    @unittest.skipUnless(hasattr(os, 'fork'), "needs os.fork()")
    @unittest.skipIf(sys.platform in platforms_to_skip, "due to known OS bug")
    def test_reinit_tls_after_fork(self):
        # Issue #13817: fork() would deadlock in a multithreaded program with
        # the ad-hoc TLS implementation.

        def do_fork_and_wait():
            # just fork a child process and wait it
            pid = os.fork()
            if pid > 0:
                support.wait_process(pid, exitcode=50)
            else:
                os._exit(50)

        # start a bunch of threads that will fork() child processes
        threads = []
        for i in range(16):
            t = threading.Thread(target=do_fork_and_wait)
            threads.append(t)
            t.start()

        for t in threads:
            t.join()

    @unittest.skipUnless(hasattr(os, 'fork'), "needs os.fork()")
    def test_clear_threads_states_after_fork(self):
        # Issue #17094: check that threads states are cleared after fork()

        # start a bunch of threads
        threads = []
        for i in range(16):
            t = threading.Thread(target=lambda : time.sleep(0.3))
            threads.append(t)
            t.start()

        pid = os.fork()
        if pid == 0:
            # check that threads states have been cleared
            if len(sys._current_frames()) == 1:
                os._exit(51)
            else:
                os._exit(52)
        else:
            support.wait_process(pid, exitcode=51)

        for t in threads:
            t.join()


class SubinterpThreadingTests(BaseTestCase):
    def pipe(self):
        r, w = os.pipe()
        self.addCleanup(os.close, r)
        self.addCleanup(os.close, w)
        if hasattr(os, 'set_blocking'):
            os.set_blocking(r, False)
        return (r, w)

    def test_threads_join(self):
        # Non-daemon threads should be joined at subinterpreter shutdown
        # (issue #18808)
        r, w = self.pipe()
        code = textwrap.dedent(r"""
            import os
            import random
            import threading
            import time

            def random_sleep():
                seconds = random.random() * 0.010
                time.sleep(seconds)

            def f():
                # Sleep a bit so that the thread is still running when
                # Py_EndInterpreter is called.
                random_sleep()
                os.write(%d, b"x")

            threading.Thread(target=f).start()
            random_sleep()
        """ % (w,))
        ret = test.support.run_in_subinterp(code)
        self.assertEqual(ret, 0)
        # The thread was joined properly.
        self.assertEqual(os.read(r, 1), b"x")

    def test_threads_join_2(self):
        # Same as above, but a delay gets introduced after the thread's
        # Python code returned but before the thread state is deleted.
        # To achieve this, we register a thread-local object which sleeps
        # a bit when deallocated.
        r, w = self.pipe()
        code = textwrap.dedent(r"""
            import os
            import random
            import threading
            import time

            def random_sleep():
                seconds = random.random() * 0.010
                time.sleep(seconds)

            class Sleeper:
                def __del__(self):
                    random_sleep()

            tls = threading.local()

            def f():
                # Sleep a bit so that the thread is still running when
                # Py_EndInterpreter is called.
                random_sleep()
                tls.x = Sleeper()
                os.write(%d, b"x")

            threading.Thread(target=f).start()
            random_sleep()
        """ % (w,))
        ret = test.support.run_in_subinterp(code)
        self.assertEqual(ret, 0)
        # The thread was joined properly.
        self.assertEqual(os.read(r, 1), b"x")

    @cpython_only
    def test_daemon_threads_fatal_error(self):
        subinterp_code = f"""if 1:
            import os
            import threading
            import time

            def f():
                # Make sure the daemon thread is still running when
                # Py_EndInterpreter is called.
                time.sleep({test.support.SHORT_TIMEOUT})
            threading.Thread(target=f, daemon=True).start()
            """
        script = r"""if 1:
            import _testcapi

            _testcapi.run_in_subinterp(%r)
            """ % (subinterp_code,)
        with test.support.SuppressCrashReport():
            rc, out, err = assert_python_failure("-c", script)
        self.assertIn("Fatal Python error: Py_EndInterpreter: "
                      "not the last thread", err.decode())


class ThreadingExceptionTests(BaseTestCase):
    # A RuntimeError should be raised if Thread.start() is called
    # multiple times.
    def test_start_thread_again(self):
        thread = threading.Thread()
        thread.start()
        self.assertRaises(RuntimeError, thread.start)
        thread.join()

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
        thread.join()

    def test_releasing_unacquired_lock(self):
        lock = threading.Lock()
        self.assertRaises(RuntimeError, lock.release)

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
                except RecursionError:
                    pass

            w = threading.Thread(target=outer)
            w.start()
            w.join()
            print('end of main thread')
            """
        expected_output = "end of main thread\n"
        p = subprocess.Popen([sys.executable, "-c", script],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        data = stdout.decode().replace('\r', '')
        self.assertEqual(p.returncode, 0, "Unexpected error: " + stderr.decode())
        self.assertEqual(data, expected_output)

    def test_print_exception(self):
        script = r"""if True:
            import threading
            import time

            running = False
            def run():
                global running
                running = True
                while running:
                    time.sleep(0.01)
                1/0
            t = threading.Thread(target=run)
            t.start()
            while not running:
                time.sleep(0.01)
            running = False
            t.join()
            """
        rc, out, err = assert_python_ok("-c", script)
        self.assertEqual(out, b'')
        err = err.decode()
        self.assertIn("Exception in thread", err)
        self.assertIn("Traceback (most recent call last):", err)
        self.assertIn("ZeroDivisionError", err)
        self.assertNotIn("Unhandled exception", err)

    def test_print_exception_stderr_is_none_1(self):
        script = r"""if True:
            import sys
            import threading
            import time

            running = False
            def run():
                global running
                running = True
                while running:
                    time.sleep(0.01)
                1/0
            t = threading.Thread(target=run)
            t.start()
            while not running:
                time.sleep(0.01)
            sys.stderr = None
            running = False
            t.join()
            """
        rc, out, err = assert_python_ok("-c", script)
        self.assertEqual(out, b'')
        err = err.decode()
        self.assertIn("Exception in thread", err)
        self.assertIn("Traceback (most recent call last):", err)
        self.assertIn("ZeroDivisionError", err)
        self.assertNotIn("Unhandled exception", err)

    def test_print_exception_stderr_is_none_2(self):
        script = r"""if True:
            import sys
            import threading
            import time

            running = False
            def run():
                global running
                running = True
                while running:
                    time.sleep(0.01)
                1/0
            sys.stderr = None
            t = threading.Thread(target=run)
            t.start()
            while not running:
                time.sleep(0.01)
            running = False
            t.join()
            """
        rc, out, err = assert_python_ok("-c", script)
        self.assertEqual(out, b'')
        self.assertNotIn("Unhandled exception", err.decode())

    def test_bare_raise_in_brand_new_thread(self):
        def bare_raise():
            raise

        class Issue27558(threading.Thread):
            exc = None

            def run(self):
                try:
                    bare_raise()
                except Exception as exc:
                    self.exc = exc

        thread = Issue27558()
        thread.start()
        thread.join()
        self.assertIsNotNone(thread.exc)
        self.assertIsInstance(thread.exc, RuntimeError)
        # explicitly break the reference cycle to not leak a dangling thread
        thread.exc = None


class ThreadRunFail(threading.Thread):
    def run(self):
        raise ValueError("run failed")


class ExceptHookTests(BaseTestCase):
    def test_excepthook(self):
        with support.captured_output("stderr") as stderr:
            thread = ThreadRunFail(name="excepthook thread")
            thread.start()
            thread.join()

        stderr = stderr.getvalue().strip()
        self.assertIn(f'Exception in thread {thread.name}:\n', stderr)
        self.assertIn('Traceback (most recent call last):\n', stderr)
        self.assertIn('  raise ValueError("run failed")', stderr)
        self.assertIn('ValueError: run failed', stderr)

    @support.cpython_only
    def test_excepthook_thread_None(self):
        # threading.excepthook called with thread=None: log the thread
        # identifier in this case.
        with support.captured_output("stderr") as stderr:
            try:
                raise ValueError("bug")
            except Exception as exc:
                args = threading.ExceptHookArgs([*sys.exc_info(), None])
                try:
                    threading.excepthook(args)
                finally:
                    # Explicitly break a reference cycle
                    args = None

        stderr = stderr.getvalue().strip()
        self.assertIn(f'Exception in thread {threading.get_ident()}:\n', stderr)
        self.assertIn('Traceback (most recent call last):\n', stderr)
        self.assertIn('  raise ValueError("bug")', stderr)
        self.assertIn('ValueError: bug', stderr)

    def test_system_exit(self):
        class ThreadExit(threading.Thread):
            def run(self):
                sys.exit(1)

        # threading.excepthook() silently ignores SystemExit
        with support.captured_output("stderr") as stderr:
            thread = ThreadExit()
            thread.start()
            thread.join()

        self.assertEqual(stderr.getvalue(), '')

    def test_custom_excepthook(self):
        args = None

        def hook(hook_args):
            nonlocal args
            args = hook_args

        try:
            with support.swap_attr(threading, 'excepthook', hook):
                thread = ThreadRunFail()
                thread.start()
                thread.join()

            self.assertEqual(args.exc_type, ValueError)
            self.assertEqual(str(args.exc_value), 'run failed')
            self.assertEqual(args.exc_traceback, args.exc_value.__traceback__)
            self.assertIs(args.thread, thread)
        finally:
            # Break reference cycle
            args = None

    def test_custom_excepthook_fail(self):
        def threading_hook(args):
            raise ValueError("threading_hook failed")

        err_str = None

        def sys_hook(exc_type, exc_value, exc_traceback):
            nonlocal err_str
            err_str = str(exc_value)

        with support.swap_attr(threading, 'excepthook', threading_hook), \
             support.swap_attr(sys, 'excepthook', sys_hook), \
             support.captured_output('stderr') as stderr:
            thread = ThreadRunFail()
            thread.start()
            thread.join()

        self.assertEqual(stderr.getvalue(),
                         'Exception in threading.excepthook:\n')
        self.assertEqual(err_str, 'threading_hook failed')

    def test_original_excepthook(self):
        def run_thread():
            with support.captured_output("stderr") as output:
                thread = ThreadRunFail(name="excepthook thread")
                thread.start()
                thread.join()
            return output.getvalue()

        def threading_hook(args):
            print("Running a thread failed", file=sys.stderr)

        default_output = run_thread()
        with support.swap_attr(threading, 'excepthook', threading_hook):
            custom_hook_output = run_thread()
            threading.excepthook = threading.__excepthook__
            recovered_output = run_thread()

        self.assertEqual(default_output, recovered_output)
        self.assertNotEqual(default_output, custom_hook_output)
        self.assertEqual(custom_hook_output, "Running a thread failed\n")


class TimerTests(BaseTestCase):

    def setUp(self):
        BaseTestCase.setUp(self)
        self.callback_args = []
        self.callback_event = threading.Event()

    def test_init_immutable_default_args(self):
        # Issue 17435: constructor defaults were mutable objects, they could be
        # mutated via the object attributes and affect other Timer objects.
        timer1 = threading.Timer(0.01, self._callback_spy)
        timer1.start()
        self.callback_event.wait()
        timer1.args.append("blah")
        timer1.kwargs["foo"] = "bar"
        self.callback_event.clear()
        timer2 = threading.Timer(0.01, self._callback_spy)
        timer2.start()
        self.callback_event.wait()
        self.assertEqual(len(self.callback_args), 2)
        self.assertEqual(self.callback_args, [((), {}), ((), {})])
        timer1.join()
        timer2.join()

    def _callback_spy(self, *args, **kwargs):
        self.callback_args.append((args[:], kwargs.copy()))
        self.callback_event.set()

class LockTests(lock_tests.LockTests):
    locktype = staticmethod(threading.Lock)

class PyRLockTests(lock_tests.RLockTests):
    locktype = staticmethod(threading._PyRLock)

@unittest.skipIf(threading._CRLock is None, 'RLock not implemented in C')
class CRLockTests(lock_tests.RLockTests):
    locktype = staticmethod(threading._CRLock)

class EventTests(lock_tests.EventTests):
    eventtype = staticmethod(threading.Event)

class ConditionAsRLockTests(lock_tests.RLockTests):
    # Condition uses an RLock by default and exports its API.
    locktype = staticmethod(threading.Condition)

class ConditionTests(lock_tests.ConditionTests):
    condtype = staticmethod(threading.Condition)

class SemaphoreTests(lock_tests.SemaphoreTests):
    semtype = staticmethod(threading.Semaphore)

class BoundedSemaphoreTests(lock_tests.BoundedSemaphoreTests):
    semtype = staticmethod(threading.BoundedSemaphore)

class BarrierTests(lock_tests.BarrierTests):
    barriertype = staticmethod(threading.Barrier)


class MiscTestCase(unittest.TestCase):
    def test__all__(self):
        extra = {"ThreadError"}
        not_exported = {'currentThread', 'activeCount'}
        support.check__all__(self, threading, ('threading', '_thread'),
                             extra=extra, not_exported=not_exported)


class InterruptMainTests(unittest.TestCase):
    def test_interrupt_main_subthread(self):
        # Calling start_new_thread with a function that executes interrupt_main
        # should raise KeyboardInterrupt upon completion.
        def call_interrupt():
            _thread.interrupt_main()
        t = threading.Thread(target=call_interrupt)
        with self.assertRaises(KeyboardInterrupt):
            t.start()
            t.join()
        t.join()

    def test_interrupt_main_mainthread(self):
        # Make sure that if interrupt_main is called in main thread that
        # KeyboardInterrupt is raised instantly.
        with self.assertRaises(KeyboardInterrupt):
            _thread.interrupt_main()

    def test_interrupt_main_noerror(self):
        handler = signal.getsignal(signal.SIGINT)
        try:
            # No exception should arise.
            signal.signal(signal.SIGINT, signal.SIG_IGN)
            _thread.interrupt_main()

            signal.signal(signal.SIGINT, signal.SIG_DFL)
            _thread.interrupt_main()
        finally:
            # Restore original handler
            signal.signal(signal.SIGINT, handler)


class AtexitTests(unittest.TestCase):

    def test_atexit_output(self):
        rc, out, err = assert_python_ok("-c", """if True:
            import threading

            def run_last():
                print('parrot')

            threading._register_atexit(run_last)
        """)

        self.assertFalse(err)
        self.assertEqual(out.strip(), b'parrot')

    def test_atexit_called_once(self):
        rc, out, err = assert_python_ok("-c", """if True:
            import threading
            from unittest.mock import Mock

            mock = Mock()
            threading._register_atexit(mock)
            mock.assert_not_called()
            # force early shutdown to ensure it was called once
            threading._shutdown()
            mock.assert_called_once()
        """)

        self.assertFalse(err)

    def test_atexit_after_shutdown(self):
        # The only way to do this is by registering an atexit within
        # an atexit, which is intended to raise an exception.
        rc, out, err = assert_python_ok("-c", """if True:
            import threading

            def func():
                pass

            def run_last():
                threading._register_atexit(func)

            threading._register_atexit(run_last)
        """)

        self.assertTrue(err)
        self.assertIn("RuntimeError: can't register atexit after shutdown",
                err.decode())


if __name__ == "__main__":
    unittest.main()
