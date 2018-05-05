#
# Unit tests for the multiprocessing package
#

import unittest
import Queue
import time
import sys
import os
import gc
import signal
import array
import socket
import random
import logging
import errno
import weakref
import test.script_helper
from test import test_support
from StringIO import StringIO
_multiprocessing = test_support.import_module('_multiprocessing')
# import threading after _multiprocessing to raise a more relevant error
# message: "No module named _multiprocessing". _multiprocessing is not compiled
# without thread support.
import threading

# Work around broken sem_open implementations
test_support.import_module('multiprocessing.synchronize')

import multiprocessing.dummy
import multiprocessing.connection
import multiprocessing.managers
import multiprocessing.heap
import multiprocessing.pool

from multiprocessing import util

try:
    from multiprocessing import reduction
    HAS_REDUCTION = True
except ImportError:
    HAS_REDUCTION = False

try:
    from multiprocessing.sharedctypes import Value, copy
    HAS_SHAREDCTYPES = True
except ImportError:
    HAS_SHAREDCTYPES = False

try:
    import msvcrt
except ImportError:
    msvcrt = None

#
#
#

latin = str

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

HAVE_GETVALUE = not getattr(_multiprocessing,
                            'HAVE_BROKEN_SEM_GETVALUE', False)

WIN32 = (sys.platform == "win32")

try:
    MAXFD = os.sysconf("SC_OPEN_MAX")
except:
    MAXFD = 256

#
# Some tests require ctypes
#

try:
    from ctypes import Structure, c_int, c_double
except ImportError:
    Structure = object
    c_int = c_double = None


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


#
# Creates a wrapper for a function which records the time it takes to finish
#

class TimingWrapper(object):

    def __init__(self, func):
        self.func = func
        self.elapsed = None

    def __call__(self, *args, **kwds):
        t = time.time()
        try:
            return self.func(*args, **kwds)
        finally:
            self.elapsed = time.time() - t

#
# Base class for test cases
#

class BaseTestCase(object):

    ALLOWED_TYPES = ('processes', 'manager', 'threads')

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

class DummyCallable(object):
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
        self.assertTrue(not current.daemon)
        self.assertIsInstance(authkey, bytes)
        self.assertTrue(len(authkey) > 0)
        self.assertEqual(current.ident, os.getpid())
        self.assertEqual(current.exitcode, None)

    @classmethod
    def _test(cls, q, *args, **kwds):
        current = cls.current_process()
        q.put(args)
        q.put(kwds)
        q.put(current.name)
        if cls.TYPE != 'threads':
            q.put(bytes(current.authkey))
            q.put(current.pid)

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
        self.assertTrue(type(self.active_children()) is list)
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

    @classmethod
    def _test_terminate(cls):
        time.sleep(1000)

    def test_terminate(self):
        if self.TYPE == 'threads':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        p = self.Process(target=self._test_terminate)
        p.daemon = True
        p.start()

        self.assertEqual(p.is_alive(), True)
        self.assertIn(p, self.active_children())
        self.assertEqual(p.exitcode, None)

        p.terminate()

        join = TimingWrapper(p.join)
        self.assertEqual(join(), None)
        self.assertTimingAlmostEqual(join.elapsed, 0.0)

        self.assertEqual(p.is_alive(), False)
        self.assertNotIn(p, self.active_children())

        p.join()

        # XXX sometimes get p.exitcode == 0 on Windows ...
        #self.assertEqual(p.exitcode, -signal.SIGTERM)

    def test_cpu_count(self):
        try:
            cpus = multiprocessing.cpu_count()
        except NotImplementedError:
            cpus = 1
        self.assertTrue(type(cpus) is int)
        self.assertTrue(cpus >= 1)

    def test_active_children(self):
        self.assertEqual(type(self.active_children()), list)

        p = self.Process(target=time.sleep, args=(DELTA,))
        self.assertNotIn(p, self.active_children())

        p.daemon = True
        p.start()
        self.assertIn(p, self.active_children())

        p.join()
        self.assertNotIn(p, self.active_children())

    @classmethod
    def _test_recursion(cls, wconn, id):
        from multiprocessing import forking
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
    def _test_sys_exit(cls, reason, testfn):
        sys.stderr = open(testfn, 'w')
        sys.exit(reason)

    def test_sys_exit(self):
        # See Issue 13854
        if self.TYPE == 'threads':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        testfn = test_support.TESTFN
        self.addCleanup(test_support.unlink, testfn)

        for reason, code in (([1, 2, 3], 1), ('ignore this', 1)):
            p = self.Process(target=self._test_sys_exit, args=(reason, testfn))
            p.daemon = True
            p.start()
            p.join(5)
            self.assertEqual(p.exitcode, code)

            with open(testfn, 'r') as f:
                self.assertEqual(f.read().rstrip(), str(reason))

        for reason in (True, False, 8):
            p = self.Process(target=sys.exit, args=(reason,))
            p.daemon = True
            p.start()
            p.join(5)
            self.assertEqual(p.exitcode, reason)

    def test_lose_target_ref(self):
        c = DummyCallable()
        wr = weakref.ref(c)
        q = self.Queue()
        p = self.Process(target=c, args=(q, c))
        del c
        p.start()
        p.join()
        self.assertIs(wr(), None)
        self.assertEqual(q.get(), 5)


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

        self.assertRaises(Queue.Full, put, 7, False)
        self.assertTimingAlmostEqual(put.elapsed, 0)

        self.assertRaises(Queue.Full, put, 7, False, None)
        self.assertTimingAlmostEqual(put.elapsed, 0)

        self.assertRaises(Queue.Full, put_nowait, 7)
        self.assertTimingAlmostEqual(put_nowait.elapsed, 0)

        self.assertRaises(Queue.Full, put, 7, True, TIMEOUT1)
        self.assertTimingAlmostEqual(put.elapsed, TIMEOUT1)

        self.assertRaises(Queue.Full, put, 7, False, TIMEOUT2)
        self.assertTimingAlmostEqual(put.elapsed, 0)

        self.assertRaises(Queue.Full, put, 7, True, timeout=TIMEOUT3)
        self.assertTimingAlmostEqual(put.elapsed, TIMEOUT3)

        child_can_start.set()
        parent_can_continue.wait()

        self.assertEqual(queue_empty(queue), True)
        self.assertEqual(queue_full(queue, MAXSIZE), False)

        proc.join()

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

        self.assertRaises(Queue.Empty, get, False)
        self.assertTimingAlmostEqual(get.elapsed, 0)

        self.assertRaises(Queue.Empty, get, False, None)
        self.assertTimingAlmostEqual(get.elapsed, 0)

        self.assertRaises(Queue.Empty, get_nowait)
        self.assertTimingAlmostEqual(get_nowait.elapsed, 0)

        self.assertRaises(Queue.Empty, get, True, TIMEOUT1)
        self.assertTimingAlmostEqual(get.elapsed, TIMEOUT1)

        self.assertRaises(Queue.Empty, get, False, TIMEOUT2)
        self.assertTimingAlmostEqual(get.elapsed, 0)

        self.assertRaises(Queue.Empty, get, timeout=TIMEOUT3)
        self.assertTimingAlmostEqual(get.elapsed, TIMEOUT3)

        proc.join()

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
        self.assertRaises(Queue.Empty, queue.get, False)

        p.join()

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

    @classmethod
    def _test_task_done(cls, q):
        for obj in iter(q.get, None):
            time.sleep(DELTA)
            q.task_done()

    def test_task_done(self):
        queue = self.JoinableQueue()

        if sys.version_info < (2, 5) and not hasattr(queue, 'task_done'):
            self.skipTest("requires 'queue.task_done()' method")

        workers = [self.Process(target=self._test_task_done, args=(queue,))
                   for i in xrange(4)]

        for p in workers:
            p.daemon = True
            p.start()

        for i in xrange(10):
            queue.put(i)

        queue.join()

        for p in workers:
            queue.put(None)

        for p in workers:
            p.join()

    def test_no_import_lock_contention(self):
        with test_support.temp_cwd():
            module_name = 'imported_by_an_imported_module'
            with open(module_name + '.py', 'w') as f:
                f.write("""if 1:
                    import multiprocessing

                    q = multiprocessing.Queue()
                    q.put('knock knock')
                    q.get(timeout=3)
                    q.close()
                """)

            with test_support.DirsOnSysPath(os.getcwd()):
                try:
                    __import__(module_name)
                except Queue.Empty:
                    self.fail("Probable regression on import lock contention;"
                              " see Issue #22853")

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
            # bpo-30595: use a timeout of 1 second for slow buildbots
            self.assertTrue(q.get(timeout=1.0))


#
#
#

class _TestLock(BaseTestCase):

    def test_lock(self):
        lock = self.Lock()
        self.assertEqual(lock.acquire(), True)
        self.assertEqual(lock.acquire(False), False)
        self.assertEqual(lock.release(), None)
        self.assertRaises((ValueError, threading.ThreadError), lock.release)

    def test_rlock(self):
        lock = self.RLock()
        self.assertEqual(lock.acquire(), True)
        self.assertEqual(lock.acquire(), True)
        self.assertEqual(lock.acquire(), True)
        self.assertEqual(lock.release(), None)
        self.assertEqual(lock.release(), None)
        self.assertEqual(lock.release(), None)
        self.assertRaises((AssertionError, RuntimeError), lock.release)

    def test_lock_context(self):
        with self.Lock():
            pass


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

        p = threading.Thread(target=self.f, args=(cond, sleeping, woken))
        p.daemon = True
        p.start()

        # wait for both children to start sleeping
        sleeping.acquire()
        sleeping.acquire()

        # check no process/thread has woken up
        time.sleep(DELTA)
        self.assertReturnsIfImplemented(0, get_value, woken)

        # wake up one process/thread
        cond.acquire()
        cond.notify()
        cond.release()

        # check one process/thread has woken up
        time.sleep(DELTA)
        self.assertReturnsIfImplemented(1, get_value, woken)

        # wake up another
        cond.acquire()
        cond.notify()
        cond.release()

        # check other has woken up
        time.sleep(DELTA)
        self.assertReturnsIfImplemented(2, get_value, woken)

        # check state is not mucked up
        self.check_invariant(cond)
        p.join()

    def test_notify_all(self):
        cond = self.Condition()
        sleeping = self.Semaphore(0)
        woken = self.Semaphore(0)

        # start some threads/processes which will timeout
        for i in range(3):
            p = self.Process(target=self.f,
                             args=(cond, sleeping, woken, TIMEOUT1))
            p.daemon = True
            p.start()

            t = threading.Thread(target=self.f,
                                 args=(cond, sleeping, woken, TIMEOUT1))
            t.daemon = True
            t.start()

        # wait for them all to sleep
        for i in xrange(6):
            sleeping.acquire()

        # check they have all timed out
        for i in xrange(6):
            woken.acquire()
        self.assertReturnsIfImplemented(0, get_value, woken)

        # check state is not mucked up
        self.check_invariant(cond)

        # start some more threads/processes
        for i in range(3):
            p = self.Process(target=self.f, args=(cond, sleeping, woken))
            p.daemon = True
            p.start()

            t = threading.Thread(target=self.f, args=(cond, sleeping, woken))
            t.daemon = True
            t.start()

        # wait for them to all sleep
        for i in xrange(6):
            sleeping.acquire()

        # check no process/thread has woken up
        time.sleep(DELTA)
        self.assertReturnsIfImplemented(0, get_value, woken)

        # wake them all up
        cond.acquire()
        cond.notify_all()
        cond.release()

        # check they have all woken
        for i in range(10):
            try:
                if get_value(woken) == 6:
                    break
            except NotImplementedError:
                break
            time.sleep(DELTA)
        self.assertReturnsIfImplemented(6, get_value, woken)

        # check state is not mucked up
        self.check_invariant(cond)

    def test_timeout(self):
        cond = self.Condition()
        wait = TimingWrapper(cond.wait)
        cond.acquire()
        res = wait(TIMEOUT1)
        cond.release()
        self.assertEqual(res, None)
        self.assertTimingAlmostEqual(wait.elapsed, TIMEOUT1)


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

#
#
#

class _TestValue(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    codes_values = [
        ('i', 4343, 24234),
        ('d', 3.625, -4.25),
        ('h', -232, 234),
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
        self.assertFalse(hasattr(arr4, 'get_lock'))
        self.assertFalse(hasattr(arr4, 'get_obj'))

        self.assertRaises(AttributeError, self.Value, 'i', 5, lock='navalue')

        arr5 = self.RawValue('i', 5)
        self.assertFalse(hasattr(arr5, 'get_lock'))
        self.assertFalse(hasattr(arr5, 'get_obj'))


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
            self.assertEqual(list(arr), range(10))
            del arr

    @unittest.skipIf(c_int is None, "requires _ctypes")
    def test_rawarray(self):
        self.test_array(raw=True)

    @unittest.skipIf(c_int is None, "requires _ctypes")
    def test_array_accepts_long(self):
        arr = self.Array('i', 10L)
        self.assertEqual(len(arr), 10)
        raw_arr = self.RawArray('i', 10L)
        self.assertEqual(len(raw_arr), 10)

    @unittest.skipIf(c_int is None, "requires _ctypes")
    def test_getobj_getlock_obj(self):
        arr1 = self.Array('i', range(10))
        lock1 = arr1.get_lock()
        obj1 = arr1.get_obj()

        arr2 = self.Array('i', range(10), lock=None)
        lock2 = arr2.get_lock()
        obj2 = arr2.get_obj()

        lock = self.Lock()
        arr3 = self.Array('i', range(10), lock=lock)
        lock3 = arr3.get_lock()
        obj3 = arr3.get_obj()
        self.assertEqual(lock, lock3)

        arr4 = self.Array('i', range(10), lock=False)
        self.assertFalse(hasattr(arr4, 'get_lock'))
        self.assertFalse(hasattr(arr4, 'get_obj'))
        self.assertRaises(AttributeError,
                          self.Array, 'i', range(10), lock='notalock')

        arr5 = self.RawArray('i', range(10))
        self.assertFalse(hasattr(arr5, 'get_lock'))
        self.assertFalse(hasattr(arr5, 'get_obj'))

#
#
#

class _TestContainers(BaseTestCase):

    ALLOWED_TYPES = ('manager',)

    def test_list(self):
        a = self.list(range(10))
        self.assertEqual(a[:], range(10))

        b = self.list()
        self.assertEqual(b[:], [])

        b.extend(range(5))
        self.assertEqual(b[:], range(5))

        self.assertEqual(b[2], 2)
        self.assertEqual(b[2:10], [2,3,4])

        b *= 2
        self.assertEqual(b[:], [0, 1, 2, 3, 4, 0, 1, 2, 3, 4])

        self.assertEqual(b + [5, 6], [0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 5, 6])

        self.assertEqual(a[:], range(10))

        d = [a, b]
        e = self.list(d)
        self.assertEqual(
            e[:],
            [[0, 1, 2, 3, 4, 5, 6, 7, 8, 9], [0, 1, 2, 3, 4, 0, 1, 2, 3, 4]]
            )

        f = self.list([a])
        a.append('hello')
        self.assertEqual(f[:], [[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 'hello']])

    def test_dict(self):
        d = self.dict()
        indices = range(65, 70)
        for i in indices:
            d[i] = chr(i)
        self.assertEqual(d.copy(), dict((i, chr(i)) for i in indices))
        self.assertEqual(sorted(d.keys()), indices)
        self.assertEqual(sorted(d.values()), [chr(i) for i in indices])
        self.assertEqual(sorted(d.items()), [(i, chr(i)) for i in indices])

    def test_namespace(self):
        n = self.Namespace()
        n.name = 'Bob'
        n.job = 'Builder'
        n._hidden = 'hidden'
        self.assertEqual((n.name, n.job), ('Bob', 'Builder'))
        del n.job
        self.assertEqual(str(n), "Namespace(name='Bob')")
        self.assertTrue(hasattr(n, 'name'))
        self.assertTrue(not hasattr(n, 'job'))

#
#
#

def sqr(x, wait=0.0):
    time.sleep(wait)
    return x*x

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
    for i in range(total):
        if i == when:
            raise SayWhenError("Somebody said when")
        yield i

class _TestPool(BaseTestCase):

    def test_apply(self):
        papply = self.pool.apply
        self.assertEqual(papply(sqr, (5,)), sqr(5))
        self.assertEqual(papply(sqr, (), {'x':3}), sqr(x=3))

    def test_map(self):
        pmap = self.pool.map
        self.assertEqual(pmap(sqr, range(10)), map(sqr, range(10)))
        self.assertEqual(pmap(sqr, range(100), chunksize=20),
                         map(sqr, range(100)))

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

    def test_async(self):
        res = self.pool.apply_async(sqr, (7, TIMEOUT1,))
        get = TimingWrapper(res.get)
        self.assertEqual(get(), 49)
        self.assertTimingAlmostEqual(get.elapsed, TIMEOUT1)

    def test_async_timeout(self):
        res = self.pool.apply_async(sqr, (6, TIMEOUT2 + 1.0))
        get = TimingWrapper(res.get)
        self.assertRaises(multiprocessing.TimeoutError, get, timeout=TIMEOUT2)
        self.assertTimingAlmostEqual(get.elapsed, TIMEOUT2)

    def test_imap(self):
        it = self.pool.imap(sqr, range(10))
        self.assertEqual(list(it), map(sqr, range(10)))

        it = self.pool.imap(sqr, range(10))
        for i in range(10):
            self.assertEqual(it.next(), i*i)
        self.assertRaises(StopIteration, it.next)

        it = self.pool.imap(sqr, range(1000), chunksize=100)
        for i in range(1000):
            self.assertEqual(it.next(), i*i)
        self.assertRaises(StopIteration, it.next)

    def test_imap_handle_iterable_exception(self):
        if self.TYPE == 'manager':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        it = self.pool.imap(sqr, exception_throwing_generator(10, 3), 1)
        for i in range(3):
            self.assertEqual(next(it), i*i)
        self.assertRaises(SayWhenError, it.next)

        # SayWhenError seen at start of problematic chunk's results
        it = self.pool.imap(sqr, exception_throwing_generator(20, 7), 2)
        for i in range(6):
            self.assertEqual(next(it), i*i)
        self.assertRaises(SayWhenError, it.next)
        it = self.pool.imap(sqr, exception_throwing_generator(20, 7), 4)
        for i in range(4):
            self.assertEqual(next(it), i*i)
        self.assertRaises(SayWhenError, it.next)

    def test_imap_unordered(self):
        it = self.pool.imap_unordered(sqr, range(1000))
        self.assertEqual(sorted(it), map(sqr, range(1000)))

        it = self.pool.imap_unordered(sqr, range(1000), chunksize=53)
        self.assertEqual(sorted(it), map(sqr, range(1000)))

    def test_imap_unordered_handle_iterable_exception(self):
        if self.TYPE == 'manager':
            self.skipTest('test not appropriate for {}'.format(self.TYPE))

        it = self.pool.imap_unordered(sqr,
                                      exception_throwing_generator(10, 3),
                                      1)
        expected_values = map(sqr, range(10))
        with self.assertRaises(SayWhenError):
            # imap_unordered makes it difficult to anticipate the SayWhenError
            for i in range(10):
                value = next(it)
                self.assertIn(value, expected_values)
                expected_values.remove(value)

        it = self.pool.imap_unordered(sqr,
                                      exception_throwing_generator(20, 7),
                                      2)
        expected_values = map(sqr, range(20))
        with self.assertRaises(SayWhenError):
            for i in range(20):
                value = next(it)
                self.assertIn(value, expected_values)
                expected_values.remove(value)

    def test_make_pool(self):
        self.assertRaises(ValueError, multiprocessing.Pool, -1)
        self.assertRaises(ValueError, multiprocessing.Pool, 0)

        p = multiprocessing.Pool(3)
        self.assertEqual(3, len(p._pool))
        p.close()
        p.join()

    def test_terminate(self):
        p = self.Pool(4)
        result = p.map_async(
            time.sleep, [0.1 for i in range(10000)], chunksize=1
            )
        p.terminate()
        join = TimingWrapper(p.join)
        join()
        self.assertTrue(join.elapsed < 0.2)

    def test_empty_iterable(self):
        # See Issue 12157
        p = self.Pool(1)

        self.assertEqual(p.map(sqr, []), [])
        self.assertEqual(list(p.imap(sqr, [])), [])
        self.assertEqual(list(p.imap_unordered(sqr, [])), [])
        self.assertEqual(p.map_async(sqr, []).get(), [])

        p.close()
        p.join()

    def test_release_task_refs(self):
        # Issue #29861: task arguments and results should not be kept
        # alive after we are done with them.
        objs = list(CountedObject() for i in range(10))
        refs = list(weakref.ref(o) for o in objs)
        self.pool.map(identity, objs)

        del objs
        time.sleep(DELTA)  # let threaded cleanup code run
        self.assertEqual(set(wr() for wr in refs), {None})
        # With a process pool, copies of the objects are returned, check
        # they were released too.
        self.assertEqual(CountedObject.n_instances, 0)


def unpickleable_result():
    return lambda: 42

class _TestPoolWorkerErrors(BaseTestCase):
    ALLOWED_TYPES = ('processes', )

    def test_unpickleable_result(self):
        from multiprocessing.pool import MaybeEncodingError
        p = multiprocessing.Pool(2)

        # Make sure we don't lose pool processes because of encoding errors.
        for iteration in range(20):
            res = p.apply_async(unpickleable_result)
            self.assertRaises(MaybeEncodingError, res.get)

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


#
# Test that manager has expected number of shared objects left
#

class _TestZZZNumberOfObjects(BaseTestCase):
    # Because test cases are sorted alphabetically, this one will get
    # run after all the other tests for the manager.  It tests that
    # there have been no "reference leaks" for the manager's shared
    # objects.  Note the comment in _TestPool.test_terminate().
    ALLOWED_TYPES = ('manager',)

    def test_number_of_objects(self):
        EXPECTED_NUMBER = 1                # the pool object is still alive
        multiprocessing.active_children()  # discard dead process objs
        gc.collect()                       # do garbage collection
        refs = self.manager._number_of_objects()
        debug_info = self.manager._debug_info()
        if refs != EXPECTED_NUMBER:
            print self.manager._debug_info()
            print debug_info

        self.assertEqual(refs, EXPECTED_NUMBER)

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
    for i in xrange(10):
        yield i*i

class IteratorProxy(BaseProxy):
    _exposed_ = ('next', '__next__')
    def __iter__(self):
        return self
    def next(self):
        return self._callmethod('next')
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
        manager = MyManager()
        manager.start()

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

        manager.shutdown()

#
# Test of connecting to a remote server and using xmlrpclib for serialization
#

_queue = Queue.Queue()
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
              #'hall\xc3\xa5 v\xc3\xa4rlden'] # UTF-8
              ]
    result = values[:]
    if test_support.have_unicode:
        #result[-1] = u'hall\xe5 v\xe4rlden'
        uvalue = test_support.u(r'\u043f\u0440\u0438\u0432\u0456\u0442 '
                                r'\u0441\u0432\u0456\u0442')
        values.append(uvalue)
        result.append(uvalue)

    @classmethod
    def _putter(cls, address, authkey):
        manager = QueueManager2(
            address=address, authkey=authkey, serializer=SERIALIZER
            )
        manager.connect()
        queue = manager.get_queue()
        # Note that xmlrpclib will deserialize object as a list not a tuple
        queue.put(tuple(cls.values))

    def test_remote(self):
        authkey = os.urandom(32)

        manager = QueueManager(
            address=(test.test_support.HOST, 0), authkey=authkey, serializer=SERIALIZER
            )
        manager.start()

        p = self.Process(target=self._putter, args=(manager.address, authkey))
        p.daemon = True
        p.start()

        manager2 = QueueManager2(
            address=manager.address, authkey=authkey, serializer=SERIALIZER
            )
        manager2.connect()
        queue = manager2.get_queue()

        self.assertEqual(queue.get(), self.result)

        # Because we are using xmlrpclib for serialization instead of
        # pickle this will cause a serialization error.
        self.assertRaises(Exception, queue.put, time.sleep)

        # Make queue finalizer run before the server is stopped
        del queue
        manager.shutdown()

class _TestManagerRestart(BaseTestCase):

    @classmethod
    def _putter(cls, address, authkey):
        manager = QueueManager(
            address=address, authkey=authkey, serializer=SERIALIZER)
        manager.connect()
        queue = manager.get_queue()
        queue.put('hello world')

    def test_rapid_restart(self):
        authkey = os.urandom(32)
        manager = QueueManager(
            address=(test.test_support.HOST, 0), authkey=authkey, serializer=SERIALIZER)
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
        manager.shutdown()

        manager = QueueManager(
            address=addr, authkey=authkey, serializer=SERIALIZER)
        manager.start()
        manager.shutdown()

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
        arr = array.array('i', range(4))

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
            except multiprocessing.BufferTooShort, e:
                self.assertEqual(e.args, (longmsg,))
            else:
                self.fail('expected BufferTooShort, got %s' % res)

        poll = TimingWrapper(conn.poll)

        self.assertEqual(poll(), False)
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
            self.assertRaises(IOError, reader.send, 2)
            self.assertRaises(IOError, writer.recv)
            self.assertRaises(IOError, writer.poll)

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
        with open(test_support.TESTFN, "wb") as f:
            fd = f.fileno()
            if msvcrt:
                fd = msvcrt.get_osfhandle(fd)
            reduction.send_handle(conn, fd, p.pid)
        p.join()
        with open(test_support.TESTFN, "rb") as f:
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
        with open(test_support.TESTFN, "wb") as f:
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
        with open(test_support.TESTFN, "rb") as f:
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

#
# Test of sending connection and socket objects between processes
#
"""
class _TestPicklingConnections(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def _listener(self, conn, families):
        for fam in families:
            l = self.connection.Listener(family=fam)
            conn.send(l.address)
            new_conn = l.accept()
            conn.send(new_conn)

        if self.TYPE == 'processes':
            l = socket.socket()
            l.bind(('localhost', 0))
            conn.send(l.getsockname())
            l.listen(1)
            new_conn, addr = l.accept()
            conn.send(new_conn)

        conn.recv()

    def _remote(self, conn):
        for (address, msg) in iter(conn.recv, None):
            client = self.connection.Client(address)
            client.send(msg.upper())
            client.close()

        if self.TYPE == 'processes':
            address, msg = conn.recv()
            client = socket.socket()
            client.connect(address)
            client.sendall(msg.upper())
            client.close()

        conn.close()

    def test_pickling(self):
        try:
            multiprocessing.allow_connection_pickling()
        except ImportError:
            return

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

        if self.TYPE == 'processes':
            msg = latin('This connection uses a normal socket')
            address = lconn.recv()
            rconn.send((address, msg))
            if hasattr(socket, 'fromfd'):
                new_conn = lconn.recv()
                self.assertEqual(new_conn.recv(100), msg.upper())
            else:
                # XXX On Windows with Py2.6 need to backport fromfd()
                discard = lconn.recv_bytes()

        lconn.send(None)

        rconn.close()
        lconn.close()

        lp.join()
        rp.join()
"""
#
#
#

class _TestHeap(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def test_heap(self):
        iterations = 5000
        maxblocks = 50
        blocks = []

        # create and destroy lots of blocks of different sizes
        for i in xrange(iterations):
            size = int(random.lognormvariate(0, 1) * 1000)
            b = multiprocessing.heap.BufferWrapper(size)
            blocks.append(b)
            if len(blocks) > maxblocks:
                i = random.randrange(maxblocks)
                del blocks[i]

        # get the heap object
        heap = multiprocessing.heap.BufferWrapper._heap

        # verify the state of the heap
        all = []
        occupied = 0
        heap._lock.acquire()
        self.addCleanup(heap._lock.release)
        for L in heap._len_to_seq.values():
            for arena, start, stop in L:
                all.append((heap._arenas.index(arena), start, stop,
                            stop-start, 'free'))
        for arena, start, stop in heap._allocated_blocks:
            all.append((heap._arenas.index(arena), start, stop,
                        stop-start, 'occupied'))
            occupied += (stop-start)

        all.sort()

        for i in range(len(all)-1):
            (arena, start, stop) = all[i][:3]
            (narena, nstart, nstop) = all[i+1][:3]
            self.assertTrue((arena != narena and nstart == 0) or
                            (stop == nstart))

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
        ('y', c_double)
        ]

class _TestSharedCTypes(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def setUp(self):
        if not HAS_SHAREDCTYPES:
            self.skipTest("requires multiprocessing.sharedctypes")

    @classmethod
    def _double(cls, x, y, foo, arr, string):
        x.value *= 2
        y.value *= 2
        foo.x *= 2
        foo.y *= 2
        string.value *= 2
        for i in range(len(arr)):
            arr[i] *= 2

    def test_sharedctypes(self, lock=False):
        x = Value('i', 7, lock=lock)
        y = Value(c_double, 1.0/3.0, lock=lock)
        foo = Value(_Foo, 3, 2, lock=lock)
        arr = self.Array('d', range(10), lock=lock)
        string = self.Array('c', 20, lock=lock)
        string.value = latin('hello')

        p = self.Process(target=self._double, args=(x, y, foo, arr, string))
        p.daemon = True
        p.start()
        p.join()

        self.assertEqual(x.value, 14)
        self.assertAlmostEqual(y.value, 2.0/3.0)
        self.assertEqual(foo.x, 6)
        self.assertAlmostEqual(foo.y, 4.0)
        for i in range(10):
            self.assertAlmostEqual(arr[i], i*2)
        self.assertEqual(string.value, latin('hellohello'))

    def test_synchronize(self):
        self.test_sharedctypes(lock=True)

    def test_copy(self):
        foo = _Foo(2, 5.0)
        bar = copy(foo)
        foo.x = 0
        foo.y = 0
        self.assertEqual(bar.x, 2)
        self.assertAlmostEqual(bar.y, 5.0)

#
#
#

class _TestFinalize(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def setUp(self):
        self.registry_backup = util._finalizer_registry.copy()
        util._finalizer_registry.clear()

    def tearDown(self):
        self.assertFalse(util._finalizer_registry)
        util._finalizer_registry.update(self.registry_backup)

    @classmethod
    def _test_finalize(cls, conn):
        class Foo(object):
            pass

        a = Foo()
        util.Finalize(a, conn.send, args=('a',))
        del a           # triggers callback for a

        b = Foo()
        close_b = util.Finalize(b, conn.send, args=('b',))
        close_b()       # triggers callback for b
        close_b()       # does nothing because callback has already been called
        del b           # does nothing because callback has already been called

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
        exc = []

        def run_finalizers():
            while not finish:
                time.sleep(random.random() * 1e-1)
                try:
                    # A GC run will eventually happen during this,
                    # collecting stale Foo's and mutating the registry
                    util._run_finalizers()
                except Exception as e:
                    exc.append(e)

        def make_finalizers():
            d = {}
            while not finish:
                try:
                    # Old Foo's get gradually replaced and later
                    # collected by the GC (because of the cyclic ref)
                    d[random.getrandbits(5)] = {Foo() for i in range(10)}
                except Exception as e:
                    exc.append(e)
                    d.clear()

        old_interval = sys.getcheckinterval()
        old_threshold = gc.get_threshold()
        try:
            sys.setcheckinterval(10)
            gc.set_threshold(5, 5, 5)
            threads = [threading.Thread(target=run_finalizers),
                       threading.Thread(target=make_finalizers)]
            with test_support.start_threads(threads):
                time.sleep(4.0)  # Wait a bit to trigger race condition
                finish = True
            if exc:
                raise exc[0]
        finally:
            sys.setcheckinterval(old_interval)
            gc.set_threshold(*old_threshold)
            gc.collect()  # Collect remaining Foo's


#
# Test that from ... import * works for each module
#

class _TestImportStar(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def test_import(self):
        modules = [
            'multiprocessing', 'multiprocessing.connection',
            'multiprocessing.heap', 'multiprocessing.managers',
            'multiprocessing.pool', 'multiprocessing.process',
            'multiprocessing.synchronize', 'multiprocessing.util'
            ]

        if HAS_REDUCTION:
            modules.append('multiprocessing.reduction')

        if c_int is not None:
            # This module requires _ctypes
            modules.append('multiprocessing.sharedctypes')

        for name in modules:
            __import__(name)
            mod = sys.modules[name]

            for attr in getattr(mod, '__all__', ()):
                self.assertTrue(
                    hasattr(mod, attr),
                    '%r does not have attribute %r' % (mod, attr)
                    )

#
# Quick test that logging works -- does not test logging output
#

class _TestLogging(BaseTestCase):

    ALLOWED_TYPES = ('processes',)

    def test_enable_logging(self):
        logger = multiprocessing.get_logger()
        logger.setLevel(util.SUBWARNING)
        self.assertTrue(logger is not None)
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
        p.daemon = True
        p.start()
        self.assertEqual(LEVEL1, reader.recv())

        logger.setLevel(logging.NOTSET)
        root_logger.setLevel(LEVEL2)
        p = self.Process(target=self._test_level, args=(writer,))
        p.daemon = True
        p.start()
        self.assertEqual(LEVEL2, reader.recv())

        root_logger.setLevel(root_level)
        logger.setLevel(level=LOG_LEVEL)


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
        time.sleep(0.5)
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
            p = self.Process(target=time.sleep, args=(1,))
            p.start()
            p.join()
            self.assertTrue(got_signal[0])
            self.assertEqual(p.exitcode, 0)
            killer.join()
        finally:
            signal.signal(signal.SIGUSR1, oldhandler)

#
# Test to verify handle verification, see issue 3321
#

class TestInvalidHandle(unittest.TestCase):

    @unittest.skipIf(WIN32, "skipped on Windows")
    def test_invalid_handles(self):
        conn = _multiprocessing.Connection(44977608)
        self.assertRaises(IOError, conn.poll)
        self.assertRaises(IOError, _multiprocessing.Connection, -1)

#
# Functions used to create test cases from the base ones in this module
#

def get_attributes(Source, names):
    d = {}
    for name in names:
        obj = getattr(Source, name)
        if type(obj) == type(get_attributes):
            obj = staticmethod(obj)
        d[name] = obj
    return d

def create_test_cases(Mixin, type):
    result = {}
    glob = globals()
    Type = type.capitalize()

    for name in glob.keys():
        if name.startswith('_Test'):
            base = glob[name]
            if type in base.ALLOWED_TYPES:
                newname = 'With' + Type + name[1:]
                class Temp(base, unittest.TestCase, Mixin):
                    pass
                result[newname] = Temp
                Temp.__name__ = newname
                Temp.__module__ = Mixin.__module__
    return result

#
# Create test cases
#

class ProcessesMixin(object):
    TYPE = 'processes'
    Process = multiprocessing.Process
    locals().update(get_attributes(multiprocessing, (
        'Queue', 'Lock', 'RLock', 'Semaphore', 'BoundedSemaphore',
        'Condition', 'Event', 'Value', 'Array', 'RawValue',
        'RawArray', 'current_process', 'active_children', 'Pipe',
        'connection', 'JoinableQueue', 'Pool'
        )))

testcases_processes = create_test_cases(ProcessesMixin, type='processes')
globals().update(testcases_processes)


class ManagerMixin(object):
    TYPE = 'manager'
    Process = multiprocessing.Process
    manager = object.__new__(multiprocessing.managers.SyncManager)
    locals().update(get_attributes(manager, (
        'Queue', 'Lock', 'RLock', 'Semaphore', 'BoundedSemaphore',
       'Condition', 'Event', 'Value', 'Array', 'list', 'dict',
        'Namespace', 'JoinableQueue', 'Pool'
        )))

testcases_manager = create_test_cases(ManagerMixin, type='manager')
globals().update(testcases_manager)


class ThreadsMixin(object):
    TYPE = 'threads'
    Process = multiprocessing.dummy.Process
    locals().update(get_attributes(multiprocessing.dummy, (
        'Queue', 'Lock', 'RLock', 'Semaphore', 'BoundedSemaphore',
        'Condition', 'Event', 'Value', 'Array', 'current_process',
        'active_children', 'Pipe', 'connection', 'dict', 'list',
        'Namespace', 'JoinableQueue', 'Pool'
        )))

testcases_threads = create_test_cases(ThreadsMixin, type='threads')
globals().update(testcases_threads)

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
                    return multiprocessing.connection.CHALLENGE
                elif self.count == 2:
                    return b'something bogus'
                return b''
            def send_bytes(self, data):
                pass
        self.assertRaises(multiprocessing.AuthenticationError,
                          multiprocessing.connection.answer_challenge,
                          _FakeConnection(), b'abc')

#
# Test Manager.start()/Pool.__init__() initializer feature - see issue 5585
#

def initializer(ns):
    ns.test += 1

class TestInitializers(unittest.TestCase):
    def setUp(self):
        self.mgr = multiprocessing.Manager()
        self.ns = self.mgr.Namespace()
        self.ns.test = 0

    def tearDown(self):
        self.mgr.shutdown()

    def test_manager_initializer(self):
        m = multiprocessing.managers.SyncManager()
        self.assertRaises(TypeError, m.start, 1)
        m.start(initializer, (self.ns,))
        self.assertEqual(self.ns.test, 1)
        m.shutdown()

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
    except Queue.Empty:
        pass

def _test_process(q):
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
        queue = multiprocessing.Queue()
        proc = multiprocessing.Process(target=_test_process, args=(queue,))
        proc.start()
        proc.join()

    def test_pool_in_process(self):
        p = multiprocessing.Process(target=pool_in_process)
        p.start()
        p.join()

    def test_flushing(self):
        sio = StringIO()
        flike = _file_like(sio)
        flike.write('foo')
        proc = multiprocessing.Process(target=lambda: flike.flush())
        flike.flush()
        assert sio.getvalue() == 'foo'

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
            p.join(10)
        finally:
            socket.setdefaulttimeout(old_timeout)

#
# Test what happens with no "if __name__ == '__main__'"
#

class TestNoForkBomb(unittest.TestCase):
    def test_noforkbomb(self):
        name = os.path.join(os.path.dirname(__file__), 'mp_fork_bomb.py')
        if WIN32:
            rc, out, err = test.script_helper.assert_python_failure(name)
            self.assertEqual(out, '')
            self.assertIn('RuntimeError', err)
        else:
            rc, out, err = test.script_helper.assert_python_ok(name)
            self.assertEqual(out.rstrip(), '123')
            self.assertEqual(err, '')

#
# Issue 12098: check sys.flags of child matches that for parent
#

class TestFlags(unittest.TestCase):
    @classmethod
    def run_in_grandchild(cls, conn):
        conn.send(tuple(sys.flags))

    @classmethod
    def run_in_child(cls):
        import json
        r, w = multiprocessing.Pipe(duplex=False)
        p = multiprocessing.Process(target=cls.run_in_grandchild, args=(w,))
        p.start()
        grandchild_flags = r.recv()
        p.join()
        r.close()
        w.close()
        flags = (tuple(sys.flags), grandchild_flags)
        print(json.dumps(flags))

    @test_support.requires_unicode  # XXX json needs unicode support
    def test_flags(self):
        import json, subprocess
        # start child process using unusual flags
        prog = ('from test.test_multiprocessing import TestFlags; ' +
                'TestFlags.run_in_child()')
        data = subprocess.check_output(
            [sys.executable, '-E', '-B', '-O', '-c', prog])
        child_flags, grandchild_flags = json.loads(data.decode('ascii'))
        self.assertEqual(child_flags, grandchild_flags)

#
# Issue #17555: ForkAwareThreadLock
#

class TestForkAwareThreadLock(unittest.TestCase):
    # We recurisvely start processes.  Issue #17555 meant that the
    # after fork registry would get duplicate entries for the same
    # lock.  The size of the registry at generation n was ~2**n.

    @classmethod
    def child(cls, n, conn):
        if n > 1:
            p = multiprocessing.Process(target=cls.child, args=(n-1, conn))
            p.start()
            p.join()
        else:
            conn.send(len(util._afterfork_registry))
        conn.close()

    def test_lock(self):
        r, w = multiprocessing.Pipe(False)
        l = util.ForkAwareThreadLock()
        old_size = len(util._afterfork_registry)
        p = multiprocessing.Process(target=self.child, args=(5, w))
        p.start()
        new_size = r.recv()
        p.join()
        self.assertLessEqual(new_size, old_size)

#
# Issue #17097: EINTR should be ignored by recv(), send(), accept() etc
#

class TestIgnoreEINTR(unittest.TestCase):

    @classmethod
    def _test_ignore(cls, conn):
        def handler(signum, frame):
            pass
        signal.signal(signal.SIGUSR1, handler)
        conn.send('ready')
        x = conn.recv()
        conn.send(x)
        conn.send_bytes(b'x'*(1024*1024))   # sending 1 MB should block

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
            self.assertEqual(conn.recv_bytes(), b'x'*(1024*1024))
            time.sleep(0.1)
            p.join()
        finally:
            conn.close()

    @classmethod
    def _test_ignore_listener(cls, conn):
        def handler(signum, frame):
            pass
        signal.signal(signal.SIGUSR1, handler)
        l = multiprocessing.connection.Listener()
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

#
#
#

testcases_other = [OtherTest, TestInvalidHandle, TestInitializers,
                   TestStdinBadfiledescriptor, TestTimeouts, TestNoForkBomb,
                   TestFlags, TestForkAwareThreadLock, TestIgnoreEINTR]

#
#
#

def test_main(run=None):
    if sys.platform.startswith("linux"):
        try:
            lock = multiprocessing.RLock()
        except OSError:
            raise unittest.SkipTest("OSError raises on RLock creation, see issue 3111!")

    check_enough_semaphores()

    if run is None:
        from test.test_support import run_unittest as run

    util.get_temp_dir()     # creates temp directory for use by all processes

    multiprocessing.get_logger().setLevel(LOG_LEVEL)

    ProcessesMixin.pool = multiprocessing.Pool(4)
    ThreadsMixin.pool = multiprocessing.dummy.Pool(4)
    ManagerMixin.manager.__init__()
    ManagerMixin.manager.start()
    ManagerMixin.pool = ManagerMixin.manager.Pool(4)

    testcases = (
        sorted(testcases_processes.values(), key=lambda tc:tc.__name__) +
        sorted(testcases_threads.values(), key=lambda tc:tc.__name__) +
        sorted(testcases_manager.values(), key=lambda tc:tc.__name__) +
        testcases_other
        )

    loadTestsFromTestCase = unittest.defaultTestLoader.loadTestsFromTestCase
    suite = unittest.TestSuite(loadTestsFromTestCase(tc) for tc in testcases)
    # (ncoghlan): Whether or not sys.exc_clear is executed by the threading
    # module during these tests is at least platform dependent and possibly
    # non-deterministic on any given platform. So we don't mind if the listed
    # warnings aren't actually raised.
    with test_support.check_py3k_warnings(
            (".+__(get|set)slice__ has been removed", DeprecationWarning),
            (r"sys.exc_clear\(\) not supported", DeprecationWarning),
            quiet=True):
        run(suite)

    ThreadsMixin.pool.terminate()
    ProcessesMixin.pool.terminate()
    ManagerMixin.pool.terminate()
    ManagerMixin.manager.shutdown()

    del ProcessesMixin.pool, ThreadsMixin.pool, ManagerMixin.pool

def main():
    test_main(unittest.TextTestRunner(verbosity=2).run)

if __name__ == '__main__':
    main()
