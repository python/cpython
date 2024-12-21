import sys
sys.modules["_asyncio"] = None

import asyncio
from test.support import threading_helper
from unittest import TestCase
from threading import Thread

threading_helper.requires_working_threading(module=True)

class TestFreeThreading(TestCase):
    def test_all_tasks_race(self) -> None:
        async def task():
            await asyncio.sleep(0)

        async def main():
            async with asyncio.TaskGroup() as tg:
                for _ in range(100):
                    tg.create_task(task())
                    asyncio.all_tasks()

        threads = []
        for _ in range(10):
            thread = Thread(target=lambda: asyncio.run(main()))
            threads.append(thread)

        with threading_helper.start_threads(threads):
            pass

