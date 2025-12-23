"""
Tests for the threading module.
"""

import test.support
from test.support import threading_helper, requires_subprocess, requires_gil_enabled
from test.support import verbose, cpython_only, os_helper
from test.support.import_helper import ensure_lazy_imports, import_module
from test.support.script_helper import assert_python_ok, assert_python_failure, spawn_python
from test.support import force_not_colorized

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
import traceback
import warnings

from unittest import mock
from test import lock_tests
from test import support

try:
    from concurrent import interpreters
except ImportError:
    interpreters = None

threading_helper.requires_working_threading(module=True)

# Between fork() and exec(), only async-safe functions are allowed (issues
# #12316 and #11870), and fork() from a worker thread is known to trigger
# problems with some operating systems (issue #3863): skip problematic tests
# on platforms known to behave badly.
platforms_to_skip = ('netbsd5', 'hp-ux11')


def skip_unless_reliable_fork(test):
    if not support.has_fork_support:
        return unittest.skip("requires working os.fork()")(test)
    if sys.platform in platforms_to_skip:
        return unittest.skip("due to known OS bug related to thread+fork")(test)
    if support.HAVE_ASAN_FORK_BUG:
        return unittest.skip("libasan has a pthread_create() dead lock related to thread+fork")(test)
    if support.check_sanitizer(thread=True):
        return unittest.skip("TSAN doesn't support threads after fork")(test)
    return test


def requires_subinterpreters(meth):
    """Decorator to skip a test if subinterpreters are not supported."""
    return unittest.skipIf(interpreters is None,
                           'subinterpreters required')(meth)


def restore_default_excepthook(testcase):
    testcase.addCleanup(setattr, threading, 'excepthook', threading.excepthook)
    threading.excepthook = threading.__excepthook__


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
    maxDiff = 9999

    @cpython_only
    def test_lazy_import(self):
        ensure_lazy_imports("threading", {"functools", "warnings"})

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

    def test_args_argument(self):
        # bpo-45735: Using list or tuple as *args* in constructor could
        # achieve the same effect.
        num_list = [1]
        num_tuple = (1,)

        str_list = ["str"]
        str_tuple = ("str",)

        list_in_tuple = ([1],)
        tuple_in_list = [(1,)]

        test_cases = (
            (num_list, lambda arg: self.assertEqual(arg, 1)),
            (num_tuple, lambda arg: self.assertEqual(arg, 1)),
            (str_list, lambda arg: self.assertEqual(arg, "str")),
            (str_tuple, lambda arg: self.assertEqual(arg, "str")),
            (list_in_tuple, lambda arg: self.assertEqual(arg, [1])),
            (tuple_in_list, lambda arg: self.assertEqual(arg, (1,)))
        )

        for args, target in test_cases:
            with self.subTest(target=target, args=args):
                t = threading.Thread(target=target, args=args)
                t.start()
                t.join()

    def test_lock_no_args(self):
        threading.Lock()  # works
        self.assertRaises(TypeError, threading.Lock, 1)
        self.assertRaises(TypeError, threading.Lock, a=1)
        self.assertRaises(TypeError, threading.Lock, 1, 2, a=1, b=2)

    def test_lock_no_subclass(self):
        # Intentionally disallow subclasses of threading.Lock because they have
        # never been allowed, so why start now just because the type is public?
        with self.assertRaises(TypeError):
            class MyLock(threading.Lock): pass

    def test_lock_or_none(self):
        import types
        self.assertIsInstance(threading.Lock | None, types.UnionType)

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
        self.assertIsNotNone(threading.current_thread().ident)
        def f():
            ident.append(threading.current_thread().ident)
            done.set()
        done = threading.Event()
        ident = []
        with threading_helper.wait_threads_exit():
            tid = _thread.start_new_thread(f, ())
            done.wait()
            self.assertEqual(ident[0], tid)

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
        dummy_thread = None
        error = None
        def f(mutex):
            try:
                nonlocal dummy_thread
                nonlocal error
                # Calling current_thread() forces an entry for the foreign
                # thread to get made in the threading._active map.
                dummy_thread = threading.current_thread()
                tid = dummy_thread.ident
                self.assertIn(tid, threading._active)
                self.assertIsInstance(dummy_thread, threading._DummyThread)
                self.assertIs(threading._active.get(tid), dummy_thread)
                # gh-29376
                self.assertTrue(
                    dummy_thread.is_alive(),
                    'Expected _DummyThread to be considered alive.'
                )
                self.assertIn('_DummyThread', repr(dummy_thread))
            except BaseException as e:
                error = e
            finally:
                mutex.release()

        mutex = threading.Lock()
        mutex.acquire()
        with threading_helper.wait_threads_exit():
            tid = _thread.start_new_thread(f, (mutex,))
            # Wait for the thread to finish.
            mutex.acquire()
        if error is not None:
            raise error
        self.assertEqual(tid, dummy_thread.ident)
        # Issue gh-106236:
        with self.assertRaises(RuntimeError):
            dummy_thread.join()
        dummy_thread._started.clear()
        with self.assertRaises(RuntimeError):
            dummy_thread.is_alive()
        # Busy wait for the following condition: after the thread dies, the
        # related dummy thread must be removed from threading._active.
        timeout = 5
        timeout_at = time.monotonic() + timeout
        while time.monotonic() < timeout_at:
            if threading._active.get(dummy_thread.ident) is not dummy_thread:
                break
            time.sleep(.1)
        else:
            self.fail('It was expected that the created threading._DummyThread was removed from threading._active.')

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
        def fail_new_thread(*args, **kwargs):
            raise threading.ThreadError()
        _start_joinable_thread = threading._start_joinable_thread
        threading._start_joinable_thread = fail_new_thread
        try:
            t = threading.Thread(target=lambda: None)
            self.assertRaises(threading.ThreadError, t.start)
            self.assertFalse(
                t in threading._limbo,
                "Failed to cleanup _limbo map on failure of Thread.start().")
        finally:
            threading._start_joinable_thread = _start_joinable_thread

    def test_finalize_running_thread(self):
        # Issue 1402: the PyGILState_Ensure / _Release functions may be called
        # very late on python exit: on deallocation of a running thread for
        # example.
        if support.check_sanitizer(thread=True):
            # the thread running `time.sleep(100)` below will still be alive
            # at process exit
            self.skipTest("TSAN would report thread leak")
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
        if support.check_sanitizer(thread=True):
            # the thread running `time.sleep(2)` below will still be alive
            # at process exit
            self.skipTest("TSAN would report thread leak")

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
                support.setswitchinterval(i * 0.0002)
                t = threading.Thread(target=lambda: None)
                t.start()
                t.join()
                l = enum()
                self.assertNotIn(t, l,
                    "#1703448 triggered after %d trials: %s" % (i, l))
        finally:
            sys.setswitchinterval(old_interval)

    @support.bigmemtest(size=20, memuse=72*2**20, dry_run=False)
    def test_join_from_multiple_threads(self, size):
        # Thread.join() should be thread-safe
        errors = []

        def worker():
            time.sleep(0.005)

        def joiner(thread):
            try:
                thread.join()
            except Exception as e:
                errors.append(e)

        for N in range(2, 20):
            threads = [threading.Thread(target=worker)]
            for i in range(N):
                threads.append(threading.Thread(target=joiner,
                                                args=(threads[0],)))
            for t in threads:
                t.start()
            time.sleep(0.01)
            for t in threads:
                t.join()
            if errors:
                raise errors[0]

    def test_join_with_timeout(self):
        lock = _thread.allocate_lock()
        lock.acquire()

        def worker():
            lock.acquire()

        thread = threading.Thread(target=worker)
        thread.start()
        thread.join(timeout=0.01)
        assert thread.is_alive()
        lock.release()
        thread.join()
        assert not thread.is_alive()

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

        restore_default_excepthook(self)

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
        with self.assertWarnsRegex(DeprecationWarning,
                                   r'get the daemon attribute'):
            t.isDaemon()
        with self.assertWarnsRegex(DeprecationWarning,
                                   r'set the daemon attribute'):
            t.setDaemon(True)
        with self.assertWarnsRegex(DeprecationWarning,
                                   r'get the name attribute'):
            t.getName()
        with self.assertWarnsRegex(DeprecationWarning,
                                   r'set the name attribute'):
            t.setName("name")

        e = threading.Event()
        with self.assertWarnsRegex(DeprecationWarning, 'use is_set()'):
            e.isSet()

        cond = threading.Condition()
        cond.acquire()
        with self.assertWarnsRegex(DeprecationWarning, 'use notify_all()'):
            cond.notifyAll()

        with self.assertWarnsRegex(DeprecationWarning, 'use active_count()'):
            threading.activeCount()
        with self.assertWarnsRegex(DeprecationWarning, 'use current_thread()'):
            threading.currentThread()

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

    @skip_unless_reliable_fork
    def test_dummy_thread_after_fork(self):
        # Issue #14308: a dummy thread in the active list doesn't mess up
        # the after-fork mechanism.
        code = """if 1:
            import _thread, threading, os, time, warnings

            def background_thread(evt):
                # Creates and registers the _DummyThread instance
                threading.current_thread()
                evt.set()
                time.sleep(10)

            evt = threading.Event()
            _thread.start_new_thread(background_thread, (evt,))
            evt.wait()
            assert threading.active_count() == 2, threading.active_count()
            with warnings.catch_warnings(record=True) as ws:
                warnings.filterwarnings(
                        "always", category=DeprecationWarning)
                if os.fork() == 0:
                    assert threading.active_count() == 1, threading.active_count()
                    os._exit(0)
                else:
                    assert ws[0].category == DeprecationWarning, ws[0]
                    assert 'fork' in str(ws[0].message), ws[0]
                    os.wait()
        """
        _, out, err = assert_python_ok("-c", code)
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')

    @skip_unless_reliable_fork
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
            # Ignore the warning about fork with threads.
            with warnings.catch_warnings(category=DeprecationWarning,
                                         action="ignore"):
                if (pid := os.fork()) == 0:
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

    @skip_unless_reliable_fork
    @unittest.skipUnless(hasattr(os, 'waitpid'), "test needs os.waitpid()")
    def test_main_thread_after_fork(self):
        code = """if 1:
            import os, threading
            from test import support

            ident = threading.get_ident()
            pid = os.fork()
            if pid == 0:
                print("current ident", threading.get_ident() == ident)
                main = threading.main_thread()
                print("main", main.name)
                print("main ident", main.ident == ident)
                print("current is main", threading.current_thread() is main)
            else:
                support.wait_process(pid, exitcode=0)
        """
        _, out, err = assert_python_ok("-c", code)
        data = out.decode().replace('\r', '')
        self.assertEqual(err, b"")
        self.assertEqual(data,
                         "current ident True\n"
                         "main MainThread\n"
                         "main ident True\n"
                         "current is main True\n")

    @skip_unless_reliable_fork
    @unittest.skipUnless(hasattr(os, 'waitpid'), "test needs os.waitpid()")
    def test_main_thread_after_fork_from_nonmain_thread(self):
        code = """if 1:
            import os, threading, sys, warnings
            from test import support

            def func():
                ident = threading.get_ident()
                with warnings.catch_warnings(record=True) as ws:
                    warnings.filterwarnings(
                            "always", category=DeprecationWarning)
                    pid = os.fork()
                    if pid == 0:
                        print("current ident", threading.get_ident() == ident)
                        main = threading.main_thread()
                        print("main", main.name, type(main).__name__)
                        print("main ident", main.ident == ident)
                        print("current is main", threading.current_thread() is main)
                        # stdout is fully buffered because not a tty,
                        # we have to flush before exit.
                        sys.stdout.flush()
                    else:
                        assert ws[0].category == DeprecationWarning, ws[0]
                        assert 'fork' in str(ws[0].message), ws[0]
                        support.wait_process(pid, exitcode=0)

            th = threading.Thread(target=func)
            th.start()
            th.join()
        """
        _, out, err = assert_python_ok("-c", code)
        data = out.decode().replace('\r', '')
        self.assertEqual(err.decode('utf-8'), "")
        self.assertEqual(data,
                         "current ident True\n"
                         "main Thread-1 (func) Thread\n"
                         "main ident True\n"
                         "current is main True\n"
                         )

    @skip_unless_reliable_fork
    @unittest.skipUnless(hasattr(os, 'waitpid'), "test needs os.waitpid()")
    def test_main_thread_after_fork_from_foreign_thread(self, create_dummy=False):
        code = """if 1:
            import os, threading, sys, traceback, _thread
            from test import support

            def func(lock):
                ident = threading.get_ident()
                if %s:
                    # call current_thread() before fork to allocate DummyThread
                    current = threading.current_thread()
                    print("current", current.name, type(current).__name__)
                print("ident in _active", ident in threading._active)
                # flush before fork, so child won't flush it again
                sys.stdout.flush()
                pid = os.fork()
                if pid == 0:
                    print("current ident", threading.get_ident() == ident)
                    main = threading.main_thread()
                    print("main", main.name, type(main).__name__)
                    print("main ident", main.ident == ident)
                    print("current is main", threading.current_thread() is main)
                    print("_dangling", [t.name for t in list(threading._dangling)])
                    # stdout is fully buffered because not a tty,
                    # we have to flush before exit.
                    sys.stdout.flush()
                    try:
                        threading._shutdown()
                        os._exit(0)
                    except:
                        traceback.print_exc()
                        sys.stderr.flush()
                        os._exit(1)
                else:
                    try:
                        support.wait_process(pid, exitcode=0)
                    except Exception:
                        # avoid 'could not acquire lock for
                        # <_io.BufferedWriter name='<stderr>'> at interpreter shutdown,'
                        traceback.print_exc()
                        sys.stderr.flush()
                    finally:
                        lock.release()

            join_lock = _thread.allocate_lock()
            join_lock.acquire()
            th = _thread.start_new_thread(func, (join_lock,))
            join_lock.acquire()
        """ % create_dummy
        # "DeprecationWarning: This process is multi-threaded, use of fork()
        # may lead to deadlocks in the child"
        _, out, err = assert_python_ok("-W", "ignore::DeprecationWarning", "-c", code)
        data = out.decode().replace('\r', '')
        self.assertEqual(err.decode(), "")
        self.assertEqual(data,
                         ("current Dummy-1 _DummyThread\n" if create_dummy else "") +
                         f"ident in _active {create_dummy!s}\n" +
                         "current ident True\n"
                         "main MainThread _MainThread\n"
                         "main ident True\n"
                         "current is main True\n"
                         "_dangling ['MainThread']\n")

    def test_main_thread_after_fork_from_dummy_thread(self, create_dummy=False):
        self.test_main_thread_after_fork_from_foreign_thread(create_dummy=True)

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
        _testcapi = import_module("_testcapi")
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
            _testcapi.call_in_temporary_c_thread(callback)

            # Call the generator in a different Python thread, check that the
            # generator didn't keep a reference to the destroyed thread state
            for test in range(3):
                # The trace function is still called here
                callback()
        finally:
            sys.settrace(old_trace)
            threading.settrace(old_trace)

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

    def test_gettrace_all_threads(self):
        def fn(*args): pass
        old_trace = threading.gettrace()
        first_check = threading.Event()
        second_check = threading.Event()

        trace_funcs = []
        def checker():
            trace_funcs.append(sys.gettrace())
            first_check.set()
            second_check.wait()
            trace_funcs.append(sys.gettrace())

        try:
            t = threading.Thread(target=checker)
            t.start()
            first_check.wait()
            threading.settrace_all_threads(fn)
            second_check.set()
            t.join()
            self.assertEqual(trace_funcs, [None, fn])
            self.assertEqual(threading.gettrace(), fn)
            self.assertEqual(sys.gettrace(), fn)
        finally:
            threading.settrace_all_threads(old_trace)

        self.assertEqual(threading.gettrace(), old_trace)
        self.assertEqual(sys.gettrace(), old_trace)

    def test_getprofile(self):
        def fn(*args): pass
        old_profile = threading.getprofile()
        try:
            threading.setprofile(fn)
            self.assertEqual(fn, threading.getprofile())
        finally:
            threading.setprofile(old_profile)

    def test_getprofile_all_threads(self):
        def fn(*args): pass
        old_profile = threading.getprofile()
        first_check = threading.Event()
        second_check = threading.Event()

        profile_funcs = []
        def checker():
            profile_funcs.append(sys.getprofile())
            first_check.set()
            second_check.wait()
            profile_funcs.append(sys.getprofile())

        try:
            t = threading.Thread(target=checker)
            t.start()
            first_check.wait()
            threading.setprofile_all_threads(fn)
            second_check.set()
            t.join()
            self.assertEqual(profile_funcs, [None, fn])
            self.assertEqual(threading.getprofile(), fn)
            self.assertEqual(sys.getprofile(), fn)
        finally:
            threading.setprofile_all_threads(old_profile)

        self.assertEqual(threading.getprofile(), old_profile)
        self.assertEqual(sys.getprofile(), old_profile)

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

    def test_leak_without_join(self):
        # bpo-37788: Test that a thread which is not joined explicitly
        # does not leak. Test written for reference leak checks.
        def noop(): pass
        with threading_helper.wait_threads_exit():
            threading.Thread(target=noop).start()
            # Thread.join() is not called

    def test_import_from_another_thread(self):
        # bpo-1596321: If the threading module is first import from a thread
        # different than the main thread, threading._shutdown() must handle
        # this case without logging an error at Python exit.
        code = textwrap.dedent('''
            import _thread
            import sys

            event = _thread.allocate_lock()
            event.acquire()

            def import_threading():
                import threading
                event.release()

            if 'threading' in sys.modules:
                raise Exception('threading is already imported')

            _thread.start_new_thread(import_threading, ())

            # wait until the threading module is imported
            event.acquire()
            event.release()

            if 'threading' not in sys.modules:
                raise Exception('threading is not imported')

            # don't wait until the thread completes
        ''')
        rc, out, err = assert_python_ok("-c", code)
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')

    def test_start_new_thread_at_finalization(self):
        code = """if 1:
            import _thread

            def f():
                print("shouldn't be printed")

            class AtFinalization:
                def __del__(self):
                    print("OK")
                    _thread.start_new_thread(f, ())
            at_finalization = AtFinalization()
        """
        _, out, err = assert_python_ok("-c", code)
        self.assertEqual(out.strip(), b"OK")
        self.assertIn(b"can't create new thread at interpreter shutdown", err)

    def test_join_daemon_thread_in_finalization(self):
        # gh-123940: Py_Finalize() prevents other threads from running Python
        # code, so join() can not succeed unless the thread is already done.
        # (Non-Python threads, that is `threading._DummyThread`, can't be
        # joined at all.)
        # We raise an exception rather than hang.
        for timeout in (None, 10):
            with self.subTest(timeout=timeout):
                code = textwrap.dedent(f"""
                    import threading


                    def loop():
                        while True:
                            pass


                    class Cycle:
                        def __init__(self):
                            self.self_ref = self
                            self.thr = threading.Thread(
                                target=loop, daemon=True)
                            self.thr.start()

                        def __del__(self):
                            assert self.thr.is_alive()
                            try:
                                self.thr.join(timeout={timeout})
                            except PythonFinalizationError:
                                assert self.thr.is_alive()
                                print('got the correct exception!')

                    # Cycle holds a reference to itself, which ensures it is
                    # cleaned up during the GC that runs after daemon threads
                    # have been forced to exit during finalization.
                    Cycle()
                """)
                rc, out, err = assert_python_ok("-c", code)
                self.assertEqual(err, b"")
                self.assertIn(b"got the correct exception", out)

    def test_join_finished_daemon_thread_in_finalization(self):
        # (see previous test)
        # If the thread is already finished, join() succeeds.
        code = textwrap.dedent("""
            import threading
            done = threading.Event()

            def set_event():
                done.set()

            class Cycle:
                def __init__(self):
                    self.self_ref = self
                    self.thr = threading.Thread(target=set_event, daemon=True)
                    self.thr.start()
                    self.thr.join()

                def __del__(self):
                    assert done.is_set()
                    assert not self.thr.is_alive()
                    self.thr.join()
                    assert not self.thr.is_alive()
                    print('all clear!')

            Cycle()
        """)
        rc, out, err = assert_python_ok("-c", code)
        self.assertEqual(err, b"")
        self.assertIn(b"all clear", out)

    @support.subTests('lock_class_name', ['Lock', 'RLock'])
    def test_acquire_daemon_thread_lock_in_finalization(self, lock_class_name):
        # gh-123940: Py_Finalize() prevents other threads from running Python
        # code (and so, releasing locks), so acquiring a locked lock can not
        # succeed.
        # We raise an exception rather than hang.
        code = textwrap.dedent(f"""
            import threading
            import time

            thread_started_event = threading.Event()

            lock = threading.{lock_class_name}()
            def loop():
                if {lock_class_name!r} == 'RLock':
                    lock.acquire()
                with lock:
                    thread_started_event.set()
                    while True:
                        time.sleep(1)

            uncontested_lock = threading.{lock_class_name}()

            class Cycle:
                def __init__(self):
                    self.self_ref = self
                    self.thr = threading.Thread(
                        target=loop, daemon=True)
                    self.thr.start()
                    thread_started_event.wait()

                def __del__(self):
                    assert self.thr.is_alive()

                    # We *can* acquire an unlocked lock
                    uncontested_lock.acquire()
                    if {lock_class_name!r} == 'RLock':
                        uncontested_lock.acquire()

                    # Acquiring a locked one fails
                    try:
                        lock.acquire()
                    except PythonFinalizationError:
                        assert self.thr.is_alive()
                        print('got the correct exception!')

            # Cycle holds a reference to itself, which ensures it is
            # cleaned up during the GC that runs after daemon threads
            # have been forced to exit during finalization.
            Cycle()
        """)
        rc, out, err = assert_python_ok("-c", code)
        self.assertEqual(err, b"")
        self.assertIn(b"got the correct exception", out)

    def test_start_new_thread_failed(self):
        # gh-109746: if Python fails to start newly created thread
        # due to failure of underlying PyThread_start_new_thread() call,
        # its state should be removed from interpreter' thread states list
        # to avoid its double cleanup
        try:
            from resource import setrlimit, RLIMIT_NPROC  # noqa: F401
        except ImportError as err:
            self.skipTest(err)  # RLIMIT_NPROC is specific to Linux and BSD
        code = """if 1:
            import resource
            import _thread

            def f():
                print("shouldn't be printed")

            limits = resource.getrlimit(resource.RLIMIT_NPROC)
            [_, hard] = limits
            resource.setrlimit(resource.RLIMIT_NPROC, (0, hard))

            try:
                handle = _thread.start_joinable_thread(f)
            except RuntimeError:
                print('ok')
            else:
                print('!skip!')
                handle.join()
        """
        _, out, err = assert_python_ok("-u", "-c", code)
        out = out.strip()
        if b'!skip!' in out:
            self.skipTest('RLIMIT_NPROC had no effect; probably superuser')
        self.assertEqual(out, b'ok')
        self.assertEqual(err, b'')

    @cpython_only
    def test_finalize_daemon_thread_hang(self):
        # gh-87135: tests that daemon threads hang during finalization
        script = textwrap.dedent('''
            import os
            import sys
            import threading
            import time
            import _testcapi

            lock = threading.Lock()
            lock.acquire()
            thread_started_event = threading.Event()
            def thread_func():
                try:
                    thread_started_event.set()
                    _testcapi.finalize_thread_hang(lock.acquire)
                finally:
                    # Control must not reach here.
                    os._exit(2)

            t = threading.Thread(target=thread_func)
            t.daemon = True
            t.start()
            thread_started_event.wait()
            # Sleep to ensure daemon thread is blocked on `lock.acquire`
            #
            # Note: This test is designed so that in the unlikely case that
            # `0.1` seconds is not sufficient time for the thread to become
            # blocked on `lock.acquire`, the test will still pass, it just
            # won't be properly testing the thread behavior during
            # finalization.
            time.sleep(0.1)

            def run_during_finalization():
                # Wake up daemon thread
                lock.release()
                # Sleep to give the daemon thread time to crash if it is going
                # to.
                #
                # Note: If due to an exceptionally slow execution this delay is
                # insufficient, the test will still pass but will simply be
                # ineffective as a test.
                time.sleep(0.1)
                # If control reaches here, the test succeeded.
                os._exit(0)

            # Replace sys.stderr.flush as a way to run code during finalization
            orig_flush = sys.stderr.flush
            def do_flush(*args, **kwargs):
                orig_flush(*args, **kwargs)
                if not sys.is_finalizing:
                    return
                sys.stderr.flush = orig_flush
                run_during_finalization()

            sys.stderr.flush = do_flush

            # If the follow exit code is retained, `run_during_finalization`
            # did not run.
            sys.exit(1)
        ''')
        assert_python_ok("-c", script)

    @skip_unless_reliable_fork
    @unittest.skipUnless(hasattr(threading, 'get_native_id'), "test needs threading.get_native_id()")
    def test_native_id_after_fork(self):
        script = """if True:
            import threading
            import os
            from test import support

            parent_thread_native_id = threading.current_thread().native_id
            print(parent_thread_native_id, flush=True)
            assert parent_thread_native_id == threading.get_native_id()
            childpid = os.fork()
            if childpid == 0:
                print(threading.current_thread().native_id, flush=True)
                assert threading.current_thread().native_id == threading.get_native_id()
            else:
                try:
                    assert parent_thread_native_id == threading.current_thread().native_id
                    assert parent_thread_native_id == threading.get_native_id()
                finally:
                    support.wait_process(childpid, exitcode=0)
            """
        rc, out, err = assert_python_ok('-c', script)
        self.assertEqual(rc, 0)
        self.assertEqual(err, b"")
        native_ids = out.strip().splitlines()
        self.assertEqual(len(native_ids), 2)
        self.assertNotEqual(native_ids[0], native_ids[1])

    def test_stop_the_world_during_finalization(self):
        # gh-137433: Test functions that trigger a stop-the-world in the free
        # threading build concurrent with interpreter finalization.
        script = """if True:
            import gc
            import sys
            import threading
            NUM_THREADS = 5
            b = threading.Barrier(NUM_THREADS + 1)
            def run_in_bg():
                b.wait()
                while True:
                    sys.setprofile(None)
                    gc.collect()

            for _ in range(NUM_THREADS):
                t = threading.Thread(target=run_in_bg, daemon=True)
                t.start()

            b.wait()
            print("Exiting...")
        """
        rc, out, err = assert_python_ok('-c', script)
        self.assertEqual(rc, 0)
        self.assertEqual(err, b"")
        self.assertEqual(out.strip(), b"Exiting...")

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

    @skip_unless_reliable_fork
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

    @skip_unless_reliable_fork
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
    @support.bigmemtest(size=40, memuse=70*2**20, dry_run=False)
    def test_4_daemon_threads(self, size):
        # Check that a daemon thread cannot crash the interpreter on shutdown
        # by manipulating internal structures that are being disposed of in
        # the main thread.
        if support.check_sanitizer(thread=True):
            # some of the threads running `random_io` below will still be alive
            # at process exit
            self.skipTest("TSAN would report thread leak")

        script = """if True:
            import os
            import random
            import sys
            import time
            import threading

            thread_has_run = set()

            def random_io():
                '''Loop for a while sleeping random tiny amounts and doing some I/O.'''
                import test.test_threading as mod
                while True:
                    with open(mod.__file__, 'rb') as in_f:
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

    def test_thread_from_thread(self):
        script = """if True:
            import threading
            import time

            def thread2():
                time.sleep(0.05)
                print("OK")

            def thread1():
                time.sleep(0.05)
                t2 = threading.Thread(target=thread2)
                t2.start()

            t = threading.Thread(target=thread1)
            t.start()
            # do not join() -- the interpreter waits for non-daemon threads to
            # finish.
            """
        rc, out, err = assert_python_ok('-c', script)
        self.assertEqual(err, b"")
        self.assertEqual(out.strip(), b"OK")
        self.assertEqual(rc, 0)

    @skip_unless_reliable_fork
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

        # Ignore the warning about fork with threads.
        with warnings.catch_warnings(category=DeprecationWarning,
                                     action="ignore"):
            # start a bunch of threads that will fork() child processes
            threads = []
            for i in range(16):
                t = threading.Thread(target=do_fork_and_wait)
                threads.append(t)
                t.start()

            for t in threads:
                t.join()

    @skip_unless_reliable_fork
    def test_clear_threads_states_after_fork(self):
        # Issue #17094: check that threads states are cleared after fork()

        # start a bunch of threads
        threads = []
        for i in range(16):
            t = threading.Thread(target=lambda : time.sleep(0.3))
            threads.append(t)
            t.start()

        try:
            # Ignore the warning about fork with threads.
            with warnings.catch_warnings(category=DeprecationWarning,
                                         action="ignore"):
                pid = os.fork()
                if pid == 0:
                    # check that threads states have been cleared
                    if len(sys._current_frames()) == 1:
                        os._exit(51)
                    else:
                        os._exit(52)
                else:
                    support.wait_process(pid, exitcode=51)
        finally:
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

    @requires_subinterpreters
    def test_threads_join_with_no_main(self):
        r_interp, w_interp = self.pipe()

        INTERP = b'I'
        FINI = b'F'
        DONE = b'D'

        interp = interpreters.create()
        interp.exec(f"""if True:
            import os
            import threading
            import time

            done = False

            def notify_fini():
                global done
                done = True
                os.write({w_interp}, {FINI!r})
                t.join()
            threading._register_atexit(notify_fini)

            def task():
                while not done:
                    time.sleep(0.1)
                os.write({w_interp}, {DONE!r})
            t = threading.Thread(target=task)
            t.start()

            os.write({w_interp}, {INTERP!r})
            """)
        interp.close()

        self.assertEqual(os.read(r_interp, 1), INTERP)
        self.assertEqual(os.read(r_interp, 1), FINI)
        self.assertEqual(os.read(r_interp, 1), DONE)

    @cpython_only
    @support.skip_if_sanitizer(thread=True, memory=True)
    def test_daemon_threads_fatal_error(self):
        import_module("_testcapi")
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
        assert_python_ok("-c", script)

    def _check_allowed(self, before_start='', *,
                       allowed=True,
                       daemon_allowed=True,
                       daemon=False,
                       ):
        import_module("_testinternalcapi")
        subinterp_code = textwrap.dedent(f"""
            import test.support
            import threading
            def func():
                print('this should not have run!')
            t = threading.Thread(target=func, daemon={daemon})
            {before_start}
            t.start()
            """)
        check_multi_interp_extensions = bool(support.Py_GIL_DISABLED)
        script = textwrap.dedent(f"""
            import test.support
            test.support.run_in_subinterp_with_config(
                {subinterp_code!r},
                use_main_obmalloc=True,
                allow_fork=True,
                allow_exec=True,
                allow_threads={allowed},
                allow_daemon_threads={daemon_allowed},
                check_multi_interp_extensions={check_multi_interp_extensions},
                own_gil=False,
            )
            """)
        with test.support.SuppressCrashReport():
            _, _, err = assert_python_ok("-c", script)
        return err.decode()

    @cpython_only
    def test_threads_not_allowed(self):
        err = self._check_allowed(
            allowed=False,
            daemon_allowed=False,
            daemon=False,
        )
        self.assertIn('RuntimeError', err)

    @cpython_only
    def test_daemon_threads_not_allowed(self):
        with self.subTest('via Thread()'):
            err = self._check_allowed(
                allowed=True,
                daemon_allowed=False,
                daemon=True,
            )
            self.assertIn('RuntimeError', err)

        with self.subTest('via Thread.daemon setter'):
            err = self._check_allowed(
                't.daemon = True',
                allowed=True,
                daemon_allowed=False,
                daemon=False,
            )
            self.assertIn('RuntimeError', err)


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

    @requires_subprocess()
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

    def test_print_exception_gh_102056(self):
        # This used to crash. See gh-102056.
        script = r"""if True:
            import time
            import threading
            import _thread

            def f():
                try:
                    f()
                except RecursionError:
                    f()

            def g():
                try:
                    raise ValueError()
                except* ValueError:
                    f()

            def h():
                time.sleep(1)
                _thread.interrupt_main()

            t = threading.Thread(target=h)
            t.start()
            g()
            t.join()
            """

        assert_python_failure("-c", script)

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

    def test_multithread_modify_file_noerror(self):
        # See issue25872
        def modify_file():
            with open(os_helper.TESTFN, 'w', encoding='utf-8') as fp:
                fp.write(' ')
                traceback.format_stack()

        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        threads = [
            threading.Thread(target=modify_file)
            for i in range(100)
        ]
        for t in threads:
            t.start()
            t.join()

    def test_dummy_thread_on_interpreter_shutdown(self):
        # GH-130522: When `threading` held a reference to itself and then a
        # _DummyThread() object was created, destruction of the dummy thread
        # would emit an unraisable exception at shutdown, due to a lock being
        # destroyed.
        code = """if True:
        import sys
        import threading

        threading.x = sys.modules[__name__]
        x = threading._DummyThread()
        """
        rc, out, err = assert_python_ok("-c", code)
        self.assertEqual(rc, 0)
        self.assertEqual(out, b"")
        self.assertEqual(err, b"")

    @requires_subprocess()
    @unittest.skipIf(os.name == 'nt', "signals don't work well on windows")
    def test_keyboard_interrupt_during_threading_shutdown(self):
        import subprocess
        source = f"""
        from threading import Thread
        import time
        import os


        def test():
            print('a', flush=True, end='')
            time.sleep(10)


        for _ in range(3):
            Thread(target=test).start()
        """

        with spawn_python("-c", source, stderr=subprocess.PIPE) as proc:
            self.assertEqual(proc.stdout.read(3), b'aaa')
            proc.send_signal(signal.SIGINT)
            proc.stderr.flush()
            error = proc.stderr.read()
            self.assertIn(b"KeyboardInterrupt", error)


class ThreadRunFail(threading.Thread):
    def run(self):
        raise ValueError("run failed")


class ExceptHookTests(BaseTestCase):
    def setUp(self):
        restore_default_excepthook(self)
        super().setUp()

    @force_not_colorized
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
    @force_not_colorized
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

    def test_signature(self):  # gh-102029
        with warnings.catch_warnings(record=True) as warnings_log:
            threading.RLock()
        self.assertEqual(warnings_log, [])

        arg_types = [
            ((1,), {}),
            ((), {'a': 1}),
            ((1, 2), {'a': 1}),
        ]
        for args, kwargs in arg_types:
            with self.subTest(args=args, kwargs=kwargs):
                self.assertRaises(TypeError, threading.RLock, *args, **kwargs)

        # Subtypes with custom `__init__` are allowed (but, not recommended):
        class CustomRLock(self.locktype):
            def __init__(self, a, *, b) -> None:
                super().__init__()

        with warnings.catch_warnings(record=True) as warnings_log:
            CustomRLock(1, b=2)
        self.assertEqual(warnings_log, [])

class EventTests(lock_tests.EventTests):
    eventtype = staticmethod(threading.Event)

class ConditionAsRLockTests(lock_tests.RLockTests):
    # Condition uses an RLock by default and exports its API.
    locktype = staticmethod(threading.Condition)

    def test_constructor_noargs(self):
        self.skipTest("Condition allows positional arguments")

    def test_recursion_count(self):
        self.skipTest("Condition does not expose _recursion_count()")

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
        restore_default_excepthook(self)

        extra = {"ThreadError"}
        not_exported = {'currentThread', 'activeCount'}
        support.check__all__(self, threading, ('threading', '_thread'),
                             extra=extra, not_exported=not_exported)

    @unittest.skipUnless(hasattr(_thread, 'set_name'), "missing _thread.set_name")
    @unittest.skipUnless(hasattr(_thread, '_get_name'), "missing _thread._get_name")
    def test_set_name(self):
        # Ensure main thread name is restored after test
        self.addCleanup(_thread.set_name, _thread._get_name())

        # set_name() limit in bytes
        truncate = getattr(_thread, "_NAME_MAXLEN", None)
        limit = truncate or 100

        tests = [
            # test short ASCII name
            "CustomName",

            # test short non-ASCII name
            "namé€",

            # embedded null character: name is truncated
            # at the first null character
            "embed\0null",

            # Test long ASCII names (not truncated)
            "x" * limit,

            # Test long ASCII names (truncated)
            "x" * (limit + 10),

            # Test long non-ASCII name (truncated)
            "x" * (limit - 1) + "é€",

            # Test long non-BMP names (truncated) creating surrogate pairs
            # on Windows
            "x" * (limit - 1) + "\U0010FFFF",
            "x" * (limit - 2) + "\U0010FFFF" * 2,
            "x" + "\U0001f40d" * limit,
            "xx" + "\U0001f40d" * limit,
            "xxx" + "\U0001f40d" * limit,
            "xxxx" + "\U0001f40d" * limit,
        ]
        if os_helper.FS_NONASCII:
            tests.append(f"nonascii:{os_helper.FS_NONASCII}")
        if os_helper.TESTFN_UNENCODABLE:
            tests.append(os_helper.TESTFN_UNENCODABLE)

        if sys.platform.startswith("sunos"):
            # Use ASCII encoding on Solaris/Illumos/OpenIndiana
            encoding = "ascii"
        else:
            encoding = sys.getfilesystemencoding()

        def work():
            nonlocal work_name
            work_name = _thread._get_name()

        for name in tests:
            if not support.MS_WINDOWS:
                encoded = name.encode(encoding, "replace")
                if b'\0' in encoded:
                    encoded = encoded.split(b'\0', 1)[0]
                if truncate is not None:
                    encoded = encoded[:truncate]
                if sys.platform.startswith("sunos"):
                    expected = encoded.decode("ascii", "surrogateescape")
                else:
                    expected = os.fsdecode(encoded)
            else:
                size = 0
                chars = []
                for ch in name:
                    if ord(ch) > 0xFFFF:
                        size += 2
                    else:
                        size += 1
                    if size > truncate:
                        break
                    chars.append(ch)
                expected = ''.join(chars)

                if '\0' in expected:
                    expected = expected.split('\0', 1)[0]

            with self.subTest(name=name, expected=expected, thread="main"):
                _thread.set_name(name)
                self.assertEqual(_thread._get_name(), expected)

            with self.subTest(name=name, expected=expected, thread="worker"):
                work_name = None
                thread = threading.Thread(target=work, name=name)
                thread.start()
                thread.join()
                self.assertEqual(work_name, expected,
                                 f"{len(work_name)=} and {len(expected)=}")

    @unittest.skipUnless(hasattr(_thread, 'set_name'), "missing _thread.set_name")
    @unittest.skipUnless(hasattr(_thread, '_get_name'), "missing _thread._get_name")
    def test_change_name(self):
        # Change the name of a thread while the thread is running

        name1 = None
        name2 = None
        def work():
            nonlocal name1, name2
            name1 = _thread._get_name()
            threading.current_thread().name = "new name"
            name2 = _thread._get_name()

        thread = threading.Thread(target=work, name="name")
        thread.start()
        thread.join()
        self.assertEqual(name1, "name")
        self.assertEqual(name2, "new name")


class InterruptMainTests(unittest.TestCase):
    def check_interrupt_main_with_signal_handler(self, signum):
        def handler(signum, frame):
            1/0

        old_handler = signal.signal(signum, handler)
        self.addCleanup(signal.signal, signum, old_handler)

        with self.assertRaises(ZeroDivisionError):
            _thread.interrupt_main()

    def check_interrupt_main_noerror(self, signum):
        handler = signal.getsignal(signum)
        try:
            # No exception should arise.
            signal.signal(signum, signal.SIG_IGN)
            _thread.interrupt_main(signum)

            signal.signal(signum, signal.SIG_DFL)
            _thread.interrupt_main(signum)
        finally:
            # Restore original handler
            signal.signal(signum, handler)

    @requires_gil_enabled("gh-118433: Flaky due to a longstanding bug")
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

    def test_interrupt_main_with_signal_handler(self):
        self.check_interrupt_main_with_signal_handler(signal.SIGINT)
        self.check_interrupt_main_with_signal_handler(signal.SIGTERM)

    def test_interrupt_main_noerror(self):
        self.check_interrupt_main_noerror(signal.SIGINT)
        self.check_interrupt_main_noerror(signal.SIGTERM)

    def test_interrupt_main_invalid_signal(self):
        self.assertRaises(ValueError, _thread.interrupt_main, -1)
        self.assertRaises(ValueError, _thread.interrupt_main, signal.NSIG)
        self.assertRaises(ValueError, _thread.interrupt_main, 1000000)

    @threading_helper.reap_threads
    def test_can_interrupt_tight_loops(self):
        cont = [True]
        started = [False]
        interrupted = [False]

        def worker(started, cont, interrupted):
            iterations = 100_000_000
            started[0] = True
            while cont[0]:
                if iterations:
                    iterations -= 1
                else:
                    return
                pass
            interrupted[0] = True

        t = threading.Thread(target=worker,args=(started, cont, interrupted))
        t.start()
        while not started[0]:
            pass
        cont[0] = False
        t.join()
        self.assertTrue(interrupted[0])


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

    @force_not_colorized
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
