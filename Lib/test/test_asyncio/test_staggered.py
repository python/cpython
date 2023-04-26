import asyncio
import functools
import unittest
from asyncio.staggered import staggered_race


# To prevent a warning "test altered the execution environment"
def tearDownModule():
    asyncio.set_event_loop_policy(None)


class TestStaggered(unittest.IsolatedAsyncioTestCase):
    @staticmethod
    async def waiting_coroutine(return_value, wait_seconds, success):
        await asyncio.sleep(wait_seconds)
        if success:
            return return_value
        raise RuntimeError(str(return_value))

    def get_waiting_coroutine_factory(self, return_value, wait_seconds, success):
        return functools.partial(self.waiting_coroutine, return_value, wait_seconds, success)

    async def test_single_success(self):
        winner_result, winner_idx, exceptions = await staggered_race(
            (self.get_waiting_coroutine_factory(0, 0.1, True),),
            0.1,
        )
        self.assertEqual(winner_result, 0)
        self.assertEqual(winner_idx, 0)
        self.assertEqual(len(exceptions), 1)
        self.assertIsNone(exceptions[0])

    async def test_single_fail(self):
        winner_result, winner_idx, exceptions = await staggered_race(
            (self.get_waiting_coroutine_factory(0, 0.1, False),),
            0.1,
        )
        self.assertEqual(winner_result, None)
        self.assertEqual(winner_idx, None)
        self.assertEqual(len(exceptions), 1)
        self.assertIsInstance(exceptions[0], RuntimeError)

    async def test_first_win(self):
        winner_result, winner_idx, exceptions = await staggered_race(
            (
                self.get_waiting_coroutine_factory(0, 0.2, True),
                self.get_waiting_coroutine_factory(1, 0.2, True),
            ),
            0.1,
        )
        self.assertEqual(winner_result, 0)
        self.assertEqual(winner_idx, 0)
        self.assertEqual(len(exceptions), 2)
        self.assertIsNone(exceptions[0])
        self.assertIsInstance(exceptions[1], asyncio.CancelledError)

    async def test_second_win(self):
        winner_result, winner_idx, exceptions = await staggered_race(
            (
                self.get_waiting_coroutine_factory(0, 0.3, True),
                self.get_waiting_coroutine_factory(1, 0.1, True),
            ),
            0.1,
        )
        self.assertEqual(winner_result, 1)
        self.assertEqual(winner_idx, 1)
        self.assertEqual(len(exceptions), 2)
        self.assertIsInstance(exceptions[0], asyncio.CancelledError)
        self.assertIsNone(exceptions[1])

    async def test_first_fail(self):
        winner_result, winner_idx, exceptions = await staggered_race(
            (
                self.get_waiting_coroutine_factory(0, 0.2, False),
                self.get_waiting_coroutine_factory(1, 0.2, True),
            ),
            0.1,
        )
        self.assertEqual(winner_result, 1)
        self.assertEqual(winner_idx, 1)
        self.assertEqual(len(exceptions), 2)
        self.assertIsInstance(exceptions[0], RuntimeError)
        self.assertIsNone(exceptions[1])

    async def test_second_fail(self):
        winner_result, winner_idx, exceptions = await staggered_race(
            (
                self.get_waiting_coroutine_factory(0, 0.2, True),
                self.get_waiting_coroutine_factory(1, 0, False),
            ),
            0.1,
        )
        self.assertEqual(winner_result, 0)
        self.assertEqual(winner_idx, 0)
        self.assertEqual(len(exceptions), 2)
        self.assertIsNone(exceptions[0])
        self.assertIsInstance(exceptions[1], RuntimeError)

    async def test_simultaneous_success_fail(self):
        # There's a potential race condition here:
        # https://github.com/python/cpython/issues/86296
        # As with any race condition, it can be difficult to reproduce.
        # This test may not fail every time.
        for i in range(201):
            time_unit = 0.0001 * i
            winner_result, winner_idx, exceptions = await staggered_race(
                (
                    self.get_waiting_coroutine_factory(0, time_unit*2, True),
                    self.get_waiting_coroutine_factory(1, time_unit, False),
                    self.get_waiting_coroutine_factory(2, 0.05, True)
                ),
                time_unit,
            )
            self.assertEqual(winner_result, 0)
            self.assertEqual(winner_idx, 0)
