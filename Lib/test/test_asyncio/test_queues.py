"""Tests for queues.py"""

import unittest
import asyncio
from types import GenericAlias
from test.test_asyncio import utils as test_utils


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class _QueueTestBase(test_utils.TestCase):

    def setUp(self):
        super().setUp()
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

        q = asyncio.Queue()
        self.assertTrue(fn(q).startswith('<Queue'), fn(q))
        id_is_present = hex(id(q)) in fn(q)
        self.assertEqual(expect_id, id_is_present)

        async def add_getter():
            q = asyncio.Queue()
            # Start a task that waits to get.
            loop.create_task(q.get())
            # Let it start waiting.
            await asyncio.sleep(0.1)
            self.assertTrue('_getters[1]' in fn(q))
            # resume q.get coroutine to finish generator
            q.put_nowait(0)

        loop.run_until_complete(add_getter())

        async def add_putter():
            q = asyncio.Queue(maxsize=1)
            q.put_nowait(1)
            # Start a task that waits to put.
            loop.create_task(q.put(2))
            # Let it start waiting.
            await asyncio.sleep(0.1)
            self.assertTrue('_putters[1]' in fn(q))
            # resume q.put coroutine to finish generator
            q.get_nowait()

        loop.run_until_complete(add_putter())
        q = asyncio.Queue()
        q.put_nowait(1)
        self.assertTrue('_queue=[1]' in fn(q))

    def test_repr(self):
        self._test_repr_or_str(repr, True)

    def test_str(self):
        self._test_repr_or_str(str, False)

    def test_generic_alias(self):
        q = asyncio.Queue[int]
        self.assertEqual(q.__args__, (int,))
        self.assertIsInstance(q, GenericAlias)

    def test_empty(self):
        q = asyncio.Queue()
        self.assertTrue(q.empty())
        q.put_nowait(1)
        self.assertFalse(q.empty())
        self.assertEqual(1, q.get_nowait())
        self.assertTrue(q.empty())

    def test_full(self):
        q = asyncio.Queue()
        self.assertFalse(q.full())

        q = asyncio.Queue(maxsize=1)
        q.put_nowait(1)
        self.assertTrue(q.full())

    def test_order(self):
        q = asyncio.Queue()
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

        q = asyncio.Queue(maxsize=2)
        self.assertEqual(2, q.maxsize)
        have_been_put = []

        async def putter():
            for i in range(3):
                await q.put(i)
                have_been_put.append(i)
            return True

        async def test():
            t = loop.create_task(putter())
            await asyncio.sleep(0.01)

            # The putter is blocked after putting two items.
            self.assertEqual([0, 1], have_been_put)
            self.assertEqual(0, q.get_nowait())

            # Let the putter resume and put last item.
            await asyncio.sleep(0.01)
            self.assertEqual([0, 1, 2], have_been_put)
            self.assertEqual(1, q.get_nowait())
            self.assertEqual(2, q.get_nowait())

            self.assertTrue(t.done())
            self.assertTrue(t.result())

        loop.run_until_complete(test())
        self.assertAlmostEqual(0.02, loop.time())


class QueueGetTests(_QueueTestBase):

    def test_blocking_get(self):
        q = asyncio.Queue()
        q.put_nowait(1)

        async def queue_get():
            return await q.get()

        res = self.loop.run_until_complete(queue_get())
        self.assertEqual(1, res)

    def test_get_with_putters(self):
        q = asyncio.Queue(1)
        q.put_nowait(1)

        waiter = self.loop.create_future()
        q._putters.append(waiter)

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

        q = asyncio.Queue()
        started = asyncio.Event()
        finished = False

        async def queue_get():
            nonlocal finished
            started.set()
            res = await q.get()
            finished = True
            return res

        async def queue_put():
            loop.call_later(0.01, q.put_nowait, 1)
            queue_get_task = loop.create_task(queue_get())
            await started.wait()
            self.assertFalse(finished)
            res = await queue_get_task
            self.assertTrue(finished)
            return res

        res = loop.run_until_complete(queue_put())
        self.assertEqual(1, res)
        self.assertAlmostEqual(0.01, loop.time())

    def test_nonblocking_get(self):
        q = asyncio.Queue()
        q.put_nowait(1)
        self.assertEqual(1, q.get_nowait())

    def test_nonblocking_get_exception(self):
        q = asyncio.Queue()
        self.assertRaises(asyncio.QueueEmpty, q.get_nowait)

    def test_get_cancelled(self):

        def gen():
            when = yield
            self.assertAlmostEqual(0.01, when)
            when = yield 0.01
            self.assertAlmostEqual(0.061, when)
            yield 0.05

        loop = self.new_test_loop(gen)

        q = asyncio.Queue()

        async def queue_get():
            return await asyncio.wait_for(q.get(), 0.051)

        async def test():
            get_task = loop.create_task(queue_get())
            await asyncio.sleep(0.01)  # let the task start
            q.put_nowait(1)
            return await get_task

        self.assertEqual(1, loop.run_until_complete(test()))
        self.assertAlmostEqual(0.06, loop.time())

    def test_get_cancelled_race(self):
        q = asyncio.Queue()

        t1 = self.loop.create_task(q.get())
        t2 = self.loop.create_task(q.get())

        test_utils.run_briefly(self.loop)
        t1.cancel()
        test_utils.run_briefly(self.loop)
        self.assertTrue(t1.done())
        q.put_nowait('a')
        test_utils.run_briefly(self.loop)
        self.assertEqual(t2.result(), 'a')

    def test_get_with_waiting_putters(self):
        q = asyncio.Queue(maxsize=1)
        self.loop.create_task(q.put('a'))
        self.loop.create_task(q.put('b'))
        test_utils.run_briefly(self.loop)
        self.assertEqual(self.loop.run_until_complete(q.get()), 'a')
        self.assertEqual(self.loop.run_until_complete(q.get()), 'b')

    def test_why_are_getters_waiting(self):
        # From issue #268.
        asyncio.set_event_loop(self.loop)

        async def consumer(queue, num_expected):
            for _ in range(num_expected):
                await queue.get()

        async def producer(queue, num_items):
            for i in range(num_items):
                await queue.put(i)

        queue_size = 1
        producer_num_items = 5

        async def create_queue():
            queue = asyncio.Queue(queue_size)
            queue._get_loop()
            return queue

        async def test():
            q = await create_queue()
            await asyncio.gather(producer(q, producer_num_items),
                                 consumer(q, producer_num_items))

        self.loop.run_until_complete(test())

    def test_cancelled_getters_not_being_held_in_self_getters(self):
        def a_generator():
            yield 0.1
            yield 0.2

        self.loop = self.new_test_loop(a_generator)

        async def consumer(queue):
            try:
                item = await asyncio.wait_for(queue.get(), 0.1)
            except asyncio.TimeoutError:
                pass

        queue = asyncio.Queue(maxsize=5)
        self.loop.run_until_complete(self.loop.create_task(consumer(queue)))
        self.assertEqual(len(queue._getters), 0)


class QueuePutTests(_QueueTestBase):

    def test_blocking_put(self):
        q = asyncio.Queue()

        async def queue_put():
            # No maxsize, won't block.
            await q.put(1)

        self.loop.run_until_complete(queue_put())

    def test_blocking_put_wait(self):

        def gen():
            when = yield
            self.assertAlmostEqual(0.01, when)
            yield 0.01

        loop = self.new_test_loop(gen)

        q = asyncio.Queue(maxsize=1)
        started = asyncio.Event()
        finished = False

        async def queue_put():
            nonlocal finished
            started.set()
            await q.put(1)
            await q.put(2)
            finished = True

        async def queue_get():
            loop.call_later(0.01, q.get_nowait)
            queue_put_task = loop.create_task(queue_put())
            await started.wait()
            self.assertFalse(finished)
            await queue_put_task
            self.assertTrue(finished)

        loop.run_until_complete(queue_get())
        self.assertAlmostEqual(0.01, loop.time())

    def test_nonblocking_put(self):
        q = asyncio.Queue()
        q.put_nowait(1)
        self.assertEqual(1, q.get_nowait())

    def test_get_cancel_drop_one_pending_reader(self):
        def gen():
            yield 0.01
            yield 0.1

        loop = self.new_test_loop(gen)

        q = asyncio.Queue()

        reader = loop.create_task(q.get())

        loop.run_until_complete(asyncio.sleep(0.01))

        q.put_nowait(1)
        q.put_nowait(2)
        reader.cancel()

        try:
            loop.run_until_complete(reader)
        except asyncio.CancelledError:
            # try again
            reader = loop.create_task(q.get())
            loop.run_until_complete(reader)

        result = reader.result()
        # if we get 2, it means 1 got dropped!
        self.assertEqual(1, result)

    def test_get_cancel_drop_many_pending_readers(self):
        def gen():
            yield 0.01
            yield 0.1

        loop = self.new_test_loop(gen)
        loop.set_debug(True)

        q = asyncio.Queue()

        reader1 = loop.create_task(q.get())
        reader2 = loop.create_task(q.get())
        reader3 = loop.create_task(q.get())

        loop.run_until_complete(asyncio.sleep(0.01))

        q.put_nowait(1)
        q.put_nowait(2)
        reader1.cancel()

        try:
            loop.run_until_complete(reader1)
        except asyncio.CancelledError:
            pass

        loop.run_until_complete(reader3)

        # It is undefined in which order concurrent readers receive results.
        self.assertEqual({reader2.result(), reader3.result()}, {1, 2})

    def test_put_cancel_drop(self):

        def gen():
            yield 0.01
            yield 0.1

        loop = self.new_test_loop(gen)

        q = asyncio.Queue(1)

        q.put_nowait(1)

        # putting a second item in the queue has to block (qsize=1)
        writer = loop.create_task(q.put(2))
        loop.run_until_complete(asyncio.sleep(0.01))

        value1 = q.get_nowait()
        self.assertEqual(value1, 1)

        writer.cancel()
        try:
            loop.run_until_complete(writer)
        except asyncio.CancelledError:
            # try again
            writer = loop.create_task(q.put(2))
            loop.run_until_complete(writer)

        value2 = q.get_nowait()
        self.assertEqual(value2, 2)
        self.assertEqual(q.qsize(), 0)

    def test_nonblocking_put_exception(self):
        q = asyncio.Queue(maxsize=1, )
        q.put_nowait(1)
        self.assertRaises(asyncio.QueueFull, q.put_nowait, 2)

    def test_float_maxsize(self):
        q = asyncio.Queue(maxsize=1.3, )
        q.put_nowait(1)
        q.put_nowait(2)
        self.assertTrue(q.full())
        self.assertRaises(asyncio.QueueFull, q.put_nowait, 3)

        q = asyncio.Queue(maxsize=1.3, )

        async def queue_put():
            await q.put(1)
            await q.put(2)
            self.assertTrue(q.full())
        self.loop.run_until_complete(queue_put())

    def test_put_cancelled(self):
        q = asyncio.Queue()

        async def queue_put():
            await q.put(1)
            return True

        async def test():
            return await q.get()

        t = self.loop.create_task(queue_put())
        self.assertEqual(1, self.loop.run_until_complete(test()))
        self.assertTrue(t.done())
        self.assertTrue(t.result())

    def test_put_cancelled_race(self):
        q = asyncio.Queue(maxsize=1)

        put_a = self.loop.create_task(q.put('a'))
        put_b = self.loop.create_task(q.put('b'))
        put_c = self.loop.create_task(q.put('X'))

        test_utils.run_briefly(self.loop)
        self.assertTrue(put_a.done())
        self.assertFalse(put_b.done())

        put_c.cancel()
        test_utils.run_briefly(self.loop)
        self.assertTrue(put_c.done())
        self.assertEqual(q.get_nowait(), 'a')
        test_utils.run_briefly(self.loop)
        self.assertEqual(q.get_nowait(), 'b')

        self.loop.run_until_complete(put_b)

    def test_put_with_waiting_getters(self):
        q = asyncio.Queue()
        t = self.loop.create_task(q.get())
        test_utils.run_briefly(self.loop)
        self.loop.run_until_complete(q.put('a'))
        self.assertEqual(self.loop.run_until_complete(t), 'a')

    def test_why_are_putters_waiting(self):
        # From issue #265.
        asyncio.set_event_loop(self.loop)

        async def create_queue():
            q = asyncio.Queue(2)
            q._get_loop()
            return q

        queue = self.loop.run_until_complete(create_queue())

        async def putter(item):
            await queue.put(item)

        async def getter():
            await asyncio.sleep(0)
            num = queue.qsize()
            for _ in range(num):
                item = queue.get_nowait()

        async def test():
            t0 = putter(0)
            t1 = putter(1)
            t2 = putter(2)
            t3 = putter(3)
            await asyncio.gather(getter(), t0, t1, t2, t3)

        self.loop.run_until_complete(test())

    def test_cancelled_puts_not_being_held_in_self_putters(self):
        def a_generator():
            yield 0.01
            yield 0.1

        loop = self.new_test_loop(a_generator)

        # Full queue.
        queue = asyncio.Queue(maxsize=1)
        queue.put_nowait(1)

        # Task waiting for space to put an item in the queue.
        put_task = loop.create_task(queue.put(1))
        loop.run_until_complete(asyncio.sleep(0.01))

        # Check that the putter is correctly removed from queue._putters when
        # the task is canceled.
        self.assertEqual(len(queue._putters), 1)
        put_task.cancel()
        with self.assertRaises(asyncio.CancelledError):
            loop.run_until_complete(put_task)
        self.assertEqual(len(queue._putters), 0)

    def test_cancelled_put_silence_value_error_exception(self):
        def gen():
            yield 0.01
            yield 0.1

        loop = self.new_test_loop(gen)

        # Full Queue.
        queue = asyncio.Queue(1)
        queue.put_nowait(1)

        # Task waiting for space to put a item in the queue.
        put_task = loop.create_task(queue.put(1))
        loop.run_until_complete(asyncio.sleep(0.01))

        # get_nowait() remove the future of put_task from queue._putters.
        queue.get_nowait()
        # When canceled, queue.put is going to remove its future from
        # self._putters but it was removed previously by queue.get_nowait().
        put_task.cancel()

        # The ValueError exception triggered by queue._putters.remove(putter)
        # inside queue.put should be silenced.
        # If the ValueError is silenced we should catch a CancelledError.
        with self.assertRaises(asyncio.CancelledError):
            loop.run_until_complete(put_task)


class LifoQueueTests(_QueueTestBase):

    def test_order(self):
        q = asyncio.LifoQueue()
        for i in [1, 3, 2]:
            q.put_nowait(i)

        items = [q.get_nowait() for _ in range(3)]
        self.assertEqual([2, 3, 1], items)


class PriorityQueueTests(_QueueTestBase):

    def test_order(self):
        q = asyncio.PriorityQueue()
        for i in [1, 3, 2]:
            q.put_nowait(i)

        items = [q.get_nowait() for _ in range(3)]
        self.assertEqual([1, 2, 3], items)


class _QueueJoinTestMixin:

    q_class = None

    def test_task_done_underflow(self):
        q = self.q_class()
        self.assertRaises(ValueError, q.task_done)

    def test_task_done(self):
        q = self.q_class()
        for i in range(100):
            q.put_nowait(i)

        accumulator = 0

        # Two workers get items from the queue and call task_done after each.
        # Join the queue and assert all items have been processed.
        running = True

        async def worker():
            nonlocal accumulator

            while running:
                item = await q.get()
                accumulator += item
                q.task_done()

        async def test():
            tasks = [self.loop.create_task(worker())
                     for index in range(2)]

            await q.join()
            return tasks

        tasks = self.loop.run_until_complete(test())
        self.assertEqual(sum(range(100)), accumulator)

        # close running generators
        running = False
        for i in range(len(tasks)):
            q.put_nowait(0)
        self.loop.run_until_complete(asyncio.wait(tasks))

    def test_join_empty_queue(self):
        q = self.q_class()

        # Test that a queue join()s successfully, and before anything else
        # (done twice for insurance).

        async def join():
            await q.join()
            await q.join()

        self.loop.run_until_complete(join())

    def test_format(self):
        q = self.q_class()
        self.assertEqual(q._format(), 'maxsize=0')

        q._unfinished_tasks = 2
        self.assertEqual(q._format(), 'maxsize=0 tasks=2')


class QueueJoinTests(_QueueJoinTestMixin, _QueueTestBase):
    q_class = asyncio.Queue


class LifoQueueJoinTests(_QueueJoinTestMixin, _QueueTestBase):
    q_class = asyncio.LifoQueue


class PriorityQueueJoinTests(_QueueJoinTestMixin, _QueueTestBase):
    q_class = asyncio.PriorityQueue


if __name__ == '__main__':
    unittest.main()
