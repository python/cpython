import asyncio
import unittest
import time

def tearDownModule():
    asyncio.set_event_loop_policy(None)


class SlowTask:
    """ Task will run for this defined time, ignoring cancel requests """
    TASK_TIMEOUT = 0.2

    def __init__(self):
        self.exited = False

    async def run(self):
        exitat = time.monotonic() + self.TASK_TIMEOUT

        while True:
            tosleep = exitat - time.monotonic()
            if tosleep <= 0:
                break

            try:
                await asyncio.sleep(tosleep)
            except asyncio.CancelledError:
                pass

        self.exited = True


class AsyncioWaitForTest(unittest.TestCase):

    async def atest_asyncio_wait_for_cancelled(self):
        t  = SlowTask()

        waitfortask = asyncio.create_task(asyncio.wait_for(t.run(), t.TASK_TIMEOUT * 2))
        await asyncio.sleep(0)
        waitfortask.cancel()
        await asyncio.wait({waitfortask})

        self.assertTrue(t.exited)

    def test_asyncio_wait_for_cancelled(self):
        asyncio.run(self.atest_asyncio_wait_for_cancelled())

    async def atest_asyncio_wait_for_timeout(self):
        t  = SlowTask()

        try:
            await asyncio.wait_for(t.run(), t.TASK_TIMEOUT / 2)
        except asyncio.TimeoutError:
            pass

        self.assertTrue(t.exited)

    def test_asyncio_wait_for_timeout(self):
        asyncio.run(self.atest_asyncio_wait_for_timeout())

    async def atest_asyncio_wait_for_hang(self, inner_steps, outer_steps):
        # bpo-42130: wait_for can swallow cancellation causing task to hang
        async def inner(steps):
            for _ in range(steps):
                await asyncio.sleep(0)

        finished = False

        async def wait_for_coro(steps):
            nonlocal finished
            await asyncio.wait_for(inner(steps), timeout=1)
            await asyncio.sleep(0.1)
            finished = True

        task = asyncio.create_task(wait_for_coro(inner_steps))
        for _ in range(outer_steps):
            await asyncio.sleep(0)
        assert not task.done()

        task.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await task
        self.assertFalse(finished)

    def test_asyncio_wait_for_hang(self):
        # Test with different number of inner/outer steps to weaken dependence
        # on implementation details
        for inner_steps in range(3):
            for outer_steps in range(3):
                with self.subTest(inner_steps=inner_steps, outer_steps=outer_steps):
                    asyncio.run(self.atest_asyncio_wait_for_hang(inner_steps, outer_steps))


if __name__ == '__main__':
    unittest.main()
