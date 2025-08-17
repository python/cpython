import itertools
import time
import unittest
import weakref
from concurrent import futures
from concurrent.futures._base import (
    CANCELLED_AND_NOTIFIED, FINISHED, Future)

from test import support

from .util import (
    PENDING_FUTURE, RUNNING_FUTURE,
    CANCELLED_AND_NOTIFIED_FUTURE, EXCEPTION_FUTURE, SUCCESSFUL_FUTURE,
    create_future, create_executor_tests, setup_module)


def mul(x, y):
    return x * y


class AsCompletedTests:
    def test_no_timeout(self):
        future1 = self.executor.submit(mul, 2, 21)
        future2 = self.executor.submit(mul, 7, 6)

        completed = set(futures.as_completed(
                [CANCELLED_AND_NOTIFIED_FUTURE,
                 EXCEPTION_FUTURE,
                 SUCCESSFUL_FUTURE,
                 future1, future2]))
        self.assertEqual(set(
                [CANCELLED_AND_NOTIFIED_FUTURE,
                 EXCEPTION_FUTURE,
                 SUCCESSFUL_FUTURE,
                 future1, future2]),
                completed)

    def test_future_times_out(self):
        """Test ``futures.as_completed`` timing out before
        completing it's final future."""
        already_completed = {CANCELLED_AND_NOTIFIED_FUTURE,
                             EXCEPTION_FUTURE,
                             SUCCESSFUL_FUTURE}

        # Windows clock resolution is around 15.6 ms
        short_timeout = 0.100
        for timeout in (0, short_timeout):
            with self.subTest(timeout):

                completed_futures = set()
                future = self.executor.submit(time.sleep, short_timeout * 10)

                try:
                    for f in futures.as_completed(
                        already_completed | {future},
                        timeout
                    ):
                        completed_futures.add(f)
                except futures.TimeoutError:
                    pass

                # Check that ``future`` wasn't completed.
                self.assertEqual(completed_futures, already_completed)

    def test_duplicate_futures(self):
        # Issue 20367. Duplicate futures should not raise exceptions or give
        # duplicate responses.
        # Issue #31641: accept arbitrary iterables.
        future1 = self.executor.submit(time.sleep, 2)
        completed = [
            f for f in futures.as_completed(itertools.repeat(future1, 3))
        ]
        self.assertEqual(len(completed), 1)

    def test_free_reference_yielded_future(self):
        # Issue #14406: Generator should not keep references
        # to finished futures.
        futures_list = [Future() for _ in range(8)]
        futures_list.append(create_future(state=CANCELLED_AND_NOTIFIED))
        futures_list.append(create_future(state=FINISHED, result=42))

        with self.assertRaises(futures.TimeoutError):
            for future in futures.as_completed(futures_list, timeout=0):
                futures_list.remove(future)
                wr = weakref.ref(future)
                del future
                support.gc_collect()  # For PyPy or other GCs.
                self.assertIsNone(wr())

        futures_list[0].set_result("test")
        for future in futures.as_completed(futures_list):
            futures_list.remove(future)
            wr = weakref.ref(future)
            del future
            support.gc_collect()  # For PyPy or other GCs.
            self.assertIsNone(wr())
            if futures_list:
                futures_list[0].set_result("test")

    def test_correct_timeout_exception_msg(self):
        futures_list = [CANCELLED_AND_NOTIFIED_FUTURE, PENDING_FUTURE,
                        RUNNING_FUTURE, SUCCESSFUL_FUTURE]

        with self.assertRaises(futures.TimeoutError) as cm:
            list(futures.as_completed(futures_list, timeout=0))

        self.assertEqual(str(cm.exception), '2 (of 4) futures unfinished')


create_executor_tests(globals(), AsCompletedTests)


def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
