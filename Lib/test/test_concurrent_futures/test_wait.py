import sys
import threading
import unittest
from concurrent import futures
from test import support
from test.support import threading_helper

from .util import (
    CANCELLED_FUTURE, CANCELLED_AND_NOTIFIED_FUTURE, EXCEPTION_FUTURE,
    SUCCESSFUL_FUTURE,
    create_executor_tests, setup_module,
    BaseTestCase, ThreadPoolMixin,
    ProcessPoolForkMixin, ProcessPoolForkserverMixin, ProcessPoolSpawnMixin)


def mul(x, y):
    return x * y

def wait_and_raise(e):
    e.wait()
    raise Exception('this is an exception')


class WaitTests:
    def test_20369(self):
        # See https://bugs.python.org/issue20369
        future = self.executor.submit(mul, 1, 2)
        done, not_done = futures.wait([future, future],
                            return_when=futures.ALL_COMPLETED)
        self.assertEqual({future}, done)
        self.assertEqual(set(), not_done)


    def test_first_completed(self):
        event = self.create_event()
        future1 = self.executor.submit(mul, 21, 2)
        future2 = self.executor.submit(event.wait)

        try:
            done, not_done = futures.wait(
                    [CANCELLED_FUTURE, future1, future2],
                     return_when=futures.FIRST_COMPLETED)

            self.assertEqual(set([future1]), done)
            self.assertEqual(set([CANCELLED_FUTURE, future2]), not_done)
        finally:
            event.set()
        future2.result()  # wait for job to finish

    def test_first_completed_some_already_completed(self):
        event = self.create_event()
        future1 = self.executor.submit(event.wait)

        try:
            finished, pending = futures.wait(
                     [CANCELLED_AND_NOTIFIED_FUTURE, SUCCESSFUL_FUTURE, future1],
                     return_when=futures.FIRST_COMPLETED)

            self.assertEqual(
                    set([CANCELLED_AND_NOTIFIED_FUTURE, SUCCESSFUL_FUTURE]),
                    finished)
            self.assertEqual(set([future1]), pending)
        finally:
            event.set()
        future1.result()  # wait for job to finish

    def test_first_exception(self):
        event1 = self.create_event()
        event2 = self.create_event()
        try:
            future1 = self.executor.submit(mul, 2, 21)
            future2 = self.executor.submit(wait_and_raise, event1)
            future3 = self.executor.submit(event2.wait)

            # Ensure that future1 is completed before future2 finishes
            def wait_for_future1():
                future1.result()
                event1.set()

            t = threading.Thread(target=wait_for_future1)
            t.start()

            finished, pending = futures.wait(
                    [future1, future2, future3],
                    return_when=futures.FIRST_EXCEPTION)

            self.assertEqual(set([future1, future2]), finished)
            self.assertEqual(set([future3]), pending)

            threading_helper.join_thread(t)
        finally:
            event1.set()
            event2.set()
        future3.result()  # wait for job to finish

    def test_first_exception_some_already_complete(self):
        event = self.create_event()
        future1 = self.executor.submit(divmod, 21, 0)
        future2 = self.executor.submit(event.wait)

        try:
            finished, pending = futures.wait(
                    [SUCCESSFUL_FUTURE,
                     CANCELLED_FUTURE,
                     CANCELLED_AND_NOTIFIED_FUTURE,
                     future1, future2],
                    return_when=futures.FIRST_EXCEPTION)

            self.assertEqual(set([SUCCESSFUL_FUTURE,
                                  CANCELLED_AND_NOTIFIED_FUTURE,
                                  future1]), finished)
            self.assertEqual(set([CANCELLED_FUTURE, future2]), pending)
        finally:
            event.set()
        future2.result()  # wait for job to finish

    def test_first_exception_one_already_failed(self):
        event = self.create_event()
        future1 = self.executor.submit(event.wait)

        try:
            finished, pending = futures.wait(
                     [EXCEPTION_FUTURE, future1],
                     return_when=futures.FIRST_EXCEPTION)

            self.assertEqual(set([EXCEPTION_FUTURE]), finished)
            self.assertEqual(set([future1]), pending)
        finally:
            event.set()
        future1.result()  # wait for job to finish

    def test_all_completed(self):
        future1 = self.executor.submit(divmod, 2, 0)
        future2 = self.executor.submit(mul, 2, 21)

        finished, pending = futures.wait(
                [SUCCESSFUL_FUTURE,
                 CANCELLED_AND_NOTIFIED_FUTURE,
                 EXCEPTION_FUTURE,
                 future1,
                 future2],
                return_when=futures.ALL_COMPLETED)

        self.assertEqual(set([SUCCESSFUL_FUTURE,
                              CANCELLED_AND_NOTIFIED_FUTURE,
                              EXCEPTION_FUTURE,
                              future1,
                              future2]), finished)
        self.assertEqual(set(), pending)

    def test_timeout(self):
        short_timeout = 0.050

        event = self.create_event()
        future = self.executor.submit(event.wait)

        try:
            finished, pending = futures.wait(
                    [CANCELLED_AND_NOTIFIED_FUTURE,
                     EXCEPTION_FUTURE,
                     SUCCESSFUL_FUTURE,
                     future],
                    timeout=short_timeout,
                    return_when=futures.ALL_COMPLETED)

            self.assertEqual(set([CANCELLED_AND_NOTIFIED_FUTURE,
                                  EXCEPTION_FUTURE,
                                  SUCCESSFUL_FUTURE]),
                             finished)
            self.assertEqual(set([future]), pending)
        finally:
            event.set()
        future.result()  # wait for job to finish


class ThreadPoolWaitTests(ThreadPoolMixin, WaitTests, BaseTestCase):

    def test_pending_calls_race(self):
        # Issue #14406: multi-threaded race condition when waiting on all
        # futures.
        event = threading.Event()
        def future_func():
            event.wait()
        oldswitchinterval = sys.getswitchinterval()
        support.setswitchinterval(1e-6)
        try:
            fs = {self.executor.submit(future_func) for i in range(100)}
            event.set()
            futures.wait(fs, return_when=futures.ALL_COMPLETED)
        finally:
            sys.setswitchinterval(oldswitchinterval)


create_executor_tests(globals(), WaitTests,
                      executor_mixins=(ProcessPoolForkMixin,
                                       ProcessPoolForkserverMixin,
                                       ProcessPoolSpawnMixin))


def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
