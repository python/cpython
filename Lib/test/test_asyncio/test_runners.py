import asyncio
import unittest

from unittest import mock
from . import utils as test_utils


class TestPolicy(asyncio.AbstractEventLoopPolicy):

    def __init__(self, loop_factory):
        self.loop_factory = loop_factory
        self.loop = None

    def get_event_loop(self):
        # shouldn't ever be called by asyncio.run()
        raise RuntimeError

    def new_event_loop(self):
        return self.loop_factory()

    def set_event_loop(self, loop):
        if loop is not None:
            # we want to check if the loop is closed
            # in BaseTest.tearDown
            self.loop = loop


class BaseTest(unittest.TestCase):

    def new_loop(self):
        loop = asyncio.BaseEventLoop()
        loop._process_events = mock.Mock()
        loop._selector = mock.Mock()
        loop._selector.select.return_value = ()
        loop.shutdown_ag_run = False

        async def shutdown_asyncgens():
            loop.shutdown_ag_run = True
        loop.shutdown_asyncgens = shutdown_asyncgens

        return loop

    def setUp(self):
        super().setUp()

        policy = TestPolicy(self.new_loop)
        asyncio.set_event_loop_policy(policy)

    def tearDown(self):
        policy = asyncio.get_event_loop_policy()
        if policy.loop is not None:
            self.assertTrue(policy.loop.is_closed())
            self.assertTrue(policy.loop.shutdown_ag_run)

        asyncio.set_event_loop_policy(None)
        super().tearDown()


class RunTests(BaseTest):

    def test_asyncio_run_return(self):
        async def main():
            await asyncio.sleep(0)
            return 42

        self.assertEqual(asyncio.run(main()), 42)

    def test_asyncio_run_raises(self):
        async def main():
            await asyncio.sleep(0)
            raise ValueError('spam')

        with self.assertRaisesRegex(ValueError, 'spam'):
            asyncio.run(main())

    def test_asyncio_run_only_coro(self):
        for o in {1, lambda: None}:
            with self.subTest(obj=o), \
                    self.assertRaisesRegex(ValueError,
                                           'a coroutine was expected'):
                asyncio.run(o)

    def test_asyncio_run_debug(self):
        async def main(expected):
            loop = asyncio.get_event_loop()
            self.assertIs(loop.get_debug(), expected)

        asyncio.run(main(False))
        asyncio.run(main(True), debug=True)

    def test_asyncio_run_from_running_loop(self):
        async def main():
            coro = main()
            try:
                asyncio.run(coro)
            finally:
                coro.close()  # Suppress ResourceWarning

        with self.assertRaisesRegex(RuntimeError,
                                    'cannot be called from a running'):
            asyncio.run(main())

    def test_asyncio_run_cancels_hanging_tasks(self):
        lo_task = None

        async def leftover():
            await asyncio.sleep(0.1)

        async def main():
            nonlocal lo_task
            lo_task = asyncio.create_task(leftover())
            return 123

        self.assertEqual(asyncio.run(main()), 123)
        self.assertTrue(lo_task.done())

    def test_asyncio_run_reports_hanging_tasks_errors(self):
        lo_task = None
        call_exc_handler_mock = mock.Mock()

        async def leftover():
            try:
                await asyncio.sleep(0.1)
            except asyncio.CancelledError:
                1 / 0

        async def main():
            loop = asyncio.get_running_loop()
            loop.call_exception_handler = call_exc_handler_mock

            nonlocal lo_task
            lo_task = asyncio.create_task(leftover())
            return 123

        self.assertEqual(asyncio.run(main()), 123)
        self.assertTrue(lo_task.done())

        call_exc_handler_mock.assert_called_with({
            'message': test_utils.MockPattern(r'asyncio.run.*shutdown'),
            'task': lo_task,
            'exception': test_utils.MockInstanceOf(ZeroDivisionError)
        })

    def test_asyncio_run_closes_gens_after_hanging_tasks_errors(self):
        spinner = None
        lazyboy = None

        class FancyExit(Exception):
            pass

        async def fidget():
            while True:
                yield 1
                await asyncio.sleep(1)

        async def spin():
            nonlocal spinner
            spinner = fidget()
            try:
                async for the_meaning_of_life in spinner:  # NoQA
                    pass
            except asyncio.CancelledError:
                1 / 0

        async def main():
            loop = asyncio.get_running_loop()
            loop.call_exception_handler = mock.Mock()

            nonlocal lazyboy
            lazyboy = asyncio.create_task(spin())
            raise FancyExit

        with self.assertRaises(FancyExit):
            asyncio.run(main())

        self.assertTrue(lazyboy.done())

        self.assertIsNone(spinner.ag_frame)
        self.assertFalse(spinner.ag_running)
