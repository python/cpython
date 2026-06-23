import asyncio
import threading
import unittest
from unittest import TestCase

from test.support import threading_helper


async def _forever():
    await asyncio.Event().wait()


@threading_helper.requires_working_threading()
class TestAllTasks(TestCase):
    def test_all_tasks_from_other_thread_includes_eager_tasks(self):
        # gh-152020: all_tasks() called from another thread used to drop
        # eager-started tasks on free-threaded builds.
        loop = asyncio.new_event_loop()

        async def setup():
            loop.set_task_factory(asyncio.eager_task_factory)
            loop.create_task(_forever(), name="EAGER")
            loop.set_task_factory(None)
            loop.create_task(_forever(), name="NORMAL")

        async def teardown():
            tasks = [t for t in asyncio.all_tasks()
                     if t is not asyncio.current_task()]
            for t in tasks:
                t.cancel()
            await asyncio.gather(*tasks, return_exceptions=True)

        thread = threading.Thread(target=loop.run_forever)
        thread.start()
        try:
            asyncio.run_coroutine_threadsafe(setup(), loop).result()
            names = {t.get_name() for t in asyncio.all_tasks(loop)}
            self.assertIn("NORMAL", names)
            self.assertIn("EAGER", names)
        finally:
            asyncio.run_coroutine_threadsafe(teardown(), loop).result()
            loop.call_soon_threadsafe(loop.stop)
            thread.join()
            loop.close()


if __name__ == "__main__":
    unittest.main()
