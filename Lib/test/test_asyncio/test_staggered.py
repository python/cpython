import asyncio
import unittest
from asyncio.staggered import staggered_race

from test import support

support.requires_working_socket(module=True)


def tearDownModule():
    asyncio._set_event_loop_policy(None)


class StaggeredTests(unittest.IsolatedAsyncioTestCase):
    async def test_empty(self):
        winner, index, excs = await staggered_race(
            [],
            delay=None,
        )

        self.assertIs(winner, None)
        self.assertIs(index, None)
        self.assertEqual(excs, [])

    async def test_one_successful(self):
        async def coro(index):
            return f'Res: {index}'

        winner, index, excs = await staggered_race(
            [
                lambda: coro(0),
                lambda: coro(1),
            ],
            delay=None,
        )

        self.assertEqual(winner, 'Res: 0')
        self.assertEqual(index, 0)
        self.assertEqual(excs, [None])

    async def test_first_error_second_successful(self):
        async def coro(index):
            if index == 0:
                raise ValueError(index)
            return f'Res: {index}'

        winner, index, excs = await staggered_race(
            [
                lambda: coro(0),
                lambda: coro(1),
            ],
            delay=None,
        )

        self.assertEqual(winner, 'Res: 1')
        self.assertEqual(index, 1)
        self.assertEqual(len(excs), 2)
        self.assertIsInstance(excs[0], ValueError)
        self.assertIs(excs[1], None)

    async def test_first_timeout_second_successful(self):
        async def coro(index):
            if index == 0:
                await asyncio.sleep(10)  # much bigger than delay
            return f'Res: {index}'

        winner, index, excs = await staggered_race(
            [
                lambda: coro(0),
                lambda: coro(1),
            ],
            delay=0.1,
        )

        self.assertEqual(winner, 'Res: 1')
        self.assertEqual(index, 1)
        self.assertEqual(len(excs), 2)
        self.assertIsInstance(excs[0], asyncio.CancelledError)
        self.assertIs(excs[1], None)

    async def test_none_successful(self):
        async def coro(index):
            raise ValueError(index)

        winner, index, excs = await staggered_race(
            [
                lambda: coro(0),
                lambda: coro(1),
            ],
            delay=None,
        )

        self.assertIs(winner, None)
        self.assertIs(index, None)
        self.assertEqual(len(excs), 2)
        self.assertIsInstance(excs[0], ValueError)
        self.assertIsInstance(excs[1], ValueError)


    async def test_multiple_winners(self):
        event = asyncio.Event()

        async def coro(index):
            await event.wait()
            return index

        async def do_set():
            event.set()
            await asyncio.Event().wait()

        winner, index, excs = await staggered_race(
            [
                lambda: coro(0),
                lambda: coro(1),
                do_set,
            ],
            delay=0.1,
        )
        self.assertIs(winner, 0)
        self.assertIs(index, 0)
        self.assertEqual(len(excs), 3)
        self.assertIsNone(excs[0], None)
        self.assertIsInstance(excs[1], asyncio.CancelledError)
        self.assertIsInstance(excs[2], asyncio.CancelledError)


    async def test_cancelled(self):
        log = []
        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(None) as cs_outer, asyncio.timeout(None) as cs_inner:
                async def coro_fn():
                    cs_inner.reschedule(-1)
                    await asyncio.sleep(0)
                    try:
                        await asyncio.sleep(0)
                    except asyncio.CancelledError:
                        log.append("cancelled 1")

                    cs_outer.reschedule(-1)
                    await asyncio.sleep(0)
                    try:
                        await asyncio.sleep(0)
                    except asyncio.CancelledError:
                        log.append("cancelled 2")
                try:
                    await staggered_race([coro_fn], delay=None)
                except asyncio.CancelledError:
                    log.append("cancelled 3")
                    raise

        self.assertListEqual(log, ["cancelled 1", "cancelled 2", "cancelled 3"])

    async def test_async_iterable_empty(self):
        async def empty_async_iterable():
            if False:
                yield lambda: asyncio.sleep(0)

        winner, index, excs = await staggered_race(
            empty_async_iterable(),
            delay=None,
        )

        self.assertIs(winner, None)
        self.assertIs(index, None)
        self.assertEqual(excs, [])

    async def test_async_iterable_one_successful(self):
        async def async_coro_generator():
            async def coro(index):
                return f'Async Res: {index}'

            yield lambda: coro(0)
            yield lambda: coro(1)

        winner, index, excs = await staggered_race(
            async_coro_generator(),
            delay=None,
        )

        self.assertEqual(winner, 'Async Res: 0')
        self.assertEqual(index, 0)
        self.assertEqual(excs, [None])

    async def test_async_iterable_first_error_second_successful(self):
        async def async_coro_generator():
            async def coro(index):
                if index == 0:
                    raise ValueError(f'Async Error: {index}')
                return f'Async Res: {index}'

            yield lambda: coro(0)
            yield lambda: coro(1)

        winner, index, excs = await staggered_race(
            async_coro_generator(),
            delay=None,
        )

        self.assertEqual(winner, 'Async Res: 1')
        self.assertEqual(index, 1)
        self.assertEqual(len(excs), 2)
        self.assertIsInstance(excs[0], ValueError)
        self.assertEqual(str(excs[0]), 'Async Error: 0')
        self.assertIs(excs[1], None)

    async def test_async_iterable_first_timeout_second_successful(self):
        async def async_coro_generator():
            async def coro(index):
                if index == 0:
                    await asyncio.sleep(10)
                return f'Async Res: {index}'

            yield lambda: coro(0)
            yield lambda: coro(1)

        winner, index, excs = await staggered_race(
            async_coro_generator(),
            delay=0.1,
        )

        self.assertEqual(winner, 'Async Res: 1')
        self.assertEqual(index, 1)
        self.assertEqual(len(excs), 2)
        self.assertIsInstance(excs[0], asyncio.CancelledError)
        self.assertIs(excs[1], None)

    async def test_async_iterable_none_successful(self):
        async def async_coro_generator():
            async def coro(index):
                raise ValueError(f'Async Error: {index}')

            yield lambda: coro(0)
            yield lambda: coro(1)

        winner, index, excs = await staggered_race(
            async_coro_generator(),
            delay=None,
        )

        self.assertIs(winner, None)
        self.assertIs(index, None)
        self.assertEqual(len(excs), 2)
        self.assertIsInstance(excs[0], ValueError)
        self.assertEqual(str(excs[0]), 'Async Error: 0')
        self.assertIsInstance(excs[1], ValueError)
        self.assertEqual(str(excs[1]), 'Async Error: 1')

    async def test_async_iterable_multiple_winners(self):
        event = asyncio.Event()

        async def async_coro_generator():
            async def coro(index):
                await event.wait()
                return f'Async Index: {index}'

            async def do_set():
                event.set()
                await asyncio.Event().wait()

            yield lambda: coro(0)
            yield lambda: coro(1)
            yield do_set

        winner, index, excs = await staggered_race(
            async_coro_generator(),
            delay=0.1,
        )

        self.assertEqual(winner, 'Async Index: 0')
        self.assertEqual(index, 0)
        self.assertEqual(len(excs), 3)
        self.assertIsNone(excs[0])
        self.assertIsInstance(excs[1], asyncio.CancelledError)
        self.assertIsInstance(excs[2], asyncio.CancelledError)

    async def test_async_iterable_with_delay(self):
        results = []

        async def async_coro_generator():
            async def coro(index):
                results.append(f'Started: {index}')
                await asyncio.sleep(0.05)
                return f'Result: {index}'

            yield lambda: coro(0)
            yield lambda: coro(1)
            yield lambda: coro(2)

        winner, index, excs = await staggered_race(
            async_coro_generator(),
            delay=0.02,
        )

        self.assertEqual(winner, 'Result: 0')
        self.assertEqual(index, 0)

        self.assertGreaterEqual(len(excs), 1)
        self.assertIsNone(excs[0])

        self.assertIn('Started: 0', results)

    async def test_async_iterable_mixed_with_regular(self):
        async def coro(index):
            return f'Mixed Res: {index}'

        winner, index, excs = await staggered_race(
            [lambda: coro(0), lambda: coro(1)],
            delay=None,
        )

        self.assertEqual(winner, 'Mixed Res: 0')
        self.assertEqual(index, 0)
        self.assertEqual(excs, [None])

    async def test_async_iterable_cancelled(self):
        log = []

        async def async_coro_generator():
            async def coro_fn():
                try:
                    await asyncio.sleep(0.1)
                except asyncio.CancelledError:
                    log.append("async cancelled")
                    raise

            yield coro_fn

        with self.assertRaises(TimeoutError):
            async with asyncio.timeout(0.01):
                await staggered_race(async_coro_generator(), delay=None)

        self.assertListEqual(log, ["async cancelled"])

    async def test_async_iterable_stop_async_iteration(self):
        async def async_coro_generator():
            async def coro():
                return "success"

            yield lambda: coro()

        winner, index, excs = await staggered_race(
            async_coro_generator(),
            delay=None,
        )

        self.assertEqual(winner, "success")
        self.assertEqual(index, 0)
        self.assertEqual(excs, [None])
