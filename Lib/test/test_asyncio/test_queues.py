"""Tests for queues.py"""

import unittest
from unittest import mock

import asyncio
from asyncio import test_utils


class _QueueTestBase(test_utils.TestCase):

    def setUp(self):
        self.loop = self.new_test_loop()


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

        loop = self.new_test_loop(gen)

        q = asyncio.Queue(loop=loop)
        self.assertTrue(fn(q).startswith('<Queue'), fn(q))
        id_is_present = hex(id(q)) in fn(q)
        self.assertEqual(expect_id, id_is_present)

        @asyncio.coroutine
        def add_getter():
            q = asyncio.Queue(loop=loop)
            # Start a task that waits to get.
            asyncio.Task(q.get(), loop=loop)
            # Let it start waiting.
            yield from asyncio.sleep(0.1, loop=loop)
            self.assertTrue('_getters[1]' in fn(q))
            # resume q.get coroutine to finish generator
            q.put_nowait(0)

        loop.run_until_complete(add_getter())

        @asyncio.coroutine
        def add_putter():
            q = asyncio.Queue(maxsize=1, loop=loop)
            q.put_nowait(1)
            # Start a task that waits to put.
            asyncio.Task(q.put(2), loop=loop)
            # Let it start waiting.
            yield from asyncio.sleep(0.1, loop=loop)
            self.assertTrue('_putters[1]' in fn(q))
            # resume q.put coroutine to finish generator
            q.get_nowait()

        loop.run_until_complete(add_putter())

        q = asyncio.Queue(loop=loop)
        q.put_nowait(1)
        self.assertTrue('_queue=[1]' in fn(q))

    def test_ctor_loop(self):
        loop = mock.Mock()
        q = asyncio.Queue(loop=loop)
        self.assertIs(q._loop, loop)

        q = asyncio.Queue(loop=self.loop)
        self.assertIs(q._loop, self.loop)

    def test_ctor_noloop(self):
        asyncio.set_event_loop(self.loop)
        q = asyncio.Queue()
        self.assertIs(q._loop, self.loop)

    def test_repr(self):
        self._test_repr_or_str(repr, True)

    def test_str(self):
        self._test_repr_or_str(str, False)

    def test_empty(self):
        q = asyncio.Queue(loop=self.loop)
        self.assertTrue(q.empty())
        q.put_nowait(1)
        self.assertFalse(q.empty())
        self.assertEqual(1, q.get_nowait())
        self.assertTrue(q.empty())

    def test_full(self):
        q = asyncio.Queue(loop=self.loop)
        self.assertFalse(q.full())

        q = asyncio.Queue(maxsize=1, loop=self.loop)
        q.put_nowait(1)
        self.assertTrue(q.full())

    def test_order(self):
        q = asyncio.Queue(loop=self.loop)
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

        loop = self.new_test_loop(gen)

        q = asyncio.Queue(maxsize=2, loop=loop)
        self.assertEqual(2, q.maxsize)
        have_been_put = []

        @asyncio.coroutine
        def putter():
            for i in range(3):
                yield from q.put(i)
                have_been_put.append(i)
            return True

        @asyncio.coroutine
        def test():
            t = asyncio.Task(putter(), loop=loop)
            yield from asyncio.sleep(0.01, loop=loop)

            # The putter is blocked after putting two items.
            self.assertEqual([0, 1], have_been_put)
            self.assertEqual(0, q.get_nowait())

            # Let the putter resume and put last item.
            yield from asyncio.sleep(0.01, loop=loop)
            self.assertEqual([0, 1, 2], have_been_put)
            self.assertEqual(1, q.get_nowait())
            self.assertEqual(2, q.get_nowait())

            self.assertTrue(t.done())
            self.assertTrue(t.result())

        loop.run_until_complete(test())
        self.assertAlmostEqual(0.02, loop.time())


class QueueGetTests(_QueueTestBase):

    def test_blocking_get(self):
        q = asyncio.Queue(loop=self.loop)
        q.put_nowait(1)

        @asyncio.coroutine
        def queue_get():
            return (yield from q.get())

        res = self.loop.run_until_complete(queue_get())
        self.assertEqual(1, res)

    def test_get_with_putters(self):
        q = asyncio.Queue(1, loop=self.loop)
        q.put_nowait(1)

        waiter = asyncio.Future(loop=self.loop)
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

        loop = self.new_test_loop(gen)

        q = asyncio.Queue(loop=loop)
        started = asyncio.Event(loop=loop)
        finished = False

        @asyncio.coroutine
        def queue_get():
            nonlocal finished
            started.set()
            res = yield from q.get()
            finished = True
            return res

        @asyncio.coroutine
        def queue_put():
            loop.call_later(0.01, q.put_nowait, 1)
            queue_get_task = asyncio.Task(queue_get(), loop=loop)
            yield from started.wait()
            self.assertFalse(finished)
            res = yield from queue_get_task
            self.assertTrue(finished)
            return res

        res = loop.run_until_complete(queue_put())
        self.assertEqual(1, res)
        self.assertAlmostEqual(0.01, loop.time())

    def test_nonblocking_get(self):
        q = asyncio.Queue(loop=self.loop)
        q.put_nowait(1)
        self.assertEqual(1, q.get_nowait())

    def test_nonblocking_get_exception(self):
        q = asyncio.Queue(loop=self.loop)
        self.assertRaises(asyncio.QueueEmpty, q.get_nowait)

    def test_get_cancelled(self):

        def gen():
            when = yield
            self.assertAlmostEqual(0.01, when)
            when = yield 0.01
            self.assertAlmostEqual(0.061, when)
            yield 0.05

        loop = self.new_test_loop(gen)

        q = asyncio.Queue(loop=loop)

        @asyncio.coroutine
        def queue_get():
            return (yield from asyncio.wait_for(q.get(), 0.051, loop=loop))

        @asyncio.coroutine
        def test():
            get_task = asyncio.Task(queue_get(), loop=loop)
            yield from asyncio.sleep(0.01, loop=loop)  # let the task start
            q.put_nowait(1)
            return (yield from get_task)

        self.assertEqual(1, loop.run_until_complete(test()))
        self.assertAlmostEqual(0.06, loop.time())

    def test_get_cancelled_race(self):
        q = asyncio.Queue(loop=self.loop)

        t1 = asyncio.Task(q.get(), loop=self.loop)
        t2 = asyncio.Task(q.get(), loop=self.loop)

        test_utils.run_briefly(self.loop)
        t1.cancel()
        test_utils.run_briefly(self.loop)
        self.assertTrue(t1.done())
        q.put_nowait('a')
        test_utils.run_briefly(self.loop)
        self.assertEqual(t2.result(), 'a')

    def test_get_with_waiting_putters(self):
        q = asyncio.Queue(loop=self.loop, maxsize=1)
        asyncio.Task(q.put('a'), loop=self.loop)
        asyncio.Task(q.put('b'), loop=self.loop)
        test_utils.run_briefly(self.loop)
        self.assertEqual(self.loop.run_until_complete(q.get()), 'a')
        self.assertEqual(self.loop.run_until_complete(q.get()), 'b')


class QueuePutTests(_QueueTestBase):

    def test_blocking_put(self):
        q = asyncio.Queue(loop=self.loop)

        @asyncio.coroutine
        def queue_put():
            # No maxsize, won't block.
            yield from q.put(1)

        self.loop.run_until_complete(queue_put())

    def test_blocking_put_wait(self):

        def gen():
            when = yield
            self.assertAlmostEqual(0.01, when)
            yield 0.01

        loop = self.new_test_loop(gen)

        q = asyncio.Queue(maxsize=1, loop=loop)
        started = asyncio.Event(loop=loop)
        finished = False

        @asyncio.coroutine
        def queue_put():
            nonlocal finished
            started.set()
            yield from q.put(1)
            yield from q.put(2)
            finished = True

        @asyncio.coroutine
        def queue_get():
            loop.call_later(0.01, q.get_nowait)
            queue_put_task = asyncio.Task(queue_put(), loop=loop)
            yield from started.wait()
            self.assertFalse(finished)
            yield from queue_put_task
            self.assertTrue(finished)

        loop.run_until_complete(queue_get())
        self.assertAlmostEqual(0.01, loop.time())

    def test_nonblocking_put(self):
        q = asyncio.Queue(loop=self.loop)
        q.put_nowait(1)
        self.assertEqual(1, q.get_nowait())

    def test_nonblocking_put_exception(self):
        q = asyncio.Queue(maxsize=1, loop=self.loop)
        q.put_nowait(1)
        self.assertRaises(asyncio.QueueFull, q.put_nowait, 2)

    def test_float_maxsize(self):
        q = asyncio.Queue(maxsize=1.3, loop=self.loop)
        q.put_nowait(1)
        q.put_nowait(2)
        self.assertTrue(q.full())
        self.assertRaises(asyncio.QueueFull, q.put_nowait, 3)

        q = asyncio.Queue(maxsize=1.3, loop=self.loop)
        @asyncio.coroutine
        def queue_put():
            yield from q.put(1)
            yield from q.put(2)
            self.assertTrue(q.full())
        self.loop.run_until_complete(queue_put())

    def test_put_cancelled(self):
        q = asyncio.Queue(loop=self.loop)

        @asyncio.coroutine
        def queue_put():
            yield from q.put(1)
            return True

        @asyncio.coroutine
        def test():
            return (yield from q.get())

        t = asyncio.Task(queue_put(), loop=self.loop)
        self.assertEqual(1, self.loop.run_until_complete(test()))
        self.assertTrue(t.done())
        self.assertTrue(t.result())

    def test_put_cancelled_race(self):
        q = asyncio.Queue(loop=self.loop, maxsize=1)

        put_a = asyncio.Task(q.put('a'), loop=self.loop)
        put_b = asyncio.Task(q.put('b'), loop=self.loop)
        put_c = asyncio.Task(q.put('X'), loop=self.loop)

        test_utils.run_briefly(self.loop)
        self.assertTrue(put_a.done())
        self.assertFalse(put_b.done())

        put_c.cancel()
        test_utils.run_briefly(self.loop)
        self.assertTrue(put_c.done())
        self.assertEqual(q.get_nowait(), 'a')
        self.assertEqual(q.get_nowait(), 'b')

        self.loop.run_until_complete(put_b)

    def test_put_with_waiting_getters(self):
        q = asyncio.Queue(loop=self.loop)
        t = asyncio.Task(q.get(), loop=self.loop)
        test_utils.run_briefly(self.loop)
        self.loop.run_until_complete(q.put('a'))
        self.assertEqual(self.loop.run_until_complete(t), 'a')


class LifoQueueTests(_QueueTestBase):

    def test_order(self):
        q = asyncio.LifoQueue(loop=self.loop)
        for i in [1, 3, 2]:
            q.put_nowait(i)

        items = [q.get_nowait() for _ in range(3)]
        self.assertEqual([2, 3, 1], items)


class PriorityQueueTests(_QueueTestBase):

    def test_order(self):
        q = asyncio.PriorityQueue(loop=self.loop)
        for i in [1, 3, 2]:
            q.put_nowait(i)

        items = [q.get_nowait() for _ in range(3)]
        self.assertEqual([1, 2, 3], items)


class JoinableQueueTests(_QueueTestBase):

    def test_task_done_underflow(self):
        q = asyncio.JoinableQueue(loop=self.loop)
        self.assertRaises(ValueError, q.task_done)

    def test_task_done(self):
        q = asyncio.JoinableQueue(loop=self.loop)
        for i in range(100):
            q.put_nowait(i)

        accumulator = 0

        # Two workers get items from the queue and call task_done after each.
        # Join the queue and assert all items have been processed.
        running = True

        @asyncio.coroutine
        def worker():
            nonlocal accumulator

            while running:
                item = yield from q.get()
                accumulator += item
                q.task_done()

        @asyncio.coroutine
        def test():
            tasks = [asyncio.Task(worker(), loop=self.loop)
                     for index in range(2)]

            yield from q.join()
            return tasks

        tasks = self.loop.run_until_complete(test())
        self.assertEqual(sum(range(100)), accumulator)

        # close running generators
        running = False
        for i in range(len(tasks)):
            q.put_nowait(0)
        self.loop.run_until_complete(asyncio.wait(tasks, loop=self.loop))

    def test_join_empty_queue(self):
        q = asyncio.JoinableQueue(loop=self.loop)

        # Test that a queue join()s successfully, and before anything else
        # (done twice for insurance).

        @asyncio.coroutine
        def join():
            yield from q.join()
            yield from q.join()

        self.loop.run_until_complete(join())

    def test_format(self):
        q = asyncio.JoinableQueue(loop=self.loop)
        self.assertEqual(q._format(), 'maxsize=0')

        q._unfinished_tasks = 2
        self.assertEqual(q._format(), 'maxsize=0 tasks=2')


if __name__ == '__main__':
    unittest.main()
