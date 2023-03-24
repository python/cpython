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

    async def asyncSetUp(self):
        await super().asyncSetUp()
        self.delay = 0.001

    async def _get(self, q, go, results, shutdown=False):
        await go.wait()
        try:
            msg = await q.get()
            results.append(not shutdown)
            return msg
        except asyncio.QueueShutDown:
            results.append(shutdown)
            return shutdown

    async def _get_nowait(self, q, go, results, shutdown=False):
        await go.wait()
        try:
            msg = q.get_nowait()
            results.append(not shutdown)
            return msg
        except asyncio.QueueShutDown:
            results.append(shutdown)
            return shutdown

    async def _get_task_done(self, q, go, results):
        await go.wait()
        try:
            msg = await q.get()
            q.task_done()
            results.append(True)
            return msg
        except asyncio.QueueShutDown:
            results.append(False)
            return False

    async def _put(self, q, go, msg, results, shutdown=False):
        await go.wait()
        try:
            await q.put(msg)
            results.append(not shutdown)
            return not shutdown
        except asyncio.QueueShutDown:
            results.append(shutdown)
            return shutdown

    async def _put_nowait(self, q, go, msg, results, shutdown=False):
        await go.wait()
        try:
            q.put_nowait(msg)
            results.append(False)
            return not shutdown
        except asyncio.QueueShutDown:
            results.append(True)
            return shutdown

    async def _shutdown(self, q, go, immediate):
        q.shutdown(immediate)
        await asyncio.sleep(self.delay)
        go.set()
        await asyncio.sleep(self.delay)

    async def _join(self, q, results, shutdown=False):
        try:
            await q.join()
            results.append(not shutdown)
            return True
        except asyncio.QueueShutDown:
            results.append(shutdown)
            return False
        except asyncio.CancelledError:
            results.append(shutdown)
            raise

    async def test_shutdown_empty(self):
        q = self.q_class()
        q.shutdown()
        with self.assertRaises(
            asyncio.QueueShutDown, msg="Didn't appear to shut-down queue"
        ):
            await q.put("data")
        with self.assertRaises(
            asyncio.QueueShutDown, msg="Didn't appear to shut-down queue"
        ):
            await q.get()

    async def test_shutdown_nonempty(self):
        q = self.q_class()
        q.put_nowait("data")
        q.shutdown()
        await q.get()
        with self.assertRaises(
            asyncio.QueueShutDown, msg="Didn't appear to shut-down queue"
        ):
            await q.get()

    async def test_shutdown_immediate(self):
        q = self.q_class()
        q.put_nowait("data")
        q.shutdown(immediate=True)
        with self.assertRaises(
            asyncio.QueueShutDown, msg="Didn't appear to shut-down queue"
        ):
            await q.get()

    async def test_shutdown_repr(self):
        q = self.q_class(4)
        # when alive, not in repr
        self.assertNotIn("alive", repr(q))

        q = self.q_class(6)
        q.shutdown(immediate=False)
        self.assertIn("shutdown", repr(q))

        q = self.q_class(8)
        q.shutdown(immediate=True)
        self.assertIn("shutdown-immediate", repr(q))

    async def test_shutdown_allowed_transitions(self):
        # allowed transitions would be from alive via shutdown to immediate
        q = self.q_class()
        self.assertEqual("alive", q._shutdown_state.value)

        q.shutdown()
        self.assertEqual("shutdown", q._shutdown_state.value)

        q.shutdown(immediate=True)
        self.assertEqual("shutdown-immediate", q._shutdown_state.value)

        q.shutdown(immediate=False)
        self.assertNotEqual("shutdown", q._shutdown_state.value)

    async def _shutdown_all_methods_in_one_task(self, immediate):
        q = asyncio.Queue()
        await q.put("L")
        q.put_nowait("O")
        q.shutdown(immediate)
        with self.assertRaises(asyncio.QueueShutDown):
            await q.put("E")
        with self.assertRaises(asyncio.QueueShutDown):
            q.put_nowait("W")

        if immediate:
            with self.assertRaises(asyncio.QueueShutDown):
                await q.get()
            with self.assertRaises(asyncio.QueueShutDown):
                q.get_nowait()
            with self.assertRaises(asyncio.QueueShutDown):
                q.task_done()
            with self.assertRaises(asyncio.QueueShutDown):
                await q.join()
        else:
            self.assertIn(await q.get(), "LO")
            q.task_done()
            self.assertIn(q.get_nowait(), "LO")
            q.task_done()
            await q.join()
            # on shutdown(immediate=False)
            # when queue is empty, should raise ShutDown Exception
            with self.assertRaises(asyncio.QueueShutDown):
                await q.get()
            with self.assertRaises(asyncio.QueueShutDown):
                q.get_nowait()

    async def test_shutdown_all_methods_in_one_task(self):
        return await self._shutdown_all_methods_in_one_task(False)

    async def test_shutdown_immediate_all_methods_in_one_task(self):
        return await self._shutdown_all_methods_in_one_task(True)

    async def _shutdown_putters(self, immediate):
        delay = self.delay
        q = self.q_class(2)
        results = []
        await q.put("E")
        await q.put("W")
        # queue full
        t = asyncio.create_task(q.put("Y"))
        await asyncio.sleep(delay)
        self.assertTrue(len(q._putters) == 1)
        with self.assertRaises(asyncio.QueueShutDown):
            # here `t` raises a QueueShuDown
            q.shutdown(immediate)
            await t
        self.assertTrue(not q._putters)

    async def test_shutdown_putters_deque(self):
        return await self._shutdown_putters(False)

    async def test_shutdown_immediate_putters_deque(self):
        return await self._shutdown_putters(True)

    async def _shutdown_getters(self, immediate):
        delay = self.delay
        q = self.q_class(1)
        results = []
        await q.put("Y")
        nb = q.qsize()
        # queue full

        asyncio.create_task(q.get())
        await asyncio.sleep(delay)
        t = asyncio.create_task(q.get())
        await asyncio.sleep(delay)
        self.assertTrue(len(q._getters) == 1)
        if immediate:
            # here `t` raises a QueueShuDown
            with self.assertRaises(asyncio.QueueShutDown):
                q.shutdown(immediate)
                await t
            self.assertTrue(not q._getters)
        else:
            # here `t` is always pending
            q.shutdown(immediate)
            await asyncio.sleep(delay)
            self.assertTrue(q._getters)
        self.assertEqual(q._unfinished_tasks, nb)


    async def test_shutdown_getters_deque(self):
        return await self._shutdown_getters(False)

    async def test_shutdown_immediate_getters_deque(self):
        return await self._shutdown_getters(True)

    async def _shutdown_get(self, immediate):
        q = self.q_class(2)
        results = []
        go = asyncio.Event()
        await q.put("Y")
        await q.put("D")
        nb = q.qsize()
        # queue full

        if immediate:
            coros = (
                (self._get(q, go, results, shutdown=True)),
                (self._get_nowait(q, go, results, shutdown=True)),
            )
        else:
            coros = (
                # one of these tasks shoud raise Shutdown
                (self._get(q, go, results)),
                (self._get_nowait(q, go, results)),
                (self._get_nowait(q, go, results)),
            )
        t = []
        for coro in coros:
            t.append(asyncio.create_task(coro))
        t.append(asyncio.create_task(self._shutdown(q, go, immediate)))
        res = await asyncio.gather(*t)
        if immediate:
            self.assertEqual(results, [True]*len(coros))
        else:
            self.assertListEqual(sorted(results), [False] + [True]*(len(coros)-1))

    async def test_shutdown_get(self):
        return await self._shutdown_get(False)

    async def test_shutdown_immediate_get(self):
        return await self._shutdown_get(True)

    async def test_shutdown_get_task_done_join(self):
        q = self.q_class(2)
        results = []
        go = asyncio.Event()
        await q.put("Y")
        await q.put("D")
        self.assertEqual(q._unfinished_tasks, q.qsize())

        # queue full

        coros = (
            (self._get_task_done(q, go, results)),
            (self._get_task_done(q, go, results)),
            (self._join(q, results)),
            (self._join(q, results)),
        )
        t = []
        for coro in coros:
            t.append(asyncio.create_task(coro))
        t.append(asyncio.create_task(self._shutdown(q, go, False)))
        res = await asyncio.gather(*t)

        self.assertEqual(results, [True]*len(coros))
        self.assertIn(t[0].result(), "YD")
        self.assertIn(t[1].result(), "YD")
        self.assertNotEqual(t[0].result(), t[1].result())
        self.assertEqual(q._unfinished_tasks, 0)

    async def _shutdown_put(self, immediate):
        q = self.q_class()
        results = []
        go = asyncio.Event()
        # queue not empty

        coros = (
            (self._put(q, go, "Y", results, shutdown=True)),
            (self._put_nowait(q, go, "D", results, shutdown=True)),
        )
        t = []
        for coro in coros:
            t.append(asyncio.create_task(coro))
        t.append(asyncio.create_task(self._shutdown(q, go, immediate)))
        res = await asyncio.gather(*t)

        self.assertEqual(results, [True]*len(coros))

    async def test_shutdown_put(self):
        return await self._shutdown_put(False)

    async def test_shutdown_immediate_put(self):
        return await self._shutdown_put(True)

    async def _shutdown_put_join(self, immediate):
        q = self.q_class(2)
        results = []
        go = asyncio.Event()
        await q.put("Y")
        await q.put("D")
        nb = q.qsize()
        # queue fulled

        async def _cancel_join_task(q, delay, t):
            await asyncio.sleep(delay)
            t.cancel()
            await asyncio.sleep(0)
            q._finished.set()

        coros = (
            (self._put(q, go, "E", results, shutdown=True)),
            (self._put_nowait(q, go, "W", results, shutdown=True)),
            (self._join(q, results, shutdown=True)),
        )
        t = []
        for coro in coros:
            t.append(asyncio.create_task(coro))
        t.append(asyncio.create_task(self._shutdown(q, go, immediate)))
        if not immediate:
            # Here calls `join` is a blocking operation
            # so wait for a delay and cancel this blocked task
            t.append(asyncio.create_task(_cancel_join_task(q, 0.01, t[2])))
            with self.assertRaises(asyncio.CancelledError) as e:
                await asyncio.gather(*t)
        else:
            res = await asyncio.gather(*t)

        self.assertEqual(results, [True]*len(coros))
        self.assertTrue(q._finished.is_set())

    async def test_shutdown_put_join(self):
        return await self._shutdown_put_join(False)

    async def test_shutdown_immediate_put_and_join(self):
        return await self._shutdown_put_join(True)


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
