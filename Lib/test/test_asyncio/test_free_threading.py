import asyncio
import unittest
from threading import Thread
from unittest import TestCase

from test.support import threading_helper

threading_helper.requires_working_threading(module=True)

def tearDownModule():
    asyncio._set_event_loop_policy(None)


class TestFreeThreading:
    def test_all_tasks_race(self) -> None:
        async def main():
            loop = asyncio.get_running_loop()
            future = loop.create_future()

            async def coro():
                await future

            tasks = set()

            async with asyncio.TaskGroup() as tg:
                for _ in range(100):
                    tasks.add(tg.create_task(coro()))

                all_tasks = self.all_tasks(loop)
                self.assertEqual(len(all_tasks), 101)

                for task in all_tasks:
                    self.assertEqual(task.get_loop(), loop)
                    self.assertFalse(task.done())

                current = self.current_task()
                self.assertEqual(current.get_loop(), loop)
                self.assertSetEqual(all_tasks, tasks | {current})
                future.set_result(None)

        def runner():
            with asyncio.Runner() as runner:
                loop = runner.get_loop()
                loop.set_task_factory(self.factory)
                runner.run(main())

        threads = []

        for _ in range(10):
            thread = Thread(target=runner)
            threads.append(thread)

        with threading_helper.start_threads(threads):
            pass


class TestPyFreeThreading(TestFreeThreading, TestCase):
    all_tasks = staticmethod(asyncio.tasks._py_all_tasks)
    current_task = staticmethod(asyncio.tasks._py_current_task)

    def factory(self, loop, coro, context=None):
        return asyncio.tasks._PyTask(coro, loop=loop, context=context)


@unittest.skipUnless(hasattr(asyncio.tasks, "_c_all_tasks"), "requires _asyncio")
class TestCFreeThreading(TestFreeThreading, TestCase):
    all_tasks = staticmethod(getattr(asyncio.tasks, "_c_all_tasks", None))
    current_task = staticmethod(getattr(asyncio.tasks, "_c_current_task", None))

    def factory(self, loop, coro, context=None):
        return asyncio.tasks._CTask(coro, loop=loop, context=context)


class TestEagerPyFreeThreading(TestPyFreeThreading):
    def factory(self, loop, coro, context=None):
        return asyncio.tasks._PyTask(coro, loop=loop, context=context, eager_start=True)


@unittest.skipUnless(hasattr(asyncio.tasks, "_c_all_tasks"), "requires _asyncio")
class TestEagerCFreeThreading(TestCFreeThreading, TestCase):
    def factory(self, loop, coro, context=None):
        return asyncio.tasks._CTask(coro, loop=loop, context=context, eager_start=True)
