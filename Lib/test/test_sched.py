#!/usr/bin/env python

import queue
import sched
import time
import unittest
from test import support
try:
    import threading
except ImportError:
    threading = None

TIMEOUT = 10


class Timer:
    def __init__(self):
        self._cond = threading.Condition()
        self._time = 0
        self._stop = 0

    def time(self):
        with self._cond:
            return self._time

    # increase the time but not beyond the established limit
    def sleep(self, t):
        assert t >= 0
        with self._cond:
            t += self._time
            while self._stop < t:
                self._time = self._stop
                self._cond.wait()
            self._time = t

    # advance time limit for user code
    def advance(self, t):
        assert t >= 0
        with self._cond:
            self._stop += t
            self._cond.notify_all()


class TestCase(unittest.TestCase):

    def test_enter(self):
        l = []
        fun = lambda x: l.append(x)
        scheduler = sched.scheduler(time.time, time.sleep)
        for x in [0.5, 0.4, 0.3, 0.2, 0.1]:
            z = scheduler.enter(x, 1, fun, (x,))
        scheduler.run()
        self.assertEqual(l, [0.1, 0.2, 0.3, 0.4, 0.5])

    def test_enterabs(self):
        l = []
        fun = lambda x: l.append(x)
        scheduler = sched.scheduler(time.time, time.sleep)
        for x in [0.05, 0.04, 0.03, 0.02, 0.01]:
            z = scheduler.enterabs(x, 1, fun, (x,))
        scheduler.run()
        self.assertEqual(l, [0.01, 0.02, 0.03, 0.04, 0.05])

    @unittest.skipUnless(threading, 'Threading required for this test.')
    def test_enter_concurrent(self):
        q = queue.Queue()
        fun = q.put
        timer = Timer()
        scheduler = sched.scheduler(timer.time, timer.sleep)
        scheduler.enter(1, 1, fun, (1,))
        scheduler.enter(3, 1, fun, (3,))
        t = threading.Thread(target=scheduler.run)
        t.start()
        timer.advance(1)
        self.assertEqual(q.get(timeout=TIMEOUT), 1)
        self.assertTrue(q.empty())
        for x in [4, 5, 2]:
            z = scheduler.enter(x - 1, 1, fun, (x,))
        timer.advance(2)
        self.assertEqual(q.get(timeout=TIMEOUT), 2)
        self.assertEqual(q.get(timeout=TIMEOUT), 3)
        self.assertTrue(q.empty())
        timer.advance(1)
        self.assertEqual(q.get(timeout=TIMEOUT), 4)
        self.assertTrue(q.empty())
        timer.advance(1)
        self.assertEqual(q.get(timeout=TIMEOUT), 5)
        self.assertTrue(q.empty())
        timer.advance(1000)
        t.join(timeout=TIMEOUT)
        self.assertFalse(t.is_alive())
        self.assertTrue(q.empty())
        self.assertEqual(timer.time(), 5)

    def test_priority(self):
        l = []
        fun = lambda x: l.append(x)
        scheduler = sched.scheduler(time.time, time.sleep)
        for priority in [1, 2, 3, 4, 5]:
            z = scheduler.enterabs(0.01, priority, fun, (priority,))
        scheduler.run()
        self.assertEqual(l, [1, 2, 3, 4, 5])

    def test_cancel(self):
        l = []
        fun = lambda x: l.append(x)
        scheduler = sched.scheduler(time.time, time.sleep)
        now = time.time()
        event1 = scheduler.enterabs(now + 0.01, 1, fun, (0.01,))
        event2 = scheduler.enterabs(now + 0.02, 1, fun, (0.02,))
        event3 = scheduler.enterabs(now + 0.03, 1, fun, (0.03,))
        event4 = scheduler.enterabs(now + 0.04, 1, fun, (0.04,))
        event5 = scheduler.enterabs(now + 0.05, 1, fun, (0.05,))
        scheduler.cancel(event1)
        scheduler.cancel(event5)
        scheduler.run()
        self.assertEqual(l, [0.02, 0.03, 0.04])

    @unittest.skipUnless(threading, 'Threading required for this test.')
    def test_cancel_concurrent(self):
        q = queue.Queue()
        fun = q.put
        timer = Timer()
        scheduler = sched.scheduler(timer.time, timer.sleep)
        now = timer.time()
        event1 = scheduler.enterabs(now + 1, 1, fun, (1,))
        event2 = scheduler.enterabs(now + 2, 1, fun, (2,))
        event4 = scheduler.enterabs(now + 4, 1, fun, (4,))
        event5 = scheduler.enterabs(now + 5, 1, fun, (5,))
        event3 = scheduler.enterabs(now + 3, 1, fun, (3,))
        t = threading.Thread(target=scheduler.run)
        t.start()
        timer.advance(1)
        self.assertEqual(q.get(timeout=TIMEOUT), 1)
        self.assertTrue(q.empty())
        scheduler.cancel(event2)
        scheduler.cancel(event5)
        timer.advance(1)
        self.assertTrue(q.empty())
        timer.advance(1)
        self.assertEqual(q.get(timeout=TIMEOUT), 3)
        self.assertTrue(q.empty())
        timer.advance(1)
        self.assertEqual(q.get(timeout=TIMEOUT), 4)
        self.assertTrue(q.empty())
        timer.advance(1000)
        t.join(timeout=TIMEOUT)
        self.assertFalse(t.is_alive())
        self.assertTrue(q.empty())
        self.assertEqual(timer.time(), 4)

    def test_empty(self):
        l = []
        fun = lambda x: l.append(x)
        scheduler = sched.scheduler(time.time, time.sleep)
        self.assertTrue(scheduler.empty())
        for x in [0.05, 0.04, 0.03, 0.02, 0.01]:
            z = scheduler.enterabs(x, 1, fun, (x,))
        self.assertFalse(scheduler.empty())
        scheduler.run()
        self.assertTrue(scheduler.empty())

    def test_queue(self):
        l = []
        fun = lambda x: l.append(x)
        scheduler = sched.scheduler(time.time, time.sleep)
        now = time.time()
        e5 = scheduler.enterabs(now + 0.05, 1, fun)
        e1 = scheduler.enterabs(now + 0.01, 1, fun)
        e2 = scheduler.enterabs(now + 0.02, 1, fun)
        e4 = scheduler.enterabs(now + 0.04, 1, fun)
        e3 = scheduler.enterabs(now + 0.03, 1, fun)
        # queue property is supposed to return an order list of
        # upcoming events
        self.assertEqual(scheduler.queue, [e1, e2, e3, e4, e5])

    def test_args_kwargs(self):
        flag = []

        def fun(*a, **b):
            flag.append(None)
            self.assertEqual(a, (1,2,3))
            self.assertEqual(b, {"foo":1})

        scheduler = sched.scheduler(time.time, time.sleep)
        z = scheduler.enterabs(0.01, 1, fun, argument=(1,2,3), kwargs={"foo":1})
        scheduler.run()
        self.assertEqual(flag, [None])

    def test_run_non_blocking(self):
        l = []
        fun = lambda x: l.append(x)
        scheduler = sched.scheduler(time.time, time.sleep)
        for x in [10, 9, 8, 7, 6]:
            scheduler.enter(x, 1, fun, (x,))
        scheduler.run(blocking=False)
        self.assertEqual(l, [])


def test_main():
    support.run_unittest(TestCase)

if __name__ == "__main__":
    test_main()
