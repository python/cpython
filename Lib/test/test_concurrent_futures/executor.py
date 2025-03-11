import threading
import time
import unittest
import weakref
from concurrent import futures
from test import support
from test.support import Py_GIL_DISABLED


def mul(x, y):
    return x * y

def capture(*args, **kwargs):
    return args, kwargs


class MyObject(object):
    def my_method(self):
        pass


def make_dummy_object(_):
    return MyObject()


class ExecutorTest:
    # Executor.shutdown() and context manager usage is tested by
    # ExecutorShutdownTest.
    def test_submit(self):
        future = self.executor.submit(pow, 2, 8)
        self.assertEqual(256, future.result())

    def test_submit_keyword(self):
        future = self.executor.submit(mul, 2, y=8)
        self.assertEqual(16, future.result())
        future = self.executor.submit(capture, 1, self=2, fn=3)
        self.assertEqual(future.result(), ((1,), {'self': 2, 'fn': 3}))
        with self.assertRaises(TypeError):
            self.executor.submit(fn=capture, arg=1)
        with self.assertRaises(TypeError):
            self.executor.submit(arg=1)

    def test_map(self):
        self.assertEqual(
                list(self.executor.map(pow, range(10), range(10))),
                list(map(pow, range(10), range(10))))

        self.assertEqual(
                list(self.executor.map(pow, range(10), range(10), chunksize=3)),
                list(map(pow, range(10), range(10))))

    def test_map_exception(self):
        i = self.executor.map(divmod, [1, 1, 1, 1], [2, 3, 0, 5])
        self.assertEqual(i.__next__(), (0, 1))
        self.assertEqual(i.__next__(), (0, 1))
        self.assertRaises(ZeroDivisionError, i.__next__)

    @support.requires_resource('walltime')
    def test_map_timeout(self):
        results = []
        try:
            for i in self.executor.map(time.sleep,
                                       [0, 0, 6],
                                       timeout=5):
                results.append(i)
        except futures.TimeoutError:
            pass
        else:
            self.fail('expected TimeoutError')

        self.assertEqual([None, None], results)

    def test_shutdown_race_issue12456(self):
        # Issue #12456: race condition at shutdown where trying to post a
        # sentinel in the call queue blocks (the queue is full while processes
        # have exited).
        self.executor.map(str, [2] * (self.worker_count + 1))
        self.executor.shutdown()

    @support.cpython_only
    def test_no_stale_references(self):
        # Issue #16284: check that the executors don't unnecessarily hang onto
        # references.
        my_object = MyObject()
        my_object_collected = threading.Event()
        def set_event():
            if Py_GIL_DISABLED:
                # gh-117688 Avoid deadlock by setting the event in a
                # background thread. The current thread may be in the middle
                # of the my_object_collected.wait() call, which holds locks
                # needed by my_object_collected.set().
                threading.Thread(target=my_object_collected.set).start()
            else:
                my_object_collected.set()
        my_object_callback = weakref.ref(my_object, lambda obj: set_event())
        # Deliberately discarding the future.
        self.executor.submit(my_object.my_method)
        del my_object

        if Py_GIL_DISABLED:
            # Due to biased reference counting, my_object might only be
            # deallocated while the thread that created it runs -- if the
            # thread is paused waiting on an event, it may not merge the
            # refcount of the queued object. For that reason, we alternate
            # between running the GC and waiting for the event.
            wait_time = 0
            collected = False
            while not collected and wait_time <= support.SHORT_TIMEOUT:
                support.gc_collect()
                collected = my_object_collected.wait(timeout=1.0)
                wait_time += 1.0
        else:
            collected = my_object_collected.wait(timeout=support.SHORT_TIMEOUT)
        self.assertTrue(collected,
                        "Stale reference not collected within timeout.")

    def test_max_workers_negative(self):
        for number in (0, -1):
            with self.assertRaisesRegex(ValueError,
                                        "max_workers must be greater "
                                        "than 0"):
                self.executor_type(max_workers=number)

    def test_free_reference(self):
        # Issue #14406: Result iterator should not keep an internal
        # reference to result objects.
        for obj in self.executor.map(make_dummy_object, range(10)):
            wr = weakref.ref(obj)
            del obj
            support.gc_collect()  # For PyPy or other GCs.

            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if wr() is None:
                    break
