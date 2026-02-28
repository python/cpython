"""Tests for asyncio.channels (memory object channels)."""

import asyncio
import math
import unittest


class TestOpenChannel(unittest.TestCase):

    def test_open_channel_returns_pair(self):
        send, recv = asyncio.open_channel(10)
        self.assertIsInstance(send, asyncio.SendChannel)
        self.assertIsInstance(recv, asyncio.ReceiveChannel)
        send.close()
        recv.close()

    def test_open_channel_zero_buffer(self):
        send, recv = asyncio.open_channel(0)
        self.assertEqual(send.statistics().max_buffer_size, 0)
        send.close()
        recv.close()

    def test_open_channel_negative_raises(self):
        with self.assertRaises(ValueError):
            asyncio.open_channel(-1)

    def test_open_channel_invalid_type_raises(self):
        with self.assertRaises(TypeError):
            asyncio.open_channel("10")

    def test_open_channel_float_non_inf_raises(self):
        with self.assertRaises(ValueError):
            asyncio.open_channel(1.5)

    def test_open_channel_inf(self):
        send, recv = asyncio.open_channel(math.inf)
        self.assertEqual(send.statistics().max_buffer_size, math.inf)
        send.close()
        recv.close()


class TestSendReceiveBuffered(unittest.IsolatedAsyncioTestCase):

    async def test_send_receive_buffered(self):
        send, recv = asyncio.open_channel(10)
        await send.send(1)
        await send.send(2)
        await send.send(3)
        self.assertEqual(await recv.receive(), 1)
        self.assertEqual(await recv.receive(), 2)
        self.assertEqual(await recv.receive(), 3)
        send.close()
        recv.close()

    async def test_send_receive_unbuffered(self):
        """Rendezvous channel (max_buffer_size=0)."""
        send, recv = asyncio.open_channel(0)
        results = []

        async def producer():
            await send.send("hello")
            await send.send("world")
            send.close()

        async def consumer():
            async for item in recv:
                results.append(item)
            recv.close()

        async with asyncio.TaskGroup() as tg:
            tg.create_task(producer())
            tg.create_task(consumer())

        self.assertEqual(results, ["hello", "world"])


class TestNowait(unittest.IsolatedAsyncioTestCase):

    async def test_send_nowait_receive_nowait(self):
        send, recv = asyncio.open_channel(5)
        send.send_nowait(42)
        send.send_nowait(43)
        self.assertEqual(recv.receive_nowait(), 42)
        self.assertEqual(recv.receive_nowait(), 43)
        send.close()
        recv.close()

    async def test_send_nowait_would_block(self):
        send, recv = asyncio.open_channel(1)
        send.send_nowait("a")
        with self.assertRaises(asyncio.WouldBlock):
            send.send_nowait("b")
        recv.close()
        send.close()

    async def test_receive_nowait_would_block(self):
        send, recv = asyncio.open_channel(5)
        with self.assertRaises(asyncio.WouldBlock):
            recv.receive_nowait()
        send.close()
        recv.close()

    async def test_direct_delivery(self):
        """send_nowait delivers directly to a waiting receiver."""
        send, recv = asyncio.open_channel(0)
        result = None

        async def consumer():
            nonlocal result
            result = await recv.receive()

        async with asyncio.TaskGroup() as tg:
            tg.create_task(consumer())
            # Let consumer start waiting.
            await asyncio.sleep(0)
            send.send_nowait("direct")

        self.assertEqual(result, "direct")
        send.close()
        recv.close()

    async def test_receive_wakes_sender(self):
        """receive_nowait unblocks a blocked sender."""
        send, recv = asyncio.open_channel(0)
        sent = False

        async def producer():
            nonlocal sent
            await send.send("item")
            sent = True

        async with asyncio.TaskGroup() as tg:
            tg.create_task(producer())
            await asyncio.sleep(0)
            # Producer should be waiting.
            self.assertFalse(sent)
            item = recv.receive_nowait()
            self.assertEqual(item, "item")
            # Let producer finish.
            await asyncio.sleep(0)

        self.assertTrue(sent)
        send.close()
        recv.close()


class TestBlocking(unittest.IsolatedAsyncioTestCase):

    async def test_send_blocks_when_full(self):
        send, recv = asyncio.open_channel(1)
        send.send_nowait("fill")
        blocked = True

        async def producer():
            nonlocal blocked
            await send.send("extra")
            blocked = False

        async with asyncio.TaskGroup() as tg:
            tg.create_task(producer())
            await asyncio.sleep(0)
            self.assertTrue(blocked)
            # Unblock by consuming.
            self.assertEqual(recv.receive_nowait(), "fill")
            await asyncio.sleep(0)

        self.assertFalse(blocked)
        self.assertEqual(recv.receive_nowait(), "extra")
        send.close()
        recv.close()

    async def test_receive_blocks_when_empty(self):
        send, recv = asyncio.open_channel(5)
        received = None

        async def consumer():
            nonlocal received
            received = await recv.receive()

        async with asyncio.TaskGroup() as tg:
            tg.create_task(consumer())
            await asyncio.sleep(0)
            self.assertIsNone(received)
            await send.send("arrived")
            await asyncio.sleep(0)

        self.assertEqual(received, "arrived")
        send.close()
        recv.close()


class TestFIFOOrdering(unittest.IsolatedAsyncioTestCase):

    async def test_fifo_ordering(self):
        send, recv = asyncio.open_channel(10)
        items = list(range(10))
        for item in items:
            send.send_nowait(item)
        received = [recv.receive_nowait() for _ in range(10)]
        self.assertEqual(received, items)
        send.close()
        recv.close()


class TestClone(unittest.IsolatedAsyncioTestCase):

    async def test_clone_send(self):
        send, recv = asyncio.open_channel(5)
        clone = send.clone()
        self.assertEqual(send.statistics().open_send_channels, 2)
        clone.send_nowait("from_clone")
        self.assertEqual(recv.receive_nowait(), "from_clone")
        clone.close()
        self.assertEqual(send.statistics().open_send_channels, 1)
        send.close()
        recv.close()

    async def test_clone_receive(self):
        send, recv = asyncio.open_channel(5)
        clone = recv.clone()
        self.assertEqual(recv.statistics().open_receive_channels, 2)
        send.send_nowait("item")
        self.assertEqual(clone.receive_nowait(), "item")
        clone.close()
        self.assertEqual(recv.statistics().open_receive_channels, 1)
        send.close()
        recv.close()

    async def test_clone_closed_raises(self):
        send, recv = asyncio.open_channel(5)
        send.close()
        with self.assertRaises(asyncio.ClosedResourceError):
            send.clone()
        recv.close()
        with self.assertRaises(asyncio.ClosedResourceError):
            recv.clone()


class TestEndOfChannel(unittest.IsolatedAsyncioTestCase):

    async def test_end_of_channel_all_senders_close(self):
        send, recv = asyncio.open_channel(5)
        send.send_nowait("last")
        send.close()
        self.assertEqual(recv.receive_nowait(), "last")
        with self.assertRaises(asyncio.EndOfChannel):
            recv.receive_nowait()
        recv.close()

    async def test_end_of_channel_last_clone(self):
        send, recv = asyncio.open_channel(5)
        clone = send.clone()
        send.close()
        # Still one sender open — no EndOfChannel yet.
        clone.send_nowait("still_alive")
        self.assertEqual(recv.receive_nowait(), "still_alive")
        clone.close()
        with self.assertRaises(asyncio.EndOfChannel):
            recv.receive_nowait()
        recv.close()

    async def test_end_of_channel_waiting_receiver(self):
        send, recv = asyncio.open_channel(0)
        result_exc = None

        async def consumer():
            nonlocal result_exc
            try:
                await recv.receive()
            except asyncio.EndOfChannel:
                result_exc = True

        async with asyncio.TaskGroup() as tg:
            tg.create_task(consumer())
            await asyncio.sleep(0)
            send.close()

        self.assertTrue(result_exc)
        recv.close()


class TestBrokenResource(unittest.IsolatedAsyncioTestCase):

    async def test_broken_resource_all_receivers_close(self):
        send, recv = asyncio.open_channel(5)
        recv.close()
        with self.assertRaises(asyncio.BrokenResourceError):
            send.send_nowait("nowhere")
        send.close()

    async def test_broken_resource_last_clone(self):
        send, recv = asyncio.open_channel(5)
        clone = recv.clone()
        recv.close()
        # Still one receiver open — no BrokenResourceError yet.
        send.send_nowait("ok")
        clone.close()
        with self.assertRaises(asyncio.BrokenResourceError):
            send.send_nowait("nope")
        send.close()

    async def test_broken_resource_waiting_sender(self):
        send, recv = asyncio.open_channel(0)
        result_exc = None

        async def producer():
            nonlocal result_exc
            try:
                await send.send("item")
            except asyncio.BrokenResourceError:
                result_exc = True

        async with asyncio.TaskGroup() as tg:
            tg.create_task(producer())
            await asyncio.sleep(0)
            recv.close()

        self.assertTrue(result_exc)
        send.close()


class TestClosedResource(unittest.IsolatedAsyncioTestCase):

    async def test_closed_send_raises(self):
        send, recv = asyncio.open_channel(5)
        send.close()
        with self.assertRaises(asyncio.ClosedResourceError):
            send.send_nowait("fail")
        with self.assertRaises(asyncio.ClosedResourceError):
            await send.send("fail")
        recv.close()

    async def test_closed_receive_raises(self):
        send, recv = asyncio.open_channel(5)
        recv.close()
        with self.assertRaises(asyncio.ClosedResourceError):
            recv.receive_nowait()
        with self.assertRaises(asyncio.ClosedResourceError):
            await recv.receive()
        send.close()

    async def test_close_idempotent(self):
        send, recv = asyncio.open_channel(5)
        send.close()
        send.close()  # should not raise
        recv.close()
        recv.close()  # should not raise


class TestCancellation(unittest.IsolatedAsyncioTestCase):

    async def test_send_cancellation(self):
        send, recv = asyncio.open_channel(0)

        async def producer():
            await send.send("item")

        task = asyncio.ensure_future(producer())
        await asyncio.sleep(0)
        # Producer should be waiting.
        self.assertEqual(send.statistics().tasks_waiting_send, 1)
        task.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await task
        self.assertEqual(send.statistics().tasks_waiting_send, 0)
        send.close()
        recv.close()

    async def test_receive_cancellation(self):
        send, recv = asyncio.open_channel(5)

        async def consumer():
            await recv.receive()

        task = asyncio.ensure_future(consumer())
        await asyncio.sleep(0)
        self.assertEqual(recv.statistics().tasks_waiting_receive, 1)
        task.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await task
        self.assertEqual(recv.statistics().tasks_waiting_receive, 0)
        send.close()
        recv.close()


class TestAsyncIteration(unittest.IsolatedAsyncioTestCase):

    async def test_async_iteration(self):
        send, recv = asyncio.open_channel(10)
        for i in range(5):
            send.send_nowait(i)
        send.close()
        results = []
        async for item in recv:
            results.append(item)
        self.assertEqual(results, [0, 1, 2, 3, 4])
        recv.close()

    async def test_async_iteration_stops(self):
        send, recv = asyncio.open_channel(5)
        send.close()
        results = []
        async for item in recv:
            results.append(item)
        self.assertEqual(results, [])
        recv.close()


class TestContextManagers(unittest.IsolatedAsyncioTestCase):

    async def test_context_manager_sync(self):
        send, recv = asyncio.open_channel(5)
        with send:
            send.send_nowait("in_context")
        with self.assertRaises(asyncio.ClosedResourceError):
            send.send_nowait("after_context")
        with recv:
            self.assertEqual(recv.receive_nowait(), "in_context")
        with self.assertRaises(asyncio.ClosedResourceError):
            recv.receive_nowait()

    async def test_context_manager_async(self):
        send, recv = asyncio.open_channel(5)
        async with send:
            await send.send("in_context")
        with self.assertRaises(asyncio.ClosedResourceError):
            await send.send("after_context")
        async with recv:
            self.assertEqual(await recv.receive(), "in_context")
        with self.assertRaises(asyncio.ClosedResourceError):
            await recv.receive()


class TestStatistics(unittest.IsolatedAsyncioTestCase):

    async def test_statistics(self):
        send, recv = asyncio.open_channel(10)
        stats = send.statistics()
        self.assertEqual(stats.current_buffer_used, 0)
        self.assertEqual(stats.max_buffer_size, 10)
        self.assertEqual(stats.open_send_channels, 1)
        self.assertEqual(stats.open_receive_channels, 1)
        self.assertEqual(stats.tasks_waiting_send, 0)
        self.assertEqual(stats.tasks_waiting_receive, 0)

        send.send_nowait("a")
        send.send_nowait("b")
        stats = recv.statistics()
        self.assertEqual(stats.current_buffer_used, 2)

        clone = send.clone()
        stats = send.statistics()
        self.assertEqual(stats.open_send_channels, 2)

        clone.close()
        send.close()
        recv.close()


class TestRepr(unittest.IsolatedAsyncioTestCase):

    async def test_repr(self):
        send, recv = asyncio.open_channel(5)
        self.assertIn('SendChannel', repr(send))
        self.assertIn('max_buffer_size=5', repr(send))
        self.assertIn('ReceiveChannel', repr(recv))
        send.close()
        self.assertIn('closed', repr(send))
        recv.close()
        self.assertIn('closed', repr(recv))


class TestConcurrentPatterns(unittest.IsolatedAsyncioTestCase):

    async def test_concurrent_producer_consumer(self):
        send, recv = asyncio.open_channel(2)
        produced = list(range(20))
        consumed = []

        async def producer():
            for item in produced:
                await send.send(item)
            send.close()

        async def consumer():
            async for item in recv:
                consumed.append(item)
            recv.close()

        async with asyncio.TaskGroup() as tg:
            tg.create_task(producer())
            tg.create_task(consumer())

        self.assertEqual(consumed, produced)

    async def test_fan_in(self):
        """Multiple sender clones, one receiver."""
        send, recv = asyncio.open_channel(10)
        results = []

        async def producer(s, prefix):
            for i in range(3):
                await s.send(f"{prefix}-{i}")
            s.close()

        async def consumer():
            async for item in recv:
                results.append(item)
            recv.close()

        clone1 = send.clone()
        clone2 = send.clone()
        send.close()  # Close original, clones keep it alive.

        async with asyncio.TaskGroup() as tg:
            tg.create_task(producer(clone1, "a"))
            tg.create_task(producer(clone2, "b"))
            tg.create_task(consumer())

        self.assertEqual(len(results), 6)
        # Check all items from both producers are present.
        self.assertIn("a-0", results)
        self.assertIn("b-2", results)

    async def test_fan_out(self):
        """One sender, multiple receiver clones."""
        send, recv = asyncio.open_channel(0)
        results_a = []
        results_b = []

        async def producer():
            for i in range(6):
                await send.send(i)
            send.close()

        async def consumer(r, results):
            async for item in r:
                results.append(item)
            r.close()

        clone = recv.clone()

        async with asyncio.TaskGroup() as tg:
            tg.create_task(producer())
            tg.create_task(consumer(recv, results_a))
            tg.create_task(consumer(clone, results_b))

        # All items distributed between the two consumers.
        all_items = sorted(results_a + results_b)
        self.assertEqual(all_items, list(range(6)))

    async def test_inf_buffer(self):
        send, recv = asyncio.open_channel(math.inf)
        for i in range(1000):
            send.send_nowait(i)
        self.assertEqual(send.statistics().current_buffer_used, 1000)
        for i in range(1000):
            self.assertEqual(recv.receive_nowait(), i)
        send.close()
        recv.close()


if __name__ == '__main__':
    unittest.main()
