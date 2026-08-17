import itertools
import threading
import time
import weakref
from concurrent import futures
from operator import add
from test import support
from test.support import Py_GIL_DISABLED, warnings_helper


def mul(x, y):
    return x * y

def capture(*args, **kwargs):
    return args, kwargs


class MyObject(object):
    def my_method(self):
        pass


def make_dummy_object(_):
    return MyObject()


# Used in test_swallows_falsey_exceptions
def raiser(exception, msg='std'):
    raise exception(msg)


def timeout_on_one(x):
    if x == 1:
        raise TimeoutError
    return x


class FalseyBoolException(Exception):
    def __bool__(self):
        return False


class FalseyLenException(Exception):
    def __len__(self):
        return 0


class ExecutorTest:

    # Executor.shutdown() and context manager usage is tested by
    # ExecutorShutdownTest.
    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_submit(self):
        future = self.executor.submit(pow, 2, 8)
        self.assertEqual(256, future.result())

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_submit_keyword(self):
        future = self.executor.submit(mul, 2, y=8)
        self.assertEqual(16, future.result())
        future = self.executor.submit(capture, 1, self=2, fn=3)
        self.assertEqual(future.result(), ((1,), {'self': 2, 'fn': 3}))
        with self.assertRaises(TypeError):
            self.executor.submit(fn=capture, arg=1)
        with self.assertRaises(TypeError):
            self.executor.submit(arg=1)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_map(self):
        self.assertEqual(
                list(self.executor.map(pow, range(10), range(10))),
                list(map(pow, range(10), range(10))))

        self.assertEqual(
                list(self.executor.map(pow, range(10), range(10), chunksize=3)),
                list(map(pow, range(10), range(10))))

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_map_exception(self):
        i = self.executor.map(divmod, [5, 5, 5, 5], [2, 3, 0, 5])
        self.assertEqual(next(i), (2, 1))
        self.assertEqual(next(i), (1, 2))
        self.assertRaises(ZeroDivisionError, next, i)
        self.assertEqual(next(i), (1, 0))
        self.assertRaises(StopIteration, next, i)
        self.assertRaises(StopIteration, next, i)

        i = self.executor.map(divmod, [5, 5, 5, 5], [2, 0, 3, 5], chunksize=3)
        self.assertEqual(next(i), (2, 1))
        self.assertRaises(ZeroDivisionError, next, i)
        self.assertEqual(next(i), (1, 2))
        self.assertEqual(next(i), (1, 0))
        self.assertRaises(StopIteration, next, i)
        self.assertRaises(StopIteration, next, i)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_map_timeout_from_callable(self):
        # A TimeoutError from the callable is not the map() timeout, whether
        # or not a map() timeout is set.
        for timeout in (None, support.SHORT_TIMEOUT):
            with self.subTest(timeout=timeout):
                i = self.executor.map(timeout_on_one, [0, 1, 2, 3],
                                      timeout=timeout)
                self.assertEqual(next(i), 0)
                self.assertRaises(TimeoutError, next, i)
                self.assertEqual(next(i), 2)
                self.assertEqual(next(i), 3)
                self.assertRaises(StopIteration, next, i)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @support.requires_resource('walltime')
    def test_map_timeout(self):
        results = []
        i = self.executor.map(time.sleep, [0, 0, 6], timeout=5)
        try:
            for result in i:
                results.append(result)
        except futures.TimeoutError:
            pass
        else:
            self.fail('expected TimeoutError')

        # gh-110097: On heavily loaded systems, the launch of the worker may
        # take longer than the specified timeout.
        self.assertIn(results, ([None, None], [None], []))

        # The remaining calls are cancelled, so the iterator is exhausted.
        self.assertRaises(StopIteration, next, i)
        self.assertRaises(StopIteration, next, i)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_map_close(self):
        i = self.executor.map(divmod, [5, 5, 5, 5], [2, 0, 3, 5])
        self.assertEqual(next(i), (2, 1))
        i.close()
        self.assertRaises(StopIteration, next, i)
        self.assertRaises(StopIteration, next, i)

        i = self.executor.map(divmod, [5, 5, 5, 5], [2, 0, 3, 5], chunksize=3)
        self.assertEqual(next(i), (2, 1))
        i.close()
        self.assertRaises(StopIteration, next, i)
        self.assertRaises(StopIteration, next, i)

    def test_map_buffersize_type_validation(self):
        for buffersize in ("foo", 2.0):
            with self.subTest(buffersize=buffersize):
                with self.assertRaisesRegex(
                    TypeError,
                    "buffersize must be an integer or None",
                ):
                    self.executor.map(str, range(4), buffersize=buffersize)

    def test_map_buffersize_value_validation(self):
        for buffersize in (0, -1):
            with self.subTest(buffersize=buffersize):
                with self.assertRaisesRegex(
                    ValueError,
                    "buffersize must be None or > 0",
                ):
                    self.executor.map(str, range(4), buffersize=buffersize)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_map_buffersize(self):
        ints = range(4)
        for buffersize in (1, 2, len(ints), len(ints) * 2):
            with self.subTest(buffersize=buffersize):
                res = self.executor.map(str, ints, buffersize=buffersize)
                self.assertListEqual(list(res), ["0", "1", "2", "3"])

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_map_buffersize_on_multiple_iterables(self):
        ints = range(4)
        for buffersize in (1, 2, len(ints), len(ints) * 2):
            with self.subTest(buffersize=buffersize):
                res = self.executor.map(add, ints, ints, buffersize=buffersize)
                self.assertListEqual(list(res), [0, 2, 4, 6])

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_map_buffersize_on_infinite_iterable(self):
        res = self.executor.map(str, itertools.count(), buffersize=2)
        self.assertEqual(next(res, None), "0")
        self.assertEqual(next(res, None), "1")
        self.assertEqual(next(res, None), "2")

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_map_buffersize_on_multiple_infinite_iterables(self):
        res = self.executor.map(
            add,
            itertools.count(),
            itertools.count(),
            buffersize=2
        )
        self.assertEqual(next(res, None), 0)
        self.assertEqual(next(res, None), 2)
        self.assertEqual(next(res, None), 4)

    def test_map_buffersize_on_empty_iterable(self):
        res = self.executor.map(str, [], buffersize=2)
        self.assertIsNone(next(res, None))

    def test_map_buffersize_without_iterable(self):
        res = self.executor.map(str, buffersize=2)
        self.assertIsNone(next(res, None))

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_map_buffersize_when_buffer_is_full(self):
        ints = iter(range(4))
        buffersize = 2
        self.executor.map(str, ints, buffersize=buffersize)
        self.executor.shutdown(wait=True)  # wait for tasks to complete
        self.assertEqual(
            next(ints),
            buffersize,
            msg="should have fetched only `buffersize` elements from `ints`.",
        )

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_shutdown_race_issue12456(self):
        # Issue #12456: race condition at shutdown where trying to post a
        # sentinel in the call queue blocks (the queue is full while processes
        # have exited).
        self.executor.map(str, [2] * (self.worker_count + 1))
        self.executor.shutdown()

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
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

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
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

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_swallows_falsey_exceptions(self):
        # see gh-132063: Prevent exceptions that evaluate as falsey
        # from being ignored.
        # Recall: `x` is falsey if `len(x)` returns 0 or `bool(x)` returns False.

        msg = 'boolbool'
        with self.assertRaisesRegex(FalseyBoolException, msg):
            self.executor.submit(raiser, FalseyBoolException, msg).result()

        msg = 'lenlen'
        with self.assertRaisesRegex(FalseyLenException, msg):
            self.executor.submit(raiser, FalseyLenException, msg).result()
