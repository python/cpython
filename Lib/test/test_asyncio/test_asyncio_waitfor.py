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


if __name__ == '__main__':
    unittest.main()
