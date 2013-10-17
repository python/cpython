"""Tests for queues.py"""

import unittest
import unittest.mock

from asyncio import events
from asyncio import futures
from asyncio import locks
from asyncio import queues
from asyncio import tasks
from asyncio import test_utils


class _QueueTestBase(unittest.TestCase):

    def setUp(self):
        self.loop = test_utils.TestLoop()
        events.set_event_loop(None)

    def tearDown(self):
        self.loop.close()


class QueueBasicTests(_QueueTestBase):

    def _test_repr_or_str(self, fn, expect_id):
        """Test Queue's repr or str.

        fn is repr or str. expect_id is True if we expect the Queue's id to
        appear in fn(Queue()).
        """
        def gen():
            when = yield
            self.assertAlmostEqual(0.1, when)
            when = yield 0.1
            self.assertAlmostEqual(0.2, when)
            yield 0.1

        loop = test_utils.TestLoop(gen)
        self.addCleanup(loop.close)

        q = queues.Queue(loop=loop)
        self.assertTrue(fn(q).startswith('<Queue'), fn(q))
        id_is_present = hex(id(q)) in fn(q)
        self.assertEqual(expect_id, id_is_present)

        @tasks.coroutine
        def add_getter():
            q = queues.Queue(loop=loop)
            # Start a task that waits to get.
            tasks.Task(q.get(), loop=loop)
            # Let it start waiting.
            yield from tasks.sleep(0.1, loop=loop)
            self.assertTrue('_getters[1]' in fn(q))
            # resume q.get coroutine to finish generator
            q.put_nowait(0)

        loop.run_until_complete(add_getter())

        @tasks.coroutine
        def add_putter():
            q = queues.Queue(maxsize=1, loop=loop)
            q.put_nowait(1)
            # Start a task that waits to put.
            tasks.Task(q.put(2), loop=loop)
            # Let it start waiting.
            yield from tasks.sleep(0.1, loop=loop)
            self.assertTrue('_putters[1]' in fn(q))
            # resume q.put coroutine to finish generator
            q.get_nowait()

        loop.run_until_complete(add_putter())

        q = queues.Queue(loop=loop)
        q.put_nowait(1)
        self.assertTrue('_queue=[1]' in fn(q))

    def test_ctor_loop(self):
        loop = unittest.mock.Mock()
        q = queues.Queue(loop=loop)
        self.assertIs(q._loop, loop)

        q = queues.Queue(loop=self.loop)
        self.assertIs(q._loop, self.loop)

    def test_ctor_noloop(self):
        try:
            events.set_event_loop(self.loop)
            q = queues.Queue()
            self.assertIs(q._loop, self.loop)
        finally:
            events.set_event_loop(None)

    def test_repr(self):
        self._test_repr_or_str(repr, True)

    def test_str(self):
        self._test_repr_or_str(str, False)

    def test_empty(self):
        q = queues.Queue(loop=self.loop)
        self.assertTrue(q.empty())
        q.put_nowait(1)
        self.assertFalse(q.empty())
        self.assertEqual(1, q.get_nowait())
        self.assertTrue(q.empty())

    def test_full(self):
        q = queues.Queue(loop=self.loop)
        self.assertFalse(q.full())

        q = queues.Queue(maxsize=1, loop=self.loop)
        q.put_nowait(1)
        self.assertTrue(q.full())

    def test_order(self):
        q = queues.Queue(loop=self.loop)
        for i in [1, 3, 2]:
            q.put_nowait(i)

        items = [q.get_nowait() for _ in range(3)]
        self.assertEqual([1, 3, 2], items)

    def test_maxsize(self):

        def gen():
            when = yield
            self.assertAlmostEqual(0.01, when)
            when = yield 0.01
            self.assertAlmostEqual(0.02, when)
            yield 0.01

        loop = test_utils.TestLoop(gen)
        self.addCleanup(loop.close)

        q = queues.Queue(maxsize=2, loop=loop)
        self.assertEqual(2, q.maxsize)
        have_been_put = []

        @tasks.coroutine
        def putter():
            for i in range(3):
                yield from q.put(i)
                have_been_put.append(i)
            return True

        @tasks.coroutine
        def test():
            t = tasks.Task(putter(), loop=loop)
            yield from tasks.sleep(0.01, loop=loop)

            # The putter is blocked after putting two items.
            self.assertEqual([0, 1], have_been_put)
            self.assertEqual(0, q.get_nowait())

            # Let the putter resume and put last item.
            yield from tasks.sleep(0.01, loop=loop)
            self.assertEqual([0, 1, 2], have_been_put)
            self.assertEqual(1, q.get_nowait())
            self.assertEqual(2, q.get_nowait())

            self.assertTrue(t.done())
            self.assertTrue(t.result())

        loop.run_until_complete(test())
        self.assertAlmostEqual(0.02, loop.time())


class QueueGetTests(_QueueTestBase):

    def test_blocking_get(self):
        q = queues.Queue(loop=self.loop)
        q.put_nowait(1)

        @tasks.coroutine
        def queue_get():
            return (yield from q.get())

        res = self.loop.run_until_complete(queue_get())
        self.assertEqual(1, res)

    def test_get_with_putters(self):
        q = queues.Queue(1, loop=self.loop)
        q.put_nowait(1)

        waiter = futures.Future(loop=self.loop)
        q._putters.append((2, waiter))

        res = self.loop.run_until_complete(q.get())
        self.assertEqual(1, res)
        self.assertTrue(waiter.done())
        self.assertIsNone(waiter.result())

    def test_blocking_get_wait(self):

        def gen():
            when = yield
            self.assertAlmostEqual(0.01, when)
            yield 0.01

        loop = test_utils.TestLoop(gen)
        self.addCleanup(loop.close)

        q = queues.Queue(loop=loop)
        started = locks.Event(loop=loop)
        finished = False

        @tasks.coroutine
        def queue_get():
            nonlocal finished
            started.set()
            res = yield from q.get()
            finished = True
            return res

        @tasks.coroutine
        def queue_put():
            loop.call_later(0.01, q.put_nowait, 1)
            queue_get_task = tasks.Task(queue_get(), loop=loop)
            yield from started.wait()
            self.assertFalse(finished)
            res = yield from queue_get_task
            self.assertTrue(finished)
            return res

        res = loop.run_until_complete(queue_put())
        self.assertEqual(1, res)
        self.assertAlmostEqual(0.01, loop.time())

    def test_nonblocking_get(self):
        q = queues.Queue(loop=self.loop)
        q.put_nowait(1)
        self.assertEqual(1, q.get_nowait())

    def test_nonblocking_get_exception(self):
        q = queues.Queue(loop=self.loop)
        self.assertRaises(queues.Empty, q.get_nowait)

    def test_get_cancelled(self):

        def gen():
            when = yield
            self.assertAlmostEqual(0.01, when)
            when = yield 0.01
            self.assertAlmostEqual(0.061, when)
            yield 0.05

        loop = test_utils.TestLoop(gen)
        self.addCleanup(loop.close)

        q = queues.Queue(loop=loop)

        @tasks.coroutine
        def queue_get():
            return (yield from tasks.wait_for(q.get(), 0.051, loop=loop))

        @tasks.coroutine
        def test():
            get_task = tasks.Task(queue_get(), loop=loop)
            yield from tasks.sleep(0.01, loop=loop)  # let the task start
            q.put_nowait(1)
            return (yield from get_task)

        self.assertEqual(1, loop.run_until_complete(test()))
        self.assertAlmostEqual(0.06, loop.time())

    def test_get_cancelled_race(self):
        q = queues.Queue(loop=self.loop)

        t1 = tasks.Task(q.get(), loop=self.loop)
        t2 = tasks.Task(q.get(), loop=self.loop)

        test_utils.run_briefly(self.loop)
        t1.cancel()
        test_utils.run_briefly(self.loop)
        self.assertTrue(t1.done())
        q.put_nowait('a')
        test_utils.run_briefly(self.loop)
        self.assertEqual(t2.result(), 'a')

    def test_get_with_waiting_putters(self):
        q = queues.Queue(loop=self.loop, maxsize=1)
        tasks.Task(q.put('a'), loop=self.loop)
        tasks.Task(q.put('b'), loop=self.loop)
        test_utils.run_briefly(self.loop)
        self.assertEqual(self.loop.run_until_complete(q.get()), 'a')
        self.assertEqual(self.loop.run_until_complete(q.get()), 'b')


class QueuePutTests(_QueueTestBase):

    def test_blocking_put(self):
        q = queues.Queue(loop=self.loop)

        @tasks.coroutine
        def queue_put():
            # No maxsize, won't block.
            yield from q.put(1)

        self.loop.run_until_complete(queue_put())

    def test_blocking_put_wait(self):

        def gen():
            when = yield
            self.assertAlmostEqual(0.01, when)
            yield 0.01

        loop = test_utils.TestLoop(gen)
        self.addCleanup(loop.close)

        q = queues.Queue(maxsize=1, loop=loop)
        started = locks.Event(loop=loop)
        finished = False

        @tasks.coroutine
        def queue_put():
            nonlocal finished
            started.set()
            yield from q.put(1)
            yield from q.put(2)
            finished = True

        @tasks.coroutine
        def queue_get():
            loop.call_later(0.01, q.get_nowait)
            queue_put_task = tasks.Task(queue_put(), loop=loop)
            yield from started.wait()
            self.assertFalse(finished)
            yield from queue_put_task
            self.assertTrue(finished)

        loop.run_until_complete(queue_get())
        self.assertAlmostEqual(0.01, loop.time())

    def test_nonblocking_put(self):
        q = queues.Queue(loop=self.loop)
        q.put_nowait(1)
        self.assertEqual(1, q.get_nowait())

    def test_nonblocking_put_exception(self):
        q = queues.Queue(maxsize=1, loop=self.loop)
        q.put_nowait(1)
        self.assertRaises(queues.Full, q.put_nowait, 2)

    def test_put_cancelled(self):
        q = queues.Queue(loop=self.loop)

        @tasks.coroutine
        def queue_put():
            yield from q.put(1)
            return True

        @tasks.coroutine
        def test():
            return (yield from q.get())

        t = tasks.Task(queue_put(), loop=self.loop)
        self.assertEqual(1, self.loop.run_until_complete(test()))
        self.assertTrue(t.done())
        self.assertTrue(t.result())

    def test_put_cancelled_race(self):
        q = queues.Queue(loop=self.loop, maxsize=1)

        tasks.Task(q.put('a'), loop=self.loop)
        tasks.Task(q.put('c'), loop=self.loop)
        t = tasks.Task(q.put('b'), loop=self.loop)

        test_utils.run_briefly(self.loop)
        t.cancel()
        test_utils.run_briefly(self.loop)
        self.assertTrue(t.done())
        self.assertEqual(q.get_nowait(), 'a')
        self.assertEqual(q.get_nowait(), 'c')

    def test_put_with_waiting_getters(self):
        q = queues.Queue(loop=self.loop)
        t = tasks.Task(q.get(), loop=self.loop)
        test_utils.run_briefly(self.loop)
        self.loop.run_until_complete(q.put('a'))
        self.assertEqual(self.loop.run_until_complete(t), 'a')


class LifoQueueTests(_QueueTestBase):

    def test_order(self):
        q = queues.LifoQueue(loop=self.loop)
        for i in [1, 3, 2]:
            q.put_nowait(i)

        items = [q.get_nowait() for _ in range(3)]
        self.assertEqual([2, 3, 1], items)


class PriorityQueueTests(_QueueTestBase):

    def test_order(self):
        q = queues.PriorityQueue(loop=self.loop)
        for i in [1, 3, 2]:
            q.put_nowait(i)

        items = [q.get_nowait() for _ in range(3)]
        self.assertEqual([1, 2, 3], items)


class JoinableQueueTests(_QueueTestBase):

    def test_task_done_underflow(self):
        q = queues.JoinableQueue(loop=self.loop)
        self.assertRaises(ValueError, q.task_done)

    def test_task_done(self):
        q = queues.JoinableQueue(loop=self.loop)
        for i in range(100):
            q.put_nowait(i)

        accumulator = 0

        # Two workers get items from the queue and call task_done after each.
        # Join the queue and assert all items have been processed.
        running = True

        @tasks.coroutine
        def worker():
            nonlocal accumulator

            while running:
                item = yield from q.get()
                accumulator += item
                q.task_done()

        @tasks.coroutine
        def test():
            for _ in range(2):
                tasks.Task(worker(), loop=self.loop)

            yield from q.join()

        self.loop.run_until_complete(test())
        self.assertEqual(sum(range(100)), accumulator)

        # close running generators
        running = False
        for i in range(2):
            q.put_nowait(0)

    def test_join_empty_queue(self):
        q = queues.JoinableQueue(loop=self.loop)

        # Test that a queue join()s successfully, and before anything else
        # (done twice for insurance).

        @tasks.coroutine
        def join():
            yield from q.join()
            yield from q.join()

        self.loop.run_until_complete(join())

    def test_format(self):
        q = queues.JoinableQueue(loop=self.loop)
        self.assertEqual(q._format(), 'maxsize=0')

        q._unfinished_tasks = 2
        self.assertEqual(q._format(), 'maxsize=0 tasks=2')


if __name__ == '__main__':
    unittest.main()
