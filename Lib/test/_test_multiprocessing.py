#
# Unit tests for the multiprocessing package
#

import unittest
import unittest.mock
import queue as pyqueue
import textwrap
import time
import io
import itertools
import sys
import os
import gc
import importlib
import errno
import functools
import signal
import array
import collections.abc
import socket
import random
import logging
import shutil
import subprocess
import struct
import tempfile
import operator
import pickle
import weakref
import warnings
import test.support
import test.support.script_helper
from test import support
from test.support import hashlib_helper
from test.support import import_helper
from test.support import os_helper
from test.support import script_helper
from test.support import socket_helper
from test.support import threading_helper
from test.support import warnings_helper


# Skip tests if _multiprocessing wasn't built.
_multiprocessing = import_helper.import_module('_multiprocessing')
# Skip tests if sem_open implementation is broken.
support.skip_if_broken_multiprocessing_synchronize()
import threading

import multiprocessing.connection
import multiprocessing.dummy
import multiprocessing.heap
import multiprocessing.managers
import multiprocessing.pool
import multiprocessing.queues
from multiprocessing.connection import wait

from multiprocessing import util

try:
    from multiprocessing import reduction
    HAS_REDUCTION = reduction.HAVE_SEND_HANDLE
except ImportError:
    HAS_REDUCTION = False

try:
    from multiprocessing.sharedctypes import Value, copy
    HAS_SHAREDCTYPES = True
except ImportError:
    HAS_SHAREDCTYPES = False

try:
    from multiprocessing import shared_memory
    HAS_SHMEM = True
except ImportError:
    HAS_SHMEM = False

try:
    import msvcrt
except ImportError:
    msvcrt = None


if support.HAVE_ASAN_FORK_BUG:
    # gh-89363: Skip multiprocessing tests if Python is built with ASAN to
    # work around a libasan race condition: dead lock in pthread_create().
    raise unittest.SkipTest("libasan has a pthread_create() dead lock related to thread+fork")


# gh-110666: Tolerate a difference of 100 ms when comparing timings
# (clock resolution)
CLOCK_RES = 0.100


def latin(s):
    return s.encode('latin')


def close_queue(queue):
    if isinstance(queue, multiprocessing.queues.Queue):
        queue.close()
        queue.join_thread()


def join_process(process):
    # Since multiprocessing.Process has the same API than threading.Thread
    # (join() and is_alive(), the support function can be reused
    threading_helper.join_thread(process)


if os.name == "posix":
    from multiprocessing import resource_tracker

    def _resource_unlink(name, rtype):
        resource_tracker._CLEANUP_FUNCS[rtype](name)


#
# Constants
#

LOG_LEVEL = util.SUBWARNING
#LOG_LEVEL = logging.DEBUG

DELTA = 0.1
CHECK_TIMINGS = False     # making true makes tests take a lot longer
                          # and can sometimes cause some non-serious
                          # failures because some calls block a bit
                          # longer than expected
if CHECK_TIMINGS:
    TIMEOUT1, TIMEOUT2, TIMEOUT3 = 0.82, 0.35, 1.4
else:
    TIMEOUT1, TIMEOUT2, TIMEOUT3 = 0.1, 0.1, 0.1

# BaseManager.shutdown_timeout
SHUTDOWN_TIMEOUT = support.SHORT_TIMEOUT

WAIT_ACTIVE_CHILDREN_TIMEOUT = 5.0

HAVE_GETVALUE = not getattr(_multiprocessing,
                            'HAVE_BROKEN_SEM_GETVALUE', False)

WIN32 = (sys.platform == "win32")

def wait_for_handle(handle, timeout):
    if timeout is not None and timeout < 0.0:
        timeout = None
    return wait([handle], timeout)

try:
    MAXFD = os.sysconf("SC_OPEN_MAX")
except:
    MAXFD = 256

# To speed up tests when using the forkserver, we can preload these:
PRELOAD = ['__main__', 'test.test_multiprocessing_forkserver']

#
# Some tests require ctypes
#

try:
    from ctypes import Structure, c_int, c_double, c_longlong
except ImportError:
    Structure = object
    c_int = c_double = c_longlong = None


def check_enough_semaphores():
    """Check that the system supports enough semaphores to run the test."""
    # minimum number of semaphores available according to POSIX
    nsems_min = 256
    try:
        nsems = os.sysconf("SC_SEM_NSEMS_MAX")
    except (AttributeError, ValueError):
        # sysconf not available or setting not available
        return
    if nsems == -1 or nsems >= nsems_min:
        return
    raise unittest.SkipTest("The OS doesn't support enough semaphores "
                            "to run the test (required: %d)." % nsems_min)


def only_run_in_spawn_testsuite(reason):
    """Returns a decorator: raises SkipTest when SM != spawn at test time.

    This can be useful to save overall Python test suite execution time.
    "spawn" is the universal mode available on all platforms so this limits the
    decorated test to only execute within test_multiprocessing_spawn.

    This would not be necessary if we refactored our test suite to split things
    into other test files when they are not start method specific to be rerun
    under all start methods.
    """

    def decorator(test_item):

        @functools.wraps(test_item)
        def spawn_check_wrapper(*args, **kwargs):
            if (start_method := multiprocessing.get_start_method()) != "spawn":
                raise unittest.SkipTest(f"{start_method=}, not 'spawn'; {reason}")
            return test_item(*args, **kwargs)

        return spawn_check_wrapper

    return decorator


class TestInternalDecorators(unittest.TestCase):
    """Logic within a test suite that could errantly skip tests? Test it!"""

    @unittest.skipIf(sys.platform == "win32", "test requires that fork exists.")
    def test_only_run_in_spawn_testsuite(self):
        if multiprocessing.get_start_method() != "spawn":
            raise unittest.SkipTest("only run in test_multiprocessing_spawn.")

        try:
            @only_run_in_spawn_testsuite("testing this decorator")
            def return_four_if_spawn():
                return 4
        except Exception as err:
            self.fail(f"expected decorated `def` not to raise; caught {err}")

        orig_start_method = multiprocessing.get_start_method(allow_none=True)
        try:
            multiprocessing.set_start_method("spawn", force=True)
            self.assertEqual(return_four_if_spawn(), 4)
            multiprocessing.set_start_method("fork", force=True)
            with self.assertRaises(unittest.SkipTest) as ctx:
                return_four_if_spawn()
            self.assertIn("testing this decorator", str(ctx.exception))
            self.assertIn("start_method=", str(ctx.exception))
        finally:
            multiprocessing.set_start_method(orig_start_method, force=True)


#
# Creates a wrapper for a function which records the time it takes to finish
#

class TimingWrapper(object):

    def __init__(self, func):
        self.func = func
        self.elapsed = None

    def __call__(self, *args, **kwds):
        t = time.monotonic()
        try:
            return self.func(*args, **kwds)
        finally:
            self.elapsed = time.monotonic() - t

#
# Base class for test cases
#

class BaseTestCase(object):

    ALLOWED_TYPES = ('processes', 'manager', 'threads')
    # If not empty, limit which start method suites run this class.
    START_METHODS: set[str] = set()
    start_method = None  # set by install_tests_in_module_dict()

    def assertTimingAlmostEqual(self, a, b):
        if CHECK_TIMINGS:
            self.assertAlmostEqual(a, b, 1)

    def assertReturnsIfImplemented(self, value, func, *args):
        try:
            res = func(*args)
        except NotImplementedError:
            pass
        else:
            return self.assertEqual(value, res)

    # For the sanity of Windows users, rather than crashing or freezing in
    # multiple ways.
    def __reduce__(self, *args):
        raise NotImplementedError("shouldn't try to pickle a test case")

    __reduce_ex__ = __reduce__

#
# Return the value of a semaphore
#

def get_value(self):
    try:
        return self.get_value()
    except AttributeError:
        try:
            return self._Semaphore__value
        except AttributeError:
            try:
                return self._value
            except AttributeError:
                raise NotImplementedError

#
# Testcases
#

class DummyCallable:
    def __call__(self, q, c):
        assert isinstance(c, DummyCallable)
        q.put(5)


class _TestProcess(BaseTestCase):

    ALLOWED_TYPES = ('processes', 'threads')

    def test_current(self):
        if self.TYPE == 'threads':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        current = self.current_process()
        authkey = current.authkey

        self.assertTrue(current.is_alive())
        self.assertFalse(current.daemon)
        self.assertIsInstance(authkey, bytes)
        self.assertTrue(len(authkey) > 0)
        self.assertEqual(current.ident, os.getpid())
        self.assertEqual(current.exitcode, None)

    def test_set_executable(self):
        if self.TYPE == 'threads':
            self.skipTest(f'test not appropriate for {self.TYPE}')
        paths = [
            sys.executable,               # str
            os.fsencode(sys.executable),  # bytes
            os_helper.FakePath(sys.executable),  # os.PathLike
            os_helper.FakePath(os.fsencode(sys.executable)),  # os.PathLike bytes
        ]
        for path in paths:
            self.set_executable(path)
            p = self.Process()
            p.start()
            p.join()
            self.assertEqual(p.exitcode, 0)

    @support.requires_resource('cpu')
    def test_args_argument(self):
        # bpo-45735: Using list or tuple as *args* in constructor could
        # achieve the same effect.
        args_cases = (1, "str", [1], (1,))
        args_types = (list, tuple)

        test_cases = itertools.product(args_cases, args_types)

        for args, args_type in test_cases:
            with self.subTest(args=args, args_type=args_type):
                q = self.Queue(1)
                # pass a tuple or list as args
                p = self.Process(target=self._test_args, args=args_type((q, args)))
                p.daemon = True
                p.start()
                child_args = q.get()
                self.assertEqual(child_args, args)
                p.join()
                close_queue(q)

    @classmethod
    def _test_args(cls, q, arg):
        q.put(arg)

    def test_daemon_argument(self):
        if self.TYPE == "threads":
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        # By default uses the current process's daemon flag.
        proc0 = self.Process(target=self._test)
        self.assertEqual(proc0.daemon, self.current_process().daemon)
        proc1 = self.Process(target=self._test, daemon=True)
        self.assertTrue(proc1.daemon)
        proc2 = self.Process(target=self._test, daemon=False)
        self.assertFalse(proc2.daemon)

    @classmethod
    def _test(cls, q, *args, **kwds):
        current = cls.current_process()
        q.put(args)
        q.put(kwds)
        q.put(current.name)
        if cls.TYPE != 'threads':
            q.put(bytes(current.authkey))
            q.put(current.pid)

    def test_parent_process_attributes(self):
        if self.TYPE == "threads":
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        self.assertIsNone(self.parent_process())

        rconn, wconn = self.Pipe(duplex=False)
        p = self.Process(target=self._test_send_parent_process, args=(wconn,))
        p.start()
        p.join()
        parent_pid, parent_name = rconn.recv()
        self.assertEqual(parent_pid, self.current_process().pid)
        self.assertEqual(parent_pid, os.getpid())
        self.assertEqual(parent_name, self.current_process().name)

    @classmethod
    def _test_send_parent_process(cls, wconn):
        from multiprocessing.process import parent_process
        wconn.send([parent_process().pid, parent_process().name])

    def test_parent_process(self):
        if self.TYPE == "threads":
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        # Launch a child process. Make it launch a grandchild process. Kill the
        # child process and make sure that the grandchild notices the death of
        # its parent (a.k.a the child process).
        rconn, wconn = self.Pipe(duplex=False)
        p = self.Process(
            target=self._test_create_grandchild_process, args=(wconn, ))
        p.start()

        if not rconn.poll(timeout=support.LONG_TIMEOUT):
            raise AssertionError("Could not communicate with child process")
        parent_process_status = rconn.recv()
        self.assertEqual(parent_process_status, "alive")

        p.terminate()
        p.join()

        if not rconn.poll(timeout=support.LONG_TIMEOUT):
            raise AssertionError("Could not communicate with child process")
        parent_process_status = rconn.recv()
        self.assertEqual(parent_process_status, "not alive")

    @classmethod
    def _test_create_grandchild_process(cls, wconn):
        p = cls.Process(target=cls._test_report_parent_status, args=(wconn, ))
        p.start()
        time.sleep(300)

    @classmethod
    def _test_report_parent_status(cls, wconn):
        from multiprocessing.process import parent_process
        wconn.send("alive" if parent_process().is_alive() else "not alive")
        parent_process().join(timeout=support.SHORT_TIMEOUT)
        wconn.send("alive" if parent_process().is_alive() else "not alive")

    def test_process(self):
        q = self.Queue(1)
        e = self.Event()
        args = (q, 1, 2)
        kwargs = {'hello':23, 'bye':2.54}
        name = 'SomeProcess'
        p = self.Process(
            target=self._test, args=args, kwargs=kwargs, name=name
            )
        p.daemon = True
        current = self.current_process()

        if self.TYPE != 'threads':
            self.assertEqual(p.authkey, current.authkey)
        self.assertEqual(p.is_alive(), False)
        self.assertEqual(p.daemon, True)
        self.assertNotIn(p, self.active_children())
        self.assertIs(type(self.active_children()), list)
        self.assertEqual(p.exitcode, None)

        p.start()

        self.assertEqual(p.exitcode, None)
        self.assertEqual(p.is_alive(), True)
        self.assertIn(p, self.active_children())

        self.assertEqual(q.get(), args[1:])
        self.assertEqual(q.get(), kwargs)
        self.assertEqual(q.get(), p.name)
        if self.TYPE != 'threads':
            self.assertEqual(q.get(), current.authkey)
            self.assertEqual(q.get(), p.pid)

        p.join()

        self.assertEqual(p.exitcode, 0)
        self.assertEqual(p.is_alive(), False)
        self.assertNotIn(p, self.active_children())
        close_queue(q)

    @unittest.skipUnless(threading._HAVE_THREAD_NATIVE_ID, "needs native_id")
    def test_process_mainthread_native_id(self):
        if self.TYPE == 'threads':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        current_mainthread_native_id = threading.main_thread().native_id

        q = self.Queue(1)
        p = self.Process(target=self._test_process_mainthread_native_id, args=(q,))
        p.start()

        child_mainthread_native_id = q.get()
        p.join()
        close_queue(q)

        self.assertNotEqual(current_mainthread_native_id, child_mainthread_native_id)

    @classmethod
    def _test_process_mainthread_native_id(cls, q):
        mainthread_native_id = threading.main_thread().native_id
        q.put(mainthread_native_id)

    @classmethod
    def _sleep_some(cls):
        time.sleep(100)

    @classmethod
    def _test_sleep(cls, delay):
        time.sleep(delay)

    def _kill_process(self, meth):
        if self.TYPE == 'threads':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        p = self.Process(target=self._sleep_some)
        p.daemon = True
        p.start()

        self.assertEqual(p.is_alive(), True)
        self.assertIn(p, self.active_children())
        self.assertEqual(p.exitcode, None)

        join = TimingWrapper(p.join)

        self.assertEqual(join(0), None)
        self.assertTimingAlmostEqual(join.elapsed, 0.0)
        self.assertEqual(p.is_alive(), True)

        self.assertEqual(join(-1), None)
        self.assertTimingAlmostEqual(join.elapsed, 0.0)
        self.assertEqual(p.is_alive(), True)

        # XXX maybe terminating too soon causes the problems on Gentoo...
        time.sleep(1)

        meth(p)

        if hasattr(signal, 'alarm'):
            # On the Gentoo buildbot waitpid() often seems to block forever.
            # We use alarm() to interrupt it if it blocks for too long.
            def handler(*args):
                raise RuntimeError('join took too long: %s' % p)
            old_handler = signal.signal(signal.SIGALRM, handler)
            try:
                signal.alarm(10)
                self.assertEqual(join(), None)
            finally:
                signal.alarm(0)
                signal.signal(signal.SIGALRM, old_handler)
        else:
            self.assertEqual(join(), None)

        self.assertTimingAlmostEqual(join.elapsed, 0.0)

        self.assertEqual(p.is_alive(), False)
        self.assertNotIn(p, self.active_children())

        p.join()

        return p.exitcode

    def test_terminate(self):
        exitcode = self._kill_process(multiprocessing.Process.terminate)
        self.assertEqual(exitcode, -signal.SIGTERM)

    def test_kill(self):
        exitcode = self._kill_process(multiprocessing.Process.kill)
        if os.name != 'nt':
            self.assertEqual(exitcode, -signal.SIGKILL)
        else:
            self.assertEqual(exitcode, -signal.SIGTERM)

    def test_cpu_count(self):
        try:
            cpus = multiprocessing.cpu_count()
        except NotImplementedError:
            cpus = 1
        self.assertIsInstance(cpus, int)
        self.assertGreaterEqual(cpus, 1)

    def test_active_children(self):
        self.assertEqual(type(self.active_children()), list)

        event = self.Event()
        p = self.Process(target=event.wait, args=())
        self.assertNotIn(p, self.active_children())

        try:
            p.daemon = True
            p.start()
            self.assertIn(p, self.active_children())
        finally:
            event.set()

        p.join()
        self.assertNotIn(p, self.active_children())

    @classmethod
    def _test_recursion(cls, wconn, id):
        wconn.send(id)
        if len(id) < 2:
            for i in range(2):
                p = cls.Process(
                    target=cls._test_recursion, args=(wconn, id+[i])
                    )
                p.start()
                p.join()

    def test_recursion(self):
        rconn, wconn = self.Pipe(duplex=False)
        self._test_recursion(wconn, [])

        time.sleep(DELTA)
        result = []
        while rconn.poll():
            result.append(rconn.recv())

        expected = [
            [],
              [0],
                [0, 0],
                [0, 1],
              [1],
                [1, 0],
                [1, 1]
            ]
        self.assertEqual(result, expected)

    @classmethod
    def _test_sentinel(cls, event):
        event.wait(10.0)

    def test_sentinel(self):
        if self.TYPE == "threads":
            self.skipTest('test not appropriate for {}'.format(self.TYPE))
        event = self.Event()
        p = self.Process(target=self._test_sentinel, args=(event,))
        with self.assertRaises(ValueError):
            p.sentinel
        p.start()
        self.addCleanup(p.join)
        sentinel = p.sentinel
        self.assertIsInstance(sentinel, int)
        self.assertFalse(wait_for_handle(sentinel, timeout=0.0))
        event.set()
        p.join()
        self.assertTrue(wait_for_handle(sentinel, timeout=1))

    @classmethod
    def _test_close(cls, rc=0, q=None):
        if q is not None:
            q.get()
        sys.exit(rc)

    def test_close(self):
        if self.TYPE == "threads":
            self.skipTest('test not appropriate for {}'.format(self.TYPE))
        q = self.Queue()
        p = self.Process(target=self._test_close, kwargs={'q': q})
        p.daemon = True
        p.start()
        self.assertEqual(p.is_alive(), True)
        # Child is still alive, cannot close
        with self.assertRaises(ValueError):
            p.close()

        q.put(None)
        p.join()
        self.assertEqual(p.is_alive(), False)
        self.assertEqual(p.exitcode, 0)
        p.close()
        with self.assertRaises(ValueError):
            p.is_alive()
        with self.assertRaises(ValueError):
            p.join()
        with self.assertRaises(ValueError):
            p.terminate()
        p.close()

        wr = weakref.ref(p)
        del p
        gc.collect()
        self.assertIs(wr(), None)

        close_queue(q)

    @support.requires_resource('walltime')
    def test_many_processes(self):
        if self.TYPE == 'threads':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        sm = multiprocessing.get_start_method()
        N = 5 if sm == 'spawn' else 100

        # Try to overwhelm the forkserver loop with events
        procs = [self.Process(target=self._test_sleep, args=(0.01,))
                 for i in range(N)]
        for p in procs:
            p.start()
        for p in procs:
            join_process(p)
        for p in procs:
            self.assertEqual(p.exitcode, 0)

        procs = [self.Process(target=self._sleep_some)
                 for i in range(N)]
        for p in procs:
            p.start()
        time.sleep(0.001)  # let the children start...
        for p in procs:
            p.terminate()
        for p in procs:
            join_process(p)
        if os.name != 'nt':
            exitcodes = [-signal.SIGTERM]
            if sys.platform == 'darwin':
                # bpo-31510: On macOS, killing a freshly started process with
                # SIGTERM sometimes kills the process with SIGKILL.
                exitcodes.append(-signal.SIGKILL)
            for p in procs:
                self.assertIn(p.exitcode, exitcodes)

    def test_lose_target_ref(self):
        c = DummyCallable()
        wr = weakref.ref(c)
        q = self.Queue()
        p = self.Process(target=c, args=(q, c))
        del c
        p.start()
        p.join()
        gc.collect()  # For PyPy or other GCs.
        self.assertIs(wr(), None)
        self.assertEqual(q.get(), 5)
        close_queue(q)

    @classmethod
    def _test_child_fd_inflation(self, evt, q):
        q.put(os_helper.fd_count())
        evt.wait()

    def test_child_fd_inflation(self):
        # Number of fds in child processes should not grow with the
        # number of running children.
        if self.TYPE == 'threads':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        sm = multiprocessing.get_start_method()
        if sm == 'fork':
            # The fork method by design inherits all fds from the parent,
            # trying to go against it is a lost battle
            self.skipTest('test not appropriate for {}'.format(sm))

        N = 5
        evt = self.Event()
        q = self.Queue()

        procs = [self.Process(target=self._test_child_fd_inflation, args=(evt, q))
                 for i in range(N)]
        for p in procs:
            p.start()

        try:
            fd_counts = [q.get() for i in range(N)]
            self.assertEqual(len(set(fd_counts)), 1, fd_counts)

        finally:
            evt.set()
            for p in procs:
                p.join()
            close_queue(q)

    @classmethod
    def _test_wait_for_threads(self, evt):
        def func1():
            time.sleep(0.5)
            evt.set()

        def func2():
            time.sleep(20)
            evt.clear()

        threading.Thread(target=func1).start()
        threading.Thread(target=func2, daemon=True).start()

    def test_wait_for_threads(self):
        # A child process should wait for non-daemonic threads to end
        # before exiting
        if self.TYPE == 'threads':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        evt = self.Event()
        proc = self.Process(target=self._test_wait_for_threads, args=(evt,))
        proc.start()
        proc.join()
        self.assertTrue(evt.is_set())

    @classmethod
    def _test_error_on_stdio_flush(self, evt, break_std_streams={}):
        for stream_name, action in break_std_streams.items():
            if action == 'close':
                stream = io.StringIO()
                stream.close()
            else:
                assert action == 'remove'
                stream = None
            setattr(sys, stream_name, None)
        evt.set()

    def test_error_on_stdio_flush_1(self):
        # Check that Process works with broken standard streams
        streams = [io.StringIO(), None]
        streams[0].close()
        for stream_name in ('stdout', 'stderr'):
            for stream in streams:
                old_stream = getattr(sys, stream_name)
                setattr(sys, stream_name, stream)
                try:
                    evt = self.Event()
                    proc = self.Process(target=self._test_error_on_stdio_flush,
                                        args=(evt,))
                    proc.start()
                    proc.join()
                    self.assertTrue(evt.is_set())
                    self.assertEqual(proc.exitcode, 0)
                finally:
                    setattr(sys, stream_name, old_stream)

    def test_error_on_stdio_flush_2(self):
        # Same as test_error_on_stdio_flush_1(), but standard streams are
        # broken by the child process
        for stream_name in ('stdout', 'stderr'):
            for action in ('close', 'remove'):
                old_stream = getattr(sys, stream_name)
                try:
                    evt = self.Event()
                    proc = self.Process(target=self._test_error_on_stdio_flush,
                                        args=(evt, {stream_name: action}))
                    proc.start()
                    proc.join()
                    self.assertTrue(evt.is_set())
                    self.assertEqual(proc.exitcode, 0)
                finally:
                    setattr(sys, stream_name, old_stream)

    @staticmethod
    def _sleep_and_set_event(evt, delay=0.0):
        time.sleep(delay)
        evt.set()

    def check_forkserver_death(self, signum):
        # bpo-31308: if the forkserver process has died, we should still
        # be able to create and run new Process instances (the forkserver
        # is implicitly restarted).
        if self.TYPE == 'threads':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))
        sm = multiprocessing.get_start_method()
        if sm != 'forkserver':
            # The fork method by design inherits all fds from the parent,
            # trying to go against it is a lost battle
            self.skipTest('test not appropriate for {}'.format(sm))

        from multiprocessing.forkserver import _forkserver
        _forkserver.ensure_running()

        # First process sleeps 500 ms
        delay = 0.5

        evt = self.Event()
        proc = self.Process(target=self._sleep_and_set_event, args=(evt, delay))
        proc.start()

        pid = _forkserver._forkserver_pid
        os.kill(pid, signum)
        # give time to the fork server to die and time to proc to complete
        time.sleep(delay * 2.0)

        evt2 = self.Event()
        proc2 = self.Process(target=self._sleep_and_set_event, args=(evt2,))
        proc2.start()
        proc2.join()
        self.assertTrue(evt2.is_set())
        self.assertEqual(proc2.exitcode, 0)

        proc.join()
        self.assertTrue(evt.is_set())
        self.assertIn(proc.exitcode, (0, 255))

    def test_forkserver_sigint(self):
        # Catchable signal
        self.check_forkserver_death(signal.SIGINT)

    def test_forkserver_sigkill(self):
        # Uncatchable signal
        if os.name != 'nt':
            self.check_forkserver_death(signal.SIGKILL)

    def test_forkserver_auth_is_enabled(self):
        if self.TYPE == "threads":
            self.skipTest(f"test not appropriate for {self.TYPE}")
        if multiprocessing.get_start_method() != "forkserver":
            self.skipTest("forkserver start method specific")

        forkserver = multiprocessing.forkserver._forkserver
        forkserver.ensure_running()
        self.assertTrue(forkserver._forkserver_pid)
        authkey = forkserver._forkserver_authkey
        self.assertTrue(authkey)
        self.assertGreater(len(authkey), 15)
        addr = forkserver._forkserver_address
        self.assertTrue(addr)

        # Demonstrate that a raw auth handshake, as Client performs, does not
        # raise an error.
        client = multiprocessing.connection.Client(addr, authkey=authkey)
        client.close()

        # That worked, now launch a quick process.
        proc = self.Process(target=sys.exit)
        proc.start()
        proc.join()
        self.assertEqual(proc.exitcode, 0)

    def test_forkserver_without_auth_fails(self):
        if self.TYPE == "threads":
            self.skipTest(f"test not appropriate for {self.TYPE}")
        if multiprocessing.get_start_method() != "forkserver":
            self.skipTest("forkserver start method specific")

        forkserver = multiprocessing.forkserver._forkserver
        forkserver.ensure_running()
        self.assertTrue(forkserver._forkserver_pid)
        authkey_len = len(forkserver._forkserver_authkey)
        with unittest.mock.patch.object(
                forkserver, '_forkserver_authkey', None):
            # With an incorrect authkey we should get an auth rejection
            # rather than the above protocol error.
            forkserver._forkserver_authkey = b'T' * authkey_len
            proc = self.Process(target=sys.exit)
            with self.assertRaises(multiprocessing.AuthenticationError):
                proc.start()
            del proc

        # authkey restored, launching processes should work again.
        proc = self.Process(target=sys.exit)
        proc.start()
        proc.join()

#
#
#

class _UpperCaser(multiprocessing.Process):

    def __init__(self):
        multiprocessing.Process.__init__(self)
        self.child_conn, self.parent_conn = multiprocessing.Pipe()

    def run(self):
        self.parent_conn.close()
        for s in iter(self.child_conn.recv, None):
            self.child_conn.send(s.upper())
        self.child_conn.close()

    def submit(self, s):
        assert type(s) is str
        self.parent_conn.send(s)
        return self.parent_conn.recv()

    def stop(self):
        self.parent_conn.send(None)
        self.parent_conn.close()
        self.child_conn.close()

class _TestSubclassingProcess(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def test_subclassing(self):
        uppercaser = _UpperCaser()
        uppercaser.daemon = True
        uppercaser.start()
        self.assertEqual(uppercaser.submit('hello'), 'HELLO')
        self.assertEqual(uppercaser.submit('world'), 'WORLD')
        uppercaser.stop()
        uppercaser.join()

    def test_stderr_flush(self):
        # sys.stderr is flushed at process shutdown (issue #13812)
        if self.TYPE == "threads":
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        testfn = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, testfn)
        proc = self.Process(target=self._test_stderr_flush, args=(testfn,))
        proc.start()
        proc.join()
        with open(testfn, encoding="utf-8") as f:
            err = f.read()
            # The whole traceback was printed
            self.assertIn("ZeroDivisionError", err)
            self.assertIn("test_multiprocessing.py", err)
            self.assertIn("1/0 # MARKER", err)

    @classmethod
    def _test_stderr_flush(cls, testfn):
        fd = os.open(testfn, os.O_WRONLY | os.O_CREAT | os.O_EXCL)
        sys.stderr = open(fd, 'w', encoding="utf-8", closefd=False)
        1/0 # MARKER


    @classmethod
    def _test_sys_exit(cls, reason, testfn):
        fd = os.open(testfn, os.O_WRONLY | os.O_CREAT | os.O_EXCL)
        sys.stderr = open(fd, 'w', encoding="utf-8", closefd=False)
        sys.exit(reason)

    def test_sys_exit(self):
        # See Issue 13854
        if self.TYPE == 'threads':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        testfn = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, testfn)

        for reason in (
            [1, 2, 3],
            'ignore this',
        ):
            p = self.Process(target=self._test_sys_exit, args=(reason, testfn))
            p.daemon = True
            p.start()
            join_process(p)
            self.assertEqual(p.exitcode, 1)

            with open(testfn, encoding="utf-8") as f:
                content = f.read()
            self.assertEqual(content.rstrip(), str(reason))

            os.unlink(testfn)

        cases = [
            ((True,), 1),
            ((False,), 0),
            ((8,), 8),
            ((None,), 0),
            ((), 0),
            ]

        for args, expected in cases:
            with self.subTest(args=args):
                p = self.Process(target=sys.exit, args=args)
                p.daemon = True
                p.start()
                join_process(p)
                self.assertEqual(p.exitcode, expected)

#
#
#

def queue_empty(q):
    if hasattr(q, 'empty'):
        return q.empty()
    else:
        return q.qsize() == 0

def queue_full(q, maxsize):
    if hasattr(q, 'full'):
        return q.full()
    else:
        return q.qsize() == maxsize


class _TestQueue(BaseTestCase):


    @classmethod
    def _test_put(cls, queue, child_can_start, parent_can_continue):
        child_can_start.wait()
        for i in range(6):
            queue.get()
        parent_can_continue.set()

    def test_put(self):
        MAXSIZE = 6
        queue = self.Queue(maxsize=MAXSIZE)
        child_can_start = self.Event()
        parent_can_continue = self.Event()

        proc = self.Process(
            target=self._test_put,
            args=(queue, child_can_start, parent_can_continue)
            )
        proc.daemon = True
        proc.start()

        self.assertEqual(queue_empty(queue), True)
        self.assertEqual(queue_full(queue, MAXSIZE), False)

        queue.put(1)
        queue.put(2, True)
        queue.put(3, True, None)
        queue.put(4, False)
        queue.put(5, False, None)
        queue.put_nowait(6)

        # the values may be in buffer but not yet in pipe so sleep a bit
        time.sleep(DELTA)

        self.assertEqual(queue_empty(queue), False)
        self.assertEqual(queue_full(queue, MAXSIZE), True)

        put = TimingWrapper(queue.put)
        put_nowait = TimingWrapper(queue.put_nowait)

        self.assertRaises(pyqueue.Full, put, 7, False)
        self.assertTimingAlmostEqual(put.elapsed, 0)

        self.assertRaises(pyqueue.Full, put, 7, False, None)
        self.assertTimingAlmostEqual(put.elapsed, 0)

        self.assertRaises(pyqueue.Full, put_nowait, 7)
        self.assertTimingAlmostEqual(put_nowait.elapsed, 0)

        self.assertRaises(pyqueue.Full, put, 7, True, TIMEOUT1)
        self.assertTimingAlmostEqual(put.elapsed, TIMEOUT1)

        self.assertRaises(pyqueue.Full, put, 7, False, TIMEOUT2)
        self.assertTimingAlmostEqual(put.elapsed, 0)

        self.assertRaises(pyqueue.Full, put, 7, True, timeout=TIMEOUT3)
        self.assertTimingAlmostEqual(put.elapsed, TIMEOUT3)

        child_can_start.set()
        parent_can_continue.wait()

        self.assertEqual(queue_empty(queue), True)
        self.assertEqual(queue_full(queue, MAXSIZE), False)

        proc.join()
        close_queue(queue)

    @classmethod
    def _test_get(cls, queue, child_can_start, parent_can_continue):
        child_can_start.wait()
        #queue.put(1)
        queue.put(2)
        queue.put(3)
        queue.put(4)
        queue.put(5)
        parent_can_continue.set()

    def test_get(self):
        queue = self.Queue()
        child_can_start = self.Event()
        parent_can_continue = self.Event()

        proc = self.Process(
            target=self._test_get,
            args=(queue, child_can_start, parent_can_continue)
            )
        proc.daemon = True
        proc.start()

        self.assertEqual(queue_empty(queue), True)

        child_can_start.set()
        parent_can_continue.wait()

        time.sleep(DELTA)
        self.assertEqual(queue_empty(queue), False)

        # Hangs unexpectedly, remove for now
        #self.assertEqual(queue.get(), 1)
        self.assertEqual(queue.get(True, None), 2)
        self.assertEqual(queue.get(True), 3)
        self.assertEqual(queue.get(timeout=1), 4)
        self.assertEqual(queue.get_nowait(), 5)

        self.assertEqual(queue_empty(queue), True)

        get = TimingWrapper(queue.get)
        get_nowait = TimingWrapper(queue.get_nowait)

        self.assertRaises(pyqueue.Empty, get, False)
        self.assertTimingAlmostEqual(get.elapsed, 0)

        self.assertRaises(pyqueue.Empty, get, False, None)
        self.assertTimingAlmostEqual(get.elapsed, 0)

        self.assertRaises(pyqueue.Empty, get_nowait)
        self.assertTimingAlmostEqual(get_nowait.elapsed, 0)

        self.assertRaises(pyqueue.Empty, get, True, TIMEOUT1)
        self.assertTimingAlmostEqual(get.elapsed, TIMEOUT1)

        self.assertRaises(pyqueue.Empty, get, False, TIMEOUT2)
        self.assertTimingAlmostEqual(get.elapsed, 0)

        self.assertRaises(pyqueue.Empty, get, timeout=TIMEOUT3)
        self.assertTimingAlmostEqual(get.elapsed, TIMEOUT3)

        proc.join()
        close_queue(queue)

    @classmethod
    def _test_fork(cls, queue):
        for i in range(10, 20):
            queue.put(i)
        # note that at this point the items may only be buffered, so the
        # process cannot shutdown until the feeder thread has finished
        # pushing items onto the pipe.

    def test_fork(self):
        # Old versions of Queue would fail to create a new feeder
        # thread for a forked process if the original process had its
        # own feeder thread.  This test checks that this no longer
        # happens.

        queue = self.Queue()

        # put items on queue so that main process starts a feeder thread
        for i in range(10):
            queue.put(i)

        # wait to make sure thread starts before we fork a new process
        time.sleep(DELTA)

        # fork process
        p = self.Process(target=self._test_fork, args=(queue,))
        p.daemon = True
        p.start()

        # check that all expected items are in the queue
        for i in range(20):
            self.assertEqual(queue.get(), i)
        self.assertRaises(pyqueue.Empty, queue.get, False)

        p.join()
        close_queue(queue)

    def test_qsize(self):
        q = self.Queue()
        try:
            self.assertEqual(q.qsize(), 0)
        except NotImplementedError:
            self.skipTest('qsize method not implemented')
        q.put(1)
        self.assertEqual(q.qsize(), 1)
        q.put(5)
        self.assertEqual(q.qsize(), 2)
        q.get()
        self.assertEqual(q.qsize(), 1)
        q.get()
        self.assertEqual(q.qsize(), 0)
        close_queue(q)

    @classmethod
    def _test_task_done(cls, q):
        for obj in iter(q.get, None):
            time.sleep(DELTA)
            q.task_done()

    def test_task_done(self):
        queue = self.JoinableQueue()

        workers = [self.Process(target=self._test_task_done, args=(queue,))
                   for i in range(4)]

        for p in workers:
            p.daemon = True
            p.start()

        for i in range(10):
            queue.put(i)

        queue.join()

        for p in workers:
            queue.put(None)

        for p in workers:
            p.join()
        close_queue(queue)

    def test_no_import_lock_contention(self):
        with os_helper.temp_cwd():
            module_name = 'imported_by_an_imported_module'
            with open(module_name + '.py', 'w', encoding="utf-8") as f:
                f.write("""if 1:
                    import multiprocessing

                    q = multiprocessing.Queue()
                    q.put('knock knock')
                    q.get(timeout=3)
                    q.close()
                    del q
                """)

            with import_helper.DirsOnSysPath(os.getcwd()):
                try:
                    __import__(module_name)
                except pyqueue.Empty:
                    self.fail("Probable regression on import lock contention;"
                              " see Issue #22853")

    def test_timeout(self):
        q = multiprocessing.Queue()
        start = time.monotonic()
        self.assertRaises(pyqueue.Empty, q.get, True, 0.200)
        delta = time.monotonic() - start
        # bpo-30317: Tolerate a delta of 100 ms because of the bad clock
        # resolution on Windows (usually 15.6 ms). x86 Windows7 3.x once
        # failed because the delta was only 135.8 ms.
        self.assertGreaterEqual(delta, 0.100)
        close_queue(q)

    def test_queue_feeder_donot_stop_onexc(self):
        # bpo-30414: verify feeder handles exceptions correctly
        if self.TYPE != 'processes':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        class NotSerializable(object):
            def __reduce__(self):
                raise AttributeError
        with test.support.captured_stderr():
            q = self.Queue()
            q.put(NotSerializable())
            q.put(True)
            self.assertTrue(q.get(timeout=support.SHORT_TIMEOUT))
            close_queue(q)

        with test.support.captured_stderr():
            # bpo-33078: verify that the queue size is correctly handled
            # on errors.
            q = self.Queue(maxsize=1)
            q.put(NotSerializable())
            q.put(True)
            try:
                self.assertEqual(q.qsize(), 1)
            except NotImplementedError:
                # qsize is not available on all platform as it
                # relies on sem_getvalue
                pass
            self.assertTrue(q.get(timeout=support.SHORT_TIMEOUT))
            # Check that the size of the queue is correct
            self.assertTrue(q.empty())
            close_queue(q)

    def test_queue_feeder_on_queue_feeder_error(self):
        # bpo-30006: verify feeder handles exceptions using the
        # _on_queue_feeder_error hook.
        if self.TYPE != 'processes':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        class NotSerializable(object):
            """Mock unserializable object"""
            def __init__(self):
                self.reduce_was_called = False
                self.on_queue_feeder_error_was_called = False

            def __reduce__(self):
                self.reduce_was_called = True
                raise AttributeError

        class SafeQueue(multiprocessing.queues.Queue):
            """Queue with overloaded _on_queue_feeder_error hook"""
            @staticmethod
            def _on_queue_feeder_error(e, obj):
                if (isinstance(e, AttributeError) and
                        isinstance(obj, NotSerializable)):
                    obj.on_queue_feeder_error_was_called = True

        not_serializable_obj = NotSerializable()
        # The captured_stderr reduces the noise in the test report
        with test.support.captured_stderr():
            q = SafeQueue(ctx=multiprocessing.get_context())
            q.put(not_serializable_obj)

            # Verify that q is still functioning correctly
            q.put(True)
            self.assertTrue(q.get(timeout=support.SHORT_TIMEOUT))

        # Assert that the serialization and the hook have been called correctly
        self.assertTrue(not_serializable_obj.reduce_was_called)
        self.assertTrue(not_serializable_obj.on_queue_feeder_error_was_called)

    def test_closed_queue_empty_exceptions(self):
        # Assert that checking the emptiness of an unused closed queue
        # does not raise an OSError. The rationale is that q.close() is
        # a no-op upon construction and becomes effective once the queue
        # has been used (e.g., by calling q.put()).
        for q in multiprocessing.Queue(), multiprocessing.JoinableQueue():
            q.close()  # this is a no-op since the feeder thread is None
            q.join_thread()  # this is also a no-op
            self.assertTrue(q.empty())

        for q in multiprocessing.Queue(), multiprocessing.JoinableQueue():
            q.put('foo')  # make sure that the queue is 'used'
            q.close()  # close the feeder thread
            q.join_thread()  # make sure to join the feeder thread
            with self.assertRaisesRegex(OSError, 'is closed'):
                q.empty()

    def test_closed_queue_put_get_exceptions(self):
        for q in multiprocessing.Queue(), multiprocessing.JoinableQueue():
            q.close()
            with self.assertRaisesRegex(ValueError, 'is closed'):
                q.put('foo')
            with self.assertRaisesRegex(ValueError, 'is closed'):
                q.get()
#
#
#

class _TestLock(BaseTestCase):

    @staticmethod
    def _acquire(lock, l=None):
        lock.acquire()
        if l is not None:
            l.append(repr(lock))

    @staticmethod
    def _acquire_event(lock, event):
        lock.acquire()
        event.set()
        time.sleep(1.0)

    def test_repr_lock(self):
        if self.TYPE != 'processes':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        lock = self.Lock()
        self.assertEqual(f'<Lock(owner=None)>', repr(lock))

        lock.acquire()
        self.assertEqual(f'<Lock(owner=MainProcess)>', repr(lock))
        lock.release()

        tname = 'T1'
        l = []
        t = threading.Thread(target=self._acquire,
                             args=(lock, l),
                             name=tname)
        t.start()
        time.sleep(0.1)
        self.assertEqual(f'<Lock(owner=MainProcess|{tname})>', l[0])
        lock.release()

        t = threading.Thread(target=self._acquire,
                             args=(lock,),
                             name=tname)
        t.start()
        time.sleep(0.1)
        self.assertEqual('<Lock(owner=SomeOtherThread)>', repr(lock))
        lock.release()

        pname = 'P1'
        l = multiprocessing.Manager().list()
        p = self.Process(target=self._acquire,
                         args=(lock, l),
                         name=pname)
        p.start()
        p.join()
        self.assertEqual(f'<Lock(owner={pname})>', l[0])

        lock = self.Lock()
        event = self.Event()
        p = self.Process(target=self._acquire_event,
                         args=(lock, event),
                         name='P2')
        p.start()
        event.wait()
        self.assertEqual(f'<Lock(owner=SomeOtherProcess)>', repr(lock))
        p.terminate()

    def test_lock(self):
        lock = self.Lock()
        self.assertEqual(lock.acquire(), True)
        self.assertTrue(lock.locked())
        self.assertEqual(lock.acquire(False), False)
        self.assertEqual(lock.release(), None)
        self.assertFalse(lock.locked())
        self.assertRaises((ValueError, threading.ThreadError), lock.release)

    @classmethod
    def _test_lock_locked_2processes(cls, lock, event, res):
        lock.acquire()
        res.value = lock.locked()
        event.set()

    def test_lock_locked_2processes(self):
        if self.TYPE != 'processes':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        lock = self.Lock()
        event = self.Event()
        res = self.Value('b', 0)
        p = self.Process(target=self._test_lock_locked_2processes,
                         args=(lock, event, res))
        p.start()
        event.wait()
        self.assertTrue(lock.locked())
        self.assertTrue(res.value)
        p.join()

    @staticmethod
    def _acquire_release(lock, timeout, l=None, n=1):
        for _ in range(n):
            lock.acquire()
        if l is not None:
            l.append(repr(lock))
        time.sleep(timeout)
        for _ in range(n):
            lock.release()

    def test_repr_rlock(self):
        if self.TYPE != 'processes':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        lock = self.RLock()
        self.assertEqual('<RLock(None, 0)>', repr(lock))

        n = 3
        for _ in range(n):
            lock.acquire()
        self.assertEqual(f'<RLock(MainProcess, {n})>', repr(lock))
        for _ in range(n):
            lock.release()

        t, l = [], []
        for i in range(n):
            t.append(threading.Thread(target=self._acquire_release,
                                      args=(lock, 0.1, l, i+1),
                                      name=f'T{i+1}'))
            t[-1].start()
        for t_ in t:
            t_.join()
        for i in range(n):
            self.assertIn(f'<RLock(MainProcess|T{i+1}, {i+1})>', l)

        rlock = self.RLock()
        t = threading.Thread(target=rlock.acquire)
        t.start()
        t.join()
        self.assertEqual('<RLock(SomeOtherThread, nonzero)>', repr(rlock))

        pname = 'P1'
        l = multiprocessing.Manager().list()
        p = self.Process(target=self._acquire_release,
                         args=(lock, 0.1, l),
                         name=pname)
        p.start()
        p.join()
        self.assertEqual(f'<RLock({pname}, 1)>', l[0])

        rlock = self.RLock()
        p = self.Process(target=self._acquire, args=(rlock,))
        p.start()
        p.join()
        self.assertEqual('<RLock(SomeOtherProcess, nonzero)>', repr(rlock))

    def test_rlock(self):
        lock = self.RLock()
        self.assertEqual(lock.acquire(), True)
        self.assertTrue(lock.locked())
        self.assertEqual(lock.acquire(), True)
        self.assertEqual(lock.acquire(), True)
        self.assertEqual(lock.release(), None)
        self.assertTrue(lock.locked())
        self.assertEqual(lock.release(), None)
        self.assertEqual(lock.release(), None)
        self.assertFalse(lock.locked())
        self.assertRaises((AssertionError, RuntimeError), lock.release)

    def test_rlock_locked_2processes(self):
        if self.TYPE != 'processes':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        rlock = self.RLock()
        event = self.Event()
        res = Value('b', 0)
        # target is the same as for the test_lock_locked_2processes test.
        p = self.Process(target=self._test_lock_locked_2processes,
                         args=(rlock, event, res))
        p.start()
        event.wait()
        self.assertTrue(rlock.locked())
        self.assertTrue(res.value)
        p.join()

    def test_lock_context(self):
        with self.Lock() as locked:
            self.assertTrue(locked)

    def test_rlock_context(self):
        with self.RLock() as locked:
            self.assertTrue(locked)


class _TestSemaphore(BaseTestCase):

    def _test_semaphore(self, sem):
        self.assertReturnsIfImplemented(2, get_value, sem)
        self.assertEqual(sem.acquire(), True)
        self.assertReturnsIfImplemented(1, get_value, sem)
        self.assertEqual(sem.acquire(), True)
        self.assertReturnsIfImplemented(0, get_value, sem)
        self.assertEqual(sem.acquire(False), False)
        self.assertReturnsIfImplemented(0, get_value, sem)
        self.assertEqual(sem.release(), None)
        self.assertReturnsIfImplemented(1, get_value, sem)
        self.assertEqual(sem.release(), None)
        self.assertReturnsIfImplemented(2, get_value, sem)

    def test_semaphore(self):
        sem = self.Semaphore(2)
        self._test_semaphore(sem)
        self.assertEqual(sem.release(), None)
        self.assertReturnsIfImplemented(3, get_value, sem)
        self.assertEqual(sem.release(), None)
        self.assertReturnsIfImplemented(4, get_value, sem)

    def test_bounded_semaphore(self):
        sem = self.BoundedSemaphore(2)
        self._test_semaphore(sem)
        # Currently fails on OS/X
        #if HAVE_GETVALUE:
        #    self.assertRaises(ValueError, sem.release)
        #    self.assertReturnsIfImplemented(2, get_value, sem)

    def test_timeout(self):
        if self.TYPE != 'processes':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        sem = self.Semaphore(0)
        acquire = TimingWrapper(sem.acquire)

        self.assertEqual(acquire(False), False)
        self.assertTimingAlmostEqual(acquire.elapsed, 0.0)

        self.assertEqual(acquire(False, None), False)
        self.assertTimingAlmostEqual(acquire.elapsed, 0.0)

        self.assertEqual(acquire(False, TIMEOUT1), False)
        self.assertTimingAlmostEqual(acquire.elapsed, 0)

        self.assertEqual(acquire(True, TIMEOUT2), False)
        self.assertTimingAlmostEqual(acquire.elapsed, TIMEOUT2)

        self.assertEqual(acquire(timeout=TIMEOUT3), False)
        self.assertTimingAlmostEqual(acquire.elapsed, TIMEOUT3)


class _TestCondition(BaseTestCase):

    @classmethod
    def f(cls, cond, sleeping, woken, timeout=None):
        cond.acquire()
        sleeping.release()
        cond.wait(timeout)
        woken.release()
        cond.release()

    def assertReachesEventually(self, func, value):
        for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
            try:
                if func() == value:
                    break
            except NotImplementedError:
                break

        self.assertReturnsIfImplemented(value, func)

    def check_invariant(self, cond):
        # this is only supposed to succeed when there are no sleepers
        if self.TYPE == 'processes':
            try:
                sleepers = (cond._sleeping_count.get_value() -
                            cond._woken_count.get_value())
                self.assertEqual(sleepers, 0)
                self.assertEqual(cond._wait_semaphore.get_value(), 0)
            except NotImplementedError:
                pass

    def test_notify(self):
        cond = self.Condition()
        sleeping = self.Semaphore(0)
        woken = self.Semaphore(0)

        p = self.Process(target=self.f, args=(cond, sleeping, woken))
        p.daemon = True
        p.start()

        t = threading.Thread(target=self.f, args=(cond, sleeping, woken))
        t.daemon = True
        t.start()

        # wait for both children to start sleeping
        sleeping.acquire()
        sleeping.acquire()

        # check no process/thread has woken up
        self.assertReachesEventually(lambda: get_value(woken), 0)

        # wake up one process/thread
        cond.acquire()
        cond.notify()
        cond.release()

        # check one process/thread has woken up
        self.assertReachesEventually(lambda: get_value(woken), 1)

        # wake up another
        cond.acquire()
        cond.notify()
        cond.release()

        # check other has woken up
        self.assertReachesEventually(lambda: get_value(woken), 2)

        # check state is not mucked up
        self.check_invariant(cond)

        threading_helper.join_thread(t)
        join_process(p)

    def test_notify_all(self):
        cond = self.Condition()
        sleeping = self.Semaphore(0)
        woken = self.Semaphore(0)

        # start some threads/processes which will timeout
        workers = []
        for i in range(3):
            p = self.Process(target=self.f,
                             args=(cond, sleeping, woken, TIMEOUT1))
            p.daemon = True
            p.start()
            workers.append(p)

            t = threading.Thread(target=self.f,
                                 args=(cond, sleeping, woken, TIMEOUT1))
            t.daemon = True
            t.start()
            workers.append(t)

        # wait for them all to sleep
        for i in range(6):
            sleeping.acquire()

        # check they have all timed out
        for i in range(6):
            woken.acquire()
        self.assertReturnsIfImplemented(0, get_value, woken)

        # check state is not mucked up
        self.check_invariant(cond)

        # start some more threads/processes
        for i in range(3):
            p = self.Process(target=self.f, args=(cond, sleeping, woken))
            p.daemon = True
            p.start()
            workers.append(p)

            t = threading.Thread(target=self.f, args=(cond, sleeping, woken))
            t.daemon = True
            t.start()
            workers.append(t)

        # wait for them to all sleep
        for i in range(6):
            sleeping.acquire()

        # check no process/thread has woken up
        time.sleep(DELTA)
        self.assertReturnsIfImplemented(0, get_value, woken)

        # wake them all up
        cond.acquire()
        cond.notify_all()
        cond.release()

        # check they have all woken
        for i in range(6):
            woken.acquire()
        self.assertReturnsIfImplemented(0, get_value, woken)

        # check state is not mucked up
        self.check_invariant(cond)

        for w in workers:
            # NOTE: join_process and join_thread are the same
            threading_helper.join_thread(w)

    def test_notify_n(self):
        cond = self.Condition()
        sleeping = self.Semaphore(0)
        woken = self.Semaphore(0)

        # start some threads/processes
        workers = []
        for i in range(3):
            p = self.Process(target=self.f, args=(cond, sleeping, woken))
            p.daemon = True
            p.start()
            workers.append(p)

            t = threading.Thread(target=self.f, args=(cond, sleeping, woken))
            t.daemon = True
            t.start()
            workers.append(t)

        # wait for them to all sleep
        for i in range(6):
            sleeping.acquire()

        # check no process/thread has woken up
        time.sleep(DELTA)
        self.assertReturnsIfImplemented(0, get_value, woken)

        # wake some of them up
        cond.acquire()
        cond.notify(n=2)
        cond.release()

        # check 2 have woken
        self.assertReachesEventually(lambda: get_value(woken), 2)

        # wake the rest of them
        cond.acquire()
        cond.notify(n=4)
        cond.release()

        self.assertReachesEventually(lambda: get_value(woken), 6)

        # doesn't do anything more
        cond.acquire()
        cond.notify(n=3)
        cond.release()

        self.assertReturnsIfImplemented(6, get_value, woken)

        # check state is not mucked up
        self.check_invariant(cond)

        for w in workers:
            # NOTE: join_process and join_thread are the same
            threading_helper.join_thread(w)

    def test_timeout(self):
        cond = self.Condition()
        wait = TimingWrapper(cond.wait)
        cond.acquire()
        res = wait(TIMEOUT1)
        cond.release()
        self.assertEqual(res, False)
        self.assertTimingAlmostEqual(wait.elapsed, TIMEOUT1)

    @classmethod
    def _test_waitfor_f(cls, cond, state):
        with cond:
            state.value = 0
            cond.notify()
            result = cond.wait_for(lambda : state.value==4)
            if not result or state.value != 4:
                sys.exit(1)

    @unittest.skipUnless(HAS_SHAREDCTYPES, 'needs sharedctypes')
    def test_waitfor(self):
        # based on test in test/lock_tests.py
        cond = self.Condition()
        state = self.Value('i', -1)

        p = self.Process(target=self._test_waitfor_f, args=(cond, state))
        p.daemon = True
        p.start()

        with cond:
            result = cond.wait_for(lambda : state.value==0)
            self.assertTrue(result)
            self.assertEqual(state.value, 0)

        for i in range(4):
            time.sleep(0.01)
            with cond:
                state.value += 1
                cond.notify()

        join_process(p)
        self.assertEqual(p.exitcode, 0)

    @classmethod
    def _test_waitfor_timeout_f(cls, cond, state, success, sem):
        sem.release()
        with cond:
            expected = 0.100
            dt = time.monotonic()
            result = cond.wait_for(lambda : state.value==4, timeout=expected)
            dt = time.monotonic() - dt
            if not result and (expected - CLOCK_RES) <= dt:
                success.value = True

    @unittest.skipUnless(HAS_SHAREDCTYPES, 'needs sharedctypes')
    def test_waitfor_timeout(self):
        # based on test in test/lock_tests.py
        cond = self.Condition()
        state = self.Value('i', 0)
        success = self.Value('i', False)
        sem = self.Semaphore(0)

        p = self.Process(target=self._test_waitfor_timeout_f,
                         args=(cond, state, success, sem))
        p.daemon = True
        p.start()
        self.assertTrue(sem.acquire(timeout=support.LONG_TIMEOUT))

        # Only increment 3 times, so state == 4 is never reached.
        for i in range(3):
            time.sleep(0.010)
            with cond:
                state.value += 1
                cond.notify()

        join_process(p)
        self.assertTrue(success.value)

    @classmethod
    def _test_wait_result(cls, c, pid):
        with c:
            c.notify()
        time.sleep(1)
        if pid is not None:
            os.kill(pid, signal.SIGINT)

    def test_wait_result(self):
        if isinstance(self, ProcessesMixin) and sys.platform != 'win32':
            pid = os.getpid()
        else:
            pid = None

        c = self.Condition()
        with c:
            self.assertFalse(c.wait(0))
            self.assertFalse(c.wait(0.1))

            p = self.Process(target=self._test_wait_result, args=(c, pid))
            p.start()

            self.assertTrue(c.wait(60))
            if pid is not None:
                self.assertRaises(KeyboardInterrupt, c.wait, 60)

            p.join()


class _TestEvent(BaseTestCase):

    @classmethod
    def _test_event(cls, event):
        time.sleep(TIMEOUT2)
        event.set()

    def test_event(self):
        event = self.Event()
        wait = TimingWrapper(event.wait)

        # Removed temporarily, due to API shear, this does not
        # work with threading._Event objects. is_set == isSet
        self.assertEqual(event.is_set(), False)

        # Removed, threading.Event.wait() will return the value of the __flag
        # instead of None. API Shear with the semaphore backed mp.Event
        self.assertEqual(wait(0.0), False)
        self.assertTimingAlmostEqual(wait.elapsed, 0.0)
        self.assertEqual(wait(TIMEOUT1), False)
        self.assertTimingAlmostEqual(wait.elapsed, TIMEOUT1)

        event.set()

        # See note above on the API differences
        self.assertEqual(event.is_set(), True)
        self.assertEqual(wait(), True)
        self.assertTimingAlmostEqual(wait.elapsed, 0.0)
        self.assertEqual(wait(TIMEOUT1), True)
        self.assertTimingAlmostEqual(wait.elapsed, 0.0)
        # self.assertEqual(event.is_set(), True)

        event.clear()

        #self.assertEqual(event.is_set(), False)

        p = self.Process(target=self._test_event, args=(event,))
        p.daemon = True
        p.start()
        self.assertEqual(wait(), True)
        p.join()

    def test_repr(self) -> None:
        event = self.Event()
        if self.TYPE == 'processes':
            self.assertRegex(repr(event), r"<Event at .* unset>")
            event.set()
            self.assertRegex(repr(event), r"<Event at .* set>")
            event.clear()
            self.assertRegex(repr(event), r"<Event at .* unset>")
        elif self.TYPE == 'manager':
            self.assertRegex(repr(event), r"<EventProxy object, typeid 'Event' at .*")
            event.set()
            self.assertRegex(repr(event), r"<EventProxy object, typeid 'Event' at .*")


# Tests for Barrier - adapted from tests in test/lock_tests.py
#

# Many of the tests for threading.Barrier use a list as an atomic
# counter: a value is appended to increment the counter, and the
# length of the list gives the value.  We use the class DummyList
# for the same purpose.

class _DummyList(object):

    def __init__(self):
        wrapper = multiprocessing.heap.BufferWrapper(struct.calcsize('i'))
        lock = multiprocessing.Lock()
        self.__setstate__((wrapper, lock))
        self._lengthbuf[0] = 0

    def __setstate__(self, state):
        (self._wrapper, self._lock) = state
        self._lengthbuf = self._wrapper.create_memoryview().cast('i')

    def __getstate__(self):
        return (self._wrapper, self._lock)

    def append(self, _):
        with self._lock:
            self._lengthbuf[0] += 1

    def __len__(self):
        with self._lock:
            return self._lengthbuf[0]

def _wait():
    # A crude wait/yield function not relying on synchronization primitives.
    time.sleep(0.01)


class Bunch(object):
    """
    A bunch of threads.
    """
    def __init__(self, namespace, f, args, n, wait_before_exit=False):
        """
        Construct a bunch of `n` threads running the same function `f`.
        If `wait_before_exit` is True, the threads won't terminate until
        do_finish() is called.
        """
        self.f = f
        self.args = args
        self.n = n
        self.started = namespace.DummyList()
        self.finished = namespace.DummyList()
        self._can_exit = namespace.Event()
        if not wait_before_exit:
            self._can_exit.set()

        threads = []
        for i in range(n):
            p = namespace.Process(target=self.task)
            p.daemon = True
            p.start()
            threads.append(p)

        def finalize(threads):
            for p in threads:
                p.join()

        self._finalizer = weakref.finalize(self, finalize, threads)

    def task(self):
        pid = os.getpid()
        self.started.append(pid)
        try:
            self.f(*self.args)
        finally:
            self.finished.append(pid)
            self._can_exit.wait(30)
            assert self._can_exit.is_set()

    def wait_for_started(self):
        while len(self.started) < self.n:
            _wait()

    def wait_for_finished(self):
        while len(self.finished) < self.n:
            _wait()

    def do_finish(self):
        self._can_exit.set()

    def close(self):
        self._finalizer()


class AppendTrue(object):
    def __init__(self, obj):
        self.obj = obj
    def __call__(self):
        self.obj.append(True)


class _TestBarrier(BaseTestCase):
    """
    Tests for Barrier objects.
    """
    N = 5
    defaultTimeout = 30.0  # XXX Slow Windows buildbots need generous timeout

    def setUp(self):
        self.barrier = self.Barrier(self.N, timeout=self.defaultTimeout)

    def tearDown(self):
        self.barrier.abort()
        self.barrier = None

    def DummyList(self):
        if self.TYPE == 'threads':
            return []
        elif self.TYPE == 'manager':
            return self.manager.list()
        else:
            return _DummyList()

    def run_threads(self, f, args):
        b = Bunch(self, f, args, self.N-1)
        try:
            f(*args)
            b.wait_for_finished()
        finally:
            b.close()

    @classmethod
    def multipass(cls, barrier, results, n):
        m = barrier.parties
        assert m == cls.N
        for i in range(n):
            results[0].append(True)
            assert len(results[1]) == i * m
            barrier.wait()
            results[1].append(True)
            assert len(results[0]) == (i + 1) * m
            barrier.wait()
        try:
            assert barrier.n_waiting == 0
        except NotImplementedError:
            pass
        assert not barrier.broken

    def test_barrier(self, passes=1):
        """
        Test that a barrier is passed in lockstep
        """
        results = [self.DummyList(), self.DummyList()]
        self.run_threads(self.multipass, (self.barrier, results, passes))

    def test_barrier_10(self):
        """
        Test that a barrier works for 10 consecutive runs
        """
        return self.test_barrier(10)

    @classmethod
    def _test_wait_return_f(cls, barrier, queue):
        res = barrier.wait()
        queue.put(res)

    def test_wait_return(self):
        """
        test the return value from barrier.wait
        """
        queue = self.Queue()
        self.run_threads(self._test_wait_return_f, (self.barrier, queue))
        results = [queue.get() for i in range(self.N)]
        self.assertEqual(results.count(0), 1)
        close_queue(queue)

    @classmethod
    def _test_action_f(cls, barrier, results):
        barrier.wait()
        if len(results) != 1:
            raise RuntimeError

    def test_action(self):
        """
        Test the 'action' callback
        """
        results = self.DummyList()
        barrier = self.Barrier(self.N, action=AppendTrue(results))
        self.run_threads(self._test_action_f, (barrier, results))
        self.assertEqual(len(results), 1)

    @classmethod
    def _test_abort_f(cls, barrier, results1, results2):
        try:
            i = barrier.wait()
            if i == cls.N//2:
                raise RuntimeError
            barrier.wait()
            results1.append(True)
        except threading.BrokenBarrierError:
            results2.append(True)
        except RuntimeError:
            barrier.abort()

    def test_abort(self):
        """
        Test that an abort will put the barrier in a broken state
        """
        results1 = self.DummyList()
        results2 = self.DummyList()
        self.run_threads(self._test_abort_f,
                         (self.barrier, results1, results2))
        self.assertEqual(len(results1), 0)
        self.assertEqual(len(results2), self.N-1)
        self.assertTrue(self.barrier.broken)

    @classmethod
    def _test_reset_f(cls, barrier, results1, results2, results3):
        i = barrier.wait()
        if i == cls.N//2:
            # Wait until the other threads are all in the barrier.
            while barrier.n_waiting < cls.N-1:
                time.sleep(0.001)
            barrier.reset()
        else:
            try:
                barrier.wait()
                results1.append(True)
            except threading.BrokenBarrierError:
                results2.append(True)
        # Now, pass the barrier again
        barrier.wait()
        results3.append(True)

    def test_reset(self):
        """
        Test that a 'reset' on a barrier frees the waiting threads
        """
        results1 = self.DummyList()
        results2 = self.DummyList()
        results3 = self.DummyList()
        self.run_threads(self._test_reset_f,
                         (self.barrier, results1, results2, results3))
        self.assertEqual(len(results1), 0)
        self.assertEqual(len(results2), self.N-1)
        self.assertEqual(len(results3), self.N)

    @classmethod
    def _test_abort_and_reset_f(cls, barrier, barrier2,
                                results1, results2, results3):
        try:
            i = barrier.wait()
            if i == cls.N//2:
                raise RuntimeError
            barrier.wait()
            results1.append(True)
        except threading.BrokenBarrierError:
            results2.append(True)
        except RuntimeError:
            barrier.abort()
        # Synchronize and reset the barrier.  Must synchronize first so
        # that everyone has left it when we reset, and after so that no
        # one enters it before the reset.
        if barrier2.wait() == cls.N//2:
            barrier.reset()
        barrier2.wait()
        barrier.wait()
        results3.append(True)

    def test_abort_and_reset(self):
        """
        Test that a barrier can be reset after being broken.
        """
        results1 = self.DummyList()
        results2 = self.DummyList()
        results3 = self.DummyList()
        barrier2 = self.Barrier(self.N)

        self.run_threads(self._test_abort_and_reset_f,
                         (self.barrier, barrier2, results1, results2, results3))
        self.assertEqual(len(results1), 0)
        self.assertEqual(len(results2), self.N-1)
        self.assertEqual(len(results3), self.N)

    @classmethod
    def _test_timeout_f(cls, barrier, results):
        i = barrier.wait()
        if i == cls.N//2:
            # One thread is late!
            time.sleep(1.0)
        try:
            barrier.wait(0.5)
        except threading.BrokenBarrierError:
            results.append(True)

    def test_timeout(self):
        """
        Test wait(timeout)
        """
        results = self.DummyList()
        self.run_threads(self._test_timeout_f, (self.barrier, results))
        self.assertEqual(len(results), self.barrier.parties)

    @classmethod
    def _test_default_timeout_f(cls, barrier, results):
        i = barrier.wait(cls.defaultTimeout)
        if i == cls.N//2:
            # One thread is later than the default timeout
            time.sleep(1.0)
        try:
            barrier.wait()
        except threading.BrokenBarrierError:
            results.append(True)

    def test_default_timeout(self):
        """
        Test the barrier's default timeout
        """
        barrier = self.Barrier(self.N, timeout=0.5)
        results = self.DummyList()
        self.run_threads(self._test_default_timeout_f, (barrier, results))
        self.assertEqual(len(results), barrier.parties)

    def test_single_thread(self):
        b = self.Barrier(1)
        b.wait()
        b.wait()

    @classmethod
    def _test_thousand_f(cls, barrier, passes, conn, lock):
        for i in range(passes):
            barrier.wait()
            with lock:
                conn.send(i)

    def test_thousand(self):
        if self.TYPE == 'manager':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))
        passes = 1000
        lock = self.Lock()
        conn, child_conn = self.Pipe(False)
        for j in range(self.N):
            p = self.Process(target=self._test_thousand_f,
                           args=(self.barrier, passes, child_conn, lock))
            p.start()
            self.addCleanup(p.join)

        for i in range(passes):
            for j in range(self.N):
                self.assertEqual(conn.recv(), i)

#
#
#

class _TestValue(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    codes_values = [
        ('i', 4343, 24234),
        ('d', 3.625, -4.25),
        ('h', -232, 234),
        ('q', 2 ** 33, 2 ** 34),
        ('c', latin('x'), latin('y'))
        ]

    def setUp(self):
        if not HAS_SHAREDCTYPES:
            self.skipTest("requires multiprocessing.sharedctypes")

    @classmethod
    def _test(cls, values):
        for sv, cv in zip(values, cls.codes_values):
            sv.value = cv[2]


    def test_value(self, raw=False):
        if raw:
            values = [self.RawValue(code, value)
                      for code, value, _ in self.codes_values]
        else:
            values = [self.Value(code, value)
                      for code, value, _ in self.codes_values]

        for sv, cv in zip(values, self.codes_values):
            self.assertEqual(sv.value, cv[1])

        proc = self.Process(target=self._test, args=(values,))
        proc.daemon = True
        proc.start()
        proc.join()

        for sv, cv in zip(values, self.codes_values):
            self.assertEqual(sv.value, cv[2])

    def test_rawvalue(self):
        self.test_value(raw=True)

    def test_getobj_getlock(self):
        val1 = self.Value('i', 5)
        lock1 = val1.get_lock()
        obj1 = val1.get_obj()

        val2 = self.Value('i', 5, lock=None)
        lock2 = val2.get_lock()
        obj2 = val2.get_obj()

        lock = self.Lock()
        val3 = self.Value('i', 5, lock=lock)
        lock3 = val3.get_lock()
        obj3 = val3.get_obj()
        self.assertEqual(lock, lock3)

        arr4 = self.Value('i', 5, lock=False)
        self.assertNotHasAttr(arr4, 'get_lock')
        self.assertNotHasAttr(arr4, 'get_obj')

        self.assertRaises(AttributeError, self.Value, 'i', 5, lock='navalue')

        arr5 = self.RawValue('i', 5)
        self.assertNotHasAttr(arr5, 'get_lock')
        self.assertNotHasAttr(arr5, 'get_obj')


class _TestArray(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    @classmethod
    def f(cls, seq):
        for i in range(1, len(seq)):
            seq[i] += seq[i-1]

    @unittest.skipIf(c_int is None, "requires _ctypes")
    def test_array(self, raw=False):
        seq = [680, 626, 934, 821, 150, 233, 548, 982, 714, 831]
        if raw:
            arr = self.RawArray('i', seq)
        else:
            arr = self.Array('i', seq)

        self.assertEqual(len(arr), len(seq))
        self.assertEqual(arr[3], seq[3])
        self.assertEqual(list(arr[2:7]), list(seq[2:7]))

        arr[4:8] = seq[4:8] = array.array('i', [1, 2, 3, 4])

        self.assertEqual(list(arr[:]), seq)

        self.f(seq)

        p = self.Process(target=self.f, args=(arr,))
        p.daemon = True
        p.start()
        p.join()

        self.assertEqual(list(arr[:]), seq)

    @unittest.skipIf(c_int is None, "requires _ctypes")
    def test_array_from_size(self):
        size = 10
        # Test for zeroing (see issue #11675).
        # The repetition below strengthens the test by increasing the chances
        # of previously allocated non-zero memory being used for the new array
        # on the 2nd and 3rd loops.
        for _ in range(3):
            arr = self.Array('i', size)
            self.assertEqual(len(arr), size)
            self.assertEqual(list(arr), [0] * size)
            arr[:] = range(10)
            self.assertEqual(list(arr), list(range(10)))
            del arr

    @unittest.skipIf(c_int is None, "requires _ctypes")
    def test_rawarray(self):
        self.test_array(raw=True)

    @unittest.skipIf(c_int is None, "requires _ctypes")
    def test_getobj_getlock_obj(self):
        arr1 = self.Array('i', list(range(10)))
        lock1 = arr1.get_lock()
        obj1 = arr1.get_obj()

        arr2 = self.Array('i', list(range(10)), lock=None)
        lock2 = arr2.get_lock()
        obj2 = arr2.get_obj()

        lock = self.Lock()
        arr3 = self.Array('i', list(range(10)), lock=lock)
        lock3 = arr3.get_lock()
        obj3 = arr3.get_obj()
        self.assertEqual(lock, lock3)

        arr4 = self.Array('i', range(10), lock=False)
        self.assertNotHasAttr(arr4, 'get_lock')
        self.assertNotHasAttr(arr4, 'get_obj')
        self.assertRaises(AttributeError,
                          self.Array, 'i', range(10), lock='notalock')

        arr5 = self.RawArray('i', range(10))
        self.assertNotHasAttr(arr5, 'get_lock')
        self.assertNotHasAttr(arr5, 'get_obj')

#
#
#

class _TestContainers(BaseTestCase):

    ALLOWED_TYPES = ('manager',)

    def test_list(self):
        a = self.list(list(range(10)))
        self.assertEqual(a[:], list(range(10)))

        b = self.list()
        self.assertEqual(b[:], [])

        b.extend(list(range(5)))
        self.assertEqual(b[:], list(range(5)))

        self.assertEqual(b[2], 2)
        self.assertEqual(b[2:10], [2,3,4])

        b *= 2
        self.assertEqual(b[:], [0, 1, 2, 3, 4, 0, 1, 2, 3, 4])

        self.assertEqual(b + [5, 6], [0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 5, 6])

        self.assertEqual(a[:], list(range(10)))

        d = [a, b]
        e = self.list(d)
        self.assertEqual(
            [element[:] for element in e],
            [[0, 1, 2, 3, 4, 5, 6, 7, 8, 9], [0, 1, 2, 3, 4, 0, 1, 2, 3, 4]]
            )

        f = self.list([a])
        a.append('hello')
        self.assertEqual(f[0][:], [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 'hello'])

    def test_list_isinstance(self):
        a = self.list()
        self.assertIsInstance(a, collections.abc.MutableSequence)

        # MutableSequence also has __iter__, but we can iterate over
        # ListProxy using __getitem__ instead. Adding __iter__ to ListProxy
        # would change the behavior of a list modified during iteration.
        mutable_sequence_methods = (
            '__contains__', '__delitem__', '__getitem__', '__iadd__',
            '__len__', '__reversed__', '__setitem__', 'append',
            'clear', 'count', 'extend', 'index', 'insert', 'pop', 'remove',
            'reverse',
        )
        for name in mutable_sequence_methods:
            with self.subTest(name=name):
                self.assertTrue(callable(getattr(a, name)))

    def test_list_iter(self):
        a = self.list(list(range(10)))
        it = iter(a)
        self.assertEqual(list(it), list(range(10)))
        self.assertEqual(list(it), [])  # exhausted
        # list modified during iteration
        it = iter(a)
        a[0] = 100
        self.assertEqual(next(it), 100)

    def test_list_proxy_in_list(self):
        a = self.list([self.list(range(3)) for _i in range(3)])
        self.assertEqual([inner[:] for inner in a], [[0, 1, 2]] * 3)

        a[0][-1] = 55
        self.assertEqual(a[0][:], [0, 1, 55])
        for i in range(1, 3):
            self.assertEqual(a[i][:], [0, 1, 2])

        self.assertEqual(a[1].pop(), 2)
        self.assertEqual(len(a[1]), 2)
        for i in range(0, 3, 2):
            self.assertEqual(len(a[i]), 3)

        del a

        b = self.list()
        b.append(b)
        del b

    def test_dict(self):
        d = self.dict()
        indices = list(range(65, 70))
        for i in indices:
            d[i] = chr(i)
        self.assertEqual(d.copy(), dict((i, chr(i)) for i in indices))
        self.assertEqual(sorted(d.keys()), indices)
        self.assertEqual(sorted(d.values()), [chr(i) for i in indices])
        self.assertEqual(sorted(d.items()), [(i, chr(i)) for i in indices])

    def test_dict_isinstance(self):
        a = self.dict()
        self.assertIsInstance(a, collections.abc.MutableMapping)

        mutable_mapping_methods = (
            '__contains__', '__delitem__', '__eq__', '__getitem__', '__iter__',
            '__len__', '__ne__', '__setitem__', 'clear', 'get', 'items',
            'keys', 'pop', 'popitem', 'setdefault', 'update', 'values',
        )
        for name in mutable_mapping_methods:
            with self.subTest(name=name):
                self.assertTrue(callable(getattr(a, name)))

    def test_dict_iter(self):
        d = self.dict()
        indices = list(range(65, 70))
        for i in indices:
            d[i] = chr(i)
        it = iter(d)
        self.assertEqual(list(it), indices)
        self.assertEqual(list(it), [])  # exhausted
        # dictionary changed size during iteration
        it = iter(d)
        d.clear()
        self.assertRaises(RuntimeError, next, it)

    def test_dict_proxy_nested(self):
        pets = self.dict(ferrets=2, hamsters=4)
        supplies = self.dict(water=10, feed=3)
        d = self.dict(pets=pets, supplies=supplies)

        self.assertEqual(supplies['water'], 10)
        self.assertEqual(d['supplies']['water'], 10)

        d['supplies']['blankets'] = 5
        self.assertEqual(supplies['blankets'], 5)
        self.assertEqual(d['supplies']['blankets'], 5)

        d['supplies']['water'] = 7
        self.assertEqual(supplies['water'], 7)
        self.assertEqual(d['supplies']['water'], 7)

        del pets
        del supplies
        self.assertEqual(d['pets']['ferrets'], 2)
        d['supplies']['blankets'] = 11
        self.assertEqual(d['supplies']['blankets'], 11)

        pets = d['pets']
        supplies = d['supplies']
        supplies['water'] = 7
        self.assertEqual(supplies['water'], 7)
        self.assertEqual(d['supplies']['water'], 7)

        d.clear()
        self.assertEqual(len(d), 0)
        self.assertEqual(supplies['water'], 7)
        self.assertEqual(pets['hamsters'], 4)

        l = self.list([pets, supplies])
        l[0]['marmots'] = 1
        self.assertEqual(pets['marmots'], 1)
        self.assertEqual(l[0]['marmots'], 1)

        del pets
        del supplies
        self.assertEqual(l[0]['marmots'], 1)

        outer = self.list([[88, 99], l])
        self.assertIsInstance(outer[0], list)  # Not a ListProxy
        self.assertEqual(outer[-1][-1]['feed'], 3)

    def test_nested_queue(self):
        a = self.list() # Test queue inside list
        a.append(self.Queue())
        a[0].put(123)
        self.assertEqual(a[0].get(), 123)
        b = self.dict() # Test queue inside dict
        b[0] = self.Queue()
        b[0].put(456)
        self.assertEqual(b[0].get(), 456)

    def test_namespace(self):
        n = self.Namespace()
        n.name = 'Bob'
        n.job = 'Builder'
        n._hidden = 'hidden'
        self.assertEqual((n.name, n.job), ('Bob', 'Builder'))
        del n.job
        self.assertEqual(str(n), "Namespace(name='Bob')")
        self.assertHasAttr(n, 'name')
        self.assertNotHasAttr(n, 'job')

#
#
#

def sqr(x, wait=0.0, event=None):
    if event is None:
        time.sleep(wait)
    else:
        event.wait(wait)
    return x*x

def mul(x, y):
    return x*y

def raise_large_valuerror(wait):
    time.sleep(wait)
    raise ValueError("x" * 1024**2)

def identity(x):
    return x

class CountedObject(object):
    n_instances = 0

    def __new__(cls):
        cls.n_instances += 1
        return object.__new__(cls)

    def __del__(self):
        type(self).n_instances -= 1

class SayWhenError(ValueError): pass

def exception_throwing_generator(total, when):
    if when == -1:
        raise SayWhenError("Somebody said when")
    for i in range(total):
        if i == when:
            raise SayWhenError("Somebody said when")
        yield i


class _TestPool(BaseTestCase):

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.pool = cls.Pool(4)

    @classmethod
    def tearDownClass(cls):
        cls.pool.terminate()
        cls.pool.join()
        cls.pool = None
        super().tearDownClass()

    def test_apply(self):
        papply = self.pool.apply
        self.assertEqual(papply(sqr, (5,)), sqr(5))
        self.assertEqual(papply(sqr, (), {'x':3}), sqr(x=3))

    def test_map(self):
        pmap = self.pool.map
        self.assertEqual(pmap(sqr, list(range(10))), list(map(sqr, list(range(10)))))
        self.assertEqual(pmap(sqr, list(range(100)), chunksize=20),
                         list(map(sqr, list(range(100)))))

    def test_starmap(self):
        psmap = self.pool.starmap
        tuples = list(zip(range(10), range(9,-1, -1)))
        self.assertEqual(psmap(mul, tuples),
                         list(itertools.starmap(mul, tuples)))
        tuples = list(zip(range(100), range(99,-1, -1)))
        self.assertEqual(psmap(mul, tuples, chunksize=20),
                         list(itertools.starmap(mul, tuples)))

    def test_starmap_async(self):
        tuples = list(zip(range(100), range(99,-1, -1)))
        self.assertEqual(self.pool.starmap_async(mul, tuples).get(),
                         list(itertools.starmap(mul, tuples)))

    def test_map_async(self):
        self.assertEqual(self.pool.map_async(sqr, list(range(10))).get(),
                         list(map(sqr, list(range(10)))))

    def test_map_async_callbacks(self):
        call_args = self.manager.list() if self.TYPE == 'manager' else []
        self.pool.map_async(int, ['1'],
                            callback=call_args.append,
                            error_callback=call_args.append).wait()
        self.assertEqual(1, len(call_args))
        self.assertEqual([1], call_args[0])
        self.pool.map_async(int, ['a'],
                            callback=call_args.append,
                            error_callback=call_args.append).wait()
        self.assertEqual(2, len(call_args))
        self.assertIsInstance(call_args[1], ValueError)

    def test_map_unplicklable(self):
        # Issue #19425 -- failure to pickle should not cause a hang
        if self.TYPE == 'threads':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))
        class A(object):
            def __reduce__(self):
                raise RuntimeError('cannot pickle')
        with self.assertRaises(RuntimeError):
            self.pool.map(sqr, [A()]*10)

    def test_map_chunksize(self):
        try:
            self.pool.map_async(sqr, [], chunksize=1).get(timeout=TIMEOUT1)
        except multiprocessing.TimeoutError:
            self.fail("pool.map_async with chunksize stalled on null list")

    def test_map_handle_iterable_exception(self):
        if self.TYPE == 'manager':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        # SayWhenError seen at the very first of the iterable
        with self.assertRaises(SayWhenError):
            self.pool.map(sqr, exception_throwing_generator(1, -1), 1)
        # again, make sure it's reentrant
        with self.assertRaises(SayWhenError):
            self.pool.map(sqr, exception_throwing_generator(1, -1), 1)

        with self.assertRaises(SayWhenError):
            self.pool.map(sqr, exception_throwing_generator(10, 3), 1)

        class SpecialIterable:
            def __iter__(self):
                return self
            def __next__(self):
                raise SayWhenError
            def __len__(self):
                return 1
        with self.assertRaises(SayWhenError):
            self.pool.map(sqr, SpecialIterable(), 1)
        with self.assertRaises(SayWhenError):
            self.pool.map(sqr, SpecialIterable(), 1)

    def test_async(self):
        res = self.pool.apply_async(sqr, (7, TIMEOUT1,))
        get = TimingWrapper(res.get)
        self.assertEqual(get(), 49)
        self.assertTimingAlmostEqual(get.elapsed, TIMEOUT1)

    def test_async_timeout(self):
        p = self.Pool(3)
        try:
            event = threading.Event() if self.TYPE == 'threads' else None
            res = p.apply_async(sqr, (6, TIMEOUT2 + support.SHORT_TIMEOUT, event))
            get = TimingWrapper(res.get)
            self.assertRaises(multiprocessing.TimeoutError, get, timeout=TIMEOUT2)
            self.assertTimingAlmostEqual(get.elapsed, TIMEOUT2)
        finally:
            if event is not None:
                event.set()
            p.terminate()
            p.join()

    def test_imap(self):
        it = self.pool.imap(sqr, list(range(10)))
        self.assertEqual(list(it), list(map(sqr, list(range(10)))))

        it = self.pool.imap(sqr, list(range(10)))
        for i in range(10):
            self.assertEqual(next(it), i*i)
        self.assertRaises(StopIteration, it.__next__)

        it = self.pool.imap(sqr, list(range(1000)), chunksize=100)
        for i in range(1000):
            self.assertEqual(next(it), i*i)
        self.assertRaises(StopIteration, it.__next__)

    def test_imap_handle_iterable_exception(self):
        if self.TYPE == 'manager':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        # SayWhenError seen at the very first of the iterable
        it = self.pool.imap(sqr, exception_throwing_generator(1, -1), 1)
        self.assertRaises(SayWhenError, it.__next__)
        # again, make sure it's reentrant
        it = self.pool.imap(sqr, exception_throwing_generator(1, -1), 1)
        self.assertRaises(SayWhenError, it.__next__)

        it = self.pool.imap(sqr, exception_throwing_generator(10, 3), 1)
        for i in range(3):
            self.assertEqual(next(it), i*i)
        self.assertRaises(SayWhenError, it.__next__)

        # SayWhenError seen at start of problematic chunk's results
        it = self.pool.imap(sqr, exception_throwing_generator(20, 7), 2)
        for i in range(6):
            self.assertEqual(next(it), i*i)
        self.assertRaises(SayWhenError, it.__next__)
        it = self.pool.imap(sqr, exception_throwing_generator(20, 7), 4)
        for i in range(4):
            self.assertEqual(next(it), i*i)
        self.assertRaises(SayWhenError, it.__next__)

    def test_imap_unordered(self):
        it = self.pool.imap_unordered(sqr, list(range(10)))
        self.assertEqual(sorted(it), list(map(sqr, list(range(10)))))

        it = self.pool.imap_unordered(sqr, list(range(1000)), chunksize=100)
        self.assertEqual(sorted(it), list(map(sqr, list(range(1000)))))

    def test_imap_unordered_handle_iterable_exception(self):
        if self.TYPE == 'manager':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        # SayWhenError seen at the very first of the iterable
        it = self.pool.imap_unordered(sqr,
                                      exception_throwing_generator(1, -1),
                                      1)
        self.assertRaises(SayWhenError, it.__next__)
        # again, make sure it's reentrant
        it = self.pool.imap_unordered(sqr,
                                      exception_throwing_generator(1, -1),
                                      1)
        self.assertRaises(SayWhenError, it.__next__)

        it = self.pool.imap_unordered(sqr,
                                      exception_throwing_generator(10, 3),
                                      1)
        expected_values = list(map(sqr, list(range(10))))
        with self.assertRaises(SayWhenError):
            # imap_unordered makes it difficult to anticipate the SayWhenError
            for i in range(10):
                value = next(it)
                self.assertIn(value, expected_values)
                expected_values.remove(value)

        it = self.pool.imap_unordered(sqr,
                                      exception_throwing_generator(20, 7),
                                      2)
        expected_values = list(map(sqr, list(range(20))))
        with self.assertRaises(SayWhenError):
            for i in range(20):
                value = next(it)
                self.assertIn(value, expected_values)
                expected_values.remove(value)

    def test_make_pool(self):
        expected_error = (RemoteError if self.TYPE == 'manager'
                          else ValueError)

        self.assertRaises(expected_error, self.Pool, -1)
        self.assertRaises(expected_error, self.Pool, 0)

        if self.TYPE != 'manager':
            p = self.Pool(3)
            try:
                self.assertEqual(3, len(p._pool))
            finally:
                p.close()
                p.join()

    def test_terminate(self):
        # Simulate slow tasks which take "forever" to complete
        sleep_time = support.LONG_TIMEOUT

        if self.TYPE == 'threads':
            # Thread pool workers can't be forced to quit, so if the first
            # task starts early enough, we will end up waiting for it.
            # Sleep for a shorter time, so the test doesn't block.
            sleep_time = 1

        p = self.Pool(3)
        args = [sleep_time for i in range(10_000)]
        result = p.map_async(time.sleep, args, chunksize=1)
        time.sleep(0.2)  # give some tasks a chance to start
        p.terminate()
        p.join()

    def test_empty_iterable(self):
        # See Issue 12157
        p = self.Pool(1)

        self.assertEqual(p.map(sqr, []), [])
        self.assertEqual(list(p.imap(sqr, [])), [])
        self.assertEqual(list(p.imap_unordered(sqr, [])), [])
        self.assertEqual(p.map_async(sqr, []).get(), [])

        p.close()
        p.join()

    def test_context(self):
        if self.TYPE == 'processes':
            L = list(range(10))
            expected = [sqr(i) for i in L]
            with self.Pool(2) as p:
                r = p.map_async(sqr, L)
                self.assertEqual(r.get(), expected)
            p.join()
            self.assertRaises(ValueError, p.map_async, sqr, L)

    @classmethod
    def _test_traceback(cls):
        raise RuntimeError(123) # some comment

    def test_traceback(self):
        # We want ensure that the traceback from the child process is
        # contained in the traceback raised in the main process.
        if self.TYPE == 'processes':
            with self.Pool(1) as p:
                try:
                    p.apply(self._test_traceback)
                except Exception as e:
                    exc = e
                else:
                    self.fail('expected RuntimeError')
            p.join()
            self.assertIs(type(exc), RuntimeError)
            self.assertEqual(exc.args, (123,))
            cause = exc.__cause__
            self.assertIs(type(cause), multiprocessing.pool.RemoteTraceback)
            self.assertIn('raise RuntimeError(123) # some comment', cause.tb)

            with test.support.captured_stderr() as f1:
                try:
                    raise exc
                except RuntimeError:
                    sys.excepthook(*sys.exc_info())
            self.assertIn('raise RuntimeError(123) # some comment',
                          f1.getvalue())
            # _helper_reraises_exception should not make the error
            # a remote exception
            with self.Pool(1) as p:
                try:
                    p.map(sqr, exception_throwing_generator(1, -1), 1)
                except Exception as e:
                    exc = e
                else:
                    self.fail('expected SayWhenError')
                self.assertIs(type(exc), SayWhenError)
                self.assertIs(exc.__cause__, None)
            p.join()

    @classmethod
    def _test_wrapped_exception(cls):
        raise RuntimeError('foo')

    def test_wrapped_exception(self):
        # Issue #20980: Should not wrap exception when using thread pool
        with self.Pool(1) as p:
            with self.assertRaises(RuntimeError):
                p.apply(self._test_wrapped_exception)
        p.join()

    def test_map_no_failfast(self):
        # Issue #23992: the fail-fast behaviour when an exception is raised
        # during map() would make Pool.join() deadlock, because a worker
        # process would fill the result queue (after the result handler thread
        # terminated, hence not draining it anymore).

        t_start = time.monotonic()

        with self.assertRaises(ValueError):
            with self.Pool(2) as p:
                try:
                    p.map(raise_large_valuerror, [0, 1])
                finally:
                    time.sleep(0.5)
                    p.close()
                    p.join()

        # check that we indeed waited for all jobs
        self.assertGreater(time.monotonic() - t_start, 0.9)

    def test_release_task_refs(self):
        # Issue #29861: task arguments and results should not be kept
        # alive after we are done with them.
        objs = [CountedObject() for i in range(10)]
        refs = [weakref.ref(o) for o in objs]
        self.pool.map(identity, objs)

        del objs
        time.sleep(DELTA)  # let threaded cleanup code run
        support.gc_collect()  # For PyPy or other GCs.
        self.assertEqual(set(wr() for wr in refs), {None})
        # With a process pool, copies of the objects are returned, check
        # they were released too.
        self.assertEqual(CountedObject.n_instances, 0)

    def test_enter(self):
        if self.TYPE == 'manager':
            self.skipTest("test not applicable to manager")

        pool = self.Pool(1)
        with pool:
            pass
            # call pool.terminate()
        # pool is no longer running

        with self.assertRaises(ValueError):
            # bpo-35477: pool.__enter__() fails if the pool is not running
            with pool:
                pass
        pool.join()

    def test_resource_warning(self):
        if self.TYPE == 'manager':
            self.skipTest("test not applicable to manager")

        pool = self.Pool(1)
        pool.terminate()
        pool.join()

        # force state to RUN to emit ResourceWarning in __del__()
        pool._state = multiprocessing.pool.RUN

        with warnings_helper.check_warnings(
                ('unclosed running multiprocessing pool', ResourceWarning)):
            pool = None
            support.gc_collect()

def raising():
    raise KeyError("key")

def unpickleable_result():
    return lambda: 42

class _TestPoolWorkerErrors(BaseTestCase):
    ALLOWED_TYPES = ('processes', )

    def test_async_error_callback(self):
        p = multiprocessing.Pool(2)

        scratchpad = [None]
        def errback(exc):
            scratchpad[0] = exc

        res = p.apply_async(raising, error_callback=errback)
        self.assertRaises(KeyError, res.get)
        self.assertTrue(scratchpad[0])
        self.assertIsInstance(scratchpad[0], KeyError)

        p.close()
        p.join()

    def test_unpickleable_result(self):
        from multiprocessing.pool import MaybeEncodingError
        p = multiprocessing.Pool(2)

        # Make sure we don't lose pool processes because of encoding errors.
        for iteration in range(20):

            scratchpad = [None]
            def errback(exc):
                scratchpad[0] = exc

            res = p.apply_async(unpickleable_result, error_callback=errback)
            self.assertRaises(MaybeEncodingError, res.get)
            wrapped = scratchpad[0]
            self.assertTrue(wrapped)
            self.assertIsInstance(scratchpad[0], MaybeEncodingError)
            self.assertIsNotNone(wrapped.exc)
            self.assertIsNotNone(wrapped.value)

        p.close()
        p.join()

class _TestPoolWorkerLifetime(BaseTestCase):
    ALLOWED_TYPES = ('processes', )

    def test_pool_worker_lifetime(self):
        p = multiprocessing.Pool(3, maxtasksperchild=10)
        self.assertEqual(3, len(p._pool))
        origworkerpids = [w.pid for w in p._pool]
        # Run many tasks so each worker gets replaced (hopefully)
        results = []
        for i in range(100):
            results.append(p.apply_async(sqr, (i, )))
        # Fetch the results and verify we got the right answers,
        # also ensuring all the tasks have completed.
        for (j, res) in enumerate(results):
            self.assertEqual(res.get(), sqr(j))
        # Refill the pool
        p._repopulate_pool()
        # Wait until all workers are alive
        # (countdown * DELTA = 5 seconds max startup process time)
        countdown = 50
        while countdown and not all(w.is_alive() for w in p._pool):
            countdown -= 1
            time.sleep(DELTA)
        finalworkerpids = [w.pid for w in p._pool]
        # All pids should be assigned.  See issue #7805.
        self.assertNotIn(None, origworkerpids)
        self.assertNotIn(None, finalworkerpids)
        # Finally, check that the worker pids have changed
        self.assertNotEqual(sorted(origworkerpids), sorted(finalworkerpids))
        p.close()
        p.join()

    def test_pool_worker_lifetime_early_close(self):
        # Issue #10332: closing a pool whose workers have limited lifetimes
        # before all the tasks completed would make join() hang.
        p = multiprocessing.Pool(3, maxtasksperchild=1)
        results = []
        for i in range(6):
            results.append(p.apply_async(sqr, (i, 0.3)))
        p.close()
        p.join()
        # check the results
        for (j, res) in enumerate(results):
            self.assertEqual(res.get(), sqr(j))

    def test_pool_maxtasksperchild_invalid(self):
        for value in [0, -1, 0.5, "12"]:
            with self.assertRaises(ValueError):
                multiprocessing.Pool(3, maxtasksperchild=value)

    def test_worker_finalization_via_atexit_handler_of_multiprocessing(self):
        # tests cases against bpo-38744 and bpo-39360
        cmd = '''if 1:
            from multiprocessing import Pool
            problem = None
            class A:
                def __init__(self):
                    self.pool = Pool(processes=1)
            def test():
                global problem
                problem = A()
                problem.pool.map(float, tuple(range(10)))
            if __name__ == "__main__":
                test()
        '''
        rc, out, err = test.support.script_helper.assert_python_ok('-c', cmd)
        self.assertEqual(rc, 0)

#
# Test of creating a customized manager class
#

from multiprocessing.managers import BaseManager, BaseProxy, RemoteError

class FooBar(object):
    def f(self):
        return 'f()'
    def g(self):
        raise ValueError
    def _h(self):
        return '_h()'

def baz():
    for i in range(10):
        yield i*i

class IteratorProxy(BaseProxy):
    _exposed_ = ('__next__',)
    def __iter__(self):
        return self
    def __next__(self):
        return self._callmethod('__next__')

class MyManager(BaseManager):
    pass

MyManager.register('Foo', callable=FooBar)
MyManager.register('Bar', callable=FooBar, exposed=('f', '_h'))
MyManager.register('baz', callable=baz, proxytype=IteratorProxy)


class _TestMyManager(BaseTestCase):

    ALLOWED_TYPES = ('manager',)

    def test_mymanager(self):
        manager = MyManager(shutdown_timeout=SHUTDOWN_TIMEOUT)
        manager.start()
        self.common(manager)
        manager.shutdown()

        # bpo-30356: BaseManager._finalize_manager() sends SIGTERM
        # to the manager process if it takes longer than 1 second to stop,
        # which happens on slow buildbots.
        self.assertIn(manager._process.exitcode, (0, -signal.SIGTERM))

    def test_mymanager_context(self):
        manager = MyManager(shutdown_timeout=SHUTDOWN_TIMEOUT)
        with manager:
            self.common(manager)
        # bpo-30356: BaseManager._finalize_manager() sends SIGTERM
        # to the manager process if it takes longer than 1 second to stop,
        # which happens on slow buildbots.
        self.assertIn(manager._process.exitcode, (0, -signal.SIGTERM))

    def test_mymanager_context_prestarted(self):
        manager = MyManager(shutdown_timeout=SHUTDOWN_TIMEOUT)
        manager.start()
        with manager:
            self.common(manager)
        self.assertEqual(manager._process.exitcode, 0)

    def common(self, manager):
        foo = manager.Foo()
        bar = manager.Bar()
        baz = manager.baz()

        foo_methods = [name for name in ('f', 'g', '_h') if hasattr(foo, name)]
        bar_methods = [name for name in ('f', 'g', '_h') if hasattr(bar, name)]

        self.assertEqual(foo_methods, ['f', 'g'])
        self.assertEqual(bar_methods, ['f', '_h'])

        self.assertEqual(foo.f(), 'f()')
        self.assertRaises(ValueError, foo.g)
        self.assertEqual(foo._callmethod('f'), 'f()')
        self.assertRaises(RemoteError, foo._callmethod, '_h')

        self.assertEqual(bar.f(), 'f()')
        self.assertEqual(bar._h(), '_h()')
        self.assertEqual(bar._callmethod('f'), 'f()')
        self.assertEqual(bar._callmethod('_h'), '_h()')

        self.assertEqual(list(baz), [i*i for i in range(10)])


#
# Test of connecting to a remote server and using xmlrpclib for serialization
#

_queue = pyqueue.Queue()
def get_queue():
    return _queue

class QueueManager(BaseManager):
    '''manager class used by server process'''
QueueManager.register('get_queue', callable=get_queue)

class QueueManager2(BaseManager):
    '''manager class which specifies the same interface as QueueManager'''
QueueManager2.register('get_queue')


SERIALIZER = 'xmlrpclib'

class _TestRemoteManager(BaseTestCase):

    ALLOWED_TYPES = ('manager',)
    values = ['hello world', None, True, 2.25,
              'hall\xe5 v\xe4rlden',
              '\u043f\u0440\u0438\u0432\u0456\u0442 \u0441\u0432\u0456\u0442',
              b'hall\xe5 v\xe4rlden',
             ]
    result = values[:]

    @classmethod
    def _putter(cls, address, authkey):
        manager = QueueManager2(
            address=address, authkey=authkey, serializer=SERIALIZER,
            shutdown_timeout=SHUTDOWN_TIMEOUT)
        manager.connect()
        queue = manager.get_queue()
        # Note that xmlrpclib will deserialize object as a list not a tuple
        queue.put(tuple(cls.values))

    def test_remote(self):
        authkey = os.urandom(32)

        manager = QueueManager(
            address=(socket_helper.HOST, 0), authkey=authkey, serializer=SERIALIZER,
            shutdown_timeout=SHUTDOWN_TIMEOUT)
        manager.start()
        self.addCleanup(manager.shutdown)

        p = self.Process(target=self._putter, args=(manager.address, authkey))
        p.daemon = True
        p.start()

        manager2 = QueueManager2(
            address=manager.address, authkey=authkey, serializer=SERIALIZER,
            shutdown_timeout=SHUTDOWN_TIMEOUT)
        manager2.connect()
        queue = manager2.get_queue()

        self.assertEqual(queue.get(), self.result)

        # Because we are using xmlrpclib for serialization instead of
        # pickle this will cause a serialization error.
        self.assertRaises(Exception, queue.put, time.sleep)

        # Make queue finalizer run before the server is stopped
        del queue


@hashlib_helper.requires_hashdigest('sha256')
class _TestManagerRestart(BaseTestCase):

    @classmethod
    def _putter(cls, address, authkey):
        manager = QueueManager(
            address=address, authkey=authkey, serializer=SERIALIZER,
            shutdown_timeout=SHUTDOWN_TIMEOUT)
        manager.connect()
        queue = manager.get_queue()
        queue.put('hello world')

    def test_rapid_restart(self):
        authkey = os.urandom(32)
        manager = QueueManager(
            address=(socket_helper.HOST, 0), authkey=authkey,
            serializer=SERIALIZER, shutdown_timeout=SHUTDOWN_TIMEOUT)
        try:
            srvr = manager.get_server()
            addr = srvr.address
            # Close the connection.Listener socket which gets opened as a part
            # of manager.get_server(). It's not needed for the test.
            srvr.listener.close()
            manager.start()

            p = self.Process(target=self._putter, args=(manager.address, authkey))
            p.start()
            p.join()
            queue = manager.get_queue()
            self.assertEqual(queue.get(), 'hello world')
            del queue
        finally:
            if hasattr(manager, "shutdown"):
                manager.shutdown()

        manager = QueueManager(
            address=addr, authkey=authkey, serializer=SERIALIZER,
            shutdown_timeout=SHUTDOWN_TIMEOUT)
        try:
            manager.start()
            self.addCleanup(manager.shutdown)
        except OSError as e:
            if e.errno != errno.EADDRINUSE:
                raise
            # Retry after some time, in case the old socket was lingering
            # (sporadic failure on buildbots)
            time.sleep(1.0)
            manager = QueueManager(
                address=addr, authkey=authkey, serializer=SERIALIZER,
                shutdown_timeout=SHUTDOWN_TIMEOUT)
            if hasattr(manager, "shutdown"):
                self.addCleanup(manager.shutdown)


class FakeConnection:
    def send(self, payload):
        pass

    def recv(self):
        return '#ERROR', pyqueue.Empty()

class TestManagerExceptions(unittest.TestCase):
    # Issue 106558: Manager exceptions avoids creating cyclic references.
    def setUp(self):
        self.mgr = multiprocessing.Manager()

    def tearDown(self):
        self.mgr.shutdown()
        self.mgr.join()

    def test_queue_get(self):
        queue = self.mgr.Queue()
        if gc.isenabled():
            gc.disable()
            self.addCleanup(gc.enable)
        try:
            queue.get_nowait()
        except pyqueue.Empty as e:
            wr = weakref.ref(e)
        self.assertEqual(wr(), None)

    def test_dispatch(self):
        if gc.isenabled():
            gc.disable()
            self.addCleanup(gc.enable)
        try:
            multiprocessing.managers.dispatch(FakeConnection(), None, None)
        except pyqueue.Empty as e:
            wr = weakref.ref(e)
        self.assertEqual(wr(), None)

#
#
#

SENTINEL = latin('')

class _TestConnection(BaseTestCase):

    ALLOWED_TYPES = ('processes', 'threads')

    @classmethod
    def _echo(cls, conn):
        for msg in iter(conn.recv_bytes, SENTINEL):
            conn.send_bytes(msg)
        conn.close()

    def test_connection(self):
        conn, child_conn = self.Pipe()

        p = self.Process(target=self._echo, args=(child_conn,))
        p.daemon = True
        p.start()

        seq = [1, 2.25, None]
        msg = latin('hello world')
        longmsg = msg * 10
        arr = array.array('i', list(range(4)))

        if self.TYPE == 'processes':
            self.assertEqual(type(conn.fileno()), int)

        self.assertEqual(conn.send(seq), None)
        self.assertEqual(conn.recv(), seq)

        self.assertEqual(conn.send_bytes(msg), None)
        self.assertEqual(conn.recv_bytes(), msg)

        if self.TYPE == 'processes':
            buffer = array.array('i', [0]*10)
            expected = list(arr) + [0] * (10 - len(arr))
            self.assertEqual(conn.send_bytes(arr), None)
            self.assertEqual(conn.recv_bytes_into(buffer),
                             len(arr) * buffer.itemsize)
            self.assertEqual(list(buffer), expected)

            buffer = array.array('i', [0]*10)
            expected = [0] * 3 + list(arr) + [0] * (10 - 3 - len(arr))
            self.assertEqual(conn.send_bytes(arr), None)
            self.assertEqual(conn.recv_bytes_into(buffer, 3 * buffer.itemsize),
                             len(arr) * buffer.itemsize)
            self.assertEqual(list(buffer), expected)

            buffer = bytearray(latin(' ' * 40))
            self.assertEqual(conn.send_bytes(longmsg), None)
            try:
                res = conn.recv_bytes_into(buffer)
            except multiprocessing.BufferTooShort as e:
                self.assertEqual(e.args, (longmsg,))
            else:
                self.fail('expected BufferTooShort, got %s' % res)

        poll = TimingWrapper(conn.poll)

        self.assertEqual(poll(), False)
        self.assertTimingAlmostEqual(poll.elapsed, 0)

        self.assertEqual(poll(-1), False)
        self.assertTimingAlmostEqual(poll.elapsed, 0)

        self.assertEqual(poll(TIMEOUT1), False)
        self.assertTimingAlmostEqual(poll.elapsed, TIMEOUT1)

        conn.send(None)
        time.sleep(.1)

        self.assertEqual(poll(TIMEOUT1), True)
        self.assertTimingAlmostEqual(poll.elapsed, 0)

        self.assertEqual(conn.recv(), None)

        really_big_msg = latin('X') * (1024 * 1024 * 16)   # 16Mb
        conn.send_bytes(really_big_msg)
        self.assertEqual(conn.recv_bytes(), really_big_msg)

        conn.send_bytes(SENTINEL)                          # tell child to quit
        child_conn.close()

        if self.TYPE == 'processes':
            self.assertEqual(conn.readable, True)
            self.assertEqual(conn.writable, True)
            self.assertRaises(EOFError, conn.recv)
            self.assertRaises(EOFError, conn.recv_bytes)

        p.join()

    def test_duplex_false(self):
        reader, writer = self.Pipe(duplex=False)
        self.assertEqual(writer.send(1), None)
        self.assertEqual(reader.recv(), 1)
        if self.TYPE == 'processes':
            self.assertEqual(reader.readable, True)
            self.assertEqual(reader.writable, False)
            self.assertEqual(writer.readable, False)
            self.assertEqual(writer.writable, True)
            self.assertRaises(OSError, reader.send, 2)
            self.assertRaises(OSError, writer.recv)
            self.assertRaises(OSError, writer.poll)

    def test_spawn_close(self):
        # We test that a pipe connection can be closed by parent
        # process immediately after child is spawned.  On Windows this
        # would have sometimes failed on old versions because
        # child_conn would be closed before the child got a chance to
        # duplicate it.
        conn, child_conn = self.Pipe()

        p = self.Process(target=self._echo, args=(child_conn,))
        p.daemon = True
        p.start()
        child_conn.close()    # this might complete before child initializes

        msg = latin('hello')
        conn.send_bytes(msg)
        self.assertEqual(conn.recv_bytes(), msg)

        conn.send_bytes(SENTINEL)
        conn.close()
        p.join()

    def test_sendbytes(self):
        if self.TYPE != 'processes':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        msg = latin('abcdefghijklmnopqrstuvwxyz')
        a, b = self.Pipe()

        a.send_bytes(msg)
        self.assertEqual(b.recv_bytes(), msg)

        a.send_bytes(msg, 5)
        self.assertEqual(b.recv_bytes(), msg[5:])

        a.send_bytes(msg, 7, 8)
        self.assertEqual(b.recv_bytes(), msg[7:7+8])

        a.send_bytes(msg, 26)
        self.assertEqual(b.recv_bytes(), latin(''))

        a.send_bytes(msg, 26, 0)
        self.assertEqual(b.recv_bytes(), latin(''))

        self.assertRaises(ValueError, a.send_bytes, msg, 27)

        self.assertRaises(ValueError, a.send_bytes, msg, 22, 5)

        self.assertRaises(ValueError, a.send_bytes, msg, 26, 1)

        self.assertRaises(ValueError, a.send_bytes, msg, -1)

        self.assertRaises(ValueError, a.send_bytes, msg, 4, -1)

    @classmethod
    def _is_fd_assigned(cls, fd):
        try:
            os.fstat(fd)
        except OSError as e:
            if e.errno == errno.EBADF:
                return False
            raise
        else:
            return True

    @classmethod
    def _writefd(cls, conn, data, create_dummy_fds=False):
        if create_dummy_fds:
            for i in range(0, 256):
                if not cls._is_fd_assigned(i):
                    os.dup2(conn.fileno(), i)
        fd = reduction.recv_handle(conn)
        if msvcrt:
            fd = msvcrt.open_osfhandle(fd, os.O_WRONLY)
        os.write(fd, data)
        os.close(fd)

    @unittest.skipUnless(HAS_REDUCTION, "test needs multiprocessing.reduction")
    def test_fd_transfer(self):
        if self.TYPE != 'processes':
            self.skipTest("only makes sense with processes")
        conn, child_conn = self.Pipe(duplex=True)

        p = self.Process(target=self._writefd, args=(child_conn, b"foo"))
        p.daemon = True
        p.start()
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with open(os_helper.TESTFN, "wb") as f:
            fd = f.fileno()
            if msvcrt:
                fd = msvcrt.get_osfhandle(fd)
            reduction.send_handle(conn, fd, p.pid)
        p.join()
        with open(os_helper.TESTFN, "rb") as f:
            self.assertEqual(f.read(), b"foo")

    @unittest.skipUnless(HAS_REDUCTION, "test needs multiprocessing.reduction")
    @unittest.skipIf(sys.platform == "win32",
                     "test semantics don't make sense on Windows")
    @unittest.skipIf(MAXFD <= 256,
                     "largest assignable fd number is too small")
    @unittest.skipUnless(hasattr(os, "dup2"),
                         "test needs os.dup2()")
    def test_large_fd_transfer(self):
        # With fd > 256 (issue #11657)
        if self.TYPE != 'processes':
            self.skipTest("only makes sense with processes")
        conn, child_conn = self.Pipe(duplex=True)

        p = self.Process(target=self._writefd, args=(child_conn, b"bar", True))
        p.daemon = True
        p.start()
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with open(os_helper.TESTFN, "wb") as f:
            fd = f.fileno()
            for newfd in range(256, MAXFD):
                if not self._is_fd_assigned(newfd):
                    break
            else:
                self.fail("could not find an unassigned large file descriptor")
            os.dup2(fd, newfd)
            try:
                reduction.send_handle(conn, newfd, p.pid)
            finally:
                os.close(newfd)
        p.join()
        with open(os_helper.TESTFN, "rb") as f:
            self.assertEqual(f.read(), b"bar")

    @classmethod
    def _send_data_without_fd(self, conn):
        os.write(conn.fileno(), b"\0")

    @unittest.skipUnless(HAS_REDUCTION, "test needs multiprocessing.reduction")
    @unittest.skipIf(sys.platform == "win32", "doesn't make sense on Windows")
    def test_missing_fd_transfer(self):
        # Check that exception is raised when received data is not
        # accompanied by a file descriptor in ancillary data.
        if self.TYPE != 'processes':
            self.skipTest("only makes sense with processes")
        conn, child_conn = self.Pipe(duplex=True)

        p = self.Process(target=self._send_data_without_fd, args=(child_conn,))
        p.daemon = True
        p.start()
        self.assertRaises(RuntimeError, reduction.recv_handle, conn)
        p.join()

    def test_context(self):
        a, b = self.Pipe()

        with a, b:
            a.send(1729)
            self.assertEqual(b.recv(), 1729)
            if self.TYPE == 'processes':
                self.assertFalse(a.closed)
                self.assertFalse(b.closed)

        if self.TYPE == 'processes':
            self.assertTrue(a.closed)
            self.assertTrue(b.closed)
            self.assertRaises(OSError, a.recv)
            self.assertRaises(OSError, b.recv)

class _TestListener(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def test_multiple_bind(self):
        for family in self.connection.families:
            l = self.connection.Listener(family=family)
            self.addCleanup(l.close)
            self.assertRaises(OSError, self.connection.Listener,
                              l.address, family)

    def test_context(self):
        with self.connection.Listener() as l:
            with self.connection.Client(l.address) as c:
                with l.accept() as d:
                    c.send(1729)
                    self.assertEqual(d.recv(), 1729)

        if self.TYPE == 'processes':
            self.assertRaises(OSError, l.accept)

    def test_empty_authkey(self):
        # bpo-43952: allow empty bytes as authkey
        def handler(*args):
            raise RuntimeError('Connection took too long...')

        def run(addr, authkey):
            client = self.connection.Client(addr, authkey=authkey)
            client.send(1729)

        key = b''

        with self.connection.Listener(authkey=key) as listener:
            thread = threading.Thread(target=run, args=(listener.address, key))
            thread.start()
            try:
                with listener.accept() as d:
                    self.assertEqual(d.recv(), 1729)
            finally:
                thread.join()

        if self.TYPE == 'processes':
            with self.assertRaises(OSError):
                listener.accept()

    @unittest.skipUnless(util.abstract_sockets_supported,
                         "test needs abstract socket support")
    def test_abstract_socket(self):
        with self.connection.Listener("\0something") as listener:
            with self.connection.Client(listener.address) as client:
                with listener.accept() as d:
                    client.send(1729)
                    self.assertEqual(d.recv(), 1729)

        if self.TYPE == 'processes':
            self.assertRaises(OSError, listener.accept)


class _TestListenerClient(BaseTestCase):

    ALLOWED_TYPES = ('processes', 'threads')

    @classmethod
    def _test(cls, address):
        conn = cls.connection.Client(address)
        conn.send('hello')
        conn.close()

    def test_listener_client(self):
        for family in self.connection.families:
            l = self.connection.Listener(family=family)
            p = self.Process(target=self._test, args=(l.address,))
            p.daemon = True
            p.start()
            conn = l.accept()
            self.assertEqual(conn.recv(), 'hello')
            p.join()
            l.close()

    def test_issue14725(self):
        l = self.connection.Listener()
        p = self.Process(target=self._test, args=(l.address,))
        p.daemon = True
        p.start()
        time.sleep(1)
        # On Windows the client process should by now have connected,
        # written data and closed the pipe handle by now.  This causes
        # ConnectNamdedPipe() to fail with ERROR_NO_DATA.  See Issue
        # 14725.
        conn = l.accept()
        self.assertEqual(conn.recv(), 'hello')
        conn.close()
        p.join()
        l.close()

    def test_issue16955(self):
        for fam in self.connection.families:
            l = self.connection.Listener(family=fam)
            c = self.connection.Client(l.address)
            a = l.accept()
            a.send_bytes(b"hello")
            self.assertTrue(c.poll(1))
            a.close()
            c.close()
            l.close()

class _TestPoll(BaseTestCase):

    ALLOWED_TYPES = ('processes', 'threads')

    def test_empty_string(self):
        a, b = self.Pipe()
        self.assertEqual(a.poll(), False)
        b.send_bytes(b'')
        self.assertEqual(a.poll(), True)
        self.assertEqual(a.poll(), True)

    @classmethod
    def _child_strings(cls, conn, strings):
        for s in strings:
            time.sleep(0.1)
            conn.send_bytes(s)
        conn.close()

    def test_strings(self):
        strings = (b'hello', b'', b'a', b'b', b'', b'bye', b'', b'lop')
        a, b = self.Pipe()
        p = self.Process(target=self._child_strings, args=(b, strings))
        p.start()

        for s in strings:
            for i in range(200):
                if a.poll(0.01):
                    break
            x = a.recv_bytes()
            self.assertEqual(s, x)

        p.join()

    @classmethod
    def _child_boundaries(cls, r):
        # Polling may "pull" a message in to the child process, but we
        # don't want it to pull only part of a message, as that would
        # corrupt the pipe for any other processes which might later
        # read from it.
        r.poll(5)

    def test_boundaries(self):
        r, w = self.Pipe(False)
        p = self.Process(target=self._child_boundaries, args=(r,))
        p.start()
        time.sleep(2)
        L = [b"first", b"second"]
        for obj in L:
            w.send_bytes(obj)
        w.close()
        p.join()
        self.assertIn(r.recv_bytes(), L)

    @classmethod
    def _child_dont_merge(cls, b):
        b.send_bytes(b'a')
        b.send_bytes(b'b')
        b.send_bytes(b'cd')

    def test_dont_merge(self):
        a, b = self.Pipe()
        self.assertEqual(a.poll(0.0), False)
        self.assertEqual(a.poll(0.1), False)

        p = self.Process(target=self._child_dont_merge, args=(b,))
        p.start()

        self.assertEqual(a.recv_bytes(), b'a')
        self.assertEqual(a.poll(1.0), True)
        self.assertEqual(a.poll(1.0), True)
        self.assertEqual(a.recv_bytes(), b'b')
        self.assertEqual(a.poll(1.0), True)
        self.assertEqual(a.poll(1.0), True)
        self.assertEqual(a.poll(0.0), True)
        self.assertEqual(a.recv_bytes(), b'cd')

        p.join()

#
# Test of sending connection and socket objects between processes
#

@unittest.skipUnless(HAS_REDUCTION, "test needs multiprocessing.reduction")
@hashlib_helper.requires_hashdigest('sha256')
class _TestPicklingConnections(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    @classmethod
    def tearDownClass(cls):
        from multiprocessing import resource_sharer
        resource_sharer.stop(timeout=support.LONG_TIMEOUT)

    @classmethod
    def _listener(cls, conn, families):
        for fam in families:
            l = cls.connection.Listener(family=fam)
            conn.send(l.address)
            new_conn = l.accept()
            conn.send(new_conn)
            new_conn.close()
            l.close()

        l = socket.create_server((socket_helper.HOST, 0))
        conn.send(l.getsockname())
        new_conn, addr = l.accept()
        conn.send(new_conn)
        new_conn.close()
        l.close()

        conn.recv()

    @classmethod
    def _remote(cls, conn):
        for (address, msg) in iter(conn.recv, None):
            client = cls.connection.Client(address)
            client.send(msg.upper())
            client.close()

        address, msg = conn.recv()
        client = socket.socket()
        client.connect(address)
        client.sendall(msg.upper())
        client.close()

        conn.close()

    def test_pickling(self):
        families = self.connection.families

        lconn, lconn0 = self.Pipe()
        lp = self.Process(target=self._listener, args=(lconn0, families))
        lp.daemon = True
        lp.start()
        lconn0.close()

        rconn, rconn0 = self.Pipe()
        rp = self.Process(target=self._remote, args=(rconn0,))
        rp.daemon = True
        rp.start()
        rconn0.close()

        for fam in families:
            msg = ('This connection uses family %s' % fam).encode('ascii')
            address = lconn.recv()
            rconn.send((address, msg))
            new_conn = lconn.recv()
            self.assertEqual(new_conn.recv(), msg.upper())

        rconn.send(None)

        msg = latin('This connection uses a normal socket')
        address = lconn.recv()
        rconn.send((address, msg))
        new_conn = lconn.recv()
        buf = []
        while True:
            s = new_conn.recv(100)
            if not s:
                break
            buf.append(s)
        buf = b''.join(buf)
        self.assertEqual(buf, msg.upper())
        new_conn.close()

        lconn.send(None)

        rconn.close()
        lconn.close()

        lp.join()
        rp.join()

    @classmethod
    def child_access(cls, conn):
        w = conn.recv()
        w.send('all is well')
        w.close()

        r = conn.recv()
        msg = r.recv()
        conn.send(msg*2)

        conn.close()

    def test_access(self):
        # On Windows, if we do not specify a destination pid when
        # using DupHandle then we need to be careful to use the
        # correct access flags for DuplicateHandle(), or else
        # DupHandle.detach() will raise PermissionError.  For example,
        # for a read only pipe handle we should use
        # access=FILE_GENERIC_READ.  (Unfortunately
        # DUPLICATE_SAME_ACCESS does not work.)
        conn, child_conn = self.Pipe()
        p = self.Process(target=self.child_access, args=(child_conn,))
        p.daemon = True
        p.start()
        child_conn.close()

        r, w = self.Pipe(duplex=False)
        conn.send(w)
        w.close()
        self.assertEqual(r.recv(), 'all is well')
        r.close()

        r, w = self.Pipe(duplex=False)
        conn.send(r)
        r.close()
        w.send('foobar')
        w.close()
        self.assertEqual(conn.recv(), 'foobar'*2)

        p.join()

#
#
#

class _TestHeap(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def setUp(self):
        super().setUp()
        # Make pristine heap for these tests
        self.old_heap = multiprocessing.heap.BufferWrapper._heap
        multiprocessing.heap.BufferWrapper._heap = multiprocessing.heap.Heap()

    def tearDown(self):
        multiprocessing.heap.BufferWrapper._heap = self.old_heap
        super().tearDown()

    def test_heap(self):
        iterations = 5000
        maxblocks = 50
        blocks = []

        # get the heap object
        heap = multiprocessing.heap.BufferWrapper._heap
        heap._DISCARD_FREE_SPACE_LARGER_THAN = 0

        # create and destroy lots of blocks of different sizes
        for i in range(iterations):
            size = int(random.lognormvariate(0, 1) * 1000)
            b = multiprocessing.heap.BufferWrapper(size)
            blocks.append(b)
            if len(blocks) > maxblocks:
                i = random.randrange(maxblocks)
                del blocks[i]
            del b

        # verify the state of the heap
        with heap._lock:
            all = []
            free = 0
            occupied = 0
            for L in list(heap._len_to_seq.values()):
                # count all free blocks in arenas
                for arena, start, stop in L:
                    all.append((heap._arenas.index(arena), start, stop,
                                stop-start, 'free'))
                    free += (stop-start)
            for arena, arena_blocks in heap._allocated_blocks.items():
                # count all allocated blocks in arenas
                for start, stop in arena_blocks:
                    all.append((heap._arenas.index(arena), start, stop,
                                stop-start, 'occupied'))
                    occupied += (stop-start)

            self.assertEqual(free + occupied,
                             sum(arena.size for arena in heap._arenas))

            all.sort()

            for i in range(len(all)-1):
                (arena, start, stop) = all[i][:3]
                (narena, nstart, nstop) = all[i+1][:3]
                if arena != narena:
                    # Two different arenas
                    self.assertEqual(stop, heap._arenas[arena].size)  # last block
                    self.assertEqual(nstart, 0)         # first block
                else:
                    # Same arena: two adjacent blocks
                    self.assertEqual(stop, nstart)

        # test free'ing all blocks
        random.shuffle(blocks)
        while blocks:
            blocks.pop()

        self.assertEqual(heap._n_frees, heap._n_mallocs)
        self.assertEqual(len(heap._pending_free_blocks), 0)
        self.assertEqual(len(heap._arenas), 0)
        self.assertEqual(len(heap._allocated_blocks), 0, heap._allocated_blocks)
        self.assertEqual(len(heap._len_to_seq), 0)

    def test_free_from_gc(self):
        # Check that freeing of blocks by the garbage collector doesn't deadlock
        # (issue #12352).
        # Make sure the GC is enabled, and set lower collection thresholds to
        # make collections more frequent (and increase the probability of
        # deadlock).
        if not gc.isenabled():
            gc.enable()
            self.addCleanup(gc.disable)
        thresholds = gc.get_threshold()
        self.addCleanup(gc.set_threshold, *thresholds)
        gc.set_threshold(10)

        # perform numerous block allocations, with cyclic references to make
        # sure objects are collected asynchronously by the gc
        for i in range(5000):
            a = multiprocessing.heap.BufferWrapper(1)
            b = multiprocessing.heap.BufferWrapper(1)
            # circular references
            a.buddy = b
            b.buddy = a

#
#
#

class _Foo(Structure):
    _fields_ = [
        ('x', c_int),
        ('y', c_double),
        ('z', c_longlong,)
        ]

class _TestSharedCTypes(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def setUp(self):
        if not HAS_SHAREDCTYPES:
            self.skipTest("requires multiprocessing.sharedctypes")

    @classmethod
    def _double(cls, x, y, z, foo, arr, string):
        x.value *= 2
        y.value *= 2
        z.value *= 2
        foo.x *= 2
        foo.y *= 2
        string.value *= 2
        for i in range(len(arr)):
            arr[i] *= 2

    def test_sharedctypes(self, lock=False):
        x = Value('i', 7, lock=lock)
        y = Value(c_double, 1.0/3.0, lock=lock)
        z = Value(c_longlong, 2 ** 33, lock=lock)
        foo = Value(_Foo, 3, 2, lock=lock)
        arr = self.Array('d', list(range(10)), lock=lock)
        string = self.Array('c', 20, lock=lock)
        string.value = latin('hello')

        p = self.Process(target=self._double, args=(x, y, z, foo, arr, string))
        p.daemon = True
        p.start()
        p.join()

        self.assertEqual(x.value, 14)
        self.assertAlmostEqual(y.value, 2.0/3.0)
        self.assertEqual(z.value, 2 ** 34)
        self.assertEqual(foo.x, 6)
        self.assertAlmostEqual(foo.y, 4.0)
        for i in range(10):
            self.assertAlmostEqual(arr[i], i*2)
        self.assertEqual(string.value, latin('hellohello'))

    def test_synchronize(self):
        self.test_sharedctypes(lock=True)

    def test_copy(self):
        foo = _Foo(2, 5.0, 2 ** 33)
        bar = copy(foo)
        foo.x = 0
        foo.y = 0
        foo.z = 0
        self.assertEqual(bar.x, 2)
        self.assertAlmostEqual(bar.y, 5.0)
        self.assertEqual(bar.z, 2 ** 33)


@unittest.skipUnless(HAS_SHMEM, "requires multiprocessing.shared_memory")
@hashlib_helper.requires_hashdigest('sha256')
class _TestSharedMemory(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    @staticmethod
    def _attach_existing_shmem_then_write(shmem_name_or_obj, binary_data):
        if isinstance(shmem_name_or_obj, str):
            local_sms = shared_memory.SharedMemory(shmem_name_or_obj)
        else:
            local_sms = shmem_name_or_obj
        local_sms.buf[:len(binary_data)] = binary_data
        local_sms.close()

    def _new_shm_name(self, prefix):
        # Add a PID to the name of a POSIX shared memory object to allow
        # running multiprocessing tests (test_multiprocessing_fork,
        # test_multiprocessing_spawn, etc) in parallel.
        return prefix + str(os.getpid())

    def test_shared_memory_name_with_embedded_null(self):
        name_tsmb = self._new_shm_name('test01_null')
        sms = shared_memory.SharedMemory(name_tsmb, create=True, size=512)
        self.addCleanup(sms.unlink)
        with self.assertRaises(ValueError):
            shared_memory.SharedMemory(name_tsmb + '\0a', create=False, size=512)
        if shared_memory._USE_POSIX:
            orig_name = sms._name
            try:
                sms._name = orig_name + '\0a'
                with self.assertRaises(ValueError):
                    sms.unlink()
            finally:
                sms._name = orig_name

    def test_shared_memory_basics(self):
        name_tsmb = self._new_shm_name('test01_tsmb')
        sms = shared_memory.SharedMemory(name_tsmb, create=True, size=512)
        self.addCleanup(sms.unlink)

        # Verify attributes are readable.
        self.assertEqual(sms.name, name_tsmb)
        self.assertGreaterEqual(sms.size, 512)
        self.assertGreaterEqual(len(sms.buf), sms.size)

        # Verify __repr__
        self.assertIn(sms.name, str(sms))
        self.assertIn(str(sms.size), str(sms))

        # Modify contents of shared memory segment through memoryview.
        sms.buf[0] = 42
        self.assertEqual(sms.buf[0], 42)

        # Attach to existing shared memory segment.
        also_sms = shared_memory.SharedMemory(name_tsmb)
        self.assertEqual(also_sms.buf[0], 42)
        also_sms.close()

        # Attach to existing shared memory segment but specify a new size.
        same_sms = shared_memory.SharedMemory(name_tsmb, size=20*sms.size)
        self.assertLess(same_sms.size, 20*sms.size)  # Size was ignored.
        same_sms.close()

        # Creating Shared Memory Segment with -ve size
        with self.assertRaises(ValueError):
            shared_memory.SharedMemory(create=True, size=-2)

        # Attaching Shared Memory Segment without a name
        with self.assertRaises(ValueError):
            shared_memory.SharedMemory(create=False)

        # Test if shared memory segment is created properly,
        # when _make_filename returns an existing shared memory segment name
        with unittest.mock.patch(
            'multiprocessing.shared_memory._make_filename') as mock_make_filename:

            NAME_PREFIX = shared_memory._SHM_NAME_PREFIX
            names = [self._new_shm_name('test01_fn'), self._new_shm_name('test02_fn')]
            # Prepend NAME_PREFIX which can be '/psm_' or 'wnsm_', necessary
            # because some POSIX compliant systems require name to start with /
            names = [NAME_PREFIX + name for name in names]

            mock_make_filename.side_effect = names
            shm1 = shared_memory.SharedMemory(create=True, size=1)
            self.addCleanup(shm1.unlink)
            self.assertEqual(shm1._name, names[0])

            mock_make_filename.side_effect = names
            shm2 = shared_memory.SharedMemory(create=True, size=1)
            self.addCleanup(shm2.unlink)
            self.assertEqual(shm2._name, names[1])

        if shared_memory._USE_POSIX:
            # Posix Shared Memory can only be unlinked once.  Here we
            # test an implementation detail that is not observed across
            # all supported platforms (since WindowsNamedSharedMemory
            # manages unlinking on its own and unlink() does nothing).
            # True release of shared memory segment does not necessarily
            # happen until process exits, depending on the OS platform.
            name_dblunlink = self._new_shm_name('test01_dblunlink')
            sms_uno = shared_memory.SharedMemory(
                name_dblunlink,
                create=True,
                size=5000
            )
            with self.assertRaises(FileNotFoundError):
                try:
                    self.assertGreaterEqual(sms_uno.size, 5000)

                    sms_duo = shared_memory.SharedMemory(name_dblunlink)
                    sms_duo.unlink()  # First shm_unlink() call.
                    sms_duo.close()
                    sms_uno.close()

                finally:
                    sms_uno.unlink()  # A second shm_unlink() call is bad.

        with self.assertRaises(FileExistsError):
            # Attempting to create a new shared memory segment with a
            # name that is already in use triggers an exception.
            there_can_only_be_one_sms = shared_memory.SharedMemory(
                name_tsmb,
                create=True,
                size=512
            )

        if shared_memory._USE_POSIX:
            # Requesting creation of a shared memory segment with the option
            # to attach to an existing segment, if that name is currently in
            # use, should not trigger an exception.
            # Note:  Using a smaller size could possibly cause truncation of
            # the existing segment but is OS platform dependent.  In the
            # case of MacOS/darwin, requesting a smaller size is disallowed.
            class OptionalAttachSharedMemory(shared_memory.SharedMemory):
                _flags = os.O_CREAT | os.O_RDWR
            ok_if_exists_sms = OptionalAttachSharedMemory(name_tsmb)
            self.assertEqual(ok_if_exists_sms.size, sms.size)
            ok_if_exists_sms.close()

        # Attempting to attach to an existing shared memory segment when
        # no segment exists with the supplied name triggers an exception.
        with self.assertRaises(FileNotFoundError):
            nonexisting_sms = shared_memory.SharedMemory('test01_notthere')
            nonexisting_sms.unlink()  # Error should occur on prior line.

        sms.close()

    def test_shared_memory_recreate(self):
        # Test if shared memory segment is created properly,
        # when _make_filename returns an existing shared memory segment name
        with unittest.mock.patch(
            'multiprocessing.shared_memory._make_filename') as mock_make_filename:

            NAME_PREFIX = shared_memory._SHM_NAME_PREFIX
            names = [self._new_shm_name('test03_fn'), self._new_shm_name('test04_fn')]
            # Prepend NAME_PREFIX which can be '/psm_' or 'wnsm_', necessary
            # because some POSIX compliant systems require name to start with /
            names = [NAME_PREFIX + name for name in names]

            mock_make_filename.side_effect = names
            shm1 = shared_memory.SharedMemory(create=True, size=1)
            self.addCleanup(shm1.unlink)
            self.assertEqual(shm1._name, names[0])

            mock_make_filename.side_effect = names
            shm2 = shared_memory.SharedMemory(create=True, size=1)
            self.addCleanup(shm2.unlink)
            self.assertEqual(shm2._name, names[1])

    def test_invalid_shared_memory_creation(self):
        # Test creating a shared memory segment with negative size
        with self.assertRaises(ValueError):
            sms_invalid = shared_memory.SharedMemory(create=True, size=-1)

        # Test creating a shared memory segment with size 0
        with self.assertRaises(ValueError):
            sms_invalid = shared_memory.SharedMemory(create=True, size=0)

        # Test creating a shared memory segment without size argument
        with self.assertRaises(ValueError):
            sms_invalid = shared_memory.SharedMemory(create=True)

    def test_shared_memory_pickle_unpickle(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                sms = shared_memory.SharedMemory(create=True, size=512)
                self.addCleanup(sms.unlink)
                sms.buf[0:6] = b'pickle'

                # Test pickling
                pickled_sms = pickle.dumps(sms, protocol=proto)

                # Test unpickling
                sms2 = pickle.loads(pickled_sms)
                self.assertIsInstance(sms2, shared_memory.SharedMemory)
                self.assertEqual(sms.name, sms2.name)
                self.assertEqual(bytes(sms.buf[0:6]), b'pickle')
                self.assertEqual(bytes(sms2.buf[0:6]), b'pickle')

                # Test that unpickled version is still the same SharedMemory
                sms.buf[0:6] = b'newval'
                self.assertEqual(bytes(sms.buf[0:6]), b'newval')
                self.assertEqual(bytes(sms2.buf[0:6]), b'newval')

                sms2.buf[0:6] = b'oldval'
                self.assertEqual(bytes(sms.buf[0:6]), b'oldval')
                self.assertEqual(bytes(sms2.buf[0:6]), b'oldval')

    def test_shared_memory_pickle_unpickle_dead_object(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                sms = shared_memory.SharedMemory(create=True, size=512)
                sms.buf[0:6] = b'pickle'
                pickled_sms = pickle.dumps(sms, protocol=proto)

                # Now, we are going to kill the original object.
                # So, unpickled one won't be able to attach to it.
                sms.close()
                sms.unlink()

                with self.assertRaises(FileNotFoundError):
                    pickle.loads(pickled_sms)

    def test_shared_memory_across_processes(self):
        # bpo-40135: don't define shared memory block's name in case of
        # the failure when we run multiprocessing tests in parallel.
        sms = shared_memory.SharedMemory(create=True, size=512)
        self.addCleanup(sms.unlink)

        # Verify remote attachment to existing block by name is working.
        p = self.Process(
            target=self._attach_existing_shmem_then_write,
            args=(sms.name, b'howdy')
        )
        p.daemon = True
        p.start()
        p.join()
        self.assertEqual(bytes(sms.buf[:5]), b'howdy')

        # Verify pickling of SharedMemory instance also works.
        p = self.Process(
            target=self._attach_existing_shmem_then_write,
            args=(sms, b'HELLO')
        )
        p.daemon = True
        p.start()
        p.join()
        self.assertEqual(bytes(sms.buf[:5]), b'HELLO')

        sms.close()

    @unittest.skipIf(os.name != "posix", "not feasible in non-posix platforms")
    def test_shared_memory_SharedMemoryServer_ignores_sigint(self):
        # bpo-36368: protect SharedMemoryManager server process from
        # KeyboardInterrupt signals.
        smm = multiprocessing.managers.SharedMemoryManager()
        smm.start()

        # make sure the manager works properly at the beginning
        sl = smm.ShareableList(range(10))

        # the manager's server should ignore KeyboardInterrupt signals, and
        # maintain its connection with the current process, and success when
        # asked to deliver memory segments.
        os.kill(smm._process.pid, signal.SIGINT)

        sl2 = smm.ShareableList(range(10))

        # test that the custom signal handler registered in the Manager does
        # not affect signal handling in the parent process.
        with self.assertRaises(KeyboardInterrupt):
            os.kill(os.getpid(), signal.SIGINT)

        smm.shutdown()

    @unittest.skipIf(os.name != "posix", "resource_tracker is posix only")
    def test_shared_memory_SharedMemoryManager_reuses_resource_tracker(self):
        # bpo-36867: test that a SharedMemoryManager uses the
        # same resource_tracker process as its parent.
        cmd = '''if 1:
            from multiprocessing.managers import SharedMemoryManager


            smm = SharedMemoryManager()
            smm.start()
            sl = smm.ShareableList(range(10))
            smm.shutdown()
        '''
        rc, out, err = test.support.script_helper.assert_python_ok('-c', cmd)

        # Before bpo-36867 was fixed, a SharedMemoryManager not using the same
        # resource_tracker process as its parent would make the parent's
        # tracker complain about sl being leaked even though smm.shutdown()
        # properly released sl.
        self.assertFalse(err)

    def test_shared_memory_SharedMemoryManager_basics(self):
        smm1 = multiprocessing.managers.SharedMemoryManager()
        with self.assertRaises(ValueError):
            smm1.SharedMemory(size=9)  # Fails if SharedMemoryServer not started
        smm1.start()
        lol = [ smm1.ShareableList(range(i)) for i in range(5, 10) ]
        lom = [ smm1.SharedMemory(size=j) for j in range(32, 128, 16) ]
        doppleganger_list0 = shared_memory.ShareableList(name=lol[0].shm.name)
        self.assertEqual(len(doppleganger_list0), 5)
        doppleganger_shm0 = shared_memory.SharedMemory(name=lom[0].name)
        self.assertGreaterEqual(len(doppleganger_shm0.buf), 32)
        held_name = lom[0].name
        smm1.shutdown()
        if sys.platform != "win32":
            # Calls to unlink() have no effect on Windows platform; shared
            # memory will only be released once final process exits.
            with self.assertRaises(FileNotFoundError):
                # No longer there to be attached to again.
                absent_shm = shared_memory.SharedMemory(name=held_name)

        with multiprocessing.managers.SharedMemoryManager() as smm2:
            sl = smm2.ShareableList("howdy")
            shm = smm2.SharedMemory(size=128)
            held_name = sl.shm.name
        if sys.platform != "win32":
            with self.assertRaises(FileNotFoundError):
                # No longer there to be attached to again.
                absent_sl = shared_memory.ShareableList(name=held_name)


    def test_shared_memory_ShareableList_basics(self):
        sl = shared_memory.ShareableList(
            ['howdy', b'HoWdY', -273.154, 100, None, True, 42]
        )
        self.addCleanup(sl.shm.unlink)

        # Verify __repr__
        self.assertIn(sl.shm.name, str(sl))
        self.assertIn(str(list(sl)), str(sl))

        # Index Out of Range (get)
        with self.assertRaises(IndexError):
            sl[7]

        # Index Out of Range (set)
        with self.assertRaises(IndexError):
            sl[7] = 2

        # Assign value without format change (str -> str)
        current_format = sl._get_packing_format(0)
        sl[0] = 'howdy'
        self.assertEqual(current_format, sl._get_packing_format(0))

        # Verify attributes are readable.
        self.assertEqual(sl.format, '8s8sdqxxxxxx?xxxxxxxx?q')

        # Exercise len().
        self.assertEqual(len(sl), 7)

        # Exercise index().
        with warnings.catch_warnings():
            # Suppress BytesWarning when comparing against b'HoWdY'.
            warnings.simplefilter('ignore')
            with self.assertRaises(ValueError):
                sl.index('100')
            self.assertEqual(sl.index(100), 3)

        # Exercise retrieving individual values.
        self.assertEqual(sl[0], 'howdy')
        self.assertEqual(sl[-2], True)

        # Exercise iterability.
        self.assertEqual(
            tuple(sl),
            ('howdy', b'HoWdY', -273.154, 100, None, True, 42)
        )

        # Exercise modifying individual values.
        sl[3] = 42
        self.assertEqual(sl[3], 42)
        sl[4] = 'some'  # Change type at a given position.
        self.assertEqual(sl[4], 'some')
        self.assertEqual(sl.format, '8s8sdq8sxxxxxxx?q')
        with self.assertRaisesRegex(ValueError,
                                    "exceeds available storage"):
            sl[4] = 'far too many'
        self.assertEqual(sl[4], 'some')
        sl[0] = 'encodés'  # Exactly 8 bytes of UTF-8 data
        self.assertEqual(sl[0], 'encodés')
        self.assertEqual(sl[1], b'HoWdY')  # no spillage
        with self.assertRaisesRegex(ValueError,
                                    "exceeds available storage"):
            sl[0] = 'encodées'  # Exactly 9 bytes of UTF-8 data
        self.assertEqual(sl[1], b'HoWdY')
        with self.assertRaisesRegex(ValueError,
                                    "exceeds available storage"):
            sl[1] = b'123456789'
        self.assertEqual(sl[1], b'HoWdY')

        # Exercise count().
        with warnings.catch_warnings():
            # Suppress BytesWarning when comparing against b'HoWdY'.
            warnings.simplefilter('ignore')
            self.assertEqual(sl.count(42), 2)
            self.assertEqual(sl.count(b'HoWdY'), 1)
            self.assertEqual(sl.count(b'adios'), 0)

        # Exercise creating a duplicate.
        name_duplicate = self._new_shm_name('test03_duplicate')
        sl_copy = shared_memory.ShareableList(sl, name=name_duplicate)
        try:
            self.assertNotEqual(sl.shm.name, sl_copy.shm.name)
            self.assertEqual(name_duplicate, sl_copy.shm.name)
            self.assertEqual(list(sl), list(sl_copy))
            self.assertEqual(sl.format, sl_copy.format)
            sl_copy[-1] = 77
            self.assertEqual(sl_copy[-1], 77)
            self.assertNotEqual(sl[-1], 77)
            sl_copy.shm.close()
        finally:
            sl_copy.shm.unlink()

        # Obtain a second handle on the same ShareableList.
        sl_tethered = shared_memory.ShareableList(name=sl.shm.name)
        self.assertEqual(sl.shm.name, sl_tethered.shm.name)
        sl_tethered[-1] = 880
        self.assertEqual(sl[-1], 880)
        sl_tethered.shm.close()

        sl.shm.close()

        # Exercise creating an empty ShareableList.
        empty_sl = shared_memory.ShareableList()
        try:
            self.assertEqual(len(empty_sl), 0)
            self.assertEqual(empty_sl.format, '')
            self.assertEqual(empty_sl.count('any'), 0)
            with self.assertRaises(ValueError):
                empty_sl.index(None)
            empty_sl.shm.close()
        finally:
            empty_sl.shm.unlink()

    def test_shared_memory_ShareableList_pickling(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                sl = shared_memory.ShareableList(range(10))
                self.addCleanup(sl.shm.unlink)

                serialized_sl = pickle.dumps(sl, protocol=proto)
                deserialized_sl = pickle.loads(serialized_sl)
                self.assertIsInstance(
                    deserialized_sl, shared_memory.ShareableList)
                self.assertEqual(deserialized_sl[-1], 9)
                self.assertIsNot(sl, deserialized_sl)

                deserialized_sl[4] = "changed"
                self.assertEqual(sl[4], "changed")
                sl[3] = "newvalue"
                self.assertEqual(deserialized_sl[3], "newvalue")

                larger_sl = shared_memory.ShareableList(range(400))
                self.addCleanup(larger_sl.shm.unlink)
                serialized_larger_sl = pickle.dumps(larger_sl, protocol=proto)
                self.assertEqual(len(serialized_sl), len(serialized_larger_sl))
                larger_sl.shm.close()

                deserialized_sl.shm.close()
                sl.shm.close()

    def test_shared_memory_ShareableList_pickling_dead_object(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                sl = shared_memory.ShareableList(range(10))
                serialized_sl = pickle.dumps(sl, protocol=proto)

                # Now, we are going to kill the original object.
                # So, unpickled one won't be able to attach to it.
                sl.shm.close()
                sl.shm.unlink()

                with self.assertRaises(FileNotFoundError):
                    pickle.loads(serialized_sl)

    def test_shared_memory_cleaned_after_process_termination(self):
        cmd = '''if 1:
            import os, time, sys
            from multiprocessing import shared_memory

            # Create a shared_memory segment, and send the segment name
            sm = shared_memory.SharedMemory(create=True, size=10)
            sys.stdout.write(sm.name + '\\n')
            sys.stdout.flush()
            time.sleep(100)
        '''
        with subprocess.Popen([sys.executable, '-E', '-c', cmd],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE) as p:
            name = p.stdout.readline().strip().decode()

            # killing abruptly processes holding reference to a shared memory
            # segment should not leak the given memory segment.
            p.terminate()
            p.wait()

            err_msg = ("A SharedMemory segment was leaked after "
                       "a process was abruptly terminated")
            for _ in support.sleeping_retry(support.LONG_TIMEOUT, err_msg):
                try:
                    smm = shared_memory.SharedMemory(name, create=False)
                except FileNotFoundError:
                    break

            if os.name == 'posix':
                # Without this line it was raising warnings like:
                #   UserWarning: resource_tracker:
                #   There appear to be 1 leaked shared_memory
                #   objects to clean up at shutdown
                # See: https://bugs.python.org/issue45209
                resource_tracker.unregister(f"/{name}", "shared_memory")

                # A warning was emitted by the subprocess' own
                # resource_tracker (on Windows, shared memory segments
                # are released automatically by the OS).
                err = p.stderr.read().decode()
                self.assertIn(
                    "resource_tracker: There appear to be 1 leaked "
                    "shared_memory objects to clean up at shutdown", err)

    @unittest.skipIf(os.name != "posix", "resource_tracker is posix only")
    def test_shared_memory_untracking(self):
        # gh-82300: When a separate Python process accesses shared memory
        # with track=False, it must not cause the memory to be deleted
        # when terminating.
        cmd = '''if 1:
            import sys
            from multiprocessing.shared_memory import SharedMemory
            mem = SharedMemory(create=False, name=sys.argv[1], track=False)
            mem.close()
        '''
        mem = shared_memory.SharedMemory(create=True, size=10)
        # The resource tracker shares pipes with the subprocess, and so
        # err existing means that the tracker process has terminated now.
        try:
            rc, out, err = script_helper.assert_python_ok("-c", cmd, mem.name)
            self.assertNotIn(b"resource_tracker", err)
            self.assertEqual(rc, 0)
            mem2 = shared_memory.SharedMemory(create=False, name=mem.name)
            mem2.close()
        finally:
            try:
                mem.unlink()
            except OSError:
                pass
            mem.close()

    @unittest.skipIf(os.name != "posix", "resource_tracker is posix only")
    def test_shared_memory_tracking(self):
        # gh-82300: When a separate Python process accesses shared memory
        # with track=True, it must cause the memory to be deleted when
        # terminating.
        cmd = '''if 1:
            import sys
            from multiprocessing.shared_memory import SharedMemory
            mem = SharedMemory(create=False, name=sys.argv[1], track=True)
            mem.close()
        '''
        mem = shared_memory.SharedMemory(create=True, size=10)
        try:
            rc, out, err = script_helper.assert_python_ok("-c", cmd, mem.name)
            self.assertEqual(rc, 0)
            self.assertIn(
                b"resource_tracker: There appear to be 1 leaked "
                b"shared_memory objects to clean up at shutdown", err)
        finally:
            try:
                mem.unlink()
            except OSError:
                pass
            resource_tracker.unregister(mem._name, "shared_memory")
            mem.close()

#
# Test to verify that `Finalize` works.
#

class _TestFinalize(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def setUp(self):
        self.registry_backup = util._finalizer_registry.copy()
        util._finalizer_registry.clear()

    def tearDown(self):
        gc.collect()  # For PyPy or other GCs.
        self.assertFalse(util._finalizer_registry)
        util._finalizer_registry.update(self.registry_backup)

    @classmethod
    def _test_finalize(cls, conn):
        class Foo(object):
            pass

        a = Foo()
        util.Finalize(a, conn.send, args=('a',))
        del a           # triggers callback for a
        gc.collect()  # For PyPy or other GCs.

        b = Foo()
        close_b = util.Finalize(b, conn.send, args=('b',))
        close_b()       # triggers callback for b
        close_b()       # does nothing because callback has already been called
        del b           # does nothing because callback has already been called
        gc.collect()  # For PyPy or other GCs.

        c = Foo()
        util.Finalize(c, conn.send, args=('c',))

        d10 = Foo()
        util.Finalize(d10, conn.send, args=('d10',), exitpriority=1)

        d01 = Foo()
        util.Finalize(d01, conn.send, args=('d01',), exitpriority=0)
        d02 = Foo()
        util.Finalize(d02, conn.send, args=('d02',), exitpriority=0)
        d03 = Foo()
        util.Finalize(d03, conn.send, args=('d03',), exitpriority=0)

        util.Finalize(None, conn.send, args=('e',), exitpriority=-10)

        util.Finalize(None, conn.send, args=('STOP',), exitpriority=-100)

        # call multiprocessing's cleanup function then exit process without
        # garbage collecting locals
        util._exit_function()
        conn.close()
        os._exit(0)

    def test_finalize(self):
        conn, child_conn = self.Pipe()

        p = self.Process(target=self._test_finalize, args=(child_conn,))
        p.daemon = True
        p.start()
        p.join()

        result = [obj for obj in iter(conn.recv, 'STOP')]
        self.assertEqual(result, ['a', 'b', 'd10', 'd03', 'd02', 'd01', 'e'])

    @support.requires_resource('cpu')
    def test_thread_safety(self):
        # bpo-24484: _run_finalizers() should be thread-safe
        def cb():
            pass

        class Foo(object):
            def __init__(self):
                self.ref = self  # create reference cycle
                # insert finalizer at random key
                util.Finalize(self, cb, exitpriority=random.randint(1, 100))

        finish = False
        exc = None

        def run_finalizers():
            nonlocal exc
            while not finish:
                time.sleep(random.random() * 1e-1)
                try:
                    # A GC run will eventually happen during this,
                    # collecting stale Foo's and mutating the registry
                    util._run_finalizers()
                except Exception as e:
                    exc = e

        def make_finalizers():
            nonlocal exc
            d = {}
            while not finish:
                try:
                    # Old Foo's get gradually replaced and later
                    # collected by the GC (because of the cyclic ref)
                    d[random.getrandbits(5)] = {Foo() for i in range(10)}
                except Exception as e:
                    exc = e
                    d.clear()

        old_interval = sys.getswitchinterval()
        old_threshold = gc.get_threshold()
        try:
            support.setswitchinterval(1e-6)
            gc.set_threshold(5, 5, 5)
            threads = [threading.Thread(target=run_finalizers),
                       threading.Thread(target=make_finalizers)]
            with threading_helper.start_threads(threads):
                time.sleep(4.0)  # Wait a bit to trigger race condition
                finish = True
            if exc is not None:
                raise exc
        finally:
            sys.setswitchinterval(old_interval)
            gc.set_threshold(*old_threshold)
            gc.collect()  # Collect remaining Foo's


#
# Test that from ... import * works for each module
#

class _TestImportStar(unittest.TestCase):

    def get_module_names(self):
        import glob
        folder = os.path.dirname(multiprocessing.__file__)
        pattern = os.path.join(glob.escape(folder), '*.py')
        files = glob.glob(pattern)
        modules = [os.path.splitext(os.path.split(f)[1])[0] for f in files]
        modules = ['multiprocessing.' + m for m in modules]
        modules.remove('multiprocessing.__init__')
        modules.append('multiprocessing')
        return modules

    def test_import(self):
        modules = self.get_module_names()
        if sys.platform == 'win32':
            modules.remove('multiprocessing.popen_fork')
            modules.remove('multiprocessing.popen_forkserver')
            modules.remove('multiprocessing.popen_spawn_posix')
        else:
            modules.remove('multiprocessing.popen_spawn_win32')
            if not HAS_REDUCTION:
                modules.remove('multiprocessing.popen_forkserver')

        if c_int is None:
            # This module requires _ctypes
            modules.remove('multiprocessing.sharedctypes')

        for name in modules:
            __import__(name)
            mod = sys.modules[name]
            self.assertHasAttr(mod, '__all__', name)
            for attr in mod.__all__:
                self.assertHasAttr(mod, attr)

#
# Quick test that logging works -- does not test logging output
#

class _TestLogging(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def test_enable_logging(self):
        logger = multiprocessing.get_logger()
        logger.setLevel(util.SUBWARNING)
        self.assertIsNotNone(logger)
        logger.debug('this will not be printed')
        logger.info('nor will this')
        logger.setLevel(LOG_LEVEL)

    @classmethod
    def _test_level(cls, conn):
        logger = multiprocessing.get_logger()
        conn.send(logger.getEffectiveLevel())

    def test_level(self):
        LEVEL1 = 32
        LEVEL2 = 37

        logger = multiprocessing.get_logger()
        root_logger = logging.getLogger()
        root_level = root_logger.level

        reader, writer = multiprocessing.Pipe(duplex=False)

        logger.setLevel(LEVEL1)
        p = self.Process(target=self._test_level, args=(writer,))
        p.start()
        self.assertEqual(LEVEL1, reader.recv())
        p.join()
        p.close()

        logger.setLevel(logging.NOTSET)
        root_logger.setLevel(LEVEL2)
        p = self.Process(target=self._test_level, args=(writer,))
        p.start()
        self.assertEqual(LEVEL2, reader.recv())
        p.join()
        p.close()

        root_logger.setLevel(root_level)
        logger.setLevel(level=LOG_LEVEL)

    def test_filename(self):
        logger = multiprocessing.get_logger()
        original_level = logger.level
        try:
            logger.setLevel(util.DEBUG)
            stream = io.StringIO()
            handler = logging.StreamHandler(stream)
            logging_format = '[%(levelname)s] [%(filename)s] %(message)s'
            handler.setFormatter(logging.Formatter(logging_format))
            logger.addHandler(handler)
            logger.info('1')
            util.info('2')
            logger.debug('3')
            filename = os.path.basename(__file__)
            log_record = stream.getvalue()
            self.assertIn(f'[INFO] [{filename}] 1', log_record)
            self.assertIn(f'[INFO] [{filename}] 2', log_record)
            self.assertIn(f'[DEBUG] [{filename}] 3', log_record)
        finally:
            logger.setLevel(original_level)
            logger.removeHandler(handler)
            handler.close()


# class _TestLoggingProcessName(BaseTestCase):
#
#     def handle(self, record):
#         assert record.processName == multiprocessing.current_process().name
#         self.__handled = True
#
#     def test_logging(self):
#         handler = logging.Handler()
#         handler.handle = self.handle
#         self.__handled = False
#         # Bypass getLogger() and side-effects
#         logger = logging.getLoggerClass()(
#                 'multiprocessing.test.TestLoggingProcessName')
#         logger.addHandler(handler)
#         logger.propagate = False
#
#         logger.warn('foo')
#         assert self.__handled

#
# Check that Process.join() retries if os.waitpid() fails with EINTR
#

class _TestPollEintr(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    @classmethod
    def _killer(cls, pid):
        time.sleep(0.1)
        os.kill(pid, signal.SIGUSR1)

    @unittest.skipUnless(hasattr(signal, 'SIGUSR1'), 'requires SIGUSR1')
    def test_poll_eintr(self):
        got_signal = [False]
        def record(*args):
            got_signal[0] = True
        pid = os.getpid()
        oldhandler = signal.signal(signal.SIGUSR1, record)
        try:
            killer = self.Process(target=self._killer, args=(pid,))
            killer.start()
            try:
                p = self.Process(target=time.sleep, args=(2,))
                p.start()
                p.join()
            finally:
                killer.join()
            self.assertTrue(got_signal[0])
            self.assertEqual(p.exitcode, 0)
        finally:
            signal.signal(signal.SIGUSR1, oldhandler)

#
# Test to verify handle verification, see issue 3321
#

class TestInvalidHandle(unittest.TestCase):

    @unittest.skipIf(WIN32, "skipped on Windows")
    def test_invalid_handles(self):
        conn = multiprocessing.connection.Connection(44977608)
        # check that poll() doesn't crash
        try:
            conn.poll()
        except (ValueError, OSError):
            pass
        finally:
            # Hack private attribute _handle to avoid printing an error
            # in conn.__del__
            conn._handle = None
        self.assertRaises((ValueError, OSError),
                          multiprocessing.connection.Connection, -1)



@hashlib_helper.requires_hashdigest('sha256')
class OtherTest(unittest.TestCase):
    # TODO: add more tests for deliver/answer challenge.
    def test_deliver_challenge_auth_failure(self):
        class _FakeConnection(object):
            def recv_bytes(self, size):
                return b'something bogus'
            def send_bytes(self, data):
                pass
        self.assertRaises(multiprocessing.AuthenticationError,
                          multiprocessing.connection.deliver_challenge,
                          _FakeConnection(), b'abc')

    def test_answer_challenge_auth_failure(self):
        class _FakeConnection(object):
            def __init__(self):
                self.count = 0
            def recv_bytes(self, size):
                self.count += 1
                if self.count == 1:
                    return multiprocessing.connection._CHALLENGE
                elif self.count == 2:
                    return b'something bogus'
                return b''
            def send_bytes(self, data):
                pass
        self.assertRaises(multiprocessing.AuthenticationError,
                          multiprocessing.connection.answer_challenge,
                          _FakeConnection(), b'abc')


@hashlib_helper.requires_hashdigest('md5')
@hashlib_helper.requires_hashdigest('sha256')
class ChallengeResponseTest(unittest.TestCase):
    authkey = b'supadupasecretkey'

    def create_response(self, message):
        return multiprocessing.connection._create_response(
            self.authkey, message
        )

    def verify_challenge(self, message, response):
        return multiprocessing.connection._verify_challenge(
            self.authkey, message, response
        )

    def test_challengeresponse(self):
        for algo in [None, "md5", "sha256"]:
            with self.subTest(f"{algo=}"):
                msg = b'is-twenty-bytes-long'  # The length of a legacy message.
                if algo:
                    prefix = b'{%s}' % algo.encode("ascii")
                else:
                    prefix = b''
                msg = prefix + msg
                response = self.create_response(msg)
                if not response.startswith(prefix):
                    self.fail(response)
                self.verify_challenge(msg, response)

    # TODO(gpshead): We need integration tests for handshakes between modern
    # deliver_challenge() and verify_response() code and connections running a
    # test-local copy of the legacy Python <=3.11 implementations.

    # TODO(gpshead): properly annotate tests for requires_hashdigest rather than
    # only running these on a platform supporting everything.  otherwise logic
    # issues preventing it from working on FIPS mode setups will be hidden.

#
# Test Manager.start()/Pool.__init__() initializer feature - see issue 5585
#

def initializer(ns):
    ns.test += 1

@hashlib_helper.requires_hashdigest('sha256')
class TestInitializers(unittest.TestCase):
    def setUp(self):
        self.mgr = multiprocessing.Manager()
        self.ns = self.mgr.Namespace()
        self.ns.test = 0

    def tearDown(self):
        self.mgr.shutdown()
        self.mgr.join()

    def test_manager_initializer(self):
        m = multiprocessing.managers.SyncManager()
        self.assertRaises(TypeError, m.start, 1)
        m.start(initializer, (self.ns,))
        self.assertEqual(self.ns.test, 1)
        m.shutdown()
        m.join()

    def test_pool_initializer(self):
        self.assertRaises(TypeError, multiprocessing.Pool, initializer=1)
        p = multiprocessing.Pool(1, initializer, (self.ns,))
        p.close()
        p.join()
        self.assertEqual(self.ns.test, 1)

#
# Issue 5155, 5313, 5331: Test process in processes
# Verifies os.close(sys.stdin.fileno) vs. sys.stdin.close() behavior
#

def _this_sub_process(q):
    try:
        item = q.get(block=False)
    except pyqueue.Empty:
        pass

def _test_process():
    queue = multiprocessing.Queue()
    subProc = multiprocessing.Process(target=_this_sub_process, args=(queue,))
    subProc.daemon = True
    subProc.start()
    subProc.join()

def _afunc(x):
    return x*x

def pool_in_process():
    pool = multiprocessing.Pool(processes=4)
    x = pool.map(_afunc, [1, 2, 3, 4, 5, 6, 7])
    pool.close()
    pool.join()

class _file_like(object):
    def __init__(self, delegate):
        self._delegate = delegate
        self._pid = None

    @property
    def cache(self):
        pid = os.getpid()
        # There are no race conditions since fork keeps only the running thread
        if pid != self._pid:
            self._pid = pid
            self._cache = []
        return self._cache

    def write(self, data):
        self.cache.append(data)

    def flush(self):
        self._delegate.write(''.join(self.cache))
        self._cache = []

class TestStdinBadfiledescriptor(unittest.TestCase):

    def test_queue_in_process(self):
        proc = multiprocessing.Process(target=_test_process)
        proc.start()
        proc.join()

    def test_pool_in_process(self):
        p = multiprocessing.Process(target=pool_in_process)
        p.start()
        p.join()

    def test_flushing(self):
        sio = io.StringIO()
        flike = _file_like(sio)
        flike.write('foo')
        proc = multiprocessing.Process(target=lambda: flike.flush())
        flike.flush()
        assert sio.getvalue() == 'foo'


class TestWait(unittest.TestCase):

    @classmethod
    def _child_test_wait(cls, w, slow):
        for i in range(10):
            if slow:
                time.sleep(random.random() * 0.100)
            w.send((i, os.getpid()))
        w.close()

    def test_wait(self, slow=False):
        from multiprocessing.connection import wait
        readers = []
        procs = []
        messages = []

        for i in range(4):
            r, w = multiprocessing.Pipe(duplex=False)
            p = multiprocessing.Process(target=self._child_test_wait, args=(w, slow))
            p.daemon = True
            p.start()
            w.close()
            readers.append(r)
            procs.append(p)
            self.addCleanup(p.join)

        while readers:
            for r in wait(readers):
                try:
                    msg = r.recv()
                except EOFError:
                    readers.remove(r)
                    r.close()
                else:
                    messages.append(msg)

        messages.sort()
        expected = sorted((i, p.pid) for i in range(10) for p in procs)
        self.assertEqual(messages, expected)

    @classmethod
    def _child_test_wait_socket(cls, address, slow):
        s = socket.socket()
        s.connect(address)
        for i in range(10):
            if slow:
                time.sleep(random.random() * 0.100)
            s.sendall(('%s\n' % i).encode('ascii'))
        s.close()

    def test_wait_socket(self, slow=False):
        from multiprocessing.connection import wait
        l = socket.create_server((socket_helper.HOST, 0))
        addr = l.getsockname()
        readers = []
        procs = []
        dic = {}

        for i in range(4):
            p = multiprocessing.Process(target=self._child_test_wait_socket,
                                        args=(addr, slow))
            p.daemon = True
            p.start()
            procs.append(p)
            self.addCleanup(p.join)

        for i in range(4):
            r, _ = l.accept()
            readers.append(r)
            dic[r] = []
        l.close()

        while readers:
            for r in wait(readers):
                msg = r.recv(32)
                if not msg:
                    readers.remove(r)
                    r.close()
                else:
                    dic[r].append(msg)

        expected = ''.join('%s\n' % i for i in range(10)).encode('ascii')
        for v in dic.values():
            self.assertEqual(b''.join(v), expected)

    def test_wait_slow(self):
        self.test_wait(True)

    def test_wait_socket_slow(self):
        self.test_wait_socket(True)

    @support.requires_resource('walltime')
    def test_wait_timeout(self):
        from multiprocessing.connection import wait

        timeout = 5.0  # seconds
        a, b = multiprocessing.Pipe()

        start = time.monotonic()
        res = wait([a, b], timeout)
        delta = time.monotonic() - start

        self.assertEqual(res, [])
        self.assertGreater(delta, timeout - CLOCK_RES)

        b.send(None)
        res = wait([a, b], 20)
        self.assertEqual(res, [a])

    @classmethod
    def signal_and_sleep(cls, sem, period):
        sem.release()
        time.sleep(period)

    @support.requires_resource('walltime')
    def test_wait_integer(self):
        from multiprocessing.connection import wait

        expected = 3
        sorted_ = lambda l: sorted(l, key=lambda x: id(x))
        sem = multiprocessing.Semaphore(0)
        a, b = multiprocessing.Pipe()
        p = multiprocessing.Process(target=self.signal_and_sleep,
                                    args=(sem, expected))

        p.start()
        self.assertIsInstance(p.sentinel, int)
        self.assertTrue(sem.acquire(timeout=20))

        start = time.monotonic()
        res = wait([a, p.sentinel, b], expected + 20)
        delta = time.monotonic() - start

        self.assertEqual(res, [p.sentinel])
        self.assertLess(delta, expected + 2)
        self.assertGreater(delta, expected - 2)

        a.send(None)

        start = time.monotonic()
        res = wait([a, p.sentinel, b], 20)
        delta = time.monotonic() - start

        self.assertEqual(sorted_(res), sorted_([p.sentinel, b]))
        self.assertLess(delta, 0.4)

        b.send(None)

        start = time.monotonic()
        res = wait([a, p.sentinel, b], 20)
        delta = time.monotonic() - start

        self.assertEqual(sorted_(res), sorted_([a, p.sentinel, b]))
        self.assertLess(delta, 0.4)

        p.terminate()
        p.join()

    def test_neg_timeout(self):
        from multiprocessing.connection import wait
        a, b = multiprocessing.Pipe()
        t = time.monotonic()
        res = wait([a], timeout=-1)
        t = time.monotonic() - t
        self.assertEqual(res, [])
        self.assertLess(t, 1)
        a.close()
        b.close()

#
# Issue 14151: Test invalid family on invalid environment
#

class TestInvalidFamily(unittest.TestCase):

    @unittest.skipIf(WIN32, "skipped on Windows")
    def test_invalid_family(self):
        with self.assertRaises(ValueError):
            multiprocessing.connection.Listener(r'\\.\test')

    @unittest.skipUnless(WIN32, "skipped on non-Windows platforms")
    def test_invalid_family_win32(self):
        with self.assertRaises(ValueError):
            multiprocessing.connection.Listener('/var/test.pipe')

#
# Issue 12098: check sys.flags of child matches that for parent
#

class TestFlags(unittest.TestCase):
    @classmethod
    def run_in_grandchild(cls, conn):
        conn.send(tuple(sys.flags))

    @classmethod
    def run_in_child(cls, start_method):
        import json
        mp = multiprocessing.get_context(start_method)
        r, w = mp.Pipe(duplex=False)
        p = mp.Process(target=cls.run_in_grandchild, args=(w,))
        with warnings.catch_warnings(category=DeprecationWarning):
            p.start()
        grandchild_flags = r.recv()
        p.join()
        r.close()
        w.close()
        flags = (tuple(sys.flags), grandchild_flags)
        print(json.dumps(flags))

    def test_flags(self):
        import json
        # start child process using unusual flags
        prog = (
            'from test._test_multiprocessing import TestFlags; '
            f'TestFlags.run_in_child({multiprocessing.get_start_method()!r})'
        )
        data = subprocess.check_output(
            [sys.executable, '-E', '-S', '-O', '-c', prog])
        child_flags, grandchild_flags = json.loads(data.decode('ascii'))
        self.assertEqual(child_flags, grandchild_flags)

#
# Test interaction with socket timeouts - see Issue #6056
#

class TestTimeouts(unittest.TestCase):
    @classmethod
    def _test_timeout(cls, child, address):
        time.sleep(1)
        child.send(123)
        child.close()
        conn = multiprocessing.connection.Client(address)
        conn.send(456)
        conn.close()

    def test_timeout(self):
        old_timeout = socket.getdefaulttimeout()
        try:
            socket.setdefaulttimeout(0.1)
            parent, child = multiprocessing.Pipe(duplex=True)
            l = multiprocessing.connection.Listener(family='AF_INET')
            p = multiprocessing.Process(target=self._test_timeout,
                                        args=(child, l.address))
            p.start()
            child.close()
            self.assertEqual(parent.recv(), 123)
            parent.close()
            conn = l.accept()
            self.assertEqual(conn.recv(), 456)
            conn.close()
            l.close()
            join_process(p)
        finally:
            socket.setdefaulttimeout(old_timeout)

#
# Test what happens with no "if __name__ == '__main__'"
#

class TestNoForkBomb(unittest.TestCase):
    def test_noforkbomb(self):
        sm = multiprocessing.get_start_method()
        name = os.path.join(os.path.dirname(__file__), 'mp_fork_bomb.py')
        if sm != 'fork':
            rc, out, err = test.support.script_helper.assert_python_failure(name, sm)
            self.assertEqual(out, b'')
            self.assertIn(b'RuntimeError', err)
        else:
            rc, out, err = test.support.script_helper.assert_python_ok(name, sm)
            self.assertEqual(out.rstrip(), b'123')
            self.assertEqual(err, b'')

#
# Issue #17555: ForkAwareThreadLock
#

class TestForkAwareThreadLock(unittest.TestCase):
    # We recursively start processes.  Issue #17555 meant that the
    # after fork registry would get duplicate entries for the same
    # lock.  The size of the registry at generation n was ~2**n.

    @classmethod
    def child(cls, n, conn):
        if n > 1:
            p = multiprocessing.Process(target=cls.child, args=(n-1, conn))
            p.start()
            conn.close()
            join_process(p)
        else:
            conn.send(len(util._afterfork_registry))
        conn.close()

    def test_lock(self):
        r, w = multiprocessing.Pipe(False)
        l = util.ForkAwareThreadLock()
        old_size = len(util._afterfork_registry)
        p = multiprocessing.Process(target=self.child, args=(5, w))
        p.start()
        w.close()
        new_size = r.recv()
        join_process(p)
        self.assertLessEqual(new_size, old_size)

#
# Check that non-forked child processes do not inherit unneeded fds/handles
#

class TestCloseFds(unittest.TestCase):

    def get_high_socket_fd(self):
        if WIN32:
            # The child process will not have any socket handles, so
            # calling socket.fromfd() should produce WSAENOTSOCK even
            # if there is a handle of the same number.
            return socket.socket().detach()
        else:
            # We want to produce a socket with an fd high enough that a
            # freshly created child process will not have any fds as high.
            fd = socket.socket().detach()
            to_close = []
            while fd < 50:
                to_close.append(fd)
                fd = os.dup(fd)
            for x in to_close:
                os.close(x)
            return fd

    def close(self, fd):
        if WIN32:
            socket.socket(socket.AF_INET, socket.SOCK_STREAM, fileno=fd).close()
        else:
            os.close(fd)

    @classmethod
    def _test_closefds(cls, conn, fd):
        try:
            s = socket.fromfd(fd, socket.AF_INET, socket.SOCK_STREAM)
        except Exception as e:
            conn.send(e)
        else:
            s.close()
            conn.send(None)

    def test_closefd(self):
        if not HAS_REDUCTION:
            raise unittest.SkipTest('requires fd pickling')

        reader, writer = multiprocessing.Pipe()
        fd = self.get_high_socket_fd()
        try:
            p = multiprocessing.Process(target=self._test_closefds,
                                        args=(writer, fd))
            p.start()
            writer.close()
            e = reader.recv()
            join_process(p)
        finally:
            self.close(fd)
            writer.close()
            reader.close()

        if multiprocessing.get_start_method() == 'fork':
            self.assertIs(e, None)
        else:
            WSAENOTSOCK = 10038
            self.assertIsInstance(e, OSError)
            self.assertTrue(e.errno == errno.EBADF or
                            e.winerror == WSAENOTSOCK, e)

#
# Issue #17097: EINTR should be ignored by recv(), send(), accept() etc
#

class TestIgnoreEINTR(unittest.TestCase):

    # Sending CONN_MAX_SIZE bytes into a multiprocessing pipe must block
    CONN_MAX_SIZE = max(support.PIPE_MAX_SIZE, support.SOCK_MAX_SIZE)

    @classmethod
    def _test_ignore(cls, conn):
        def handler(signum, frame):
            pass
        signal.signal(signal.SIGUSR1, handler)
        conn.send('ready')
        x = conn.recv()
        conn.send(x)
        conn.send_bytes(b'x' * cls.CONN_MAX_SIZE)

    @unittest.skipUnless(hasattr(signal, 'SIGUSR1'), 'requires SIGUSR1')
    def test_ignore(self):
        conn, child_conn = multiprocessing.Pipe()
        try:
            p = multiprocessing.Process(target=self._test_ignore,
                                        args=(child_conn,))
            p.daemon = True
            p.start()
            child_conn.close()
            self.assertEqual(conn.recv(), 'ready')
            time.sleep(0.1)
            os.kill(p.pid, signal.SIGUSR1)
            time.sleep(0.1)
            conn.send(1234)
            self.assertEqual(conn.recv(), 1234)
            time.sleep(0.1)
            os.kill(p.pid, signal.SIGUSR1)
            self.assertEqual(conn.recv_bytes(), b'x' * self.CONN_MAX_SIZE)
            time.sleep(0.1)
            p.join()
        finally:
            conn.close()

    @classmethod
    def _test_ignore_listener(cls, conn):
        def handler(signum, frame):
            pass
        signal.signal(signal.SIGUSR1, handler)
        with multiprocessing.connection.Listener() as l:
            conn.send(l.address)
            a = l.accept()
            a.send('welcome')

    @unittest.skipUnless(hasattr(signal, 'SIGUSR1'), 'requires SIGUSR1')
    def test_ignore_listener(self):
        conn, child_conn = multiprocessing.Pipe()
        try:
            p = multiprocessing.Process(target=self._test_ignore_listener,
                                        args=(child_conn,))
            p.daemon = True
            p.start()
            child_conn.close()
            address = conn.recv()
            time.sleep(0.1)
            os.kill(p.pid, signal.SIGUSR1)
            time.sleep(0.1)
            client = multiprocessing.connection.Client(address)
            self.assertEqual(client.recv(), 'welcome')
            p.join()
        finally:
            conn.close()

class TestStartMethod(unittest.TestCase):
    @classmethod
    def _check_context(cls, conn):
        conn.send(multiprocessing.get_start_method())

    def check_context(self, ctx):
        r, w = ctx.Pipe(duplex=False)
        p = ctx.Process(target=self._check_context, args=(w,))
        p.start()
        w.close()
        child_method = r.recv()
        r.close()
        p.join()
        self.assertEqual(child_method, ctx.get_start_method())

    def test_context(self):
        for method in ('fork', 'spawn', 'forkserver'):
            try:
                ctx = multiprocessing.get_context(method)
            except ValueError:
                continue
            self.assertEqual(ctx.get_start_method(), method)
            self.assertIs(ctx.get_context(), ctx)
            self.assertRaises(ValueError, ctx.set_start_method, 'spawn')
            self.assertRaises(ValueError, ctx.set_start_method, None)
            self.check_context(ctx)

    def test_context_check_module_types(self):
        try:
            ctx = multiprocessing.get_context('forkserver')
        except ValueError:
            raise unittest.SkipTest('forkserver should be available')
        with self.assertRaisesRegex(TypeError, 'module_names must be a list of strings'):
            ctx.set_forkserver_preload([1, 2, 3])

    def test_set_get(self):
        multiprocessing.set_forkserver_preload(PRELOAD)
        count = 0
        old_method = multiprocessing.get_start_method()
        try:
            for method in ('fork', 'spawn', 'forkserver'):
                try:
                    multiprocessing.set_start_method(method, force=True)
                except ValueError:
                    continue
                self.assertEqual(multiprocessing.get_start_method(), method)
                ctx = multiprocessing.get_context()
                self.assertEqual(ctx.get_start_method(), method)
                self.assertStartsWith(type(ctx).__name__.lower(), method)
                self.assertStartsWith(ctx.Process.__name__.lower(), method)
                self.check_context(multiprocessing)
                count += 1
        finally:
            multiprocessing.set_start_method(old_method, force=True)
        self.assertGreaterEqual(count, 1)

    def test_get_all_start_methods(self):
        methods = multiprocessing.get_all_start_methods()
        self.assertIn('spawn', methods)
        if sys.platform == 'win32':
            self.assertEqual(methods, ['spawn'])
        elif sys.platform == 'darwin':
            self.assertEqual(methods[0], 'spawn')  # The default is first.
            # Whether these work or not, they remain available on macOS.
            self.assertIn('fork', methods)
            self.assertIn('forkserver', methods)
        else:
            # POSIX
            self.assertIn('fork', methods)
            if other_methods := set(methods) - {'fork', 'spawn'}:
                # If there are more than those two, forkserver must be one.
                self.assertEqual({'forkserver'}, other_methods)
            # The default is the first method in the list.
            self.assertIn(methods[0], {'forkserver', 'spawn'},
                          msg='3.14+ default must not be fork')
            if methods[0] == 'spawn':
                # Confirm that the current default selection logic prefers
                # forkserver vs spawn when available.
                self.assertNotIn('forkserver', methods)

    def test_preload_resources(self):
        if multiprocessing.get_start_method() != 'forkserver':
            self.skipTest("test only relevant for 'forkserver' method")
        name = os.path.join(os.path.dirname(__file__), 'mp_preload.py')
        rc, out, err = test.support.script_helper.assert_python_ok(name)
        out = out.decode()
        err = err.decode()
        if out.rstrip() != 'ok' or err != '':
            print(out)
            print(err)
            self.fail("failed spawning forkserver or grandchild")

    @unittest.skipIf(sys.platform == "win32",
                     "Only Spawn on windows so no risk of mixing")
    @only_run_in_spawn_testsuite("avoids redundant testing.")
    def test_mixed_startmethod(self):
        # Fork-based locks cannot be used with spawned process
        for process_method in ["spawn", "forkserver"]:
            queue = multiprocessing.get_context("fork").Queue()
            process_ctx = multiprocessing.get_context(process_method)
            p = process_ctx.Process(target=close_queue, args=(queue,))
            err_msg = "A SemLock created in a fork"
            with self.assertRaisesRegex(RuntimeError, err_msg):
                p.start()

        # non-fork-based locks can be used with all other start methods
        for queue_method in ["spawn", "forkserver"]:
            for process_method in multiprocessing.get_all_start_methods():
                queue = multiprocessing.get_context(queue_method).Queue()
                process_ctx = multiprocessing.get_context(process_method)
                p = process_ctx.Process(target=close_queue, args=(queue,))
                p.start()
                p.join()

    @classmethod
    def _put_one_in_queue(cls, queue):
        queue.put(1)

    @classmethod
    def _put_two_and_nest_once(cls, queue):
        queue.put(2)
        process = multiprocessing.Process(target=cls._put_one_in_queue, args=(queue,))
        process.start()
        process.join()

    def test_nested_startmethod(self):
        # gh-108520: Regression test to ensure that child process can send its
        # arguments to another process
        queue = multiprocessing.Queue()

        process = multiprocessing.Process(target=self._put_two_and_nest_once, args=(queue,))
        process.start()
        process.join()

        results = []
        while not queue.empty():
            results.append(queue.get())

        # gh-109706: queue.put(1) can write into the queue before queue.put(2),
        # there is no synchronization in the test.
        self.assertSetEqual(set(results), set([2, 1]))


@unittest.skipIf(sys.platform == "win32",
                 "test semantics don't make sense on Windows")
class TestResourceTracker(unittest.TestCase):

    def test_resource_tracker(self):
        #
        # Check that killing process does not leak named semaphores
        #
        cmd = '''if 1:
            import time, os
            import multiprocessing as mp
            from multiprocessing import resource_tracker
            from multiprocessing.shared_memory import SharedMemory

            mp.set_start_method("spawn")


            def create_and_register_resource(rtype):
                if rtype == "semaphore":
                    lock = mp.Lock()
                    return lock, lock._semlock.name
                elif rtype == "shared_memory":
                    sm = SharedMemory(create=True, size=10)
                    return sm, sm._name
                else:
                    raise ValueError(
                        "Resource type {{}} not understood".format(rtype))


            resource1, rname1 = create_and_register_resource("{rtype}")
            resource2, rname2 = create_and_register_resource("{rtype}")

            os.write({w}, rname1.encode("ascii") + b"\\n")
            os.write({w}, rname2.encode("ascii") + b"\\n")

            time.sleep(10)
        '''
        for rtype in resource_tracker._CLEANUP_FUNCS:
            with self.subTest(rtype=rtype):
                if rtype in ("noop", "dummy"):
                    # Artefact resource type used by the resource_tracker
                    # or tests
                    continue
                r, w = os.pipe()
                p = subprocess.Popen([sys.executable,
                                     '-E', '-c', cmd.format(w=w, rtype=rtype)],
                                     pass_fds=[w],
                                     stderr=subprocess.PIPE)
                os.close(w)
                with open(r, 'rb', closefd=True) as f:
                    name1 = f.readline().rstrip().decode('ascii')
                    name2 = f.readline().rstrip().decode('ascii')
                _resource_unlink(name1, rtype)
                p.terminate()
                p.wait()

                err_msg = (f"A {rtype} resource was leaked after a process was "
                           f"abruptly terminated")
                for _ in support.sleeping_retry(support.SHORT_TIMEOUT,
                                                  err_msg):
                    try:
                        _resource_unlink(name2, rtype)
                    except OSError as e:
                        # docs say it should be ENOENT, but OSX seems to give
                        # EINVAL
                        self.assertIn(e.errno, (errno.ENOENT, errno.EINVAL))
                        break

                err = p.stderr.read().decode('utf-8')
                p.stderr.close()
                expected = ('resource_tracker: There appear to be 2 leaked {} '
                            'objects'.format(
                            rtype))
                self.assertRegex(err, expected)
                self.assertRegex(err, r'resource_tracker: %r: \[Errno' % name1)

    def check_resource_tracker_death(self, signum, should_die):
        # bpo-31310: if the semaphore tracker process has died, it should
        # be restarted implicitly.
        from multiprocessing.resource_tracker import _resource_tracker
        pid = _resource_tracker._pid
        if pid is not None:
            os.kill(pid, signal.SIGKILL)
            support.wait_process(pid, exitcode=-signal.SIGKILL)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            _resource_tracker.ensure_running()
        pid = _resource_tracker._pid

        os.kill(pid, signum)
        time.sleep(1.0)  # give it time to die

        ctx = multiprocessing.get_context("spawn")
        with warnings.catch_warnings(record=True) as all_warn:
            warnings.simplefilter("always")
            sem = ctx.Semaphore()
            sem.acquire()
            sem.release()
            wr = weakref.ref(sem)
            # ensure `sem` gets collected, which triggers communication with
            # the semaphore tracker
            del sem
            gc.collect()
            self.assertIsNone(wr())
            if should_die:
                self.assertEqual(len(all_warn), 1)
                the_warn = all_warn[0]
                self.assertIsSubclass(the_warn.category, UserWarning)
                self.assertIn("resource_tracker: process died",
                              str(the_warn.message))
            else:
                self.assertEqual(len(all_warn), 0)

    def test_resource_tracker_sigint(self):
        # Catchable signal (ignored by semaphore tracker)
        self.check_resource_tracker_death(signal.SIGINT, False)

    def test_resource_tracker_sigterm(self):
        # Catchable signal (ignored by semaphore tracker)
        self.check_resource_tracker_death(signal.SIGTERM, False)

    @unittest.skipIf(sys.platform.startswith("netbsd"),
                     "gh-125620: Skip on NetBSD due to long wait for SIGKILL process termination.")
    def test_resource_tracker_sigkill(self):
        # Uncatchable signal.
        self.check_resource_tracker_death(signal.SIGKILL, True)

    @staticmethod
    def _is_resource_tracker_reused(conn, pid):
        from multiprocessing.resource_tracker import _resource_tracker
        _resource_tracker.ensure_running()
        # The pid should be None in the child process, expect for the fork
        # context. It should not be a new value.
        reused = _resource_tracker._pid in (None, pid)
        reused &= _resource_tracker._check_alive()
        conn.send(reused)

    def test_resource_tracker_reused(self):
        from multiprocessing.resource_tracker import _resource_tracker
        _resource_tracker.ensure_running()
        pid = _resource_tracker._pid

        r, w = multiprocessing.Pipe(duplex=False)
        p = multiprocessing.Process(target=self._is_resource_tracker_reused,
                                    args=(w, pid))
        p.start()
        is_resource_tracker_reused = r.recv()

        # Clean up
        p.join()
        w.close()
        r.close()

        self.assertTrue(is_resource_tracker_reused)

    def test_too_long_name_resource(self):
        # gh-96819: Resource names that will make the length of a write to a pipe
        # greater than PIPE_BUF are not allowed
        rtype = "shared_memory"
        too_long_name_resource = "a" * (512 - len(rtype))
        with self.assertRaises(ValueError):
            resource_tracker.register(too_long_name_resource, rtype)

    def _test_resource_tracker_leak_resources(self, cleanup):
        # We use a separate instance for testing, since the main global
        # _resource_tracker may be used to watch test infrastructure.
        from multiprocessing.resource_tracker import ResourceTracker
        tracker = ResourceTracker()
        tracker.ensure_running()
        self.assertTrue(tracker._check_alive())

        self.assertIsNone(tracker._exitcode)
        tracker.register('somename', 'dummy')
        if cleanup:
            tracker.unregister('somename', 'dummy')
            expected_exit_code = 0
        else:
            expected_exit_code = 1

        self.assertTrue(tracker._check_alive())
        self.assertIsNone(tracker._exitcode)
        tracker._stop()
        self.assertEqual(tracker._exitcode, expected_exit_code)

    def test_resource_tracker_exit_code(self):
        """
        Test the exit code of the resource tracker.

        If no leaked resources were found, exit code should be 0, otherwise 1
        """
        for cleanup in [True, False]:
            with self.subTest(cleanup=cleanup):
                self._test_resource_tracker_leak_resources(
                    cleanup=cleanup,
                )

    @unittest.skipUnless(hasattr(signal, "pthread_sigmask"), "pthread_sigmask is not available")
    def test_resource_tracker_blocked_signals(self):
        #
        # gh-127586: Check that resource_tracker does not override blocked signals of caller.
        #
        from multiprocessing.resource_tracker import ResourceTracker
        orig_sigmask = signal.pthread_sigmask(signal.SIG_BLOCK, set())
        signals = {signal.SIGTERM, signal.SIGINT, signal.SIGUSR1}

        try:
            for sig in signals:
                signal.pthread_sigmask(signal.SIG_SETMASK, {sig})
                self.assertEqual(signal.pthread_sigmask(signal.SIG_BLOCK, set()), {sig})
                tracker = ResourceTracker()
                tracker.ensure_running()
                self.assertEqual(signal.pthread_sigmask(signal.SIG_BLOCK, set()), {sig})
                tracker._stop()
        finally:
            # restore sigmask to what it was before executing test
            signal.pthread_sigmask(signal.SIG_SETMASK, orig_sigmask)

class TestSimpleQueue(unittest.TestCase):

    @classmethod
    def _test_empty(cls, queue, child_can_start, parent_can_continue):
        child_can_start.wait()
        # issue 30301, could fail under spawn and forkserver
        try:
            queue.put(queue.empty())
            queue.put(queue.empty())
        finally:
            parent_can_continue.set()

    def test_empty_exceptions(self):
        # Assert that checking emptiness of a closed queue raises
        # an OSError, independently of whether the queue was used
        # or not. This differs from Queue and JoinableQueue.
        q = multiprocessing.SimpleQueue()
        q.close()  # close the pipe
        with self.assertRaisesRegex(OSError, 'is closed'):
            q.empty()

    def test_empty(self):
        queue = multiprocessing.SimpleQueue()
        child_can_start = multiprocessing.Event()
        parent_can_continue = multiprocessing.Event()

        proc = multiprocessing.Process(
            target=self._test_empty,
            args=(queue, child_can_start, parent_can_continue)
        )
        proc.daemon = True
        proc.start()

        self.assertTrue(queue.empty())

        child_can_start.set()
        parent_can_continue.wait()

        self.assertFalse(queue.empty())
        self.assertEqual(queue.get(), True)
        self.assertEqual(queue.get(), False)
        self.assertTrue(queue.empty())

        proc.join()

    def test_close(self):
        queue = multiprocessing.SimpleQueue()
        queue.close()
        # closing a queue twice should not fail
        queue.close()

    # Test specific to CPython since it tests private attributes
    @test.support.cpython_only
    def test_closed(self):
        queue = multiprocessing.SimpleQueue()
        queue.close()
        self.assertTrue(queue._reader.closed)
        self.assertTrue(queue._writer.closed)


class TestPoolNotLeakOnFailure(unittest.TestCase):

    def test_release_unused_processes(self):
        # Issue #19675: During pool creation, if we can't create a process,
        # don't leak already created ones.
        will_fail_in = 3
        forked_processes = []

        class FailingForkProcess:
            def __init__(self, **kwargs):
                self.name = 'Fake Process'
                self.exitcode = None
                self.state = None
                forked_processes.append(self)

            def start(self):
                nonlocal will_fail_in
                if will_fail_in <= 0:
                    raise OSError("Manually induced OSError")
                will_fail_in -= 1
                self.state = 'started'

            def terminate(self):
                self.state = 'stopping'

            def join(self):
                if self.state == 'stopping':
                    self.state = 'stopped'

            def is_alive(self):
                return self.state == 'started' or self.state == 'stopping'

        with self.assertRaisesRegex(OSError, 'Manually induced OSError'):
            p = multiprocessing.pool.Pool(5, context=unittest.mock.MagicMock(
                Process=FailingForkProcess))
            p.close()
            p.join()
        for process in forked_processes:
            self.assertFalse(process.is_alive(), process)


@hashlib_helper.requires_hashdigest('sha256')
class TestSyncManagerTypes(unittest.TestCase):
    """Test all the types which can be shared between a parent and a
    child process by using a manager which acts as an intermediary
    between them.

    In the following unit-tests the base type is created in the parent
    process, the @classmethod represents the worker process and the
    shared object is readable and editable between the two.

    # The child.
    @classmethod
    def _test_list(cls, obj):
        assert obj[0] == 5
        assert obj.append(6)

    # The parent.
    def test_list(self):
        o = self.manager.list()
        o.append(5)
        self.run_worker(self._test_list, o)
        assert o[1] == 6
    """
    manager_class = multiprocessing.managers.SyncManager

    def setUp(self):
        self.manager = self.manager_class()
        self.manager.start()
        self.proc = None

    def tearDown(self):
        if self.proc is not None and self.proc.is_alive():
            self.proc.terminate()
            self.proc.join()
        self.manager.shutdown()
        self.manager = None
        self.proc = None

    @classmethod
    def setUpClass(cls):
        support.reap_children()

    tearDownClass = setUpClass

    def wait_proc_exit(self):
        # Only the manager process should be returned by active_children()
        # but this can take a bit on slow machines, so wait a few seconds
        # if there are other children too (see #17395).
        join_process(self.proc)

        timeout = WAIT_ACTIVE_CHILDREN_TIMEOUT
        start_time = time.monotonic()
        for _ in support.sleeping_retry(timeout, error=False):
            if len(multiprocessing.active_children()) <= 1:
                break
        else:
            dt = time.monotonic() - start_time
            support.environment_altered = True
            support.print_warning(f"multiprocessing.Manager still has "
                                  f"{multiprocessing.active_children()} "
                                  f"active children after {dt:.1f} seconds")

    def run_worker(self, worker, obj):
        self.proc = multiprocessing.Process(target=worker, args=(obj, ))
        self.proc.daemon = True
        self.proc.start()
        self.wait_proc_exit()
        self.assertEqual(self.proc.exitcode, 0)

    @classmethod
    def _test_event(cls, obj):
        assert obj.is_set()
        obj.wait()
        obj.clear()
        obj.wait(0.001)

    def test_event(self):
        o = self.manager.Event()
        o.set()
        self.run_worker(self._test_event, o)
        assert not o.is_set()
        o.wait(0.001)

    @classmethod
    def _test_lock(cls, obj):
        obj.acquire()
        obj.locked()

    def test_lock(self, lname="Lock"):
        o = getattr(self.manager, lname)()
        self.run_worker(self._test_lock, o)
        o.release()
        self.assertRaises(RuntimeError, o.release)  # already released

    @classmethod
    def _test_rlock(cls, obj):
        obj.acquire()
        obj.release()
        obj.locked()

    def test_rlock(self, lname="RLock"):
        o = getattr(self.manager, lname)()
        self.run_worker(self._test_rlock, o)

    @classmethod
    def _test_semaphore(cls, obj):
        obj.acquire()

    def test_semaphore(self, sname="Semaphore"):
        o = getattr(self.manager, sname)()
        self.run_worker(self._test_semaphore, o)
        o.release()

    def test_bounded_semaphore(self):
        self.test_semaphore(sname="BoundedSemaphore")

    @classmethod
    def _test_condition(cls, obj):
        obj.acquire()
        obj.release()

    def test_condition(self):
        o = self.manager.Condition()
        self.run_worker(self._test_condition, o)

    @classmethod
    def _test_barrier(cls, obj):
        assert obj.parties == 5
        obj.reset()

    def test_barrier(self):
        o = self.manager.Barrier(5)
        self.run_worker(self._test_barrier, o)

    @classmethod
    def _test_pool(cls, obj):
        # TODO: fix https://bugs.python.org/issue35919
        with obj:
            pass

    def test_pool(self):
        o = self.manager.Pool(processes=4)
        self.run_worker(self._test_pool, o)

    @classmethod
    def _test_queue(cls, obj):
        assert obj.qsize() == 2
        assert obj.full()
        assert not obj.empty()
        assert obj.get() == 5
        assert not obj.empty()
        assert obj.get() == 6
        assert obj.empty()

    def test_queue(self, qname="Queue"):
        o = getattr(self.manager, qname)(2)
        o.put(5)
        o.put(6)
        self.run_worker(self._test_queue, o)
        assert o.empty()
        assert not o.full()

    def test_joinable_queue(self):
        self.test_queue("JoinableQueue")

    @classmethod
    def _test_list(cls, obj):
        case = unittest.TestCase()
        case.assertEqual(obj[0], 5)
        case.assertEqual(obj.count(5), 1)
        case.assertEqual(obj.index(5), 0)
        obj += [7]
        case.assertIsInstance(obj, multiprocessing.managers.ListProxy)
        case.assertListEqual(list(obj), [5, 7])
        obj *= 2
        case.assertIsInstance(obj, multiprocessing.managers.ListProxy)
        case.assertListEqual(list(obj), [5, 7, 5, 7])
        double_obj = obj * 2
        case.assertIsInstance(double_obj, list)
        case.assertListEqual(list(double_obj), [5, 7, 5, 7, 5, 7, 5, 7])
        double_obj = 2 * obj
        case.assertIsInstance(double_obj, list)
        case.assertListEqual(list(double_obj), [5, 7, 5, 7, 5, 7, 5, 7])
        copied_obj = obj.copy()
        case.assertIsInstance(copied_obj, list)
        case.assertListEqual(list(copied_obj), [5, 7, 5, 7])
        obj.extend(double_obj + copied_obj)
        obj.sort()
        obj.reverse()
        for x in obj:
            pass
        case.assertEqual(len(obj), 16)
        case.assertEqual(obj.pop(0), 7)
        obj.clear()
        case.assertEqual(len(obj), 0)

    def test_list(self):
        o = self.manager.list()
        o.append(5)
        self.run_worker(self._test_list, o)
        self.assertIsNotNone(o)
        self.assertEqual(len(o), 0)

    @classmethod
    def _test_dict(cls, obj):
        case = unittest.TestCase()
        case.assertEqual(len(obj), 1)
        case.assertEqual(obj['foo'], 5)
        case.assertEqual(obj.get('foo'), 5)
        case.assertListEqual(list(obj.items()), [('foo', 5)])
        case.assertListEqual(list(obj.keys()), ['foo'])
        case.assertListEqual(list(obj.values()), [5])
        case.assertDictEqual(obj.copy(), {'foo': 5})
        obj |= {'bar': 6}
        case.assertIsInstance(obj, multiprocessing.managers.DictProxy)
        case.assertDictEqual(dict(obj), {'foo': 5, 'bar': 6})
        x = reversed(obj)
        case.assertIsInstance(x, type(iter([])))
        case.assertListEqual(list(x), ['bar', 'foo'])
        x = {'bar': 7, 'baz': 7} | obj
        case.assertIsInstance(x, dict)
        case.assertDictEqual(dict(x), {'foo': 5, 'bar': 6, 'baz': 7})
        x = obj | {'bar': 7, 'baz': 7}
        case.assertIsInstance(x, dict)
        case.assertDictEqual(dict(x), {'foo': 5, 'bar': 7, 'baz': 7})
        x = obj.fromkeys(['bar'], 6)
        case.assertIsInstance(x, dict)
        case.assertDictEqual(x, {'bar': 6})
        x = obj.popitem()
        case.assertIsInstance(x, tuple)
        case.assertTupleEqual(x, ('bar', 6))
        obj.setdefault('bar', 0)
        obj.update({'bar': 7})
        case.assertEqual(obj.pop('bar'), 7)
        obj.clear()
        case.assertEqual(len(obj), 0)

    def test_dict(self):
        o = self.manager.dict()
        o['foo'] = 5
        self.run_worker(self._test_dict, o)
        self.assertIsNotNone(o)
        self.assertEqual(len(o), 0)

    @classmethod
    def _test_value(cls, obj):
        case = unittest.TestCase()
        case.assertEqual(obj.value, 1)
        case.assertEqual(obj.get(), 1)
        obj.set(2)

    def test_value(self):
        o = self.manager.Value('i', 1)
        self.run_worker(self._test_value, o)
        self.assertEqual(o.value, 2)
        self.assertEqual(o.get(), 2)

    @classmethod
    def _test_array(cls, obj):
        case = unittest.TestCase()
        case.assertEqual(obj[0], 0)
        case.assertEqual(obj[1], 1)
        case.assertEqual(len(obj), 2)
        case.assertListEqual(list(obj), [0, 1])

    def test_array(self):
        o = self.manager.Array('i', [0, 1])
        self.run_worker(self._test_array, o)

    @classmethod
    def _test_namespace(cls, obj):
        case = unittest.TestCase()
        case.assertEqual(obj.x, 0)
        case.assertEqual(obj.y, 1)

    def test_namespace(self):
        o = self.manager.Namespace()
        o.x = 0
        o.y = 1
        self.run_worker(self._test_namespace, o)

    @classmethod
    def _test_set_operator_symbols(cls, obj):
        case = unittest.TestCase()
        obj.update(['a', 'b', 'c'])
        case.assertEqual(len(obj), 3)
        case.assertIn('a', obj)
        case.assertNotIn('d', obj)
        result = obj | {'d', 'e'}
        case.assertSetEqual(result, {'a', 'b', 'c', 'd', 'e'})
        result = {'d', 'e'} | obj
        case.assertSetEqual(result, {'a', 'b', 'c', 'd', 'e'})
        obj |= {'d', 'e'}
        case.assertSetEqual(obj, {'a', 'b', 'c', 'd', 'e'})
        case.assertIsInstance(obj, multiprocessing.managers.SetProxy)

        obj.clear()
        obj.update(['a', 'b', 'c'])
        result = {'a', 'b', 'd'} - obj
        case.assertSetEqual(result, {'d'})
        result = obj - {'a', 'b'}
        case.assertSetEqual(result, {'c'})
        obj -= {'a', 'b'}
        case.assertSetEqual(obj, {'c'})
        case.assertIsInstance(obj, multiprocessing.managers.SetProxy)

        obj.clear()
        obj.update(['a', 'b', 'c'])
        result = {'b', 'c', 'd'} ^ obj
        case.assertSetEqual(result, {'a', 'd'})
        result = obj ^ {'b', 'c', 'd'}
        case.assertSetEqual(result, {'a', 'd'})
        obj ^= {'b', 'c', 'd'}
        case.assertSetEqual(obj, {'a', 'd'})
        case.assertIsInstance(obj, multiprocessing.managers.SetProxy)

        obj.clear()
        obj.update(['a', 'b', 'c'])
        result = obj & {'b', 'c', 'd'}
        case.assertSetEqual(result, {'b', 'c'})
        result = {'b', 'c', 'd'} & obj
        case.assertSetEqual(result, {'b', 'c'})
        obj &= {'b', 'c', 'd'}
        case.assertSetEqual(obj, {'b', 'c'})
        case.assertIsInstance(obj, multiprocessing.managers.SetProxy)

        obj.clear()
        obj.update(['a', 'b', 'c'])
        case.assertSetEqual(set(obj), {'a', 'b', 'c'})

    @classmethod
    def _test_set_operator_methods(cls, obj):
        case = unittest.TestCase()
        obj.add('d')
        case.assertIn('d', obj)

        obj.clear()
        obj.update(['a', 'b', 'c'])
        copy_obj = obj.copy()
        case.assertSetEqual(copy_obj, obj)
        obj.remove('a')
        case.assertNotIn('a', obj)
        case.assertRaises(KeyError, obj.remove, 'a')

        obj.clear()
        obj.update(['a'])
        obj.discard('a')
        case.assertNotIn('a', obj)
        obj.discard('a')
        case.assertNotIn('a', obj)
        obj.update(['a'])
        popped = obj.pop()
        case.assertNotIn(popped, obj)

        obj.clear()
        obj.update(['a', 'b', 'c'])
        result = obj.intersection({'b', 'c', 'd'})
        case.assertSetEqual(result, {'b', 'c'})
        obj.intersection_update({'b', 'c', 'd'})
        case.assertSetEqual(obj, {'b', 'c'})

        obj.clear()
        obj.update(['a', 'b', 'c'])
        result = obj.difference({'a', 'b'})
        case.assertSetEqual(result, {'c'})
        obj.difference_update({'a', 'b'})
        case.assertSetEqual(obj, {'c'})

        obj.clear()
        obj.update(['a', 'b', 'c'])
        result = obj.symmetric_difference({'b', 'c', 'd'})
        case.assertSetEqual(result, {'a', 'd'})
        obj.symmetric_difference_update({'b', 'c', 'd'})
        case.assertSetEqual(obj, {'a', 'd'})

    @classmethod
    def _test_set_comparisons(cls, obj):
        case = unittest.TestCase()
        obj.update(['a', 'b', 'c'])
        result = obj.union({'d', 'e'})
        case.assertSetEqual(result, {'a', 'b', 'c', 'd', 'e'})
        case.assertTrue(obj.isdisjoint({'d', 'e'}))
        case.assertFalse(obj.isdisjoint({'a', 'd'}))

        case.assertTrue(obj.issubset({'a', 'b', 'c', 'd'}))
        case.assertFalse(obj.issubset({'a', 'b'}))
        case.assertLess(obj, {'a', 'b', 'c', 'd'})
        case.assertLessEqual(obj, {'a', 'b', 'c'})

        case.assertTrue(obj.issuperset({'a', 'b'}))
        case.assertFalse(obj.issuperset({'a', 'b', 'd'}))
        case.assertGreater(obj, {'a'})
        case.assertGreaterEqual(obj, {'a', 'b'})

    def test_set(self):
        o = self.manager.set()
        self.run_worker(self._test_set_operator_symbols, o)
        o = self.manager.set()
        self.run_worker(self._test_set_operator_methods, o)
        o = self.manager.set()
        self.run_worker(self._test_set_comparisons, o)

    def test_set_init(self):
        o = self.manager.set({'a', 'b', 'c'})
        self.assertSetEqual(o, {'a', 'b', 'c'})
        o = self.manager.set(["a", "b", "c"])
        self.assertSetEqual(o, {"a", "b", "c"})
        o = self.manager.set({"a": 1, "b": 2, "c": 3})
        self.assertSetEqual(o, {"a", "b", "c"})
        self.assertRaises(RemoteError, self.manager.set, 1234)

    def test_set_contain_all_method(self):
        o = self.manager.set()
        set_methods = {
            '__and__', '__class_getitem__', '__contains__', '__iand__', '__ior__',
            '__isub__', '__iter__', '__ixor__', '__len__', '__or__', '__rand__',
            '__ror__', '__rsub__', '__rxor__', '__sub__', '__xor__',
            '__ge__', '__gt__', '__le__', '__lt__',
            'add', 'clear', 'copy', 'difference', 'difference_update', 'discard',
            'intersection', 'intersection_update', 'isdisjoint', 'issubset',
            'issuperset', 'pop', 'remove', 'symmetric_difference',
            'symmetric_difference_update', 'union', 'update',
        }
        self.assertLessEqual(set_methods, set(dir(o)))


class TestNamedResource(unittest.TestCase):
    @only_run_in_spawn_testsuite("spawn specific test.")
    def test_global_named_resource_spawn(self):
        #
        # gh-90549: Check that global named resources in main module
        # will not leak by a subprocess, in spawn context.
        #
        testfn = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, testfn)
        with open(testfn, 'w', encoding='utf-8') as f:
            f.write(textwrap.dedent('''\
                import multiprocessing as mp
                ctx = mp.get_context('spawn')
                global_resource = ctx.Semaphore()
                def submain(): pass
                if __name__ == '__main__':
                    p = ctx.Process(target=submain)
                    p.start()
                    p.join()
            '''))
        rc, out, err = script_helper.assert_python_ok(testfn)
        # on error, err = 'UserWarning: resource_tracker: There appear to
        # be 1 leaked semaphore objects to clean up at shutdown'
        self.assertFalse(err, msg=err.decode('utf-8'))


class _TestAtExit(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    @classmethod
    def _write_file_at_exit(self, output_path):
        import atexit
        def exit_handler():
            with open(output_path, 'w') as f:
                f.write("deadbeef")
        atexit.register(exit_handler)

    def test_atexit(self):
        # gh-83856
        with os_helper.temp_dir() as temp_dir:
            output_path = os.path.join(temp_dir, 'output.txt')
            p = self.Process(target=self._write_file_at_exit, args=(output_path,))
            p.start()
            p.join()
            with open(output_path) as f:
                self.assertEqual(f.read(), 'deadbeef')


class _TestSpawnedSysPath(BaseTestCase):
    """Test that sys.path is setup in forkserver and spawn processes."""

    ALLOWED_TYPES = {'processes'}
    # Not applicable to fork which inherits everything from the process as is.
    START_METHODS = {"forkserver", "spawn"}

    def setUp(self):
        self._orig_sys_path = list(sys.path)
        self._temp_dir = tempfile.mkdtemp(prefix="test_sys_path-")
        self._mod_name = "unique_test_mod"
        module_path = os.path.join(self._temp_dir, f"{self._mod_name}.py")
        with open(module_path, "w", encoding="utf-8") as mod:
            mod.write("# A simple test module\n")
        sys.path[:] = [p for p in sys.path if p]  # remove any existing ""s
        sys.path.insert(0, self._temp_dir)
        sys.path.insert(0, "")  # Replaced with an abspath in child.
        self.assertIn(self.start_method, self.START_METHODS)
        self._ctx = multiprocessing.get_context(self.start_method)

    def tearDown(self):
        sys.path[:] = self._orig_sys_path
        shutil.rmtree(self._temp_dir, ignore_errors=True)

    @staticmethod
    def enq_imported_module_names(queue):
        queue.put(tuple(sys.modules))

    def test_forkserver_preload_imports_sys_path(self):
        if self._ctx.get_start_method() != "forkserver":
            self.skipTest("forkserver specific test.")
        self.assertNotIn(self._mod_name, sys.modules)
        multiprocessing.forkserver._forkserver._stop()  # Must be fresh.
        self._ctx.set_forkserver_preload(
            ["test.test_multiprocessing_forkserver", self._mod_name])
        q = self._ctx.Queue()
        proc = self._ctx.Process(
                target=self.enq_imported_module_names, args=(q,))
        proc.start()
        proc.join()
        child_imported_modules = q.get()
        q.close()
        self.assertIn(self._mod_name, child_imported_modules)

    @staticmethod
    def enq_sys_path_and_import(queue, mod_name):
        queue.put(sys.path)
        try:
            importlib.import_module(mod_name)
        except ImportError as exc:
            queue.put(exc)
        else:
            queue.put(None)

    def test_child_sys_path(self):
        q = self._ctx.Queue()
        proc = self._ctx.Process(
                target=self.enq_sys_path_and_import, args=(q, self._mod_name))
        proc.start()
        proc.join()
        child_sys_path = q.get()
        import_error = q.get()
        q.close()
        self.assertNotIn("", child_sys_path)  # replaced by an abspath
        self.assertIn(self._temp_dir, child_sys_path)  # our addition
        # ignore the first element, it is the absolute "" replacement
        self.assertEqual(child_sys_path[1:], sys.path[1:])
        self.assertIsNone(import_error, msg=f"child could not import {self._mod_name}")


class MiscTestCase(unittest.TestCase):
    def test__all__(self):
        # Just make sure names in not_exported are excluded
        support.check__all__(self, multiprocessing, extra=multiprocessing.__all__,
                             not_exported=['SUBDEBUG', 'SUBWARNING'])

    @only_run_in_spawn_testsuite("avoids redundant testing.")
    def test_spawn_sys_executable_none_allows_import(self):
        # Regression test for a bug introduced in
        # https://github.com/python/cpython/issues/90876 that caused an
        # ImportError in multiprocessing when sys.executable was None.
        # This can be true in embedded environments.
        rc, out, err = script_helper.assert_python_ok(
            "-c",
            """if 1:
            import sys
            sys.executable = None
            assert "multiprocessing" not in sys.modules, "already imported!"
            import multiprocessing
            import multiprocessing.spawn  # This should not fail\n""",
        )
        self.assertEqual(rc, 0)
        self.assertFalse(err, msg=err.decode('utf-8'))

    def test_large_pool(self):
        #
        # gh-89240: Check that large pools are always okay
        #
        testfn = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, testfn)
        with open(testfn, 'w', encoding='utf-8') as f:
            f.write(textwrap.dedent('''\
                import multiprocessing
                def f(x): return x*x
                if __name__ == '__main__':
                    with multiprocessing.Pool(200) as p:
                        print(sum(p.map(f, range(1000))))
            '''))
        rc, out, err = script_helper.assert_python_ok(testfn)
        self.assertEqual("332833500", out.decode('utf-8').strip())
        self.assertFalse(err, msg=err.decode('utf-8'))


#
# Mixins
#

class BaseMixin(object):
    @classmethod
    def setUpClass(cls):
        cls.dangling = (multiprocessing.process._dangling.copy(),
                        threading._dangling.copy())

    @classmethod
    def tearDownClass(cls):
        # bpo-26762: Some multiprocessing objects like Pool create reference
        # cycles. Trigger a garbage collection to break these cycles.
        test.support.gc_collect()

        processes = set(multiprocessing.process._dangling) - set(cls.dangling[0])
        if processes:
            test.support.environment_altered = True
            support.print_warning(f'Dangling processes: {processes}')
        processes = None

        threads = set(threading._dangling) - set(cls.dangling[1])
        if threads:
            test.support.environment_altered = True
            support.print_warning(f'Dangling threads: {threads}')
        threads = None


class ProcessesMixin(BaseMixin):
    TYPE = 'processes'
    Process = multiprocessing.Process
    connection = multiprocessing.connection
    current_process = staticmethod(multiprocessing.current_process)
    parent_process = staticmethod(multiprocessing.parent_process)
    active_children = staticmethod(multiprocessing.active_children)
    set_executable = staticmethod(multiprocessing.set_executable)
    Pool = staticmethod(multiprocessing.Pool)
    Pipe = staticmethod(multiprocessing.Pipe)
    Queue = staticmethod(multiprocessing.Queue)
    JoinableQueue = staticmethod(multiprocessing.JoinableQueue)
    Lock = staticmethod(multiprocessing.Lock)
    RLock = staticmethod(multiprocessing.RLock)
    Semaphore = staticmethod(multiprocessing.Semaphore)
    BoundedSemaphore = staticmethod(multiprocessing.BoundedSemaphore)
    Condition = staticmethod(multiprocessing.Condition)
    Event = staticmethod(multiprocessing.Event)
    Barrier = staticmethod(multiprocessing.Barrier)
    Value = staticmethod(multiprocessing.Value)
    Array = staticmethod(multiprocessing.Array)
    RawValue = staticmethod(multiprocessing.RawValue)
    RawArray = staticmethod(multiprocessing.RawArray)


class ManagerMixin(BaseMixin):
    TYPE = 'manager'
    Process = multiprocessing.Process
    Queue = property(operator.attrgetter('manager.Queue'))
    JoinableQueue = property(operator.attrgetter('manager.JoinableQueue'))
    Lock = property(operator.attrgetter('manager.Lock'))
    RLock = property(operator.attrgetter('manager.RLock'))
    Semaphore = property(operator.attrgetter('manager.Semaphore'))
    BoundedSemaphore = property(operator.attrgetter('manager.BoundedSemaphore'))
    Condition = property(operator.attrgetter('manager.Condition'))
    Event = property(operator.attrgetter('manager.Event'))
    Barrier = property(operator.attrgetter('manager.Barrier'))
    Value = property(operator.attrgetter('manager.Value'))
    Array = property(operator.attrgetter('manager.Array'))
    list = property(operator.attrgetter('manager.list'))
    dict = property(operator.attrgetter('manager.dict'))
    Namespace = property(operator.attrgetter('manager.Namespace'))

    @classmethod
    def Pool(cls, *args, **kwds):
        return cls.manager.Pool(*args, **kwds)

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.manager = multiprocessing.Manager()

    @classmethod
    def tearDownClass(cls):
        # only the manager process should be returned by active_children()
        # but this can take a bit on slow machines, so wait a few seconds
        # if there are other children too (see #17395)
        timeout = WAIT_ACTIVE_CHILDREN_TIMEOUT
        start_time = time.monotonic()
        for _ in support.sleeping_retry(timeout, error=False):
            if len(multiprocessing.active_children()) <= 1:
                break
        else:
            dt = time.monotonic() - start_time
            support.environment_altered = True
            support.print_warning(f"multiprocessing.Manager still has "
                                  f"{multiprocessing.active_children()} "
                                  f"active children after {dt:.1f} seconds")

        gc.collect()                       # do garbage collection
        if cls.manager._number_of_objects() != 0:
            # This is not really an error since some tests do not
            # ensure that all processes which hold a reference to a
            # managed object have been joined.
            test.support.environment_altered = True
            support.print_warning('Shared objects which still exist '
                                  'at manager shutdown:')
            support.print_warning(cls.manager._debug_info())
        cls.manager.shutdown()
        cls.manager.join()
        cls.manager = None

        super().tearDownClass()


class ThreadsMixin(BaseMixin):
    TYPE = 'threads'
    Process = multiprocessing.dummy.Process
    connection = multiprocessing.dummy.connection
    current_process = staticmethod(multiprocessing.dummy.current_process)
    active_children = staticmethod(multiprocessing.dummy.active_children)
    Pool = staticmethod(multiprocessing.dummy.Pool)
    Pipe = staticmethod(multiprocessing.dummy.Pipe)
    Queue = staticmethod(multiprocessing.dummy.Queue)
    JoinableQueue = staticmethod(multiprocessing.dummy.JoinableQueue)
    Lock = staticmethod(multiprocessing.dummy.Lock)
    RLock = staticmethod(multiprocessing.dummy.RLock)
    Semaphore = staticmethod(multiprocessing.dummy.Semaphore)
    BoundedSemaphore = staticmethod(multiprocessing.dummy.BoundedSemaphore)
    Condition = staticmethod(multiprocessing.dummy.Condition)
    Event = staticmethod(multiprocessing.dummy.Event)
    Barrier = staticmethod(multiprocessing.dummy.Barrier)
    Value = staticmethod(multiprocessing.dummy.Value)
    Array = staticmethod(multiprocessing.dummy.Array)

#
# Functions used to create test cases from the base ones in this module
#

def install_tests_in_module_dict(remote_globs, start_method,
                                 only_type=None, exclude_types=False):
    __module__ = remote_globs['__name__']
    local_globs = globals()
    ALL_TYPES = {'processes', 'threads', 'manager'}

    for name, base in local_globs.items():
        if not isinstance(base, type):
            continue
        if issubclass(base, BaseTestCase):
            if base is BaseTestCase:
                continue
            assert set(base.ALLOWED_TYPES) <= ALL_TYPES, base.ALLOWED_TYPES
            if base.START_METHODS and start_method not in base.START_METHODS:
                continue  # class not intended for this start method.
            for type_ in base.ALLOWED_TYPES:
                if only_type and type_ != only_type:
                    continue
                if exclude_types:
                    continue
                newname = 'With' + type_.capitalize() + name[1:]
                Mixin = local_globs[type_.capitalize() + 'Mixin']
                class Temp(base, Mixin, unittest.TestCase):
                    pass
                if type_ == 'manager':
                    Temp = hashlib_helper.requires_hashdigest('sha256')(Temp)
                Temp.__name__ = Temp.__qualname__ = newname
                Temp.__module__ = __module__
                Temp.start_method = start_method
                remote_globs[newname] = Temp
        elif issubclass(base, unittest.TestCase):
            if only_type:
                continue

            class Temp(base, object):
                pass
            Temp.__name__ = Temp.__qualname__ = name
            Temp.__module__ = __module__
            remote_globs[name] = Temp

    dangling = [None, None]
    old_start_method = [None]

    def setUpModule():
        multiprocessing.set_forkserver_preload(PRELOAD)
        multiprocessing.process._cleanup()
        dangling[0] = multiprocessing.process._dangling.copy()
        dangling[1] = threading._dangling.copy()
        old_start_method[0] = multiprocessing.get_start_method(allow_none=True)
        try:
            multiprocessing.set_start_method(start_method, force=True)
        except ValueError:
            raise unittest.SkipTest(start_method +
                                    ' start method not supported')

        if sys.platform.startswith("linux"):
            try:
                lock = multiprocessing.RLock()
            except OSError:
                raise unittest.SkipTest("OSError raises on RLock creation, "
                                        "see issue 3111!")
        check_enough_semaphores()
        util.get_temp_dir()     # creates temp directory
        multiprocessing.get_logger().setLevel(LOG_LEVEL)

    def tearDownModule():
        need_sleep = False

        # bpo-26762: Some multiprocessing objects like Pool create reference
        # cycles. Trigger a garbage collection to break these cycles.
        test.support.gc_collect()

        multiprocessing.set_start_method(old_start_method[0], force=True)
        # pause a bit so we don't get warning about dangling threads/processes
        processes = set(multiprocessing.process._dangling) - set(dangling[0])
        if processes:
            need_sleep = True
            test.support.environment_altered = True
            support.print_warning(f'Dangling processes: {processes}')
        processes = None

        threads = set(threading._dangling) - set(dangling[1])
        if threads:
            need_sleep = True
            test.support.environment_altered = True
            support.print_warning(f'Dangling threads: {threads}')
        threads = None

        # Sleep 500 ms to give time to child processes to complete.
        if need_sleep:
            time.sleep(0.5)

        multiprocessing.util._cleanup_tests()

    remote_globs['setUpModule'] = setUpModule
    remote_globs['tearDownModule'] = tearDownModule


@unittest.skipIf(not hasattr(_multiprocessing, 'SemLock'), 'SemLock not available')
@unittest.skipIf(sys.platform != "linux", "Linux only")
class SemLockTests(unittest.TestCase):

    def test_semlock_subclass(self):
        class SemLock(_multiprocessing.SemLock):
            pass
        name = f'test_semlock_subclass-{os.getpid()}'
        s = SemLock(1, 0, 10, name, False)
        _multiprocessing.sem_unlink(name)
