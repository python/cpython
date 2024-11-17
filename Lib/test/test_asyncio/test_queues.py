"""Tests for queues.py"""

import asyncio
import unittest
from types import GenericAlias


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class QueueBasicTests(unittest.IsolatedAsyncioTestCase):

    async def _test_repr_or_str(self, fn, expect_id):
        """Test Queue's repr or str.

        fn is repr or str. expect_id is True if we expect the Queue's id to
        appear in fn(Queue()).
        """
        q = asyncio.Queue()
        self.assertTrue(fn(q).startswith('<Queue'), fn(q))
        id_is_present = hex(id(q)) in fn(q)
        self.assertEqual(expect_id, id_is_present)

        # getters
        q = asyncio.Queue()
        async with asyncio.TaskGroup() as tg:
            # Start a task that waits to get.
            getter = tg.create_task(q.get())
            # Let it start waiting.
            await asyncio.sleep(0)
            self.assertTrue('_getters[1]' in fn(q))
            # resume q.get coroutine to finish generator
            q.put_nowait(0)

        self.assertEqual(0, await getter)

        # putters
        q = asyncio.Queue(maxsize=1)
        async with asyncio.TaskGroup() as tg:
            q.put_nowait(1)
            # Start a task that waits to put.
            putter = tg.create_task(q.put(2))
            # Let it start waiting.
            await asyncio.sleep(0)
            self.assertTrue('_putters[1]' in fn(q))
            # resume q.put coroutine to finish generator
            q.get_nowait()

        self.assertTrue(putter.done())

        q = asyncio.Queue()
        q.put_nowait(1)
        self.assertTrue('_queue=[1]' in fn(q))

    async def test_repr(self):
        await self._test_repr_or_str(repr, True)

    async def test_str(self):
        await self._test_repr_or_str(str, False)

    def test_generic_alias(self):
        q = asyncio.Queue[int]
        self.assertEqual(q.__args__, (int,))
        self.assertIsInstance(q, GenericAlias)

    async def test_empty(self):
        q = asyncio.Queue()
        self.assertTrue(q.empty())
        await q.put(1)
        self.assertFalse(q.empty())
        self.assertEqual(1, await q.get())
        self.assertTrue(q.empty())

    async def test_full(self):
        q = asyncio.Queue()
        self.assertFalse(q.full())

        q = asyncio.Queue(maxsize=1)
        await q.put(1)
        self.assertTrue(q.full())

    async def test_order(self):
        q = asyncio.Queue()
        for i in [1, 3, 2]:
            await q.put(i)

        items = [await q.get() for _ in range(3)]
        self.assertEqual([1, 3, 2], items)

    async def test_maxsize(self):
        q = asyncio.Queue(maxsize=2)
        self.assertEqual(2, q.maxsize)
        have_been_put = []

        async def putter():
            for i in range(3):
                await q.put(i)
                have_been_put.append(i)
            return True

        t = asyncio.create_task(putter())
        for i in range(2):
            await asyncio.sleep(0)

        # The putter is blocked after putting two items.
        self.assertEqual([0, 1], have_been_put)
        self.assertEqual(0, await q.get())

        # Let the putter resume and put last item.
        await asyncio.sleep(0)
        self.assertEqual([0, 1, 2], have_been_put)
        self.assertEqual(1, await q.get())
        self.assertEqual(2, await q.get())

        self.assertTrue(t.done())
        self.assertTrue(t.result())


class QueueGetTests(unittest.IsolatedAsyncioTestCase):

    async def test_blocking_get(self):
        q = asyncio.Queue()
        q.put_nowait(1)

        self.assertEqual(1, await q.get())

    async def test_get_with_putters(self):
        loop = asyncio.get_running_loop()

        q = asyncio.Queue(1)
        await q.put(1)

        waiter = loop.create_future()
        q._putters.append(waiter)

        self.assertEqual(1, await q.get())
        self.assertTrue(waiter.done())
        self.assertIsNone(waiter.result())

    async def test_blocking_get_wait(self):
        loop = asyncio.get_running_loop()
        q = asyncio.Queue()
        started = asyncio.Event()
        finished = False

        async def queue_get():
            nonlocal finished
            started.set()
            res = await q.get()
            finished = True
            return res

        queue_get_task = asyncio.create_task(queue_get())
        await started.wait()
        self.assertFalse(finished)
        loop.call_later(0.01, q.put_nowait, 1)
        res = await queue_get_task
        self.assertTrue(finished)
        self.assertEqual(1, res)

    def test_nonblocking_get(self):
        q = asyncio.Queue()
        q.put_nowait(1)
        self.assertEqual(1, q.get_nowait())

    def test_nonblocking_get_exception(self):
        q = asyncio.Queue()
        self.assertRaises(asyncio.QueueEmpty, q.get_nowait)

    async def test_get_cancelled_race(self):
        q = asyncio.Queue()

        t1 = asyncio.create_task(q.get())
        t2 = asyncio.create_task(q.get())

        await asyncio.sleep(0)
        t1.cancel()
        await asyncio.sleep(0)
        self.assertTrue(t1.done())
        await q.put('a')
        await asyncio.sleep(0)
        self.assertEqual('a', await t2)

    async def test_get_with_waiting_putters(self):
        q = asyncio.Queue(maxsize=1)
        asyncio.create_task(q.put('a'))
        asyncio.create_task(q.put('b'))
        self.assertEqual(await q.get(), 'a')
        self.assertEqual(await q.get(), 'b')

    async def test_why_are_getters_waiting(self):
        async def consumer(queue, num_expected):
            for _ in range(num_expected):
                await queue.get()

        async def producer(queue, num_items):
            for i in range(num_items):
                await queue.put(i)

        producer_num_items = 5

        q = asyncio.Queue(1)
        async with asyncio.TaskGroup() as tg:
            tg.create_task(producer(q, producer_num_items))
            tg.create_task(consumer(q, producer_num_items))

    async def test_cancelled_getters_not_being_held_in_self_getters(self):
        queue = asyncio.Queue(maxsize=5)

        with self.assertRaises(TimeoutError):
            await asyncio.wait_for(queue.get(), 0.1)

        self.assertEqual(len(queue._getters), 0)


class QueuePutTests(unittest.IsolatedAsyncioTestCase):

    async def test_blocking_put(self):
        q = asyncio.Queue()

        # No maxsize, won't block.
        await q.put(1)
        self.assertEqual(1, await q.get())

    async def test_blocking_put_wait(self):
        q = asyncio.Queue(maxsize=1)
        started = asyncio.Event()
        finished = False

        async def queue_put():
            nonlocal finished
            started.set()
            await q.put(1)
            await q.put(2)
            finished = True

        loop = asyncio.get_running_loop()
        loop.call_later(0.01, q.get_nowait)
        queue_put_task = asyncio.create_task(queue_put())
        await started.wait()
        self.assertFalse(finished)
        await queue_put_task
        self.assertTrue(finished)

    def test_nonblocking_put(self):
        q = asyncio.Queue()
        q.put_nowait(1)
        self.assertEqual(1, q.get_nowait())

    async def test_get_cancel_drop_one_pending_reader(self):
        q = asyncio.Queue()

        reader = asyncio.create_task(q.get())

        await asyncio.sleep(0)

        q.put_nowait(1)
        q.put_nowait(2)
        reader.cancel()

        try:
            await reader
        except asyncio.CancelledError:
            # try again
            reader = asyncio.create_task(q.get())
            await reader

        result = reader.result()
        # if we get 2, it means 1 got dropped!
        self.assertEqual(1, result)

    async def test_get_cancel_drop_many_pending_readers(self):
        q = asyncio.Queue()

        async with asyncio.TaskGroup() as tg:
            reader1 = tg.create_task(q.get())
            reader2 = tg.create_task(q.get())
            reader3 = tg.create_task(q.get())

            await asyncio.sleep(0)

            q.put_nowait(1)
            q.put_nowait(2)
            reader1.cancel()

            with self.assertRaises(asyncio.CancelledError):
                await reader1

            await reader3

        # It is undefined in which order concurrent readers receive results.
        self.assertEqual({reader2.result(), reader3.result()}, {1, 2})

    async def test_put_cancel_drop(self):
        q = asyncio.Queue(1)

        q.put_nowait(1)

        # putting a second item in the queue has to block (qsize=1)
        writer = asyncio.create_task(q.put(2))
        await asyncio.sleep(0)

        value1 = q.get_nowait()
        self.assertEqual(value1, 1)

        writer.cancel()
        try:
            await writer
        except asyncio.CancelledError:
            # try again
            writer = asyncio.create_task(q.put(2))
            await writer

        value2 = q.get_nowait()
        self.assertEqual(value2, 2)
        self.assertEqual(q.qsize(), 0)

    def test_nonblocking_put_exception(self):
        q = asyncio.Queue(maxsize=1, )
        q.put_nowait(1)
        self.assertRaises(asyncio.QueueFull, q.put_nowait, 2)

    async def test_float_maxsize(self):
        q = asyncio.Queue(maxsize=1.3, )
        q.put_nowait(1)
        q.put_nowait(2)
        self.assertTrue(q.full())
        self.assertRaises(asyncio.QueueFull, q.put_nowait, 3)

        q = asyncio.Queue(maxsize=1.3, )

        await q.put(1)
        await q.put(2)
        self.assertTrue(q.full())

    async def test_put_cancelled(self):
        q = asyncio.Queue()

        async def queue_put():
            await q.put(1)
            return True

        t = asyncio.create_task(queue_put())

        self.assertEqual(1, await q.get())
        self.assertTrue(t.done())
        self.assertTrue(t.result())

    async def test_put_cancelled_race(self):
        q = asyncio.Queue(maxsize=1)

        put_a = asyncio.create_task(q.put('a'))
        put_b = asyncio.create_task(q.put('b'))
        put_c = asyncio.create_task(q.put('X'))

        await asyncio.sleep(0)
        self.assertTrue(put_a.done())
        self.assertFalse(put_b.done())

        put_c.cancel()
        await asyncio.sleep(0)
        self.assertTrue(put_c.done())
        self.assertEqual(q.get_nowait(), 'a')
        await asyncio.sleep(0)
        self.assertEqual(q.get_nowait(), 'b')

        await put_b

    async def test_put_with_waiting_getters(self):
        q = asyncio.Queue()
        t = asyncio.create_task(q.get())
        await asyncio.sleep(0)
        await q.put('a')
        self.assertEqual(await t, 'a')

    async def test_why_are_putters_waiting(self):
        queue = asyncio.Queue(2)

        async def putter(item):
            await queue.put(item)

        async def getter():
            await asyncio.sleep(0)
            num = queue.qsize()
            for _ in range(num):
                queue.get_nowait()

        async with asyncio.TaskGroup() as tg:
            tg.create_task(getter())
            tg.create_task(putter(0))
            tg.create_task(putter(1))
            tg.create_task(putter(2))
            tg.create_task(putter(3))

    async def test_cancelled_puts_not_being_held_in_self_putters(self):
        # Full queue.
        queue = asyncio.Queue(maxsize=1)
        queue.put_nowait(1)

        # Task waiting for space to put an item in the queue.
        put_task = asyncio.create_task(queue.put(1))
        await asyncio.sleep(0)

        # Check that the putter is correctly removed from queue._putters when
        # the task is canceled.
        self.assertEqual(len(queue._putters), 1)
        put_task.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await put_task
        self.assertEqual(len(queue._putters), 0)

    async def test_cancelled_put_silence_value_error_exception(self):
        # Full Queue.
        queue = asyncio.Queue(1)
        queue.put_nowait(1)

        # Task waiting for space to put a item in the queue.
        put_task = asyncio.create_task(queue.put(1))
        await asyncio.sleep(0)

        # get_nowait() remove the future of put_task from queue._putters.
        queue.get_nowait()
        # When canceled, queue.put is going to remove its future from
        # self._putters but it was removed previously by queue.get_nowait().
        put_task.cancel()

        # The ValueError exception triggered by queue._putters.remove(putter)
        # inside queue.put should be silenced.
        # If the ValueError is silenced we should catch a CancelledError.
        with self.assertRaises(asyncio.CancelledError):
            await put_task


class LifoQueueTests(unittest.IsolatedAsyncioTestCase):

    async def test_order(self):
        q = asyncio.LifoQueue()
        for i in [1, 3, 2]:
            await q.put(i)

        items = [await q.get() for _ in range(3)]
        self.assertEqual([2, 3, 1], items)


class PriorityQueueTests(unittest.IsolatedAsyncioTestCase):

    async def test_order(self):
        q = asyncio.PriorityQueue()
        for i in [1, 3, 2]:
            await q.put(i)

        items = [await q.get() for _ in range(3)]
        self.assertEqual([1, 2, 3], items)


class _QueueJoinTestMixin:

    q_class = None

    def test_task_done_underflow(self):
        q = self.q_class()
        self.assertRaises(ValueError, q.task_done)

    async def test_task_done(self):
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

        async with asyncio.TaskGroup() as tg:
            tasks = [tg.create_task(worker())
                     for index in range(2)]

            await q.join()
            self.assertEqual(sum(range(100)), accumulator)

            # close running generators
            running = False
            for i in range(len(tasks)):
                q.put_nowait(0)

    async def test_join_empty_queue(self):
        q = self.q_class()

        # Test that a queue join()s successfully, and before anything else
        # (done twice for insurance).

        await q.join()
        await q.join()

    async def test_format(self):
        q = self.q_class()
        self.assertEqual(q._format(), 'maxsize=0')

        q._unfinished_tasks = 2
        self.assertEqual(q._format(), 'maxsize=0 tasks=2')


class QueueJoinTests(_QueueJoinTestMixin, unittest.IsolatedAsyncioTestCase):
    q_class = asyncio.Queue


class LifoQueueJoinTests(_QueueJoinTestMixin, unittest.IsolatedAsyncioTestCase):
    q_class = asyncio.LifoQueue


class PriorityQueueJoinTests(_QueueJoinTestMixin, unittest.IsolatedAsyncioTestCase):
    q_class = asyncio.PriorityQueue


class _QueueShutdownTestMixin:
    q_class = None

    def assertRaisesShutdown(self, msg="Didn't appear to shut-down queue"):
        return self.assertRaises(asyncio.QueueShutDown, msg=msg)

    async def test_format(self):
        q = self.q_class()
        q.shutdown()
        self.assertEqual(q._format(), 'maxsize=0 shutdown')

    async def test_shutdown_empty(self):
        # Test shutting down an empty queue

        # Setup empty queue, and join() and get() tasks
        q = self.q_class()
        loop = asyncio.get_running_loop()
        get_task = loop.create_task(q.get())
        await asyncio.sleep(0)  # want get task pending before shutdown

        # Perform shut-down
        q.shutdown(immediate=False)  # unfinished tasks: 0 -> 0

        self.assertEqual(q.qsize(), 0)

        # Ensure join() task successfully finishes
        await q.join()

        # Ensure get() task is finished, and raised ShutDown
        await asyncio.sleep(0)
        self.assertTrue(get_task.done())
        with self.assertRaisesShutdown():
            await get_task

        # Ensure put() and get() raise ShutDown
        with self.assertRaisesShutdown():
            await q.put("data")
        with self.assertRaisesShutdown():
            q.put_nowait("data")

        with self.assertRaisesShutdown():
            await q.get()
        with self.assertRaisesShutdown():
            q.get_nowait()

    async def test_shutdown_nonempty(self):
        # Test shutting down a non-empty queue

        # Setup full queue with 1 item, and join() and put() tasks
        q = self.q_class(maxsize=1)
        loop = asyncio.get_running_loop()

        q.put_nowait("data")
        join_task = loop.create_task(q.join())
        put_task = loop.create_task(q.put("data2"))

        # Ensure put() task is not finished
        await asyncio.sleep(0)
        self.assertFalse(put_task.done())

        # Perform shut-down
        q.shutdown(immediate=False)  # unfinished tasks: 1 -> 1

        self.assertEqual(q.qsize(), 1)

        # Ensure put() task is finished, and raised ShutDown
        await asyncio.sleep(0)
        self.assertTrue(put_task.done())
        with self.assertRaisesShutdown():
            await put_task

        # Ensure get() succeeds on enqueued item
        self.assertEqual(await q.get(), "data")

        # Ensure join() task is not finished
        await asyncio.sleep(0)
        self.assertFalse(join_task.done())

        # Ensure put() and get() raise ShutDown
        with self.assertRaisesShutdown():
            await q.put("data")
        with self.assertRaisesShutdown():
            q.put_nowait("data")

        with self.assertRaisesShutdown():
            await q.get()
        with self.assertRaisesShutdown():
            q.get_nowait()

        # Ensure there is 1 unfinished task, and join() task succeeds
        q.task_done()

        await asyncio.sleep(0)
        self.assertTrue(join_task.done())
        await join_task

        with self.assertRaises(
            ValueError, msg="Didn't appear to mark all tasks done"
        ):
            q.task_done()

    async def test_shutdown_immediate(self):
        # Test immediately shutting down a queue

        # Setup queue with 1 item, and a join() task
        q = self.q_class()
        loop = asyncio.get_running_loop()
        q.put_nowait("data")
        join_task = loop.create_task(q.join())

        # Perform shut-down
        q.shutdown(immediate=True)  # unfinished tasks: 1 -> 0

        self.assertEqual(q.qsize(), 0)

        # Ensure join() task has successfully finished
        await asyncio.sleep(0)
        self.assertTrue(join_task.done())
        await join_task

        # Ensure put() and get() raise ShutDown
        with self.assertRaisesShutdown():
            await q.put("data")
        with self.assertRaisesShutdown():
            q.put_nowait("data")

        with self.assertRaisesShutdown():
            await q.get()
        with self.assertRaisesShutdown():
            q.get_nowait()

        # Ensure there are no unfinished tasks
        with self.assertRaises(
            ValueError, msg="Didn't appear to mark all tasks done"
        ):
            q.task_done()

    async def test_shutdown_immediate_with_unfinished(self):
        # Test immediately shutting down a queue with unfinished tasks

        # Setup queue with 2 items (1 retrieved), and a join() task
        q = self.q_class()
        loop = asyncio.get_running_loop()
        q.put_nowait("data")
        q.put_nowait("data")
        join_task = loop.create_task(q.join())
        self.assertEqual(await q.get(), "data")

        # Perform shut-down
        q.shutdown(immediate=True)  # unfinished tasks: 2 -> 1

        self.assertEqual(q.qsize(), 0)

        # Ensure join() task is not finished
        await asyncio.sleep(0)
        self.assertFalse(join_task.done())

        # Ensure put() and get() raise ShutDown
        with self.assertRaisesShutdown():
            await q.put("data")
        with self.assertRaisesShutdown():
            q.put_nowait("data")

        with self.assertRaisesShutdown():
            await q.get()
        with self.assertRaisesShutdown():
            q.get_nowait()

        # Ensure there is 1 unfinished task
        q.task_done()
        with self.assertRaises(
            ValueError, msg="Didn't appear to mark all tasks done"
        ):
            q.task_done()

        # Ensure join() task has successfully finished
        await asyncio.sleep(0)
        self.assertTrue(join_task.done())
        await join_task


class QueueShutdownTests(
    _QueueShutdownTestMixin, unittest.IsolatedAsyncioTestCase
):
    q_class = asyncio.Queue


class LifoQueueShutdownTests(
    _QueueShutdownTestMixin, unittest.IsolatedAsyncioTestCase
):
    q_class = asyncio.LifoQueue


class PriorityQueueShutdownTests(
    _QueueShutdownTestMixin, unittest.IsolatedAsyncioTestCase
):
    q_class = asyncio.PriorityQueue


if __name__ == '__main__':
    unittest.main()
